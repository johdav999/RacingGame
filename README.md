# RacingGame

A browser racing game built with Johan's C++20/WebAssembly
[WebEngine](https://github.com/johdav999/WebEngine).

The playable prototype is a six-racer cyberpunk city race: choose a wheeled
car or hovercraft and compete against five AI rivals over three laps through
Kairox. The garage offers Vektor, Bastion, Skimmer and Vector, with imported
models shared by player and rivals; see [vehicle integration](docs/vehicle-integration.md).
The garage also offers [Desert Solar / Helios Run](docs/desert-solar-track.md),
a 1.02 km circuit through orange dunes, solar mirror fields, a sandstone arch
and blue energy conduits. Select it in the garage or use `?track=desert`.
Two more [desert circuits](docs/desert-circuits.md) reuse that same art and daylight:
**Sirocco Solar / Dune Sweep** (`?track=sirocco`) and **Canyon Relay / Twin Mesa**
(`?track=canyon`). Both support all four chassis, three-lap races and separate
time-trial personal bests and ghosts.
The [Desert race kit](docs/desert-race-kit.md) adds 592 textured edge dividers and
12 physical checkpoint gantries made from two optimized Tripo models.
Two [dune villages](docs/dune-villages.md) add twelve buildings using five supplied
models, with graded sand foundations outside the opening and western straights.
A [solar operations house](docs/solar-operations.md) oversees the heliostat field,
with detailed sand, shallow drifts and textured sandstone rubble around the circuit.
Twelve textured trackside buildings use seven generated models, including a
market arcade, player garage, corner apartments and loading warehouse;
see [building integration](docs/building-integration.md).
The [transit and street kit](docs/street-kit-integration.md) adds a supported
station underpass, shopfronts, safety fences, spectator railings and depot props.
The [city ground and service areas](docs/city-ground.md) add raised paving, recessed drains, loading platforms, utility clusters, parked service vehicles, contact shading and damp light accents.
The [utility room and skyline](docs/skyline-expansion.md) replace the service
box and all 143 skyline placeholders with shared textured buildings.
It includes live positions, finishing results, boost, vehicle and barrier
collisions, ordered checkpoints, lap timing, recovery, pause and restart.
Both classes and AI rivals have pulse cannons and energy shields; see the
[combat controls and rules](docs/combat-prototype.md).
The [ElevenLabs audio mix](docs/racing-audio.md) adds four chassis engine loops,
positional rivals, tire/boost sounds, combat impacts and race cues. Use **SOUND**
for saved volume controls and mute; entering a race activates browser audio.
The [Tripo pulse cannon](docs/weapon-upgrade.md) adds fitted mounts, PBR materials,
visible upgrade ranks, alternating muzzle flashes and recoil to all four chassis.
Final standings award credits for separate car and hovercraft drive, cannon and
shield upgrades. Choose **BODY PAINT** in the garage for a live colour preview and saved paint per chassis. **ORIGINAL** restores the authored finish; see [vehicle paint](docs/vehicle-paint.md).
The garage saves progression locally between sessions; see
[winnings, upgrades and saving](docs/progression.md).
See [prototype controls, implementation and tests](docs/prototype.md).

Choose **League career** for Street, District, Metro and Elite leagues leading to
the **Master Championship**. Four-round leagues award 15/12/10/8/6/4 points and
qualify the top three for a promotion final. Master is decided over six rounds.
Standings, titles and the current season save with your credits and upgrades;
see [championship rules, progression and saving](docs/championship.md). The garage also offers **Continue League** for the next saved round and **My League Results** to review each league's current, latest and best standings.

Choose **RIVALS → Easy / Normal / Hard** in the garage for
[corner-aware braking and traffic-aware overtaking](docs/racing-ai.md).
Difficulty saves locally and is locked when a race starts.

Choose **Time trial / ghost** in the garage for a solo, stock-tuned lap with
three sector splits, personal bests per track/chassis, a cyan best-run ghost and
**Watch best run** playback. Clean bests save locally in the browser; see
[time-trial rules, storage and checks](docs/time-trial.md).

## Run locally

Prerequisites: Git, Python 3.10+, CMake 3.22+, Ninja, and a C++20 compiler for
native tests. On Windows use the installed MinGW toolchain on PATH (or configure
an appropriate Visual Studio developer terminal). Browser builds install their
own pinned Emscripten toolchain.

```powershell
python scripts/dev.py doctor
python scripts/dev.py setup
python scripts/dev.py native
python scripts/dev.py build
python scripts/dev.py serve
```

Open [localhost:8080](http://localhost:8080). The renderer links select WebGPU or
WebGL 2. Use localhost or HTTPS for WebGPU.

`setup` downloads Emscripten 6.0.11. To reuse the existing SDK on this computer:

```powershell
$env:EMSDK = 'C:\Users\Johan\source\repos\WebEngine\.tools\emsdk'
python scripts/dev.py build
```

Every build uses the pinned `external/WebEngine` by default. To explicitly test
the separate development engine:

```powershell
python scripts/dev.py build --engine ../WebEngine
python scripts/dev.py serve --development-engine
```

This uses a separate build directory. Uncommitted engine changes cannot run in
cloud tasks until published and selected by a new submodule commit.

## Codex and GitHub

- [Cyberpunk racing game design](docs/game-design.md)
- [Playable prototype and controls](docs/prototype.md)
- [Game architecture and implementation stages](docs/architecture.md)
- [Visual, vehicle, circuit and city design](docs/visual-design.md)
- [Local/cloud setup and synchronization](docs/development.md)
- [Tripo, ElevenLabs and asset conventions](docs/assets.md)
- [Generated barriers, gantries, lamps, signs and sidewalk kit](docs/track-kit-integration.md)
- [Generated market, service, skyline and rooftop kit](docs/city-detail-kit.md)
- [Measured rendering improvements and shared model LODs](docs/performance-diagnosis.md)
- [Performance presets, resolution scale and frame-time measurements](docs/performance-settings.md)
- [Paved market plazas, drainage and damp reflections](docs/market-plaza.md)
- [Repository instructions](AGENTS.md)

Repository: [johdav999/RacingGame](https://github.com/johdav999/RacingGame).
For a fresh checkout: `git clone --recurse-submodules https://github.com/johdav999/RacingGame.git`.
