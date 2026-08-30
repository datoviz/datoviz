#!/usr/bin/env python3
"""Shared manifest and media helpers for the v0.4 gallery toolchain."""

from __future__ import annotations

import hashlib
from pathlib import Path
import subprocess
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
CANONICAL_ANIMATION_SIZE = "1280x720"
MP4_MAX_CRF = 40


def prepared_dataset_paths(entry: dict) -> tuple[str, ...]:
    """Return the manifest-declared prepared input paths for one example."""
    dataset = entry.get("dataset") or {}
    if not isinstance(dataset, dict):
        return ()
    paths: list[str] = []
    for key, value in dataset.items():
        if not str(key).endswith("prepared_path"):
            continue
        values = value if isinstance(value, list) else [value]
        for item in values:
            paths.extend(part.strip() for part in str(item).split(";") if part.strip())
    return tuple(paths)


def _hash_path_content(digest: "hashlib._Hash", label: str, path: Path) -> None:
    """Add one file or directory tree to a content digest."""
    digest.update(label.encode("utf8"))
    digest.update(b"\0")
    if not path.exists():
        digest.update(b"missing\0")
        return

    files = (
        [path]
        if path.is_file()
        else sorted(child for child in path.rglob("*") if child.is_file())
    )
    digest.update(b"file\0" if path.is_file() else b"directory\0")
    for child in files:
        name = child.name if path.is_file() else child.relative_to(path).as_posix()
        digest.update(name.encode("utf8"))
        digest.update(b"\0")
        try:
            with child.open("rb") as stream:
                for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                    digest.update(chunk)
        except OSError as exc:
            digest.update(f"unreadable:{type(exc).__name__}".encode("ascii"))
        digest.update(b"\0")


def _data_gitlink(root: Path) -> str:
    """Return the committed data gitlink when no narrower prepared path is declared."""
    try:
        result = subprocess.run(
            ["git", "rev-parse", "HEAD:data"],
            cwd=root,
            check=False,
            capture_output=True,
            text=True,
        )
    except OSError:
        return "unavailable"
    value = result.stdout.strip()
    return value if result.returncode == 0 and value else "unavailable"


def prepared_dataset_fingerprint(entry: dict, root: Path = ROOT) -> str:
    """Fingerprint the narrow prepared inputs that can affect one gallery capture."""
    paths = prepared_dataset_paths(entry)
    data = entry.get("data") or {}
    data_kind = str(data.get("kind", "")) if isinstance(data, dict) else ""
    if not paths and data_kind != "prepared":
        return ""

    digest = hashlib.sha256()
    if paths:
        for value in paths:
            path = Path(value)
            _hash_path_content(digest, value, path if path.is_absolute() else root / path)
    else:
        digest.update(b"data-gitlink\0")
        digest.update(_data_gitlink(root).encode("ascii"))
    return digest.hexdigest()


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


def animation_media_policy_errors(entry: dict) -> list[str]:
    """Return violations of the canonical animated-gallery media policy."""
    preview = preview_metadata(entry)
    if preview.get("kind") != ANIMATED_WEBP_KIND:
        return []

    entry_id = str(entry.get("id", "<missing>"))
    errors: list[str] = []
    size = str(preview.get("size", entry.get("capture", {}).get("size", CANONICAL_ANIMATION_SIZE)))
    if size != CANONICAL_ANIMATION_SIZE:
        errors.append(
            f"{entry_id}: animated preview size must be {CANONICAL_ANIMATION_SIZE}, got {size}"
        )

    card = preview.get("card") or {}
    if not isinstance(card, dict):
        return [*errors, f"{entry_id}: media.preview.card must be a mapping"]
    if "size" in card:
        errors.append(f"{entry_id}: media.preview.card.size is redundant and must be removed")
    if "fallback_fps" in card:
        errors.append(
            f"{entry_id}: media.preview.card.fallback_fps is implicit and must be removed"
        )

    if card.get("preferred") != "video-mp4":
        return errors

    preview_fps = int(preview.get("fps", 0))
    card_fps = int(card.get("fps", preview_fps))
    sample_step = int(card.get("sample_step", card.get("step", 1)))
    base_crf = int(card.get("mp4_crf", 32))
    if preview_fps <= 0 or card_fps <= 0:
        errors.append(f"{entry_id}: preview and MP4 card fps must be positive")
    if sample_step <= 0:
        errors.append(f"{entry_id}: MP4 card sample_step must be positive")
    elif preview_fps != card_fps * sample_step:
        errors.append(
            f"{entry_id}: preview fps {preview_fps} must equal card fps "
            f"{card_fps} * sample_step {sample_step}"
        )
    if not 0 <= base_crf <= MP4_MAX_CRF:
        errors.append(
            f"{entry_id}: media.preview.card.mp4_crf must be between 0 and {MP4_MAX_CRF}"
        )
    return errors


def validate_animation_media_policy(entry: dict) -> None:
    """Raise when one manifest entry violates the animated-gallery media policy."""
    errors = animation_media_policy_errors(entry)
    if errors:
        raise ValueError("; ".join(errors))


def manifest_media_policy_errors(manifest: dict) -> list[str]:
    """Return animated-gallery media policy violations for a loaded manifest."""
    return [
        error
        for entry in manifest.get("examples", [])
        for error in animation_media_policy_errors(entry)
    ]


def is_animated_preview(entry: dict) -> bool:
    return preview_metadata(entry).get("kind") == ANIMATED_WEBP_KIND


def preferred_preview_kind(entry: dict) -> str:
    """Return the preferred public card kind for one manifest entry."""
    preview = preview_metadata(entry)
    card = preview.get("card") or {}
    if isinstance(card, dict) and card.get("preferred"):
        return str(card["preferred"])
    return str(preview.get("kind", ""))


def video_preview_keys(manifest: dict) -> set[tuple[str, str]]:
    """Return animated preview keys whose public card is MP4-owned."""
    return {
        entry_key(entry)
        for entry in manifest.get("examples", [])
        if is_animated_preview(entry) and preferred_preview_kind(entry) == "video-mp4"
    }


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
