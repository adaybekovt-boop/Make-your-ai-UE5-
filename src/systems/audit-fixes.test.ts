import { readFileSync } from 'node:fs'
import { describe, expect, it, vi } from 'vitest'
import {
  COMPLAINT_DAILY_CHANCE,
  CONTRACT_BREACH_REPUTATION,
  CONTRACT_DIRTY_DAYS,
  CONTRACT_NO_CHEAT_DAYS,
  COURT_DAILY_CHANCE,
  DATA_LOT_OFFICIAL,
  HIRE_COST,
  OPEN_SOURCE_IQ_THRESHOLD,
  RANDOM_EVENT_DAILY_CHANCE,
  SEASON_SUMMER_MULT,
  SUMMER_START_DAY,
} from './config'
import { gameClock, gameDay } from './calendar'
import { acceptContract, closeCompletedContract, greyContractRiskActive } from './contracts'
import { dailyComplaintRoll, dailyCourtRoll, dailyRandomEvent } from './events'
import { CHIP_SHOPS } from './city'
import { chipPriceForDay, seasonalityMult, userCapacity } from './market'
import { MAX_PENDING_EQUIPMENT_ORDERS } from './procurement'
import { createInitialGame } from './simulation'
import { hireEmployee } from './team'
import { withInstalledServers } from './serverGrid'
import type { ActionResult, GameState, Rng } from './types'
import {
  buyCompanyLocation,
  buyCompanyOffice,
  buyModelData,
  createCompanyGame,
  hireCompanyEmployee,
  orderCompanyEquipment,
  setQuantization,
} from './models'
import { applyModel, expectedCompanyCash } from './models/state'
import type { CompanyActionResult, CompanyState } from './models'
import { decodeSave, makeSaveEnvelope } from '../persistence/saves'
import { makeCompanySave, validateCompanyState } from '../persistence/companySaves'
import { expectedCompetitorGrowth } from './competitor'

function legacyOk(result: ActionResult): GameState {
  if (!result.ok) throw new Error(result.error)
  return result.state
}
function companyOk(result: CompanyActionResult): CompanyState {
  if (!result.ok) throw new Error(result.error)
  return result.state
}
function pending(state: GameState): GameState {
  return {
    ...state,
    contracts: {
      ...state.contracts,
      pending: {
        id: 1,
        clientName: 'Audit Client',
        officialPayout: 10_000,
        greyPayout: 18_000,
        enterprise: false,
        enterprisePayout: 30_000,
        expiresDay: 99,
      },
    },
  }
}
function grey(state = createInitialGame()): GameState {
  return legacyOk(acceptContract(pending(state), 'grey'))
}
function official(state = createInitialGame()): GameState {
  return legacyOk(acceptContract(pending({ ...state, reputation: 80 }), 'official'))
}
function seq(values: number[]): Rng {
  let index = 0
  return () => values[index++] ?? 0.99
}
function fundedCompany(): CompanyState {
  const state = createCompanyGame('flagship')
  return { ...state, startingCapital: 1_000_000, company: { ...state.company, cash: 1_000_000 } }
}

describe('audit L1 — finite GMI ad boost', () => {
  it('applies only before adBoostUntil and disappears exactly at expiry', () => {
    const base = createInitialGame()
    const economy = { effectiveCompute: 10 }
    const without = userCapacity({ ...base, benchmark: { ...base.benchmark, adBoostUntil: null } }, economy)
    const before = userCapacity({ ...base, elapsedGameHours: 9.999, benchmark: { ...base.benchmark, adBoostUntil: 10 } }, economy)
    const expired = userCapacity({ ...base, elapsedGameHours: 10, benchmark: { ...base.benchmark, adBoostUntil: 10 } }, economy)
    expect(before).toBeGreaterThan(without)
    expect(expired).toBeCloseTo(without)
  })

  it('save/load preserves the absolute expiry instead of extending it', () => {
    const state = createInitialGame()
    state.elapsedGameHours = 7
    state.benchmark.adBoostUntil = 10
    const restored = decodeSave(JSON.parse(JSON.stringify(makeSaveEnvelope(state)))).game
    expect(restored.elapsedGameHours).toBe(7)
    expect(restored.benchmark.adBoostUntil).toBe(10)
    const before = userCapacity(restored, { effectiveCompute: 10 })
    restored.elapsedGameHours = 10
    expect(userCapacity(restored, { effectiveCompute: 10 })).toBeLessThan(before)
  })

  it('changing quantization invalidates the boost rather than carrying it to a new model version', () => {
    let state = createCompanyGame('flagship')
    state.models[0].benchmark.adBoostUntil = 50
    state = companyOk(setQuantization(state, state.models[0].id, 1))
    expect(state.models[0].benchmark.adBoostUntil).toBeNull()
  })
})

