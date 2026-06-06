#!/usr/bin/env python3
"""Check generated public example JSON manifests for drift."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any

import build_capabilities
import build_examples_manifest


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_EXAMPLES = ROOT / "docs/examples/examples.json"
DEFAULT_CAPABILITIES = ROOT / "docs/examples/capabilities.json"
DEFAULT_MANIFEST = ROOT / "examples/c/MANIFEST.yaml"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--examples", type=Path, default=DEFAULT_EXAMPLES)
    parser.add_argument("--capabilities", type=Path, default=DEFAULT_CAPABILITIES)
    return parser.parse_args()


def _canonical_json(payload: dict[str, Any]) -> str:
    return json.dumps(payload, indent=2) + "\n"


def _check_file(path: Path, expected: str) -> bool:
    if not path.exists():
        print(f"missing generated manifest: {path}")
        return False
    actual = path.read_text(encoding="utf8")
    if actual == expected:
        return True
    print(f"generated manifest drift: {path}")
    print(f"regenerate with: python3 {path_to_tool(path)}")
    return False


def path_to_tool(path: Path) -> str:
    if path.name == "examples.json":
        return "tools/build_examples_manifest.py"
    if path.name == "capabilities.json":
        return "tools/build_capabilities.py"
    return "tools/build_examples_manifest.py"


def main() -> int:
    args = parse_args()
    checks = [
        (
            args.examples,
            _canonical_json(build_examples_manifest.build_payload(args.manifest)),
        ),
        (
            args.capabilities,
            _canonical_json(build_capabilities.build_payload(args.manifest)),
        ),
    ]
    ok = all(_check_file(path, expected) for path, expected in checks)
    if ok:
        print("generated example manifests are up to date")
        return 0
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
