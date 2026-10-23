# GPU selection and frame-time stutter — 2026-10-21

The reported 10–20 FPS stutter was reproduced using the frozen laser build on
port 8111 (`build/laser-site`), at 2223 x 1244, DPR1. The in-app preview also
displayed 11 FPS with the race paused. Rendering remained expensive when driving
and combat simulation stopped.

The machine has both Intel UHD Graphics 770 and NVIDIA RTX 5060 Ti (16 GB).
An instrumented Chrome run captured the **game's actual** `requestAdapter`
call, including `powerPreference: high-performance`, and its returned adapter:
Intel `gen-12lp`. This is stronger evidence than making a separate default
adapter request, which need not select the same device as the game.

The unchanged game in a temporary Chrome process launched with
`--force_high_performance_gpu` selected NVIDIA `blackwell` instead. No Windows
preferences, engine sources, assets, resolution or game quality were changed.

| Measurement | Default Chrome / Intel | Forced high-performance Chrome / NVIDIA |
| --- | ---: | ---: |
| Warm fixed garage view FPS | 12.74; repeat 16.10 | 47.33 |
| Frame-time p95 | 200 ms; repeat 180 ms | 20.2 ms |
| GPU time, rolling mean | 68.59 ms; repeat 54.62 ms | 1.34 ms |

The NVIDIA measurement was a 15-second warmed fixed-view sample, not a complete
race or an in-app-browser retest after changing Windows settings. Browser was
Chrome 155.0.8059.85. Same generated Wasm SHA-256:
`2a8b0aad568e49f7c1be58b08e9e132dc57eeb7fe7c33b036016b2a2bfd3b733`.

## Fix on this computer

In Windows Settings > System > Display > Graphics, select the application
hosting the game (ChatGPT for the in-app preview; Chrome for a standalone game),
set its GPU preference to **High performance / NVIDIA GeForce RTX 5060 Ti**, save,
and fully restart that application. Reloading only the game tab may retain the
existing browser GPU process. See [Microsoft's per-app graphics instructions](https://support.microsoft.com/en-us/windows/hardware/display-graphics/optimizations-for-windowed-games-in-windows-11).

The game already asks WebGPU for high-performance hardware. A web application
cannot force the host's Windows GPU preference; the preference must be corrected
at the browser/application level. Verify the actual returned adapter after a
restart. Do not infer it from the WebGPU backend label alone.

## Remaining engine optimization

The selected engine's `shaders/mesh.wgsl` evaluates BRDF/shadow terms for all six
nearby streetlights even where their distance attenuation is exactly zero.
An isolated diagnostic shader short-circuit increased the fixed-view Intel
measurement to 26.25 FPS, versus the 12.74/16.10 FPS bracketing baselines. It was
not applied to production, published, or accepted through a full renderer test.
This is a useful follow-up in shared WebEngine, especially for integrated-GPU
devices; publish and validate there before changing the game's submodule pin.

The initial paused Intel sample submitted 347 draws, 1.30M triangles and three
passes, with 206 LOD parts, no shadow pass and no new asset uploads. Renderer CPU
time was about 8 ms, GPU work about 63 ms. Suppressing GPU draws diagnostically
restored 49.94 FPS. Quarter pixel count reached 17.07 FPS; resolution reduction
alone is not the preferred fix when this machine's RTX is available.

Windows utilization samples also found a separate ChatGPT process consuming
about 33% of the same 3D engine while the diagnostic Chrome process consumed
63–65%. NVIDIA was almost idle at that point. Background application GPU load
can compound the issue; absolute results should not be extrapolated to a fully
idle machine or other hardware.

Evidence: `build/evidence/stutter-diagnosis/{profile,actual-adapter,nvidia-adapter}.json`
and `build/evidence/stutter-light-probe/results.json`. Diagnostic-only harnesses
are retained under `artifacts/profile-{stutter,actual-adapter,nvidia-adapter,zero-light}.cjs`.
