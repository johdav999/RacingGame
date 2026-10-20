# Particle FX integration (2026-10-17)

Development WebEngine adds optional `ParticleShape::{Box,Sphere,Disc,Cone,MeshSurface}`,
`radius`, `coneAngle` (radians), `speedMin/Max` and sphere `surfaceOnly`. Shape position
adds to existing box position ranges; speed adds to random velocity. Cone/disc axis
is emitter-local +Y. MeshSurface uses immutable static `emissionMesh` triangle area
and geometric face normals, with a per-emitter derived CDF; no skin/morph/LOD sampling.
Validate before replacing definitions; preserve active state on failure. Publish all
shared definition/mesh/texture resources immutable.

Motion remains CPU-owned in the scene variable tick: optional `drag`, `wind`,
`turbulence` per-axis sinusoidal acceleration amplitude, `turbulenceFrequency`, and
`velocityOverLife`/`forceOverLife` normalized-age `VectorKey` curves. Wind is drag's
target velocity and has no effect at zero drag; velocity curves add displacement
velocity, force curves add acceleration. Fields use selected local/world simulation
space. Constant gravity/drag integrate analytically; curves/gusts use midpoint steps
near 1/60 second capped at 128 per live particle/update. Large updates are approximate;
varying fields are not exactly invariant to frame partition. Restart repeats the seed.

Opt into `ParticleAlignment::Velocity`, `stretch` seconds and `softDistance` world
view-depth units. Per-camera stretch preserves width and adds worldSpeed*stretch to
length; zero/projected-zero velocity uses a stable square. `FxFrame` retains alpha
order and now emits 112-byte WebGPU instances; fallback retains individual draws.
Soft fading uses opaque World depth on both renderers and attenuates alpha/additive
radiance. Clear depth retains opacity; transparent meshes/particles do not supply
soft depth. It shares water's per-view opaque color/depth snapshot and excludes water
requiring scene inputs. No physics or picking state depends on these choices.

Active masked soft emitters conservatively request existing scratch targets, even
when paused/drained. Disabled subtrees/absent masks release canvas-size storage if
water/underwater also do not need it. Views snapshot before transparency with 1x/4x,
perspective/orthographic, resize and recovery support. Soft-only views add capture
and continuation passes plus final 1x presentation; color is captured alongside depth.
HDR/bloom, collision, fluid solvers and GPU simulation remain extensions.
For the delivered FX editor and isolated preview see [FX authoring](fx-authoring.md).

Format 17 archives retain definitions/static mesh references and live turbulence
phases; CDFs rebuild on load. Formats 1–16 keep legacy defaults (Box, Camera, new
motion/fading off). Clone/editor Play use the same codec. Deploy host and worker
together. Read engine `docs/fx.md`, `docs/fx-validation.md`, README FX contracts and
`architecture.md`; verify availability in the consuming game's selected engine pin.
The new playground preset is **Visual effects → Fountain and mist** (`fountainEffect`).

Build via `./scripts/build.ps1 -Configuration Release` using the repo `.tools/emsdk`
and `.emscripten-version` (6.0.11). Build/run CTest `fx`, `scene_archive`, `persistence`,
`water`, `terrain`; Windows native compiler runtime DLLs may require its bin on PATH.
Run existing `scripts/fx-smoke.cjs`, new `scripts/fx-milestone-smoke.cjs` (both forced
backends) and `scripts/water-depth-smoke.cjs`, using Playwright/pngjs, Chrome and
`DEMO_URL` for the exact staged host/Wasm. Check depth fades, alpha/additive, stretch,
1x/4x, ordered views/masks, resize/recovery, archives and public controls. Desktop
Chrome evidence is not physical-mobile or sustained-performance acceptance.

Archive v18 adds optional startDelay; see [authoring](fx-authoring.md).

Ribbon mode and archive v19: see [ribbon trails](trails.md).

Plane alignment and archive v20: see [plane particles](plane-particles.md).

Static mesh particles and current archive v21: see [mesh particles](mesh-particles.md).
Soft-distance fading remains supported on quad/ribbon particles, not Mesh mode.

Particle contacts and projected decals: [integration](fx-contacts-decals.md). Collision is optional; applications own queued-impact consumption and decal lifetime.

Event-driven child effects and managed fading decals: [composition contracts](fx-composition.md) and [validation](fx-composition-validation.md). Use native fx_composition plus FX/persistence/lifecycle regressions, and scripts/fx-composition-smoke.cjs on WebGPU/WebGL2 at DPR 1 and 2. Archive v29 retains nested dependencies and live continuation; events/counters remain transient.
