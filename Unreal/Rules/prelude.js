// Loaded before the trusted rules bundle. Plain JSON game data is the only
// structuredClone caller in the source; no DOM, Node, filesystem or network API.
if (typeof globalThis.structuredClone !== 'function') {
  globalThis.structuredClone = value => JSON.parse(JSON.stringify(value));
}
