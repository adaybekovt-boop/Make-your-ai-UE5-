import {
  CONTRACT_BREACH_REPUTATION,
  CONTRACT_DIRTY_DAYS,
  CONTRACT_ENTERPRISE_PAYOUT,
  CONTRACT_EXPIRY_DAYS,
  CONTRACT_GREY_MULTIPLIER,
  CONTRACT_INTERVAL_MAX_DAYS,
  CONTRACT_INTERVAL_MIN_DAYS,
  CONTRACT_NO_CHEAT_DAYS,
  CONTRACT_OFFICIAL_PAYOUT,
  CONTRACT_OFFICIAL_REPUTATION_MIN,
} from './config'
import { gameDay, pushNotice } from './market'
import { changeReputation } from './reputation'
import type { ActionResult, ContractKind, ContractOffer, GameState, Rng } from './types'

export const GREY_CONTRACT_REPUTATION_HIT = 4
export const GREY_CONTRACT_COURT_RISK_MULT = 2
export const GREY_CONTRACT_COMPLAINT_RISK_MULT = 1.5
export const GREY_CONTRACT_EVENT_RISK_MULT = 1.5

const FICTIONAL_CLIENTS = [
  'Аврора Логистика',
  'Городская библиотека №4',
  'Пекарня «Тесто»',
  'Метеослужба «Ветер»',
  'Таксопарк «Вольт»',
  'Ателье «Стежка»',
  'Депо «Северные ворота»',
  'Студия «Тихий кадр»',
]

function intervalDays(rng: Rng): number {
  const span = CONTRACT_INTERVAL_MAX_DAYS - CONTRACT_INTERVAL_MIN_DAYS
  return CONTRACT_INTERVAL_MIN_DAYS + Math.floor(rng() * (span + 1))
}

export function nextOfferDay(state: GameState, rng: Rng): number {
  return gameDay(state) + intervalDays(rng)
}

export function greyContractRiskActive(state: GameState): boolean {
  return state.contracts.active?.kind === 'grey' && state.market.dirtyRiskUntil !== null && state.elapsedGameHours < state.market.dirtyRiskUntil
}

/**
 * Release the slot only after the data obligation is settled AND all time windows end.
 * Official next-training obligations use the no-cheat deadline as their finite due date;
 * old saves therefore cannot leave a contract stuck forever.
 */
export function closeCompletedContract(state: GameState): GameState {
  let next = state
  let active = next.contracts.active
  if (!active || next.ending) return next
  const day = gameDay(next)

  if (active.requiresOfficialData && active.fulfilled === null) {
    if (active.noCheatUntilDay !== null && day <= active.noCheatUntilDay) return next
    next = changeReputation(next, -CONTRACT_BREACH_REPUTATION)
    active = { ...active, fulfilled: false }
    next = pushNotice(
      { ...next, contracts: { ...next.contracts, active } },
      `Контракт «${active.clientName}» нарушен: обязательное официальное обучение не выполнено в срок.`,
    )
  }

  if (active.noCheatUntilDay !== null && day <= active.noCheatUntilDay) return next
  if (active.dirtyUntilDay !== null && day <= active.dirtyUntilDay) return next
  if (active.kind === 'grey' && next.market.dirtyRiskUntil !== null && next.elapsedGameHours < next.market.dirtyRiskUntil) return next

  const cleared = active.kind === 'grey'
    ? { ...next, market: { ...next.market, dirtyRiskUntil: null }, contracts: { ...next.contracts, active: null } }
    : { ...next, contracts: { ...next.contracts, active: null } }
  return pushNotice(cleared, `Контракт «${active.clientName}» закрыт. Компания может принимать новые предложения.`)
}

/** One offer at a time: the same deal in an official and a grey variant. */
export function maybeGenerateOffer(state: GameState, rng: Rng): GameState {
  if (state.ending || state.contracts.pending || state.contracts.active) return state
  if (gameDay(state) < state.contracts.nextOfferDay) return state

  const clientName = FICTIONAL_CLIENTS[Math.floor(rng() * FICTIONAL_CLIENTS.length)]
  const offer: ContractOffer = {
    id: state.contracts.seq + 1,
    clientName,
    officialPayout: CONTRACT_OFFICIAL_PAYOUT,
    greyPayout: Math.round(CONTRACT_OFFICIAL_PAYOUT * CONTRACT_GREY_MULTIPLIER),
    enterprise: state.model.tech.includes('context') && rng() < 0.25,
    enterprisePayout: CONTRACT_ENTERPRISE_PAYOUT,
    expiresDay: gameDay(state) + CONTRACT_EXPIRY_DAYS,
  }
  const next: GameState = {
    ...state,
    contracts: {
      ...state.contracts,
      seq: offer.id,
      pending: offer,
      nextOfferDay: nextOfferDay(state, rng),
    },
  }
  return pushNotice(next, `Новое предложение: контракт от «${offer.clientName}». Решение ждёт на панели.`)
}

