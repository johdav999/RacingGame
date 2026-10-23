# Progression validation — 2026-10-20

Tested the frozen generated consumer site at `http://127.0.0.1:8090`, staged in
`build/progression-site`, using Chrome 155.0.8059.85. WebGPU and WebGL2 were forced
and the actual backend checked. Suites ran serially against identical files.

- Engine: clean published pin `9b597da84d4582973d010fa9ba3e328c20181892`.
- Emscripten: 6.0.11, Release.
- Wasm SHA-256: `d7c89d403782f816ce282ff8aa9b3d6dbe799963353e3b39bdc4c6ab55c9a463`.
- Desktop viewport: 1440 × 900; narrow layout: 390 × 844, DPR 1.
- No browser, JavaScript or GPU console errors in either suite.

## Full races and earned-credit purchases

`scripts/prototype-smoke.cjs` ran four complete three-lap combat races, with all
six racers finishing each race and zero player recoveries. It tested keyboard
driving, firing, shields, boost, pause/blur, restart and results, then spent the
actual prize through visible garage buttons and reloaded the page.

| Renderer | Player | Time | Position | Prize | Purchased with earned credits | Remaining credits |
| --- | --- | ---: | ---: | ---: | --- | ---: |
| WebGPU | Car | 100.383 s | 4 | 500 | Cannon level 1 | 200 |
| WebGPU | Hovercraft | 102.383 s | 2 | 900 | Cannon, shield and drive level 1 | 100 |
| WebGL2 | Car | 98.933 s | 2 | 900 | Cannon and shield level 1 | 300 |
| WebGL2 | Hovercraft | 102.500 s | 4 | 500 | Cannon and shield level 1 | 200 |

Each backend began with a fresh zero-credit career; the hover race retained the
car's remaining balance. Exact prize and purchase debits, completed-race counts,
separate vehicle ranks and selected vehicle survived reload. Reloading did not
replay a payout. Final standings sorted penalty-inclusive times.

Evidence: `build/evidence/progression-races/results.json`, screenshots,
`site-manifest.json` (all served file hashes), `source-manifest.json`, and
`native-tests.log`.

## All upgrade tiers and save failure paths

`scripts/progression-smoke.cjs` passed on both renderers. Explicit funded-save
fixtures cover boundary cases independently of the earned-money races above:

- All 18 upgrades bought at exact prices, with the other vehicle unchanged.
- Maximum-rank and insufficient-credit commands rejected in native game rules.
- Both vehicles driven with maximum upgrades, firing at a 0.5-second interval
  and activating/draining their 175-energy shields; HUD meters use capacity.
- Abandoning a race grants no payout; purchases are unavailable during races.
- Garage scrolling at 390 pixels keeps the start action visible; screenshots
  reviewed for readable upgrade levels, effects, prices and save status.
- A new browser context restores only persisted storage: credits, both builds,
  completed-race count and selected vehicle match. Switching renderer on the
  same origin preserves the career.
- Corrupt and unknown-version saves remain untouched, with visible session-only
  play status. Quota failure is also visibly unsaved and remains playable.
- A newer save from a second tab is preserved when the older tab changes selection;
  the older tab asks for reload instead of overwriting it.

Evidence: `build/evidence/progression-ui/results.json` and screenshots.

## Native simulation

`python scripts/dev.py native` passed in 63.77 seconds. It covers codec validation,
all prices/prizes, insufficient funds and maximum ranks, independent builds,
pending-result command guards, once-only payout, upgraded cooldown/recharge,
and stock plus fully upgraded complete physical races for both chassis.

An isolated 80-tick throttle comparison measured 19.59 → 38.12 km/h for the car
and 45.34 → 58.94 km/h for the hovercraft after buying all three drive levels.
These are regression measurements for this setup, not a general acceleration
or performance benchmark. Imported car geometry, wheel bindings and existing
physics/input regressions also passed.

## Scope

Browser full-race runs use stock player builds and then spend earned winnings;
the focused browser suite drives and fights with fully upgraded builds. Complete
fully upgraded races are additionally verified in the native simulation. No
test-only race completion, teleportation or credit injection is used in the
earned-money browser flow. Funded profiles are limited to the explicitly marked
boundary-case suite. This is desktop Chromium acceptance, not physical mobile,
touch/gamepad, cross-device/cloud saving or human handling approval.
