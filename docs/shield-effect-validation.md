# Shield effect validation — 2026-10-21

Tested the generated RacingGame consumer with clean pinned WebEngine
`9b597da84d4582973d010fa9ba3e328c20181892`, Emscripten 6.0.11 and
Chrome 155.0.8059.85 on Windows. A frozen copy at `build/shield-site` was served
on localhost:8099 so concurrent development could not alter a running test.

## Results

- `python scripts/dev.py native`: the existing track, street, integration/race
  and alternate-chassis race suites passed (4/4, 179.66 seconds) after shield
  integration. The focused `racing_shield` test passed after the final ray
  refinement, including a real physics raycast into an AI's active shield.
- `python scripts/dev.py build`: passed against the selected submodule.
- `scripts/shield-effect-smoke.cjs`: all four chassis passed sequentially on
  WebGPU and WebGL2. Actual car-region pixels changed, with no browser/page
  console errors. Checks covered activation, pause, release fade, depletion
  collapse, held-input lockout, recharge/reactivation, movement, recovery and
  garage return. Both backends also rendered the effect at 390-by-844.
- Screenshots of active Vektor/Bastion shields, amber collapse and the narrow
  WebGL2 viewport were visually inspected. The field remains transparent enough
  to read the vehicle and track; rivals share the same effect.

Full lifecycle evidence: `build/evidence/shield-effect/results.json` and PNGs.
Final surface-hit refinement evidence: `build/evidence/shield-surface-hit/`.
The latter is a focused Vektor repeat on both backends; the off-centre ray and
actual AI block are covered by the final native test. `SHIELD_CHASSIS=0` selects
this subset; omit the variable to run the full four-chassis suite.

Final frozen bundle SHA-256:

```text
racing.wasm 56b9d20673d9ef23b50e6028a685372ed705e9f4858d5c52e4e80583f9a54f36
racing.js   9c12a956a6e8022e1cd30d56a098498db0362f7f95e314a78c5b0bc2dcc6208a
```

The shared checkout contained existing uncommitted work and concurrent changes
to the city/plaza. That work was preserved; no engine pin was advanced. The
checks above establish native logic and desktop-browser rendering, not physical
mobile acceptance, a sustained GPU/frame-time budget, multiplayer readiness or
production deployment. No audio asset was added.
