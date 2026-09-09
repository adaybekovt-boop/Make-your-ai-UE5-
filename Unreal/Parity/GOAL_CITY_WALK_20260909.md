# City / first-person goal progress — 2026-09-09

This is an incremental local candidate, **not completion of the full game goal**.
The earlier published beta is not replaced by this report.

## Implemented

- First-person mouse look no longer requires RMB. Tab alternates captured look
  and UI cursor; NativeOnPreviewKeyDown prevents Slate focus navigation from
  swallowing the return toggle. WASD is inactive while the cursor is released.
- A center dot and compact lower-left help replace the management panels during
  captured walking. Tab restores the full management interface.
- E and its HUD hint select the same visible, in-range point in the forward view
  cone, filtered to the current interior; not the nearest point behind the player.
- Removed simultaneous distance-triggered global shadow/LOD switches. HISM
  screen-size LOD remains active at a stable scale; quality is reapplied after
  the generated city is actually loaded.
- Night emissive strength is bounded, zero in full daylight, and retains authored
  color masks. Night exposure has a floor of -1 EV100 after the first tested
  +1 EV candidate proved too dark in screenshots.
- Added distance- and derivative-filtered world-space brick/stone/roof/asphalt
  detail (color, roughness, surface-gradient normal). This is new UE art, **not
  numerical parity with Blender's noise/bump**, and not a replacement for a
  larger architectural/foliage geometry overhaul.
- Original synthesized server-fan ambience is tied to installed chassis in the
  current room, with smoothed transitions and the existing master volume. No
  third-party audio files. Built successfully; subjective listening not verified.

## Evidence

All paths below are relative to `Unreal/MakeYourAI/Saved/Verification/`.

- Final Editor build `20260909T103656Z-f39ab4d2`: PASS.
- Automation `20260909T103710Z-a0438df0`: 13 succeeded, 0 warnings, 0 failed.
- Python source/tools `goal-source-20260909-final.log`: 38 tests, 1 skipped, OK.
  The initial sandboxed attempt had temporary-directory access failures; the
  subsequent authorized Windows run passed. Do not cite the failed attempt as PASS.
- Real content import `20260909T102541Z-51cfa130`: revision
  `323926a6d0309ac66996`, 216 meshes, 29,395 original instances, 1,695 batches,
  plus 90 exurban data-center pieces. No material-compile errors found in log.
  The first custom material graph failed to connect a mask input and was fixed
  before this successful import; its incomplete generation remains separate.
- Three-resolution GPU/action/restart run `20260909T102711Z-0725d1fc`: completed
  at 1280x720, 1600x900, 1920x1080. This predates the compact HUD, Tab-preview
  correction and server audio; do not claim those final changes have the same
  three-resolution coverage yet.
- Windows UI test of the final Editor binary in isolated namespace
  `goal-hud-20260909-1038`: Continue, pause, City, enter Garage; dot/compact HUD
  visible, Tab restores panels, second Tab hides panels again. A cursor-moving
  scroll action (no mouse button held) visibly rotated the first-person camera.
  This found and fixed the prior Tab focus bug. This is tool-driven UI evidence,
  not a human playtest or a full 32-step campaign.
- A first manual QA namespace used an older QA save whose simulation had advanced
  to day 5. It was discarded from the final control check; the fresh capture save
  used above was paused at day 1 08:43, cash 5,662 dollars.
- Personal Player save SHA256 remains
  `794f4ef1e316703c7ed56aeda53112436b319e0308b58f4b58e74bf851b7784f`.

Short 1280x720 sample windows in the three-resolution run: overview 49.09 mean
FPS / 24.88 ms p95; close day 59.20 FPS; close night 60.00 FPS; Garage 59.06 FPS.
These include scene transitions and a 60 FPS cap, **not an isolated GPU benchmark
or minimum-spec proof**. Sustained CPU/GPU/RAM/VRAM comparison remains necessary.
An obsolete isolated Shipping QA process was subsequently discovered still
running, so these numbers are also **confounded by a second game process**.
That exact QA process (validated executable and `-userdir=.../Shipping-QA-20260909`)
was closed before further performance measurements. Do not publish these sample
numbers as performance claims or hardware requirements.

