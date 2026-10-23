# Desert Solar / Helios Run

Playable second circuit, added 2026-10-22. Select **Desert Solar / Helios Run**
in the garage, or open `?track=desert&renderer=webgpu` (`webgl2` also supported).
The default remains Kairox. Switching circuits reloads the scene and retains
the renderer selection, wallet, chassis and upgrades saved in this browser.

## Route and scenery

The closed, approximately **1,017 metre** circuit has 192 evenly spaced route
samples, 12 ordered sectors, a 20 metre road, three laps and six racers.
The opening solar-field straight leads into a sandstone canyon and a 2.2×
rock arch, then a broad northern bend and the blue conduit return corridor.
All four chassis, AI combat, shields, boost, recovery, payouts and restart use
the same authoritative race simulation as Kairox.

All **12 prepared Tripo models** appear in **253 placements**: intact and broken
heliostats, two receiver towers, four cliff shapes, the arch, and four conduit
pieces. The dunes use one static mesh with fine, irregular sand grain; grading
is limited to the road and working facility. Cyan/amber road edges, sector pylons,
finish markings and the existing next-gate beacon provide navigation.

Cliff bases are buried in continuous dune terrain with sloping talus and scattered
fallen stone. Overlapping faces follow the eastern/northern bends, with taller
formations behind them. Rotated footprints retain at least 13 metres of clearance
from the route centreline. Terrain uses a 3.5 metre grid; native checks sample the
rendered triangles across the full asphalt width so scenery cannot bury the road.

The solar field has connected maintenance aisles, covered cable trenches and two
service compounds containing shelters, transformer cabinets with insulators,
entrance fences and parked utility pickups. Gravel, exposed rock, sparse scrub,
wind deposits and fragments around broken mirrors add near-road detail. These are
deterministic procedural scenery, baked into material batches in 48 metre cells;
they require no new provider assets or downloads. They are outside the driving
barriers and have no service gameplay or independent collision.

Warm directional sunlight, cooler ambient light and distance haze separate the
formations. The desert has its own shared `Helios desert daylight` environment:
pale sky above, warm sand bounce below, and a reflection sun aligned with the
directional light. Its sky gradient matches the visible sky; it replaces the
default blue studio environment without changing material maps or sun intensity.
WebGPU uses this diffuse/specular lighting and the engine's directional shadow map. WebGL2 at
this pin does not support shadow maps; both backends use feathered, terrain-aligned
contact shading under rocks and equipment. Contact meshes share terrain grid
diagonals to avoid intersecting sloping ground.

WebGL2 also lacks image-based environment lighting at this pin: it displays the
matching pale sky and retains its existing ambient/directional material lighting.
The environment correction therefore applies fully to WebGPU; this is not a new
WebGL2 reflection implementation.

Imported landscape, race-kit and village components explicitly use a neutral
white instance colour. WebEngine's default primitive colour is blue
`{0.3, 0.65, 0.9}`; leaving that default multiplied the otherwise warm albedo maps
by a cyan tint on both renderers. The loaders now preserve the authored colours.
Matched browser evidence for this correction is in
`build/evidence/desert-lighting/neutral-tint/`.

The driving surface is level. Dunes and canyon berms are scenery, not physically
banked road corners. Road barriers contain cars and hovercraft. The arch has
triangle collision preserving its opening; other scenery stays outside the
barriers. The static heliostats do not articulate or render planar reflections.

Two [dune villages](dune-villages.md) use all five user-supplied building designs
in twelve placements outside the opening and western straights. Their foundations
are graded into the dune surface, with simple building collision outside the
driving envelope. The five shared models add 16.91 MiB to the selected desert
download; readiness now covers 19 environment models including the race kit.

## Implementation and budgets

- `src/race_rules.hpp`: track identity and distance-resampled desert spline.
  Road, barriers, gates, recovery, AI and minimap consume the same points.
- `src/desert_solar.*`: placements, shared landscape height, terrain-aligned
  dressing, verified GLB imports, shared meshes/maps, static LODs and arch collision.
- `src/desert_art_contract.hpp`: names, SHA256 hashes, triangle/material counts
  and metric bounds from the prepared kit manifest.
