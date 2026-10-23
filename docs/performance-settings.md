# Performance settings

Open **Performance** in the top-right corner in the garage or during a race.
Opening it pauses an active race; close the panel and use Resume to continue.
Settings apply at the next ready renderer frame and persist separately from
career progression in `neon-ascension.performance.v1`. Invalid saved settings
fall back to Balanced. Unavailable browser storage leaves settings usable for
the current session.

| Preset | Default scale | Maximum pixel ratio | Pixel budget | Shadow maps | MSAA |
| --- | ---: | ---: | ---: | ---: | ---: |
| Low | 75% | 1 | 2,073,600 | 0 | Off |
| Balanced (default) | 100% | 1 | 3,686,400 | 1 | Off |
| High | 100% | 2 | 8,294,400 | 4 | 4x if supported |

Resolution scale is independently adjustable from 50% to 100%, in 5% steps.
Selecting a preset restores that preset's default scale. Scale multiplies the
device pixel ratio after its cap; engine dimension/device/pixel limits can
reduce the result further. The panel shows the actual canvas dimensions and
effective MSAA. HTML controls remain at the browser's native size. LOD stays
enabled in every preset. Physics, race rules, assets and progression are unchanged
by these controls.

The selected WebGL2 engine backend does not support shadow maps and reports that
limitation in the panel. Its resolution and MSAA controls still apply. Policy
lives in `src/performance_settings.hpp`; no engine changes or pin updates are
needed. Native tests exercise the same portable policy and resolution resolver.
This pinned engine has native core tests, not a Windows game renderer.

## Measurements

The top-left HUD shows FPS, average interval and p95. The panel also shows worst
interval, count of intervals over 33.34 ms, sample count/duration, and average
CPU callback duration. Samples come from completed game callbacks, not a second
independent animation loop. CPU callback duration includes simulation, rendering
submission and game UI publication; it does **not** measure GPU completion.

Measurements retain callbacks from the last ten seconds (at most 2,400 samples)
and refresh the displayed statistics twice per second. Long visible hitches are
retained. Loading, hidden tabs and renderer unavailability reset the baseline,
as do applied settings and viewport changes. Reset measurements starts a fresh
comparison. Paused rendering remains measurable; resume and drive to measure
race workload. Settings changes can cause a brief render-target rebuild.

For comparisons, use the same camera, viewport, backend and warmed scene, reset,
and sample at least ten seconds. These controls do not force Windows GPU
selection; see [the GPU/stutter diagnosis](stutter-diagnosis.md).

## Verification

- `node scripts/frame-times-test.cjs`: percentile calculation, visible long
  stalls, inactive/hidden reset and bounded history.
- `python scripts/dev.py native`: includes `racing_performance_settings`, testing
  all presets, resolution/pixel limits, MSAA capability fallback and rejected
  settings. The existing eleven race/asset tests also passed during this change.
- `node scripts/performance-settings-smoke.cjs`: actual generated site on forced
  WebGPU and WebGL2, all presets, 50% backing resolution, persistence/reload,
  malformed settings, race pause/resume, and 390px layout. Set `DEMO_URL` to the
  generated-site server and `NODE_PATH` to an installed Playwright package.

Browser report and screenshots: `build/evidence/performance-settings/`. The
report records engine/build identity, served Wasm SHA-256, effective settings and
frame-time samples. Warm 1280x800 WebGPU garage samples on the test machine were
about 21 ms across all presets; this is functional evidence, not a demonstrated
performance improvement or a full-race benchmark.
