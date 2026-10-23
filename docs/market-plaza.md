# Market plaza

Two paved market frontages replace the empty ground between the outer track
barrier and shops, at circuit samples 0–26 and 92–118. A continuous narrow grated
channel separates the plaza from the existing sidewalk. The road and barrier
collision remain owned by the circuit.

The plaza uses two new ImageGen-to-Tripo assets:

| Asset | Tripo task | Credits | Runtime triangles | GLB bytes |
| --- | --- | ---: | ---: | ---: |
| Paving | `22fb5dd8-a051-47bc-95ea-5fdba7bdd340` | 60 | 3,500 | 4,333,692 |
| Drain | `9536133d-30e4-48ae-a4a8-474543aa440e` | 60 | 5,999 | 3,617,872 |

Both jobs completed successfully; total usage was 120 credits. References and
raw results are retained in `artifacts/model-input-images/kairox-plaza-*` and
`artifacts/model-jobs/kairox-plaza-*`. Reference prompts are in
`docs/plaza-prompts.json`. Runtime assets and sanitized provenance are under
`assets/runtime/models/plaza/`. Records retain the provider rights status.

`scripts/prepare-plaza.py` runs in Blender, fits the two modules to metric
dimensions, reduces geometry and embeds 1024px maps. `scripts/inspect-plaza.py`
renders each prepared asset from three directions. No additional provider jobs
are needed to rebuild the prepared kit from the retained source files.

`src/market_plaza.cpp` conforms 156 paving modules and 26 drain modules to the
track curvature. Textures are shared, and the foundation covers irregular mesh
edges. The existing sidewalk reaches 13.95 metres from the centreline; the drain
starts at 14.05 metres and the paving at 14.5 metres. Paving reaches a maximum
height of 0.215 metres. Market props within the footprint are lifted to match.

Most paving uses roughness 0.92. Six small irregular damp overlays use feathered
alpha and static water shading with inexpensive environment reflections.
Planar city captures are disabled for these small accents. Reflections
strengthen at grazing angles; dry paving does not request reflection shading.
The patches cover less than two percent of the plaza footprint. No rain, large
puddles or whole-ground mirror effect is added.

The two models join the existing hash-checked download queue. Readiness waits for
both imports. Failed downloads keep the retry path available; malformed or
duplicate imports cannot partially install another plaza.

Validation:

- Native `racing_market_plaza` checks source contracts, installed counts, road and
  sidewalk clearance, height, normal validity, shared maps and atomic imports.
- The existing native gameplay suite also loads the plaza before starting races.
- `scripts/market-plaza-smoke.cjs` tests WebGPU and WebGL2 serially against a frozen
  generated site, corrupts one drain response, retries it, checks hashes, captures
  both market frontages and close reflections, and drives a complete lap.
- Browser reports and screenshots are in `build/evidence/market-plaza/`.
- Authoring cameras use `?plazaReview=0` / `1` for the two frontages and `2` / `3`
  for damp-patch inspection. These cameras are only active with that query flag.

The validated engine is the selected clean submodule, commit
`9b597da84d4582973d010fa9ba3e328c20181892`, built with Emscripten 6.0.11.

On 2026-10-21 the native plaza and gameplay checks passed. The browser run
completed valid laps in WebGPU (34.95 s) and WebGL2 (34.72 s), both with zero
recoveries and no browser errors. Both renderers passed the corrupt-download
retry check. The exact tested frozen site was copied to the existing port 8096
preview; its WASM SHA256 matches the validation snapshot.