describe('audit L2 — grey contract risk lifecycle', () => {
  it('keeps the ordinary contract clean while grey pays more, costs reputation and opens a finite risk window', () => {
    const clean = official()
    const risky = grey()
    expect(clean.contracts.active?.kind).toBe('official')
    expect(clean.market.dirtyRiskUntil).toBeNull()
    expect(risky.contracts.active?.kind).toBe('grey')
    expect(risky.contracts.active!.payout).toBeGreaterThan(clean.contracts.active!.payout)
    expect(risky.reputation).toBeLessThan(createInitialGame().reputation)
    expect(risky.market.dirtyRiskUntil).toBe(CONTRACT_DIRTY_DAYS * 24)
    expect(greyContractRiskActive(risky)).toBe(true)
  })

  it('raises court, complaint and daily-event pressure only while grey risk is active', () => {
    const legal = official()
    const risky = grey()
    const courtLegal = dailyCourtRoll(legal, () => COURT_DAILY_CHANCE * 1.5)
    const courtGrey = dailyCourtRoll(risky, () => COURT_DAILY_CHANCE * 1.5)
    expect(courtLegal.cash).toBe(legal.cash)
    expect(courtGrey.cash).toBeLessThan(risky.cash)

    const overloaded = (state: GameState): GameState => ({
      ...state,
      locations: state.locations.map((location, index) => index === 0 ? { ...location, owned: true, servers: 100 } : location),
    })
    const complaintLegal = dailyComplaintRoll(overloaded(legal), () => COMPLAINT_DAILY_CHANCE * 1.25)
    const complaintGrey = dailyComplaintRoll(overloaded(risky), () => COMPLAINT_DAILY_CHANCE * 1.25)
    expect(complaintLegal.pendingNotices.length).toBe(legal.pendingNotices.length)
    expect(complaintGrey.pendingNotices.length).toBeGreaterThan(risky.pendingNotices.length)

    const eventLegal = dailyRandomEvent(legal, seq([RANDOM_EVENT_DAILY_CHANCE * 1.25, 0.45]))
    const eventGrey = dailyRandomEvent(risky, seq([RANDOM_EVENT_DAILY_CHANCE * 1.25, 0.45]))
    expect(eventLegal.reputation).toBe(legal.reputation)
    expect(eventGrey.reputation).toBeLessThan(risky.reputation)
  })

  it('ends the grey risk and closes the obligation after its deadline', () => {
    let state = grey()
    state = { ...state, elapsedGameHours: 119 }
    expect(greyContractRiskActive(state)).toBe(true)
    state = { ...state, elapsedGameHours: 136 }
    state = closeCompletedContract(state)
    expect(state.contracts.active).toBeNull()
    expect(state.market.dirtyRiskUntil).toBeNull()
    expect(greyContractRiskActive(state)).toBe(false)
  })

  it('persists and restores the exact grey risk deadline', () => {
    const state = grey()
    const restored = decodeSave(JSON.parse(JSON.stringify(makeSaveEnvelope(state)))).game
    expect(restored.market.dirtyRiskUntil).toBe(state.market.dirtyRiskUntil)
    expect(greyContractRiskActive(restored)).toBe(true)
  })

  it('turns an ignored official next-training obligation into a breach after its finite due date', () => {
    let state = official()
    expect(state.contracts.active?.noCheatUntilDay).toBe(gameDay(state) + CONTRACT_NO_CHEAT_DAYS)
    const beforeRep = state.reputation
    state = { ...state, elapsedGameHours: 256 }
    state = closeCompletedContract(state)
    expect(state.contracts.active).toBeNull()
    expect(state.reputation).toBe(beforeRep - CONTRACT_BREACH_REPUTATION)
    expect(state.pendingNotices.some((notice) => notice.includes('не выполнено в срок'))).toBe(true)
  })
})

