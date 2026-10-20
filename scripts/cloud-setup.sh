#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
# Use a Codex universal/Ubuntu environment with Python 3.10+, Git, CMake 3.22+,
# Ninja and a C++20 compiler. No provider key is required for any setup step.
python3 scripts/dev.py doctor
python3 scripts/dev.py setup
python3 scripts/dev.py check-skills
python3 scripts/dev.py native
# Warm the Wasm compiler/port/dependency caches while setup networking is available.
python3 scripts/dev.py build
