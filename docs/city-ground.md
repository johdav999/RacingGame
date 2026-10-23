# Kairox ground and service areas

The city ground is implemented in `src/city_ground.hpp/.cpp`. The scene now has
raised promenades, building plinths and facade aprons, stepped loading platforms,
inset grated channels, weathered slab materials, utility clusters, parked service
vehicles, contact shading and local light/damp accents.

## Geometry and placement

- 332 promenade sections join the existing imported sidewalks at 13.95 metres
  from the circuit centreline. Their top is Y=.185 m. Existing outer market
  plaza sectors retain their original paving and drains.
- 332 adjacent channels have a bed at Y=.045 m, with separated metal crossbars
  above it. The promenade has a visible vertical edge rather than a floating
  single surface.
- 160 low concrete plinths anchor street buildings, the utility room and skyline.
  Street frontages have textured aprons; four depot/warehouse loading extensions
  have raised platforms, steps and safety markings.
- Twelve frontage clusters contain service cabinets, ventilation louvers, pipe
  risers and framed cargo crates. Their placements derive from building fronts.
- Three service parking sites reject the complete road, building, city-detail
  and street-module envelopes. They reuse the existing validated Bastion model
  with roof racks, restrained equipment cases and amber lamps. Instances share
  immutable optimized geometry, materials and distance LODs; no new download is
  added and no player paint is applied to these scenic vehicles.

The opaque dressing stays at least 13.99 m from the track centreline in the
current layout. It adds no rigid bodies or colliders behind the existing crash
barriers. The original flat foundation collider remains at Y=0; only its old
plain render surface is lowered to avoid hiding the new floor at distance.
The desert track retains its existing scenery and physical surfaces.

## Materials, shading and lighting

Two deterministic, shared 512×512 material sources provide service concrete and
promenade slabs. Metric UVs retain consistent scale. Albedo includes seams,
aggregate, stains and repaired sections; normal and roughness maps support the
pinned engine's PBR path. The fallback also receives the authored albedo detail.
Resources are built once and shared across scene restarts, with no network
generation or frame-time texture work.

There are 199 static contact stamps around building bases, stair feet, support
columns, utility clusters and parked vehicles. These are authored soft shading
on receiver surfaces, not SSAO or dynamic cast shadows. Forty warm receiver pools
align with the existing streetlight and doorway emitter placements. The existing
six nearest point lights still provide local dynamic illumination; the new
accents add no point lights or changes to the global city lighting.

Seventeen small damp patches use the existing still-water material with low
ripple/reflection strength and subtle cyan streaks. They use environment
reflection plus authored neon accents. They do not request a full-city planar
reflection or refraction pass, and do not claim screen-space reflection of every
nearby object.

Opaque details and alpha accents batch by circuit sector and material, retaining
district-sized culling. The assembly has 112 ground/detail meshes and 31,480
triangles. The three imported service vehicles add 27 visual components,
including their small roof fittings; their source geometry and LODs are shared.
These are resource bounds, not an FPS or physical-mobile performance claim.

## Ownership and verification

`StarterScene` builds the immutable ground assembly once, after existing city
placements are final. The returned `Report` contains value-only counts,
authoring viewpoints and parked-vehicle placements. The scene owns all actors;
there are no retained callbacks, scene pointers or per-frame geometry rebuilds.
The existing Bastion import publishes scenic vehicles only after its normal
asset contract passes. No WebEngine API, archive, career save or dependency pin
changes are required.

Native checks: build `racing_city_ground`, then run CTest `racing_city_ground`.
The test verifies actual geometry clearance and heights, absence of added
collision, shared textures, actual imported parked-model sharing/LODs, resource
bounds, the foundation visual/collider separation and desert isolation.

Browser checks: build the site with `python scripts/dev.py build --jobs 4`,
freeze the complete site in `build/city-ground-site`, and set `DEMO_URL` for
`node scripts/city-ground-smoke.cjs`. Its `validation-manifest.json` records the
served Wasm SHA256; acceptance rejects a mismatch. The script checks both forced
backends, captures five authoring views and a transit overview, then performs
real keyboard driving and a clean physical lap with pause/resume.
`?groundReview=0` through `4` are explicit authoring cameras; ordinary racing
keeps its chase camera. Review controls do not affect simulation or saves.

See [recorded acceptance](city-ground-validation.md) for results and boundaries.
