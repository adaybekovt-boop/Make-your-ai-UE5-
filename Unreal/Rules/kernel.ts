import { useGameStore } from '../../src/store/gameStore'
import { decodeCompanySave, makeCompanySave, validateCompanyState } from '../../src/persistence/companySaves'
import { companyView, companyLearnedIQ, companyUsers, portfolioEconomy } from '../../src/systems/models'
import { stageSave, storageSequence } from './nativePersistence'
import { makeView, initialUi, uiCommand, type HostView, type UiState } from './view'

const store = useGameStore
const actions = new Set([
  'beginSetup','cancelSetup','startNewGame','selectLocation','selectModel','sellAt','overclockAt','deployReserve',
  'purchaseOffice','purchaseCityTower','purchaseLocation','openProcurement','closeProcurement','orderEquipment','orderKit',
  'mountChip','mountChassis','togglePause','setSpeed','dismissNotice','buyDataLot','startTraining','setPersonality',
  'runBenchmark','togglePreparing','acceptContract','declineContract','hireEmployee','fireEmployee','toggleOverwork',
  'toggleAdvertising','setTokenPrice','toggleInsurance','toggleLicense','chooseOpenSource','unlockTech','attemptEspionage',
  'unlockRegion','acceptAcquisition','declineAcquisition','renameSelected','purchaseBase','setModelQuantization','setAllocations',
])
let rng = 1
let ui: UiState = initialUi()
let response: string | null = null
let inFlight = false
let nativeReserve = 0
let generation = 0
let reviewedLots = new Set<string>()
const random = () => {
  rng = (rng + 0x6D2B79F5) >>> 0
  let t = rng
  t = Math.imul(t ^ (t >>> 15), t | 1)
  t ^= t + Math.imul(t ^ (t >>> 7), t | 61)
  return ((t ^ (t >>> 14)) >>> 0) / 4294967296
}
// Same stochastic rules, with native restart-continuous RNG. Tests run the browser
// reference with this identical generator, not a favourable constant random value.
Math.random = random
;(globalThis as unknown as { __maiReservedCompute: () => number }).__maiReservedCompute = () => nativeReserve