## Remaining goal work

- Inspect near-ground masonry/roof detail and camera sweeps, measure flicker and
  load; do not infer complete flicker removal from static PNGs.
- Further city architectural, foliage and street-level detail; richer night
  windows/street illumination. Current geometry still visibly stylized.
- Finish all browser UI layouts/visualizations, settings/localization checks.
  Original training screen was inspected again in the in-app browser: it has
  four category cards, a training ring, radio-domain controls and integrated
  quantization cards not yet reproduced exactly by the native screen.
- Aimed interaction needs broader room-by-room OS checks; all-room bounds tests
  remain covered by Automation, but not a full visual playthrough of nine rooms.
- Audition/measure server ambience; traffic audio and moving traffic not added.
- Full campaign/endings/32-step runtime pass and standalone candidate validation
  before claiming readiness for main/release publication.

Technique reference: [Epic — Bump Mapping Without Tangent Space](https://dev.epicgames.com/documentation/unreal-engine/bump-mapping-without-tangent-space-in-unreal-engine).

## Standalone candidate and follow-up exposure correction

Shipping BuildCookRun completed successfully in 90.79 seconds; log
`Saved/Verification/package-city-walk-20260909.log`.
Candidate directory (relative to the task workspace, not repository):
`outputs/Neuron-city-walk-20260909-Windows/Windows/MakeYourAI.exe`.
Shipping game binary SHA256:
`d1e8186ec0046feec68cfec5bce4213ab9c322d088f6ac9c82f3ff698e84b7ff`.

Actual Windows UI sequence on this EXE: Continue, pause at day 1 08:42 / cash
5,663 dollars, City, enter Garage, Tab to restore toolbar, Save, close process,
relaunch, Continue. The furnished Garage, paused time and 5,663 dollars restored.
The QA root is `outputs/Shipping-city-walk-QA-20260909`; personal saves untouched.

This test also revealed the room was brighter when entered from a loaded city
than when loaded directly by Continue. The walking camera now explicitly sets
AutoExposureBias to 0.65 instead of inheriting the city's bias. That correction
was made **after the above Shipping binary**; do not claim it is included there.
Editor build `20260909T104744Z-85329e86` and Automation
`20260909T104750Z-f1b473c4` include this correction. A new GPU run is in progress
as `20260909T104806Z-a38df54f`: all three resolutions completed with process
restart checks. The compact Garage HUD PNG was inspected at 1280x720.
Automation: 13 passed, no warnings/failures. Refreshed Python source checks:
`goal-source-20260909-exposure.log`, 38 tests / one skip / OK.

Shipping v2 in `outputs/Neuron-city-walk-20260909-Windows-v2` includes the explicit
interior exposure bias; BuildCookRun PASS in 35.82 seconds, log
`Saved/Verification/package-city-walk-v2-20260909.log`.
Its game binary SHA256 is
`952b0fa79a16b15d20363369d00bb1b2d507ba4d23e5a7a9f07515dea105fe67`.
The v2 EXE was launched without Editor, loaded the same saved Garage and returned
to the city through its UI. Paused day 1 08:42 / 5,663 dollars preserved.

The 30-second stationary overview measurement with only this game process running
is `Saved/Verification/city-walk-v2-overview-load-20260909.json`: game CPU mean
8.26% across 24 logical processors, working set mean 1,315.83 MiB. GPU readings
cover the whole desktop (including the open original browser game): first/last
30/29% utilization, 2,635/2,631 MiB VRAM, 166.93/166.96 W, 55 C. This is **not**
a moving-camera frame-time benchmark or process-specific GPU measurement.
