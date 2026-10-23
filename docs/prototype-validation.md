# AI race prototype validation

Validated on 2026-10-20 using the generated `build/web/site` consumer build.

## Build identity

- Engine commit: `9b597da84d4582973d010fa9ba3e328c20181892`.
- Engine working tree: clean; development-engine mode: false.
- Emscripten: 6.0.11; Release build.
- Browser: desktop Chromium 155.0.8059.85.
- Viewports: 1440 by 900 and a 390 by 844 layout check, DPR 1.
- Wasm SHA256: `b5e0a2c012b6ee928d97fbbd1801f4d992b4d05dabf7d350a9b57b4aaac676f7`.

## Results

`python scripts/dev.py native` passed the game integration suite in 28.23 seconds.
It covers ordered forward checkpoint progression, reverse and shortcut rejection,
three-lap completion for both physical vehicles, camera-relative right steering,
acceleration, braking, boost consumption/recharge, manual and automatic recovery,
boosted barrier containment, pause, host suspension and restart. It now also
checks valid-sector ranking, lap precedence, six-racer grid order, mixed vehicle
classes, whole-field pause/restart, and complete physical races for all AI rivals.
Both player selections reached final classification; one AI recovery in the car
race exercised penalty-inclusive result ordering. Final clocks stayed frozen.

The prior isolated engine fix passed `engine_core`, `physics`, and `vehicle` regressions.
Its tests include near-quarter-turn transforms and the actual quaternion-derived
matrix that caused the original physics crash. The published fix is proposed in
[WebEngine PR 1](https://github.com/johdav999/WebEngine/pull/1).

`node scripts/prototype-smoke.cjs` passed WebGPU and WebGL2 serially on the same
unchanged build. Both vehicles passed real keyboard acceleration, screen-right D
steering, braking, boost, pause/time freeze, blur auto-pause, recovery, restart,
and vehicle selection. The suite then drove full races using synthetic actions
through the normal input adapter and real browser/Jolt clock, without teleports
or checkpoint injection. Every race waited for all five AI rivals to complete
three valid laps and checked six unique positions sorted by penalty-inclusive
times. Whole-field pause and restart passed. AI recovery was exercised in the
WebGPU hover race and WebGL2 car race. The browser reported no console, page or
GPU errors.

| Renderer | Player vehicle | Player position | Player time | Field completed | Player recoveries |
| --- | --- | ---: | ---: | ---: | ---: |
| webgpu | car | 1 / 6 | 92.017 s | 6 / 6 | 0 |
| webgpu | hover | 3 / 6 | 93.967 s | 6 / 6 | 0 |
| webgl2 | car | 1 / 6 | 92.517 s | 6 / 6 | 0 |
| webgl2 | hover | 2 / 6 | 93.950 s | 6 / 6 | 0 |

Times above are rounded to milliseconds. These are observed runs, not fixed
expected placements: physical contacts and browser input timing can change
overtakes and finishing order.

Screenshots were inspected for the city, vehicles, HUD, garage, finish screen,
and narrow layout, including all six result rows and both result actions.
Detailed local evidence is in
[the final browser report](../build/evidence/ai-race/results.json), with
screenshots in the same directory. Earlier diagnostic runs are separate from
this final evidence.

This is desktop browser and synthetic-control acceptance, not physical mobile,
real-controller, sustained frame-rate, human handling, or final art acceptance.
The current milestone is a six-racer physical AI race; the larger career and
combat design remains future work. AI uses a circuit-specific driving policy
with basic passing and recovery, not a general traffic or combat planner.
