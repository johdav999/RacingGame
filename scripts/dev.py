#!/usr/bin/env python3
"""Portable local/cloud tooling. Python 3.10+, standard library only."""
import argparse
import functools
import http.server
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
VERSION = (ROOT / ".emscripten-version").read_text().strip()


def run(args, env=None):
    print("+", " ".join(map(str, args)), flush=True)
    subprocess.run(list(map(str, args)), cwd=ROOT, env=env, check=True)


def engine_path(explicit=None):
    return Path(explicit).resolve() if explicit else ROOT / "external/WebEngine"


def sdk_path():
    return Path(os.environ.get("EMSDK", ROOT / ".tools/emsdk")).resolve()


def sdk_environment():
    sdk = sdk_path()
    if not (sdk / ".emscripten").is_file():
        raise RuntimeError("SDK missing. Run python scripts/dev.py setup, or set EMSDK.")
    env = os.environ.copy()
    env["EMSDK"] = str(sdk)
    env["EM_CONFIG"] = str(sdk / ".emscripten")
    env["PATH"] = os.pathsep.join([str(sdk / "upstream/emscripten"), str(sdk / "upstream/bin"), env["PATH"]])
    compiler = sdk / "upstream/emscripten/emcc.py"
    actual = subprocess.check_output([sys.executable, str(compiler), "--version"], env=env, text=True)
    if VERSION not in actual.splitlines()[0].split():
        raise RuntimeError(f"Emscripten {VERSION} required; run setup to activate it.")
    return sdk, env


def setup():
    run(["git", "submodule", "sync", "--recursive"])
    run(["git", "submodule", "update", "--init", "--recursive"])
    sdk = sdk_path()
    if not sdk.exists():
        sdk.parent.mkdir(parents=True, exist_ok=True)
        run(["git", "clone", "--depth", "1", "https://github.com/emscripten-core/emsdk.git", sdk])
    if not (sdk / "emsdk.py").is_file():
        raise RuntimeError(f"Incomplete SDK directory: {sdk}")
    run([sys.executable, sdk / "emsdk.py", "install", VERSION])
    run([sys.executable, sdk / "emsdk.py", "activate", VERSION])


def build(args, native=False):
    engine = engine_path(args.engine)
    if not (engine / "CMakeLists.txt").is_file():
        raise RuntimeError("WebEngine missing. Run setup first.")
    # Never mix caches compiled against the shared development checkout with the gitlink.
    suffix = "-dev-engine" if args.engine else ""
    output = ROOT / "build" / (("native" if native else "web") + suffix)
    configure = ["cmake", "-S", ROOT, "-B", output, "-G", "Ninja",
                 "-DCMAKE_BUILD_TYPE=Release", f"-DWEBENGINE_SOURCE_DIR={engine}"]
    env = os.environ.copy()
    if native and os.name == "nt" and shutil.which("g++"):
        # Match MinGW runtime DLLs to the compiler, ahead of other programs' DLLs.
        compiler_bin = str(Path(shutil.which("g++")).parent)
        env["PATH"] = compiler_bin + os.pathsep + env["PATH"]
    if not native:
        sdk, env = sdk_environment()
        configure = [sys.executable, sdk / "upstream/emscripten/emcmake.py", *configure]
    run(configure, env)
    run(["cmake", "--build", output, "--target", "racing_smoke" if native else "racing_site",
         "--parallel", str(args.jobs)], env)
    if native:
        run(["ctest", "--test-dir", output, "--output-on-failure", "--timeout", "120",
             "-R", "^racing_(smoke|chassis_races|street_kit|track_kit|market_plaza|shield|gate_marker|weapon|laser|static_art|desert|desert_lighting|desert_circuits|desert_trials|desert_race_kit|dune_villages|performance_settings|paint|audio|trial|trial_rules|ai|ai_races|championship)$"], env)
    else:
        revision = subprocess.check_output(["git", "-C", str(engine), "rev-parse", "HEAD"], text=True).strip()
        dirty = bool(subprocess.check_output(["git", "-C", str(engine), "status", "--porcelain"], text=True).strip())
        (output / "site/build-info.json").write_text(json.dumps({
            "engineCommit": revision, "engineDirty": dirty, "developmentEngine": bool(args.engine),
            "emscripten": VERSION,
        }, indent=2) + "\n")
        print(f"Browser site: {output / 'site'}")


def check_skills():
    for skill in ("tripo", "elevenlabs"):
        run([sys.executable, "-m", "unittest", "discover", "-s",
             ROOT / ".agents/skills" / skill / "scripts", "-p", "test_*.py"])
        run([sys.executable, ROOT / ".agents/skills" / skill / "scripts" / f"{skill}.py",
             "status", "--repo", ROOT])


def doctor():
    required = ("git", "cmake", "ninja")
    missing = []
    for tool in required:
        present = shutil.which(tool)
        print(f"{tool}: {present or 'MISSING'}")
        if not present:
            missing.append(tool)
    print(f"Python: {sys.version.split()[0]}")
    print(f"WebEngine: {'present' if (engine_path() / 'CMakeLists.txt').exists() else 'run setup'}")
    print(f"Emscripten: {sdk_path()} (required {VERSION})")
    if missing:
        raise RuntimeError("Install missing tools: " + ", ".join(missing))


def serve(args):
    site = ROOT / "build" / ("web-dev-engine" if args.development_engine else "web") / "site"
    if not (site / "racing.wasm").is_file():
        raise RuntimeError("Browser build missing. Run build first.")
    # Never expose the repository root or .secrets over HTTP.
    handler = functools.partial(http.server.SimpleHTTPRequestHandler, directory=str(site))
    server = http.server.ThreadingHTTPServer((args.host, args.port), handler)
    print(f"Serving {site} at http://{args.host}:{args.port}", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    for name in ("setup", "doctor", "check-skills"):
        commands.add_parser(name)
    for name in ("build", "native"):
        child = commands.add_parser(name)
        child.add_argument("--engine", help="Explicit development checkout; uses a separate build directory")
        child.add_argument("--jobs", type=int, default=min(4, os.cpu_count() or 2))
    child = commands.add_parser("serve")
    child.add_argument("--host", default="127.0.0.1", help="Use 0.0.0.0 only for a cloud preview")
    child.add_argument("--port", type=int, default=8080)
    child.add_argument("--development-engine", action="store_true")
    args = parser.parse_args()
    try:
        if args.command == "setup":
            setup()
        elif args.command == "doctor":
            doctor()
        elif args.command == "check-skills":
            check_skills()
        elif args.command == "serve":
            serve(args)
        else:
            build(args, native=args.command == "native")
    except (RuntimeError, subprocess.CalledProcessError, OSError) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
