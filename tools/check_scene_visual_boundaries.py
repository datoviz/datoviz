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


def _iter_generic_sources(root: Path) -> list[Path]:
    paths: list[Path] = []
    for rel in GENERIC_ENUM_ROOTS:
        full = root / rel
        if full.exists():
            paths.extend(path for path in full.rglob("*") if path.suffix in SOURCE_SUFFIXES)
    paths.extend((root / "src/scene/visuals").glob("*.c"))
    return sorted(set(paths))


def _allowlist_entries(path: Path | None) -> set[str]:
    if path is None or not path.exists():
        return set()
    entries = set()
    for raw in path.read_text(encoding="utf8").splitlines():
        line = raw.strip()
        if line and not line.startswith("#"):
            entries.add(line)
    return entries


def _entry(path: Path, line_no: int, kind: str, text: str, root: Path) -> str:
    rel = path.relative_to(root).as_posix()
    return f"{rel}:{line_no}:{kind}:{text.strip()}"


def _violations(root: Path, allowlist: set[str]) -> list[str]:
    failures: list[str] = []
    for path in _iter_generic_sources(root):
        for line_no, line in enumerate(path.read_text(encoding="utf8").splitlines(), start=1):
            if VISUAL_ENUM_RE.search(line):
                entry = _entry(path, line_no, "visual-enum", line, root)
                if entry not in allowlist:
                    failures.append(entry)

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
            entry = _entry(path, line_no, "private-include", line, root)
            if entry not in allowlist:
                failures.append(entry)
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
        entries = _violations(root, set())
        allowlist_path.write_text(
            "# Current transitional scene visual-boundary violations.\n"
            "# Each line is path:line:kind:source and must be removed as migration slices land.\n"
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
