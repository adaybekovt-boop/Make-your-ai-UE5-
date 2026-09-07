# UE5 environment audit — 2026-09-07

**UE5 reference gate: NOT VERIFIED**

Repository: `adaybekovt-boop/Make-your-ai-UE5-`.
Audited main/source commit: `cda0b460257e656c57b31c8ccebfb79097023d4a`.
Working branch: `agent/ue5-core-vertical-slice`; created from that commit, not merged.
Baseline workflow commit: `7ba500695be46eb10acfec00458188f7cedd8506`.

## Two different execution environments

The editing container is Debian 13.3, Linux x86_64, kernel 6.18.35. `clang++ --version` returned Clang 17.0.0; `g++ --version` returned GCC 14.2.0; both commands exited 0. These are installed system compilers, NOT a validated UE toolchain. Node is 22.16.0, npm 10.9.2, Git 2.47.3.

UnrealEditor, UnrealEditor-Cmd, UnrealBuildTool, dotnet and GPU diagnostic executables were not found on PATH. A bounded search under /opt, /usr/local, /home, /mnt, /app and /tmp (depth 7, excluding node_modules/.cache) found no Editor, UBT or Build.version. That search exited 1 because some searched paths were absent; it is not proof about every possible external installation. No UE5 installation is available through the tools used in this run. Engine version: **not selected / not pinned**, rather than an invented release number.

DISPLAY=:0 is set, but /dev/dri and /dev/nvidia0 are absent, no Xorg/Xvfb process was found by the process check, and no rendered Editor session was available. GPU rendering: **not verified**. DISPLAY alone is not evidence of a usable GPU.

Local `git ls-remote https://github.com/adaybekovt-boop/Make-your-ai-UE5-.git` exited **128**, with `Could not resolve host: github.com`. Consequently the local preparation directory is not a repository clone. Local git status/HEAD are not claimed as verified. GitHub API reads/writes work; the actual clone and browser execution were performed separately on GitHub Actions.

## Actual browser baseline

[Workflow run 34137561414](https://github.com/adaybekovt-boop/Make-your-ai-UE5-/actions/runs/34137561414), [job 101791946056 and logs](https://github.com/adaybekovt-boop/Make-your-ai-UE5-/actions/runs/34137561414/job/101791946056).

The runner used Ubuntu 24.04.4, Node **22.23.2**, npm **10.9.8**. It fetched the exact audited source commit, used detached HEAD and a sparse checkout excluding Unreal working-tree binaries, and checked `git rev-parse HEAD`. Initial `git status --short` was empty. No gameplay port existed in the tested commit.

| Executed command | Exit code | Observed result |
| --- | ---: | --- |
| npm ci | 0 | 117 packages added; npm reported 0 vulnerabilities in this run |
| npm test | 0 | **388/388 tests; 29/29 files** |
| npm run typecheck | 0 | TypeScript build check succeeded |
| npm run build | 0 | Vite production build succeeded |

Existing warning: DRACOLoader chunk 615.55 kB, above the 500 kB warning threshold. Post-test status showed untracked `artifacts/portfolio/`; those generated artifacts were NOT committed. setup-node v4 also produced an action-runtime deprecation warning; it ran under the runner's Node 24 action runtime while the project itself used Node 22.23.2. Do not confuse the two.

No browser interaction, real-time human playtest or new 1x balance measurement was performed. Automated success does not close those earlier playtest requirements.

## Required assets at the audited commit

`git ls-tree -r -l HEAD -- <the four paths>` completed on the runner. These are full-sized Git blobs, not small LFS pointer replacements. This is repository/object evidence, not an Unreal import.

| Path | Git blob SHA | Bytes |
| --- | --- | ---: |
| Unreal/Assets/interiors/garage/garage.fbx | 3f291e6dd3755de290bc06d0aaa05ac0da51000a | 3979452 |
| Unreal/Assets/interiors/garage/garage-enclosure.fbx | 41fa99fd34791ae204b4cb414209d38db321f3dc | 2151916 |
| Unreal/CityV4/city-v4.fbx | 67105e990ad02eae231c9ab97d2630f8e6731c8c | 57088860 |
| Unreal/Assets/characters/person-1/person-1.fbx | 3e333924ecf6e59b9032d05c498834d3a2b8cfdf | 5302972 |

person-1/asset.json reports 58,579 source triangles and 22 bones, with unreal_editor_verified=false. These are authoring metadata, not measured UE skeletal LOD0/1/2. Exact imported LOD counts: unknown.

## Changes and remaining blockers

This audit adds this report and the evidence/source-audit files, and corrects the false LFS instruction in UNREAL_MIGRATION.md. `.gitattributes`, browser code, assets, authoring .blend files and main are not changed by this audit.

Development Editor compilation, project generation, Editor launch, all Unreal Automation tests, Nanite, Lumen, World Partition, collisions and visual renders remain unverified. A preparation-only C++ module can be added without claiming those steps passed. No gameplay migration, auction, nuclear tariff, suburb or NPC system is authorized past the unverified reference gate.

Next blocker: an accessible, actually installed UE5 Editor + UBT/toolchain and an Editor-capable rendering machine. Pin that installation, compile Development Editor, launch the minimal project, then build and test the real reference scene.
