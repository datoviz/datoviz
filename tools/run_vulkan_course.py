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


def _installed_build(temporary: Path, discovery: list[str]) -> Path:
    """Configure and build the step programs as a standalone consumer of an installed package.

    `discovery` holds the CMake arguments that point at the package, mirroring what the course's
    chapter 1 tells the reader to pass.
    """
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
    _run(["cmake", "-S", str(project), "-B", str(build), *discovery])
    _run(["cmake", "--build", str(build)])
    return build


def _wheel_install(spec: str, temporary: Path) -> tuple[Path, list[str]]:
    """Install a published datoviz wheel into a throwaway venv.

    Returns the package directory holding the runtime payload, and the CMake discovery arguments
    from `datoviz-config --cmake-dir` — the exact command the course documents.
    """
    venv = temporary / "venv"
    _run([sys.executable, "-m", "venv", str(venv)])
    python = venv / ("Scripts/python.exe" if platform.system() == "Windows" else "bin/python")
    # PIP_USER is set in some developer environments and is incompatible with a venv install.
    env = os.environ.copy()
    env["PIP_USER"] = "false"
    _run([str(python), "-m", "pip", "install", "--quiet", "--upgrade", "pip"], env=env)
    _run([str(python), "-m", "pip", "install", "--quiet", "--pre", spec], env=env)

    config = venv / ("Scripts/datoviz-config" if platform.system() == "Windows" else "bin/datoviz-config")
    if not config.is_file():
        raise RuntimeError(f"the installed wheel provides no datoviz-config: {config}")
    cmake_dir = _run([str(config), "--cmake-dir"], env=env).strip()
    if not cmake_dir:
        raise RuntimeError("datoviz-config --cmake-dir returned nothing")
    # <package>/lib/cmake/datoviz -> <package>
    package_dir = Path(cmake_dir).resolve().parents[2]
    print(f"installed {spec} into {venv}")
    return package_dir, [f"-Ddatoviz_DIR={cmake_dir}"]


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
    parser.add_argument(
        "--wheel",
        metavar="SPEC",
        help="pip-install this datoviz spec (e.g. datoviz==0.4.0) into a throwaway venv and build "
        "the steps against it, exactly as the course's chapter 1 instructs",
    )
    parser.add_argument("--runtime-dir", action="append", type=Path, default=[])
    args = parser.parse_args()
    if args.installed_prefix is not None and args.wheel is not None:
        parser.error("--installed-prefix and --wheel are mutually exclusive")

    with tempfile.TemporaryDirectory(prefix="datoviz-vulkan-course-") as scratch:
        temporary = Path(scratch)
        executables = args.executables_dir
        prefix = None
        runtime_dirs = list(args.runtime_dir)
        if args.wheel is not None:
            package_dir, discovery = _wheel_install(args.wheel, temporary)
            # A wheel carries its library and Vulkan loader in the package directory itself.
            runtime_dirs.append(package_dir)
            executables = _installed_build(temporary, discovery)
        elif args.installed_prefix is not None:
            prefix = args.installed_prefix.resolve()
            executables = _installed_build(temporary, [f"-DCMAKE_PREFIX_PATH={prefix}"])
        env = _runtime_environment(prefix, runtime_dirs)

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
