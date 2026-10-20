---
name: engine-skill
description: Build and extend playable browser games using Johan's WebEngine C++20/WebAssembly engine. Use for game mechanics, levels, input, cameras, interactions, physics, assets, HUD, audio, saves and game integration in WebEngine, or engine extensions needed by those games. Applies to this repository's engine, not unrelated web game frameworks.
---

# Engine Skill

Turn game requests into playable WebEngine applications that compose existing systems. Keep game rules in application code; extract reusable mechanisms when a concrete mechanic needs them.

## Establish current contracts

Use the user's chosen checkout. For RacingGame, the default engine is the pinned `external/WebEngine` submodule. Use the adjacent `../WebEngine` checkout only when explicitly selected with `--engine`; it may contain unpublished work. Preserve the user's engine choice.

Read applicable `AGENTS.md`, then `architecture.md` before planning or changing engine APIs, platform/build boundaries or game/demo behaviour. Inspect `git status --short` and preserve unrelated work. Read relevant README sections and feature documents, then verify exact signatures and implementations in `include/engine/`, `src/` and `web/`. Select documents through [the contract map](references/contract-map.md).

Current source and contracts take precedence over this guide's API landmarks. Roadmaps/readiness reviews can lag implementation: verify claims before scheduling an existing system for reimplementation. Do not treat intended extensions as delivered features or expand a game request to implement unrelated services.

The supporting references were checked against the working checkout on 2026-10-11, including its uncommitted features. Recheck current contracts when using them. Read [feature integration details](references/feature-integration.md) when composing rendering backends, mobile loading, movement, precise interaction, animation or saved behaviour.

## Implement a coherent game

Infer mechanics, controls and target devices from the request. Resolve routine choices independently; ask only when missing information materially affects the result. For a new game, deliver a small playable loop with clear controls, feedback and appropriate completion/failure/restart behaviour. For an existing game, extend the requested behaviour within its established loop.

Keep levels, goals, scoring, progression, inventory, hints, move legality and undo policy in application/example code. Inspect `src/main.cpp` and CMake targets before choosing integration. Existing demo placement has documented exceptions; avoid copying those into new generic modules or reorganizing unrelated demos. Prefer a clear game-specific source boundary reusing the engine and browser host.

Compose only the systems the game needs:

