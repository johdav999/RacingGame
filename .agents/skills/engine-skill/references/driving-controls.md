# Gamepad input and composed follow cameras

Read selected engine docs/driving-controls.md and docs/driving-controls-validation.md,
then inspect web/gamepad.js, input.js, camera_component.hpp, camera_controller.hpp,
camera_follow.hpp, physics.hpp and codecs. Verify the consumer checkout/pin first.
This development functionality is not established by the CatGame engine pin.

Browser: load input.js then gamepad.js. Attach input.gamepad = new
EngineInput.GamepadInput(input,{allowed,getGamepads}) to the existing BrowserInput.
flush polls once before the normal Application input snapshot; do not add a second
RAF/simulation/input owner. bind({context,kind:'axis'|'button',index,action,value,
deadZone,invert,exponent}) returns an unbinder. Axes clamp -1..1; buttons0..1;
missing/non-finite samples become0. Axial dead zone remaps magnitude, then raises
it to exponent. Defaults .15 axis/.05 button, exponent1, inversion off. Validate
0<=deadZone<1, exponent.25..4, finite value components -1..1, index0..63; max128
bindings and16 device slots. No radial/temporal response filter is added.

Sources come from BrowserInput.nextSource and remain independent of keyboard and
virtual controls. Pad release/disconnection/binding removal/selection zeros only
owned sources. Context handoff retains the existing global cancellation policy.
select(null) chooses/retains a standard controller automatically; explicit index
supports custom mappings supplied by applications. Connection events cancel a
matching active index, including between polls. Device replacement, blur, hidden
page, resize, text/GUI ownership, context/focus, enable and lifecycle changes require
neutral before re-arming. Device arrays/status/errors are bounded. Canvas or a
registered virtual control must own focus; allowed() is a host gate. Dispose owned
subscriptions/adapter before its input owner; BrowserInput.dispose handles the
attached adapter. No browser/permission/device state belongs to scene archives.

Application vehicle.js maps leftX steer, RT throttle, LT brake, A handbrake and B
reverse. The gamepad/status/controller/inversion and hold-preview controls live in
the vehicle panel. They reuse action sources and work during isolated Play/live
simulation; idle authoring cannot drive. Browser editor data-live-input-control
allows designated host controls during Play while authored scene fields stay gated.
Games own mappings, response and speed-dependent steering policy. No user settings
store, haptics, hardware calibration or controller availability is promised.

CameraTarget.poseSource defaults Simulation; Presentation samples the same current
PhysicsWorld.presentationMatrix as collection, including body/character descendants
and bound wheel visuals, independent of visibility/LOD/renderer. It never creates
physics or writes simulation. CameraTarget.velocity uses authoritative rigid-body
point velocity (else0), not render finite differences. TrackingCameraBehaviour adds
lookAheadSeconds(default0) and maxLookAhead(default5m), finite nonnegative; velocity
*time is clamped to that distance. Use target-local offsets deliberately.

CameraComponent stores validated CameraFollowSettings data. Application-owned
CameraFollowRig(scene,cameraHandle) explicitly composes Follow, Tracking, half-life
Smoothing and final Collision after fixed simulation/interpolation, before render.
Scene outlives the rig; clear at scene/Play/view handoffs. Keep one camera writer;
do not also call OrbitController. CameraFollowRig.reset cuts temporal history and
next valid update snaps with collision still applied. Missing/disabled/removed
camera or target freezes output safely and clears history. New target generations
and pose sources rebind/snap. Composition cannot influence vehicle simulation.

Defaults: disabled/unbound; presentationPose/localOffset/collision true; focus
(0,1,0), boom(0,3,-8), look-ahead.3s/max4m, position/rotation half-lives.12/.1s,
radius.3m/clearance.05m, layers0xffff. Offsets finite within +/-1e6m, time/half-lives
0..10s, max look-ahead0..10000m, radius(0,100]m, clearance0..100m. setFollow validates
before replacement. Saved target strings are empty or valid stable identities;
editor assignment resolves an actor/component or '-' clears. Unresolved portable
saved targets may wait for application spawning. Direct self-follow rejects.
PrefabCameraSettings.follow supports the existing camera factory.

Select CameraComponent -> Camera follow for typed history (target, pose source,
offsets, look-ahead, half-lives, collision/mask). The viewport camera's follow data
is editable while navigation still owns transform/lens/structure. Play clones
settings and has a separate rig; Stop preserves source bytes/history. Runtime
smoothing/transition state, callbacks, devices, held input and bindings are transient.
Current archive writer v31 retains settings; old cameras default follow off.
Duplicate internal target IDs remap; external saved references stay external in
the same scene. Reconstruct rig/bindings in applications; no automatic camera tick.
Deploy matched host/main/worker artifacts. v25 introduced physical surface fields;
v24 introduced vehicle fields. Camera collision is boom shortening against current
simulation geometry, not general penetration recovery or guaranteed framing.

Build with selected repo-local pinned SDK (.emscripten-version,6.0.11 here).
Native CTest camera_follow plus camera_controller/input/editor/prefab/scene_archive/
persistence/vehicle/physical_surface; run full suite after schema changes.
Build camera_follow_wasm_tests and affected vehicle/physical_surface_wasm_tests,
run .js via Node. node scripts/gamepad-adapter-tests.cjs checks device source/gate
contracts without hardware. Publish web_assets, serve actual dist and run
scripts/driving-controls-smoke.cjs at DPR1 and DPR2/separate EVIDENCE_DIR, both forced
WebGPU/WebGL2. It checks synthetic getGamepads, source overlap, disconnect/event,
selection, gates, touch cancellation, interpolation/look-ahead/collision, authoring,
Play/hash, save/IndexedDB, resize/recovery. Regress vehicle-smoke and physical-surface-smoke on both backends; input-smoke accepts RENDERER=webgpu|webgl2 for existing
keyboard/touch/text/GUI checks. Record actual browser/viewport/backend, hashes,
telemetry, errors and inspected screenshots. Synthetic desktop tests do not prove
physical controllers/mobile devices, latency, sustained performance or handling.

`OrbitController::adoptCamera(pivotDistance)` validates/captures the current local eye/yaw/pitch without camera writes; roll is not an orbit degree of freedom. Use at a real composed-to-orbit/editor handoff to avoid stale cached orientation. Preserve restored orbit metadata when orbit already owns the view. Native/Wasm and both-backend browser tests cover chase-to-editor handoff.
