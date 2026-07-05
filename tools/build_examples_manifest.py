#!/usr/bin/env python3
"""Generate a machine-readable public C example manifest."""

from __future__ import annotations

import argparse
import json
from collections import Counter
from pathlib import Path
from typing import Any

import build_gallery


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "examples/c/MANIFEST.yaml"
DEFAULT_OUTPUT = ROOT / "docs/examples/examples.json"
SCHEMA = "datoviz.public-examples.v1"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    return parser.parse_args()


def _manifest_entries_by_id(manifest: dict[str, Any]) -> dict[str, dict[str, Any]]:
    return {str(entry["id"]): entry for entry in manifest["examples"]}


def _primary_fields(entry: dict[str, Any]) -> dict[str, str]:
    fields = {}
    for key in ("primary_visual", "primary_feature", "primary_runtime", "primary_composite"):
        if key in entry:
            fields[key] = str(entry[key])
    return fields


def _media_fields(example: build_gallery.Example) -> dict[str, Any]:
    screenshot_expected = "screenshot" in example.validation
    source = build_gallery.DEFAULT_IMAGE_DIR / example.lane / f"{example.id}.png"
    site_asset = f"{build_gallery.DEFAULT_IMAGE_URL_BASE}/{example.lane}/{example.id}.webp"
    status = "available" if screenshot_expected and source.exists() else "pending"
    if not screenshot_expected:
        status = "not-required"
    return {
        "screenshot": {
            "expected": screenshot_expected,
            "status": status,
            "source": source.relative_to(ROOT).as_posix(),
            "site_asset": site_asset,
        }
    }


def _webgpu_fields(example: build_gallery.Example) -> dict[str, Any]:
    item: dict[str, Any] = {
        "webgpu_status": example.webgpu_status,
        "webgpu_status_label": build_gallery.webgpu_status_label(example.webgpu_status),
        "webgpu_requirements": list(example.webgpu_requirements),
    }
    if example.webgpu_reason:
        item["webgpu_reason"] = example.webgpu_reason
    if example.webgpu:
        item["webgpu"] = example.webgpu
    if example.webgpu_route:
        item["webgpu_route"] = example.webgpu_route
        item["webgpu_site_route"] = example.webgpu_site_route
    return item


def _json_example(example: build_gallery.Example, entry: dict[str, Any]) -> dict[str, Any]:
    item: dict[str, Any] = {
        "id": example.id,
        "title": example.title,
        "category": example.category,
        "lane": example.lane,
        "stage": example.stage,
        "status": example.status,
        "source": example.source,
        "source_url": build_gallery.source_url(example),
        "page": example.page_path,
        "summary": example.summary,
        "validation": example.validation,
        "build_command": f"just example-c {example.rel_executable}",
        "smoke_command": f"./build/examples/c/{example.rel_executable} --png",
        "tags": list(example.tags),
        "data": example.data,
        "data_kind": str(example.data.get("kind", "")),
        "media": _media_fields(example),
    }
    item.update(_webgpu_fields(example))
    item.update(_primary_fields(entry))
    if example.dataset:
        item["dataset"] = example.dataset
    if example.encoding:
        item["encoding"] = example.encoding
    if example.python_source is not None:
        item["python_source"] = example.python_source
        item["python_source_url"] = f"{build_gallery.SOURCE_BASE_URL}/{example.python_source}"
    return item


def build_payload(manifest_path: Path) -> dict[str, Any]:
    manifest = build_gallery.load_manifest(manifest_path)
    examples = build_gallery.collect_examples(manifest)
    entries = _manifest_entries_by_id(manifest)
    category_counts = Counter(example.category for example in examples)
    status_counts = Counter(example.status for example in examples)

    return {
        "schema": SCHEMA,
        "source": manifest_path.relative_to(ROOT).as_posix(),
        "generated_by": "tools/build_examples_manifest.py",
        "count": len(examples),
        "categories": dict(sorted(category_counts.items())),
        "statuses": dict(sorted(status_counts.items())),
        "examples": [_json_example(example, entries[example.id]) for example in examples],
    }


def main() -> int:
    args = parse_args()
    payload = build_payload(args.manifest)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf8")
    print(f"Generated {payload['count']} C example entries in {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
