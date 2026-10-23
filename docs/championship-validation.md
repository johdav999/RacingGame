# Championship validation — 2026-10-23

The five-league career is implemented in RacingGame. No WebEngine API or pin was
changed by this task. The selected engine is `9b597da84d4582973d010fa9ba3e328c20181892` and the
browser build uses repository-local Emscripten 6.0.11.
Native checks and WebGL2 acceptance were completed on October 22; the final
WebGPU and focused UI runs were completed on October 23.

## Evidence

| Target | Result | What was checked |
| --- | --- | --- |
| Native Windows / MinGW | 20 selected regressions passed, zero failed | Existing gameplay, saves, AI races, time trials, assets and championship |
| Focused native championship | Passed | Scoring, ties, complete ladder, failed seasons/finals, migration, invalid saves, stale/duplicate results and scene travel |
| Native physical championship | Five races passed | Street and Master equipment on both tracks, plus a physically won promotion final; exact final classifications commit once with credits |
| Chrome WebGPU | Passed, no console/page errors | Real three-lap league race, per-driver points, once-only payout, standings, reload, next-track loading and later-career fixtures |
| Chrome WebGL2 | Passed, no console/page errors | Same acceptance flow against the identical frozen JavaScript/Wasm pair |
| Focused browser UI | Passed on WebGPU and WebGL2 | Real W-key acceleration, immediate canvas focus, exact per-driver points presentation, league capacity telemetry and quick-race preference restoration across reload |
| Responsive layout | Passed | 1440×900 and 390×844 Chromium layout; career panel remains within viewport |

Both browser races finished first, earned 15 championship points and changed the
controlled starting wallet from 1,000 to 2,200 credits. All six finishing positions
were displayed with their points. Later-career profiles explicitly cover final
qualification, lost-final retry eligibility, promotion, failed seasons, Master
title restoration and starting the final Master round. Corrupt championship data
retains the original save and visibly blocks durable writes; storage failure
prevents track travel from discarding the current session.

Native physical Master cases verify the upgraded field completes both circuits;
the conservative test driver finished sixth. This is functional coverage, not
evidence of full-career difficulty balance or a naturally earned Master title.

Generated browser output changed during concurrent checkout work, so final browser
acceptance used an immutable copy in `build/championship-validation-site`, served
at `http://127.0.0.1:8137`. The harness checks both served artifacts:

- Wasm SHA256: `3f9a4a53319e159b3aab253ecf3ee0e6e82e63dd96da22e4512d493a62abbd7a`
- JavaScript glue SHA256: `e0b0f3265c942fcb71dd00303c41e386e7df593a9d373184db702ffbc9d6d9fb`

The snapshot's championship UI, integration script, difficulty adapter and HTML
hashes match the delivered source files. The main mutable build may continue to
change with other work; these results belong to the frozen artifact above.

## Reproduce

Build with `python scripts/dev.py native` and `python scripts/dev.py build` using
the repository-local SDK. Start the generated-site server. Set `DEMO_URL` to its
origin and make the installed Playwright package available to Node, then run:

```powershell
node scripts/championship-smoke.cjs
node scripts/championship-ui-smoke.cjs
```

The acceptance harness can isolate each backend with `BACKENDS=webgpu` or
`BACKENDS=webgl2`; `SITE_DIR` selects the expected generated output, and
`EVIDENCE_DIR` selects its evidence folder. Run browser suites serially and serve
a frozen copy when other tasks may rebuild the main output.

Machine-readable results are in `build/evidence/championship/validation-summary.json`,
`frozen-webgpu/results.json` and `frozen-webgl2/results.json`, with screenshots
beside each browser report. Physical mobile devices, sustained performance and a
rendered native RacingGame host are outside this validation. The selected consumer
currently provides a browser game and native core/gameplay tests.