- `src/starter_scene.*`: scene selection, desert lighting, terrain and road.
- `src/browser_main.cpp`, `web/prototype.js`: track-specific asset queue and
  readiness; the race cannot start while required assets are missing.

Only the selected environment downloads and instantiates. Desert loads about
94.2 MiB of environment GLBs plus the shared vehicles, weapon and beacon; it
does not fetch the Kairox buildings, plaza, street or track kits. The importer
allows at most 64 MiB decoded maps per model (the tower has four 2048px maps).
Instances share immutable resources and use reordering/LOD support. Conduit
glow uses emissive textures, without a separate light for every segment.

## Verification

`ctest --test-dir build/native -R '^racing_desert_lighting$' --output-on-failure`
checks the actual baked cube's sun direction, warm lower-hemisphere diffuse light,
shared bake across restarts and unchanged Kairox environment. Run
`node scripts/desert-lighting-smoke.cjs` for matched village views, served Wasm
identity, driving and pause on both forced backends. Before/after evidence is in
`build/evidence/desert-lighting/`.

The 2026-10-22 lighting check passed on Chrome 155 in WebGPU and WebGL2 with
no console/page errors and matching served/local Wasm
`3f9a4a53319e159b3aab253ecf3ee0e6e82e63dd96da22e4512d493a62abbd7a`.
Matched WebGPU village views show a modest reduction in the blue cast; the
remaining ambient light and source material colours were deliberately retained.

`python scripts/dev.py native` includes `racing_desert`. Its dedicated checks
import all models through the real engine, validate readiness, reserve separated
road sections, ray-test the driving envelope including the arch, and exercise
recovery and pause. Four complete races use Vektor, Skimmer, Bastion and Vector;
each player finishes three laps without recovery, and all five opponents finish.
Combat can cause opponents to use the normal penalized recovery path.

`node scripts/desert-solar-smoke.cjs` tests the exact generated site on forced
WebGPU and WebGL2: hashes and build identity, all 12 models, no city downloads,
all four chassis with real keyboard driving/fire/recovery/pause, narrow layout,
track switching with saved career, Kairox race startup and six scenic views.
Reports and screenshots are in `build/evidence/desert-solar/`.

The native checks exercise the Windows core/physics executable, not a native
desktop rendering host. Browser checks use Chrome at 1440×900; they are not a
hardware-wide performance guarantee. No engine pin or generated source model
was changed for this integration.

The full regression run also exposed a contaminated acceleration benchmark:
collision-layer filtering prevented hull contact but not AI weapon hits. Both
benchmark runs now hold shields and assert zero received hits. Vehicle tuning
and combat behavior remain unchanged.

Original integration run (2026-10-22): native `racing_smoke` passed in 177.32s and
`racing_desert` in 100.03s; the other nine native checks passed in the earlier
regression run. Both browser reports contain all 12 desert assets and zero
console/page errors. The served Wasm hash matches the build output:
`c052d55742f1e1e9159be555cbbd5a1bbd9182630f631b5fa542d06b67a4fe0e`.

Landscape revision verification (2026-10-22): focused native `racing_desert`
passed in 123.19s, including full-footprint cliff burial, asphalt/terrain clearance,
physics clearance and complete races with all four chassis. The final browser
smoke passed on WebGPU and WebGL2 with zero console/page errors, all four chassis,
track switching, narrow layout and six scenic captures per backend. This combined
checkout also includes concurrent race-divider/gantry integration (14 loaded
models); that infrastructure is separate from the landscape dressing.

The tested snapshot is `build/desert-landscape-site`, served on localhost:8117;
its assets link to `assets/runtime` to avoid duplicating large model files.
Wasm SHA256: `3c84cdb7c237efd35d0dbf0e0a8fdecbdf5e3f14dfd2935f5f565da22ba20ab2`.
Evidence: `build/evidence/desert-solar/report.json` and backend/sector PNGs.
Visual inspection confirms the canyon and terrain changes on both renderers.
WebGL2 still renders some imported metallic/emissive tower and race-kit surfaces
overbright; its material appearance is not equivalent to WebGPU. Native evidence
is core/physics gameplay, not Windows desktop rendering or physical-device UAT.
