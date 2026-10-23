# Desert Solar race infrastructure

Two ImageGen-referenced Tripo assets generated and integrated on 2026-10-22.
The divider replaces the primitive edge walls; the gantry replaces the primitive
checkpoint pylons and finish bridge on Desert Solar only.

| Model | Runtime triangles | Embedded PBR maps | Runtime instances |
| --- | ---: | --- | ---: |
| Desert race divider | 3,480 | Four 1024 x 1024 PNGs | 592 |
| Desert race gate | 11,980 | Four 2048 x 2048 PNGs | 12 |

Models live under `assets/runtime/models/desert-race-kit/`. The manifest and
individual `.asset.json` files record hashes, source counts, provider task IDs,
reference hashes, preparation and rights status. Original downloads, reference
images and editable Blender scenes remain under `artifacts/model-jobs/` and
`artifacts/model-input-images/`. Exact prompts are in
`docs/desert-race-kit-prompts.json`. Generation consumed 120 Tripo credits total.

## Preparation and placement

`scripts/prepare-desert-race-kit.py` preserves the original models, orients and
bottom-centres them, reduces geometry and texture budgets, retains normal and
packed material maps, and adds cyan-only emission. It exports the GLBs and
`src/desert_race_contract.hpp`. Geometry and map hashes are checked before runtime
installation. Both sides were inspected in offline renders and in the game.

The divider repeats along the existing collision chords with a small visual
joint overlap. Its visible envelope leaves the full 20 metre asphalt width free;
the minimum sampled centreline distance is 10.49 metres. Original continuous
boundary colliders remain authoritative. All divider instances share one mesh,
material and generated LOD set; instance reordering allows renderer batching.

The gantry source is 28 metres wide, 7.23 metres high and 4.49 metres deep. Runtime
height is multiplied by 1.5 (10.84 metres) to clear the chase camera, which was
obstructed by the first integration. Matching scaled triangle collision preserves
the opening and solid feet. All twelve ordered checkpoints have physical gates;
the first marks the finish. Checkpoint rules and recovery positions are unchanged.

The pinned WebGL2 renderer uses an unmasked emissive factor for ordinary
materials. These two models override that factor to zero on WebGL2 so the
whole surface does not wash out white; their textured cyan strips remain.
WebGPU retains the masked emissive maps. This is a game material fallback,
not an engine update.

Readiness requires all fourteen Desert models. Import contract failure leaves the
model unpublished. Browser asset requests verify SHA-256 before C++ installation.

## Provider recovery

The provider returned UUID task IDs that an existing helper incorrectly rejected.
The original two successful jobs were recovered through read-only account history
and resumed without new submissions. The helper now accepts UUIDs and documented
`task_` IDs while rejecting paths/URLs. Offline tests cover persistence, GET-only
resume, invalid IDs and authenticated-provider versus unauthenticated-CDN requests.

## Validation

- Native asset tests: placements, atomic imports, shared resources, full road width,
  original 192 collision walls and actual gantry opening/foot raycasts all pass.
- Opening raycasts pass at heights through 6.5 metres for the final taller gates.
- Full native Desert races with all four chassis pass. This ran before the final
  height-only gate correction; the targeted collision test passed after it.
- Prepared GLB SHA-256, triangle counts and all eight embedded texture dimensions
  verified against the manifest.
- `scripts/desert-race-kit-smoke.cjs` checks the served game on forced WebGPU and
  WebGL2: fourteen loaded assets, 592 dividers, twelve gates, driving, recovery,
  pause, six gate views, console errors and active-scene frame times. Evidence is
  in `build/evidence/desert-race-kit/`; native logs are in `build/evidence/`.
- Browser frame measurements use desktop Chrome at 1440 x 900, default Balanced
  quality. They are local headless-browser results, not in-app or device-wide
  performance guarantees. The car is stationary after recovery during the timed
  sample, while the AI race remains active.

The review site is `http://127.0.0.1:8116/?renderer=webgpu&track=desert` while its
server is running. Its JS/Wasm are copied into `build/desert-race-site`; its assets
link to the existing runtime directory to avoid duplicating large files.
Final browser run: both backends averaged 50 FPS over approximately ten seconds,
with 20.1 ms p95 and zero frames above 33.3 ms. Corrected textures and camera
clearance were visually inspected. Served Wasm SHA-256:
`ee2ce8fce989f8373c75428e797ffa58d2ff498f02711380d57cf5dbe16d220a`.
