#!/usr/bin/env python3
"""Print or validate the intended v0.4 wheel target matrix."""

from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, os.fspath(ROOT))

from tools.datoviz_build_backend.tags import print_matrix  # noqa: E402
from tools.datoviz_build_backend.validate import validate_dist  # noqa: E402


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--include-prerelease", action="store_true")
    parser.add_argument("--dist-dir", type=Path, default=ROOT / "dist")
    parser.add_argument("--validate-dist", action="store_true")
    parser.add_argument("--version", default=None)
    parser.add_argument(
        "--platform-tag",
        action="append",
        default=[],
        help="restrict artifact validation to a platform tag such as manylinux_2_34_x86_64",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.validate_dist:
        validate_dist(args.dist_dir, version=args.version, platform_tags=args.platform_tag)
        return 0
    print_matrix(as_json=args.json)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
