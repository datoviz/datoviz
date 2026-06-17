#!/usr/bin/env python3
"""Print or validate the intended v0.4 wheel target matrix."""

from __future__ import annotations

import argparse
import json
from dataclasses import asdict, dataclass
from pathlib import Path


PYTHONS_REQUIRED = ("3.10", "3.11", "3.12", "3.13", "3.14")
PYTHONS_PRERELEASE = ("3.15",)


@dataclass(frozen=True)
class WheelTarget:
    os: str
    arch: str
    platform_tag: str
    required: bool


TARGETS = (
    WheelTarget("linux", "x86_64", "manylinux_2_34_x86_64", True),
    WheelTarget("linux", "aarch64", "manylinux_2_34_aarch64", True),
    WheelTarget("macos", "x86_64", "macosx_10_13_x86_64", True),
    WheelTarget("macos", "arm64", "macosx_11_0_arm64", True),
    WheelTarget("windows", "AMD64", "win_amd64", True),
    WheelTarget("windows", "ARM64", "win_arm64", True),
)


def expected_wheel_tags() -> list[str]:
    tags: list[str] = []
    for target in TARGETS:
        tags.append(f"py3-none-{target.platform_tag}")
    return tags


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--include-prerelease", action="store_true")
    parser.add_argument("--dist-dir", type=Path)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    data = {
        "required_pythons": PYTHONS_REQUIRED,
        "prerelease_pythons": PYTHONS_PRERELEASE,
        "targets": [asdict(target) for target in TARGETS],
        "expected_wheel_tags": expected_wheel_tags(),
    }
    if args.json:
        print(json.dumps(data, indent=2, sort_keys=True))
    else:
        print("Required Python test versions:", " ".join(PYTHONS_REQUIRED))
        print("Prerelease Python test versions:", " ".join(PYTHONS_PRERELEASE))
        for target in TARGETS:
            mark = "required" if target.required else "optional"
            print(f"{target.os:7} {target.arch:8} {target.platform_tag:28} {mark}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
