# Wheeled vehicle integration

Verify the selected checkout/pin before using these development APIs. Vehicle
foundation is implemented in WebEngine; it is not established by a CatGame pin.
Read engine docs/vehicles.md and docs/vehicles-validation.md first.

Include engine/vehicle_component.hpp. Optional VehicleComponent uses a shared
immutable VehicleDefinition and separate per-instance commands/telemetry/checkpoint.
It requires a top-level unit-scale chassis actor with a direct solid dynamic
RigidBodyComponent and convex-leaf ColliderComponent (including compounds). Exactly four wheels, ordered
FL/FR/RL/RR; +Y up, +Z forward. Private Jolt constraints/listeners/testers are owned
by scene physics. No Jolt or browser types enter portable interfaces.

Supply vehicle.submit({throttle,steering,brake,handbrake}) before fixed simulation.
Signed throttle [-1,1] requests automatic forward/reverse, +steering turns right;
brake/handbrake are [0,1]. Commands hold until replaced/cancelled. Cancel on control,
lifecycle and input handoffs. Normal Application scheduling remains authoritative.
Post-step state() exposes active, signed speed m/s, RPM, gear and coherent wheel
contact/normal/handle, suspension, steer/spin/angular velocity, slip and local poses.
Resolve snapshot actor handles before using. Never query presentation to drive physics.

Defaults: 1200 kg is the example preset, not a component/body default. Definition:
500 N m, 1000–6000 RPM, ratios 2.66/1.78/1.3/1/.74, reverse -2.9, differential 3.42,
rear drive (frontTorqueFraction 0), shifts 4000/2000 RPM, anti-roll 1000. Wheels:
.35 radius/.2 width, suspension .1–.45 m, 2 Hz/.6 damping, front steer .5 radians,
1500 N m brakes, rear handbrakes 4000 N m. Copy/validate/publish definitions; never
mutate published resources through retained mutable aliases. Configuration rebuild
resets commands/drivetrain/contact state; no automatic remapping of held input.

RigidBodyComponent adds world addTorque(N m), addForceAtPosition(N,metres),
addImpulseAtPosition(N s,metres), pointVelocity(worldPoint) in m/s. Invalid/inactive
bodies reject commands; force/impulse requires dynamic motion. COM offset is
actor-local and does not move geometry. inertiaScale modifies local inertia tensor
as S I S with S=diag(sqrt(scale)), not a principal-axis override. Settings rebuild
bodies and follow velocity-reset policy. Do not duplicate Jolt vehicle force work.

Bind optional direct nonphysics visual children with setVisuals(array of saved IDs).
Their local transforms are offsets/scale after the sampled wheel pose. Renderer
collection interpolates chassis and wheel previous/current poses together without
writing authoritative transforms. Teleport clears interpolation and vehicle runtime;
synchronize before issuing new commands. Missing body/collider suspends registration;
re-enable/replacement recreates it. Scene teardown/body removal releases listeners
before solver bodies. Clear removed visual IDs before rebuilding/saving.

Editor: ?editor=1&scene=vehicle; select VehicleComponent for engine/ratios/axles and
four wheel property groups, visual saved IDs (- clears), plus body COM/inertia.
Vehicle foundation controls live in the Details dock during edit/Play and in the
playground sidebar otherwise. Create vehicle preset is undoable. Add/remove,
duplication, history, files and isolated Play reuse existing editor paths. Telemetry
is read-only and reports the first vehicle. The example ?scene=vehicle has flat,
hill, bank, kerb, ramp, wall and reset content, block wheels and a following orbit
camera. W/S drive/reverse, A/D steer, Space brake, Shift handbrake through existing
actions; toggle Driving controls and focus canvas. Reset is runtime-only.

