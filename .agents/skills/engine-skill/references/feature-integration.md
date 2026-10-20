# Feature integration contracts

Checked against working source and architecture on 2026-10-11. Paths refer to the selected repository. Read the referenced contract when using a feature; this guide highlights decisions that can otherwise cause incorrect game integration.

## Editor workspace layout

Read [workspace integration](editor-workspace.md) for browser docking/resizing,
layout storage and primitive/component shortcuts. The browser host moves existing
DOM/canvas instances; layout never owns authoritative scene or GPU state.

## Scene authoring

The first three editor increments were delivered on 2026-10-15. Link optional
`engine_editor` for `EditorSession`, `EditorSnap`, `EditorPropertyRegistry` and `EditorPlaySession`.
Sessions borrow an idle/non-ticking scene and optional stable-address `AssetLibrary`;
dispose before either owner. The browser adapter stays in the executable. Hosts own
pause policy and the first render view's camera/backing viewport. Viewport picking
is CPU World-mesh bounds picking without gameplay interaction providers.

Typed bounded history covers transform transactions, descriptor properties, material
bindings, imported-library snapshots, created/removed components and reparented subtrees. One drag is one command;
branches discard redo. Undo/redo re-resolves saved identities and guards supported
target/parent state. Creation undo parks ownership outside the scene; redo creates
fresh runtime handles for the same duplicate saved identities. External writes are
not overwritten. Registries are shared immutable registrations; captured callback
dependencies must outlive them. Their setters are trusted application code, not
rollback for external side effects. Extend this registry/history instead of bypassing
dirty revision or writing scene properties directly from JavaScript.

Use `setTransform` or begin/preview/commit/cancel; `beginGizmo`/`dragGizmo` compose
world axes with validated TRS. `setProperty`, `setMaterial`, `clearMaterial`,
`importAssets`, `placeMesh` and `duplicateSelected` create typed commands.
`revision()` changes on committed edits/undo/redo; `markSaved()` is the file
checkpoint, and `markRecovered()` leaves a recovered document unsaved to file.
Numeric inspectors use local TRS/degrees, while C++ uses Euler radians. Snapping
defaults off with grid=1 and angle=15 degrees: absolute world translation-axis/grid
anchors and relative rotation deltas. Numeric transform fields stay exact local edits.

Scene root/managed subtrees and the navigation camera are protected. Body/motor
transforms stay identity, but supported settings are editable; physics pose edits
use teleport without clearing velocity, with exact solver unit scale. Configuration
changes may rebuild physics and are not a solver-state rewind. `Scene::synchronize()`
reconciles edits without ticks. Per-slot material edits publish immutable instance
bindings, preserving other instances, source resources, maps and unedited advanced
settings. Opacity blending requires Blend/Additive alpha mode.

`SceneArchive::duplicateSubtree` reuses built-in node encoding with a reference-only
resource table, shares immutable assets, stages independent nodes/playback and
regenerates every saved identity. It copies no executable callbacks or physics
joints and rejects scene/protected roots or unsupported custom subclasses before
publication. No scene archive version is added. Imported GLB/FBX and images use
32 MiB source limits; `.weassets` uses archive budgets. Local file reading is async,
while decoding is currently synchronous/bounded. External FBX images require
separate import/map assignment. Unplaced resources are retained in the explicit
library; source labels/preferences use the existing opaque host metadata. Ground
placement uses world Y=0, preserves asset units, and offsets by source minimum Y.
It does not implicitly add colliders or raycast arbitrary placement surfaces.

Browser scene binding releases the old session before scene/camera destruction and
binds replacement metadata only after it reaches its final owner. Edit mode forces
pause, keeps camera navigation/rendering, and restores prior pause policy on exit.
Pointer/capture/blur/resize/visibility cancellation rolls back uncommitted gestures.
File-read completions check document generations. Source editing is direct; isolated Play uses a separate Application and immutable-resource-sharing scene clone. Save/open use ordinary staged archive/audio
publication, so malformed data retains the current scene. History and selection
are runtime-only; imported resources/configuration/materials persist.

Autosave uses a 3-second default debounce (1–300 configurable), only after committed
revisions and outside drags. `IndexedDBStorage` namespace `scene-editor`, key `latest`,
holds one versioned local recovery envelope. Report success only on transaction
completion. Byte capture must not mark the file clean; the manual download path
acknowledges its own checkpoint. Explicit recovery opens through the staged loader
and marks the result unsaved to a separate file. Storage/parse failures preserve
the current scene and previous checkpoint. Document generations abort stale writes
and suppress stale status updates. Turning autosave off retains recovery; page
shutdown cannot guarantee a final asynchronous write. Other tabs share the slot.

`selectMany`/`toggleSelection` maintain ordered saved IDs; the last is the inspector
primary. `selectionRoots` removes selected descendants beneath ancestors. Group
gizmos, duplication, removal and reparenting commit one command with full selection
restoration. Local TRS fields are disabled for multiple selections; descriptor and
material forms edit the primary. Move/rotate use world axes and a mean-root pivot;
Scale uses the first root's local axes, with an explicit center uniform handle.
TRS/shear or physics scale rejection preserves all previous poses. Cancellation
restores unaffected previews without overwriting an external conflicting writer.

`addComponent(type, mesh)` supports Component, Mesh/SkeletalMesh, Camera, Light,
Collider, RigidBody, CharacterMotor and Interaction built-ins. Select one actor or
an owned component; mesh creation requires a selected immutable asset, collider
defaults to a half-unit box and light to point. `removeSelectedComponents` parks
selected component roots in history. `reparentSelected(parentId, KeepWorld)` stages
new local poses, validates all targets/cycles/topology, and restores sibling order
through idle `Node::setChildIndex`. Managed descendants and every camera subtree
borrowed by a render view are protected. Solver components remain direct children
of top-level actors; duplicate physics components reject. Actor/node deletion and
arbitrary component factories are not exposed by this editor menu.

`EditorPlaySession::start(source, views, assets)` stages `SceneArchive::clone`, then
owns an independent Application/scene runtime. The reference-only bounded codec
shares immutable assets, retains saved identities in separate scene scopes, and
owns fresh mutable nodes, handles, animation/effects, physics and clocks. `advance`
uses ordinary scheduling; forward host suspension with `setSuspended`. Browser
Play supplies copied-camera navigation and sky/water clocks while retaining the
source paused document/session/library. Stop rebinds original views **before**
`stop` releases preview owners. Authoring revision/dirty checkpoint/history/selection
and source camera remain unchanged. Abort outstanding autosave and gate files,
recovery/imports and structural/property edits during Play. No apply-back operation
is provided. Custom executable classes reject before publication; game callbacks,
input bindings, joints, GUI trees, Web Audio voices and demo-specific policies need
explicit application reconstruction rather than assuming the built-in preview has
copied them. No archive version was added.

See `docs/scene-editor.md` and `docs/scene-editor-structure-validation.md`. Further
work includes advanced factories/inspectors, marquee selection, explicit sibling
ordering, Play apply-back/custom-runtime hooks and prefab definition authoring.

## Reflective animated water

Water contracts were checked against working source on 2026-10-15. Use `MaterialShader::Water` with `Material::water` or the full optional `MaterialInstance::water` override. Published materials remain immutable. Water uses a subdivided local-XZ surface; wave sizes are local units and mesh transforms apply afterward. It renders three bounded Gerstner waves and analytic normals, two moving normal-map samples plus procedural fine ripples, dielectric Fresnel reflection and direct highlights. `normalTexture` is optional linear +Y data. Source bounds expand by the largest effective wave amplitude across slots, including overrides; GPU displacement never changes CPU meshes, physics, buoyancy or triangle picking. Existing alpha sorting, masks/layers, mirrored/double-sided state, instancing and MSAA contracts apply.

