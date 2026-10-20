# FX authoring and reusable effects (2026-10-17)

Read engine docs/fx-authoring.md and docs/fx-authoring-validation.md. Development
WebEngine now has **Scene editor → Details → Particle FX**: preset creation, grouped
emitter settings, imported texture/static mesh references, RGBA/size/vector/burst
key tables and draggable curves, draggable start-delay timeline, emitter add/copy/
remove/rename, standalone .wefx import/export and isolated preview diagnostics.
The development feature does not establish availability in a consuming game's pin.

Portable engine_editor EditorSession::selectedEffect/createEffect/setEffectAsset
publishes immutable EffectAsset/definition replacements through the existing bounded
history. Candidates validate before publication. Replacing the effect retains root
saved identity/common state and same-index emitter saved IDs/common flags/tags, sibling order and
an exact alternate subtree in history; runtime handles change and must be resolved
again. Edit resets that instance playback; undo restores prior live state. Parent
world pose and subtree/resource fingerprints guard conflicting writers. Additional
custom children on effects or emitters are rejected. Never mutate published shared definitions or
bypass history/dirty revision with DOM-owned state.

EffectPreview owns a separate Scene, camera, component, particles and clocks. Hosts
bind its stable scene()/camera() and release renderer bindings before teardown.
start/restart, pause, advance, step and isolate control only preview state. advance
accepts 0–0.25 seconds, step positive seconds up to 0.25. Browser uses presentation
delta capped at 0.1; hidden/suspended clocks freeze. Pause/Restart/Step 1/60/Isolate
and Close/Escape do not tick source simulation. Live asset changes restart preview
while preserving pause/isolation and camera address. Renderer recovery preserves
CPU state. Diagnostics report live particles, dropped emission and latest frame
particle draws/upload bytes; WebGL retains individual draws and counter limits.

The browser host owns renderer rebind-before-stop and gates full Play, scene files,
recovery and editor exit while FX preview views are borrowed. Autosave defers until
preview closes; effect export still works. Curve/timeline drafts commit once on
pointer release; Escape/blur/cancel discard drafts. Async file reads reject document,
selection and revision changes. Maintain these cancellation and lifetime contracts.

Portable engine_core EffectArchive::save/load reuses bounded SceneArchive assets:
.wefx contains exactly one EffectAsset entry with embedded shared texture/static
mesh references. It is not JSON or a scene; browser imports are capped at 64 MiB
plus decoded ArchiveLimits. Failures preserve valid state. Exported resource names
are independent of scene node names. Create/duplicate an actor for independent
playback sharing immutable definitions.

ParticleEmitterDefinition::startDelay (finite 0–86400 seconds, zero by default)
postpones the first rate/burst cycle once. Burst times remain within duration;
loops do not repeat the delay; restart resets it. Archive v18 persists delay in
scene, assets, .wefx and clones; formats 1–17 default it to zero. Deploy matching
host/worker codecs. Runtime callbacks, audio/gameplay, preview/isolation/camera,
editor selection and history do not enter .wefx resources.

Use repo .tools/emsdk, .emscripten-version (6.0.11) and
./scripts/build.ps1 -Configuration Release. Build/run CTest editor_fx, editor,
editor_content, editor_structure, editor_terrain, fx, scene_archive, persistence.
Windows tests may need configured compiler runtime bin on PATH. Run
scripts/editor-fx-smoke.cjs on the exact staged Release host/Wasm with
Playwright/pngjs, BROWSER_EXECUTABLE and DEMO_URL; repeat DEVICE_SCALE_FACTOR=2.
Run existing editor-structure-smoke.cjs (EDITOR_FIXTURE can select a native fixture)
and fx-milestone-smoke.cjs for regressions. Desktop browser evidence does not
establish physical Android/iOS or sustained mobile performance.

Ribbon mode and archive v19: see [ribbon trails](trails.md).

Plane alignment and archive v20: see [plane particles](plane-particles.md).

Static mesh particles and current archive v21: see [mesh particles](mesh-particles.md).
Soft-distance fading remains supported on quad/ribbon particles, not Mesh mode.

Particle contacts and projected decals: [integration](fx-contacts-decals.md). Collision is optional; applications own queued-impact consumption and decal lifetime.

Event-driven child effects and managed fading decals: [composition contracts](fx-composition.md) and [validation](fx-composition-validation.md). Use native fx_composition plus FX/persistence/lifecycle regressions, and scripts/fx-composition-smoke.cjs on WebGPU/WebGL2 at DPR 1 and 2. Archive v29 retains nested dependencies and live continuation; events/counters remain transient.
