# UE5 reference gate report — 2026-09-07

**UE5 reference gate: NOT VERIFIED**

Audited browser/asset source: `cda0b460257e656c57b31c8ccebfb79097023d4a`.
Prepared C++ project and tooling: `6ba31ef5ec7c96c9654d752beaa8875d6cdc3f08`.
Branch: `agent/ue5-core-vertical-slice`. No merge into main.
This report records a blocked gate, not acceptance of the reference scene.

## Actual scene state

| Requirement | Observed result |
| --- | --- |
| Installed engine version | No accessible UE5 installation found in the editing environment; no version selected or pinned |
| Map | Intended name `L_Reference_GarageCity`; **not created**; no .umap |
| Garage and separate enclosure | Original FBX bytes verified in CI; not imported into UE |
| Separate CityV4 fragment | Original full city FBX bytes verified; fragment not selected/imported |
| Static architecture / Nanite | Not configured or verified |
| Lumen GI and dynamic shadows | Not configured or verified |
| Gameplay-like camera | Not placed or visually checked |
| Skeletal object | person-1 source FBX present; no imported SkeletalMesh/Skeleton/PhysicsAsset verified |
| Simplified Garage/interactive collisions | Existing source UCX descriptions are not an engine test; no line traces performed |
| World Partition | Not created or tested; cell size, streaming range and Data Layers are **unknown / unset**, not assumed defaults |
| Repeated props via ISM/HISM/Foliage | Not implemented or measured in a reference level |
| Editor screenshots / visual render | None produced; no Blender render reused as UE evidence |

## Skeletal LOD measurements

| Level | Requested triangle budget | Exact imported triangle count |
| --- | --- | --- |
| LOD0 | Approximately 5000–8000 | **Unknown; not measured** |
| LOD1 | Approximately 2000 | **Unknown; not measured** |
| LOD2 | Approximately 500 | **Unknown; not measured** |

`Unreal/Assets/characters/person-1/asset.json` reports **58579 source triangles and 22 bones**, with `unreal_editor_verified: false`. Those are authoring metadata. They are not any of the three imported LOD measurements and do not satisfy the requested budgets. No LOD meshes were generated in this run; no authoring .blend was modified.

## What was actually executed

[Browser baseline run 34137561414](https://github.com/adaybekovt-boop/Make-your-ai-UE5-/actions/runs/34137561414), [raw job logs](https://github.com/adaybekovt-boop/Make-your-ai-UE5-/actions/runs/34137561414/job/101791946056).

[Preparation run 34138706257](https://github.com/adaybekovt-boop/Make-your-ai-UE5-/actions/runs/34138706257), [raw job logs](https://github.com/adaybekovt-boop/Make-your-ai-UE5-/actions/runs/34138706257/job/101795557971).

| Command/check | Execution context | Exit code | Meaning |
| --- | --- | ---: | --- |
| npm test | CI; original cda0b46 | 0 | 388 browser tests, 29 files passed |
| npm run typecheck | CI; original cda0b46 | 0 | Browser TypeScript check passed |
| npm run build | CI; original cda0b46 | 0 | Browser production build passed; existing large-chunk warning remains |
| python3 -m unittest discover -s Unreal/Tools/tests -v | CI; prepared 6ba31ef | 0 | 16 preparation-tool tests; **not Unreal Automation** |
| FBX header and original Git blob assertions | CI; prepared 6ba31ef | 0 | Four complete source files read; hashes match; none are LFS pointers |
| python Unreal/Tools/ue5.py build --output /mnt/data/ue5-work/Unreal/Evidence/2026-09-07/build-attempt | Local preparation directory, not a clone | 2 (wrapper) | Blocked before UBT; underlying build process exit code is **null**, not 2 |
| Actual Unreal project-file generation | Not executed | N/A | No installed engine |
| Actual Development Editor compilation | Not executed | N/A | No installed engine/UBT |
| Unreal Automation tests | Not executed | N/A | No engine test executable or implemented gameplay tests |
| Editor launch / scene inspection / visual render | Not executed | N/A | No accessible Editor rendering session |

The local build receipt is [local-build-attempt.json](Evidence/2026-09-07/local-build-attempt.json). It records `UE_ROOT / --engine-root is not set; no installed UE5 version can be selected.` The separate failed `git rev-parse HEAD` in that receipt reflects the local non-clone preparation directory. It does not invalidate the actual detached clones and successful checks on GitHub Actions.

[Preparation evidence summary](Evidence/2026-09-07/preparation-ci.json) retains source hashes and the exact CI run/job references. Source-byte integrity is not proof of FBX importability, skeletal deformation, collision, LOD quality, Nanite or rendering.

## Acceptance work still required — not executed commands/results

On a machine with an actual UE5 installation, use `Unreal/Tools/ue5.py audit`, then `pin-engine`, `generate` and `build`, passing `--engine-root` or setting UE_ROOT. The tool reads the actual Build.version rather than choosing a guessed release. Inspect the genuine UBT log and launch the selected installation's Editor with this .uproject. See [project instructions](MakeYourAI/README.md).

After the minimal project compiles and loads, author the real `L_Reference_GarageCity` map. Import the Garage, separate enclosure, a bounded CityV4 fragment and one skeletal object from the repository. Preserve the original source files. Check scale/orientation and materials in the engine, enable and inspect Nanite for eligible static architecture, and verify Lumen with dynamic shadows on the actual rendering hardware.

For every interactive reference object, retain a collision trace result identifying the actor/component, collision channel, start/end points and expected hit. Include meaningful miss/doorway tests so an oversized solid box cannot masquerade as valid Garage collision. Existing UCX names or enabled Nanite are insufficient evidence.

Create the real three-level skeletal LOD chain, record each exact imported triangle count and inspect deformation at each LOD. Configure World Partition and record the actual cell size, streaming range, Data Layers and streaming observations. Use and inspect ISM/HISM or Foliage for repeats. Record the final map/object paths, engine version, build/test commands and exit codes, genuine logs and screenshots tied to the tested commit.

## Gate decision

Compile verified: **NO**. Automation tests verified: **NO**. Editor scene verified: **NO**. Visual render verified: **NO**.

No gameplay port, Garage order/save vertical slice, auction, nuclear tariff, suburb or NPC implementation is accepted or started beyond this gate. This run changes only preparation code, CI and documentation/evidence; it creates no scene assets.

Next blocker: accessible UE5 Editor + UBT/toolchain and rendering hardware. First pin the actual installation, compile Development Editor and launch the minimal project; only then validate the reference scene.
