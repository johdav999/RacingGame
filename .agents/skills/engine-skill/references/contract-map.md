# WebEngine contract map

Paths are relative to the selected repository, not the skill. Read only relevant rows, then current headers/implementation. Use `rg --files` and `rg -n` to locate moved files, exact signatures and consumers. This is navigation, not a frozen feature inventory. Header-only names below refer to `include/engine/`.

| Task | Contracts | Source landmarks |
| --- | --- | --- |
| Scene authoring, component/hierarchy mutations, multi-selection/scale and isolated Play/Stop | `docs/scene-editor.md`, `docs/scene-editor-structure-validation.md` | `editor.hpp`, `editor_properties.hpp`, `editor_play.hpp`, `src/editor*.cpp`, `web/editor*.js`, `tests/editor*_tests.cpp`, `scripts/editor*-smoke.cjs` |
| Runtime, levels, pause and transitions | `docs/application-lifecycle.md` | `application.hpp`, `src/application.cpp`, `src/main.cpp` |
| Hierarchy, handles, lifecycle and transforms | README sections “Reusable scene objects”, “Actor identity, components and lifecycle”, “Component lifecycle, updates and settings” | `scene.hpp`, `node.hpp`, `actor.hpp`, `component.hpp` |
| Repeated objects/factories | `docs/prefabs.md` | `prefab.hpp`, `prefab_components.hpp` |
| Physical input, bindings, context stacks, virtual controls and contact ownership | README “Mouse, keyboard and mobile input”, `docs/input-validation.md`, `docs/ui-input-integration.md` | `input.hpp`, `web/input.js` |
| Game AI, navmesh revisions, clearance, recovery and perception | `docs/ai.md`, `docs/navmesh.md`, `docs/navigation-updates-validation.md` | `ai.hpp`, `navmesh.hpp`, optional `engine_ai`, `NavigationSource`, `Navigator::navigateSource`, `tests/navmesh_tests.cpp`, `scripts/navmesh-smoke.cjs` |
| AI state machines and graphical editor | `docs/ai-state-machines.md`, `docs/ai-state-machines-validation.md` | `state_machine.hpp`, `editor_ai.hpp`, `src/editor_ai_browser.cpp`, `web/editor-ai.js`, `state_machine_tests`, `scripts/editor-ai-smoke.cjs` |
| Hierarchical AI states | selected engine docs/ai-hierarchy.md and ai-hierarchy-validation.md | AiState parent/initialChild, StateMachine activePath/stateSeconds, transitioned callback, editor hierarchy forms, scripts/editor-ai-hierarchy-smoke.cjs |
| Typed AI event queues | selected engine docs/ai-events.md and ai-events-validation.md | AiEventDefinition/AiEvent/AiEventScope, StateMachine postEvent/currentEvent/eventHistory, AiControllerComponent adapters, editor-ai-events.js, scripts/editor-ai-events-smoke.cjs |
| Actor AI controllers, bindings, scene persistence and live debugging | `docs/ai-controllers.md`, `docs/ai-controllers-validation.md` | `ai_controller.hpp`, `AiBindingRegistry`, `aiControllerCodecs`, `EditorSession::setAiController`, `editor-ai-controller.js`, `ai_controller_tests`, `scripts/editor-ai-controller-smoke.cjs` |
| Typed AI blackboards | selected engine docs/ai-blackboards.md and ai-blackboards-validation.md | ai_value.hpp, StateMachineData::blackboard, AiBinding::sources, AiControllerComponent::setBlackboard, editor-ai-blackboard.js, ai_controller_tests, scripts/editor-ai-blackboard-smoke.cjs |
| Movement, jumping and grounding | `docs/movement.md`, `docs/physics.md` | `character_motor_component.hpp`, `rigid_body_component.hpp`, `collider_component.hpp` |
| Heightfields, layered materials, seamless LOD, sculpt/paint transactions, source/resident ground, collision and bounded derived streaming, source paging and asynchronous generation, paged brushes and atomic dirty writeback | `docs/terrain.md`, `docs/terrain-detail-validation.md`, `docs/terrain-edit-validation.md`, `docs/terrain-paging-validation.md`, `docs/terrain-page-edit-validation.md` | `terrain.hpp`, `terrain_paging.hpp`, `terrain_browser.hpp`, `material.hpp`, `mesh.hpp`, `render_frame.hpp`, `src/terrain_demo.*`, `web/terrain.js`, CTest `terrain` / `terrain_paging` / `terrain_page_edit`, `scripts/terrain-smoke.cjs`, `scripts/terrain-detail-smoke.cjs`, `scripts/terrain-edit-smoke.cjs`, `scripts/terrain-paging-smoke.cjs`, `scripts/terrain-page-edit-smoke.cjs`, `web/terrain-storage.js`, `web/terrain-worker.js`, `tools/terrain_demo_pages.py` |
| Collision, queries and triggers | `docs/physics.md` | `physics.hpp`, `src/physics.cpp` |
| Hinges, sliders, springs and physics grabs | `docs/physics-joints.md` | `physics_joint.hpp`, `physics.hpp` |
| Picking, actions, dragging and rotation | `docs/selection.md` | `interaction_component.hpp`, `selection.hpp`, `manipulation.hpp` |
| Follow/orbit/framing and camera collision | `docs/camera-controllers.md`, README camera contracts | `camera_component.hpp`, `camera_controller.hpp`, `orbit_controller.hpp` |
| Loading, cancellation, cache and preparation | `docs/assets.md` | `assets.hpp`, `asset_browser.hpp`, `fbx_importer.hpp`, `glb_importer.hpp` |
| Worker decode, GPU upload admission and KTX2/fallback delivery | `docs/mobile-assets.md`, `docs/assets.md` | `asset_decode.hpp`, `asset_browser.hpp`, `upload_budget.hpp`, `texture_bundle.hpp`, `src/asset_worker_browser.cpp`, `web/asset-worker.js`, `tools/texture_tool.cpp` |
| FBX/GLB imports and mesh optimization | README “FBX mesh importer”, “GLB mesh importer”, “Imported mesh optimization” | `fbx_importer.hpp`, `glb_importer.hpp`, `mesh_optimizer.hpp` |
| PBR maps, material overrides, IBL and baked graphs | README “Materials and image-based lighting”, `docs/material-graph.md`, `docs/materials-validation.md` | `material.hpp`, `material_graph.hpp`, `environment.hpp`, `texture.hpp`, `mesh_component.hpp` |
| Animated water, interaction ripples/wakes, underwater fog/absorption and caustics, planar reflections, depth-aware refraction/foam and caller-owned clock | `docs/water.md` | `water.hpp`, `MaterialShader::Water`, `MaterialInstance::water`, `Renderer::setWaterTime`, `src/water_demo.*`, `tests/water_tests.cpp`, `scripts/water-smoke.cjs`, `scripts/water-depth-smoke.cjs`, `scripts/water-planar-smoke.cjs`, `scripts/water-underwater-smoke.cjs`, `scripts/water-caustics-smoke.cjs`, `scripts/water-interactions-smoke.cjs` |
| Visible skies, spheres, atmosphere, clouds and day/night | `docs/sky.md` | `sky.hpp`, `Scene::setSky`, `SkyGeometry`, `advanceSky`, `RenderView::skyEnabled`, `tests/sky_tests.cpp`, `scripts/sky-smoke.cjs`, `scripts/sky-weather-smoke.cjs` |
| Directional shadows and lighting | README “Directional shadow maps”, `docs/shadows-validation.md` | `light_component.hpp`, `shadow.hpp`, `render_layer.hpp` |
| Particles, flipbooks and effect presets | `docs/fx.md`, README “Particles, flipbooks and reusable effects”, [integration](fx.md) | `effect_component.hpp`, `effect_presets.hpp`, `particle_emitter_component.hpp`, `fx_frame.hpp` |
| Animation, transitions, markers and root motion | `docs/animation-control.md`, README skeletal animation section | `skeletal_mesh_component.hpp`, `animation_component.hpp`, `animation_controller.hpp` |
| Animation library and graphical controller authoring | `docs/animation-authoring.md`, [integration](animation-authoring.md) | `animation_library.hpp`, `editor_animation.hpp`, `EditorSession::setAnimationLibrary` |
| Morph targets and instance weights | README “Morph targets” | `mesh.hpp`, `mesh_component.hpp`, `tests/morph_tests.cpp`, `scripts/morph-smoke.cjs` |
| HUD, menus, text and accessibility | `docs/gui.md`, `docs/ui-input-integration.md` | `gui.hpp`, `gui_browser.hpp`, `web/gui.js` |
| Music, spatial audio and activation | `docs/audio.md` | `audio.hpp`, `audio_browser.hpp`, `src/audio_demo.cpp`, `web/audio-system.js` |
| Saves, migrations and custom codecs | `docs/persistence.md`, `docs/scene-archives.md` | `persistence.hpp`, `scene_archive.hpp`, `web/storage.js` |
| Ordered camera views, viewport coordinates, visibility masks and layers | README “Reusable scene objects”, “Layered renderer”, `docs/rendering-fallback.md` | `camera_component.hpp`, `render_view.hpp`, `render_layer.hpp`, `scripts/compositing-smoke.cjs` |
| WebGPU/WebGL 2 selection, reduced capabilities and context recovery | `docs/rendering-fallback.md`, `docs/rendering-quality.md` | `renderer.hpp`, `render_quality.hpp`, `src/renderer.cpp`, `src/webgpu_renderer.hpp`, `src/webgl_renderer.hpp` |
| Device capabilities, quality budgets and page lifecycle | `docs/rendering-quality.md` | `render_quality.hpp`, `web/quality.js`, `web/app.js` |
| Culling, instancing, LOD and performance | README performance sections, `docs/mesh-detail-validation.md` | `render_frame.hpp`, `mesh_detail.hpp`, `performance.hpp`, `scripts/performance-smoke.cjs` |

