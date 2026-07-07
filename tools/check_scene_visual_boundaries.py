#!/usr/bin/env python3
"""Check scene visual-family boundary guardrails."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]

GENERIC_ENUM_ROOTS = (
    Path("src/scene/scene_emit"),
    Path("src/scene/runtime"),
    Path("src/scene/render_contract"),
    Path("src/scene/query"),
)
GENERIC_VISUAL_GLOB = "src/scene/visuals/*.c"
SOURCE_SUFFIXES = {".c", ".h", ".cpp", ".hpp"}

VISUAL_ENUM_RE = re.compile(
    r"DVZ_VISUAL_TYPE_"
    r"(POINT|PIXEL|MARKER|SEGMENT|PATH|IMAGE|MESH|VOLUME|PRIMITIVE|SPHERE|GLYPH|TEXT|LABELS|SPLAT|VECTOR)"
)
VISUAL_DESC_RE = re.compile(r"DVZ_SCENE_VISUAL_DESC_[A-Z0-9_]+")
PRIVATE_INCLUDE_RE = re.compile(r'#include "([a-z_]+/internal\.h)"')
VISUAL_FAMILY_NAMES = {
    "glyph",
    "image",
    "labels",
    "marker",
    "mesh",
    "path",
    "pixel",
    "point",
    "primitive",
    "segment",
    "sphere",
    "splat",
    "stroke",
    "text",
    "vector",
    "volume",
}
VISUAL_DESC_CENTRAL_PATHS = {
    "src/scene/visuals/_visual_pipeline.h",
}
VISUAL_DESC_CENTRAL_PREFIXES = ("src/scene/visuals/registry/",)
VISUAL_DESC_OWNER_PREFIXES = {
    "DVZ_SCENE_VISUAL_DESC_POINT": ("src/scene/visuals/point/",),
    "DVZ_SCENE_VISUAL_DESC_PIXEL": ("src/scene/visuals/pixel/",),
    "DVZ_SCENE_VISUAL_DESC_MARKER": ("src/scene/visuals/marker/",),
    "DVZ_SCENE_VISUAL_DESC_SEGMENT": ("src/scene/visuals/segment/",),
    "DVZ_SCENE_VISUAL_DESC_PATH": ("src/scene/visuals/path/",),
    "DVZ_SCENE_VISUAL_DESC_PRIMITIVE": ("src/scene/visuals/primitive/",),
    "DVZ_SCENE_VISUAL_DESC_TEXTURED_MESH": ("src/scene/visuals/mesh/",),
    "DVZ_SCENE_VISUAL_DESC_IMAGE": ("src/scene/visuals/image/",),
    "DVZ_SCENE_VISUAL_DESC_LABELS_SINT": ("src/scene/visuals/labels/",),
    "DVZ_SCENE_VISUAL_DESC_LABELS_UINT": ("src/scene/visuals/labels/",),
    "DVZ_SCENE_VISUAL_DESC_GLYPH": ("src/scene/visuals/glyph/",),
    "DVZ_SCENE_VISUAL_DESC_VOLUME": ("src/scene/visuals/volume/",),
    "DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_SINT": ("src/scene/visuals/volume/",),
    "DVZ_SCENE_VISUAL_DESC_VOLUME_LABELS_UINT": ("src/scene/visuals/volume/",),
    "DVZ_SCENE_VISUAL_DESC_SPHERE": ("src/scene/visuals/sphere/",),
    "DVZ_SCENE_VISUAL_DESC_SPLAT": ("src/scene/visuals/splat/",),
}


class Allowlist:
    """Parsed scene visual-boundary allowlist."""

    def __init__(self) -> None:
        self.exact: set[str] = set()
        self.counts: dict[tuple[str, str, str], int] = {}


def _iter_generic_sources(root: Path) -> list[Path]:
    paths: list[Path] = []
    for rel in GENERIC_ENUM_ROOTS:
        full = root / rel
        if full.exists():
            paths.extend(path for path in full.rglob("*") if path.suffix in SOURCE_SUFFIXES)
    paths.extend((root / "src/scene/visuals").glob("*.c"))
    return sorted(set(paths))


def _allowlist_entries(path: Path | None) -> Allowlist:
    allowlist = Allowlist()
    if path is None or not path.exists():
        return allowlist
    for raw in path.read_text(encoding="utf8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split(":", 3)
        if len(parts) == 4 and parts[1] == "visual-desc-kind":
            try:
                allowlist.counts[(parts[0], parts[1], parts[2])] = int(parts[3])
            except ValueError:
                allowlist.exact.add(line)
            continue
        allowlist.exact.add(line)
    return allowlist


def _entry(path: Path, line_no: int, kind: str, text: str, root: Path) -> str:
    rel = path.relative_to(root).as_posix()
    return f"{rel}:{line_no}:{kind}:{text.strip()}"


def _visual_desc_location_allowed(rel: str, token: str) -> bool:
    if token == "DVZ_SCENE_VISUAL_DESC_NONE":
        return True
    if rel in VISUAL_DESC_CENTRAL_PATHS:
        return True
    if any(rel.startswith(prefix) for prefix in VISUAL_DESC_CENTRAL_PREFIXES):
        return True
    if rel.startswith("src/scene/tests/"):
        return True
    return any(rel.startswith(prefix) for prefix in VISUAL_DESC_OWNER_PREFIXES.get(token, ()))


def _visual_desc_counts(root: Path) -> dict[tuple[str, str, str], int]:
    counts: dict[tuple[str, str, str], int] = {}
    scene_root = root / "src/scene"
    for path in sorted(scene_root.rglob("*")):
        if path.suffix not in SOURCE_SUFFIXES:
            continue
        rel = path.relative_to(root).as_posix()
        for line in path.read_text(encoding="utf8").splitlines():
            for match in VISUAL_DESC_RE.finditer(line):
                token = match.group(0)
                if _visual_desc_location_allowed(rel, token):
                    continue
                key = (rel, "visual-desc-kind", token)
                counts[key] = counts.get(key, 0) + 1
    return counts


def _visual_desc_count_entry(key: tuple[str, str, str], count: int) -> str:
    rel, kind, token = key
    return f"{rel}:{kind}:{token}:{count}"


def _exact_violations(root: Path) -> list[str]:
    entries: list[str] = []
    for path in _iter_generic_sources(root):
        for line_no, line in enumerate(path.read_text(encoding="utf8").splitlines(), start=1):
            if VISUAL_ENUM_RE.search(line):
                entries.append(_entry(path, line_no, "visual-enum", line, root))

    scene_root = root / "src/scene"
    for path in sorted(scene_root.rglob("*")):
        if path.suffix not in SOURCE_SUFFIXES:
            continue
        rel = path.relative_to(root).as_posix()
        rel_parts = Path(rel).parts
        if rel.startswith("src/scene/tests/"):
            continue
        if rel.startswith("src/scene/visuals/") and len(rel_parts) > 4:
            continue
        if rel == "src/scene/visuals/registry/registry.c":
            continue
        for line_no, line in enumerate(path.read_text(encoding="utf8").splitlines(), start=1):
            match = PRIVATE_INCLUDE_RE.search(line)
            if match is None:
                continue
            family_name = match.group(1).split("/", 1)[0]
            if family_name not in VISUAL_FAMILY_NAMES:
                continue
            entries.append(_entry(path, line_no, "private-include", line, root))
    return entries


def _violations(root: Path, allowlist: Allowlist) -> list[str]:
    failures: list[str] = []
    exact_entries = set(_exact_violations(root))
    for entry in sorted(exact_entries):
        if entry not in allowlist.exact:
            failures.append(entry)
    for entry in sorted(allowlist.exact):
        if entry not in exact_entries:
            failures.append(f"{entry}:stale-allowlist")

    desc_counts = _visual_desc_counts(root)
    for key, count in sorted(desc_counts.items()):
        allowed = allowlist.counts.get(key, 0)
        if count > allowed:
            rel, kind, token = key
            failures.append(f"{rel}:{kind}:{token}:count={count} allowed={allowed}")
    for key, allowed in sorted(allowlist.counts.items()):
        current = desc_counts.get(key, 0)
        if current < allowed:
            rel, kind, token = key
            failures.append(f"{rel}:{kind}:{token}:stale-allowlist current={current} allowed={allowed}")
    return failures


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=REPO_ROOT)
    parser.add_argument("--allowlist", type=Path, default=REPO_ROOT / "tools/scene_visual_boundary_allowlist.txt")
    parser.add_argument(
        "--update-allowlist",
        action="store_true",
        help="replace the allowlist with current violations",
    )
    args = parser.parse_args()

    root = args.root.resolve()
    allowlist_path = args.allowlist.resolve()

    if args.update_allowlist:
        entries = _exact_violations(root)
        entries.extend(
            _visual_desc_count_entry(key, count)
            for key, count in sorted(_visual_desc_counts(root).items())
        )
        entries = sorted(set(entries))
        allowlist_path.write_text(
            "# Current transitional scene visual-boundary violations.\n"
            "# Each line is path:line:kind:source and must be removed as migration slices land.\n"
            "# Descriptor-kind entries use path:visual-desc-kind:TOKEN:count.\n"
            + "\n".join(entries)
            + ("\n" if entries else ""),
            encoding="utf8",
        )
        return 0

    failures = _violations(root, _allowlist_entries(allowlist_path))
    if failures:
        for failure in failures:
            print(failure, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