- Own scenes through `Application`; assemble actors/components through `Scene::spawn<T>()`, `Actor::addComponent<T>()` and the owned hierarchy. Use `Prefab` for repeated configured hierarchies when useful.
- Map physical input to portable actions/contexts, then gameplay commands. Route GUI, camera and scene contacts explicitly so one gesture has one owner. Cancel capture/held commands on blur, context changes, transitions and target loss; keep DOM/key/touch details in adapters.
- Use character motors for command-driven movement, physics queries/sensors for collision/triggers, and selection/actions for interaction. Choose transform manipulation for eligible non-physics actors or joint/grab commands for physics actors. Avoid competing transform writers; collision layers are distinct from render masks.
- Optional `engine_ai` provides immutable `WaypointGraph` and static collision-baked `NavMesh` navigation through `NavigationSurface`, bounded A*, `PathFollower`, handle-safe sight and `DecisionClock`. `Navigator` (`WaypointNavigator` compatibility alias) owns a shared surface and motor handle, checks capsule clearance before movement, and supports bounded delayed recovery (default 64 queries/step, 256 expansions, four replans, 60 s total deadline). Call from owning actor fixed ticks before child motors and cancel on disable/detach, target loss or handoff. Include `engine/navmesh.hpp` and link `engine_ai`; private vendored Recast/Detour 1.6.0 types never cross portable headers. Scene bakes use synchronized registered static solid collision plus occupancy filtering; raw triangle bakes describe caller-defined surfaces/interiors. Defaults: .2/.1 m cells, .35 m radius, 1.9 m height, pi/4 slope, .3 m step, .5 m endpoint/vertical snap; 262144 cells, 2000000 raster/occupancy work, 4096 polygons. Bake synchronously on the idle scene owner and publish only a successful immutable replacement. Motor profile compatibility is checked. `NavigationSource` adds owner-thread revision invalidation before collision changes and one matching complete publication per dirty revision. Use `Navigator::navigateSource`; stale active routes enter WaitingForSurface before child motor ticks, spend no search queries while dirty, and replan retained goals after publication without resetting retry/deadline limits. Failed/stale bakes never reactivate old topology; terminal requests require explicit retry/new goals. No automatic collision subscriptions or worker-safe scene access is implied. See `docs/navigation-updates-validation.md`. Moving baked geometry requires explicit rebaking; runtime sweeps cannot prove support in stale topology. No dynamic carving, crowd avoidance, AI runtime checkpoint codec or general stair-walking acceptance is delivered. Games own goals, actions and work budgets across agents. See [game AI](references/game-ai.md), [navmesh integration](references/navmesh.md), `docs/navmesh-validation.md`, native `ai`/`navmesh` and both-renderer `scripts/ai-smoke.cjs` / `scripts/navmesh-smoke.cjs`. Development APIs may be absent from a game pin.
- Reusable flat or hierarchical `StateMachine` in optional `engine_ai` executes shared immutable `StateMachineDefinition` assets with named application behavior/condition bindings, enter/update/exit, per-agent simulation timers and ordered transitions. Defaults: one transition and 1024 conditions per step; limits 1..64 / 1..65536. Cycles defer at transition limits; condition exhaustion/callback exceptions clean up, mark Failed and rethrow. `StateMachineHost` is synchronous and non-retained; exit is noexcept, reentry is forbidden, and lifecycle/control handoff must stop the instance and cancel persistent navigation. Guard component lookups during deferred removal. `engine_editor` now publicly links `engine_ai` and exposes independent `AiGraphEditor` immutable document/history (default 128 edits). Details -> AI state machines opens graphical node/edge authoring, entry/priority/delay/binding edits, drag cancellation, independent undo/redo and isolated C++ preview with controllable conditions. `.weai` version 5 files (reading v1-v4) retain definitions/layout, not callbacks or runtime state; load limit 512 KiB, 1..256 states, <=1024 transitions, finite positions within +/-10000, delays 0..86400, names/keys 1..256 bytes. Save graph files separately from scenes. Use in AI demo validates known application bindings before adoption; games supply their own hosts. No concurrent states, visual action scripting, graph autosave or automatic game callback discovery is delivered. See [state machines and graphical editor](references/state-machines.md), selected engine `docs/ai-state-machines-validation.md`, CTest `state_machine`/`ai`/`editor` and `scripts/editor-ai-smoke.cjs` on both backends including DPR2. A game pin may lack these development APIs.
- Optional `AiControllerComponent` attaches immutable graphs and typed `AiBindingRegistry` mappings to ordinary actors, with independent runtime/timers and a bounded 32-transition log. A controller must be a direct actor component; only one owns an actor. Generic `Component::fixedTickOrder()` defaults 0; stable ascending sibling fixed traversal runs AI at -100 before motors regardless of insertion order, preserving actor-first dispatch, hierarchy and render ticks. Registry keys bind C++ callbacks with Boolean/Number/Vector/saved-Actor parameters; built-ins are idle/move and always/flag/not-flag/near. Complete configuration/schema/reference validation precedes replacement. Disable/detach/control handoff cancels commands; re-enable needs explicit restart. Supply optional `aiControllerCodecs(registry)` to SceneArchive save/load/clone and EditorPlaySession::start; payload v2 (reading v1) embeds definition/configuration, not callbacks, runtime timers or commands. Core stays independent of AI. Browser scene files/Play register the codec. Details -> Actor AI controller assigns the open .weai graph, reports missing bindings, supplies typed forms and scene undo/redo; Live debugger during Play shows selected runtime state/time/history while authoring stays read-only. Registry callback captures must remain valid for every live component/codec. No automatic ground/navmesh, callback discovery, runtime checkpoints or custom-component subtree duplication is implied. See [actor AI controllers](references/ai-controllers.md), selected engine `docs/ai-controllers-validation.md`, CTest `ai_controller` and `scripts/editor-ai-controller-smoke.cjs` on WebGPU/WebGL2 including DPR2.
- Typed per-agent AI blackboards extend StateMachineData with up to 64 AiParameter fields and independent controller live values. setBlackboard(patch) atomically validates types/ranges/actor IDs and movement constraints without restarting state/timers; restart retains inputs, configure resets defaults. AiBinding::sources maps parameters to compatible fields with contained numeric/vector bounds; callbacks receive resolved copies. Owner-thread actor fixed ticks feed perception before AI (-100) and motors. Graph editor field forms/history and Play-only live edits/transition guard labels preserve source defaults. WEAI5 reads v1-v4 and AIC2 reads v1; live inputs stay transient. See [AI blackboards](references/ai-blackboards.md), selected engine docs/ai-blackboards-validation.md, native ai_controller/state_machine/ai and scripts/editor-ai-blackboard-smoke.cjs on WebGPU/WebGL2 at DPR1/2. Typed queued occurrences are described in [AI events](references/ai-events.md).
- Hierarchical AI states use optional AiState parent/initialChild IDs, root graph entry and depth <=16. Each instance owns a single active path and ancestor timers; updates enter outer-to-inner, exits unwind inner-to-outer. Parent interrupts win before child edges; vector order resolves same-source priority. External transitions retain only common strict ancestors of both declared endpoints; group self/ancestor-target transitions reenter their subtree. Stop/failure cancels the complete path; budgets remain shared. activePath() and stateSeconds(activeId) expose borrowed/current diagnostics; transitioned(edge,fromLeaf,toLeaf) records one accepted edge after entry. Graph editor parent/default-child forms, grouped move/delete history, collapse and breadcrumbs compose existing ownership; collapse/focus are presentation only. Play highlights active ancestors/leaf and exact edges. WEAI5 embeds hierarchy/events; old v1-v3 files remain readable through AIC2/custom codec v1. See [hierarchical AI](references/ai-hierarchy.md), selected engine docs/ai-hierarchy-validation.md and scripts/editor-ai-hierarchy-smoke.cjs on both backends at DPR1/2. No parallel regions or history/internal transitions.
- Typed AI event queues extend immutable graphs with up to 32 AiEventDefinition schemas (8 parameters each) and AiTransition::event triggers; empty keys remain polled guards. Each noncopyable machine owns a bounded FIFO (default 32, max64), default8 examined events/step (1..64) and max32 event records. Post on the scene owner; events posted in StateMachine enter/update/guard callbacks wait until the next tick cutoff. Matching event and polled edges share parent-first/source-vector priority; consumed occurrences trigger once, unmatched/early/false-guard occurrences discard, budgets defer unexamined input, and overflow rejects newest. eventScope(activeId) captures instance/activation identity; exits cancel queued scoped inputs and late/cross-agent/reentered posts reject. Stop/restart/failure clears queues; configure resets runtime/history and rejects callback reentry. currentEvent is borrowed only during matched guard/transition callbacks; copy payload data into declared blackboard fields deliberately. Controller posts validate saved actor IDs; later resolution remains handle-safe. Event definitions/trigger forms and preview/Play injection expose typed payloads, scopes, outcome/edge/sequence traces and preserve source bytes. WEAI5 reads v1-v4; AIC2/custom codec1 retain definitions only, never queues/tokens/traces. See [typed AI events](references/ai-events.md), selected engine docs/ai-events-validation.md and scripts/editor-ai-events-smoke.cjs on WebGPU/WebGL2 at DPR1/2. Games still own animation/perception subscriptions; controller task execution is described below.
- State-owned AI task sequences provide AiTaskSpec typed constants/blackboard sources, per-task timeouts, ordered execution, Running/Succeeded/Failed and activation-scoped result events. Controllers own <=16 tasks/state and tick one per active state/step; cancel noexcept releases work on completion/exit/lifecycle. Built-ins wait, wait-event, move-to via explicit Navigator source and face-target; applications register animation/game tasks in the optional third AiBindingRegistry::create vector. setNavigationSource stops the controller; reattach providers after load/Play and restart explicitly. Graph forms and Actor Play diagnostics expose sequence order/progress/failures; raw graph preview does not execute tasks. WEAI5 reads1..5; runtime work stays transient. See [AI tasks](references/ai-tasks.md), selected engine docs/ai-tasks-validation.md, CTest ai_tasks and scripts/editor-ai-tasks-smoke.cjs on both backends/DPR1/2.
- Give each camera one writer; evaluate camera controllers after simulation and before rendering. Presentation poses, GUI focus and view selection must not determine authoritative gameplay state.
- Share immutable mesh/material/texture/skeleton/clip assets; keep transforms, overrides and playback state per instance. Gate transitions on asset preparation and retain the active scene on failure. Verify current decode/upload capabilities before assuming loading is stall-free or GPU-prewarmed.
- Optional terrain combines immutable float/image heightfields, layered materials, seamless camera/shadow LOD, full-source brushes and bounded streaming. `TerrainPagedStream` separately bounds source pages/jobs/admissions; browser workers read/generate chunks. Opt-in `enableEditing(writer)` queues atomic cross-page sculpt/paint, loads/pins every required halo and preserves handles; paint shares geometry/collision. Dirty pages stay pinned until generation-checked atomic writeback succeeds. Failures/conflicts retain edits and stop automatic retries; flush retries, clear rejects unsaved state, explicit discard reloads saved data. Native hosts supply IO/executors and atomic writers; browser loaders/writer share an IndexedDB namespace. Scene outlives the owner, and destruction discards unsaved state: flush/await clean before releasing it when edits must survive. Format 15 snapshots contain current source edits, excluding dirty flags/generations/jobs/provider/history. Read `docs/terrain.md` and [feature integration](references/feature-integration.md) for budgets, lifecycle, synchronous edit-rebuild costs and worker artifacts. Run CTest `terrain`, `terrain_paging`, `terrain_page_edit` and all six terrain browser suites on both backends. Demo `?scene=terrain` includes paged brushes, save/retry/discard and persistent reload; 16-stamp undo remains full-source only. GL supports terrain color/camera LOD with basic lighting (directional shadows require the development extension below). Geomorphing, remote write adapters and automatic conflict merging remain extensions.
- Use optional `Scene::setSky(SkyBackground)` for gradient/panorama/atmosphere skies on both browser backends. `SkyGeometry::Sphere` draws a shared inward unlit mesh per camera; it never enters scene picking/shadows/physics. `SkyClouds` provides procedural coverage/density/wind phase. Opt into `SkyDayNight` and call `advanceSky(scene, chosenDelta, directionalLightId)` once after simulation to update sun/moon, ambient and existing IBL intensity. Choose clock and light ownership explicitly; resolve scene-scoped handles and restore alternative lighting policy deliberately. Set `RenderView::skyEnabled=false` for load-color views preserving earlier backgrounds. Format-6 archives retain sky/weather/cycle state, independently of GPU resources and caller handles. See `docs/sky.md`, native `sky_tests`, `scripts/sky-smoke.cjs` and `scripts/sky-weather-smoke.cjs`. The playground defaults to an atmosphere mesh sphere with clouds, 09:00 and a 180-second day; these are application defaults, while reusable scenes start with sky off. Clouds are 2D, and scattering is single scattering; no volumetric clouds, cloud shadows, multiple scattering or automatic IBL rebake is provided.
- Use the browser `Renderer` facade and reported `RenderCapabilities`. Automatic initialization prefers WebGPU and can fall back to WebGL 2 before canvas acquisition; the fallback has reduced lighting/features. Service recovery at host frame boundaries even while the renderer is not ready. Keep backend resource types private.
- Water uses `MaterialShader::Water`, `Material::water` and optional full `MaterialInstance::water` overrides on subdivided local-XZ meshes. Both backends support Gerstner waves, ripple normals, Fresnel/highlights, depth-aware refraction/foam, one-plane-per-view opaque planar reflections, and underwater fog/absorption with optional animated caustics. WebGPU can use IBL; WebGL 2 uses authored horizon/zenith colors. Sky is independent. Supply `Renderer::setWaterTime(seconds)` once per frame with explicit pause policy; views share it and the renderer never advances it. Bounds expand without changing CPU geometry, physics or picking. Refraction, foam, planar and underwater strengths default zero. Planar capture selects the first visible requesting plane, shares it with coplanar water, clips submerged objects and reverses winding; other planes retain fallback. `underwaterStrength` selects the first active masked World water containing the eye below its transformed local-Y=0 plane and within its local XZ bounds, even when the surface is offscreen. `underwaterFogDensity`, `underwaterColor`, `underwaterMaxDistance` and existing `absorption` control depth attenuation; use `doubleSided=true` to see the underside. Fog captures completed World color/depth after transparency, before Overlay/GUI, clips rays to the surface/sides, and retains per-view 1x/4x, resize/recovery and archive contracts. It has no bottom boundary or inferred arbitrary volume. Current v16 archives include v11 underwater settings and v13 caustics; older water defaults new effects off. Caustics strength defaults zero; scale/speed/depth fade control procedural receiver lighting inside submerged views, sharing the fog composite and caller clock. `Renderer::setWaterInteractions` copies up to 16 transient world-space ring/wake sources; applications own contact/motion emission. Surface normals/crests change without geometry, physics or added passes. Inputs share water time, filter by camera masks/age, survive GPU recovery and clear on scene replacement; they are not material/archive state. Extra reflection planes, transparent-depth fog and buoyancy remain extensions. Renderer time stays application state. Read `docs/water.md` and [feature integration](references/feature-integration.md); use CTest `water` and the six water smoke scripts in [build validation](references/build-validation.md). Demo: `?scene=water`, submerged/deep views, effect presets and colored distance markers.
- Optional MSAA uses `RenderQuality::msaaSamples=1|4`, `RenderCapabilities::msaa4x` and `Renderer::effectiveMsaaSamples()`. Profiles default to off; unsupported devices retain the requested policy and render at 1x. Scene layers/views resolve once before single-sample GUI; shadow filtering and canvas resolution remain separate. See `docs/rendering-quality.md#optional-msaa` and `scripts/msaa-smoke.cjs`.
- For larger content, use the optional isolated `browserDecodedAssetLoader`, per-frame `UploadBudget` and offline KTX2/fallback texture tooling where appropriate. These mechanisms are implemented; CPU asset readiness does not mean every draw is GPU-ready. Loading-screen and eviction policies remain application choices.
- Scene authoring uses optional `engine_editor`: `EditorSession` / `EditorPropertyRegistry` over an idle scene and stable-address asset library. **Scene editor** / `?editor=1` provides files, typed component/material forms, assets/placement/snapping, component add/remove, KeepWorld reparenting, ordered multi-selection, grouped move/rotate/scale/duplicate history and local autosave/recovery. `EditorPlaySession` owns a separate Application and built-in scene clone; Stop rebinds retained source views before releasing the preview, preserving authoring bytes, camera, selection and history. Immutable assets are shared; mutable simulation and clocks are isolated. Reconstruct game callbacks, input bindings, joints, GUI and audio explicitly when needed. Preserve physics topology, managed children and renderer-borrowed camera lifetimes. Gate authoring/files/autosave during Play. Read `docs/scene-editor.md` and [scene-authoring integration](references/feature-integration.md); run CTest `editor`, `editor_content`, `editor_structure` plus all three editor browser smoke scripts on both renderers, including DPR2. Advanced factories/inspectors, marquee selection, Play apply-back and prefab definition authoring remain extensions.
- Compose retained GUI, audio and persistence as the loop requires. Preserve browser audio activation/resume handling. Save game data with stable saved identities and custom codecs; reconstruct executable behaviour after loading.

