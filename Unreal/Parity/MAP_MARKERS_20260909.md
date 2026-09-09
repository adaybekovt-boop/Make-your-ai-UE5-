# Compact native map markers

Incremental UI work after `78c0680`, not full browser/Unreal visual parity.

Seven native location markers retain the original `ui:location` actions and
world positions. Unselected markers are 42x38 logical pixels with authored
vector building/campus/server icons. Selected, hovered or keyboard-focused
markers reveal the full name; width adapts between 120 and 246 pixels.
Full names also remain in tooltips and the All locations list.

Selection is supplied by `view.selectedLocation` from canonical UI state, not
a separate widget-only selection variable. Ordering prioritizes selected markers
and then stable IDs. Collision spacing uses actual marker rectangles instead
of the old fixed 220x46 label size. This does not reposition physical sites or
claim that every marker is visible behind the left panel or outside the camera.

## Evidence

- Initial build `20260909T122903Z-c37e8d93` PASS. 1280x720 capture/restart
  `20260909T122913Z-37176f39` PASS; inspected city screenshot shows compact
  icons and selected Garage label. Final width adjustment follows this image.
- Final build `20260909T123151Z-414dcc6f` PASS.
- Final three-resolution capture/restart `20260909T123159Z-25e74745`: PASS
  at 1280x720, 1600x900 and 1920x1080. Final 720p city image inspected;
  selected Garage label is compact and unselected icons leave the city visible.
- Unreal Automation `20260909T123345Z-cacc0ad5`: all 13 tests passed.
- Rules parity: 568,852 differential comparisons / 3,418 trace steps /
  24 scenarios, zero failures. Additional UI assertions switch garage, campus,
  north and south data centers and verify canonical selected marker identity.
  `Saved/Verification/map-markers-parity-20260909.log`.
- TypeScript rules typecheck PASS; Python tools/source suite 41 tests,
  one skipped, OK (`map-markers-source-20260909.log`).

## Standalone pointer check and follow-up

Shipping `outputs/Neuron-map-ui-20260909-Windows/Windows/MakeYourAI.exe`
(workspace-relative) built successfully in 76.18 seconds. Game binary SHA256:
`595394039fb4047279942e6ced4ca4249f86391f5f2450c9a7bf9a0478233533`.
Actual Windows UI: Continue loaded isolated QA campaign, 663 dollars, paused
day 1 08:42. Moving the cursor to Campus expanded its name without selection;
clicking Campus selected it and displayed its matching purchase card.

This revealed duplicate white tooltips and a purchase button that looked active
without enough cash. Follow-up suppresses the tooltip while the marker name is
expanded, separates location price from the button label, disables purchase
when cash is insufficient, and displays the exact shortfall. Underlying purchase
commands/prices are unchanged. Rules assertions verify the price row and that
button enablement agrees with the displayed shortfall for selectable locations.

Follow-up Shipping v2 built successfully:
`outputs/Neuron-map-ui-20260909-Windows-v2/Windows/MakeYourAI.exe`.
Log `Saved/Verification/package-map-ui-v2-20260909.log`.
Rules parity 568,852 comparisons / 24 scenarios / 3,418 steps, zero failures;
Python suite 41 tests, one skipped, OK. Logs `map-ui-v2-parity-20260909.log`
and `map-ui-v2-source-20260909.log` in Saved/Verification.
The two follow-up UI changes are NOT yet OS-pointer verified in v2.
Keyboard navigation, exhaustive campaign and visual acceptance remain open.
No GitHub publication or release claim.