Existing `tests/` and `scripts/*-smoke.cjs` supply usage/regression examples. Inspect setup and assertions rather than copying old suite counts or thresholds. `docs/puzzle-game-readiness.md` can help scope puzzle integration, but recheck its capability claims against source and do not impose puzzle rules on other games.

When evidence matters, follow companion validation documents such as `docs/application-lifecycle-validation.md`, `docs/assets-validation.md`, `docs/prefabs-validation.md`, `docs/selection-validation.md`, `docs/gui-validation.md`, `docs/audio-validation.md`, `docs/scene-archives-validation.md` and `docs/persistence-validation.md`. Their recorded results describe past scoped runs. They are not current validation of a new game or change. Ship stress/instancing investigations are examples and measurements, not game-specific engine policy.

Browser editor workspace docking, resizing and Quick Add: [integration guidance](editor-workspace.md), `docs/scene-editor.md`, `docs/scene-editor-layout-validation.md`, `web/editor-layout.js`, `web/editor-layout.css` and `scripts/editor-layout-smoke.cjs`.

- Scene atmospheric fog: `docs/fog.md`, `docs/fog-validation.md`, `include/engine/fog.hpp`, `Scene::setFog`, `RenderView::fogEnabled`, `src/fog.cpp`, shared `shaders/fog.wgsl` / `fog.glsl`, CTest `fog`, `scripts/fog-smoke.cjs`.