WebGPU material pipelines in the development engine now specialize material
family, alpha mode, terrain layers, optional map presence and reflection clipping.
Applications continue to configure ordinary immutable `Material`/`MaterialInstance`
values; no game-side shader fork or new quality option is required. Verify this
implementation in the selected checkout: it is not established by the CatGame
engine pin. See `docs/rendering-quality.md#webgpu-material-pipeline-specialization`.

## Preserve runtime invariants

`Application::advance()` owns input snapshots, variable scene updates, fixed physics steps and presentation clocks. Do not run scene/animation updates again inside its callbacks. RAF timestamps enter as seconds. Use simulation time for gameplay, fixed ticks for fixed-step logic and an explicit input/presentation time policy for menus, cameras and audio. Pause and host suspension differ: preserve explicit pause through page visibility changes and avoid hidden-time catch-up. Standalone schedulers must deliberately follow the scene update contract.

Use `Scene::synchronize()` for lifecycle/registration reconciliation without ticks; `Scene::update(0)` still performs a variable tick. `fixedUpdate()` does not advance animation clocks. Opt game actor/component tick hooks in through their supported flags. Preserve bounded fixed-step catch-up and the application transition/preparation boundary before input snapshots.

The scene hierarchy owns nodes uniquely. Query pointers/references are borrowed. Delayed work should retain scene-scoped, generation-checked `ActorId`/`ComponentId` handles and resolve them before access. Destruction excludes targets immediately and defers removal until guarded traversal ends. Detach/reparent only at allowed idle boundaries; preserve cache invalidation and handle `KeepWorld` failure. Protected skeletal implementation children cannot be removed independently.

