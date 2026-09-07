# Source parity notes — 2026-09-07

Source commit: `cda0b460257e656c57b31c8ccebfb79097023d4a`.
Baseline execution: workflow commit `7ba500695be46eb10acfec00458188f7cedd8506`; [run](https://github.com/adaybekovt-boop/Make-your-ai-UE5-/actions/runs/34137561414).
This is a source review and future-port checklist, NOT implemented C++ gameplay.

## Review coverage

Read README.md, UNREAL_MIGRATION.md, REFERENCE_CITY_REPORT.md, INTERIOR_CITY_REPORT.md, Unreal/Assets/README_RU.md and Unreal/Assets/UNREAL_IMPORT_RU.md before changing project source. Reviewed .gitattributes, package.json, procurement.ts and procurement.test.ts, initial-state/purchase portions of simulation.ts, location/procurement definitions in config.ts, and person-1/asset.json. All 388 browser tests were executed by the baseline workflow; that is not a claim that every unrelated domain file has received a full manual audit. More source review is required before porting each subsystem.

## Existing procurement contract

Source: `src/systems/config.ts`, `procurement.ts`, `procurement.test.ts`.

- Official multiplier 1.6; grey 1.1; factory defect threshold strictly RNG < 0.08. Official equipment does not consume a factory-defect RNG draw.
- Discount is 0.05 for each identical unit after the first, capped at 0.20; valid integer quantity 1 through 24. Unit price is rounded first, then the discounted order total is rounded. Preserve both rounding stages.
- Kit purchase composes rack and chip orders against immutable state. Either the caller receives the successful complete state or the request fails without applying a partial rack payment. Never port this as an in-place debit followed by a fallible second debit.
- Delivery is due at `arriveAt <= elapsedGameHours`, adds inventory once, and performs no mounting, defect draw or extra charge. The future pause/clock port must be verified against the actual simulation/store clock, not just deliverOrders in isolation.
- Inventory belongs to the location. Grey counters retain provenance. Official/previously verified stock is used first. Save tests cover grey stock, pending orders and installed equipment.
- Mounting requires an owned location, an available valid grid cell/rig, compatible chassis and enough local power. Failed power validation does not consume inventory or RNG.
- Mounting has no purchase charge. An upgrade must increase compute. Chassis and the previous server ID are retained; the old working chip is returned to inventory even if the replacement fails its defect roll.
- Defect handling reuses equipment disposal, with 15% of the base price charged for cleanup. A failed chip leaves the chassis/rig. A failed chassis is consumed.

The baseline ran **34 procurement tests** within the 388-test suite. Unreal equivalents are not implemented or executed.

## Documentation/scope differences

1. UNREAL_MIGRATION.md incorrectly claimed Git LFS; .gitattributes has binary declarations without LFS filters. Corrected documentation only; no asset conversion.
2. Historical reports mention 231 or 245 tests. They are dated/archival observations, not the current baseline, which is 388. Their historical results were not rewritten.
3. config.ts includes `dc-north` and `dc-south` in addition to Garage, Workshop, Technopark, Server Hall and Campus. Do not silently discard these existing location IDs from future save migration just because the first vertical slice covers Garage.
4. createInitialGame sets existing base locations to owned=false. buyLocation checks ownership, cash and board restrictions; it does not impose a sequential five-location unlock progression. A new Locked/Available/Owned representation must not invent progression/balance rules. Regional locks use the existing region unlock mechanism.
5. STARTING_CASH is 12000; GAME_HOURS_PER_REAL_SECOND is 1/60; Garage is 3x3, purchase price 1500, power limit 3 kW. These are source values, not new design choices.
6. Procurement rounds purchase prices, while baseline economy output contains fractional cash. The fixed-point unit, accumulation and rounding policy for operating income need an explicit parity decision before replacing floating-point money with int64. This run does not change that policy.
7. person-1 source metadata is 58,579 triangles / 22 bones, not a verified 5,000–8,000 / 2,000 / 500 skeletal LOD chain. Existing UCX/Blender validation does not prove UE line-trace collision.
8. CityV4 geometry of an auction, nuclear station and suburb does not establish their gameplay prices, tariff formula or risk rules. Those systems remain after the reference and Garage gates; no unsupported constants were invented.

## Explicitly unverified

No UE import, skeletal deformation/LOD measurement, collision trace, real-time human playtest, 1x balance remeasurement, engine compilation or UE save/restart cycle was performed. The previous human-playtest requirement remains open. No office/tax/interior gameplay expansion was started.

Next step: establish the actual UE5 installation and reference scene before translating these rules; then review the current clock, model-company simulation and save migrations in full for the Garage implementation.
