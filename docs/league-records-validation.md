# Garage league continuation and saved results validation — 2026-10-23

This follow-up adds direct league continuation from the garage and persistent
current, latest and best league results. It changes RacingGame application code;
no WebEngine API or pin changes were required.

## Fresh acceptance evidence

| Target | Result | Coverage |
| --- | --- | --- |
| Native Windows / MinGW championship | Passed, 84.65 seconds | Scoring, progression, scene integration, five physical race scenarios, latest/best records, worse replays, partial-season archival, strict archive validation and save migration |
| Native Windows / MinGW paint regression | Passed, 0.94 seconds | Existing vehicle paint contracts remain compatible with the new save envelope |
| Chrome WebGPU | Passed, zero browser errors | Garage continuation, keyboard focus and acceleration, scheduled track travel, saved results, reload, replay, partial runs, paint preservation and stale navigation intent |
| Chrome WebGL2 | Passed, zero browser errors | Identical continuation and persistence acceptance against the same frozen site |
| Responsive browser UI | Passed | Results and garage continuation at 1440×900 and 390×844 |

Commands used:

```text
cmake --build build/native --target racing_championship racing_paint --parallel 4
ctest --test-dir build/native -R '^racing_championship$' --output-on-failure
ctest --test-dir build/native -R '^racing_paint$' --output-on-failure
python scripts/dev.py build --jobs 4
node scripts/league-records-smoke.cjs
```

The browser build uses the repository-local Emscripten SDK. Acceptance ran against
a frozen copy in `build/championship-records-site` on port 8138. Its served Wasm
SHA-256 was `89f83aa0f8afbcfc627ea33c1a6d8afc827ea240e1d5cfcd34b9131572b4ba69`.
The smoke checks that hash before running. The machine-readable report and
screenshots are in `build/evidence/league-records/`.

Historical results use a controlled profile emitted by the native championship
test executable: a Street title worth 60 points, a worse latest season worth
16 points, and an active District season. Both renderers preserved the best
title after the worse replay and preserved all four chassis paint values.
Continuation starts the current round immediately, including a one-use intent
when travel to another track is required. Abandoning an unfinished round awards
no points; stale intents leave the saved profile unchanged.

## Persistence and limits

`NA5` retains the player profile and paint values while `LC2` stores the active
season plus five bounded latest/best record pairs. Native checks cover legacy
NA3/NA4 migration, NA5 round trips and malformed archive rejection; existing
NA1/NA2 migration remains supported by the profile decoder. A league change or
restart archives any scored partial season. Finishing the points season and
promotion-final attempts update the records automatically.

Old builds cannot supply historical seasons they already overwrote. Local saves
remain specific to their browser profile and origin. These browser fixtures do
not establish naturally earned full-career balance, cloud storage or physical
mobile acceptance. Native validation covers portable rules and scene behaviour;
the selected consumer has no rendered native RacingGame host.

The preview at port 8137 now serves the same frozen build without changing its
origin or the player's save. An already running page needs a reload to adopt it;
the active race was left undisturbed.