Declare lifetimes for renderer/camera/scene references, controllers, subscriptions and async interests. Release borrowed presentation references before scene destruction; dispose browser adapters before portable owners. Reject stale completions targeting replacement scenes. Validate candidates before publication so malformed data or failed construction preserves valid state.

Simulation positions/collision/game rules remain independent of render interpolation, culling and LOD. Consume animation markers/root motion explicitly; gameplay owns movement/collision policy. Physics saves are not exact solver rewind. Built-in animation controller definitions/state, markers and pending output are archived. Runtime handles, executable callbacks, input bindings, GUI trees, audio voices, camera behaviours and physics joints are not automatically restored: verify codecs and rebuild runtime behaviour intentionally. Reject unknown custom component types rather than silently degrading them.

Keep DOM, Emscripten, WebGPU and solver implementation types out of new portable interfaces. Update `architecture.md` in the same change if responsibilities, ownership, frame execution, dependencies, platform boundaries or extension status change. Keep affected feature contracts consistent with delivered code. Whenever new engine functionality is added, existing functionality is extended or architecture changes, update this installed `engine-skill` and affected supporting references in the same task before reporting completion. Document delivered APIs, usage, defaults, limitations, architectural contracts, build commands and validation guidance.

Browser editor docking, floating panels, resizing, saved layouts and Quick Add are
documented in [editor workspace guidance](references/editor-workspace.md). Preserve
the existing canvas/control instances and keep layout storage outside scene history.

