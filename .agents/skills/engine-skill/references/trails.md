# Ribbon trails (2026-10-17)

Read WebEngine docs/trails.md and docs/trails-validation.md, and verify the selected
engine pin. ParticleEmitterDefinition::renderMode defaults Billboard; Ribbon uses
one component-owned bounded ring in trail.cpp, sampled only by scene variable ticks.
Defaults: trailMaxPoints=256 (2-4096), trailLifetime=1 (>0, <=86400),
trailMinDistance=.05 (>=.0001), trailBreakDistance=5 (>=sample distance),
trailTextureLength=1 (>=.0001). All must be finite. At most 256 distance samples per
update; overload starts a strip and records drops. Capacity evicts oldest points;
expiry uses the simulation clock. trailPointCount()/trailPoint(index) inspect the
ring. particles() counts billboards only; droppedParticles() includes trail drops.

Move the source before scene.update(chosenDelta). World trails retain world samples
and birth width scale. Local trails sample the emitter origin in its parent's local
coordinates: emitter-relative movement draws; parent movement carries history.
This differs from local billboards following the emitter itself. Distance uses the
selected space's units. breakTrail() explicitly splits a stroke; teleports beyond
trailBreakDistance and resume also split it. Pause/disable freeze; disabled motion
needs explicit break when a bridge is unwanted. Stop drains sampled points; clear
and restart empty them. Duration/loop/startDelay apply. Rate/bursts/birth shapes,
forces, alignment, rotation and flipbooks do not drive ribbon geometry.

Size is full width and RGBA curves use point age/trailLifetime. Full texture repeats
along accumulated U length; V spans width. SceneFrame carries portable RibbonDraw
segments, FxFrame builds camera-specific six-vertex quads with shared tangent joins
and camera-parallel fallback. Alpha segments sort with meshes/billboards by center;
additive follows. Both renderers provide World depth test, masks, fog and optional
softDistance using the existing opaque-depth capture with its receiver limitations.
WebGPU retains per-view buffers and skips static uploads; GL streams batches.
FrameCounters exposes trailSegments/trailDrawCalls/trailUploadBytes. No GPU types
enter portable APIs. No collision, decals, per-particle subtrails, mesh/ground-aligned
particles, fancy caps or miter joins are delivered.

Editor Ribbon trail preset/group reuses immutable EffectAsset edits, curves,
textures, undo/redo and .wefx. EffectPreview owns a separate scene and optional
source orbit (setTrailMotion / Move trail source checkbox); never move authoring
nodes for preview. Both local/world preview motion moves source children. Archive
v19 retains definitions plus full live ring/sampler; versions 1-18 default Billboard.
Exact undo retains runtime state while fresh handles rebuild; deploy matching host
and worker codecs. Preview motion controls are transient.

Use repo-local .tools/emsdk and .emscripten-version with scripts/build.ps1 Release.
Build native trail_tests, fx_tests, editor_fx_tests, persistence_tests,
render_frame_tests; run CTest trail/fx/editor_fx/persistence/render_frame with native
compiler bin on PATH when needed. Run scripts/trail-smoke.cjs (DPR 1 and 2),
scripts/trail-depth-smoke.cjs, editor-fx-smoke.cjs and fx-milestone-smoke.cjs on the
exact staged host/Wasm with Playwright/pngjs, NODE_PATH, BROWSER_EXECUTABLE,
DEMO_URL and EVIDENCE_DIR. Desktop evidence does not establish physical mobile,
non-Chromium browser acceptance or sustained performance.

Plane alignment and archive v20: see [plane particles](plane-particles.md).

Static mesh particles and current archive v21: see [mesh particles](mesh-particles.md).
Soft-distance fading remains supported on quad/ribbon particles, not Mesh mode.

Explicit SurfaceTrail now accepts supplied world contacts/normals with gaps and
camera-independent tangent-plane geometry. It leaves Ribbon alignment unchanged.
See [surface trails](surface-trails.md) for receiver policy, v28 live-normal
persistence and current writer/validation guidance.
