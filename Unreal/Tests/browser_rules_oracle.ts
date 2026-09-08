// Read-only executable oracle using the original store and original systems.
// No native kernel, view or compute-reservation patch is imported here.
import { useGameStore as store } from '../../src/store/gameStore'
let seed=0x12345678
Math.random=()=>{seed=(seed+0x6D2B79F5)>>>0;let t=seed;t=Math.imul(t^(t>>>15),t|1);t^=t+Math.imul(t^(t>>>7),t|61);return ((t^(t>>>14))>>>0)/4294967296}
;(globalThis as any).Oracle={
  async boot(){await store.getState().initialize()},
  async execute(r:any){if(r.method==='command'){const f=(store.getState() as any)[r.action];await f(...r.args)}
    else if(r.method==='tick')store.getState().tick(r.seconds)
    return store.getState().company},
}
