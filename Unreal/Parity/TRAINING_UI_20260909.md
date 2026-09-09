# Native training interface — 9 September 2026

This is an incremental UI pass after `55f5a4215632f3a5ee20913fa83f836ff99a8c1d`,
not a declaration that the full game or browser migration is finished.

## Implemented

- Native Slate training ring with actual run progress, percentage, and an idle
  dash instead of simulated animation. It uses no browser/webview dependency.
- Four GMI category cards, separating effective scores from purchased starting
  scores. Earned IQ remains a separate headline, matching the browser's meaning.
- Five visible domain choices with selected states and unchanged store actions.
- Named data offers, price/volume, provenance guidance and cash-aware availability.
- Russian queue heading; training cannot start without an online model and data.
- A compact summary/category layout, revised after inspecting the initial
  1280x720 capture. The long page still scrolls at this resolution; not every
  action fits into the initial viewport.
- GPU verification now also scrolls to the training controls and captures them,
  rather than treating a screenshot of the page header as coverage of the ring.

## Checks

- TypeScript: PASS; generated rules bundle SHA256
  `4564fdb21103641405a5e873c5d069cb419ee0b4564e055afe93484ea7cab14d`.
- Final Editor build: `20260909T111239Z-9ad2fb57`, PASS.
- Automation: `20260909T111328Z-f18db310`, 13 tests PASS.
  Repeated after the domain persistence correction as
  `20260909T112431Z-c6d78b5b`, also PASS.
- Native/browser parity: `training-ui-rules-parity-20260909.log`, 568,852
  recursive comparisons, 3,418 trace steps, 24 scenarios, zero failures.
  Additional assertions cover the initial active ring value, disabled running
  action, four categories and all five domain button dispatches. Their calls
  do not change company state or RNG.
- Python tools/source checks: `training-ui-source-20260909.log`, 38 tests,
  one skip, OK.
- Final GPU run: `20260909T111345Z-6466a76a`. The 1280x720 header and scrolled
  training-controls PNGs have been inspected, along with the 1920x1080 controls
  image. All 1280x720, 1600x900 and 1920x1080 capture/resume processes completed,
  runner exit 0. Each capture now contains 14 PNGs.

Paths for checks above are relative to `Unreal/MakeYourAI/Saved/Verification`.
These checks are not a full pointer-driven campaign playthrough.

## Remaining scope

Inline browser-style quantization, richer team/technology sections and full
campaign coverage remain. Source city geometry is still stylized; this UI pass
does not claim an AAA art overhaul, new performance measurements, or a fix for
every flicker. No new GitHub release or main push is asserted here.

Personal save SHA256 remained
`794f4ef1e316703c7ed56aeda53112436b319e0308b58f4b58e74bf851b7784f`.

## Shipping pointer checks and discovered regression

First candidate: workspace `outputs/Neuron-training-ui-20260909-Windows`,
BuildCookRun PASS, 67.88 seconds, `package-training-ui-20260909.log`.
Game binary SHA256:
`7f02d2ee7e5d67a9de69c60bca444f92fb965bb92f04e4824ccf9d152a3ba1b0`.

Using the Windows computer-use skill, loaded a **copy of a QA save** in the
separate `outputs/Shipping-training-ui-QA-20260909` user directory. Clicked
Training, scrolled, selected Coding, bought one 100-unit unofficial lot for
5,000 virtual dollars. Cash changed from 5,663 to 663; repeated purchase became
unavailable. Save, real process close, launch and Continue restored cash, pause,
day 1 08:42 and the queue entry `Неофициальные · Код · 100 ед.`.

This exposed a pre-existing load bug: the domain **picker** reset to General,
although the purchased lot retained Coding. Fixed validation and restoration
of `ui.domain` in `kernel.ts`, preserving compatibility with old saves that
omit it. Added valid round-trip and invalid-domain rejection assertions. The
first Shipping candidate does **not** contain that fix. Also changed the ring's
idle caption to “Данные в очереди” when a queue exists.

Clicking Start in this QA session displayed the existing requirement to use the
Garage desk; it did not start a training run. The successful-Save toast still
needs improvement. These are not asserted as fully resolved gameplay UX.

## Corrected Shipping candidate

`outputs/Neuron-training-ui-20260909-Windows-v2/Windows/MakeYourAI.exe`
(workspace-relative) packaged successfully in 52.53 seconds; log
`package-training-ui-v2-20260909.log`. Native game binary SHA256 is unchanged
from the first candidate because this correction changes the packaged rules,
not C++ code. The rules bundle hash is the final hash listed above.

In the v2 EXE, used ordinary Windows UI clicks to load the QA campaign, select
Coding, Save, close the process, relaunch and Continue. **Coding remained
selected** on the restored training screen; cash 663, paused day 1 08:42 and
the queued-data caption also persisted. The game remains paused in this isolated
QA directory. This is the direct Shipping verification of the persistence fix.

The three-resolution automated captures predate this last caption/persistence
correction; the final rules were checked again by parity, Automation and this
1280x720 Shipping pointer test. No claim of a new full 32-step campaign pass.