Editor camera flight/framing: `include/engine/orbit_controller.hpp`,
`src/orbit_controller.cpp`, `src/editor_browser.cpp`, `web/editor-camera.js`,
`scripts/editor-camera-smoke.cjs`, `docs/scene-editor-camera-validation.md` and
[workspace integration](editor-workspace.md).

Editor terrain authoring: `src/editor_terrain.cpp`, `include/engine/editor.hpp`,
`web/editor-terrain.js`, `tests/editor_terrain_tests.cpp`,
`scripts/editor-terrain-smoke.cjs`, `docs/scene-editor-terrain-validation.md` and
[workspace integration](editor-workspace.md).

Terrain layer texture assignment: `TerrainAsset::withLayers` in terrain.hpp/.cpp,
`EditorSession::setTerrainLayerTexture` in editor.hpp/editor_terrain.cpp,
`web/editor-terrain.js`, `scripts/editor-terrain-textures-smoke.cjs`,
`docs/scene-editor-terrain-textures-validation.md` and
[integration](editor-terrain-textures.md).

FX authoring: engine `docs/fx-authoring.md`, `editor_fx.cpp`, `effect_preview.hpp`,
`effect_archive.hpp`, `web/editor-fx.js`, and [integration](fx-authoring.md).

Ribbon trails: engine `docs/trails.md`, `trail.cpp`, `fx_frame.hpp`,
`tests/trail_tests.cpp`, `scripts/trail-smoke.cjs`, `scripts/trail-depth-smoke.cjs`
and [integration](trails.md).

Ground/plane particles: engine `docs/plane-particles.md`, `particle_emitter_component.hpp`,
`fx_frame.cpp`, `tests/plane_particle_tests.cpp`, plane browser smoke scripts and
[integration](plane-particles.md).

