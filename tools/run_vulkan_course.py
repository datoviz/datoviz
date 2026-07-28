#!/usr/bin/env python3
"""Run the Vulkan course step programs offscreen and validate their captures.

Every chapter's program must exit cleanly, report zero Vulkan validation errors, and produce a
capture of the expected size. Captures must also be reproducible: each program is run twice and the
two files must be identical, which is what makes chapter previews safe to regenerate. Chapters that
are expected to draw geometry are additionally checked for non-flat output.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import hashlib
import os
from pathlib import Path
import platform
import subprocess
import sys
import tempfile

from capture_gallery import png_extrema


ROOT = Path(__file__).resolve().parents[1]
SOURCES = ROOT / "examples" / "c" / "vulkan"
SIZE = (800, 600)


@dataclass(frozen=True)
class Step:
    name: str
    # Chapters 1-3 render a flat clear color on purpose; later chapters draw geometry.
    captures: bool = True
    expect_geometry: bool = False


STEPS = (
    Step("step01", captures=False),
    Step("step02"),
    Step("step03"),
)


def _run(command: list[str], env: dict[str, str] | None = None) -> str:
    result = subprocess.run(
        command, cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT
    )
    if result.returncode != 0:
        print(result.stdout, file=sys.stderr)
        raise RuntimeError(f"failed with status {result.returncode}: {' '.join(command)}")
    return result.stdout


def _runtime_environment(prefix: Path | None, runtime_dirs: list[Path]) -> dict[str, str]:
    env = os.environ.copy()
    directories = [str(path.resolve()) for path in runtime_dirs]
    if prefix is not None:
        directories = [str(prefix / "lib"), str(prefix / "lib64"), *directories]
    if not directories:
        return env
    variable = "PATH"
    if platform.system() == "Darwin":
        variable = "DYLD_FALLBACK_LIBRARY_PATH"
    elif platform.system() != "Windows":
        variable = "LD_LIBRARY_PATH"
    previous = env.get(variable)
    env[variable] = os.pathsep.join([*directories, *([previous] if previous else [])])
    return env


def _installed_build(prefix: Path, temporary: Path) -> Path:
    """Configure and build the step programs as a standalone consumer of an installed package."""
    project = temporary / "project"
    project.mkdir()
    (project / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.21)\n"
        "project(vulkan_course C)\n"
        "find_package(datoviz CONFIG REQUIRED)\n"
        + "".join(
            f"add_executable({step.name} {SOURCES / (step.name + '.c')})\n"
            f"target_link_libraries({step.name} PRIVATE datoviz::datoviz)\n"
            for step in STEPS
        )
    )
    build = temporary / "build"
    _run(["cmake", "-S", str(project), "-B", str(build), f"-DCMAKE_PREFIX_PATH={prefix}"])
    _run(["cmake", "--build", str(build)])
    return build


def _validate_capture(step: Step, png: Path) -> None:
    width, height, extrema = png_extrema(png)
    if (width, height) != SIZE:
        raise RuntimeError(f"{step.name}: expected {SIZE[0]}x{SIZE[1]}, got {width}x{height}")
    spread = max(high - low for low, high in extrema["color"])
    if step.expect_geometry and spread <= 2:
        raise RuntimeError(f"{step.name}: capture is flat, expected drawn geometry")
    if not step.expect_geometry and spread > 2:
        print(f"  note: {step.name} capture is not flat; update expect_geometry if intended")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--executables-dir", type=Path, default=ROOT / "build" / "examples" / "c" / "vulkan"
    )
    parser.add_argument("--installed-prefix", type=Path)
    parser.add_argument("--runtime-dir", action="append", type=Path, default=[])
    args = parser.parse_args()

    with tempfile.TemporaryDirectory(prefix="datoviz-vulkan-course-") as scratch:
        temporary = Path(scratch)
        executables = args.executables_dir
        prefix = None
        if args.installed_prefix is not None:
            prefix = args.installed_prefix.resolve()
            executables = _installed_build(prefix, temporary)
        env = _runtime_environment(prefix, args.runtime_dir)

        for step in STEPS:
            executable = executables / step.name
            if platform.system() == "Windows":
                executable = executable.with_suffix(".exe")
            if not executable.is_file():
                raise FileNotFoundError(executable)

            if not step.captures:
                _run([str(executable)], env=env)
                print(f"{step.name}: ran, no capture expected")
                continue

            digests = []
            for attempt in (1, 2):
                png = temporary / f"{step.name}-{attempt}.png"
                output = _run([str(executable), "--png", str(png)], env=env)
                if "validation errors: 0" not in output:
                    raise RuntimeError(f"{step.name}: Vulkan validation errors reported")
                _validate_capture(step, png)
                digests.append(hashlib.sha256(png.read_bytes()).hexdigest())
            if digests[0] != digests[1]:
                raise RuntimeError(f"{step.name}: capture is not reproducible across runs")
            print(f"{step.name}: capture valid and reproducible")

    print(f"Vulkan course smoke: {len(STEPS)} steps OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