V24 scene/application/effect envelopes retain shared definitions, remapped internal
visual references, COM/inertia settings and partial checkpoint RPM/gear/wheel spin
and angular velocities alongside existing chassis pose/velocities. checkpoint()/
restore() validate/stage data; held commands, contacts, solver/listener handles,
clutch/shift timers and render caches are excluded. Old bodies default zero COM /
unit inertia axes. This is not deterministic replay. Play shares definitions while
owning independent solver state. Archive duplication rejects external visual refs.
Standalone vehicle asset-library cataloguing, traffic, damage and
simulation-grade calibration remain later work. Stage 2 physical surfaces and
compound chassis are implemented; see the extension below.

Validation: repository-local SDK pinned by .emscripten-version, currently 6.0.11.
Native cmake --build <native-build> --target vehicle_tests; ctest --test-dir
<native-build> -R vehicle --output-on-failure. Regress physics/joints, editor,
scene_archive, persistence and prefabs. Build vehicle_wasm_tests in an Emscripten
build and run its .js with Node to check the 32-bit ABI. Build/publish the exact
web_assets target, serve its dist, set NODE_PATH to Playwright/pngjs,
BROWSER_EXECUTABLE to Chrome/Edge, and DEMO_URL to actual local URL, then run
node scripts/vehicle-smoke.cjs. Repeat with DPR=2 and a separate EVIDENCE_DIR.
The suite forces WebGPU/WebGL2 and checks simulation invariance, actual pixels,
editor history/invalid input, Play isolation, binary saves/IndexedDB, blur/suspension,
resize/recovery. Native tests check slopes, banks, CCD wall, handbrake, interpolation,
point forces and lifecycle. Retain exact build/evidence metadata; these checks do
not establish physical mobile, real-controller, sustained fleet performance or
human handling approval. See recorded validation for checks actually run.

## Stage 2 extension

Compound chassis, physical surfaces and combined wheel grip are now available in the development engine. See [physical-surface integration](physical-surfaces.md); verify consumer pin availability. Current writer v31 retains v25 physical-surface and v24 vehicle fields. Damage/traffic/game rules remain application work.

## Stage 3 extension

[Gamepad and camera integration](driving-controls.md) supplies shared action mapping, isolated live preview controls and inspectable device status. Camera follow settings live on CameraComponent; CameraFollowRig is an explicit presentation writer with bounded velocity look-ahead and final collision, independent of vehicle simulation. Defaults and persistence are v26; devices/bindings/held state remain transient.

## Closed-circuit application example

Use the selected engine's ?scene=formula and docs/formula-demo.md as a composition
example for closed spline asphalt/kerbs, source-collidable terrain, barriers,
vehicle tuning and a single chase-camera writer. src/formula_laps.hpp owns ordered
checkpoints, forward finish crossing, transient best/current lap timing and invalid
shortcut detection. These are game rules outside engine_core. Browser formula.js
adapts existing input actions and provides the HUD/minimap, reset/pause and touch
buttons; it owns no second vehicle simulation. The 2.69 MB red driving GLB retains the
supplied source model, with baked visual wheels; do not imply separate wheel art
animation or race-state persistence. Validate native formula_demo_tests and
scripts/formula-smoke.cjs on both backends; verify selected consumer pin availability.

The portable VehicleCommand steering contract is positive-right (+X when facing
+Z). The private Jolt adapter now negates that command at SetDriverInput; Jolt's
native positive angle turns left. Preserve this adapter conversion, do not invert
individual keyboard/gamepad bindings. Directional vehicle/formula tests assert
actual chassis travel. Wheel steerAngle telemetry remains the solver's angle.

Formula circuit mouse steering is inverted and on by default in ?scene=formula. Click the
canvas, then move horizontally relative to its centre; a 4% dead zone straightens
and full steering is reached at 35% of canvas width. The HUD toggle disables it.
web/formula.js owns one allocated BrowserInput Move source; throttle/brake keep
independent sources. Held A/D/arrows override mouse steering until fresh movement.
Focus/context/lifecycle gates, pointer exit, reset/pause and resize neutralize it;
touch/pen events cannot write this source. No core API or native build changes
are required. Validate scripts/formula-mouse-smoke.cjs on both renderer backends.

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
