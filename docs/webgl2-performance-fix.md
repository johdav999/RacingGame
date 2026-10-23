# WebGL2 ordinary-material shader performance fix

## Cause and repair

The WebGL2 fallback used one large water/layered/masked/reflection mesh program
for ordinary opaque buildings, the track and other simple surfaces. Its unused
dynamic feature and discard paths were a dominant GPU cost on Intel UHD 770 via
ANGLE D3D11. Dense real models made that existing fallback cost more visible.

A paused racing-camera probe measured roughly 70–80 ms of GPU work. Suppressing
GPU draws restored about 50 FPS; caching uniform lookups barely helped. Mipmapped
sampling did not help (13.3 FPS before/after). Compiling water/layer/ribbon features
out reached 34 FPS; also compiling inactive mask/reflection-discard paths out
reached 49 FPS, with restored general-shader baseline returning to about 12 FPS.
Those diagnostic substitutions were confined to temporary browsers.

The production repair adds a second private, context-owned WebGL2 program. It is
selected only for non-water, non-layered, unmasked draws outside reflection
passes. It retains the existing lighting, texture, fog, tint, tone mapping and
blend expressions. Water, layered and masked materials, reflected draws, ribbons
and particles use the general program. Both programs receive per-view state and
are deleted/recreated through the existing context lifecycle.

All 143 skyline locations still use the four real models. The city kit retains
164 instances and 14 hashed resources. No assets, geometry, LODs, lights, resolution
or quality settings were reduced. WebGPU and portable/native code are unchanged.

## Selected engine

Published engine commit: `619ce5e7b5fc2965754b4fd4ce0c4cf96494f018`, based on the
previously selected `9b597da84d4582973d010fa9ba3e328c20181892`.
[Draft engine PR #3](https://github.com/johdav999/WebEngine/pull/3) records the change.
The PR requires integration with the current main branch; RacingGame selects the
published tested commit directly. The adjacent development checkout was preserved.
The normal build reports `engineDirty=false` and `developmentEngine=false`.

## Matched consumer evidence

Chrome 155, 1440 x 900, DPR1, Intel UHD 770 / ANGLE D3D11, one diagnostic game at
a time. Other local apps were present, so these are scoped desktop measurements,
not sustained 60 FPS or other-device acceptance.

| WebGL2 measurement | Before | Fixed |
| --- | ---: | ---: |
| Paused fixed racing camera | 12.8 FPS | 35.5 FPS |
| Fixed camera p95 frame time | 103.2 ms | 60 ms |
| Complete driven lap | 13.5 FPS | 36.2 FPS |
| Driven lap p95 frame time | 120 ms | 60 ms |

The fixed view submitted exactly 437 draws, 1,991,100 triangles and 214 selected
LOD parts in both builds. Placements, asset hashes and canvas dimensions matched
exactly. The driven-lap gain was 2.68x. Both backends completed valid laps with
zero recoveries and no console/page errors. The WebGPU fixed view was unchanged
at 20.8 FPS; its differing lap samples are not attributed to this WebGL-only fix.

The actual compiled simple/general programs were observed during drawing. Water
continued using the general path, and forced WebGL context loss/restoration
recreated the programs with no GL error and all 164 placements retained.

Four fresh review URLs per backend cover skyline, underpass, damp plaza and close
paving. The WebGL2 skyline world-region comparison was pixel-identical; the other
WebGL2 views had no pixels differing by more than 16/255. This static comparison
is separate from the paused racing-camera benchmark, where rivals' poses differ.

Windows checks passed for all 164 placements, road/building/peer clearance,
atomic installation, shared meshes/maps and all 48 static resources' exact
full-detail surfaces, materials, monotonic LODs and automatic selection. Native
checks exercise the portable core; this browser backend change does not add a
native GPU renderer to the selected historical engine.

Raw profiles, browser reports, images and native results are retained under
`build/evidence/webgl2-material-fix/`. The original site is preserved at
`build/glperf-before/site`. The existing port-8197 preview serves the normal fixed
build copied into `build/skyline-validation-site`.

To repeat, serve the preserved baseline separately on port 8190 and run:

```powershell
$env:BEFORE_URL='http://127.0.0.1:8190'
$env:AFTER_URL='http://127.0.0.1:8197'
node scripts/webgl2-performance-smoke.cjs
node scripts/webgl2-material-views.cjs
```

Set `NODE_PATH` to the installed Playwright package directory if needed.

Final normal-build Wasm SHA256: `2286ac0a8178802242da7906e84b45738d2e8b7d83b5ca9401a22b54ebcd1c48`.
The final clean pinned build passed city-kit corruption/retry, all 14 hashes and
164 placements, and valid driven laps with zero recoveries/errors on both renderers.
