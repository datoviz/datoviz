#!/usr/bin/env python3
"""Build or run the RC3 Vulkan tutorial examples and validate offscreen captures."""

from __future__ import annotations

import argparse
import hashlib
import os
from pathlib import Path
import platform
import subprocess
import sys
import tempfile

from capture_gallery import png_is_nonblank


ROOT = Path(__file__).resolve().parents[1]
TUTORIAL_SOURCE = ROOT / "examples" / "c" / "tutorial"
CHAPTERS = ("first_triangle", "shaders_and_pipeline", "vertex_buffers")
EXECUTABLES = (*CHAPTERS, "indexed_depth_spike")


def _run(command: list[str], env: dict[str, str] | None = None) -> None:
    result = subprocess.run(
        command,
        cwd=ROOT,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    if result.returncode != 0:
        print(result.stdout, file=sys.stderr)
        raise RuntimeError(f"command failed with status {result.returncode}: {' '.join(command)}")
    if result.stdout:
        print(result.stdout, end="")


def _runtime_environment(prefix: Path | None, runtime_dirs: list[Path]) -> dict[str, str]:
    env = os.environ.copy()
    directories = [str(path) for path in runtime_dirs]
    if prefix is not None:
        directories = [str(prefix / "lib"), str(prefix / "lib64"), *directories]
    variable = "PATH"
    if platform.system() == "Darwin":
        variable = "DYLD_LIBRARY_PATH"
    elif platform.system() != "Windows":
        variable = "LD_LIBRARY_PATH"
    previous = env.get(variable)
    env[variable] = os.pathsep.join([*directories, *([previous] if previous else [])])
    return env


def _installed_executables(prefix: Path) -> tuple[Path, tempfile.TemporaryDirectory[str]]:
    temporary = tempfile.TemporaryDirectory(prefix="datoviz-vulkan-tutorial-")
    build_dir = Path(temporary.name)
    _run(
        [
            "cmake",
            "-S",
            str(TUTORIAL_SOURCE),
            "-B",
            str(build_dir),
            "-GNinja",
            f"-DCMAKE_PREFIX_PATH={prefix}",
        ]
    )
    _run(["cmake", "--build", str(build_dir)])
    return build_dir, temporary


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--executables-dir",
        type=Path,
        default=ROOT / "build" / "examples" / "c" / "tutorial",
    )
    parser.add_argument("--installed-prefix", type=Path)
    parser.add_argument("--runtime-dir", action="append", type=Path, default=[])
    args = parser.parse_args()

    temporary_build: tempfile.TemporaryDirectory[str] | None = None
    executables_dir = args.executables_dir
    if args.installed_prefix is not None:
        executables_dir, temporary_build = _installed_executables(args.installed_prefix.resolve())

    env = _runtime_environment(args.installed_prefix, args.runtime_dir)
    hashes: dict[str, str] = {}
    with tempfile.TemporaryDirectory(prefix="datoviz-vulkan-captures-") as capture_directory:
        captures = Path(capture_directory)
        for chapter in EXECUTABLES:
            executable = executables_dir / chapter
            if platform.system() == "Windows":
                executable = executable.with_suffix(".exe")
            if not executable.is_file():
                raise FileNotFoundError(executable)
            png = captures / f"{chapter}.png"
            _run(
                [
                    str(executable),
                    "--offscreen",
                    "--frames",
                    "3",
                    "--validate",
                    "--png",
                    str(png),
                ],
                env=env,
            )
            valid, detail = png_is_nonblank(png, (800, 600))
            if not valid:
                raise RuntimeError(f"{chapter}: invalid capture: {detail}")
            hashes[chapter] = hashlib.sha256(png.read_bytes()).hexdigest()

    if hashes["first_triangle"] == hashes["shaders_and_pipeline"]:
        raise RuntimeError("chapter two capture does not differ from chapter one")
    print("Vulkan tutorial examples smoke: OK")
    if temporary_build is not None:
        temporary_build.cleanup()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
