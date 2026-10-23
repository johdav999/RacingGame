# Pulse combat prototype

Both the Vektor car and Wisp hovercraft carry a pulse cannon and energy shield.
The five AI rivals use the same weapon, defense and disruption rules.

| Control | Behavior |
| --- | --- |
| Hold F | Fire a pulse every 0.8 seconds |
| Hold E | Activate a shield while energy lasts |
| Release E | Recharge; release is required after depletion |

The HUD shows the visible forward target, weapon cooldown, shield energy,
drive disruption, hits and blocks. White-core cyan [laser pulses](laser-beam.md) show shots; a fitted cyan
[energy field](shield-effect.md) surrounds each shielded chassis, with impact
ripples and an amber depletion collapse. Cannon mounts
remain attached when the imported player-car model replaces its placeholder.
Results include the player's shot, hit and block totals.

## Rules

- Pulse range is 42 metres, with about 20 degrees of forward aim assistance.
  Target acquisition and the shot use Jolt raycasts from the alternating cannon muzzle. The shooter is excluded;
  solid barriers and intervening vehicles block the shot. A shot without an
  acquired target travels straight forward and still checks physical cover.
- An unshielded hit disrupts drive for 0.65 seconds: throttle becomes 35%,
  boost is suppressed, and drag slows the vehicle. Steering remains available.
  Hits do not remove laps, destroy cars or cause forced recovery.
- A successful disruptive hit grants 1.65 seconds of hit protection. Repeated
  shots during protection cannot extend the disruption. Protected hits do not
  count as successful hits.
- Shield capacity is 100. Holding it drains 32 units per second; each blocked
  pulse costs another 24. Recharge is 18 units per second while inactive, with
  at least 25 needed to activate. Depletion requires releasing E before reuse.
  A partially charged active shield can absorb the hit that depletes it.
- Boost and shielding have separate energy meters. Firing and shielding can
  be used together. All defenses update before attacks resolve on each fixed
  tick, avoiding a shield-activation advantage from racer update order.
- AI fires at visible rivals ahead and raises shields when another racer is
  close behind and facing it. It uses the same range, cooldown, energy and hit
  rules. These are basic circuit-combat decisions, not a general combat planner.
- Countdown and garage disable combat. Finishers cannot fire or take hits.
  Pause freezes the simulation; blur cancels held commands. Restart resets all
  combat counters and energy. Recovery clears disruption and gives brief hit
  protection, while preserving energy, counters and the usual race penalty.

## Source and verification

`src/combat_rules.hpp` owns portable timers, energy, counters and hit rules.
`src/starter_scene.cpp` owns physics queries, AI decisions, force effects and
the fixed pools of laser/shield visuals. `src/laser_beam.cpp` owns shared beam geometry,
the feathered glow and bounded contact effects. The browser only sends actions and
presents telemetry; it cannot award hits or alter combat resources.

The native suite covers cooldowns, shield absorption/depletion, the release
requirement, hit protection, physical cover, range and forward targeting for
both classes. Full physical races verify AI shots, damaging hits and blocks,
complete classification, post-finish protection and whole-field combat reset.

`scripts/prototype-smoke.cjs` exercises keyboard F/E, combat pause/reset and
complete combat races for each player class on WebGPU and WebGL2. Browser
evidence and build identity are recorded in `docs/combat-validation.md`.

The loadout remains a pulse cannon and energy shield. [Garage upgrades](progression.md)
now improve their cooldown, capacity and recharge using race winnings. Additional
weapon types, hull destruction and combat audio remain unimplemented.
