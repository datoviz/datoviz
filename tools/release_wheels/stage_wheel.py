#!/usr/bin/env python3
"""Stage the Datoviz Python wheel tree from an existing native build."""

from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, os.fspath(ROOT))

from tools.datoviz_build_backend.config import DEFAULT_STAGE, parse_config_settings  # noqa: E402
from tools.datoviz_build_backend.native_payload import stage_payload  # noqa: E402


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
    config = parse_config_settings(
        {
            "datoviz.release-wheel": "true",
            "datoviz.native-build-dir": os.fspath(args.build_dir),
            "datoviz.stage-dir": os.fspath(args.stage_dir),
            "datoviz.include-qtbridge": str(args.include_qtbridge).lower(),
        }
    )
    stage_payload(config, clean=args.clean)
    print(f"staged wheel tree: {os.fspath(config.stage_dir)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
