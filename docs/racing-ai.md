# Racing AI

Select **RIVALS → Easy / Normal / Hard** in the garage before starting a race.
Normal is the default. The choice is saved locally in a separate settings key,
survives track and chassis changes, and is locked during an event. Time trials
disable the selector and retain the preference for the next race. Storage failure
keeps the choice for the current session and displays that limitation.

## Driving decisions

The game-owned `src/racing_ai.hpp` planner measures curvature and segment length
from the same 192-point Circuit used by the road and checkpoints. It computes a
speed ceiling from lateral acceleration, then propagates braking constraints
backwards around the closed route, including its seam. Rivals consequently slow
before a corner and accelerate out of it; the profiles differ for Kairox and
Desert Solar. Tight corners and large heading errors add conservative limits.
Look-ahead uses metres and current speed, rather than a fixed point count.

| Difficulty | Free-road ceiling | Lateral acceleration budget | Planned braking | Following headway |
| --- | ---: | ---: | ---: | ---: |
| Easy | 63 km/h | 5.2 m/s² | 3.8 m/s² | 1.25 s |
| Normal | 75.6 km/h | 6.6 m/s² | 4.6 m/s² | 1.0 s |
| Hard | 90 km/h | 8.2 m/s² | 5.5 m/s² | 0.8 s |

These are planning limits, not guaranteed attained speeds. Traffic, corners,
physical grip, collisions and combat can reduce the actual pace. All difficulties
use the existing chassis, throttle, steering, service brakes and physics; there
are no difficulty-specific velocity writes, grip bonuses or progress advances.
Combat targeting, rewards and existing off-track/stall recovery rules are retained.

Traffic is compared in circuit distance, including across the start-line seam.
A rival considers the two passing sides only when approaching slower traffic on
a sufficiently gentle section. It checks current and predicted relative positions,
including faster cars behind, then holds a passing decision for two seconds.
Lane motion is rate-limited. Newly blocked merges are abandoned without switching
to the opposite side, and returning to the normal line requires clearance.
Boxed-in drivers reduce their target speed and brake. The clearance check permits
a stopped queue to pull out around a stopped leader instead of rejecting every
escape route. Vehicle collisions remain possible, especially during combat.

The planner is updated on simulation ticks. Pause freezes its timers. Grid reset,
chassis change and recovery clear transient decisions. Pure planning code emits
commands only; `StarterScene` owns all vehicle state and combat decisions.

## Integration and validation

- Native `racing_ai`: corner approach braking on both real circuits, ordered
  difficulty profiles, blocked lanes, fast approaching side traffic, physical
  brake commands, gradual committed passes, changing traffic and stopped queues.
- Native `racing_ai_races`: five real physics-driven opponents complete three
  ordered laps on both circuits at all three difficulties; verifies field recovery
  bounds, faster average finishes at higher difficulty and event-setting guards.
- Existing `racing_smoke` / `racing_desert`: regression races with the imported
  vehicle models, live combat, ordered checkpoints, finishing and progression.
- `scripts/racing-ai-smoke.cjs`: exact served Wasm hash, forced WebGPU/WebGL2,
  selector/save/reload, trial isolation, mid-race rejection, live braking/passing
  telemetry, checkpoint progress, pause/resume, track switches and narrow layout.

Browser evidence is written to `build/evidence/racing-ai/`. Runtime telemetry is
available as `window.racingAi`; its target speeds are km/h. The settings key is
`neon-ascension.difficulty.v1`. No engine changes or generated assets are required.

Validated 2026-10-22: all tests listed above passed, along with native audio and
time-trial rules regressions. Both browser backends reported no console/page
errors and matched the staged Wasm SHA256
`812ad4d53c5f43017aed0526d75217918c52311c1fd925bd48fd35e6908af21f`.
In the six-difficulty-race fixture, mean opponent finish times were:

| Track | Easy | Normal | Hard |
| --- | ---: | ---: | ---: |
| Kairox | 117.0 s | 104.4 s | 82.9 s |
| Desert Solar | 204.6 s | 163.0 s | 151.9 s |

That fixture keeps the player stationary and combat active; these are deterministic
test results, not a promise of lap times in every race. Existing recovery can still
occur after collisions. Separate regression races loaded the imported vehicle art.
