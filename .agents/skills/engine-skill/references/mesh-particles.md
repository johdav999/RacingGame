# Mesh particles (2026-10-17)

Read engine docs/mesh-particles.md and docs/mesh-particles-validation.md. Verify
availability in the consuming game's selected engine pin. ParticleRenderMode::Mesh
uses the existing bounded seeded pool; particleMesh is a shared immutable static
Mesh, distinct from emissionMesh birth sampling. No node/component/GPU mesh per
particle. Max one million vertices / three million triangle indices, valid material
parts, finite positions/normals/UVs, no skeleton/skin/morph/Water material, and
maxParticles * max(1, partCount) <=1000000. Invalid settings retain current state.

meshRotationMin/Max are ordered finite XYZ Euler radians, meshAngularVelocity is
finite XYZ radians/sec, defaults zero. Sampling consumes RNG only in Mesh mode.
Existing rotation/angularVelocity add local Z rotation. Rotation uses Rz*Ry*Rx,
not rigid-body angular dynamics. Local pose follows current hierarchy. World pose
captures full birth linear matrix including scale/mirror/shear, with independent
simulated world translation. Size curves uniformly multiply geometry, without
multiplying billboard sizeScale twice. Singular/non-finite poses and zero size
suppress draws without changing live simulation. Culling/LOD never drives ticks. CPU pool records now contain mesh rotation/birth
matrices in every mode; account for the increased per-particle storage.

Mesh material ranges/maps/scalars are retained. Age RGB multiplies base color;
alpha multiplies opacity. meshTextureOverride=false preserves mesh base textures;
true uses FX texture on every slot. Native mesh UVs are used, not flipbook cells.
Alignment/stretch/plane offset do not drive Mesh mode. Nonzero softDistance is
rejected explicitly; it remains a quad/ribbon capability in this increment.

Alpha/Additive force that blend. Mesh-only Opaque preserves authored material alpha
modes; color-curve alpha<1 promotes non-additive parts to blended fading, including
formerly masked parts. Shadow casting is off; normal World lighting, material shadow
reception where supported, fog, masks and depth testing remain. No collision, picking,
rigid body, contact queries, terrain projection or decals are introduced. Fragments
can pass through the floor. The application owns source placement/contact policy.

mesh_particle.cpp emits SceneFrame MeshDraws/layer indices with shared resources,
normal matrices, bounds and visibility. RenderFrame instances compatible opaque
parts (opaque reordering is opted in); FxFrame sorts transparent parts with meshes,
quads and trails. WebGPU reuses instancing; GL retains per-instance submission and
existing material/light limitations. No shader/pass changes. meshParticles counts
collected live records per view; mesh/object metrics cover draws/uploads.

Editor Mesh debris preset and Rendering mode Mesh particles use existing immutable
EffectAsset replacement, undo/redo and isolated EffectPreview. Mesh particles group
selects imported Assets GLB/FBX geometry, XYZ rotation ranges/spin and base-texture
override. Choose geometry and zero soft fade before switching to Mesh; Opaque blend
must return to Alpha/Additive before switching back to quads/ribbons. Missing meshes
and unsupported settings preserve the last valid effect. Preview never ticks source.

Archive v21 embeds particle geometry/material/texture dependencies, rotation settings,
texture override and live rotation/birth basis. Versions 1-20 retain old modes and
zero/identity defaults. Scenes/assets/.wefx/Play/cloning/application envelopes share
the codec. Resources remain immutable/shared; saved identities persist, handles
refresh. Deploy matching host and isolated-worker codecs.

Build scripts/build.ps1 -Configuration Release using repo-local .tools/emsdk pinned
by .emscripten-version. Native mesh_particle_tests plus plane_particle_tests,
trail_tests, fx_tests, editor_fx_tests, scene_archive_tests, persistence_tests,
render_frame_tests; CTest mesh_particle/plane_particle/trail/fx/editor_fx/scene_archive/
persistence/render_frame. Compiler bin may need PATH on Windows. Browser suites
scripts/mesh-particle-smoke.cjs and mesh-editor-smoke.cjs (DPR1/2 using
DEVICE_SCALE_FACTOR), plus affected quad/plane/trail/authoring regressions. Use
Playwright/pngjs, NODE_PATH, BROWSER_EXECUTABLE, DEMO_URL and EVIDENCE_DIR on exact
staged host/Wasm. Desktop Chromium is not physical mobile or sustained performance.

Particle contacts and projected decals: [integration](fx-contacts-decals.md). Collision is optional; applications own queued-impact consumption and decal lifetime.
