# Time trial and personal-best ghost

Choose **Time trial / ghost** in the garage's Event selector, then a circuit and
chassis. **Start time trial** runs one lap from a standing start. A cyan ghost
automatically replays that track/chassis's personal best alongside you. After
setting a best, **Watch best run** is available in the garage and results screen.
Escape pauses both driving and playback; the pause menu can restart or return to
the garage. Replay ends at the recorded finish time.

Both Kairox and Desert Solar have independent records for Vektor, Skimmer,
Bastion and Vector (eight records). Trials use stock tuning, regardless of career
upgrades. Boost is allowed. Rivals and combat are disabled, and trials award no
career credits. Switching back to Race restores the normal three-lap field and
career upgrades.

The three sector lines are ordered checkpoints 4, 8 and the finish at 0. The HUD
shows each sector's duration and the cumulative time difference from the previous
best at that line: negative is ahead, positive is behind. The comparison stays
fixed throughout an attempt, including when that attempt sets a new best.

Only a clean, strictly faster finish replaces a record. All twelve checkpoints
must be crossed in order and forward. Leaving the validated circuit, recovering
(including automatic recovery), or reaching the ten-minute limit invalidates the
attempt. Returning to the road does not clear invalidation; restart for a fresh
attempt. Pausing does not add time. Abandoning an attempt leaves the best intact.

## Implementation and storage

`src/time_trial.hpp` owns the versioned run codec, bounded pose recording and
interpolation. `src/time_trial.cpp` owns scene integration and ghost visuals.
`StarterScene` advances trial timing on its normal fixed simulation clock; the
browser never decides whether a run is valid or faster. Sector timing has the
same fixed-step resolution as race timing. Poses are recorded at approximately
20 Hz with an explicit start and finish, then interpolated with shortest-path
Euler rotation. The ghost uses the selected chassis's shared meshes and cyan
translucent materials, with no rigid body, colliders, AI or combat components.
It records the chassis pose; wheel spin, suspension animation, sound and effects
are not recorded. Playback follows the ghost with the chase camera.

Browser saves use `neon-ascension.trial.v1.<track>.<chassis>`, independently from
career progression. The `NATT1` payload includes track/chassis, exact finish and
split times, and bounded timestamped poses. Decoding validates version, identity,
finite values, ordered splits/timestamps, sample continuity, endpoint coverage,
size and duration before replacing any in-memory record. Future rule, route or
tuning changes that affect comparability must increment the format/storage version.

Unreadable saves are preserved. Storage denial, quota exhaustion or a conflicting
write from another tab displays a status and keeps new bests for the session.
Records are local to the browser and origin; changing the localhost port uses a
different save store. There is no account sync or online leaderboard.

## Checks

`python scripts/dev.py native` includes `racing_trial_rules` and `racing_trial`.
The latter drives physical laps for all four chassis on both circuits and checks
stock tuning, replay lifecycle, pause, recovery invalidation, record identity,
disabled rivals/combat and absence of career rewards. It uses the existing native
integration host; this repository's selected engine has no native game renderer.

After `python scripts/dev.py build`, serve the generated site and run
`node scripts/time-trial-smoke.cjs` with Playwright on `NODE_PATH` and `DEMO_URL`
set to that local server (default port 8093). It drives physical Kairox laps on
forced WebGPU and WebGL 2, checks saving and reload, watches full replays, races
the saved ghost, verifies recovery, chassis/track isolation, normal-race return,
and exercises corrupt, full and concurrently changed storage fixtures. Screenshots
and the report are written to `build/evidence/time-trial`. Narrow-layout checks
are browser viewport tests, not physical mobile-device acceptance.

## Validation, 2026-10-22

The final native `racing_trial_rules`, `racing_audio` and `racing_trial` checks
passed (3/3). All eight physical trial laps completed cleanly; the final test also
checks slower-record protection and the exact ten-minute cutoff. The existing
14 native regression checks passed during implementation, including full races
and Desert Solar.

The full browser suite passed on both forced WebGPU and WebGL 2 in Chrome
155.0.8059.85, with no captured page/console errors. It covered real input-driven
laps, durable saves, exact reload times, complete replays, pause, ghost retries,
recovery invalidation, record isolation, return to normal races and all three
storage failure cases. Desktop and 390px screenshots were inspected. A focused
final replay check also passed on both renderers for the playback minimap and
mode-preserving renderer links.

Both backends verified the served Wasm against the generated file:
`f506ff5648b1182294f21f00ddb3b5864f0a9b63464729baf83ccc73e53a37fd`.
The selected engine was clean at `9b597da84d4582973d010fa9ba3e328c20181892`,
using Emscripten 6.0.11, with development-engine mode disabled. Machine-local
screenshots, recorded-run fixtures and the full report are in
`build/evidence/time-trial/`.
