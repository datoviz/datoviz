#!/usr/bin/env python3
"""Watch and rebuild one C example target."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import shlex
import signal
import subprocess
import sys
import time


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_BUILD_DIR = ROOT / "build-dev"
WATCH_EXTENSIONS = {
    ".c",
    ".cc",
    ".cpp",
    ".h",
    ".hpp",
    ".cmake",
    ".glsl",
    ".vert",
    ".frag",
    ".comp",
    ".geom",
    ".tesc",
    ".tese",
}
WATCH_FILES = ("CMakeLists.txt", "justfile")
WATCH_DIRS = ("cmake", "examples/c", "include", "shaders", "src")


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Incrementally rebuild and restart one Datoviz C example."
    )
    parser.add_argument("name", help="Example name as group/name, for example visuals/mesh.")
    parser.add_argument("example_args", nargs=argparse.REMAINDER, help="Arguments for the example.")
    parser.add_argument(
        "--build-dir",
        default=os.environ.get("DVZ_EXAMPLE_BUILD_DIR", os.fspath(DEFAULT_BUILD_DIR)),
        help="CMake build directory. Defaults to build-dev or DVZ_EXAMPLE_BUILD_DIR.",
    )
    parser.add_argument(
        "--build-type",
        default=os.environ.get("DVZ_EXAMPLE_BUILD_TYPE", "Debug"),
        help="CMake build type for first configure. Defaults to Debug.",
    )
    parser.add_argument(
        "--poll",
        type=float,
        default=float(os.environ.get("DVZ_EXAMPLE_WATCH_POLL", "0.5")),
        help="Polling interval in seconds. Defaults to 0.5.",
    )
    return parser.parse_args()


def _example_name_parts(name: str) -> tuple[str, str]:
    parts = [part for part in name.split("/") if part]
    if len(parts) != 2:
        raise ValueError("example-watch expects a grouped example name such as visuals/mesh")
    return parts[0], parts[1]


def _target_name(name: str) -> str:
    group, example = _example_name_parts(name)
    return f"example_c_{group}_{example}"


def _example_exe(build_dir: Path, name: str) -> Path:
    group, example = _example_name_parts(name)
    suffix = ".exe" if os.name == "nt" else ""
    return build_dir / "examples" / "c" / group / f"{example}{suffix}"


def _configure_if_needed(build_dir: Path, build_type: str) -> None:
    if (build_dir / "build.ninja").exists():
        return

    build_dir.mkdir(parents=True, exist_ok=True)
    cmake_args = shlex.split(os.environ.get("DVZ_CMAKE_ARGS", ""))
    env = os.environ.copy()
    env.setdefault("CMAKE_CXX_COMPILER_LAUNCHER", "ccache")
    cmd = [
        "cmake",
        "-S",
        os.fspath(ROOT),
        "-B",
        os.fspath(build_dir),
        "-G",
        "Ninja",
        f"-DCMAKE_BUILD_TYPE={build_type}",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        *cmake_args,
    ]
    print("+ " + shlex.join(cmd), flush=True)
    subprocess.run(cmd, cwd=ROOT, env=env, check=True)


def _build(build_dir: Path, target: str) -> bool:
    cmd = ["cmake", "--build", os.fspath(build_dir), "--target", target, "-j"]
    print("+ " + shlex.join(cmd), flush=True)
    return subprocess.run(cmd, cwd=ROOT).returncode == 0


def _example_env() -> dict[str, str]:
    env = os.environ.copy()
    if sys.platform == "darwin":
        vulkan_sdk = env.get("VULKAN_SDK")
        if vulkan_sdk:
            vk_lib = Path(vulkan_sdk) / "lib"
            if vk_lib.is_dir():
                fallback = os.fspath(vk_lib)
                previous = env.get("DYLD_FALLBACK_LIBRARY_PATH")
                if previous:
                    fallback = f"{fallback}:{previous}"
                env["DYLD_FALLBACK_LIBRARY_PATH"] = fallback
    return env


def _run_example(exe: Path, args: list[str]) -> subprocess.Popen[bytes]:
    if not exe.exists():
        raise FileNotFoundError(f"{exe} was not produced by the target build")

    cmd = [os.fspath(exe), *args]
    print("+ " + shlex.join(cmd), flush=True)
    return subprocess.Popen(cmd, cwd=ROOT, env=_example_env())


def _terminate(process: subprocess.Popen[bytes] | None) -> None:
    if process is None or process.poll() is not None:
        return

    process.terminate()
    try:
        process.wait(timeout=2)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait()


def _watched_paths() -> list[Path]:
    paths: list[Path] = []
    for rel in WATCH_FILES:
        path = ROOT / rel
        if path.exists():
            paths.append(path)

    for rel in WATCH_DIRS:
        root = ROOT / rel
        if not root.exists():
            continue
        for path in root.rglob("*"):
            if path.is_file() and path.suffix in WATCH_EXTENSIONS:
                paths.append(path)
    return paths


def _snapshot() -> dict[Path, int]:
    return {path: path.stat().st_mtime_ns for path in _watched_paths()}


def _changed(previous: dict[Path, int]) -> tuple[bool, dict[Path, int]]:
    current = _snapshot()
    return current != previous, current


def main() -> int:
    args = _parse_args()
    try:
        target = _target_name(args.name)
    except ValueError as e:
        print(f"error: {e}", file=sys.stderr)
        return 2

    build_dir = Path(args.build_dir).resolve()
    exe = _example_exe(build_dir, args.name)

    try:
        _configure_if_needed(build_dir, args.build_type)
    except subprocess.CalledProcessError as e:
        return e.returncode

    process: subprocess.Popen[bytes] | None = None
    stopping = False

    def stop(_signum: int, _frame: object) -> None:
        nonlocal stopping
        stopping = True
        _terminate(process)

    signal.signal(signal.SIGINT, stop)
    signal.signal(signal.SIGTERM, stop)

    snapshot = _snapshot()
    if _build(build_dir, target):
        process = _run_example(exe, args.example_args)
    else:
        print("Initial build failed; waiting for changes.", file=sys.stderr, flush=True)

    while not stopping:
        time.sleep(args.poll)
        has_changed, snapshot = _changed(snapshot)
        if not has_changed:
            continue

        print("\nChange detected.", flush=True)
        _terminate(process)
        process = None
        if _build(build_dir, target):
            process = _run_example(exe, args.example_args)
        else:
            print("Build failed; waiting for changes.", file=sys.stderr, flush=True)

    _terminate(process)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
