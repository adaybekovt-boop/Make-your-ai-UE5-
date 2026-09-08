# Native parity delivery — 2026-09-08

> This document records the cloud handoff. Subsequent local UE 5.8 build,
> rendering corrections and verification are recorded in [LOCAL_UE58_STATUS.md](LOCAL_UE58_STATUS.md).

## Status

Incomplete migration, not a full native copy. There is no Unreal Editor, UBT/UHT, GPU, Blender or Windows/PowerShell installation in the local cloud container. C++ compiler, Node and Python checks actually run; Blender source export runs through the explicitly created GitHub Actions workflow. Browser rendering in the cloud is blocked (`ERR_BLOCKED_BY_ADMINISTRATOR`). No UE screenshots, assets or maps are fabricated.

Work branch: `agent/browser-parity-cityv4-20260908`. Base: `agent/ue5-core-vertical-slice` at `cf087025697e9ba3e3ab3ca5380bcdee54edea3d`. Browser reference: `fd270196525c4c6fe61217327c81274e67a58d4b`. Main is untouched. No AGENTS.md exists in the audited tree. Authoring assets and browser sources are not changed.

## Architecture

The earlier standalone C++ economy represented a reduced flagship game. Rewriting its formulas behind a larger UI would not port the browser game. This delivery bundles the original 29 TypeScript rule/source modules at build time and runs them in-process in pinned QuickJS-NG (`1ab8676f4b6d6d669baeb5f21790fb9734636a20`, MIT). Runtime has no Node, browser, WebView, DOM, React renderer, network or JS filesystem module. Unreal owns the UMG/Slate interface, scene, input and durable file storage. This adds an embedded VM dependency rather than silently changing source behavior.

The only explicit economy adapter reserves compute used by native AI-review; a zero reservation is an identity. Source files remain unchanged. Native campaign extensions project the canonical ledger and must not run a second money clock. Three new C++ regressions cover the projection and already-paid datasets. Native-only difficulty, review and ending behavior is separately listed in the matrix and is not declared browser parity.

## Actual evidence

See `evidence/` for logs from this session, not earlier reports. The native campaign suite executed with ASan/UBSan: 46 cases, 1604 assertions, 0 failures; byte-equivalent continuation in a separate process. The new rules differential suite compared both strategies against an independently bundled original browser store for 3418 steps, 568852 recursive assertions and 24 scenarios. Delivery uses 1x as its primary clock. 11 actual filesystem assertions cover atomic replacement, backup, bounds and failed writes. Nine synthetic CityV4 binary/grouping tests do not claim a Blender or UE import.

The differential suite is not a human playtest or UE UI run. Its seeded generator is shared with the independent oracle, not a favourable constant RNG. Original browser tests remain intact. Automation source existence or a zero editor exit code is not considered a passed UE test.

## CityV4 findings

Actual v1 source export, run twice from the pinned authoring file, found per scene: 29,395 visible mesh objects, 251 source mesh datablocks, 756,364 unique source triangles and 8,162,396 evaluated triangles with instances. Original Blend SHA-256: `a25970ac39b81def36ddc6564a4aa2af1988f205f393912cc16432969e4610fe`; FBX: `ebea2633562cf35ed1719e13e0001ec6305431054edb08a749889f5137ce5dea`. Before/after hashes agree.

The first exporter deduplicated to 216 geometry resources but omitted a source vertex-color channel. That is an actual exporter defect, not evidence about the user's broken UE import. v2 adds per-corner source color data and rejects missing named color attributes. v1 measurements must not be presented as v2 verification. Final source-export evidence is recorded separately when the v2 workflow runs.

The old combined-FBX path is disabled for CityV4 before any asset write. New imports retain indexed topology, source materials, normals, UVs, color attributes, transforms and the real Blender perspective camera. HISM batches separate spatial cells, semantic group, material bindings and gameplay identity. Default baseline disables Nanite and reduction; opt-in Nanite retains the full fallback. Each imported mesh records source LOD0, render LOD0, Nanite/fallback/reduction and renderer details. Mismatched triangle counts fail the build. The original cause of the local triangle corruption is **not established**.

Real assets are created only by Unreal's importer/material/editor save APIs in a revision-specific `/Game/Generated/CityV4/` namespace. Hash receipts refuse to overwrite edited derived assets, and repeated content runs reuse owned output. Source graphs consuming vertex color are mapped to vertex color, not replaced with grey. Noise-to-bump normal graphs remain an explicit visual gap. The day/night intensity/exposure defaults are an unverified UE baseline, not measured Blender matching.

## Windows runner

Install the desired UE5 release with C++ tooling, Visual Studio C++ build tools, PowerShell 7.2+, Node 22, CMake, Git and Blender 4.5.x. Do not infer the UE version from old reports. The runner discovers launcher/registry installations or accepts UE_ROOT, reads real Build.version and pins its hash without rewriting EngineAssociation.

From the repository root:

```powershell
pwsh -NoProfile -File .\Unreal\Tools\Run-NativeParity.ps1 -Stage audit
pwsh -NoProfile -File .\Unreal\Tools\Run-NativeParity.ps1 -Stage all -Blender "$env:BLENDER_EXE"
```

When multiple engines are installed, set `UE_ROOT` explicitly to the engine being tested. `BLENDER_EXE` is the full installed Blender executable path. No user-specific path is embedded. Existing source/assets are retained. For separately produced v2 exports use `-CityManifest <absolute manifest.json>`. Nanite is intentionally opt-in using `-EnableNanite` after the unreduced baseline.

Stages: audit → pin-engine → generate native dependency/rules/project → actual Editor build → explicit Automation report validation → read-only Blender export and actual UE content creation → real GPU captures at 1280×720, 1600×900, 1920×1080 plus a separate UE process loading the saved company. Reports include process exit codes and logs under `Unreal/MakeYourAI/Saved/Verification/`. Capture slots are isolated from the player slot. A PNG must exist with the requested dimensions; no generated/Blender image is accepted.

The capture subsystem dispatches real UMG semantic actions, not OS pointer clicks. Its report explicitly marks `humanPlaytest:false` and `fullFlowVerified:false`. It captures menus, city, procurement and training UI and validates a separate-process save/load; it is not the full delivery→review→training→ending manual acceptance. Source scripts, UHT and engine APIs may still fail until the real installed UE build is run; such failures retain logs and do not become PASS.

## Manual acceptance still required

Run New Game → both strategies/difficulties → Prologue → City → Garage purchase → compatible order → real-time 1x delivery → install chassis/chip → walk to desk → paid dataset → each review method → training → Save → close executable → new executable → Load → continue → each reachable ending → new game. Test insufficient money, wrong location, incompatible kit, overcapacity, unreviewed data, insufficient compute, damaged save, cancelled/failed world load and completed-party re-entry. Include bad-timing fire/court/server events, not only favourable seeds. Record `humanPlaytest:true` only for an actual manual run.

The main implementation gaps are listed row-by-row in BROWSER_TO_UE.md: authored interiors/avatars/traffic, procedural material normals, exact visual styling and all native extension bindings. No complete-copy claim is made. The next gate is the installed-engine build and actual GPU/UI acceptance, not another portable test result.
