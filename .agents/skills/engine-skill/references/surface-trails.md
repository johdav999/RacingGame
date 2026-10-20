# Surface-following trails (development engine, 2026-10-19)

Verify the selected checkout/game pin before using these APIs. Stage 6 introduces
ParticleRenderMode::SurfaceTrail in engine_core, sharing the existing bounded FX
ring, texture, size/RGBA curves and private renderer trail path. It is explicitly
surface-oriented, unlike camera-facing Ribbon. Include
engine/particle_emitter_component.hpp and engine/effect_presets.hpp.

Use surfaceTrailEffect() or an immutable ParticleEmitterDefinition with SurfaceTrail
and ParticleSpace::World. Call submitSurfaceContact(worldPoint, worldNormal) on the
owning thread before each normal scene variable update; last valid submission wins.
No submission means a gap. Do not add a second scene update. Games own tyre slip,
contact/query timing, receiver revision/eligibility and per-wheel emitters. There
are no implicit physics queries, retained receiver handles or terrain subscriptions.
Marks stay in baked world coordinates across receiver movement/terrain edits/page
eviction: applications breakTrail(), stop(true) or replace streams explicitly.

Preset: 512 points, 12 s, .05 m samples, 2 m teleport break, 1 m texture length,
.32 m full width, .015 m normal offset, dark alpha with final-fifth fade. Capacity
2..4096; lifetime >0..86400; sample/repeat >=.0001; break>=sample. Contact/normal
components finite +/-1e6, normal length>=1e-6; normalize on submission. Local space,
particle collision and offsets outside +/-1 m are rejected. Invalid input retains
valid state. Ordinary shared definition validation still applies.

Distance/residual sampling interpolates straight endpoints, normals and birth
times, <=256 new points per update; overflow starts a new stroke and counts drops.
Capacity evicts oldest; ages use scene variable time, pause freezes, stop drains,
stop(true)/play(true) clear, resume breaks. Missing input, opposed normals and
teleports break. Disabled state freezes/hides; break across disabled movement.
trailPointCount()/trailPoint(i) expose the bounded ring with normal/birth/distance.

SceneFrame RibbonDraw.surfaceAligned/beginNormal/endNormal feed FxFrame's
camera-independent tangent-plane sides. Degenerate/reversed sides suppress the
segment. Full-width terrain projection, miter/round caps and receiver reprojection
are absent. At most capacity segments, 2*capacity triangles, 6*capacity 48-byte
vertices per emitter/view; applications bound aggregate emitters/views. Existing
World masks, depth/no-write, alpha/additive, fog, soft fade, sort/upload/recovery
apply on both renderers. Physics is unaffected. Use DecalComponent for discrete marks.

Details -> Particle FX -> Surface trail uses immutable asset history, appearance
and size/RGBA curves plus Trails parameter controls. EffectPreview owns a sloped
receiver and orbit contact fixture with airborne intervals, not source scene or
physics queries. Host framing, move/pause/restart/step/isolate, trail point/drop and
segment/upload diagnostics are transient. Release renderer borrows before teardown.

Archive v28 adds definitions/live point+source normals with ring/clock/residual
continuation; prior trails default +Y. Current shared checkout writer is v29 due to
separate FX composition work; readers include v28. Pending contacts, receivers and
render caches are not saved. Resubmit contact before the next update to continue.
Scenes/application envelopes retain live state; .wefx retains definitions only.
Duplicate and Play reuse shared definitions with independent live state. Deploy
matched host/worker/Wasm. Keep pre-publication allocation/normal/chronology checks.

Use pinned repository .tools/emsdk (6.0.11), README build commands and selected
build's web_assets target to publish dist. Focused native CTest surface_trail plus
trail/fx/editor_fx/persistence/render_frame; explicit surface_trail_wasm_tests runs
in Node. Run scripts/surface-trail-smoke.cjs at DEVICE_SCALE_FACTOR=1 and 2;
scripts/trail-depth-smoke.cjs with SURFACE_TRAIL=1 checks surface pixels, depth,
fog, soft fade, alpha/additive, MSAA, masks, save/open and recovery on both backends.
Use NODE_PATH, BROWSER_EXECUTABLE, DEMO_URL and EVIDENCE_DIR as in build-validation.
Desktop/emulated checks do not establish physical mobile or max-budget performance.
Read selected-engine docs/surface-trails.md and surface-trails-validation.md.