Provide `Renderer::setWaterTime(double seconds)` once before rendering. It validates finite time within +/-1000000 seconds, starts at zero and is retained through recovery; `waterTime()` returns it. The renderer never advances the clock, including across multiple views. Select simulation/presentation time and pause/suspension policy explicitly. Time only changes frame uniforms for views collecting water and never enters material cache identity. Amplitude and ripple strength both zero give a stationary surface; speed zero freezes it. `WaterSettings` defaults/ranges are in `docs/water.md` and `water.hpp`.

WebGPU uses positive-intensity `Scene::environment` for roughness-filtered reflections, otherwise material horizon/zenith radiance; WebGL 2 always uses the approximation while supporting water-specific normal mapping/highlights. This does not add general fallback PBR/IBL. Visible sky/clouds are separate from reflection lighting: no probe capture or automatic rebake exists. Format 9 adds planar strength; format 8 retains refraction/foam with planar capture disabled, format 7 retains reflective water with depth effects disabled, and formats 1-6 retain legacy shader meanings. Current archives write v14 alongside terrain/material-layer records; v13 adds optional caustics. Version 11 adds underwater settings; formats 1-10 leave underwater rendering disabled. Renderer time must be saved through application state when required.

Set `planarReflectionStrength` to [0,1] (default 0) for nearby opaque/masked World reflections on either backend. `reflectionStrength=0` disables the capture too. `waterPlanarReflectionRequired(scene, mask)` gates optional storage; `waterReflection(draws, eye)` selects the first visible requesting World plane from transformed local Y=0 and returns its camera-facing plane/mirror matrix. Coplanar water shares one capture per view (plane distance <0.01, normal dot >0.999); other planes retain fallback. Backends mirror the original view-projection and camera position, reverse winding, and clip at a 0.002-world-unit plane bias without mutating scene cameras or ticking simulation. Retained opaque batches include off-camera objects using existing masks, poses and selected LODs.

Captures exclude water, transparency/particles, sky and GUI, with no recursive pass. Transparent clear pixels retain analytic/IBL reflection. Separate capture bindings avoid sampling the active attachment; projected samples clamp to viewport half-texel bounds and compose decoded display-light colors before refraction/foam. Reflection targets are always single-sample, independent of main 1x/4x MSAA, reused serially across ordered views, resized/recovered privately, and reduced to 1x1 when no active mask requests them. Counters include one extra pass plus color/depth memory; WebGPU World timing includes capture. No reflection mip chain, independent capture LOD, multi-bounce or arbitrary extra planes exist. Additional planes, transparent reflections and fluid simulation remain extensions. Demo: `?scene=water&renderer=webgpu|webgl2`, inspector planar-off/still-mirror presets, colored reflection landmarks, pause/orbit/reset, an oblique orthographic Camera views inset and Return to demo.

Opt-in World refraction and shoreline foam were checked on 2026-10-14. `WaterSettings::refractionStrength` and `foamStrength` default zero. Defaults/ranges for distortion, absorption, foam width/scale/color are in `docs/water.md`; widths/absorption are world-unit quantities. Current-view non-water opaque/masked color/depth is captured before water/transparency. Snapshots are shared serially across cameras and sampled within half-texel viewport bounds. Inverse view projection reconstructs perspective/orthographic thickness and plane gap; refraction rejects foreground, above-plane and clear-depth samples. It combines decoded tone-mapped colors in linear display light, avoiding a second tone map. Foam follows the water clock and skips clear depth. Background/Overlay water keeps surface shading; hidden/offscreen/transparent geometry and other water surfaces cannot be refracted. Opaque alpha-one water is the recommended refractive surface; legacy blended water retains approximate mesh-origin sorting.

Backend-owned targets allocate conservatively for enabled World water overlapping active camera masks, and release canvas-sized storage when effects are off. WebGPU averages color/minimum depth for intermediate MSAA snapshots, then continues original attachments and performs the final canvas resolve before GUI. WebGL uses complete RGB8/DEPTH_COMPONENT24 framebuffer textures and same-size nearest color/depth blits; no float-color extension is needed. Single-sample water scenes use accumulation and final presentation. Resize/recovery rebuild targets; memory and World capture/continuation timing are reported. Asset upload budgets do not govern mandatory resize-dependent targets. Affected demo: sloping lakebed, submerged markers and presets for refraction/foam/surface-only inspection. Use `scripts/water-depth-smoke.cjs` alongside the existing water/material/compositing/fallback suites; physical mobile costs remain a separate gate.

## Underwater water rendering

`underwaterStrength` defaults 0, range [0,1]. Defaults are `underwaterFogDensity=.12` per world unit, `underwaterMaxDistance=100` and `underwaterColor={.015,.12,.16}` in linear display light. Density range is [0,100], max distance [.01,10000], color [0,1]; all must be finite. Existing `absorption` coefficients also apply underwater. Density zero isolates absorption, absorption zero isolates scattering, and strength zero or attenuation and caustics all zero skips the effect. Full material overrides validate atomically and archive with shared assets.

`underwaterView(scene, eye, mask)` returns a copied `UnderwaterView` containing settings, inverse transform and authored local XZ bounds. The first enabled World water containing the eye more than .01 world units below transformed local Y=0 wins. Masks/ancestors apply; surface frustum visibility does not. The rectangular footprint below the mean plane has no bottom. Transforms, including rotation/mirroring/nonuniform scale, retain local orientation. No polygonal footprint, holes, cave or arbitrary volume is inferred. `underwaterDistance` clips a world segment to the sides/surface and caps its world length. This presentation selection neither moves cameras nor ticks simulation; wave crests do not toggle entry.

Backends snapshot completed World color/depth after transparency, then reconstruct pixel rays (perspective eye or orthographic near-plane origin). Clear depth uses the far endpoint. Fog combines decoded display-light color with exponential RGB absorption and scattering toward the fog color, with no second tone map. It clips each ray to the sides and surface and caps distance. Transparent/particle color is included but uses opaque/surface depth behind it. Overlay and GUI remain outside the fog pass. No separate transparent depth, volumetric lighting or buoyancy is supplied. Set surface `doubleSided=true` when its underside should render; existing surface reflection is approximate without total internal reflection.

Submerged views add one snapshot and one fog composite, sharing existing private water scratch targets serially across views; pure underwater requests these even with surface refraction/foam off. Dry cameras skip both. Captures use existing 1x/4x resolve rules, fog writes the original scene attachment at its sample count, and final presentation/resolve precedes GUI. Underwater-only at 1x also adds the final presentation pass. Targets shrink when unused and rebuild on resize/recovery; actual storage/pass counters and WebGPU World timing include the work. WebGPU caches unchanged underwater view uniforms. Per-view uniforms and copied selection data borrow no scene resource lifetimes. Settings serialize from format 11; clock policy remains application state.

Demo View controls offer above water, underwater and deep underwater; white submerged markers compare distances and presets isolate fog/absorption/off. Return restores the original camera transform/projection and orbit along with previous scene/sky/light enable state. `scripts/water-underwater-smoke.cjs` verifies actual pixels, transitions, view/layer boundaries, recovery and archive loading.

## Animated underwater caustics

Both backends implement `WaterSettings::causticsStrength` (default 0, range [0,4]), `causticsScale` (2, [.01,100]), `causticsSpeed` (1, [0,10]) and `causticsDepth` (8 world units, [.01,10000]); all must be finite. Strength also scales by `underwaterStrength`. Fog and absorption can be zero for caustics-only rendering. Full instance overrides validate atomically and material identity includes every setting. Format 13 adds these fields; older water defaults caustics off. Current writer v31 also retains terrain extensions; readers accept v1..31. Renderer time remains application state.

The existing post-World underwater composite projects smooth animated cellular ridges in water-plane coordinates onto depth-reconstructed receivers. Inverse-transform axis lengths compensate projection scale approximately in world units; general shear remains approximate. It rejects clear depth, dry/outside-footprint endpoints and a local surface band of `2*amplitude+.02`, weights geometric depth-normal orientation toward surfaces parallel to the plane, and fades to zero at `causticsDepth` below the mean plane. It multiplies decoded receiver color before RGB absorption/fog. No new targets, passes or timestamp queries are added; the private view uniform is 256 bytes. Enabled caustics use the caller water clock times caustics speed; speed zero freezes just this pattern, pause freezes the caller clock, and WebGPU caches unchanged payloads. Recovery retains appearance/time.

