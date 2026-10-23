# Circuit road materials

The Midnight Circuit uses dark aggregate asphalt and cast concrete shoulders on
both sides. The asphalt has fine grain and subtle discoloration; the lighter
concrete has pores and narrow expansion joints. Neon edges and lane/finish
markings remain above the road, and the existing flat collision surface and
vehicle handling are unchanged.

`src/track_surface.hpp` creates deterministic, seamless 512 by 512 albedo,
tangent normal and metallic/roughness textures. Materials are immutable and
shared across scene instances. Generation happens once per material, with no
network requests, external assets or per-frame pixel updates. The two material
sets use 6 MiB of source RGBA pixels before renderer mip chains/uploads.

Ribbon UVs follow cumulative circuit distance at approximately four metres per
tile. An integer repeat count closes the start/finish seam. Transverse spacing
uses metres and the mesh generates tangents after UV construction. Both concrete
shoulders share the same material and aligned longitudinal joints.

The selected WebEngine supports PBR normal/roughness maps on WebGPU. WebGL2 uses
its existing basic lighting and the same albedo texture, preserving the visible
grain, discoloration and concrete joints without requiring an engine change.

## Validation, 2026-10-21

The generated consumer site uses clean engine revision
`9b597da84d4582973d010fa9ba3e328c20181892`, Emscripten 6.0.11, with
`developmentEngine: false`. This is the existing selected submodule checkout;
the task does not change the engine or its selection.

- `python scripts/dev.py native`: integration suite passed (64.32 seconds).
- `python scripts/dev.py build`: Release consumer build passed.
- `scripts/prototype-smoke.cjs` with `SKIP_FULL_RACE=1`: WebGPU and WebGL2
  passed serially in Chromium 155.0.8059.85 at 1440 by 900, with a 390 by 844
  layout check. Car/hover driving, steering, brakes, boost, combat, pause,
  recovery, restart and vehicle switching passed with no console/page errors.
  Full browser races were intentionally skipped for this surface-only change.
- Inspected both renderers' driving screenshots: visible asphalt grain and
  mottling, lighter shoulders, and readable neon/lane markings through corners.
- Browser report and screenshots: `build/evidence/track-surface/`.
- Wasm SHA256: `ea59edaab2949c4c192013d7bc3a54888029c4968c69f6b47e08f7f7ac78b171`.

Browser validation covers desktop Chromium on the exact generated site. It is
not physical mobile or sustained performance acceptance.
