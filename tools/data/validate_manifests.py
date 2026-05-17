#!/usr/bin/env python3
"""Validate Datoviz example-data manifests."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from common import SCHEMA, EXAMPLES_ROOT, sha256_file


def _validate_manifest(path: Path) -> list[str]:
    errors: list[str] = []
    root = path.parent
    try:
        manifest = json.loads(path.read_text(encoding="utf8"))
    except (OSError, json.JSONDecodeError) as exc:
        return [f"{path}: cannot read JSON: {exc}"]

    if manifest.get("schema") != SCHEMA:
        errors.append(f"{path}: unexpected schema {manifest.get('schema')!r}")
    for key in ("id", "title", "status", "producer", "source", "processing", "artifacts"):
        if key not in manifest:
            errors.append(f"{path}: missing {key}")

    artifacts = manifest.get("artifacts", [])
    if not isinstance(artifacts, list):
        errors.append(f"{path}: artifacts must be a list")
        return errors

    for i, artifact in enumerate(artifacts):
        rel = artifact.get("path")
        if not isinstance(rel, str):
            errors.append(f"{path}: artifact {i} missing path")
            continue
        file_path = root / rel
        if not file_path.exists():
            errors.append(f"{path}: artifact {rel} does not exist")
            continue
        expected_bytes = artifact.get("bytes")
        if expected_bytes is not None and expected_bytes != file_path.stat().st_size:
            errors.append(f"{path}: artifact {rel} byte count is stale")
        expected_hash = artifact.get("sha256")
        if expected_hash is not None and expected_hash != sha256_file(file_path):
            errors.append(f"{path}: artifact {rel} sha256 is stale")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("paths", nargs="*", type=Path, help="manifest paths or bundle roots")
    args = parser.parse_args()

    paths: list[Path] = []
    if args.paths:
        for path in args.paths:
            paths.append(path / "manifest.json" if path.is_dir() else path)
    else:
        paths = sorted(EXAMPLES_ROOT.glob("**/manifest.json"))

    errors: list[str] = []
    for path in paths:
        errors.extend(_validate_manifest(path))

    if errors:
        for error in errors:
            print(error, file=sys.stderr)
        return 1
    print(f"validated {len(paths)} manifest(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