This is an artistic screen-space approximation, without refracted sunlight tracing, light/shadow visibility, above-water caustic visibility or separate transparent receivers. Transparent/particle color uses opaque depth behind it. Overlay/GUI remain untouched. The demo enables patterns on the sloping lakebed; use Underwater/Deep underwater and Caustics off/Still caustics. Fog/absorption isolation presets disable caustics. `scripts/water-caustics-smoke.cjs` checks both backend pixels, animation, zero speed, pause/resume, depth fade, dry/ceiling rejection, view boundaries, MSAA, resize/recovery, archive restoration and unchanged pass/texture costs.

## Water interaction ripples and wakes

`Renderer::setWaterInteractions(span<const WaterInteraction>)` copies at most 16 transient inputs atomically; empty clears. Applications own contact detection, object velocity and emission cadence, with no automatic physics subscriptions, node handles or history. `WaterInteraction` defaults: world origin zero, direction (1,0,0), startTime 0 on the renderer water clock, strength .6, radius .7, wavelength .6, propagation speed 2, lifetime 4, wakeLength 0 and all camera-mask bits. Finite bounds: origin/direction components +/-1e6; startTime +/-1e6; strength [0,1]; radius/wavelength [.05,100]; speed [0,100]; lifetime [.01,60]; wakeLength [0,100]. A positive wakeLength requires nonzero direction. Radius is packet/arm width and surface-contact distance in world units. Full snapshot validation precedes publication; invalid input/count retains valid state.

Portable `WaterInteractions` owns fixed copied storage; `waterInteractionParameters` stages active relative ages and normalized headings, filtering future/expired, zero-strength and camera-mask-excluded sources. Rings propagate radially, directed V wakes trail the projected heading, and both fade over lifetime. Refresh continuous wakes from current contact position, velocity and startTime; retain impulses for their lifetime. The renderer never advances time, and rewinding can reactivate retained earlier inputs. All views share one snapshot/time but filter masks independently. `setScene` clears on success, `setViews` retains inputs; resize/device recovery retains CPU state. Explicitly clear when ending gameplay without replacing the scene. Inputs are absent from material identity and scene archives; application save state may rebuild them.

Both shaders evaluate World-water normal perturbations and bounded bright crests after refraction. Projection is in world units on the water plane; distant-height sources are rejected. Background/Overlay water ignores inputs. No vertex, culling-bound, shadow-geometry, collision, buoyancy or picking change occurs; this is not fluid obstacle simulation, spray or interaction-driven caustics. No passes/textures are added; per-fragment work scales with active source count. WebGPU's bounded frame payload is 2400 bytes and caches unchanged values; WebGL 2 stages matching fragment uniforms. Shared source data borrows no resources or callback lifetimes.

Demo Object interactions offers both, contact ripples, moving wake or off, with a yellow moving source and orange contact source. Calm interaction pool isolates the effects from waves. Demo motion and source history are application-only; pause and the Still water preset freeze them. `scripts/water-interactions-smoke.cjs` verifies actual independent rings/wakes, propagation/direction, future/expiry, masks/elevation, pause/resume, ordered perspective/orthographic cameras, 1x/4x, resize, paused recovery, scene replacement, no extra pass/texture costs and moving-source/restart controls. Existing water suites explicitly disable interactions when isolating older effects.

## Renderer and views

Optional MSAA was checked against working source on 2026-10-14. Set `RenderQuality::msaaSamples` to 1 or 4; invalid counts reject the candidate. Existing profiles use 1. `RenderCapabilities::msaa4x` reports color/depth support and `Renderer::effectiveMsaaSamples()` reports actual attachments (1 before creation). WebGPU applies changes at its next render boundary, retains multisampled color across all ordered views/layers and resolves once before GUI. WebGL 2 queries the actual format sample arrays, uses a staged complete multisampled framebuffer when supported, then blits color before GUI. Unsupported devices retain requested 4 and render at 1. WebGL color storage matches the opaque RGB8 default framebuffer for resolve; incomplete candidate framebuffers reject quality without replacing valid pixels. Resize/recovery recreate targets and preserve requested policy. Shadow maps stay single-sampled; resolution and texture/specular aliasing remain separate. Storage estimates account for sample-scaled color/depth; timestamp totals exclude resolve/GUI. See `docs/rendering-quality.md#optional-msaa`.

Enable MSAA through the facade after choosing application quality policy:

```cpp
auto quality = renderer.quality();
quality.msaaSamples = 4;
renderer.setQuality(quality);
// Query after rendering has created/applied the attachments.
const auto activeSamples = renderer.effectiveMsaaSamples();
const bool supports4x = renderer.capabilities().msaa4x;
```

Set `msaaSamples=1` to disable it. The playground exposes **Rendering quality -> Anti-aliasing -> 4x MSAA**; changing it marks the profile custom, and selecting a profile restores its off default. Quality is runtime presentation policy rather than scene archive data. Keep application resolution budgets explicit: MSAA improves geometric coverage but cannot restore detail lost by upscaling a smaller backing canvas.

`include/engine/renderer.hpp` is a browser renderer facade without WebGPU/GLES/Emscripten public types. It remains compiled into `webengine`; native core/test targets do not provide native graphics. Private backends own GPU resources. Use `RenderBackendPreference` and `RenderCapabilities`, not direct backend dependencies in game or portable engine interfaces. See `docs/rendering-fallback.md`.

- Automatic mode prefers WebGPU, then selects WebGL 2 if API/secure-context availability or initial adapter/device acquisition fails, including its bounded acquisition timeout. The host calls `serviceRecovery()` at frame boundaries even before readiness. Explicit `WebGpu` preference disables automatic fallback.
- After a canvas acquires a context, backend selection stays fixed. Later WebGPU loss uses bounded WebGPU reacquisition/retry; WebGL loss waits for context restoration and rebuilds resources. Do not replace the canvas or reload the page to simulate automatic fallback. Scenes, CPU assets, GUI and explicit pause state survive recovery.
- WebGL 2 supports RGBA textured meshes, basic ambient/directional/point/unlit/emissive lighting, CPU skeletal/morph deformation, alpha/additive particles, ordered views/layers, camera LOD/culling and canvas GUI. Full PBR/environment lighting, instancing, compressed uploads and GPU timing are unavailable. Directional shadows require the development extension below. Check whether that visual target meets the game request; do not claim backend parity. WebGL 1 is unsupported.
- Ordered `RenderView`s compose Background, World and Overlay per view; GUI draws once afterward. Pick with the actual active camera/viewport and distinguish CSS coordinates from backing pixels. Shared camera depth conventions are converted by the fallback backend, not game code.

Quality profiles/resolution/DPR/shadow budgets expose capabilities without choosing game policy. Keep authored shared static LOD chains separate from collision/original picking geometry. Restore/regenerate derived data deliberately; archives omit LOD caches and compressed texture variants. See `docs/rendering-quality.md` and `docs/mesh-detail-validation.md`.

## Visible sky backgrounds and weather

Sky contracts checked against the working source on 2026-10-14. `Scene::setSky(std::optional<SkyBackground>)` atomically validates scene-owned gradient, panorama or single-scattering atmosphere settings. Reusable scenes default off. `SkyGeometry::Sphere` draws shared inward-facing unlit mesh geometry per view; `Fullscreen` draws one triangle. Both backends exclude sky from scene collection, picking, shadows, LOD and physics. Camera-relative projection preserves translation invariance; forced far clip depth handles scene near/far planes, and orthographic cameras use parallel rays with affine extent coverage. See `docs/sky.md`.

