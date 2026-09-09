# Reproducible near-city camera timing

Adds an opt-in `-CameraSweep` to Run-NativeParity's visual stage. Shipping
gameplay is unchanged; capture activation remains disabled in Shipping.
The close-day capture is followed by a 14-second sinusoidal zoom/orbit around
its ground target. First two seconds are excluded. Remaining samples use
monotonic wall time between game-thread ticks, independently of game time
scaling, with mean, p95, maximum and count above 50 ms, plus raw samples.

This is a repeatable diagnostic route, not an automatic flicker detector,
an isolated GPU timer, a broad hardware benchmark or proof of full gameplay.
It covers one near-city trajectory at the selected resolution. Capture still
performs the existing process restart/save comparison afterwards.

## Iterations

- Initial Editor build `20260909T121917Z-4cc5ced5` succeeded and visual run
  `20260909T122025Z-8a372153` completed capture/restart. Its sweep used game
  DeltaTime; it is superseded for performance analysis by monotonic timing.
- Build `20260909T122258Z-9a078586` failed on the missing SweepPreviousTick
  declaration after a partial patch attempt. Declaration was fixed; build
  `20260909T122328Z-8d14e26a` succeeded.
- Final visual run `20260909T122336Z-14488e2d`: capture and actual process
  restart completed with exit 0 at 1280x720. 721 measured ticks; mean
  16.6677 ms, p95 16.7190 ms, maximum 17.3106 ms, zero above 50 ms.
  These results do not establish visual flicker absence or performance at 4K.
- Final Python tools/source suite: 41 tests, one skipped, OK;
  `Saved/Verification/camera-sweep-final-source-20260909.log`.

The isolated old QA Shipping window was closed before the diagnostic run;
the personal Player save was not opened or modified. No new Shipping package
or GitHub release is claimed for this diagnostic-only change.
