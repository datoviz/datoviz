#!/usr/bin/env python3
"""Build a wheel from a staged Datoviz wheel tree."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage-dir", type=Path, default=ROOT / "build" / "wheel-stage")
    parser.add_argument("--dist-dir", type=Path, default=ROOT / "dist")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    args.dist_dir.mkdir(parents=True, exist_ok=True)
    subprocess.run(
        [sys.executable, "-m", "pip", "wheel", str(args.stage_dir), "-w", str(args.dist_dir), "--no-deps"],
        check=True,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
