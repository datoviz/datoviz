#!/usr/bin/env python3
"""Stage the Datoviz Python wheel tree from an existing native build."""

from __future__ import annotations

import argparse
import os
import platform
import shutil
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_STAGE = ROOT / "build" / "wheel-stage"


def _copy_file(src: Path, dst: Path) -> None:
    if not src.exists():
        raise FileNotFoundError(src)
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)


def _copy_tree(src: Path, dst: Path) -> None:
    if not src.exists():
        raise FileNotFoundError(src)
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst, ignore=shutil.ignore_patterns("__pycache__", "*.pyc"))


def _first_existing(patterns: list[str], build_dir: Path) -> Path:
    for pattern in patterns:
        matches = sorted(build_dir.glob(pattern))
        if matches:
            return matches[0]
    raise FileNotFoundError(f"none of these build artifacts were found: {patterns}")


def _copy_matches(patterns: list[str], dst: Path, *, required: bool = False) -> list[Path]:
    copied: list[Path] = []
    for pattern in patterns:
        for src in sorted(ROOT.glob(pattern)):
            if src.is_file():
                _copy_file(src, dst / src.name)
                copied.append(dst / src.name)
    if required and not copied:
        raise FileNotFoundError(f"no files matched required patterns: {patterns}")
    return copied


def _stage_python(stage: Path) -> None:
    package_dir = stage / "datoviz"
    package_dir.mkdir(parents=True, exist_ok=True)
    for src in sorted((ROOT / "datoviz").glob("*.py")):
        _copy_file(src, package_dir / src.name)
    for subpackage in ("experimental",):
        src = ROOT / "datoviz" / subpackage
        if src.exists():
            _copy_tree(src, package_dir / subpackage)
    _copy_file(ROOT / "pyproject.toml", stage / "pyproject.toml")
    tools_dir = stage / "tools" / "release_wheels"
    tools_dir.mkdir(parents=True, exist_ok=True)
    _copy_file(ROOT / "tools" / "release_wheels" / "check_wheel.py", tools_dir / "check_wheel.py")


def _stage_c_integration(package_dir: Path) -> None:
    script = ROOT / "tools" / "copy_wheel_c_integration.sh"
    if not script.exists():
        raise FileNotFoundError(script)
    import subprocess

    subprocess.run([str(script), str(package_dir)], cwd=ROOT, check=True)


def _stage_native(build_dir: Path, package_dir: Path, include_qtbridge: bool) -> None:
    system = platform.system()
    if system == "Linux":
        lib = _first_existing(["libdatoviz.so", "**/libdatoviz.so"], build_dir)
        _copy_file(lib, package_dir / lib.name)
        _copy_matches(["libs/vulkan/linux/libvulkan.so.1"], package_dir)
        _copy_matches(["libs/shaderc/linux/*.so*"], package_dir)
        _copy_matches(["bin/vulkan/linux/glslc"], package_dir)
    elif system == "Darwin":
        lib = _first_existing(["libdatoviz.dylib", "**/libdatoviz.dylib"], build_dir)
        _copy_file(lib, package_dir / lib.name)
        for pattern in (
            "build/libvulkan*.dylib",
            "build/libshaderc*.dylib",
            "build/libfreetype*.dylib",
            "build/libpng*.dylib",
            "build/libMoltenVK.dylib",
            "build/MoltenVK_icd.json",
        ):
            _copy_matches([pattern], package_dir)
    elif system == "Windows":
        copied = _copy_matches(["build/*.dll"], package_dir)
        if not any(path.name.lower() in {"datoviz.dll", "libdatoviz.dll"} for path in copied):
            raise FileNotFoundError("no datoviz DLL was copied from build/*.dll")
        gcc = shutil.which("gcc")
        if gcc is not None:
            mingw = Path(gcc).resolve().parent
            for name in ("libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll"):
                src = mingw / name
                if src.exists():
                    _copy_file(src, package_dir / name)
    else:
        raise RuntimeError(f"unsupported platform: {system}")

    if include_qtbridge:
        suffixes = {
            "Linux": ["build/qtbridge/libdatoviz_qtbridge.so"],
            "Darwin": ["build/qtbridge/libdatoviz_qtbridge.dylib"],
            "Windows": ["build/qtbridge/*.dll"],
        }[system]
        copied = _copy_matches(suffixes, package_dir)
        if not copied:
            raise FileNotFoundError("Qt bridge requested but no datoviz_qtbridge library was found")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build")
    parser.add_argument("--stage-dir", type=Path, default=DEFAULT_STAGE)
    parser.add_argument("--clean", action="store_true", help="remove the stage directory first")
    parser.add_argument(
        "--include-qtbridge",
        action="store_true",
        help="include datoviz_qtbridge in the main wheel stage",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    stage = args.stage_dir.resolve()
    build_dir = args.build_dir.resolve()
    if args.clean and stage.exists():
        shutil.rmtree(stage)
    stage.mkdir(parents=True, exist_ok=True)
    package_dir = stage / "datoviz"

    _stage_python(stage)
    _stage_native(build_dir, package_dir, args.include_qtbridge)
    _stage_c_integration(package_dir)

    print(f"staged wheel tree: {os.fspath(stage)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
