# Desert dune villages

The five user-supplied `DuneBuilding1.glb` through `DuneBuilding5.glb` now form
two separate settlements on Desert Solar / Helios Run. Sunward village stands
outside the opening straight (route sample 10); Westwind village stands outside
the western return (sample 145). Each has six buildings and uses all five designs.
They face the road in two rows, with lanes between the buildings.

## Asset preparation

`scripts/prepare-dune-villages.py` follows the repository's headless Blender 3.5
flow: import, finite geometry and UV checks, uniform metre scaling, bottom-centre
pivot, embedded texture reduction, GLB export, re-import, and front/reverse renders.
The original files remain untouched in `assets/runtime/models/buildings`.

All 42,027 source triangles are retained across the five models. Their largest
footprint dimension is 16 metres; heights range from 5.68 to 10.94 metres. Three
4096px maps per source become 1024px runtime maps. The five prepared GLBs total
17,735,752 bytes (16.91 MiB), fetched once and shared by all twelve instances.
The runtime renderer also uses the existing static-mesh LOD preparation.

Prepared GLBs and source/output hashes live in
`assets/runtime/models/dune-villages`; `src/dune_village_contract.hpp` records
their bounds, hashes, triangle and material counts. Editable Blender files and
ten QA renders live under ignored `artifacts/model-jobs/dune-villages`.
Provenance identifies these as user-supplied files; no provider calls were made.

## Integration

`src/dune_villages.*` owns the two layouts, shared terrain-grading regions and
validated imports. Flat village ground blends smoothly into the existing dunes.
The underlying foundation visual sits 10 cm below its unchanged physics surface,
preventing depth fighting where the graded terrain is only 2 mm above ground.
All building footprints are outside the barriers and have simple solid collision;
the buildings do not offer drivable interiors. Existing solar equipment, canyon
formations, conduits, ordered gates and race rules remain in use.

The five new models use desert asset IDs 14–18. Desert readiness waits for all
19 environment models; Kairox does not request village assets. Both backends use
the same prepared textured buildings. Review cameras are available through
`?track=desert&villageReview=0` and `villageReview=1`.

## Verification

- `racing_dune_villages`: real engine imports, both clusters/all five designs,
  shared meshes, complete footprint grounding against the rendered terrain grid,
  separation from other scenery, collision and driving-envelope clearance.
- `racing_desert_race_kit`: existing dividers and gantries pass.
- `racing_desert`: full desert regression passes, including races with all four
  chassis (132.95 seconds in this run).
- `scripts/dune-villages-smoke.cjs`: generated-site and source hashes, forced
  WebGPU/WebGL2, twelve placements, both village views, driving, recovery, pause
  and return to Kairox. Evidence is written to `build/evidence/dune-villages`.

Native checks exercise Windows core/physics, not a native desktop renderer.
Browser evidence is headless Chrome on this machine, not physical-device UAT.

Final browser verification on 2026-10-22 passed on both forced backends with
19 loaded environment assets, 12 village placements and zero console/page errors.
Both village views were visually inspected, including the corrected ground
surface. Original file hashes remain unchanged. The existing WebGL2 receiver
tower still appears overbright in the Westwind background; village textures are
present and readable on both backends.

Served generated site: `http://127.0.0.1:8123/?track=desert`.
Tested Wasm SHA256:
`718782b1a72f8b114de4d649a46adf9a420f49d06f98ae5e5adc43ebe4202cb8`.
The focused native village check also passed after the visual-only foundation
correction; the complete race regression above preceded that correction.