`SkyClouds` supplies a procedural 2D cloud layer (coverage, density, scale, wind and saved phase), not volumetric clouds or cloud shadows. Atmosphere uses bounded Rayleigh/Cornette-Shanks aerosol single scattering, planet sunlight occlusion and sun/moon discs. It does not implement multiple scattering or object aerial perspective. Panorama radiance remains sRGB RGBA8 with repeated longitude/clamped latitude; brightness multiplies all modes, and Y rotation turns panoramas. Sky mode remains independent of IBL resources.

Applications own scheduling: call `advanceSky(scene, deltaSeconds, optionalDirectionalLightId)` once per selected clock step after simulation, before rendering. Cloud phase follows this delta; `SkyDayNight::enabled` additionally owns ambient, existing IBL intensity and the selected sun/moon light. Handles are scene-scoped and validated; stale/foreign/wrong-type handles and singular light transforms reject before any publication. Pause/suspension use zero delta; `running=false` holds the hour while wind can continue. Disabling policy leaves last lighting values for the caller to restore. The helper retains no scene pointer, callback or subscription. It never rebakes IBL or changes simulation poses.

`RenderView::skyEnabled` defaults true independently of clearing; disable it for overlays preserving earlier sky. Backends reuse asset admission/caches and rebuild sky GPU state after loss. Format-6 archives retain geometry/weather/cycle phase and panorama sharing; v5 loads original gradient/panorama with weather/cycle disabled, and v1-4 load sky off. The derived sphere and caller's sun handle are rebuilt. Run native `sky`, `scene_archive`, `persistence` plus both `scripts/sky-smoke.cjs` and `scripts/sky-weather-smoke.cjs`. Desktop Chrome evidence is not physical mobile or sustained-performance acceptance.

The playground starts with `SkyGeometry::Sphere`, `SkyMode::Atmosphere`, clouds enabled at coverage 0.58, and a running 180-second cycle starting at hour 9. Reusable scenes remain sky-off; a newly constructed `SkyBackground` defaults to fullscreen gradient with clouds/cycle disabled. `skySphereMesh()` supplies a shared 64-segment/32-ring unit mesh (3,968 inward triangles) and an unlit material. The inspector exposes geometry, mode, coverage/density, scattering/haze, hour and speed; its clock follows actual scene state and global Pause freezes wind/time. See `docs/sky-validation.md` for recorded evidence.

## Mobile assets and loading

Read `docs/mobile-assets.md` and `docs/assets.md` before selecting a pipeline.

- `browserAssetLoader` performs asynchronous HTTP but invokes custom decoders synchronously on the browser owner thread. `browserDecodedAssetLoader` supports FBX, GLB, images and texture bundles with at most two active isolated workers per main Wasm module. Workers own HTTP/decode/timeout/cancellation; they do not own scenes or GPU state. Owner hydration still validates/copies resources; main-thread work and temporary memory remain.
- Ship `asset-worker.js`, `asset_decode.js` and `asset_decode.wasm` together and respect CSP/CORS and relative worker URLs. No shared Wasm memory, pthreads or cross-origin isolation is required. This loader does not implicitly discover companion files or transport arbitrary decoder callbacks. Native hosts supply their own executors.
- `Renderer::setAssetUploadBudget(UploadBudget)` applies optional per-frame new-asset admission. Zero limits are unlimited. Resources are indivisible; the first oversized unit may exceed the budget to make progress. Deferred draws retry on later frames without affecting simulation. Dynamic writes, GUI, framebuffer/shadow allocation and pipeline creation are outside admission; a time budget is not a hard frame-latency cap.
- `AssetPreparation` gates CPU dependency readiness, not GPU prewarming/full visual readiness. Choose loading-screen/reveal policy accordingly. Explicit cache eviction can release manager ownership while live shared resources remain valid; there is no automatic total-memory/LRU policy.
- Native `texture_tool` exports semantic mips and packs pre-encoded KTX2 variants with a valid RGBA fallback into `.wetex` delivery bundles. Color filters in linear light, data channels remain raw, and normal mips renormalize. Outputs reject overwrite. Read `texture_bundle.hpp`/the mobile asset contract for restricted complete 2D BC7/ETC2-RGBA/ASTC-4x4 chains. Encoding is external; no bundled block encoder, Basis/UASTC runtime transcoder, supercompression inflater or compressed GLB extension is implied. WebGPU selects supported variants; WebGL 2 retains RGBA.

## Heightfield terrain

Read `docs/terrain.md` before composing terrain. `TerrainData` is a row-major source-local XZ float grid (`heights[z*samplesX+x]`), spacing, 1–128 chunk cells, texture metres and shared material. Dimensions are 2–4097 per axis, at most 1,048,576 samples and 4,096 chunks. `create(data)` eagerly builds immutable chains; `create(data,false)` retains source only (`chunks()` empty, `chunkCount()` metadata, `buildChunk(index)` explicit caller ownership). `fromHeightmap` maps decoded RGBA8 red linearly to a chosen height range without gamma conversion or flipping.

New data defaults to three LOD candidates, steps 2/4/8 and `lodPixels={240,120,60}`. Only triangle-reducing levels survive. Every LOD retains every source perimeter segment with matching normals/UVs and coarse-interior fans; arbitrary neighboring levels need no skirts/negotiation. Selection remains discrete with existing 10% hysteresis. `MeshComponent::lodPolicy` has camera/shadow scales (>0 through 64, default 1) and fixed levels (-1 automatic, 0 full, positive clamped). RenderFrame owns camera-pixel and independent per-light shadow-texel history; global quality flags still gate LOD. Presentation choices never change source ground/collision. GL supports camera LOD; directional shadows and caster LOD require the development extension below.

`Material::layers` optionally blends four albedo/color/roughness/UV layers through normalized linear Data RGBA weights (zero/absent weights select layer zero; absent albedos are white). Use Clamp weights and `weightScale={textureMeters/terrainWidth,textureMeters/terrainDepth}`. Layer `uvScale=textureMeters/repeatMetres`. Global material UVs apply first. Both backends blend color/alpha; only WebGPU supplies PBR roughness lighting. Four Color plus one Data and four packed Normal/height maps use nine map slots, excluding conventional metallic/occlusion/emissive textures; scalar metallic/emissive/tint remain. Water rejects layering. MaterialInstance can override the full layer block; all parameters/semantic roles participate in backend identity/recovery.

`TerrainAsset::edit(TerrainBrush)` prepares a version-specific `TerrainEdit` containing before/after assets, affected indices and height/paint flags. Radius is finite 0.0001–1,000,000 m in source-local X/Z. Raise uses signed delta (negative lowers); Flatten blends toward targetHeight; Smooth blends toward the frozen source's clipped 3x3 sample mean. Flatten/Smooth/Paint strength is 0–1. Height limits remain ±1,000,000 m. Zero/outside stamps preserve source identity. Work copies/scans bounded source synchronously; it has no frame-time guarantee.

Height edits regenerate only intersecting chunk chains plus a conservative two-sample halo for coarse normals/interpolation and share unchanged mesh pointers. Paint layer 0–3 requires enabled layers and positive weight/material UV scales. Global material UV scale/offset participate in texel-to-source mapping. It blends normalized bytes toward the layer, repairs their total to 255, replaces immutable weights/material and shares all geometry. Missing weights create a source-sized grass map with texture axes capped at 4096. Existing texel dimensions and weight scale/offset map into source metres; painting sets Clamp wrapping. Do not mutate published source/meshes/textures. Applications own stroke sampling, cancellation and bounded history; old versions retain source/changed resources.

`makeTerrainActor(name,asset,collision=false)` prepares a complete detached actor with one TerrainComponent per chunk. Physics adds one full-source static triangle collider/body, on a direct Scene actor with unit scene/actor scale and identity chunk transforms. Translation/rotation follow existing physics rules. Idle `applyTerrainEdit(actor,edit)` rejects stale versions/topology changes and incomplete instances, prepares replacement meshes/collision/paint bindings, then preserves IDs, transforms and detail policies. Call Scene::synchronize before querying/stepping replaced collision. Whole-instance height edits recook the whole collider; painting keeps its pointer. Independent inherited-mesh replacement is rejected. Applications choose wake/reposition policy for dynamic bodies; the demo wakes its balls after sculpting.

