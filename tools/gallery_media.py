#!/usr/bin/env python3
"""Shared manifest and media helpers for the v0.4 gallery toolchain."""

from __future__ import annotations

from pathlib import Path
from typing import Iterable

import yaml


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "examples/c/MANIFEST.yaml"

MEDIA_LANES = ("start", "visuals", "features", "runtime", "composites", "showcases")
DOC_LANES = (*MEDIA_LANES, "advanced")

CATEGORY_TO_LANE = {
    "visual": "visuals",
    "feature": "features",
    "runtime": "runtime",
    "composite": "composites",
    "showcase": "showcases",
    "advanced": "advanced",
}
LANE_TO_CATEGORY = {lane: category for category, lane in CATEGORY_TO_LANE.items()}

ANIMATED_WEBP_KIND = "animated-webp"


def load_manifest(path: Path = DEFAULT_MANIFEST) -> dict:
    with path.open("r", encoding="utf8") as f:
        manifest = yaml.safe_load(f) or {}
    if not isinstance(manifest.get("examples"), list):
        raise ValueError(f"{path} does not contain an examples list")
    return manifest


def reviewed_example_ids(manifest: dict) -> set[str]:
    """Return example IDs that are approved for public website generation."""
    batches = manifest.get("batches") or {}
    if not isinstance(batches, dict):
        return set()
    ids: set[str] = set()
    for batch_ids in batches.values():
        if batch_ids is None:
            continue
        ids.update(str(example_id) for example_id in batch_ids)
    return ids


def split_values(values: Iterable[str]) -> set[str]:
    return {
        part.strip()
        for value in values
        for part in str(value).split(",")
        if part.strip()
    }


def lane_for_entry(entry: dict) -> str:
    raw_category = entry.get("category")
    if raw_category is not None:
        category = str(raw_category)
        return CATEGORY_TO_LANE.get(category, category)
    return str(entry.get("lane", ""))


def category_for_entry(entry: dict) -> str:
    raw_category = entry.get("category")
    if raw_category is not None:
        return str(raw_category)
    lane = str(entry.get("lane", ""))
    return LANE_TO_CATEGORY.get(lane, lane)


def entry_key(entry: dict) -> tuple[str, str]:
    return lane_for_entry(entry), str(entry.get("id", ""))


def preview_metadata(entry: dict) -> dict:
    media = entry.get("media") or {}
    preview = media.get("preview") or {}
    if not isinstance(preview, dict):
        return {}
    return preview


def is_animated_preview(entry: dict) -> bool:
    return preview_metadata(entry).get("kind") == ANIMATED_WEBP_KIND


def animated_preview_keys(manifest: dict, lanes: Iterable[str] = MEDIA_LANES) -> set[tuple[str, str]]:
    allowed_lanes = set(lanes)
    keys: set[tuple[str, str]] = set()
    for entry in manifest.get("examples", []):
        lane, example_id = entry_key(entry)
        if not example_id or lane not in allowed_lanes:
            continue
        if is_animated_preview(entry):
            keys.add((lane, example_id))
    return keys


def source_executable_path(source: str, build_examples_dir: Path) -> Path:
    rel = Path(source).relative_to("examples/c").with_suffix("")
    return build_examples_dir / rel


def gallery_png_path(example: object, image_dir: Path) -> Path:
    return image_dir / getattr(example, "lane") / f"{getattr(example, 'id')}.png"


def gallery_webp_path(example: object, output_dir: Path) -> Path:
    return output_dir / getattr(example, "lane") / f"{getattr(example, 'id')}.webp"