function state() {
  const current = store.getState()
  return { company: current.company, game: current.game, economy: portfolioEconomy(current.company),
    learnedIQ: companyLearnedIQ(current.company), users: companyUsers(current.company),
    phase: current.phase, selectedId: current.selectedId, selectedModelId: current.selectedModelId,
    notice: current.notice, eventLog: current.eventLog, procurement: current.procurement,
    generation, ui, storageSequence: storageSequence(), rng, reservedCompute: nativeReserve }
}
function boundedDelta(value: unknown, name: string) {
  if (typeof value !== 'number' || !Number.isFinite(value) || Math.abs(value) > 1e9) throw new Error(`Invalid native ${name}`)
  return value
}
function plainObject(value: unknown): asserts value is Record<string, unknown> {
  if (!value || typeof value !== 'object' || Array.isArray(value)) throw new Error('Object expected')
}
async function execute(request: Record<string, unknown>) {
  const method = request.method
  if (method === 'boot') {
    if (typeof request.seed !== 'number' || !Number.isSafeInteger(request.seed) || request.seed < 1 || request.seed > 0xffffffff) throw new Error('Invalid session seed')
    rng = request.seed >>> 0
    await store.getState().initialize()
    return state()
  }
  if (method === 'sync-review') {
    if (!Array.isArray(request.verified) || request.verified.length > 512 || request.verified.some(v => typeof v !== 'string' || !/^model-[1-9]\d*:[1-9]\d*$/.test(v))) throw new Error('Invalid review approvals')
    const reserved = request.reservedCompute
    if (typeof reserved !== 'number' || !Number.isFinite(reserved) || reserved < 0 || reserved > 1e6) throw new Error('Invalid review reservation')
    reviewedLots = new Set(request.verified as string[]); nativeReserve = reserved
    return state()
  }
  if (method === 'state') return state()
  if (method === 'view') return makeView(store.getState(), ui, request.host as unknown as HostView)
  if (method === 'tick') {
    if (typeof request.seconds !== 'number' || !Number.isFinite(request.seconds) || request.seconds < 0 || request.seconds > 2) throw new Error('Native tick must be 0..2 real seconds')
    const reserved = request.reservedCompute ?? 0
    if (typeof reserved !== 'number' || !Number.isFinite(reserved) || reserved < 0 || reserved > 1e6) throw new Error('Invalid compute reservation')
    nativeReserve = reserved
    store.getState().tick(request.seconds)
    validateCompanyState(store.getState().company)
    return state()
  }
  if (method === 'save') {
    if (typeof request.savedAt !== 'string') throw new Error('Host save timestamp required')
    return { nativeRulesVersion: 1, browser: makeCompanySave(store.getState().company, request.savedAt),
      rng, selectedModelId: store.getState().selectedModelId, selectedId: store.getState().selectedId,
      // The original event log was UI-only. Native saves retain it as an explicitly separate extension.
      eventLog: store.getState().eventLog, phase: store.getState().phase, ui }
  }
  if (method === 'validate' || method === 'load') {
    plainObject(request.payload)
    const raw = request.payload
    const native = Object.hasOwn(raw, 'nativeRulesVersion')
    if (native && raw.nativeRulesVersion !== 1) throw new Error('Unsupported native rules save')
    const decoded = decodeCompanySave(native ? raw.browser : raw)
    const restoredRng = native ? raw.rng : rng
    if (typeof restoredRng !== 'number' || !Number.isSafeInteger(restoredRng) || restoredRng < 0 || restoredRng > 0xffffffff) throw new Error('Invalid saved RNG')
    const selectedModelId = native ? raw.selectedModelId : decoded.game.models[0].id
    if (!decoded.game.models.some(m => m.id === selectedModelId)) throw new Error('Invalid saved model selection')
    const selectedId = native ? raw.selectedId : 'garage'
    if (!decoded.game.company.locations.some(l => l.id === selectedId)) throw new Error('Invalid saved location selection')
    const eventLog = native ? raw.eventLog : []
    if (!Array.isArray(eventLog) || eventLog.length > 40 || eventLog.some(v => !v || typeof v.message !== 'string' || v.message.length > 4096 || !['success','error','info'].includes(v.kind) || !Number.isFinite(v.atHours) || v.atHours < 0 || v.atHours > decoded.game.company.elapsedGameHours || !Number.isSafeInteger(v.id) || v.id < 1)) throw new Error('Invalid saved journal')
    if (native && !['menu','setup','playing'].includes(String(raw.phase ?? 'playing'))) throw new Error('Invalid saved phase')
    if (native && raw.ui !== undefined && JSON.stringify(raw.ui).length > 16384) throw new Error('Invalid saved UI')
    if (native && raw.ui && typeof raw.ui === 'object') { const u = raw.ui as Record<string, unknown>; let candidate = initialUi();
      if (u.location !== undefined && (typeof u.location !== 'string' || ![...decoded.game.company.locations,...decoded.game.company.regionLocations].some(l=>l.id===u.location))) throw new Error('Invalid saved UI location')
      if (u.fields && typeof u.fields === 'object') for (const [k,v] of Object.entries(u.fields)) candidate = uiCommand(candidate,'field',[k,v],store.getState()).ui
      if (u.domain !== undefined) candidate = uiCommand(candidate,'field',['domain',u.domain],store.getState()).ui
      if (typeof u.page === 'string') uiCommand(candidate,'page',[u.page],store.getState())
    }
    if (method === 'validate') return { ok: true, browserSchema: decoded.schemaVersion, ended: decoded.game.company.ending !== null }
    stageSave(decoded)
    await store.getState().restore()
    if (!store.getState().storageEnabled) throw new Error('Restore rejected')
    store.setState({ selectedModelId: selectedModelId as never, selectedId: selectedId as never, eventLog })
    rng = restoredRng >>> 0; ui = initialUi();
    if (native && raw.ui && typeof raw.ui === 'object') {
      const saved = raw.ui as Record<string, unknown>
      if (saved.fields && typeof saved.fields === 'object') for (const [k,v] of Object.entries(saved.fields)) {
        if (typeof v !== 'string' || v.length > 512) throw new Error('Invalid saved input')
        ui = uiCommand(ui,'field',[k,v],store.getState()).ui
      }
      if (typeof saved.location === 'string') ui = uiCommand(ui,'location',[saved.location],store.getState()).ui
      if (typeof saved.page === 'string') ui = uiCommand(ui,'page',[saved.page],store.getState()).ui
      if (saved.domain !== undefined) ui = uiCommand(ui,'field',['domain',saved.domain],store.getState()).ui
    }
    if (native) store.setState({ phase: (raw.phase ?? 'playing') as 'menu'|'setup'|'playing' })
    nativeReserve = 0; reviewedLots.clear(); generation += 1
    return state()
  }
  if (method === 'native-ledger') {
    // Only the C++ extension adapter calls this: reviews/endings already present
    // in the UE campaign. UI actions cannot supply arbitrary ledger deltas.
    const s = store.getState(), c = s.company
    const cash = boundedDelta(request.cash ?? 0, 'cash'), expenses = boundedDelta(request.expenses ?? 0, 'expenses')
    const capex = boundedDelta(request.capex ?? 0, 'capex'), revenue = boundedDelta(request.revenue ?? 0, 'revenue')
    const startingCapital = boundedDelta(request.startingCapital ?? 0, 'startingCapital')
    const reputation = boundedDelta(request.reputation ?? 0, 'reputation')
    const next = { ...c, startingCapital: c.startingCapital + startingCapital, company: { ...c.company, cash: c.company.cash + cash,
      totalExpenses: c.company.totalExpenses + expenses, totalCapex: c.company.totalCapex + capex, totalRevenue: c.company.totalRevenue + revenue,
      reputation: Math.max(0, Math.min(100, c.company.reputation + reputation)) } }
    validateCompanyState(next)
    store.setState({ company: next, game: companyView(next) })
    return state()
  }
  if (method === 'command') {
    if (typeof request.action !== 'string') throw new Error('Action required')
    if (request.action.startsWith('ui:')) {
      const resolved = uiCommand(ui, request.action.slice(3), request.args, store.getState())
      ui = resolved.ui
      if (!resolved.action) return state()
      request = { ...request, action: resolved.action, args: resolved.args }
    }
    if (typeof request.action !== 'string' || !actions.has(request.action) || !Array.isArray(request.args) || request.args.length > 8) throw new Error('Unknown or invalid action')
    if (store.getState().company.company.ending && !['beginSetup','cancelSetup','startNewGame','dismissNotice'].includes(request.action)) throw new Error('Компания завершена. Начните новую игру.')
    if (request.action === 'startTraining') {
      const current = store.getState(), model = current.company.models.find(m => m.id === current.selectedModelId)!
      if (model.state.queue.some(l => !reviewedLots.has(`${model.id}:${l.id}`))) throw new Error('Unreviewed dataset: сначала завершите проверку данных.')
    }
    const f = (store.getState() as unknown as Record<string, unknown>)[request.action]
    if (typeof f !== 'function') throw new Error('Action unavailable')
    const beforeCompany = store.getState().company
    await f(...request.args)
    validateCompanyState(store.getState().company)
    if (request.action === 'startNewGame' && store.getState().company !== beforeCompany) { ui = initialUi(); reviewedLots.clear(); generation += 1 }
    return state()
  }
  throw new Error('Unknown native method')
}