export function offerVariantAvailable(state: GameState, variant: ContractKind): boolean {
  if (variant === 'official') return state.reputation >= CONTRACT_OFFICIAL_REPUTATION_MIN
  return true
}

export function clearExpiredOffer(state: GameState): GameState {
  const pending = state.contracts.pending
  if (!pending || gameDay(state) <= pending.expiresDay) return state
  return pushNotice(
    { ...state, contracts: { ...state.contracts, pending: null } },
    `Предложение от «${pending.clientName}» истекло без ответа.`,
  )
}

export function acceptContract(state: GameState, variant: ContractKind): ActionResult {
  const pending = state.contracts.pending
  if (!pending) return { ok: false, error: 'Активного предложения нет.' }
  if (state.ending) return { ok: false, error: 'Компания уже продана.' }
  if (state.contracts.active) return { ok: false, error: 'Сначала завершите действующий контракт.' }
  if (gameDay(state) > pending.expiresDay) return { ok: false, error: 'Срок предложения истёк.' }
  if (!offerVariantAvailable(state, variant)) {
    return { ok: false, error: 'Репутация слишком низка: клиент работает только по официальным правилам.' }
  }
  if (variant === 'enterprise' && !pending.enterprise) {
    return { ok: false, error: 'Этот клиент не предлагает энтерпрайз-формат.' }
  }

  const payout = variant === 'official' ? pending.officialPayout : variant === 'grey' ? pending.greyPayout : pending.enterprisePayout
  const active = {
    kind: variant,
    clientName: pending.clientName,
    payout,
    requiresOfficialData: variant === 'official',
    noCheatUntilDay: variant === 'official' ? gameDay(state) + CONTRACT_NO_CHEAT_DAYS : null,
    dirtyUntilDay: variant === 'grey' ? gameDay(state) + CONTRACT_DIRTY_DAYS : null,
    fulfilled: null,
  }
  let next: GameState = {
    ...state,
    cash: state.cash + payout,
    totalRevenue: state.totalRevenue + payout,
    contracts: { ...state.contracts, pending: null, active },
  }
  if (variant === 'grey') {
    next = changeReputation(
      { ...next, market: { ...next.market, dirtyRiskUntil: next.elapsedGameHours + CONTRACT_DIRTY_DAYS * 24 } },
      -GREY_CONTRACT_REPUTATION_HIT,
    )
  }
  const label = variant === 'official' ? 'официальный' : variant === 'grey' ? 'серый' : 'энтерпрайз'
  return {
    ok: true,
    state: pushNotice(next, `Подписан ${label} контракт с «${pending.clientName}»: выплата ${payout}.`),
  }
}

export function declineContract(state: GameState): GameState {
  const pending = state.contracts.pending
  if (!pending) return state
  return pushNotice(
    { ...state, contracts: { ...state.contracts, pending: null } },
    `Вы отказались от контракта «${pending.clientName}».`,
  )
}

export function activeContractLabel(state: GameState): string | null {
  const active = state.contracts.active
  if (!active) return null
  if (active.requiresOfficialData && active.fulfilled !== null) {
    const outcome = active.fulfilled ? 'Обязательство по обучению выполнено.' : 'Обязательство по обучению нарушено.'
    const restriction = active.noCheatUntilDay !== null && gameDay(state) <= active.noCheatUntilDay
      ? ` Без манипуляций с бенчмарками до конца дня ${active.noCheatUntilDay}.`
      : ' Ожидает закрытия на суточной границе.'
    return `Контракт «${active.clientName}»: ${outcome}${restriction}`
  }
  if (active.requiresOfficialData && active.fulfilled === null && active.noCheatUntilDay !== null) {
    return `Контракт «${active.clientName}»: следующее обучение — только официальные данные, срок до конца дня ${active.noCheatUntilDay}.`
  }
  if (active.kind === 'grey' && state.market.dirtyRiskUntil !== null) {
    const hours = Math.max(0, Math.ceil(state.market.dirtyRiskUntil - state.elapsedGameHours))
    return `Серый контракт «${active.clientName}»: повышенный риск ещё ${hours} ч.`
  }
  return `Контракт с «${active.clientName}» в силе.`
}