describe('audit L3 — accounting invariant', () => {
  it('books hiring and direct datasets as expenses', () => {
    const start = createInitialGame()
    const hired = legacyOk(hireEmployee(start, 'engineer'))
    expect(start.cash - hired.cash).toBe(HIRE_COST.engineer)
    expect(hired.totalExpenses - start.totalExpenses).toBe(HIRE_COST.engineer)

    const company = createCompanyGame('flagship')
    const bought = companyOk(buyModelData(company, company.models[0].id, 'official'))
    expect(company.company.cash - bought.company.cash).toBe(DATA_LOT_OFFICIAL.price)
    expect(bought.company.totalExpenses - company.company.totalExpenses).toBe(DATA_LOT_OFFICIAL.price)
  })

  it('keeps cash = startingCapital + revenue - expenses - capex across mixed spending commands and persistence', () => {
    let state = fundedCompany()
    state = companyOk(buyCompanyLocation(state, 'garage'))
    expect(state.company.cash).toBeCloseTo(expectedCompanyCash(state))
    state = companyOk(hireCompanyEmployee(state, 'engineer'))
    expect(state.company.cash).toBeCloseTo(expectedCompanyCash(state))
    state = companyOk(buyModelData(state, state.models[0].id, 'official'))
    expect(state.company.cash).toBeCloseTo(expectedCompanyCash(state))
    state = companyOk(orderCompanyEquipment(state, { locationId: 'garage', kind: 'chassis', item: 'rack-basic', channel: 'official', qty: 1 }))
    expect(state.company.cash).toBeCloseTo(expectedCompanyCash(state))
    state = companyOk(buyCompanyOffice(state))
    expect(state.company.cash).toBeCloseTo(expectedCompanyCash(state))
    expect(() => makeCompanySave(state)).not.toThrow()
  })

  it('rejects a save where cash is changed without a matching accounting bucket', () => {
    const state = fundedCompany()
    state.company.cash -= 1
    expect(() => validateCompanyState(state)).toThrow('финансовый инвариант')
  })
})

describe('audit L4 — one game calendar starting at 08:00', () => {
  it('covers Day 1 08:00, Day 1 23:59, midnight and Day 2 08:00', () => {
    expect(gameClock(0)).toMatchObject({ day: 1, hour: 8, minute: 0, time: '08:00' })
    expect(gameClock(15 + 59 / 60 + 1e-8)).toMatchObject({ day: 1, hour: 23, minute: 59, time: '23:59' })
    expect(gameClock(16)).toMatchObject({ day: 2, hour: 0, minute: 0, time: '00:00' })
    expect(gameClock(24)).toMatchObject({ day: 2, hour: 8, minute: 0, time: '08:00' })
  })

  it('uses the same day for hiring and seasonal tariff boundaries', () => {
    let dayOne = createInitialGame()
    dayOne.elapsedGameHours = 15.9
    expect(legacyOk(hireEmployee(dayOne, 'engineer')).team.employees[0].hiredDay).toBe(1)
    let dayTwo = createInitialGame()
    dayTwo.elapsedGameHours = 16
    expect(legacyOk(hireEmployee(dayTwo, 'engineer')).team.employees[0].hiredDay).toBe(2)

    const summer = createInitialGame()
    summer.elapsedGameHours = SUMMER_START_DAY * 24 - 8
    expect(seasonalityMult(summer)).toBe(SEASON_SUMMER_MULT)
  })
})

