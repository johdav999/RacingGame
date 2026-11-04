# Sirocco Solar and Canyon Relay

Two new selectable circuits reuse Desert Solar's complete environment and race
kit. Choose them from **CIRCUIT** in the garage, or use:

- `?track=sirocco&renderer=webgpu` — **Sirocco Solar / Dune Sweep**, approximately
  1,277 metres. A long opening solar straight leads into the eastern sweep,
  linked dune bends and the broad western return.
- `?track=canyon&renderer=webgpu` — **Canyon Relay / Twin Mesa**, approximately
  1,462 metres. Alternating eastern mesa bends lead into a northern crown and
  the western canyon/conduit return.

Replace `webgpu` with `webgl2` to select the fallback renderer. Circuit travel
retains renderer and event selection, credits, upgrades and chassis. Existing
league schedules continue to use their original circuits.

## Shared art and race behavior

Both tracks use Desert Solar's orange sand, irregular dunes, warm daylight and
haze, textured sandstone formations and rock arch, mirror fields and receiver
towers, solar operations house, blue energy conduits, two dune villages,
track dividers, checkpoint gantries, asphalt and contact shading. The same 20
verified environment GLBs load only for the selected desert scene. Existing
vehicle, weapon, audio and navigation assets are shared too. No assets were
generated or duplicated and no provider jobs were submitted.

Each circuit has its own closed spline, 192 distance-resampled road points,
20 metre asphalt width and 12 ordered checkpoints. Road mesh, barriers,
checkpoint gates, AI planning, minimap and recovery share those points. The
existing scenery planner follows the new routes, grades the terrain and keeps
formations outside the road. These are new routes rather than reversed laps.

All four chassis support three-lap six-racer races, combat, boost, pause,
recovery, final standings and payouts. Solo trials retain stock tuning, three
sectors, clean-run validation, a personal-best ghost and replay. Stable track
IDs 4 (Sirocco) and 5 (Canyon) give each chassis separate `NATT1` browser records;
existing Kairox/Helios records keep IDs 0/1. IDs 2/3 are reserved for the city
circuits being developed concurrently in a separate branch. Career saves and
league records keep their existing format.

## Validation

Use the pinned `external/WebEngine`, then build and run:

```powershell
python scripts/dev.py native
python scripts/dev.py build
python scripts/dev.py serve --port 8142
node scripts/desert-circuits-smoke.cjs
node scripts/desert-circuits-storage.cjs
```

The browser script requires Playwright and desktop Chrome (override its path
with `BROWSER_EXECUTABLE`); `DEMO_URL` can select a different serving address.
It checks both forced renderers against the local Wasm SHA-256, all twenty
loaded environment models, absence of city downloads, both distinct minimaps,
all four chassis with keyboard driving/fire/recovery/pause, trial invalidation,
scene travel with saved career, narrow layout and original Helios startup.
Scenic screenshots and reports are written to `build/evidence/desert-circuits/`.

Native `racing_desert_circuits` imports the real art and tests road separation,
rendered sand/asphalt clearance, cliff burial, physical driving clearance,
recovery and pause, then complete physical races with every chassis on both
tracks. `racing_desert_trials` checks a completed physical solo lap, saved best,
ghost replay, pause, reload and invalidation on each addition.
`racing_trial_rules` covers ordered sectors, record codec and key isolation;
`racing_ai` covers corner-aware planning on both new routes.

The core/physics tests use Windows executables. Visual acceptance uses desktop
Chrome; physical phones and a native desktop rendering host are untested.
The existing renderer differences documented in `desert-solar-track.md` remain.

The storage browser check uses different synthetic codec records for IDs 4/5
and verifies that garage restoration and replay select the correct record on
both backends. These fixtures are storage tests, not lap-performance evidence.
The native trials produce and reload real physically driven records.

## Recorded desktop acceptance — 2026-11-04

Chrome **156.0.8078.80** passed `desert-circuits-smoke.cjs` on forced WebGPU and
WebGL2: both tracks, all four chassis, all 20 environment models, keyboard
controls/combat/recovery/pause, trial invalidation, career-preserving scene
travel, narrow layouts and original Helios startup. Both backend runs produced
no page or console errors. Thirty-two garage, race and scenic screenshots were
captured and representative views inspected on both renderers.

`desert-circuits-storage.cjs` passed distinct personal-best restoration and replay
on both backends. Native `racing_desert_trials` passed actual completed solo
laps, saved-best reload and ghost replay on both additions. The affected native
AI planner, trial codec, championship, lighting, race-kit and village checks
also passed.

The exact browser Wasm SHA-256 was
`5eeb18ead6a27401633736a0b379093a765ad56c246ad7de8fe3fe77e210eb37`.
The build uses clean published engine pin
`619ce5e7b5fc2965754b4fd4ce0c4cf96494f018` and Emscripten 6.0.11.
Reports are `build/evidence/desert-circuits/report.json` and
`storage-report.json` in that directory.

Native `racing_desert_circuits` passed in **363.00 seconds**: eight complete
three-lap physical events covering Vektor, Skimmer, Bastion and Vector on each
new circuit. Every player and AI racer finished, with zero recoveries across
all 48 finishes. The checks also verified exact payouts, frozen final results,
restart state, full driving clearance, road separation and terrain clearance.
The test driver uses ordinary corner-aware steering/braking and can physically
reverse out of traffic contact; it never teleports the player around a corner.
