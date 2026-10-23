# Combat validation

Validation date: 2026-10-20. This extends the earlier AI-race evidence with
pulse weapons and shields on the player car, player hovercraft and five rivals.

## Build

- Engine: clean pinned `9b597da84d4582973d010fa9ba3e328c20181892`.
- Emscripten 6.0.11, Release; development-engine mode disabled.
- Generated consumer site copied from `build/web/site` to `build/combat-site`
  and served at `http://127.0.0.1:8089`. The copy remains fixed during testing,
  independent of concurrent car-model builds.
- Wasm SHA256: `a23d55f2bb184cf00114726418c3ff24cc73c2321152191c96a5da801999b107`.
- Desktop Chromium 155.0.8059.85; 1440 × 900 and 390 × 844, DPR 1.

## Native

`python scripts/dev.py native` passed in 37.99 seconds. Coverage includes:

- Pulse cooldown, shield drain and absorption, depletion/release/reactivation,
  and hit protection that outlasts disruption.
- Physical line of sight, range and rear-target rejection for both chassis.
  Explicit unshielded raycast hits reach the rival and activate disruption;
  unrelated AI attackers are parked away in this isolated fixture.
- Full three-lap combat races with normal racer collisions: all six finishers
  with either player vehicle. AI attacks and actual shield blocks are asserted.
- Finish protection, frozen race times, and full combat reset on restart.
- Existing drivetrain, imported model/wheel, steering, boost, brake, recovery,
  pause, suspension, ranking and checkpoint regressions.

The native car combat run recorded 119 player shots, 6 hits and 36 blocks; the
hovercraft recorded 126 shots, 21 hits and 27 blocks. Player recoveries were zero.
These counts describe observed runs, not fixed tuning expectations.

## Browser runs

The full race suite and focused combat UI suite are run serially against the
same frozen site. See the local reports for per-racer times and combat counters:

- `build/evidence/combat-final/results.json`: complete races and keyboard flows.
- `build/evidence/combat-ui/results.json`: energy lifecycle and narrow HUD.
- `build/evidence/combat-final/site-manifest.json`: SHA256 for all 18 served files.

Both suites passed WebGPU and WebGL2 with zero page, console or GPU errors.
All four full races ended with six classified finishers and zero player
recoveries. The player fired and used shields in each race; AI shots, disruptive
hits, shield uses and successful blocks were asserted rather than inferred from
visual effects.

| Backend | Player | Place | Time | Player shots | Hits | Blocks |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| WebGPU | Car | 1 / 6 | 98.833 s | 121 | 0 | 46 |
| WebGPU | Hovercraft | 2 / 6 | 102.367 s | 126 | 12 | 30 |
| WebGL2 | Car | 3 / 6 | 98.800 s | 121 | 1 | 43 |
| WebGL2 | Hovercraft | 3 / 6 | 102.733 s | 126 | 15 | 32 |

The leading WebGPU car fired mostly into open track and recorded no disruptive
hits; both chassis also passed explicit targeted-hit native fixtures. Across
the four browser races, AI landed 106, 74, 103 and 102 disruptive hits respectively.
Physical traffic and browser input timing can change outcomes between runs.

The focused UI suite verified shield energy drain, empty-shield lockout while
E stays held, recharge, release/reactivation, pause freezing combat, and resume
clearing held fire/shield inputs on both renderers. HUD bounding boxes did not
overlap at 390 × 844. Screenshots were inspected for the cannon tracer, player
and AI shields, imported car compatibility, results and narrow energy feedback.

Reproduce after building and serving a selected site:

```powershell
$env:DEMO_URL = 'http://127.0.0.1:8089'
$env:EVIDENCE_DIR = 'build/evidence/combat-final'
node scripts/prototype-smoke.cjs
$env:EVIDENCE_DIR = 'build/evidence/combat-ui'
node scripts/combat-ui-smoke.cjs
```

`NODE_PATH` must resolve installed `playwright` and `pngjs` packages. Use
`BROWSER_EXECUTABLE` to select a supported local Chromium executable.

This is desktop browser/synthetic-control evidence. It does not establish
physical mobile, gamepad, audio, sustained performance or human balance acceptance.
