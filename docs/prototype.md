# First playable prototype

The current build is a six-racer, three-lap race on Kairox's Midnight Circuit.
Choose the Vektor wheeled car or the Wisp hovercraft in the garage, enter the
circuit, complete the countdown, and pass each checkpoint in order. The result
screen reports all six finishing positions and times, best player lap, and
recovery penalties. Five named AI rivals drive a mixture of cars and hovercraft.

## Play

```powershell
python scripts/dev.py native
python scripts/dev.py build
python scripts/dev.py serve --port 8088
```

If the local SDK is not installed, run `python scripts/dev.py setup` first. On
this Windows machine the existing SDK may be selected with:

```powershell
$env:EMSDK = 'C:\Users\Johan\source\repos\WebEngine\.tools\emsdk'
```

This reuses only the toolchain; game builds still select `external/WebEngine`.
Open `http://localhost:8088/?renderer=webgpu` or
`http://localhost:8088/?renderer=webgl2`. Click Enter Circuit to focus driving.

| Control | Action |
| --- | --- |
| W or Up | Accelerate |
| S or Down | Brake, then reverse a wheeled car once it slows; hover air brake |
| A and D or Left and Right | Steer |
| X | Reverse directly (brakes a moving car before changing direction) |
| Space | Car handbrake or hover slide |
| Shift | Boost while accelerating |
| F (hold) | Fire the pulse cannon when its cooldown permits |
| E (hold) | Activate the energy shield; release to recharge |
| R | Recover to the last cleared gate, with a three-second penalty |
| Escape | Pause or resume |

Losing window focus pauses the run and cancels held inputs. Resume explicitly;
release and press controls again after an input handoff. Restart clears the
current run, and Garage / Upgrades returns to the garage. Finishing time includes
recovery penalties. Lap records last only for the current run; credits and vehicle
upgrades save between sessions. Final standings must settle before leaving results.

## Delivered systems

The Vektor player car uses the [textured Cybercar with independently animated
wheels](player-car.md). Wheel spin, front steering and suspension follow the
physics drivetrain.

The current build includes [pulse combat and shields](combat-prototype.md) for
the player and every AI rival.

- Two distinct physical vehicles. The car uses WebEngine's four-wheel Jolt
  drivetrain and suspension. The hovercraft uses a dynamic collision body with
  spring height control, yaw stabilization, thrust and lateral damping.
- An approximately 550-metre closed circuit with a 20-metre road, crash barriers,
  twelve checkpoint gantries, road markings, neon towers and a sky transit line.
- A chase camera, countdown, speedometer, battery meter, minimap, next-gate
  marker, lap timer, results, restart, pause and automatic off-course recovery.
- Boost consumes a regenerating battery. The vehicle classes differ in braking,
  lateral grip, slide behavior and forward propulsion.
- Five physical AI rivals use the same car/hover construction, drivetrain
  controller, collision bodies, ordered gates and three-second recovery penalty.
  They follow offset circuit lines with different target speeds, adjust their
  line around nearby traffic, brake for large heading errors, and recover if
  overturned, off course or stalled. There is no progress teleport or speed
  rubber-banding.
- A staggered start grid, live six-place leaderboard and rival minimap markers.
  Positions interpolate only inside each racer's last validated sector.
  Finishing classification sorts total time including penalties, with racer ID
  as the deterministic tie-breaker. Until everyone finishes, the results are
  explicitly marked live; unfinished entries show their current lap.
- After crossing the finish, the player's vehicle follows the circuit under AI
  control while the remaining racers finish. Finished clocks freeze. Once all
  six finish, the field brakes and the classification becomes final. Pause and
  restart affect the entire field, including post-finish driving.
- Sequential forward gate crossings, bounded position steps, off-course
  invalidation and explicit recovery prevent shortcut or reverse-lap credit.
- Native gameplay regressions and browser acceptance against both actual
  renderer backends.

The scenery and rival vehicle meshes are procedural prototype art. Race winnings,
upgrade purchases and local saves are described in [progression](progression.md).
Driving, combat and race-cue sound is now implemented; see [racing audio](racing-audio.md).
[League career](championship.md) is implemented. Additional weapon types, gamepad and touch driving are not yet
implemented. There is no free flight or open-world city.

