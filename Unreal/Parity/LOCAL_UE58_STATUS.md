# Local Unreal 5.8 verification — 2026-09-08

This is an operational native port, not a completed feature/visual clone of the browser game.

## Rendering correction

The MAIMSH02 interchange preserved authored split normals but its triangle facing
was opposite Unreal MeshDescription's convention. Engine-computed triangle normals
confirmed the mismatch on all 216 unique city meshes. The importer now compares
engine face normals with authored corner normals and reverses polygon facing when
the majority is opposed. Positions, instances, material slots and authored shading
normals are retained; making everything two-sided is not used as a workaround.

Confirmed by the real game after reimport: previously black airport geometry is
visible, roofs no longer produce the broad white glow, and close-up facades are
solid. A new content revision is generated; previous revisions and Blender sources
are not overwritten. Import evidence: `20260908T081555Z-84a3f803`.

Window emission now follows the lighting cycle: zero in full daylight, authored
strength at night. Exposure follows the light cycle; the far-camera mode applies
-1.2 EV compensation. The final Shipping overview was inspected through native
Windows input: geometry and vegetation are present, but distant colors remain
pale. Lighting polish is still an open issue, not a completed visual acceptance.

## Earlier verified work

- UE 5.8.2 Editor target compiles with VS 2022.
- 13 Unreal Automation tests passed (`20260908T084233Z-0d3c8271`).
- 29,395 city instances and 216 unique meshes imported.
- UMG semantic action capture and real process save/restart passed at 720p,
  900p and 1080p (`20260908T084445Z-34e6887a`), before the final far-camera
  compensation and vegetation-distance adjustment.
- Real OS mouse wheel zoom and saving/loading a purchased garage were checked.
- Native dark-teal/mint UI, responsive menu panels, mouse camera controls and
  seven location markers are implemented.

## 0.0.1 beta Shipping check

- Win64 Shipping build, cook and packaging succeeded with UE 5.8.2.
- Packaged content revision: `bc8f3d3775d2b9eb9749`.
- Native OS clicks in the standalone executable completed new company `BetaQA`,
  normal difficulty, prologue, personality choice, city loading and garage purchase.
- Clicked Save, closed the process normally, relaunched the root executable and
  clicked Continue. The company loaded and the garage remained owned; the UI
  explicitly displayed that the last save was loaded. No injected semantic actions
  were used for this Shipping smoke check. This is not a full campaign playthrough.
- The first check ran at 1600x900; relaunch used the desktop's 2560x1440 fullscreen.
- 11 Python city-format/planning regression tests passed.

## Optimization and measured load

City grouping was reduced from 4,563 to 1,695 batches while retaining 29,395 source
instances. The two detached data-center campuses add 90 instances. 195 meshes have
three source LODs; the remaining 21 have one. LOD0 preserves the original geometry.
Far mode disables dynamic sunlight shadows with distance hysteresis. Vegetation
culls at 60,000–80,000 cm. The application applies a default 60 FPS limit; native
widget layout prepass runs on UI refresh instead of every frame.

On i9-12900KF / RTX 3090 24 GB / 64 GB RAM, a 30-second standalone Shipping overview
sample at 1600x900 measured mean process CPU 10.8% (24 logical processors), peak
13.3%, and mean working set 1,388 MiB (peak 1,390 MiB). Early GPU samples showed
50–51% utilization, roughly 2,536 MiB VRAM, 239 W and 63 C. GPU measurements cover
the entire desktop, not only the game. This is neither a minimum-spec test nor a
frame-time benchmark; it does not prove stutter-free navigation.

The earlier automated 900p close-up capture averaged 58.65 FPS with p95 frame time
16.67 ms. Those short rolling samples include a different test configuration and
must not be advertised as a measured Shipping frame rate.

## Reproduce

Set `UE_ROOT` to an installed UE 5.8 engine. Put CMake, Node and Git on PATH.
Use `Unreal/Tools/Run-NativeParity.ps1` stages `audit`, `pin-engine`, `generate`,
`build`, `automation`, `content`, `visual`. The dependency lock pins QuickJS.
Export CityV4 using `export_cityv4_instances.py` in Blender first; pass its manifest
to the `content` stage. Generated assets, build output, engine installation,
personal saves and dependency checkout are deliberately excluded from Git.

## Limits

Not yet a full playthrough. Mouse orbit/pan are implemented
but not independently verified with OS drag. Browser UI parity, authored interiors,
traffic, characters and some campaign features remain incomplete. Procedural Blender
bump graphs are recorded but not equivalent Unreal micro-normal shaders. Performance
samples are recent game-frame deltas, not a sustained GPU benchmark. Automated PNG
creation alone does not certify visual quality.

The distant map remains pale; the north data-center marker can wrap below its
background. New data-center campuses have no authored road connection yet.
The beta is a playable early port, not the requested finished browser-equivalent
game. Minimum/recommended hardware still needs testing on additional machines.