`TerrainStream(scene,sourceOnlyAsset,settings)` borrows a longer-lived Scene and owns scene-root actor handles. Defaults: radiusChunks 1, maxResident 9, buildsPerUpdate 2, collision false, origin zero translation. Limits: radius 0–32, residents 1–256, admissions 1–64, finite origin within ±1,000,000 per axis. No rotation/scale. Immutable settings change by preparing a replacement owner. `update(localFocus)` clips a candidate square, sorts by physical center distance/index, caps residents, prepares at most the build budget, evicts outside candidates and adopts/synchronizes admissions. Focus outside the source clamps to the nearest edge chunk. Residents stay bounded; staging can hold one extra admission batch. Update/apply/clear/destruction require an idle scene boundary. No implicit camera focus, source IO, workers or scheduler.

Each resident has ordinary geometry/LOD and optionally one full-resolution chunk collider/body. `stream.apply(edit)` rebuilds only dirty resident chains/colliders; editing absent chunks generates nothing until admission from the latest source. `clear()`/destruction removes actors and invalidates handles. Source-only assets retain no generated cache; backend weak caches release GPU resources after previous frame references expire. External resource pointers/history can extend lifetime. Full source heights/global weights remain resident under existing caps. stats expose resident/pending, cumulative built/evicted, lastRebuilt and visual vertex/index geometryBytes across all levels (excluding source, history, collider/driver overhead). Choose gameplay-safe resident regions explicitly; eviction removes solid ground.

`asset->ground(x,z)` is O(1) exact source triangle interpolation across the entire source, including absent chunks. `terrainGround(root,x,z)` and normalized two-sided `terrainRaycast` visit active resident source triangles with AABB rejection and return scene-scoped handles. They include subtree ancestor transforms, exclude disabled/pending removal, ignore render masks/culling/LOD/interpolation and never initialize physics. No BVH guarantee. Apply world-to-source conversion before brushes.

Format 15 stores optional patch interior/global-offset/page metadata and edited source/material/layer sharing, LOD settings/policies, eager/source-only mode and resident components/optional collision. Eager chains regenerate once per shared asset; source-only loads generate only saved resident components with conservative per-chunk allocation charges. Asset-library source-only resources generate nothing. Resident paged snapshots retain only bounded source halos and rebuild without IO; provider revision/URLs/configuration are not archived. Runtime stream owners/focus/budgets, pending admissions, undo history and GPU/hysteresis state are not archived: replace restored resident actors when explicitly creating a new streaming owner. Formats 10–13 terrain remain eager; 10/11 also disable LOD/layers and use neutral policies. Other generic mesh LOD cache regeneration and solver/controller restoration contracts remain unchanged.

Demo `?scene=terrain` uses 97x97 samples, 0.5 m spacing, 24-cell chunks and grass/sand/rock/soil. Navigate uses shared orbit/query input; brush mode exclusively owns pointer gestures. Stamps require radius/4 travel and cap at eight per input frame. Release/cancel stops further work but keeps committed stamps. Coordinate Apply, 16-stamp undo/redo, explicit stream radius/count/build/focus controls and affected/resident readouts share engine APIs. New admissions inherit selected camera/shadow detail and surface inspection mode. Settings changes clear demo balls/history; reset restores full original terrain/detail/perspective. Save/open preserves edited source and current resident snapshot. GL shadow controls remain disabled. See `docs/terrain-edit-validation.md` for evidence; geomorphing and holes/overhangs remain extensions.


`terrain_paging.hpp` adds bounded source paging separately from full-source edits. `TerrainPageLayout` describes 2–65,537 samples per axis and up to 1,048,576 pages without allocating the world. Each `TerrainPage` contains one chunk with a clipped two-sample height/optional sample-sized RGBA halo and immutable nonzero 64-bit revision. `region`/`dimensions` define its payload; WETP v2 encode/decode (also reading v1 base pages with generation zero) checks counts, finite heights, CRC32, trailing bytes and a 256 KiB response cap. Global positions/UVs/perimeter normals match the full-source generator. Page materials share four tiles, retain no global weights, and map local Clamp masks at sample centers including positive global UV scale/offset.

`generateTerrainPage(page,material,collision)` is pure CPU base/LOD/raw-triangle generation. `TerrainPagedStream(scene,layout,revision,loaderFactory,settings)` owns per-job AssetManagers and resident root ActorIds, borrowing the longer-lived Scene. Native applications supply IO/executors via a factory of ordinary AssetLoaders; callbacks capture owned resources and weak AssetLoadContexts, never Scene pointers. Idle update/clear/destruction reuse scene guards, handles and deferred destruction. There is no implicit native executor or simulation scheduler.

Paging settings reuse TerrainStream radius/resident/admission/translation defaults, plus maxInFlight 1–8 (default 2) and maxSourceBytes default 4 MiB, maximum 64 MiB, at least one worst-case page. Desired-set distance/index ordering reserves heights plus RGBA even when masks are absent. Resident source bytes count heights/masks only; geometryBytes counts visual chains, excluding colliders, shared tiles, external pointers, worker heaps and transport. Pending ready results remain in bounded job slots. Idle update cancels obsolete jobs, evicts outside pages, validates layout/ID/revision/source/geometry/collision and overlapping height/weight halos, admits at most buildsPerUpdate and launches within maxInFlight. Failures retain other residents and remain bounded to the desired set; retryFailed explicitly retries. Read-only clear/destruction cancels pending work; late reports are safe. Editable dirty ownership follows the writeback contract below. Stats distinguish resident/inFlight/queued/failed, source/geometry bytes, started/completed/cancelled/evicted and workerCompleted.

`browserTerrainPageLoader` in engine_assets_browser fetches/decodes/generates in isolated workers with two active and 32 queued/running terrain requests per main module. Ship terrain-worker.js and terrain-storage.js with matching asset_decode.js/.wasm, respect CSP/CORS/relative URLs, and retain no global mask in the template. Defaults: 256 KiB response, 30 s timeout (maximum 120 s), 8 MiB result; collision false. Owner hydration validates/copies prepared base/LOD meshes without regenerating topology. Jolt cooking/registration and GPU admission stay on owner/backend paths; budgets are not hard latency/total-memory caps. No pthreads/shared memory/cross-origin isolation required.

Paged ground is resident-only: TerrainPagedStream::ground and patch asset ground accept global source-local metres, return no unloaded ground and trigger no IO. World queries still use active Scene source triangles. TerrainAsset::createPatch owns a bounded halo/interior with global sample offsets; patch edit is rejected. Provider revisions are immutable; replace the owner to change revision. Use the opt-in paged stream transaction API below for sculpt/paint and dirty writeback. Clean-page prefetch caches and in-place dataset revision changes remain extensions. Keep gameplay within resident collision coverage.

Paged editing is opt-in: enableEditing(TerrainPageWriter,TerrainPageEditingSettings) requires a writer addressing the same dataset/generations as the loaders. edit(TerrainBrush) validates and queues one stamp (true means accepted, not applied); update prioritizes/loads every page whose two-sample source halo can contain a changed sample. Required pages plus dirty pins must fit resident/source and write-staging budgets, otherwise no edit is published. Global heights/normalized RGBA evaluate once from frozen samples, including clipped global 3x3 smoothing. All replacements validate before an idle atomic commit. Height edits rebuild affected base/LOD/colliders; paint shares exact mesh/collision pointers. Unchanged pages retain sharing, and actor/component handles, transforms and policies survive. Failure/invalid source leaves prior data valid and reports editError. Focus change or cancelEdit cancels only unpublished work; applications own contact release/cancellation policy. Brush/rebuild work is synchronous on the owner and indivisible; admission counts do not give a hard edit frame-time cap. Paged undo/history remains application-owned.

