# Local continuation — 2026-09-09

## Visual/readability revision — later 09:30–10:00 UTC pass

Latest user feedback prioritizes a readable, attractive native UI over the previous
icon-only toolbar. This revision keeps labeled navigation alongside vector icons,
separates status/time/save from navigation, highlights the active page, adds three
model metric cards, and splits training into work and data/team columns. Dropdowns
use custom native text and dark styling rather than the default gray control.

The city exposure curve previously interpolated EV linearly against linear sun
lux. It now uses log2(lux / 2.5), with a visually tuned daylight compensation of
1.25 stops, reduced bloom, neutral daylight and stronger sky fill. Camera distance
no longer changes exposure. Nearby city meshes retain authored detail longer with
LOD distance scale 1.65; overview uses the previous scale 1 and shadow reduction.
This is better preservation of existing geometry, not newly authored buildings.

The GPU flow now enters the room from its equipment panel and asserts that the
walking UI appears, then captures the city after leaving. This covers the panel
that previously obscured the room and makes the interior-to-city lighting visible.

- Editor build PASS: `20260909T094455Z-9864ea9a`.
- 13 Automation tests PASS: `20260909T094506Z-45166e06`, including the new
  logarithmic exposure assertions within the existing editable-profile test.
- Rules parity: 568,852 assertions, zero failures after the dashboard changes.
- Rules TypeScript typecheck PASS; Python source: 35 tests, one skipped, no failures.
- Runtime bundle: `fe822edfb64f7ea2033f90137a310de38f936c208acb54bffc0082d9c4ec37b8`.
- Visually inspected actual 1280x720 day overview and close view from
  `20260909T094522Z-b0bae6d8`. The first darker calibration was rejected after
  inspecting `20260909T094017Z-c803cd3c`; those earlier images are not the final look.
- The final GPU action flow and separate-process resume completed with exit 0 at
  all three resolutions (1280x720, 1600x900, 1920x1080). Reviewed the final Garage,
  training and city-return PNGs too. This is still a semantic smoke, not the full
  campaign. Short 1280x720 frame windows: city 54.99 FPS, close day 58.62 FPS,
  Garage 57.44 FPS; all three p95 frame deltas 16.67 ms. Windows contain transition
  frames and are not isolated/sustained performance benchmarks.
- Personal Player save SHA256 remained
  `794f4ef1e316703c7ed56aeda53112436b319e0308b58f4b58e74bf851b7784f`.
- Post-capture Shipping input test found the occupied rear rack row blocked the
  initial camera view. Moved PlayerStart to the side aisle and added rack-column
  clearance assertions for all nine profiles. Final build for this correction:
  `20260909T095412Z-7a5570fe`; final 13-test Automation PASS:
  `20260909T095418Z-dd132420`. The preceding three-resolution capture predates
  this spawn-only correction; do not describe its camera position as the final spawn.
- Normal OS clicks in the first visual-refresh Shipping package loaded the QA
  company, entered Garage, returned to city, saved and closed. The 17:42 QA company
  has $4,688 and an installed Terra T1. This uses a separate `-userdir`, not the
  user's Player directory. Subsequent candidate verification is recorded separately.

### Final local Shipping preview of this pass

- Package `outputs/Neuron-visual-refresh-20260909-Windows-v2/Windows` built/cooked/
  archived successfully. Log: `Saved/Verification/package-visual-refresh-20260909-v2.log`.
- Real OS input loaded the previous QA save, entered Garage at the new clear side
  aisle, saved inside it, closed the process, started the same exe again and used
  Continue. The room, $4,688, paused 17:42 state and -$206/h were restored visibly.
  The final test window is left paused in this isolated QA company.
- Shipping binary SHA256:
  `5c191bd2ea850fee12128275f44c3c6c91c117999c8925478961801195932210`.
- Local zip `outputs/Neuron-visual-refresh-20260909-Windows-x64.zip`:
  432,596,562 bytes, SHA256
  `2b7eeab392e690e86981b8d06b9d351b9b6ae420f9a2ffcf5abd8d0e2604e17e`.
  All 32 archived files were decompressed to streams and SHA256-compared with the
  packaged files, with no mismatches. This is a local preview, not a new release.
- No GitHub release/main update is claimed: full migration acceptance remains
  incomplete (full dashboard parity, complete campaign route and broad playtesting).

This does not claim full browser parity, newly modeled city assets, all campaign
endings, or completion of the wider migration. Packaging/playtest results below
must identify the exact candidate; previous packages are not silently updated.

## Later local pass (08:00–09:00 UTC): supersedes the limitations below where noted

This remains an integration candidate, not a completed 1:1 browser port or a
replacement of the published 0.0.1-beta. The earlier pass below is historical.

### Implemented and inspected

- Opened the live original in the Codex in-app browser. Created a QA company,
  bought Garage, inspected the infrastructure grid and training dashboard.
- Native toolbar now has separate cash/profit metrics, a flexible spacer and
  vector-drawn icon controls. Strategy selection, location cards and server grid
  were moved closer to the original. Other dashboards still lack full layout parity.
- Imported seven furnished source FBX interiors with explicit opaque, non-emissive
  PBR materials and mesh collision. Nine server profiles load these rooms; the two
  overseas sites reuse the appropriate larger server-room models.
- Replaced the visible checker graybox with authored furnishings, a closed ceiling,
  broad interior lights and first-person view. Added a solid perimeter backup and
  retained out-of-bounds recovery. Removed city labels inside rooms and unreadable
  3D Cyrillic interaction labels; contextual hints are native UMG.
- Added real rack/front-panel visuals only for installed equipment. Empty slots
  remain query targets aligned with the authored placement bays.