## Validate and hand off

Follow [build and validation guidance](references/build-validation.md). Use focused native checks for portable logic and actual browser play/smoke checks for controls, adapters and rendering. Exercise the requested loop and affected restart, transition and cancellation paths. Documentation-only changes need content/link checks rather than runtime builds.

Report delivered behaviour, launch instructions, checks actually run and material limits. For substantial phased work, leave useful evidence and a repository-local continuation note. Previous evidence is not a new run; core-only native builds are not desktop games; mobile emulation is not physical-device acceptance; fewer uploads/draws do not establish FPS improvement. Deployment and external actions stay within the user's authorized scope.

## Exponential height fog (2026-10-16)

Development WebEngine now has optional `Scene::setFog(FogSettings)` / `setFog(std::nullopt)` and `Scene::fog()`. `RenderView::fogEnabled` defaults true. Fog settings are color (linear display light), density, startDistance, baseHeight and heightFalloff; zero falloff is uniform exponential fog. New scenes default fog off. World materials, terrain, alpha surfaces and particles use their own positions; additive effects attenuate to zero. Sky integrates to the far plane; Background meshes, Overlay and GUI are excluded. Both backends share stable analytic math, with perspective/orthographic rays, MSAA and recovery. Water composites already fogged captures without repeat fog; underwater remains a separate approximate composite. Format 16 saves/clones scene fog and view opt-out; older archives load without fog. Read `docs/fog.md` and the fog entries in supporting references. This development feature is not evidence of availability in a game pin.

## Editor camera navigation (2026-10-16)

The WebEngine development editor supports right-mouse look/WASD flight, Q/E
vertical movement, Shift speed, middle-mouse pan and double-click/F outline
framing. Read [editor camera integration](references/editor-workspace.md).
Keep device bindings in the browser adapter and navigation in OrbitController;
camera gestures do not create object history or dirty revisions. Validate both
renderers and DPR2 with the camera smoke suite. This does not change game pins.

## Terrain authoring in the editor (2026-10-17)

The development browser editor now provides Quick Add Terrain and a Terrain
section in dockable Details: flat/imported red-channel heightmap creation,
Raise/Lower, Flatten, Smooth and four-layer Paint, with a projected brush ring.
EditorSession owns one bounded history command per complete stroke, with preview,
commit/cancel and existing save/autosave/Play integration. Stream/paged owners are
read-only in this editor. Read [terrain editor integration](references/editor-workspace.md).
Verify the consuming game's engine pin before assuming this authoring host exists.

## Terrain paint layer textures (2026-10-17)

Details → Terrain → Paint layer textures assigns or clears imported Assets images
for all four layers. Each assignment is undoable, updates the whole selected
terrain, and preserves weights, geometry, collision, tint and tiling. Painting,
scene saves/recovery and Play retain the bindings. Read
[layer texture integration](references/editor-terrain-textures.md) for the portable
APIs, ownership gates and focused validation. Both renderers are supported.


## Shaped and soft particle FX (2026-10-17)

Development WebEngine now supports shaped emission, drag/wind, seeded gusts, vector
motion curves, per-camera velocity-stretched billboards and optional opaque-depth
soft particles on both renderers. Defaults preserve legacy effects. Archive v17
retains definitions and live phases; read [FX integration](references/fx.md) for
APIs, ownership, limits, capture costs, build commands and validation. The playground
adds Fountain and mist. Verify the consuming engine pin before using these additions.


## FX authoring (2026-10-17)

Development WebEngine now provides a Particle FX inspector, curves/bursts and emitter
delay timeline, versioned .wefx assets and isolated previews with diagnostics.
Read [FX authoring](references/fx-authoring.md) for APIs, immutable history, renderer
rebind/cancellation, preview clocks, v18 persistence, builds and both-backend checks.
Verify the consuming engine pin; this documentation does not advance a game gitlink.

Ribbon trails: see [integration](references/trails.md) for bounded local/world paths,
per-camera rendering, editor preview and archive v19 contracts.

