#!/usr/bin/env python3
"""Build a wheel from a staged Datoviz wheel tree."""

from __future__ import annotations

import argparse
import platform
import shutil
import subprocess
import sys
import sysconfig
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage-dir", type=Path, default=ROOT / "build" / "wheel-stage")
    parser.add_argument("--dist-dir", type=Path, default=ROOT / "dist")
    parser.add_argument(
        "--platform-tag",
        help="retag the pure wheel as a platform wheel, for example manylinux_2_34_x86_64",
    )
    return parser.parse_args()


def _default_platform_tag() -> str:
    return sysconfig.get_platform().replace("-", "_").replace(".", "_")


def _retag(dist_dir: Path, platform_tag: str | None) -> None:
    tag = platform_tag or _default_platform_tag()
    wheels = sorted(dist_dir.glob("datoviz-*-py3-none-any.whl"))
    if not wheels:
        return
    if len(wheels) > 1:
        raise RuntimeError(f"multiple pure wheels found for retagging: {wheels}")
    subprocess.run(
        [sys.executable, "-m", "wheel", "tags", "--platform-tag", tag, str(wheels[0])],
        check=True,
    )
    wheels[0].unlink()


def _single_wheel(dist_dir: Path) -> Path:
    wheels = sorted(dist_dir.glob("datoviz-*.whl"))
    if len(wheels) != 1:
        raise RuntimeError(f"expected one wheel in {dist_dir}, found: {wheels}")
    return wheels[0]


def _repair_macos(dist_dir: Path) -> None:
    if platform.system() != "Darwin":
        return
    delocate = shutil.which("delocate-wheel")
    if delocate is None:
        raise RuntimeError("delocate-wheel is required to repair macOS wheels")
    wheel = _single_wheel(dist_dir)
    repaired = dist_dir / ".repaired"
    if repaired.exists():
        shutil.rmtree(repaired)
    repaired.mkdir(parents=True)
    subprocess.run([delocate, "-w", str(repaired), str(wheel)], check=True)
    repaired_wheel = _single_wheel(repaired)
    wheel.unlink()
    repaired_wheel.replace(dist_dir / repaired_wheel.name)
    shutil.rmtree(repaired)


def main() -> int:
    args = parse_args()
    args.dist_dir.mkdir(parents=True, exist_ok=True)
    for wheel in args.dist_dir.glob("datoviz-*.whl"):
        wheel.unlink()
    subprocess.run(
        [sys.executable, "-m", "pip", "wheel", str(args.stage_dir), "-w", str(args.dist_dir), "--no-deps"],
        check=True,
    )
    _retag(args.dist_dir, args.platform_tag)
    _repair_macos(args.dist_dir)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