- Packaging now imports required interiors before cook. Fixed a discovered Shipping
  failure by moving ProjectPackagingSettings into DefaultGame.ini (the UE settings
  class is config=Game). A source regression checks both required cook directories.

### Executed checks (Windows, UE 5.8.2)

| Check | Result / local evidence |
| --- | --- |
| Final Editor build | PASS, `20260909T083348Z-be52199e` |
| Unreal Automation | 13 PASS, `20260909T083356Z-afd40ff2` |
| GPU semantic UI + actual process restart | PASS at 1280x720, 1600x900, 1920x1080; `20260909T083412Z-c8db1c0d` |
| Native core, fresh MSVC binary | 46 cases, 1,355 assertions; 336 generated fixture quotes |
| Native campaign | 46 cases, 1,611 assertions |
| Rules / QuickJS parity | 568,852 assertions, 3,418 trace steps, 24 scenarios, 2 strategies |
| Durable persistence | 27 assertions; separate-process export/resume/recovery passed |
| Cross-language | 347 tests PASS |
| Browser regression | 388 tests PASS |
| Root + Rules TypeScript | PASS |
| Browser production build | PASS (large-chunk warning remains) |
| Python source, after packaging regression | 35 tests, 1 skipped, no failures |
| Python tools | 16 tests PASS |

Build/capture results are in ignored `MakeYourAI/Saved/Verification`; source/native
logs are in ignored `Unreal/Tests/.out`. Runtime freeze Rules bundle SHA256:
`9a0abba660c7cd0de444bfd0caffa1e388a9dcb35bb80ca2defc94a64d361c3f`.
The packaging INI/source test changed after the runtime freeze; no C++ or Rules
behavior changed as part of that packaging correction.

### OS-input playtest and bounded measurements

Using normal window input, continued the QA save, entered the furnished Garage,
pressed E to open procurement for cell 3:2, waited for delivery, installed a rack
and Terra T1, paused and saved. UI showed 1 compute and 2/3 kW. This is a real input
check, not proof of all 32 campaign steps or visits to all nine rooms. The UI tool
does not support sustained WASD/RMB hold; continuous walking/orbit is not claimed
as OS-input verified. Isolated-world Automation tests cover room construction,
collision and escape recovery for the nine profiles.

A ~16-second Editor-game Garage/modal sample measured process RAM about 3,182 MiB
and mean CPU 5.95% across 24 logical processors. Whole-device GPU utilization was
32–33%, about 3,053–3,078 MiB VRAM, 170–171 W, 53 C. Other apps were open; these are
not exclusive game GPU measurements or established minimum requirements. Capture
frame windows include transitions and must not be presented as steady benchmarks.

### Still not complete

Full original dashboard layout/interaction parity; the full 32-step campaign and
endings via UI; sustained movement/orbit input checks; all nine OS-played interiors;
authored NPC presentation; broad hardware/performance qualification. No updated
release or main-branch publication is implied by local test success.

Base: `6f03e1bf85596f86dcbbafceec0d389537417e61`. This is partial work, not a
finished browser-equivalent vertical slice. Existing 0.0.1 beta is not replaced.

## Changes

- Nine server-location profiles accept walking, including both data centers and
  overseas server sites. Greenhaven has no server grid and is not enabled.
- Runtime room dimensions and rack counts follow the location catalog. Room
  interaction points and warehouse dispatch use the current location, not Garage.
- Solid perimeter walls plus a capsule-position recovery guard prevent an escaped
  or fallen character from remaining outside the room. Recovery uses its spawn.
- Manual review accepts a desk in any owned server room; advisor remains Garage-only.
- Native menu copy and 460px width, location-card 272px width and smaller text are
  derived from local browser sources. Long marker labels no longer auto-wrap.
  This is NOT complete layout, icon, font, panel or interaction parity.
- Source plugin test ignores disabled plugins. The source hash manifest was
  mechanically refreshed; matching hashes are not represented as test evidence.

## Executed checks

- UE 5.8.2 Editor builds succeeded: `20260909T075040Z-4dd2f278`,
  `20260909T075150Z-7fac6dd2`, `20260909T075527Z-5e471ca7`.
- 13 Unreal Automation tests passed: `20260909T075048Z-ffc147aa`.
  The real-world room test builds all nine profiles and tests capsule recovery
  from X/Y escape and below-floor positions. These are isolated physics tests,
  not a player visit to all nine locations.
- Python source suite: 34 tests, 1 skipped, no failures after corrections.
- TypeScript typecheck passed; browser regression suite: 388 tests passed.
- GPU semantic UI and process-restart check passed at 1280x720, 1600x900,
  1920x1080: `20260909T075156Z-b40f78d5`.
- Additional real game semantic flow entered Garage, captured its rendered room,
  returned to city, opened equipment/procurement and saved successfully:
  `20260909-walk-smoke-v1`. This additional run did not itself run a resume pass.
- Native standalone MSVC campaign test launch was attempted but shell quoting
  failed before compilation; no native campaign PASS is claimed for this change.

## Visual review and remaining work

Reviewed actual Unreal PNGs for menu, city and Garage. The Garage is still a
checker-material graybox with placeholder avatar/interaction meshes, not an
authored finished interior. The toolbar is not arranged like the browser original.
The requested 1:1 UI and finished attractive rooms are NOT complete.

Browser UI tools were unavailable in preceding turns; this pass used local source
CSS/React instead and did not bypass browser safety checks. No live browser visual
comparison, OS-pointer walkthrough of all interiors, fresh Shipping package, full
32-step campaign pass or new release is claimed. Local evidence is under the
project's ignored `Saved/Verification` directory. No new GitHub publication was
made because the user's visual acceptance condition is not satisfied.
