#!/usr/bin/env python3
"""Run the local v0.4 release preflight command sequence."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


PROFILES = {
    "rc1": [
        ["git", "diff", "--check"],
        ["just", "build"],
        ["just", "test"],
        ["just", "spec-check"],
        ["just", "ctypes"],
        ["just", "wheel-matrix"],
        ["just", "check-example-manifests"],
        ["just", "docs-api-check"],
    ],
    "docs": [
        ["git", "diff", "--check"],
        ["just", "gallery"],
        ["just", "check-example-manifests"],
        ["just", "docs-api-check"],
    ],
}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("profile", choices=sorted(PROFILES), nargs="?", default="rc1")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args(argv)

    for cmd in PROFILES[args.profile]:
        print("+ " + " ".join(cmd), flush=True)
        if args.dry_run:
            continue
        rc = subprocess.call(cmd, cwd=ROOT)
        if rc != 0:
            return rc
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
