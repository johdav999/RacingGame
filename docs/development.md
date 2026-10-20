# Local and cloud development

## Reproducible inputs

GitHub is the shared source of truth. WebEngine remains a separate repository,
pinned as a submodule at `external/WebEngine`. The initial pin is
`0426d9319de715584a2e34ff9862cd6ba873ecd0`. Emscripten is pinned in
`.emscripten-version`. Each browser output includes `build-info.json`.
Builds, SDKs, secret stores and provider working files are ignored and do not sync.

The adjacent WebEngine on this computer contains unpublished work, including
the Formula demo. That demo is not part of this initial game dependency.
Use `--engine ../WebEngine` only when intentionally developing against it.
Never claim that a successful development-engine build validates the published pin.

## Configure Codex cloud once

1. Connect GitHub in Codex and grant access to `johdav999/RacingGame` and WebEngine.
2. Create/select a cloud environment for RacingGame, starting from `main`.
3. Use a Linux universal environment with Python 3.10+, CMake 3.22+, Ninja, Git,
   and a C++20 compiler. If needed, install `cmake ninja-build g++` with apt.
4. Set the setup script to `bash scripts/cloud-setup.sh`.
5. Set the maintenance script to the same command. It refreshes the selected
   submodule and incrementally rebuilds caches when the task branch changes.
6. Run the environment setup and inspect its logs. This downloads the pinned SDK,
   fetches Jolt, runs offline helper tests/native integration and builds the site.
7. Start tasks on the branch containing the work you want. Ask Codex to follow
   `AGENTS.md` and create a PR for changes.

Setup networking needs GitHub/codeload and the Emscripten SDK/port download hosts
(including storage.googleapis.com). Build dependencies are warmed during setup.
If a task changes dependencies, permit the necessary network access or refresh
setup. Setup exports do not persist into the agent shell; `dev.py` resolves the
SDK explicitly on every browser build.

The cloud settings are account-side settings; committing this script does not
automatically register an environment. A successful Windows build does not prove
that a cloud environment has been provisioned.

For cloud browser preview: `python3 scripts/dev.py serve --host 0.0.0.0`,
then use the environment's forwarded port. Browser/GPU access depends on the host.
CI publishes a downloadable browser build artifact, not a hosted production site.

If WebEngine is private, access to RacingGame alone is insufficient. Provide a
read-only credential authorized for WebEngine through the host's supported secret
mechanism during submodule setup. GitHub Actions' default repository token cannot
read another private repository; configure a narrowly scoped GitHub App/token
and adapt checkout credentials before expecting CI to clone it. Never put tokens
in `.gitmodules`, Git remote URLs, shell arguments or committed files.

## Keep this computer and cloud tasks in sync

Before starting a task from main, with a clean working tree:

```text
git switch main
git pull --ff-only
git submodule update --init --recursive
git switch -c feature/your-task
```

Before moving work from this computer to cloud:

```text
git status
git add <the-specific-files-for-this-task>
git commit -m "Describe the change"
git push -u origin HEAD
```

Select that pushed branch for the cloud task. Uncommitted local files are not
visible in cloud. After cloud work, review and merge its PR, then repeat the
main pull/submodule commands locally. To continue an unmerged branch locally,
fetch and switch to that branch instead of pulling main.

Use separate branches/worktrees for simultaneous tasks. Do not have two agents
edit the same working directory or push competing work to the same branch.
If `--ff-only` fails, inspect divergence and merge/rebase deliberately; do not
reset away local work. Do not apply a cloud diff and also merge the same PR.
Commit architectural decisions and handoff notes; chats and personal skills
are not automatically shared through Git.

## Updating WebEngine

Implement and test reusable changes in its own repository, then commit and push
them there first. In RacingGame, select the published engine commit:

```text
git -C external/WebEngine fetch origin
git -C external/WebEngine checkout <published-engine-commit>
python scripts/dev.py native
python scripts/dev.py build
git add external/WebEngine
git commit -m "Update WebEngine pin"
git push
```

Verify both browser renderers when the engine change affects rendering/input.
Do not use `submodule update --remote` in normal setup: it silently changes the
chosen dependency instead of reproducing it.

## Skills and secrets

The repository includes full copies of the engine, Tripo, ElevenLabs and ImageGen
skills under `.agents/skills`. Prefer these paths for this project. They are
versioned snapshots, not symlinks to another computer. Updates must be reviewed,
tested and committed; the central Windows copies do not auto-sync.

Use `docs/assets.md` for provider configuration. Legacy Codex Cloud environment
secrets are available only in setup, then removed before the agent runs. Do not
save them into files to work around that restriction. The practical default is
to generate paid assets locally and commit reviewed runtime files; a cloud host
with an explicitly supported runtime secret integration may use the helpers'
environment-variable inputs.

Official references checked 2026-10-20:
[cloud environments](https://learn.chatgpt.com/docs/environments/cloud-environment)
and [repository skill discovery](https://learn.chatgpt.com/docs/build-skills).
