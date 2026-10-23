# Vehicle shield effect

Hold **E** during a race. The player and all AI chassis use the same effect:
a translucent cyan hexagonal membrane, soft silhouette, and travelling scan.
Activation ramps in over 125 ms; release fades out over 200 ms. Below 25% of
the installed capacity the field warms toward amber. Depletion produces a
360 ms expanding amber collapse. Blocked pulses stop at the ellipsoid's ray
intersection and emit a 480 ms ripple at that point, including a final hit
that exhausts the shield. Protection, charge and upgrades still belong to
`CombatState`; fading visuals do not extend protection.

`ShieldEffect` in `src/shield_effect.hpp/.cpp` is owned by each vehicle actor.
Chassis installation fits its envelope to imported vertices, resting wheels
and mounted equipment. Art swaps retain the component. The scene advances
its transient state on fixed simulation ticks after shots resolve. Presentation
positions the silhouette relative to the camera without advancing time. Pause
freezes animation; recovery and restart clear impact locations and fades.

The implementation uses the selected WebEngine's existing additive, depth-tested
unlit materials. No custom shader, engine edit, provider call or downloaded
texture is required. Opaque scenery and cars occlude the field normally; the
effect does not cast shadows, receive decals, or add colliders. It keeps the
existing collision-based combat rules: the visible shell is presentation, not
an enlarged targeting volume.

Resources are shared and immutable: one 64-by-32 sphere, one 96-segment feathered
annulus, a 512-by-512 antialiased lattice and a 1-by-64 alpha ramp. Three ripple
slots per vehicle overwrite the oldest impact during bursts. The maximum is
six mesh submissions and 4,928 triangles per visible shield, or 36 submissions
for all six racers; an inactive shield submits nothing. Per-frame work changes
transforms and material parameters, without spawning objects or rebuilding mesh
or texture data. These are bounded costs, not a sustained FPS guarantee.

## Verification

Build the pinned consumer with `python scripts/dev.py build`. Serve only the
generated site. With Playwright and pngjs on `NODE_PATH`, run:

```text
cmake --build build/native --target racing_shield
ctest --test-dir build/native -R "^racing_shield$" --output-on-failure
node scripts/shield-effect-smoke.cjs
```

Set `DEMO_URL` to the served site; default is `http://127.0.0.1:8098`.
The browser suite runs WebGPU then WebGL2, covers all four chassis, compares
actual car-region pixels, and exercises pause, release, depletion, reactivation,
movement, recovery, garage return and a 390px viewport. Evidence is saved under
`build/evidence/shield-effect`. `window.racingShieldVisual` exposes read-only
presentation telemetry; it cannot activate the shield or change resources.

The native test covers immutable resource sharing, bounded impact bursts,
first-tick depletion, inside-camera handling, off-centre ray intersections,
reset and an actual physics-raycast block by an AI racer. See
[validation record](shield-effect-validation.md) for the build tested and limits.