export const MaiNative = {
  call(json: string) {
    if (inFlight) throw new Error('Concurrent native request')
    if (typeof json !== 'string' || json.length > 8 * 1024 * 1024) throw new Error('Native request too large')
    const before = store.getState(), beforeUi = ui, beforeRng = rng, beforeReserve = nativeReserve, beforeGeneration = generation, beforeReviewed = new Set(reviewedLots)
    response = null; inFlight = true
    void (async () => {
      try {
        const request: unknown = JSON.parse(json); plainObject(request)
        const value = await execute(request)
        const notice = store.getState().notice
        response = JSON.stringify({ ok: !(request.method === 'command' && notice?.kind === 'error' && notice.id !== before.notice?.id), value,
          error: request.method === 'command' && notice?.kind === 'error' && notice.id !== before.notice?.id ? notice.message : null })
      } catch (error) {
        store.setState(before, true); ui = beforeUi; rng = beforeRng; nativeReserve = beforeReserve; generation = beforeGeneration; reviewedLots = beforeReviewed
        response = JSON.stringify({ ok: false, error: error instanceof Error ? error.message : 'Native rules failure' })
      } finally { inFlight = false }
    })()
  },
  takeResponse() { const result = response; response = null; return result },
}
;(globalThis as unknown as { MaiNative: typeof MaiNative }).MaiNative = MaiNative
