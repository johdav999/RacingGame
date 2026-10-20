# General region streaming integration

Verify `region_stream.hpp` and selected-engine `docs/region-streaming.md` before use.
This development feature may not exist in a game's pinned engine checkout.

`RegionManifestData` owns scope, 1..128 unique keyed world AABBs, separate logical
visual/collision source IDs, 1 KiB..16 MiB part allowances and requiredOnRestore
collision keys. Scope/keys are 1..24 ASCII letters/digits/underscore/dash/dot.
`RegionManifest::create` validates before immutable publication.
`RegionManifestComponent(name, sharedManifest)` authors metadata only; hierarchy
world transform stays identity. Built-in prefab factory accepts that shared asset.

Register `RegionSource` through AssetManager. `capture(part, actorSpan)` validates
ordinary static actors and shares immutable resources while preserving saved IDs;
`create(part, bytes)` checks transport type/size, with full decode/content validation
at admission. Explicit visual payloads accept exact static MeshComponent (no
skin/morph/skeleton) / LightComponent. Collision payloads accept solid static body
and collider roots. Source IDs <=48 bytes, <=512 nodes, depth16. Content must fit
manifest bounds. All source nodes must be enabled; structural validation also
rejects disabled branches. An inactive target scene suspends publication and
eviction and reports readiness blocked until reactivated. Views, application data, ticking/custom gameplay components,
vehicles and derived terrain/route/FX payloads are not accepted. Terrain streams
retain their own ownership; prepare explicit static geometry/collision.

RegionStream borrows Scene/AssetManager; destroy it first while the scene is idle.
Keep the target Scene world transform identity. Construction and editor creation
reject transformed targets; later transformed/inactive scenes fail closed.
Application drives assets.update(), then stream.update(interests, extraPins).
Interests independently request visual/collision with bounded integer priority.
Defaults: resident8, in-flight4, starts2/publication attempts2 per update,
512 nodes/part, 1024 moving bodies, 64 MiB reservations, 2 m body margin. Active
parts reserve 3*allowance; encoded bytes and decoded allocations are separately
limited to allowance. This is admission accounting, not a hard process/GPU/Jolt
memory cap; provider/global asset caches remain application owned. Publication
and collision cooking are synchronous, with no frame-time guarantee.

Pins use authoritative active nonstatic Jolt body AABBs (including sleeping,
kinematic and sensor bodies), plus <=128 extra pin boxes. Set body margin/extra
pins for wheels, suspension, motors and other consumers. Unregistered/excess bodies
fail closed. Required pinned collision wins admission pressure and cannot be
evicted. collisionReady(predictedBounds) checks ready collision union coverage;
visualReady(key) is independent. Coverage is geometry presence, not a guarantee of
continuous road support. Applications must gate predicted movement and teleports;
no automatic braking, predictive prefetch or world rebasing is delivered.

IDs rg:<scope>:<key>:v|c:<sourceSavedId> survive eviction; runtime handles do not.
resolve() returns a borrowed resident node. regionOf(ActorId) exposes membership.
Keep persistent entities outside immutable payloads; retain logical references
for unloaded content. Do not edit/destroy managed residents. clear() refuses
occupied collision. Release consumers/extra pins before clearing; if destruction
cannot safely clear, scene-owned actors remain until scene teardown. Pending
requests drop safely; late callbacks never capture/mutate scene ownership.

Archive v31 reads v1..31, saves manifests/source references/selected restore keys,
and excludes managed transient resident subtrees and their resource discovery.
checkpoint(requiredKeys, applicationData, views, assets, codecs) clones persistent
state without altering authoring. Restore into a paused/unpublished Scene, create
its stream, drive IO, check restorationReady(), call finishRestore(), then hand
control to gameplay. Missing required collision prevents handoff. No solver-exact
or automatic per-region dynamic-state checkpoint is added. SceneArchive::cloneSubtree
preserves IDs for detached source preparation; duplicateSubtree still creates new IDs.

EditorSession createRegionManifest / selectedRegionManifest / setRegionManifest
provide exact grouped undo/redo. Browser Details -> Region streaming authors boxes,
keys, source IDs, allowances and restore requirements. Capture static selected
actors to separate .wescene sources; register validated files by ID for the current
preview session. Scene files retain references; consumers provide real loaders.
?editor=1&scene=regions opens the application-owned road example. Its isolated
Scene/camera/manager/stream shows delay/failure/focus/retry/deferred teleport,
65 m/s real VehicleComponent traversal and residency/pins/budget diagnostics.
Close preview before document edits/files or Play. Release renderer borrows before
destroying the preview. Synthetic delay is measured in preview steps, not seconds.

Validation: build/run native region_stream_tests (CTest region_stream) plus
scene_archive/persistence/application/assets/editor/editor_structure/prefab/physics/
vehicle regressions. Explicit Emscripten region_stream_wasm_tests.cjs runs in Node.
Use pinned local SDK 6.0.11 and web_assets to publish matching selected Wasm/host/
worker assets, serve the actual dist, and run scripts/region-stream-smoke.cjs for
WebGPU and forced WebGL2 at DPR1/2. It checks authoring/overlay/history, v31 reload,
source isolation, delay/failure/retry, collision gating at speed, pins/budgets,
teleport, resize and GPU recovery. Read the selected engine validation report for
actual runs; desktop Chromium checks do not establish physical mobile acceptance.
