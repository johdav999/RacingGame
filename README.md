# RacingGame

A browser racing game built with Johan's C++20/WebAssembly
[WebEngine](https://github.com/johdav999/WebEngine).

The initial project provides a small garage preview, native engine integration
test, portable build scripts, GitHub CI and repository-local asset skills.
Driving, tracks and race rules are the next stage.

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

- [Local/cloud setup and synchronization](docs/development.md)
- [Tripo, ElevenLabs and asset conventions](docs/assets.md)
- [Repository instructions](AGENTS.md)

Repository: [johdav999/RacingGame](https://github.com/johdav999/RacingGame).
For a fresh checkout: `git clone --recurse-submodules https://github.com/johdav999/RacingGame.git`.