Mesh particles: engine `docs/mesh-particles.md`, `mesh_particle.cpp`,
`tests/mesh_particle_tests.cpp`, mesh browser smoke scripts and
[integration](mesh-particles.md).


Runtime terrain surface relief: `material.hpp` / material.cpp, mesh.wgsl,
webgpu_renderer.cpp and webgl_renderer.cpp; `docs/terrain.md#surface-relief-and-projection`,
`docs/terrain-surface-validation.md`, `scripts/terrain-surface-smoke.cjs`, CTest
terrain / terrain_paging / material / scene_archive. Archive v23; deploy matched workers.

Particle contacts and projected decals: [integration](fx-contacts-decals.md). Collision is optional; applications own queued-impact consumption and decal lifetime.


Wheeled vehicles and body mechanics: engine docs/vehicles.md and docs/vehicles-validation.md; vehicle_component.hpp, rigid_body_component.hpp, physics.hpp, src/physics_vehicle.inc, vehicle editor properties/preset and archive v24. See [vehicle integration](vehicles.md); CTest vehicle, Node vehicle_wasm_tests and scripts/vehicle-smoke.cjs on both renderers/DPRs.

Driving Stage 2: engine docs/physical-surfaces.md and docs/physical-surfaces-validation.md; physical_surface.hpp, collider_component.hpp, physics.hpp, terrain_paging.hpp; [integration](physical-surfaces.md). CTest physical_surface, Node physical_surface_wasm_tests, scripts/physical-surface-smoke.cjs on both renderers/DPRs.

Driving Stage 3: selected engine docs/driving-controls.md / driving-controls-validation.md; web/gamepad.js, input.js, camera_follow.hpp, camera_controller.hpp and camera_component.hpp. See [integration](driving-controls.md), CTest camera_follow, Node camera_follow_wasm_tests and scripts/driving-controls-smoke.cjs on both backends/DPRs.

Driving playback rate, editor preview and separate tuning persistence: `docs/audio.md`,
`docs/driving-audio.md`, `docs/driving-audio-validation.md`; see [integration](driving-audio.md).

Splines, swept profiles and authored route geometry: `docs/splines-routes.md`,
`docs/splines-routes-validation.md`; APIs `spline.hpp`, `route.hpp`, EditorSession;
see [integration](splines-routes.md).

- Surface-following strips/contact traces: `docs/surface-trails.md`, `docs/surface-trails-validation.md`; [integration](surface-trails.md).

Custom GUI controls and hooks: read [integration](gui-widgets.md) and selected-engine
docs/gui-widgets.md / docs/gui-widgets-validation.md; APIs live in gui.hpp and
browser semantics in gui_browser.cpp / web/gui.js. Demo: ?gui=1&guiSettings=1.

Event-driven child effects and managed fading decals: [composition contracts](fx-composition.md) and [validation](fx-composition-validation.md). Use native fx_composition plus FX/persistence/lifecycle regressions, and scripts/fx-composition-smoke.cjs on WebGPU/WebGL2 at DPR 1 and 2. Archive v29 retains nested dependencies and live continuation; events/counters remain transient.


GUI theme defaults and state styles: [integration](gui-themes.md), selected-engine
docs/gui-themes.md / docs/gui-themes-validation.md, gui.hpp and gui.cpp. Demo:
?gui=1&guiTheme=1; both-renderer/DPR acceptance: scripts/gui-theme-smoke.cjs.


GUI min/max/content sizing, alignment and grid: [integration](gui-layout.md),
selected-engine docs/gui-layout.md / docs/gui-layout-validation.md; gui.hpp,
gui.cpp, tests/gui_tests.cpp and scripts/gui-layout-smoke.cjs. Demo:
?gui=1&guiLayout=1. Keep column-selection policy in applications.

- Spotlights/headlights: `docs/spotlights.md`, `docs/spotlights-validation.md`; [integration](spotlights.md).

General region residency: selected-engine docs/region-streaming.md and
region-streaming-validation.md; region_stream.hpp, PhysicsWorld::bodyBounds,
SceneArchive v31 and EditorSession. See [integration](region-streaming.md).

Shadowed lights, sun cascades, HDR bloom/color grading and both-backend validation: [current lighting contracts](lighting-post-processing.md).

| AI task sequences | selected engine docs/ai-tasks.md and ai-tasks-validation.md | AiTaskSpec/AiTask/AiTaskBinding, controller task ownership, ai_tasks, scripts/editor-ai-tasks-smoke.cjs |