## Ownership and source map

`src/race_rules.hpp` owns the shared circuit geometry and portable race rules.
The same circuit points feed rendering, collision-barrier placement, checkpoint
locations and the browser minimap.

`src/starter_scene.cpp` assembles the game scene and vehicles. A scene-owned actor
runs driving and race rules on fixed ticks before Jolt steps. Jolt owns vehicle
translation and collisions; the hover controller applies forces and angular
velocity rather than rewriting the body's transform. Recovery teleports only at
an idle scene boundary. A single CameraFollowRig owns camera presentation.
All 143 skyline locations use shared textured city-kit models with automatic LODs.
`VehicleRuntime` holds each chassis's drivetrain, controls and race progress.
The application-owned circuit pilot sends the same `DrivingInput` consumed by
the player vehicle. It does not use a second physics solver or frame scheduler.

`src/browser_main.cpp` owns Application, Renderer, input submission, lifecycle
commands and telemetry. Application is the only simulation scheduler. Browser
commands are queued and applied at frame boundaries; scene updates are not run
by a second JavaScript loop. The host suspends simulation when hidden or while
the renderer is unavailable.

`web/prototype.js` maps physical keys into source-isolated engine actions and
presents telemetry through the HTML HUD. It contains no vehicle simulation or
race-authority state. `web/prototype.css` owns responsive presentation.

## Engine dependency

The selected engine is published commit
`9b597da84d4582973d010fa9ba3e328c20181892` on
`codex/stable-vehicle-rotation`, proposed in
[WebEngine PR 1](https://github.com/johdav999/WebEngine/pull/1).
It fixes valid physics poses being rejected near 90-degree headings. The fix
was made in an isolated checkout; the adjacent development checkout was not
modified. The submodule is clean and the normal consumer build uses this pin.

Steering is defined relative to the chase camera: D turns toward screen-right.
Because the vehicle faces local +Z, this decreases world yaw. Native checks
measure heading and travel against the camera's right vector; browser checks
exercise the actual D key. Recheck this mapping when changing the engine's
VehicleComponent adapter instead of relying on a command sign alone.

## Repeatable validation

See [the recorded validation results](prototype-validation.md) for the tested
engine pin, Wasm hash, browser version and completed race results.
The later [combat validation](combat-validation.md) covers weapons, shields and
the combined imported-car build.
The [garage and progression](progression.md) adds race payouts, separate vehicle
upgrades and browser-local saves. The race suite also buys upgrades with earned
credits and verifies them after reloading.

`python scripts/dev.py native` checks race ordering, reverse/shortcut rejection,
recovery penalties, automatic off-course recovery, boosted barrier containment,
acceleration, screen-right steering, braking, boost consumption and recharge,
pause/suspension, whole-field restart, chase framing and three physical laps for
both player vehicles and every AI rival. Ranking checks cover valid-sector
interpolation, lap precedence, penalty-inclusive ordering and frozen finish times.
Its timeout allows 120 seconds for slower CI hosts.

With Playwright and pngjs installed, set `NODE_PATH` to their node_modules
location and optionally set `BROWSER_EXECUTABLE` to Chrome. Then run:

```powershell
$env:DEMO_URL = 'http://127.0.0.1:8088'
node scripts/prototype-smoke.cjs
```

The suite runs WebGPU and WebGL2 serially, checks the clean engine build identity,
uses real keyboard controls, tests pause/blur/recovery/restart, checks a 390-pixel
layout, and drives both vehicles through three laps with synthetic actions on
the ordinary browser/Jolt clock. It waits for all five AI rivals to finish each
race and verifies the final six-position classification. It captures screenshots, telemetry and console
errors in `build/evidence/prototype`. Set `EVIDENCE_DIR` to choose another output;
`RENDERER=webgpu` or `webgl2` narrows the run. `SKIP_FULL_RACE=1` runs only the
short interaction checks and must not be reported as full-race acceptance.

Browser automation establishes desktop Chromium behavior. Synthetic steering is
not a human handling review, and a narrow viewport is not mobile-device or touch
acceptance. Renderer CPU counters do not establish sustained frame rate.
