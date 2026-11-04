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
