#!/usr/bin/env python3
"""Build a Datoviz wheel from a staged release wheel tree."""

from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, os.fspath(ROOT))

from tools.datoviz_build_backend.wheel import build_from_stage  # noqa: E402


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage-dir", type=Path, default=ROOT / "build" / "wheel-stage")
    parser.add_argument("--dist-dir", type=Path, default=ROOT / "dist")
    parser.add_argument(
        "--platform-tag",
        help="build a platform wheel, for example manylinux_2_34_x86_64",
    )
    parser.add_argument(
        "--skip-repair",
        action="store_true",
        help="skip platform repair; intended for local tests only",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    wheel = build_from_stage(
        args.stage_dir.resolve(),
        args.dist_dir.resolve(),
        args.platform_tag,
        root=ROOT,
        skip_repair=args.skip_repair,
    )
    print(wheel)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
