# Plane / ground particle alignment (2026-10-17)

Read engine docs/plane-particles.md and docs/plane-particles-validation.md. Verify
availability in the selected game pin. ParticleAlignment::Plane extends existing
pooled Billboard particles; Ribbon ignores alignment. planeNormal defaults {0,1,0},
is emitter-local at birth, finite/nonzero (length >=1e-6, finite squared length) and
is normalized by runtime. surfaceOffset is a finite signed world-unit offset along
the rendered normal, default zero. Offset changes collected presentation, sorting,
fog and fade positions, not simulation positions/motion/collision.

Local particles transform normals with the current inverse-transpose hierarchy
matrix. World particles capture the transformed normal at birth. Nonuniform scale
retains correct orientation; size keeps existing largest-axis scale policy. Singular
local transforms suppress plane draws; singular world births drop/count emissions.
The stable normal basis has a reference-axis threshold, so continuously changing
local normals can change texture phase. Authored rotation/angularVelocity spin in
the plane. Velocity stretch remains Velocity-only. Plane mode does not constrain
motion or project onto surfaces. Applications own contacts and placement. For a
world normal use an identity source transform, or convert through transpose(world
linear matrix) to emitter-local normal space. Publish definitions immutable.

FxFrame creates camera-independent axes; ParticleInstance grows from 80 to112 bytes.
WGSL and GL CPU expansion use the same basis and existing atlas, curves, blending,
fog, masks, depth test/no writes and optional softDistance. Existing particle counters
include planes; no new captures/passes except the existing opt-in soft depth. Small
surface gaps can soft-fade strongly; choose offset/fade distance together. This is
unlit transparent geometry without curved-surface conforming, decals, terrain
projection, physics queries or mesh particles.

Editor Ground ring preset supplies a procedural expanding ring. Appearance Alignment
adds Plane / supplied normal; Plane alignment controls expose normal XYZ and signed
offset. Existing immutable history, curves, texture/atlas and rotation/spin controls
apply. EffectPreview owns its isolated scene/floor and never edits/ticks the source.
Archive v20 retains normal/offset and normalized live birth normals; prior versions
retain old alignment and up/zero defaults. Scenes/assets/.wefx/Play/cloning/application
envelopes share the codec. Deploy host and worker together; malformed fields preserve
valid state. Existing camera and velocity defaults remain unchanged.

Build scripts/build.ps1 -Configuration Release with repo-local .tools/emsdk pinned by
.emscripten-version. Native targets plane_particle_tests, fx_tests, trail_tests,
editor_fx_tests, scene_archive_tests, persistence_tests, render_frame_tests; CTest
plane_particle/fx/trail/editor_fx/scene_archive/persistence/render_frame. Put compiler
bin on PATH if needed on Windows. Browser: scripts/plane-particle-smoke.cjs and
scripts/plane-editor-smoke.cjs (DPR1/2 via DEVICE_SCALE_FACTOR), plus existing FX,
authoring and trail regressions. Use Playwright/pngjs, NODE_PATH,
BROWSER_EXECUTABLE, DEMO_URL and EVIDENCE_DIR with exact staged host/Wasm. Desktop
Chromium checks do not establish physical mobile, non-Chromium or sustained performance.

Static mesh particles and current archive v21: see [mesh particles](mesh-particles.md).
Soft-distance fading remains supported on quad/ribbon particles, not Mesh mode.
