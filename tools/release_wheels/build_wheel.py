#!/usr/bin/env python3
"""Build a wheel from a staged Datoviz wheel tree."""

from __future__ import annotations

import argparse
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
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
