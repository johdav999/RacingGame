# Kairox city ground validation — 2026-10-23

The five requested environment improvements are implemented in RacingGame using
its pinned WebEngine (`9b597da84d4582973d010fa9ba3e328c20181892`). No engine API,
dependency pin, player save or physics-surface changes were required.

## Native Windows evidence

Seven selected MinGW CTest checks passed, zero failed, in 225.41 seconds:
`racing_paint`, `racing_city_ground`, `racing_market_plaza`, `racing_track_kit`,
`racing_street_kit`, `racing_smoke` and `racing_wheel_travel`.
The full prototype smoke includes rules, physical driving, complete races,
collisions, lifecycle and existing asset integration. A subsequent visual-only
adjustment seats the parked contact stamp on its parking apron; the focused
`racing_city_ground` check passed again in 1.36 seconds after that adjustment.

The new focused test verifies:

- 332 raised promenade sections and 332 genuinely inset grated channels.
- 160 foundations, four stepped loading platforms and twelve utility clusters.
- Three parked vehicles imported from the existing validated Bastion resource,
  sharing optimized geometry, materials and distance LODs.
- 199 contact stamps, forty lamp pools and seventeen small damp patches.
- 112 ground/detail meshes, 31,480 triangles and two shared material-map sets;
  parked models and their 27 visual components are additional shared art.
- Minimum opaque ground-detail clearance of 13.9919 m from the route centreline.
- No new scenic colliders, intact foundation collider and separated old visual,
  finite geometry, normalized normals and desert-track isolation.
- No requested full-city water reflection pass.

Native tests cover portable rules, resource and scene contracts. This selected
consumer builds a browser game and native tests; it has no rendered native
RacingGame host. A native rendered visual acceptance is therefore not claimed.

## Browser evidence

The final application was built with repository-local Emscripten 6.0.11 and
served from `build/city-ground-site` at port 8139. Its SHA-256 is recorded in
`validation-manifest.json` and checked against the actual served Wasm before
acceptance. Immutable model assets reuse the existing frozen staged asset store
through a local junction to avoid a second large duplicate; runtime download
hash checks remain enabled.

Both forced renderers passed with zero console/page errors. WebGPU and WebGL2 each completed a clean 35.117 s lap with zero recoveries. The final served Wasm SHA-256 was 5fa666fa7d8021a6f220d887b2d2e51a796642e819b1c0b419fc0ce30ee54d4d.

The browser report is `build/evidence/city-ground/results.json`. Screenshots
cover the transit ground, loading extension, service frontage, parked service
vehicle, trackside paving, the transit overview, keyboard driving and race gates.
The script forces both WebGPU and WebGL2, verifies actual backend identity and
native-published assembly counts, uses actual W-key acceleration and pause/resume,
then drives a physical lap with the normal rules and collision active. Each lap
must finish cleanly with zero recoveries. Console and page errors must be empty.

Commands:

```text
cmake --build build/native --target racing_city_ground racing_smoke --parallel 4
ctest --test-dir build/native -R '^racing_(city_ground|market_plaza|street_kit|track_kit|smoke|wheel_travel|paint)$' --output-on-failure
python scripts/dev.py build --jobs 4
node scripts/city-ground-smoke.cjs
```

Contact shading and lamp receiver pools are authored static accents; they are
not a new SSAO or dynamic shadow feature. Damp patches combine environment
reflection with restrained neon streaks, without screen-space or full-city
planar reflection claims. Resource counts and desktop browser laps do not
establish an FPS gain, sustained mobile performance or physical-device acceptance.

See [the delivered feature contract](city-ground.md) for placement, material,
lighting and scene-lifetime details.

The existing preview at port 8137 serves this exact verified Wasm/JavaScript pair. Reload an already open page to adopt the new scenery.
