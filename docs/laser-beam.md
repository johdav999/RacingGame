# Vehicle laser pulses

Hold F to fire the fitted cannons. Each shot alternates left/right apertures and
produces a bright white core with a soft cyan glow. Solid cover and vehicle hits
produce a contact flash and six short sparks. Shield hits produce a cyan contact
flash and the existing fitted shield ripple. A miss fades out at maximum range
without inventing a contact effect.

These are instantaneous laser pulses with a 0.14-second afterglow. Their shot
segments follow the firing car's translation and rotation for the full fade,
using the same interpolated pose as its rendered cannon. The initial muzzle ray
is retained in car-local space and transformed afresh each frame, preserving its
length and aim angle without accumulating drift. Contact afterglow follows that
segment too; damage and shield absorption still resolve only once at firing.
The existing 42-metre range, aim assistance, cooldown upgrades,
drive disruption, immunity and shield costs remain the combat rules. This is not
a new continuous-damage weapon or a projectile with travel time.

## Physics and rendering

Previously the amber box started at a gun, but its hit query started on the car
centreline. `StarterScene::weaponTarget` and `firePulse` now both query from the
next firing aperture, so muzzle-side obstacles stop both damage and the beam.
Jolt provides the first physical contact; a defending shield shortens the visible
segment to its fitted ellipsoid and retains its existing absorption rules.

`LaserBeam` is game-owned presentation using the selected engine's ordinary
mesh/material components. Every racer shares immutable capped core geometry,
spark geometry, a small generated feather texture and contact sphere geometry.
Each shot has at most nine draws: core, camera-facing glow, contact and six sparks.
There are no dynamic geometry uploads, provider calls, extra colliders or engine
changes. Effects use unlit additive materials with depth testing, no depth writes,
no shadows and no decals. The glow does not depend on bloom being enabled.

`CombatState::beamTime` owns the whole lifetime; rendering cannot advance it.
Pause freezes the shot, expiry disables it, and recovery/restart/chassis selection
clear both the retained segment and contact data. AI uses the same implementation.
`window.racingLaserVisual` reports the rendered segment for browser verification;
it cannot award hits or modify combat state.

## Verification

- `racing_laser` checks exact beam endpoints, shared geometry, pause/expiry,
  shield versus solid contact effects, misses, degenerate segments, both firing
  apertures, range and narrow muzzle-side cover using real Jolt queries on all
  four chassis, including live translation/rotation without repeated damage.
  Recovery and restart clear the retained shot.
- `python scripts/dev.py native` includes that suite plus existing weapon,
  shield and complete race/combat regressions.
- `scripts/laser-beam-smoke.cjs` runs real F-key firing, screenshot pixel checks,
  pause, release, held repeat, moving-pulse tracking, recovery and garage return for all four chassis
  serially on WebGPU and WebGL2. Set `NODE_PATH` to Playwright/pngjs and `DEMO_URL`
  to the generated game server. Evidence goes to `build/evidence/laser-beam/`.

Validated on 2026-10-21 against clean selected engine
`9b597da84d4582973d010fa9ba3e328c20181892`, Emscripten 6.0.11 and desktop
Chromium 155.0.8059.85. The nine existing native suites passed, including full
race/combat and chassis completion; the new laser suite passed its final focused
rerun after correcting its test setup to skip inactive vehicles. Logs are retained
as `native-regressions.log` and `native-laser-final.log` in the evidence directory.

All four chassis passed on both WebGPU and WebGL2 with zero recorded page/console
errors. Each captured shot changed 1,317–3,343 cyan pixels in the checked central
world region. Screenshots were also visually inspected on both renderers.
`report.json` retains the exact shot endpoints, hit types, checks and build identity.
The frozen `build/laser-site/racing.wasm` SHA-256 is
`2a8b0aad568e49f7c1be58b08e9e132dc57eeb7fe7c33b036016b2a2bfd3b733`.
This establishes desktop behaviour and rendering, not physical-mobile acceptance
or a production frame-rate budget.

### Car-following correction — 2026-10-22

The original world-fixed fade was replaced with the car-relative segment described
above. Focused Windows native `racing_laser` and `racing_shield` passed. The laser
test checks translated/rotated muzzle attachment, both rendered core endpoints,
unchanged range and no repeated combat hits on all four chassis.

The updated browser smoke passed all four chassis on both WebGPU and WebGL2 with
zero page/console errors. Consecutive frames of the same live pulse moved its
origin by 0.16–0.62 metres while driving; the old fixed segment would remain still.
Pause, expiry, held repeat, recovery and garage reset also passed. Moving-pulse
screenshots were inspected on both backends. Current evidence is in
`build/evidence/laser-beam/report.json` and `*-moving-laser.png`.
The generated `build/web/site/racing.wasm` SHA-256 is
`f1adba2b445043e3d258c8b6ccae35aa28832b60ea1029c4a0f7530a5808cb17`,
using the same clean selected engine pin and SDK noted above.

The additional native `racing_weapon` run failed during existing car asset import:
`Texture dimensions or decoded size exceed the limit: cybercar1_parts_normal`.
That asset-import failure precedes its weapon presentation checks; this change
does not modify the asset or importer. Native evidence here covers portable
Windows tests, not a native graphical game host.
