#!/usr/bin/env python3
"""Print or validate the intended v0.4 wheel target matrix."""

from __future__ import annotations

import argparse
import json
import re
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

ROOT = Path(__file__).resolve().parents[2]
WHEEL_RE = re.compile(
    r"^datoviz-(?P<version>.+)-(?P<python>[^-]+)-(?P<abi>[^-]+)-(?P<platform>[^-]+)\.whl$"
)


def expected_wheel_tags() -> list[str]:
    tags: list[str] = []
    for target in TARGETS:
        tags.append(f"py3-none-{target.platform_tag}")
    return tags


def _project_version() -> str:
    text = (ROOT / "pyproject.toml").read_text(encoding="utf8")
    match = re.search(r'^version\s*=\s*"([^"]+)"', text, flags=re.MULTILINE)
    if match is None:
        raise RuntimeError("could not find project.version in pyproject.toml")
    return match.group(1)


def _expected_tags(platform_tags: list[str]) -> list[str]:
    if platform_tags:
        return [f"py3-none-{tag}" for tag in platform_tags]
    return expected_wheel_tags()


def _wheel_parts(path: Path) -> tuple[str, str]:
    match = WHEEL_RE.match(path.name)
    if match is None:
        raise RuntimeError(f"unexpected wheel filename: {path.name}")
    tag = "-".join((match.group("python"), match.group("abi"), match.group("platform")))
    return match.group("version"), tag


def _validate_dist(dist_dir: Path, expected_version: str, expected_tags: list[str]) -> None:
    wheels = sorted(dist_dir.glob("datoviz-*.whl"))
    if not wheels:
        raise RuntimeError(f"no datoviz wheels found in {dist_dir}")

    found: dict[str, Path] = {}
    errors: list[str] = []
    for wheel in wheels:
        version, tag = _wheel_parts(wheel)
        if version != expected_version:
            errors.append(f"{wheel.name}: version {version!r} != expected {expected_version!r}")
        if tag in found:
            errors.append(f"duplicate wheel tag {tag}: {found[tag].name} and {wheel.name}")
        found[tag] = wheel

    expected = set(expected_tags)
    actual = set(found)
    for tag in sorted(expected - actual):
        errors.append(f"missing wheel tag: {tag}")
    for tag in sorted(actual - expected):
        errors.append(f"unexpected wheel tag: {tag}")

    if errors:
        raise RuntimeError("wheel artifact validation failed:\n" + "\n".join(f"- {e}" for e in errors))

    print(f"Validated {len(wheels)} wheel artifact(s) in {dist_dir}")
    for tag in expected_tags:
        print(f"{tag}: {found[tag].name}")


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
        _validate_dist(
            args.dist_dir,
            expected_version=args.version or _project_version(),
            expected_tags=_expected_tags(args.platform_tag),
        )
        return 0

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
