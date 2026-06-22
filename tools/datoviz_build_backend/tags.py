"""Wheel tag and matrix helpers for Datoviz release wheels."""

from __future__ import annotations

import json
import re
from dataclasses import asdict, dataclass
from pathlib import Path

try:
    import tomllib
except ModuleNotFoundError:  # pragma: no cover - Python < 3.11 fallback.
    import tomli as tomllib  # type: ignore[no-redef]

from .config import ROOT


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
    WheelTarget("macos", "x86_64", "macosx_15_0_x86_64", True),
    WheelTarget("macos", "arm64", "macosx_15_0_arm64", True),
    WheelTarget("windows", "AMD64", "win_amd64", True),
    WheelTarget("windows", "ARM64", "win_arm64", True),
)

WHEEL_RE = re.compile(
    r"^datoviz-(?P<version>.+)-(?P<python>[^-]+)-(?P<abi>[^-]+)-(?P<platform>[^-]+)\.whl$"
)


def expected_wheel_tags() -> list[str]:
    """Return all expected release wheel tags."""

    return [f"py3-none-{target.platform_tag}" for target in TARGETS]


def project_version(root: Path = ROOT) -> str:
    """Return the project version from pyproject.toml."""

    data = tomllib.loads((root / "pyproject.toml").read_text(encoding="utf8"))
    version = data.get("project", {}).get("version")
    if not version:
        raise RuntimeError("could not find project.version in pyproject.toml")
    return str(version)


def expected_tags(platform_tags: list[str]) -> list[str]:
    """Return expected complete wheel tags for platform tags."""

    if platform_tags:
        return [f"py3-none-{tag}" for tag in platform_tags]
    return expected_wheel_tags()


def wheel_parts(path: Path) -> tuple[str, str]:
    """Return version and complete tag from a wheel filename."""

    match = WHEEL_RE.match(path.name)
    if match is None:
        raise RuntimeError(f"unexpected wheel filename: {path.name}")
    tag = "-".join((match.group("python"), match.group("abi"), match.group("platform")))
    return match.group("version"), tag


def matrix_json() -> str:
    """Return the release matrix as JSON."""

    data = {
        "required_pythons": PYTHONS_REQUIRED,
        "prerelease_pythons": PYTHONS_PRERELEASE,
        "targets": [asdict(target) for target in TARGETS],
        "expected_wheel_tags": expected_wheel_tags(),
    }
    return json.dumps(data, indent=2, sort_keys=True)


def print_matrix(*, as_json: bool = False) -> None:
    """Print the release wheel matrix."""

    if as_json:
        print(matrix_json())
        return
    print("Required Python test versions:", " ".join(PYTHONS_REQUIRED))
    print("Prerelease Python test versions:", " ".join(PYTHONS_PRERELEASE))
    for target in TARGETS:
        mark = "required" if target.required else "optional"
        print(f"{target.os:7} {target.arch:8} {target.platform_tag:28} {mark}")