describe('other confirmed audit regressions', () => {
  it('removes the exact server selected by the fire roll', () => {
    const base = createInitialGame()
    const garage = withInstalledServers({ ...base.locations[0], owned: true }, [
      { id: 'server-1', chip: 'consumer-gpu', chassis: 'rack-basic', overclock: 1, gridPosition: { row: 0, col: 0 } },
      { id: 'server-2', chip: 'pro-gpu', chassis: 'rack-basic', overclock: 1, gridPosition: { row: 0, col: 1 } },
    ], 2)
    const state = { ...base, locations: base.locations.map((location, index) => index === 0 ? garage : location) }
    const burned = dailyRandomEvent(state, seq([0, 0, 0.99]))
    const ids = burned.locations[0].installedServers!.map((server) => server.id)
    expect(ids).toContain('server-1')
    expect(ids).not.toContain('server-2')
  })

  it('uses distinct chip phases and a named pending-order limit', () => {
    expect(MAX_PENDING_EQUIPMENT_ORDERS).toBe(198)
    const source = readFileSync(new URL('./market.ts', import.meta.url), 'utf8')
    expect(source).toContain("flagship: 3 * Math.PI / 2")
    expect(chipPriceForDay('accelerator', 7, () => 0.5).price).toBeGreaterThan(0)
    expect(chipPriceForDay('flagship', 7, () => 0.5).price).toBeGreaterThan(0)
  })

  it('keeps the store copy aligned with delayed warehouse delivery', () => {
    expect(CHIP_SHOPS.every((shop) => shop.description.includes('достав'))).toBe(true)
    expect(CHIP_SHOPS.every((shop) => shop.description.includes('склад'))).toBe(true)
  })

  it('does not use global Math.random for the competitor forecast rendered by the testing screen', () => {
    const state = createInitialGame()
    const random = vi.spyOn(Math, 'random')
    expect(expectedCompetitorGrowth(state)).toBe(expectedCompetitorGrowth(state))
    expect(random).not.toHaveBeenCalled()
    random.mockRestore()
    const screen = readFileSync(new URL('../ui/TestingScreen.tsx', import.meta.url), 'utf8')
    expect(screen).not.toContain('Math.random()')
    expect(screen).toContain('const currentLast = stale ? null : last')
  })

  it('uses the configured employee limit instead of a duplicated UI literal', () => {
    const screen = readFileSync(new URL('../ui/TrainingScreen.tsx', import.meta.url), 'utf8')
    expect(screen).toContain('team.employees.length >= MAX_EMPLOYEES')
    expect(screen).not.toContain('team.employees.length >= 12')
  })

  it('guards open-source transitions below the threshold and after the decision', () => {
    let state = createCompanyGame('flagship')
    const id = state.models[0].id
    const choose = (openSource: boolean) => (view: GameState): ActionResult => ({ ok: true, state: { ...view, model: { ...view.model, openSourceChosen: true, openSource } } })
    state.models[0].state.iq = OPEN_SOURCE_IQ_THRESHOLD - 1
    expect(applyModel(state, id, choose(true)).ok).toBe(false)
    state.models[0].state.iq = OPEN_SOURCE_IQ_THRESHOLD
    state = companyOk(applyModel(state, id, choose(true)))
    expect(state.models[0].state.openSource).toBe(true)
    expect(applyModel(state, id, choose(false)).ok).toBe(false)
  })

  it('pins UE source fixes without pretending they are an Editor runtime test', () => {
    const camera = readFileSync(new URL('../../Unreal/MakeYourAI/Source/MakeYourAI/Private/World/MaiCameraPawn.cpp', import.meta.url), 'utf8')
    expect(camera).toContain('FMath::Clamp(Desired.Z,250.,150000.)')
    expect(camera).not.toContain('if(Next.Z>=250&&Next.Z<=150000)')

    const scaffold = readFileSync(new URL('../../Unreal/MakeYourAI/Source/MakeYourAI/Private/World/MaiScaffoldWorld.cpp', import.meta.url), 'utf8')
    expect(scaffold).toContain('MarkerPositions.Find(TEXT("garage"))')
    expect(scaffold).toContain('skipping demo NPC')

    const subsystem = readFileSync(new URL('../../Unreal/MakeYourAI/Source/MakeYourAI/Private/Rules/MaiRulesSubsystem.cpp', import.meta.url), 'utf8')
    const presentation = readFileSync(new URL('../../Unreal/MakeYourAI/Source/MakeYourAI/Private/Rules/MaiRulesPresentation.cpp', import.meta.url), 'utf8')
    expect(subsystem).toContain('SaveSlot(TEXT("autosave"))')
    expect(presentation).toContain('SaveSlot(TEXT("campaign"))')

    const view = readFileSync(new URL('../../Unreal/Rules/view.ts', import.meta.url), 'utf8')
    expect(view).toContain('gameClock(v.atHours)')
    expect(view).toContain("kind==='kit'?text('quantity-fixed','Количество: 1 комплект')")

    const kernel = readFileSync(new URL('../../Unreal/Rules/kernel.ts', import.meta.url), 'utf8')
    expect(kernel).toContain('selectedId: ui.location')
    expect(kernel).toContain('...decoded.game.company.regionLocations')
  })
})
