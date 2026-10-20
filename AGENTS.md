# RacingGame

Browser racing game using Johan's separate C++20/WebAssembly WebEngine.
This repository currently contains an engine integration starter, not a finished racing game.

## Start here

- Read `README.md` and `docs/development.md`. Inspect Git status before editing.
- The reproducible engine is the Git submodule `external/WebEngine`. Never assume
  that features in the adjacent development checkout have been published.
- Read `.agents/skills/engine-skill/SKILL.md` for engine work, then the selected
  engine's `architecture.md` and relevant headers. The project's selected engine
  takes precedence over the skill's historical machine-specific default path.
- Keep race rules, tracks, car tuning and UI in this repository. Keep reusable
  engine changes in WebEngine, publish those first, then update this gitlink.
- Do not modify/reset the adjacent engine checkout as part of routine game setup.

## Commands

- `python scripts/dev.py doctor`
- `python scripts/dev.py setup` (network required, installs pinned Emscripten)
- `python scripts/dev.py native` (builds and runs the integration smoke test)
- `python scripts/dev.py build` (browser build, outputs `build/web/site`)
- `python scripts/dev.py serve` (localhost:8080, serves only generated site)
- `python scripts/dev.py check-skills` (offline provider-helper tests)
- Linux cloud setup/maintenance: `bash scripts/cloud-setup.sh`.

Run relevant native checks for core changes. For renderer/input/game UI changes,
verify the exact generated site in both `?renderer=webgpu` and `?renderer=webgl2`.
Compilation is not browser or physical-device acceptance. Do not run engine demo
publishing targets; this consumer builds explicit targets into its own build tree.

## Asset skills and credentials

Read the repository copies of `tripo` and `elevenlabs` under `.agents/skills`.
Read `docs/assets.md` before generation. Helpers take an explicit `--repo`.
Use built-in ImageGen first for Tripo references; stop if it is unavailable.
No paid generation is authorized by repository setup or verification.
Never read, print, commit, upload or bundle credentials. `/.secrets/` is ignored.
Use helper `status` to check configuration. Windows DPAPI keys stay on this machine.
Cloud setup secrets are not automatically available to the agent phase; do not
persist them to bypass that boundary.

## Collaboration

GitHub is the shared source of truth. Commit and push before switching machines.
Use one branch per task and pull requests for concurrent work. Pull merged changes
with `--ff-only`, then update submodules. Never force-push or discard another task's
work. Keep handoff decisions in committed docs; chat history is not repository state.
