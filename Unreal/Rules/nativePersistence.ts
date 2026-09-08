import { decodeCompanySave, makeCompanySave } from '../../src/persistence/companySaves'
import type { CompanyState } from '../../src/systems/models'

// The host owns durable I/O. Keeping the original store's promise queue makes
// snapshots ordered without exposing a filesystem, browser API or network to JS.
let saved: unknown = null
let sequence = 0
export async function loadCompanyGame() { return saved === null ? null : decodeCompanySave(saved) }
export async function saveCompanyGame(game: CompanyState): Promise<string> {
  const envelope = makeCompanySave(game)
  saved = envelope
  sequence += 1
  return envelope.savedAt
}
export function stageSave(value: unknown) { saved = value === null ? null : decodeCompanySave(value) }
export function storageSequence() { return sequence }
