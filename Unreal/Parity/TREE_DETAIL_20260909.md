# City tree geometry and explicit distance LODs

Incremental work after `ccc6142c41d03dcda0fca4e366c9cbc02aa53e1f`.
This does not claim photorealistic/AAA completion of the city or full migration.

## Scope and ownership

The source manifest contains 21,062 instances of three tree templates. The
original source files, transforms, pivots, bounds and material slot meanings
remain unchanged. New deterministic near meshes are generated as owned
MAIMSH02 derivatives under Saved/TreeDetailSources and imported into an immutable
CityV4 revision. No downloaded tree assets or third-party texture licenses.

`city_tree_detail.py` constructs tapered trunks, branches, thin closed leaves,
smooth branch normals and terminal foliage. No translucent or masked leaf cards.
Each tree has four runtime LODs: new near geometry, reduced new geometry, the
original source geometry, and reduced original geometry. Near LOD screen-size
cutoff is .10, then .008 and .003 (also affected by the camera's existing HISM
distance scale). This is not an all-distance increase across all 21k trees.

`CreateTreeDetail` checks matching material counts and audited source ownership,
refuses replacement of existing different assets, and verifies near/original
render topology before saving. The report lists both source and render triangles
per LOD. One original tree has 76 source triangles but only 48 rendered triangles
due to existing coincident vertices; preserving its source description and its
rendered count are checked separately, not mistaken for a new import loss.

## Iteration and evidence

- Geometry tests verify deterministic output for all three variants, bounds,
  material slots, finite normals, non-degenerate triangles, and closed edges.
- First import `20260909T114710Z-288ed831` failed because its Python verifier
  incorrectly compared the original 76 source triangles against 48 rendered
  triangles. The C++/Python audits now check both counts explicitly.
- Import `20260909T114915Z-4168a323` completed with revision
  `c1db82030a42c8e9c17c`, preserving all 29,395 original instances / 1,695 batches.
- Initial three-resolution GPU run `20260909T115124Z-829a752e` completed with
  restart checks, but its supposed tree close-up was occluded by a roof and is
  **not** evidence of acceptable tree art.
- The capture camera was moved to an existing West hills tree. The inspected
  `20260909T115556Z-a964229b` 1280x720 image exposed angular leaves and bare tips.
- A revised leaf shape and branch normals were inspected in
  `20260909T115957Z-6f7ecaf9`; oversized individual leaves were still apparent.
  The final iteration reduces their size and increases leaf count, while using
  the highest-detail LOD only closer to the camera.
- The verification driver now accepts `-VisualResolution` for focused visual
  iteration; default remains all three resolutions. A single-resolution run
  must not be described as a full three-resolution pass.

## Final candidate evidence

- Editor build `20260909T120324Z-bf45b463` PASS.
- Content `20260909T120332Z-efe6d794` PASS, revision `8071b6a33abbd790ee06`.
  Render triangle counts by LOD: pine 19,876 / 2,386 / 48 / 48;
  both broadleaf variants 22,204 / 2,664 / 164 / 64. The original pine
  source description still contains its original 76 triangles.
- GPU capture + process restart `20260909T120439Z-aca7cf31` PASS at 1280x720.
  Inspected `05f-tree-detail.png`: finer individual leaves and real branching
  are visible. Trees remain stylized procedural art, not photoreal assets.
- Python tools/source suite: 41 tests, one skipped, OK;
  `Saved/Verification/tree-detail-final-source-20260909.log`.
- Final Automation run `20260909T121606Z-1706ec60` completed successfully.
  Initial driver attempt `20260909T121522Z-19d05599` failed before execution
  because CMake was absent from that shell's PATH; rerun used the installed
  Visual Studio CMake path. No tool installation was necessary.
- Shipping build/cook/package PASS in 80.35 seconds;
  `Saved/Verification/package-tree-detail-20260909.log`.
  Candidate: workspace `outputs/Neuron-tree-detail-20260909-Windows/Windows/MakeYourAI.exe`.
  Actual Shipping binary SHA256:
  `68113fb7274723a948b436796a199f6ceb91cd22e5513e32d63c0ee2a0292b79`.
- Windows pointer test in that executable: Continue loaded the isolated QA
  campaign (663 dollars, paused day 1 08:42, Coding domain and queued data).
  Tab released the walking cursor; Return to city loaded the generated map.
  Wheel over city zoomed; the same wheel delta over the garage panel did not
  change the city view. RMB/MMB drag and a long flicker sweep are not covered.
- Personal Player save SHA256 remained
  `794f4ef1e316703c7ed56aeda53112436b319e0308b58f4b58e74bf851b7784f`.

## Measurement limits

One Shipping process, foreground 1280x720, paused moderately zoomed city,
20-second sample: CPU mean 12.91% of 24 logical processors; working set mean
1,350.69 MiB. Desktop GPU endpoint samples 71–72%, 2,639–2,652 MiB VRAM,
329–332 W, 70–71 C. Raw log:
`Saved/Verification/tree-detail-shipping-load-20260909.json`.
GPU data covers the whole desktop. This is neither an FPS benchmark nor a
controlled before/after comparison; do not claim a performance improvement.
The earlier Editor capture's short recent-frame windows also include transitions
and another paused QA process; they are not a sustained isolated tree benchmark.

No main push or release publication is claimed. Full migration, visual polish,
all interiors and the complete campaign remain outside this incremental result.
