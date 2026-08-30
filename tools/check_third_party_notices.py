#!/usr/bin/env python3
"""Validate the third-party license payload required by release packages."""

from __future__ import annotations

import os
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, os.fspath(ROOT / "tools"))

from datoviz_build_backend.license_payload import package_license_paths


def main() -> int:
    payloads = package_license_paths(ROOT)
    print(f"validated {len(payloads)} package license payload file(s)")
    for source, destination in payloads:
        print(f"{destination}: {source.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
