# Four playable chassis

The garage now selects Vektor, Bastion, Skimmer and Vector. The same imported
models appear on the five AI rivals, with geometry and decoded materials shared
between matching vehicles. Chassis choice persists across reloads and returning
from a race. Existing careers migrate to the extended save format.

| Chassis | Class | Runtime model | Geometry | Handling |
| --- | --- | --- | ---: | --- |
| Vektor | Wheeled | `models/cybercar-clean-detail/cybercar.glb` | 49,034 triangles, six parts; closed authored wheels and wheel-well liners | Existing 1,150 kg / 650 Nm setup |
| Bastion | Wheeled | `models/bastion/bastion.glb` | 73,884 triangles, six parts; closed authored wheels and wheel-well liners | 1,350 kg / 720 Nm; 36 m/s base throttle taper |
| Skimmer | Hover | `models/skimmer/skimmer.glb` | 47,314 triangles | Existing 850 kg / 9,500 N drive; 40 m/s cutoff |
| Vector | Hover | `models/vector/vector.glb` | 47,399 triangles | 800 kg / 10,200 N drive; 43 m/s cutoff |

Paths above are relative to `assets/runtime`. Throttle limits are drive controls,
not hard velocity clamps. Boost and class upgrades compose with each chassis's
base values. Vektor/Bastion share the wheeled upgrade build; Skimmer/Vector share
the hover upgrade build. The garage states this explicitly. All four are currently
available without a chassis purchase or unlock.

## Runtime art preparation

`scripts/prepare-vehicles.py` creates the three new runtime GLBs, sanitized
`.asset.json` records and `src/vehicle_art_contract.hpp` from retained source GLBs
under `artifacts/model-jobs/`. It makes no provider calls. Rebuilding assets requires
Pillow and those ignored sources; ordinary game builds use the runtime files.

The copies use meters, +Y up and +Z forward. Bastion measures 4.6 x 2.25 x 1.28 m;
Skimmer 4.6 x 2.35 x 1.0 m; Vector 4.8 x 2.35 x 1.0 m (length x width x height).
Hover width is reduced from the broad source proportions to fit the race corridor.
The existing simple gameplay hull remains separate from visual details.

Hover source triangles are retained. Bastion's fused provider wheels are removed
with clipped wheel-well apertures; body normals and UVs are interpolated at the
cuts. Four closed tire/rim/spoke/hub assemblies replace the old triangle masks.
Recessed liners close the view into the chassis. The C++ importer identifies wheels
by their four exact names, recenters them around saved pivots and binds them to
`VehicleComponent`; the liners remain fixed to the body.
The physics presentation path supplies suspension, front steering and wheel spin;
there is no independent render-clock rotation. The new hover hulls bank modestly
with the existing hover simulation. Guns sit on hood supports or the forward pods;
combat, shields, recovery and camera ownership retain the established game paths.

Three embedded body maps per new chassis are reduced from 4096 to 1024 pixels.
Bastion adds a shared 256 x 32 color/ORM wheel palette (64 KiB decoded), keeping
its existing body textures. Vektor retains the clean cybercar's original three 4K
body maps and uses the same small wheel palette; see [the player car](player-car.md).
Source models and 4K textures are retained locally.

## Bastion wheel repair — 2026-10-22

The old centroid mask put body fragments into rotating wheels and left tire
fragments on the static chassis. Rebuild with `python scripts/prepare-vehicles.py
--only bastion`; this also regenerates the combined contract while leaving both
hover GLBs untouched. The retained Tripo source is never modified.

`scripts/bastion-wheel-geometry.py` fits circular wheels to the established
0.381218 m radius, axle pivots and individual physics widths (about 0.32 m front,
0.36 m rear). Suspension remains 0.24–0.38 m, with the authored resting pose at
0.30 m and front steering at ±0.42 rad. Widths are explicit contract values, so
changes to visual mesh bounds cannot silently change handling.

`python scripts/check-bastion-wheels.py` checks every wheel's 25 closed components,
consistent winding, zero open/nonmanifold edges, removed static tire fragments
and unchanged prepared body maps. Its cylinder support bound covers every spin
angle, both suspension endpoints and the steering continuum, including 10 mm
of bump-stop overtravel. Minimum clearance is 15.4 mm front and 28.8 mm rear.