Plane particles: see [integration](references/plane-particles.md) for ground/supplied-normal
alignment, offset, local/world orientation, Ground ring authoring and archive v20.

Mesh particles: see [integration](references/mesh-particles.md) for shared static
geometry/materials, seeded XYZ rotation/spin, instancing, authoring and archive v21.


## Terrain surface relief (2026-10-17)

Development terrain materials now support optional per-layer normalHeightTexture
(RGB tangent normal / alpha height), heightBlend and world-space triplanar mapping.
Defaults preserve existing surfaces. Archive v23 persists these appearance fields;
use matching main/worker artifacts. Both renderers apply normals, while WebGL2
retains basic lighting; directional shadows require the development extension below. See [surface integration](references/feature-integration.md#terrain-surface-relief-development-engine-2026-10-17)
and run scripts/terrain-surface-smoke.cjs on both backends. Verify the selected
engine checkout before consuming these development APIs in a pinned game.

## Particle contacts and projected decals (2026-10-17)

Development WebEngine adds opt-in swept-sphere particle collision with Bounce,
Slide, Kill and Settle, bounded query/impact queues, and CPU-clipped DecalComponent
projectors on solid opaque static mesh geometry. Applications consume queued
impacts and own mark creation/lifetime. Both renderers share the ordinary mesh
path. Read [contacts and decals](references/fx-contacts-decals.md) for APIs, defaults,
filters, budgets, ownership, editor integration, archive v22 fields (current writer
v24), build commands and validation guidance. Verify the selected engine pin.


Development server startup: `scripts/serve.ps1` now tries up to 100 consecutive
ports from `-Port` (default 8080), skipping busy/access-denied binds and printing the
actual URL. Use `-PortAttempts 1` for fixed-port automation. Direct serve.py stays
fixed-port unless `--port-attempts` is supplied. This is development tooling,
independent of renderer/scene state; see [build validation](references/build-validation.md).


Development WebEngine now provides optional VehicleComponent with shared immutable
four-wheel configuration, command-driven Jolt suspension/drivetrain/brakes, coherent
post-step telemetry, interpolated wheel visuals, COM/inertia and point-force APIs.
Editor properties/presets/history/isolated Play and v24 scene/application checkpoint
persistence are integrated. Use [vehicle integration](references/vehicles.md) for
defaults, topology/lifetime/input rules, partial-state limits and both-renderer
validation. Verify the selected game pin; driving rules remain application code.

- Driving Stage 2 adds immutable physical surfaces, bounded compound collision, source query/wheel/contact metadata and pre-solver closing velocity. Use [physical-surface integration](references/physical-surfaces.md) for friction policy, editor history, v25/WETP3 persistence, lifetime/budget contracts and both-renderer validation. Verify APIs in the selected engine checkout; consumer pins may lag.

- Driving Stage 3 supplies optional source-isolated gamepad input and explicit composed follow cameras with presentation sampling, bounded velocity look-ahead, typed authoring, isolated preview and v26 persistence. Read [driving input/camera integration](references/driving-controls.md) for defaults, ownership, cancellation, reset, limits and both-renderer validation; verify the selected consumer pin.

Camera ownership handoffs may use `OrbitController::adoptCamera(pivotDistance)` to resample the displayed local eye/yaw/pitch without moving it. The driving host adopts only when its composed rig last wrote that view, retaining ordinary archive orbit metadata. See [Stage 3](references/driving-controls.md).

Playback-rate voices and the optional driving audio example are covered in
[driving audio](references/driving-audio.md). Keep RPM/load/slip/surface sound policy
in applications; editor tuning has its own document/history, and audio runtime
state remains transient. Verify API availability against the selected engine pin.

Immutable splines and procedural route sweeps, protected static collision, grouped
editor authoring and v27 source-only persistence are documented in
[spline/route integration](references/splines-routes.md). Keep lane/junction policy
in games and choose explicit rebuild/query budgets. Verify availability at game pins.

Surface-following contact strips are available in the development engine through
explicit SurfaceTrail mode, supplied world positions/normals, bounded sampling and
existing FX authoring/preview. Read [surface trails](references/surface-trails.md)
for defaults, receiver policy, persistence, budgets and both-renderer validation.
Verify the consuming engine pin; games own tyre/contact emission policy.

## Custom GUI widgets and settings controls (2026-10-19)

Development WebEngine adds optional Widget::onPaint(GuiPainter&), onPointer,
onKey, onActivate, describeAccessibility and onAccessibleValue hooks. Local
design-unit painting/input inherits GuiSystem scaling/clipping, focus, capture
and semantic identity; no DOM/GPU types cross the portable interface. Handled
presses allow scroll takeover; Capture on Down reserves a drag. Deliver Cancel
on host ownership loss; hiding/disabling/clipping and Escape cancel captures.
Checkbox defaults unchecked; horizontal Slider defaults range 0..1, step .01,
value .5. Both notify only on changed effective values, including programmatic
setters. Slider ranges/values reject invalid input before replacement, clamp and
snap relative to minimum, and retain reachable maximum. Native checkbox/range
mirrors share C++ actions and values. Settings preview is ?gui=1&guiSettings=1;
it updates local presentation state, not an audio bus. Read
[custom widget contracts](references/gui-widgets.md), selected-engine
docs/gui-widgets.md and docs/gui-widgets-validation.md. Run native gui/input and
input_adapter, then scripts/gui-widgets-smoke.cjs on actual WebGPU/WebGL2 at DPR1
and DPR2, retaining gui-smoke/ui-input/input regressions. Explicit sizing and
unique ownership remain; no child removal or measurement hook is delivered.
Preserve callback lifetimes and avoid reentrant traversal/tree mutation inside
paint/semantic hooks. Verify the selected game engine pin before using these
development APIs; refreshing this guidance does not advance a consumer gitlink.

## Event-driven FX composition (2026-10-19)

EffectAsset now owns bounded acyclic Birth/Impact/Death rules referencing child
assets and/or finite projected decal presets. EffectComponent consumes optional
emitter events in Component::postTick after existing child variable ticks; new
children start next update. Root defaults: 64 child effects, 128 decals, 64 spawns
per update, depth 4 and 256 events per emitter. Overflow drops without backlog.
Children share root spawn/active/depth limits, preserve world event anchors and
inherit optional world velocity once at birth. Source lifetime owns descendants;
pause/stop/restart propagate, and completed one-shots retire. Game code selects
surface responses, damage and audio. Renderer/culling never owns simulation.

Decal lifetime/fade default zero (persistent). Trigger presets default to five
seconds/one-second fade; effective opacity changes without per-frame projection
rebuilds. Archive v29 stores graph dependencies and live child/anchor/decal-age
state; transient events and diagnostics do not replay. Editor Event composition
supports child presets/imported .wefx, decal settings, root limits, immutable undo
and isolated full-chain preview; Impact chain is the example preset. Read
[composition contracts](references/fx-composition.md) and
[validation](references/fx-composition-validation.md) for delivered APIs, ranges,
limitations and isolated local-SDK build commands. Verify the consuming game pin.


## GUI themes and state styles (2026-10-19)

Development WebEngine adds GUI-owned GuiTheme values (dark/default and light),
component defaults, partial local normal/state patches, and shared effective
GuiPainter::style()/GuiSystem::resolvedStyle() values. Theme replacement validates
all layers, copies the candidate, and preserves IDs, values, layout and focus.
Normal layers are defaults/common/component/local; active states apply checked,
hovered, focused, pressed, disabled, each common/component/local. Colors and opacity
are finite 0..1. Existing widget.style.color={r,g,b,a} assignment remains supported;
colors now use inheritable GuiColorOverride fields with reset()/hasValue(), rather
than writable channel members. Read effective colors from the resolved style and
let painter primitives apply opacity once. Parent-local styles do not inherit;
disabled state still follows active ancestors. See [GUI themes](references/gui-themes.md),
selected-engine docs/gui-themes.md and docs/gui-themes-validation.md. Demo:
?gui=1&guiTheme=1. Run native gui/input/input_adapter and scripts/gui-theme-smoke.cjs
on actual WebGPU/WebGL2 at DPR1/DPR2, plus widget/GUI/input regressions. No font/layout
metrics, transition animation or theme persistence are added. Verify API availability
at the consuming game's selected engine pin; skill updates do not change that pin.

## Water depth capture boundary (2026-10-19)

Water refraction and depth foam fade to surface shading between 75% and 98%
of the reconstructed near-to-far view-distance range on both backends. This
prevents a visible colour band when an opaque receiver leaves the depth
capture. Clear/background and foreground samples remain rejected; no material
or archive API changes. See `docs/water.md` and
`docs/water-depth-transition-validation.md` in the selected engine revision.
Verify the consumer pin includes the fix; no application shader fork is needed.


## Measured GUI layout (2026-10-19)

Development engine_gui adds Widget::setSizeLimits/setMinSize/setMaxSize,
setContentSize(width,height), setAlignment/clearAlignment/setContentAlignment,
and GridLayout{columns,columnGap,rowGap}. Explicit sizes/anchors stay default.
Natural or explicit size resolves before stretch, then finite nonnegative size
limits clamp it. Width resolves before wrapped text height. Containers measure
visible children/padding/gaps; dependent stretch is ignored during content
measurement to avoid cycles. Grid uses 1..64 equal columns, visible row-major
placement and maximum measured row heights. Alignment packs groups/cells without
flex weights. Layout stages measurements/bounds/lines/scroll extents and rejects
overflow or depth >128 / >65536 measurement requests before publishing. Custom
leaves use preferred sizes; no custom measure hook or child removal is added.
Read [GUI layout](references/gui-layout.md), selected-engine docs/gui-layout.md
and docs/gui-layout-validation.md. Demo: ?gui=1&guiLayout=1. Run native
gui/input/input_adapter, scripts/gui-layout-smoke.cjs on WebGPU/WebGL2 at DPR1/DPR2,
and affected theme/widget/GUI/UI-input regressions. Native runtime DLLs must match
the configured compiler. Keep adaptive column policy/content/game rules in apps;
verify APIs against the selected consumer engine pin before use.

## WebGL 2 directional shadows (2026-10-19)

The isolated development engine in CatGame `build/engine-water-depth-transition`
now supports one directional shadow map per view on WebGL 2. This is not yet in
the game's pinned dependency. Existing LightComponent shadow settings and
Renderer casting/receiving/culling/LOD controls apply; maxShadowMaps is capped
at one (zero disables), with device-bounded resolution and 1/9 PCF taps. The
DEPTH_COMPONENT24 target is consumed per view and recreated after context loss.
Opaque/masked World casters retain off-camera selection, CPU skin/morph poses,
mirrored winding and layered alpha cutouts. Ambient/emission remain unshadowed.
The existing sun-shadow demo controls exercise it; Paws beach acceptance uses
`scripts/beach-shadows-smoke.cjs` on both backends, including visible pixel
comparisons, MSAA, resize, scene return and WebGL context recovery. Basic lighting,
no PBR/IBL, individual draws and no GPU timing remain WebGL limits. See the
selected engine's `docs/rendering-fallback.md`. Verify checkout/pin availability.

Unshadowed Spot lights now share eight masked light slots on both development
renderers, with typed editor cone/range/direction controls, selected outlines,
normal hierarchy mounting and v30 persistence. Read [spotlights](references/spotlights.md)
for defaults, bounds, ownership, capabilities and validation; verify the game pin.

## Formula circuit application example (2026-10-19)

The development WebEngine playground has ?scene=formula: a supplied-model driving
example composed from existing VehicleComponent, closed SplineAsset/RouteAsset,
TerrainAsset and CameraFollowRig. Read selected-engine docs/formula-demo.md,
src/formula_demo.hpp and src/formula_laps.hpp before adapting it. Race gates,
lap validity, timing, HUD and circuit placement are application policy, not core
APIs or archive state. WASD/arrows drive/steer, Space brakes, Shift handbrakes,
R resets; held touch controls use existing BrowserInput sources. The optimized
assets/formula1-red-driving.glb is derived offline with tools/prepare_formula_car.py
from the untouched formula1_red.glb (all 18,977 triangles retained; 2048 px textures). Its merged visual has baked wheels; physical
wheels still steer independently. Native formula_demo_tests and browser
scripts/formula-smoke.cjs verify scoped handling/race rules and both renderers.
No physical mobile, real-controller or real Formula 1 handling acceptance follows.
Verify these example files exist at the consumer pin; they do not update a game pin.

## General region residency

For static content beyond terrain pages, use optional RegionManifest/RegionStream
and existing AssetManager/Scene ownership. Separate visual and collision sources;
applications own predictive movement/teleport gates and explicit restore handoff.
Managed residents are transient and editor read-only; authored manifests/source
references and selected world state use v31 archives. Read [integration and validation](references/region-streaming.md)
for supported payloads, pinning, budgets, editor preview and lifetime limitations.
Verify availability in the selected engine checkout before using these APIs.

Formula circuit mouse steering is inverted and on by default in ?scene=formula. Click the
canvas, then move horizontally relative to its centre; a 4% dead zone straightens
and full steering is reached at 35% of canvas width. The HUD toggle disables it.
web/formula.js owns one allocated BrowserInput Move source; throttle/brake keep
independent sources. Held A/D/arrows override mouse steering until fresh movement.
Focus/context/lifecycle gates, pointer exit, reset/pause and resize neutralize it;
touch/pen events cannot write this source. No core API or native build changes
are required. Validate scripts/formula-mouse-smoke.cjs on both renderer backends.

## Animation library and visual state machine (2026-10-19)

The development engine now provides immutable animation libraries with stable clip
IDs, folder/tag/display metadata, validated same-rig clip imports and `.weanim`
persistence. Details -> Animation library & state machine authors typed gameplay
parameters, clip/blend states and conditional ordered transitions over the existing
player. Preview runs an isolated Play clone; Apply is one guarded undoable scene edit.
Use Boolean for persistent conditions and Trigger for one-shot requests. Save library
organization/layout separately from scene runtime state. Read
[animation authoring](references/animation-authoring.md) for API ownership, limits,
file semantics, lifetime guards and native/both-backend acceptance. Verify availability
in the selected game pin; skill updates do not change that dependency.


Formula easy handling is application-owned in src/formula_handling.hpp. The
FormulaDrivingAssist chassis component shapes desired commands on fixed ticks
before vehicle physics; no second solver, render clock or core API is added.
FormulaHandlingSettings defaults: 450 N m torque, 2.1 tyre friction, .30 rad front
steering, .12 s steering half-life, half steering at 18 m/s with .25 minimum,
33.3333 m/s forward and 8 m/s reverse throttle limits with 5 m/s taper, and .25
front torque share. Neutral/cancel, pause/reset and component disable/detach clear
filter/commands immediately; body/solver control stays with VehicleComponent.
Other vehicle defaults are unchanged. Limits fade engine input, not velocities
or coasting/impulses. Desired/filter state is transient with no Play/archive codec.
Browser formulaInputState reports requested input; vehicleState reports the
shaped command. Preserve the user-selected inverted mouse mapping. Verify
native formula_demo and formula_handling, and both Formula browser smoke scripts
on both renderers. Read selected-engine docs/formula-demo.md for the parameter
table and docs/formula-demo-validation.md for recorded evidence/limitations.

## Shadowed lighting and HDR post-processing (2026-10-19)

The development engine now supports directional/spot shadows on both browser
backends, optional2..4 camera-fitted sun cascades, and opt-in RGBA16F bloom with
exposure/saturation/contrast. Defaults keep post off and authored fixed sun volumes.
Read [lighting integration](references/lighting-post-processing.md) for budgets,
capability fallback, frame ordering, persistence limits and native/browser checks.
This supersedes older no-spot-shadow/no-WebGL-shadow notes for this checkout only.
Verify consumer pins; preserve Paws-specific dependency guidance.
