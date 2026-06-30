#!/usr/bin/env python3
"""Generate machine-readable public example capability coverage."""

from __future__ import annotations

import argparse
import json
from collections import defaultdict
from pathlib import Path
from typing import Any

import build_examples_manifest


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "examples/c/MANIFEST.yaml"
DEFAULT_OUTPUT = ROOT / "docs/examples/capabilities.json"
SCHEMA = "datoviz.public-capabilities.v1"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    return parser.parse_args()


def _example_ref(example: dict[str, Any]) -> dict[str, Any]:
    return {
        "id": example["id"],
        "title": example["title"],
        "category": example["category"],
        "status": example["status"],
        "agent_copy_safe": example["agent_copy_safe"],
        "source": example["source"],
        "page": example["page"],
    }


def _append_capability(
    capabilities: dict[str, dict[str, list[dict[str, Any]]]],
    group: str,
    name: str,
    example: dict[str, Any],
) -> None:
    capabilities[group][name].append(_example_ref(example))


def _summarize_group(items: dict[str, list[dict[str, Any]]]) -> dict[str, Any]:
    return {
        name: {
            "count": len(examples),
            "examples": examples,
        }
        for name, examples in sorted(items.items())
    }


def build_payload(manifest_path: Path) -> dict[str, Any]:
    examples_payload = build_examples_manifest.build_payload(manifest_path)
    capabilities: dict[str, dict[str, list[dict[str, Any]]]] = {
        "visuals": defaultdict(list),
        "features": defaultdict(list),
        "runtime": defaultdict(list),
        "composites": defaultdict(list),
        "tags": defaultdict(list),
        "data_kinds": defaultdict(list),
    }

    for example in examples_payload["examples"]:
        if "primary_visual" in example:
            _append_capability(capabilities, "visuals", example["primary_visual"], example)
        if "primary_feature" in example:
            _append_capability(capabilities, "features", example["primary_feature"], example)
        if "primary_runtime" in example:
            _append_capability(capabilities, "runtime", example["primary_runtime"], example)
        if "primary_composite" in example:
            _append_capability(capabilities, "composites", example["primary_composite"], example)
        for tag in example.get("tags", []):
            _append_capability(capabilities, "tags", tag, example)
        data_kind = example.get("data", {}).get("kind")
        if data_kind:
            _append_capability(capabilities, "data_kinds", str(data_kind), example)

    return {
        "schema": SCHEMA,
        "source": manifest_path.relative_to(ROOT).as_posix(),
        "generated_by": "tools/build_capabilities.py",
        "example_manifest": "docs/examples/examples.json",
        "example_count": examples_payload["count"],
        "categories": examples_payload["categories"],
        "statuses": examples_payload["statuses"],
        "agent_copy_safe": examples_payload["agent_copy_safe"],
        "capabilities": {
            group: _summarize_group(items) for group, items in sorted(capabilities.items())
        },
    }


def main() -> int:
    args = parse_args()
    payload = build_payload(args.manifest)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf8")
    print(f"Generated capability coverage in {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
