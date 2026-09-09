# Local continuation — 2026-09-09

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