TerrainPage revision is immutable dataset identity; mutable generation increments per changed page, with v1 files defaulting to zero. Dirty pages stay pinned within the same residency/source limits until their exact version is acknowledged. TerrainPageEditingSettings defaults autoWriteback true, maxWriteBytes 8 MiB (256 KiB–64 MiB). One immutable TerrainPageWrite batch can be pending while newer edits continue; an older acknowledgement advances only its captured versions, leaving newer edits dirty. TerrainPageWriter returns an ordinary AssetLoader completing TerrainPageWriteAck{token} only after atomically comparing all expectedGenerations and committing every page/halo replacement. encode/decodeTerrainPageWrite use bounded WETW v1, at most 256 pages/64 MiB with CRC. Native filesystem/server adapters are application-supplied. Failed/conflicting writes preserve local dirty data and stop automatic retries; flush explicitly saves/retries. clear() rejects dirty/pending writes; clear(true) and destruction cancel interests and discard unsaved state. Flush and await clean state before releasing an owner when edits must survive. Aborting cannot undo an already committed transaction; no automatic conflict merge is provided.

BrowserTerrainOptions.storageNamespace selects the IndexedDB overlay before HTTP; default empty retains read-only HTTP. browserTerrainPageWriter(namespace,timeout=30000) must use that same namespace (1–128 ASCII letters/digits/_.:-; timeout at most 120 s). Ship terrain-storage.js on both host and worker alongside matching terrain-worker.js and asset_decode.js/.wasm. The adapter uses one strict IndexedDB readwrite transaction for batch generation compare-and-replace and acknowledges oncomplete. Stored CRC/layout/revision/generation checks reject corrupt/incompatible data; failures never silently load stale base files. Origin storage remains browser/user-evictable. Stats add editPending/edits/lastAffected/lastRebuilt/editError, dirty/dirtyBytes, writePending/writing/writeFailed/writeError and writes/writtenPages. Pending batch encoding/source, edit staging, old bound materials, colliders, worker heaps and driver storage are outside resident sample counters. Snapshot format 15 stores edited patch source/weights, not per-page generations, dirty flags, store namespace or pending writes; saving a scene does not flush the provider or restore a writer.

The paged demo now edits the 1025x1025 / 512x512 m world over its 1,849 base files, persisting halo-coherent overrides in IndexedDB across eviction/reload. Shared pointer/coordinate tools queue brushes, and cancellation drops unpublished stamps. Readouts show dirty/writing/error state, Save dirty pages retries, and Discard unsaved page edits reloads the last stored versions. Reset/return/budget replacement retain unsaved state until writeback or explicit discard. Clear saved page edits is available outside paging. New publications inherit camera/shadow/surface policy, and height changes wake demo balls. Paged history is not the full-source 16-stamp undo stack. See docs/terrain-page-edit-validation.md for current evidence.

## Movement, interaction and physics

Read `docs/movement.md`, `docs/physics.md`, `docs/physics-joints.md` and `docs/selection.md` as appropriate.

- Collision-aware character commands use world horizontal XZ velocity and a latched jump press edge. Free mode uses full XYZ without ground/gravity/physics. One active motor controls a scene-root actor with unit world scale; a rigid body on that actor is rejected. Motors do not currently collide with each other. Gameplay owns normalization, camera-relative direction, coyote time and jump buffering. Read movement results after a fixed step.
- Sensor Enter/Exit detects post-fixed-step overlaps, including stationary bodies; it does not guarantee detecting a complete crossing between samples. Resolve retained handles in exit/contact callbacks. Query filters and simulation layer-pair policy are separate.
- Selection defaults to bounds/proxies; opt-in CPU triangle picking supports original mesh parts, morph/skinned/interpolated poses and surface attributes. Optional visible-mesh triangle occlusion includes noninteractive blockers. This path is independent of render LOD and GPU pixels; texture alpha/shader displacement and accelerated geometry queries are outside its contract.
- `ManipulationController` owns a transform-writing session for eligible non-physics objects. Commit retains the pose; cancel rolls back only while ownership is valid. External writes/parent motion/target loss release ownership without overwriting later changes. The scene outlives the controller; game code routes capture, cancel, snapping, legality and feedback.
- Physics joints are world-owned non-owning `PhysicsJointId` handles. Hinge, slider, distance spring and hard point grab are implemented. Disabling/removing/transferring/rebuilding an endpoint permanently removes its joints; re-enable does not resurrect them. Game code cancels grabs and recreates desired joints after bodies register. Motor drives, break thresholds, force-limited grabs, joint prefab components and built-in joint archive codecs remain extensions.

## Animation, content, UI and saved behaviour

Read `docs/animation-control.md`, `docs/prefabs.md`, `docs/gui.md`, `docs/ui-input-integration.md`, `docs/audio.md`, `docs/scene-archives.md` and `docs/persistence.md` for the systems used.

- Shared animation controller definitions supply typed parameters, ordered transitions and two-clip blends; clip marker lists are authored explicitly (FBX/GLB import produces empty lists). Consume events/root-motion output regularly. Both blend contributors can emit markers; gameplay chooses weight filtering/merging and collision-aware movement. Controller transitions preserve pending output. Scene archives retain definitions, instance state, markers and pending output; executable marker responses are rebuilt.
- Prefabs are in-memory definitions with stable definition-local keys, independent instance state, shared assets and versioned application factories. Instantiation validates a detached candidate before publication; trusted factory external side effects are not rolled back. Definition keys, runtime handles and stable saved identities are different. Prefab factory registration does not register a persistence codec; register custom `ComponentCodecs` separately. There is no general prefab definition-file format or serialization of arbitrary executable factories.
- Input context changes release owned sources; bind through `Application::submitInput` rather than mutating its read-only snapshot. GUI has first refusal. Dispose virtual controls before removing DOM elements. GUI logical scale differs from DPR. Canvas text uses trusted font assets and single-line input; shaping/bidi/grapheme selection/font fallback/multiline editing are not delivered. Native semantic DOM/text adapters must be disposed before GUI destruction.
- Runtime audio owns independent voices/buses/fades and scene-scoped emitter/listener handles. Choose its time domain and synchronize after simulation. It has no built-in scene archive codec; the older browser waveform/MP3/WAV archive workflow is separate. Browser activation/audible device evidence is explicit.
- Built-in archives save scene/resources and application payloads; custom subclasses need versioned `ComponentCodecs`, with unknown types rejected. Decode/migration/validation stages ownership before publication. Runtime handles are recreated, while saved identities resolve persisted references. Rebuild action executors/subscriptions, input contexts, GUI, camera behaviours, audio and joints. Physics restoration preserves supported data, not exact solver history/undo.
- Material graph authoring bakes material/maps; editable graph source has separate JSON persistence. Mesh/PBR/light/effect features and per-instance morphs already exist; inspect relevant README sections before creating substitutes. Verify importer warnings/content limits, semantic texture use and asset provenance. General level editors, networking, pathfinding, gamepad input, timers/tweens and PWA packaging remain separate work when a concrete game requires them.


## WebGPU material specialization

The development backend caches variants for shader family, alpha mode, material
layers, optional normal/surface/occlusion/emissive map presence and reflection
clipping. Shader constants and cache keys must agree with resolved material
uniforms, including map replacement, masked shadows, skinned/mirrored instances,
water vertex deformation and reflection passes. Opaque nonreflection variants
eliminate discard and unused paths; absent maps keep their white/neutral default
math. WebGL 2 remains unchanged. This introduces no portable API or archive change.
Use material, layered-material/terrain, water, shadow and recovery browser checks
when altering the variant policy, then measure the actual game at matching
resolution/quality. The CatGame pin may predate this implementation.

## Scene exponential height fog

Use optional validated `FogSettings` with linear display-light color; defaults are color (.65,.75,.85), density .02, startDistance 0, baseHeight 0 and heightFalloff .15. Scene defaults off. Color must be finite in [0,1], density/falloff [0,100], start [0,1e6], base [-1e6,1e6]. Invalid setters preserve state. `RenderView::fogEnabled=false` disables material/particle/sky fog for that camera. `fogTransmission` is the portable analytic segment reference; start distance excludes the segment beginning, exponent evaluation saturates at +/-40 and optical depth at 80. Perspective starts at the eye, orthographic at the pixel near plane. World PBR/unlit/terrain/alpha/particles use surface positions; additive fades to zero, with opacity unchanged. Background meshes, Overlay and GUI are unaffected. Fog is blended after tone mapping in linear display light before encoding, without added passes/textures. Sky rays share fog color/integration at the far plane, independently of IBL/day-night lighting.