Motion review uses `scripts/review-cybercar-wheels.py` with `bastion` after its
output-directory argument. It renders 72 poses from both sides. Browser review
uses `CHASSIS=bastion node scripts/cybercar-wheel-motion.cjs`; the `wheelReview`
query enables a close camera and a one-shot physical body drop in solo time-trial
mode. Wheel transforms remain entirely physics-driven. Normal gameplay does not
enable this diagnostic. Native `racing_wheel_travel` exercises both cars at full
bump/droop and compares every visual pose with the solver's wheel pose.

Final runtime GLB SHA-256:
`b958ca02c957321ee8e1feb336874a5b1b4e477bc2818f4f806aad54effcd2c0`.
The generated Wasm and the port-8117 preview both match
`beea28884a94017856f7da0ed56d1cad2e18f9ea528ce8fe264e595b1edb89e7`.

Validation on the final repair:

- Native `racing_smoke`, `racing_desert`, `racing_chassis_races` and
  `racing_wheel_travel` pass. Stock/upgraded races, all four chassis, full wheel
  travel and fixed liner presentation are covered. These are simulation tests;
  this engine pin does not provide a native rendered game host.
- Actual WebGPU and WebGL2 motion capture reaches the 0.24 m bump stop (with
  normal transient solver overtravel) and 0.38 m droop, steering in both directions
  and rotating wheels. The body and liners remain clear of the wheels.
- `vehicle-art-smoke.cjs` passes on both backends: exact asset hashes, failed
  download/retry, all four selections, acceleration, steering, weapons/shields,
  recovery, pause, return to garage, persisted selection and narrow viewport.
- Skimmer/Vector hashes are unchanged. No provider generation was needed.

Evidence: `build/evidence/bastion-wheel-repair/`,
`artifacts/model-review/bastion/wheel-motion-final/`,
`build/bastion-native-tests.log` and `build/bastion-wheel-travel-final.log`.

The browser loads the four assets in order, verifies SHA-256 against the compiled
contracts and enables Enter Circuit only when every model is installed. A failed
asset offers retry and resumes the remaining queue. Successfully installed assets
are not imported again. C++ also rejects start requests before all art is ready.

## Save compatibility

The `NA2` snapshot adds the selected chassis after the six class upgrade ranks.
`NA1` snapshots migrate to Vektor or Skimmer according to their saved class;
credits, completed races and ranks survive. Invalid chassis/class combinations
are rejected atomically. Storage remains `neon-ascension.career.v1`, preserving
the existing save location, quota handling and changed-tab protection.

## Validation — 2026-10-21

- Selected engine: clean `9b597da84d4582973d010fa9ba3e328c20181892` submodule.
- Generated consumer site: `build/web/site`, Emscripten 6.0.11.
- Wasm SHA-256: `c39ff81e1f6173a79a5cc275cf07508bc04fa36f1987fae8ed3f1ce51a354e7c`.
- `scripts/vehicle-art-smoke.cjs` passed serially on desktop Chromium 155 on
  WebGPU and WebGL2: invalid-asset rejection/retry, exact hashes, four-chassis AI
  field, all player selections, acceleration, steering, Bastion wheel bindings
  and physics spin, front-only steer, pulse/shield, recovery, pause, return to the
  same chassis, reload persistence and a 390 x 844 garage. No recorded console,
  page or GPU errors. Desktop screenshots use 1440 x 900.
- `scripts/progression-smoke.cjs` passed on both backends after integration:
  all upgrade purchases, installed effects, old-profile migration, restoration,
  renderer switching, invalid saves, changed-tab protection and quota failure.
- Native `racing_smoke` covers exact retained triangle totals, all chassis
  acceleration, wheel bindings, stationary countdown, legacy/new save restore,
  reverse/forward behavior, existing handling/combat/lifecycle checks, and
  stock/upgraded Vektor and Skimmer races. `racing_chassis_races` separately
  exercises full Bastion and Vector three-lap races with every AI rival finishing.

Evidence is retained in `build/evidence/vehicle-art/`,
`build/evidence/progression-ui/` and `build/native/Testing/Temporary/`.
Complete-race acceptance here is native simulation; browser checks exercise the
start of races and lifecycle controls. Physical mobile devices, real controllers,
long-session fleet performance and provider release entitlement are unverified.

Repeat native checks with `python scripts/dev.py native`. For browser checks,
build and serve the generated site, set `NODE_PATH` to Playwright and `DEMO_URL`
to that server, then run the two scripts serially. Compilation alone does not
establish browser acceptance.
