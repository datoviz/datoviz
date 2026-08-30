"""Authoritative third-party license payload paths for Datoviz packages."""

from __future__ import annotations

from pathlib import Path

from .config import ROOT


MANIFEST = Path("licenses/THIRD_PARTY_LICENSES.txt")
NOTICE = Path("licenses/THIRD_PARTY_NOTICES.md")
PROJECT_LICENSE = Path("LICENSE")


def third_party_license_paths(root: Path = ROOT) -> list[Path]:
    """Return validated repository-relative license authorities from the manifest."""

    manifest = root / MANIFEST
    if not manifest.is_file():
        raise FileNotFoundError(f"third-party license manifest is missing: {manifest}")

    paths: list[Path] = []
    seen: set[Path] = set()
    for raw in manifest.read_text(encoding="utf8").splitlines():
        text = raw.strip()
        if not text or text.startswith("#"):
            continue
        rel = Path(text)
        if rel.is_absolute() or ".." in rel.parts:
            raise ValueError(f"license manifest path must be safe and relative: {text}")
        if rel in seen:
            raise ValueError(f"duplicate third-party license manifest path: {text}")
        path = root / rel
        if not path.is_file():
            raise FileNotFoundError(f"third-party license authority is missing: {rel}")
        seen.add(rel)
        paths.append(rel)
    if not paths:
        raise ValueError("third-party license manifest is empty")
    return paths


def package_license_paths(root: Path = ROOT) -> list[tuple[Path, Path]]:
    """Return source and wheel-relative paths for required package license payloads."""

    project = root / PROJECT_LICENSE
    notice = root / NOTICE
    manifest = root / MANIFEST
    for path in (project, notice, manifest):
        if not path.is_file():
            raise FileNotFoundError(f"required package license payload is missing: {path}")

    payloads = [
        (project, PROJECT_LICENSE),
        (notice, NOTICE),
        (manifest, MANIFEST),
    ]
    for rel in third_party_license_paths(root):
        payloads.append((root / rel, rel))
    return payloads