Water intrinsic shading is fogged before composition. Refraction is already fogged and is not fogged again; absorption body color/foam receive surface fog. Planar captures integrate eye/near plane to water and reflected receiver with one start-distance budget. Fallback environment reflections use surface distance. Atmospheric fog extends underwater; existing underwater absorption/caustics runs later, retaining opaque-depth transparency limits. This is not physical coupled air/water transport. Format 16 archives and scene clones preserve fog/view opt-out, older versions default scene fog off. Deploy matching host/worker codecs. Local volumes/light shafts remain future work.

Editor mouse navigation and outline focus use the borrowed OrbitController and
application input clock; Play owns a separate controller. Follow
[camera integration and cancellation](editor-workspace.md), including temporary
browser bindings, parent-space movement, lens fitting and camera archive state.

Terrain creation/sculpt/paint is also available in EditorSession and the browser
editor, with complete eager-instance ownership and whole-stroke history. Keep
stream/paged editing with its existing owner; do not mutate resident chunks behind
that owner. Follow [terrain authoring](editor-workspace.md) and docs/scene-editor.md.

The terrain editor now assigns/clears each layer's albedo from imported Assets,
using immutable TerrainAsset::withLayers and EditorSession::setTerrainLayerTexture.
Keep the source and all instance chunks consistent so subsequent painting retains
textures. Read [layer texture integration](editor-terrain-textures.md).


## Particle FX milestone

Read [FX integration](fx.md) and engine `docs/fx.md` for shaped CPU emitters,
velocity alignment, soft depth capture, v17 archives and both-renderer limits.


FX editor/preview contracts: [FX authoring](fx-authoring.md). Use EditorSession
immutable asset commands and an independently owned EffectPreview. Archive v18
adds startDelay; source documents remain idle during preview.


## Optional game AI

Use [game AI integration](game-ai.md) and the selected checkout's `docs/ai.md`.
`engine_ai` is opt-in and owns no scene, browser or renderer. Actor fixed ticks
produce movement before child motor ticks; scene clocks remain application-owned.
Explicitly cancel on interruption/disable/detach. Sight resolves scene handles,
ignores target/observer bodies and sensors, and never uses render visibility.
Waypoint edges and start/goal connectors require authored support/access.
Optional `WaypointNavigator` now tests motor-capsule clearance and replans around
obstacles with bounded query/search/retry/deadline settings. It checks the next
segment before movement, stops immediately on blockage, and performs at most one
synchronous search per eligible fixed step after a simulation-time delay.
Waiting can recover automatically; terminal failures require explicit retry or a
new goal. Navigation retains no borrowed scene pointers or query callbacks.
Static `NavMesh` now composes the same `Navigator`; see [navmesh integration](navmesh.md).
Bake from synchronized static collision on the idle scene owner, check agent
profile and publish only successful replacements. Raw triangle sources define
their own interiors. Dynamic carving, crowd avoidance, walking-support proof and
AI persistence remain absent.


### Terrain surface relief (development engine, 2026-10-17)

MaterialLayer.normalHeightTexture is optional linear RGB tangent normal (+Y/OpenGL),
alpha height. normalScale defaults 1 (0..16), heightScale 1 (0..16), heightBias 0
(-16..16). MaterialLayers.heightBlend defaults 0 for existing linear blending;
positive width <=1 weights active layers by height plus authored weight, preserving
zero-weight exclusion. triplanar defaults false; triplanarScale defaults 1 world
repeat/unit (.0001..1,000,000), triplanarSharpness defaults 4 (1..16). Signed world
planes blend color/height/reoriented normals; weight maps remain mesh-UV based so
paging and painting retain their coordinates. World projection stays fixed as
actors move. No displacement, parallax, physics/query change or new render pass.
Nine material textures fit with seven WebGPU frame textures. WebGPU uses Normal
mips with linear alpha; WebGL2 uses base-level filtering and applies normals to
basic lighting, without PBR/shadows. Triplanar costs three samples per map/layer.
Asset/instance archive format 23 persists maps/parameters; older archives keep old
surface defaults. Match host and worker JS/Wasm. withLayers edits preserve meshes;
paged hydration reconnects owner-shared maps. Demo Surface compares linear UV,
normals, height blend, triplanar and combined. See selected engine docs/terrain.md,
docs/terrain-surface-validation.md and scripts/terrain-surface-smoke.cjs on both
backends, plus existing terrain and material regression suites. These development
APIs do not establish availability in CatGame's pinned engine or game integration.


## Vehicle foundation integration

Use [vehicle integration](vehicles.md) for four-wheel physics, command/input and
interpolation contracts, body force/COM/inertia APIs, typed editor and archive v24
checkpoint ownership. Preserve fixed-step authority and cancel held commands at
control/lifecycle handoffs. The development engine may be ahead of a game pin.

## Navigation geometry changes

Use `NavigationSource` and `Navigator::navigateSource` when doors, static props
or terrain collision may change. Invalidate BEFORE mutation, synchronize on the
scene owner, prepare a complete `NavMesh::bake` candidate and publish with the
matching revision token. Dirty sources retain diagnostic data only: active
navigators stop stale commands before child motors, wait without search work,
then replan original goals. Deadlines/search allowances survive revisions.
Failed/stale/duplicate publications cannot revive old topology; terminal requests
need explicit retry/new goals. This is synchronous explicit replacement, without
automatic collision subscriptions, tile carving or thread-safe scene access.
See [navmesh integration](navmesh.md) for exact contracts and limitations.

## State-machine behaviors and graphical authoring

Use [state-machine integration](state-machines.md). Keep shared definitions
immutable, per-agent timers/current state independent, and game behavior/guards in
StateMachineHost callbacks. Run from the owning actor fixed tick before motors;
stop on disable/detach/despawn and control handoff. Exit must not throw or reenter.
Graphical editor documents/history and C++ preview are independent of scene bytes
and scene undo. Save .weai separately; no automatic scene attachment or runtime
instance codec is supplied. Use in AI demo validates its supported binding keys.
The editor publicly links engine_ai; engine_core never depends on AI/editor.

## Driving Stage 2 physical contracts

Read [physical-surface integration](physical-surfaces.md) before adding compounds, surface-dependent traction, impact consumers or physical terrain maps. Portable engine mechanisms are independent of visual materials; games own material names, damage and sound policies. Use v25 matched host/worker artifacts and check consumer pin availability.

## Gamepads and authored follow cameras

Read [Stage 3 integration](driving-controls.md) before adding analogue driving or chase cameras. Keep device bindings/status transient, use the existing action snapshot, require neutral after gates, and release only owned device sources. Update one application-owned camera rig after interpolation with collision last. Authored settings persist in v26; runtime rig and input history do not.

## Driving audio and rate

Read [driving audio](driving-audio.md) for variable-rate voice/backend contracts,
application-owned RPM/load/slip/surface/impact mappings, callback/proxy lifetimes,
editor preview and version-1 tuning documents. Scene audio codecs are still explicit;
Play/Stop and scene replacement must stop runtime voices.

## Actor-bound AI integration

Read [AI controllers](ai-controllers.md). Complete graph/binding configuration
validation is atomic. Register the same immutable AiBindingRegistry with components
and aiControllerCodecs for file load and codec-aware scene cloning/Play. Actor
references are saved IDs resolved in the reconstructed scene; no callback code or
mutable instance state is persisted. Component fixedTickOrder is a generic stable
sibling ordering contract (AI -100, default 0); actor ticks still precede children.
Do not compete with AI motor commands. Stop before control/tick handoff; disable/
detach cancels and re-enable requires explicit restart. Inspector changes use scene
history, standalone graph changes use their own history, and live debugging reads
only the isolated Play instance. Default browser bindings are idle/move and
always/flag/near; games can supply different native registries and codec captures.

