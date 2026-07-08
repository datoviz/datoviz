#!/usr/bin/env python3
"""Check canonical C example IDs against their source paths."""

from __future__ import annotations

import argparse
import re
from pathlib import Path

import gallery_media


ROOT = gallery_media.ROOT


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=gallery_media.DEFAULT_MANIFEST)
    return parser.parse_args()


def canonical_id(source: str) -> str:
    rel = Path(source).relative_to("examples/c").with_suffix("")
    return rel.as_posix().replace("/", "_")


def scenario_ids(source: str) -> set[str]:
    path = ROOT / source
    if not path.exists():
        return set()
    text = path.read_text(encoding="utf8")
    return set(re.findall(r"\.id\s*=\s*\"([^\"]+)\"", text))


def main() -> int:
    args = parse_args()
    manifest = gallery_media.load_manifest(args.manifest)
    seen: set[str] = set()
    errors: list[str] = []
    for entry in manifest.get("examples", []):
        id_ = str(entry.get("id", ""))
        source = str(entry.get("source", ""))
        if not id_:
            errors.append("entry missing id")
            continue
        if id_ in seen:
            errors.append(f"{id_}: duplicate id")
        seen.add(id_)
        if not source.startswith("examples/c/"):
            continue
        expected = canonical_id(source)
        if id_ != expected:
            errors.append(f"{id_}: expected id {expected} from source {source}")
        ids = scenario_ids(source)
        if ids and id_ not in ids:
            found = ", ".join(sorted(ids))
            errors.append(f"{id_}: source {source} does not declare matching scenario id ({found})")

    if errors:
        print("example id check failed:")
        for error in errors:
            print(f"  - {error}")
        return 1
    print(f"example id check: {len(seen)} ids")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