## Splines and route source resources

See [spline/route integration](splines-routes.md) for Hermite/arc/nearest contracts,
profile transport/banking/UVs, bounded synchronous publication, protected static
collision children, grouped editor/terrain-point baking and v27 source-only archives.
Do not reference generated cache meshes as persistent route identities or infer
lane/junction logic. Scene ownership and physics invalidation contracts still apply.

## Surface-following contact strips

Use explicit World-only SurfaceTrail and submitSurfaceContact before the normal
scene variable update. Games own contact/slip and receiver changes; missing input
breaks. World marks never retain/move with receivers. Reuse FX history/preview,
textures/curves, depth/fog and bounded live archive normals. See [contracts and
validation](surface-trails.md); verify the selected game pin.

## Typed AI blackboard integration

Declare immutable graph fields with defaults and numeric/vector bounds, then map AiBinding::sources parameter keys to compatible field keys. Controller callbacks receive current resolved parameters; supply patches on the scene owner in actor fixed ticks before child AI/motors. Live actor references are saved IDs resolved per scene and may disappear safely. setBlackboard validates complete patches atomically and preserves timers/history; restart preserves inputs, configure resets defaults. Source configuration and live Play edits have separate ownership. See [blackboard contracts](ai-blackboards.md).

## Custom GUI widget integration (2026-10-19)

Compose Checkbox and Slider in optional engine_gui. Widget painting, portable
pointer/key/activation and semantic hooks can define application controls without
modifying a renderer. GuiSystem still owns clipping, scale, capture and focus.
Use Capture on pointer Down for slider-like drags; Handled allows ancestor scroll
takeover with Cancel. Values are application presentation/input state; callbacks
choose game/audio policy. Keep callbacks alive and adapters detached before GUI
destruction; do not retain hook contexts or reenter traversal. See
[custom widget APIs and defaults](gui-widgets.md), selected-engine docs/gui-widgets.md
and its validation document. A game pin may lack these development APIs.

## Hierarchical AI integration

Declare optional parent/initialChild IDs with root entry and depth <=16. Register a binding for every group/leaf; enter/update run outer-to-inner and exit runs inner-to-outer. Parent interrupts precede child transitions; per-source vector order remains deterministic. Retained ancestors retain timers/resources; external source/target ancestor or self-transitions reenter that subtree. Stop and failures unwind the entire active path. Keep command ownership explicit across active callbacks and cancel navigation in exit. The blackboard stays one per agent; activePath spans are borrowed until the next operation/destruction. See [hierarchy contracts](ai-hierarchy.md).


## GUI theme integration (2026-10-19)

GuiSystem::setTheme owns a validated GuiTheme value. Configure defaults and
forKind(GuiStyleKind), then optional widget.style/stateStyles partial patches.
GuiPainter::style supplies resolved colors; primitives multiply opacity once.
Use the resolved style in custom painters rather than raw inline color reads.
Normal layers precede checked/hovered/focused/pressed/disabled layers. Local
normal colors can be superseded by state defaults; supply matching local state
patches to retain them. Existing color assignment is supported, reset restores
inheritance, and GuiStyle full assignment pins every normal field. See
[theme API, migration and limits](gui-themes.md). Keep theme choice/persistence,
font selection and layout policy in applications; verify consumer engine pins.

## Water depth capture boundary (2026-10-19)

Water refraction and depth foam fade to surface shading between 75% and 98%
of the reconstructed near-to-far view-distance range on both backends. This
prevents a visible colour band when an opaque receiver leaves the depth
capture. Clear/background and foreground samples remain rejected; no material
or archive API changes. See `docs/water.md` and
`docs/water-depth-transition-validation.md` in the selected engine revision.
Verify the consumer pin includes the fix; no application shader fork is needed.


## Measured GUI layout integration (2026-10-19)

Compose optional content sizing/finite minimum and maximum limits with existing
explicit sizes, stretch, anchors, clipping and themes. Content-height panels
propagate nested visible extents into ScrollPanel; grid uses equal columns and
measured row heights. Start/Center/End align groups and cells; child cross/cell
overrides use setAlignment, while clearAlignment restores defaults/anchors.
Constraints beat stretch; group packing does not distribute flex weights.
Layout stages complete geometry before publication and preserves identity,
callbacks and input ownership. Custom leaves retain preferred intrinsic sizes.
Use [API/defaults and cycle rules](gui-layout.md). Demo callbacks should retain
persistent toggle state in widget/application values or shared captured state:
the engine copies callbacks before invocation to preserve safe lifetimes.
Verify consumer engine pins before using these development APIs.

## Typed AI event integration

Declare immutable typed event schemas and optional transition event keys. Post exact payload maps on the scene owner; capture eventScope when starting asynchronous work, retain scene-scoped controller handles in adapters and resolve before posting completion. Matched callbacks borrow currentEvent payloads; deliberately copy durable data into compatible blackboard fields. Scope exit/reentry/restart/other instances reject late completions, while producers still cancel subscriptions. FIFO events and polled guards share ancestor/source priority; examined false/ineligible events discard. Keep default queue/dispatch/transition/condition budgets explicit across agents. Stop/failure cancels pending input, configure replaces it; queues/tokens/traces never enter scenes/Play clones. See [event contracts](ai-events.md).

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

## Spotlights

Use ordinary LightComponent with LightType::Spot, local emitted direction and
validated world-unit range/radian half angles. Both renderers share eight slots;
spot shadows are unavailable. Editor/Play/clone/archive ownership is unchanged.
See [spotlight contracts](spotlights.md) and verify the consuming engine pin.

Animation libraries and graphical controller authoring are described in [animation authoring](animation-authoring.md). Same-rig imports preserve target skin bindings; gameplay parameters and per-instance playback reuse AnimationComponent. Save .weanim organization/layout separately from scene runtime state.

Shadowed lights, sun cascades, HDR bloom/color grading and both-backend validation: [current lighting contracts](lighting-post-processing.md).

## AI task sequences

AiState now carries <=16 ordered AiTaskSpec entries with registered keys, exact typed parameters, compatible blackboard sources, timeout (0,86400] default30, optional observation event and zero-payload succeededEvent/failedEvent names. AiBindingRegistry::create accepts an optional third vector of up to128 AiTaskBinding factories. Each controller owns fresh AiTask instances; start(controller,spec,activationScope) snapshots inputs, tick returns Running/Succeeded/Failed, and cancel noexcept releases work on every terminal/exit path. At most one task ticks per active state per fixed step; callbacks run only on the scene owner. Scoped terminal events defer through the existing cutoff, retry once per later update while the queue is full and cancel on exit. Retained ancestors retain tasks. Controller stop from callbacks defers teardown safely; configure/restart/source replacement cannot replace executing ownership.

Built-ins: wait(seconds), wait-event(spec.event), move-to(target Vector/speed/distance) via existing Navigator and explicit transient setNavigationSource(source) followed by restart(), face-target(target Vector,+Z facing). Motor/pose tasks require an existing motor, and competing active AI writers reject. Source revision invalidation remains authoritative; no automatic bake/ground/animation subscription. Application animation factories use scoped component handles and own/cancel subscriptions. See [task contracts](ai-tasks.md).

Graph task forms add/reorder/remove tasks, typed constants/source mapping, timeouts and result events with one Apply history edit. Actor Play live tasks show index/key, elapsed, result, reason and pending delivery, with <=32 history records. Standalone manual-guard preview explicitly leaves task execution to Actor Play/custom hosts. WEAI5 reads1..5 inside unchanged AIC2/outer codec1; objects, progress, providers/subscriptions and history stay transient. Run ai_tasks plus state_machine/ai_controller/ai/editor/editor_structure/application/scene_archive checks and scripts/editor-ai-tasks-smoke.cjs on WebGPU/WebGL2 at DPR1/2; preserve exact artifacts and report any concurrent-suite failures.
