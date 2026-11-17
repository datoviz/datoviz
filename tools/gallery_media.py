#!/usr/bin/env python3
"""Shared manifest and media helpers for the v0.4 gallery toolchain."""

from __future__ import annotations

import copy
import hashlib
import math
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


HOMEPAGE_PRODUCTS_ROOT = ROOT / "build/homepage-review/products"
HOMEPAGE_IDS = frozenset({
    "showcases_protein", "showcases_point_cloud", "showcases_cortical_activity",
    "showcases_textured_planet",
})


def resolve_media_entry(entry: dict, profile: str = "") -> dict:
    """Resolve one capture profile without mutating the source manifest entry."""
    if profile not in {"", "homepage"}:
        raise ValueError(f"unknown media profile {profile!r}")
    result = copy.deepcopy(entry)
    media = result.setdefault("media", {})
    profiles = media.pop("profiles", {})
    if not profile:
        return result
    entry_id = str(entry.get("id", ""))
    if entry_id not in HOMEPAGE_IDS:
        raise ValueError(f"{entry_id}: homepage profile is restricted to homepage videos")
    if not isinstance(profiles, dict) or not isinstance(profiles.get(profile), dict):
        raise ValueError(f"{entry_id}: missing media.profiles.{profile} mapping")
    overrides = profiles[profile]
    allowed = {"frames", "fps", "sample_stride", "time_scale", "presentation", "card", "required_inputs"}
    unknown = set(overrides) - allowed
    if unknown:
        raise ValueError(f"{entry_id}: unknown homepage profile fields: {sorted(unknown)}")
    preview = media.setdefault("preview", {})
    if not isinstance(preview, dict):
        raise ValueError(f"{entry_id}: media.preview must be a mapping")
    card = preview.get("card", {})
    if not isinstance(card, dict):
        raise ValueError(f"{entry_id}: media.preview.card must be a mapping")
    for key, value in overrides.items():
        if key in {"presentation", "card", "required_inputs"}:
            continue
        if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value) or value <= 0:
            raise ValueError(f"{entry_id}: profile {key} must be positive and finite")
        if key != "time_scale" and int(value) != value:
            raise ValueError(f"{entry_id}: profile {key} must be an integer")
        preview[key] = value
    presentation = overrides.get("presentation", {})
    if not isinstance(presentation, dict) or set(presentation) - {"kind", "amplitude_deg"}:
        raise ValueError(f"{entry_id}: invalid profile presentation fields")
    kind = presentation.get("kind", "static")
    amplitude = presentation.get("amplitude_deg", 0)
    if kind not in {"static", "sway", "default"}:
        raise ValueError(f"{entry_id}: presentation kind must be default, static or sway")
    if isinstance(amplitude, bool) or not isinstance(amplitude, (int, float)) or not math.isfinite(amplitude) or amplitude < 0:
        raise ValueError(f"{entry_id}: presentation amplitude must be nonnegative and finite")
    preview["presentation"] = {"kind": kind, "amplitude_deg": amplitude}
    required_inputs = overrides.get("required_inputs", [])
    if not isinstance(required_inputs, list):
        raise ValueError(f"{entry_id}: required_inputs must be a list of relative file paths")
    for value in required_inputs:
        if not isinstance(value, str) or not value or Path(value).is_absolute() or ".." in Path(value).parts:
            raise ValueError(f"{entry_id}: required_inputs must contain relative file paths without '..'")
    if required_inputs:
        preview["required_inputs"] = list(required_inputs)
    card_overrides = overrides.get("card", {})
    if not isinstance(card_overrides, dict) or set(card_overrides) - {"mp4_crf", "mp4_max_crf", "video_budget_bytes", "webp_quality", "av1", "mp4_bitrate_kbps", "mp4_preset", "mp4_fps", "mp4_two_pass"}:
        raise ValueError(f"{entry_id}: invalid homepage card override fields")
    # Only explicit named-profile policy can raise the default gallery byte budget.
    preview.setdefault("card", {}).update(card_overrides)
    preview["card"].update({"fps": preview["fps"], "sample_step": 1})
    validate_profile_codecs(entry_id, preview)
    validate_animation_media_policy(result)
    return result


def validate_profile_codecs(entry_id: str, preview: dict) -> None:
    """Validate the narrowly supported homepage candidate codec settings."""
    card = preview["card"]
    def integer(value: object, label: str, low: int, high: int) -> int:
        if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value) or int(value) != value or not low <= value <= high:
            raise ValueError(f"{entry_id}: {label} must be a finite integer between {low} and {high}")
        return int(value)
    source_fps = integer(preview["fps"], "preview fps", 1, 240)
    if "mp4_bitrate_kbps" in card:
        integer(card["mp4_bitrate_kbps"], "mp4_bitrate_kbps", 1, 1_000_000)
    if "mp4_preset" in card and (not isinstance(card["mp4_preset"], str) or card["mp4_preset"] not in {"ultrafast", "superfast", "veryfast", "faster", "fast", "medium", "slow", "slower", "veryslow", "placebo"}):
        raise ValueError(f"{entry_id}: unsupported mp4_preset")
    if "mp4_two_pass" in card and not isinstance(card["mp4_two_pass"], bool):
        raise ValueError(f"{entry_id}: mp4_two_pass must be boolean")
    if card.get("mp4_two_pass") and "mp4_bitrate_kbps" not in card:
        raise ValueError(f"{entry_id}: mp4_two_pass requires mp4_bitrate_kbps")
    if "mp4_fps" in card:
        fps = integer(card["mp4_fps"], "mp4_fps", 1, source_fps)
        if source_fps % fps:
            raise ValueError(f"{entry_id}: mp4_fps must divide capture fps")
        card.update({"fps": fps, "sample_step": source_fps // fps})
    av1 = card.get("av1")
    if "av1" not in card:
        return
    keys = {"encoder_version", "preset", "tune", "crf", "fps", "video_budget_bytes", "temporal_filtering", "film_grain"}
    if not isinstance(av1, dict) or set(av1) != keys:
        raise ValueError(f"{entry_id}: av1 must specify exactly {sorted(keys)}")
    if av1["encoder_version"] != "4.3.0":
        raise ValueError(f"{entry_id}: AV1 requires SVT-AV1 4.3.0")
    for key, low, high in (("preset", 0, 13), ("tune", 0, 4), ("crf", 0, 70), ("fps", 1, source_fps), ("video_budget_bytes", 1, 1_000_000_000), ("film_grain", 0, 0)):
        av1[key] = integer(av1[key], f"av1.{key}", low, high)
    if source_fps % av1["fps"]:
        raise ValueError(f"{entry_id}: AV1 fps must divide capture fps")
    if av1["temporal_filtering"] is not False:
        raise ValueError(f"{entry_id}: AV1 temporal_filtering must be false")


def configure_profile_paths(args: object, defaults: dict[str, tuple[Path, str]]) -> None:
    """Keep every named-profile CLI artifact in its isolated product tree."""
    profile = getattr(args, "profile", "")
    if not profile:
        return
    if profile != "homepage":
        raise ValueError(f"unknown media profile {profile!r}")
    for name, (default, relative) in defaults.items():
        path = getattr(args, name)
        if path == default:
            path = HOMEPAGE_PRODUCTS_ROOT / relative
        if not path.resolve().is_relative_to(HOMEPAGE_PRODUCTS_ROOT.resolve()):
            raise ValueError(f"--{name.replace('_', '-')} with --profile must be within {HOMEPAGE_PRODUCTS_ROOT}")
        setattr(args, name, path)


def prepared_dataset_paths(entry: dict) -> tuple[str, ...]:
    """Return the manifest-declared prepared input paths for one example."""
    dataset = entry.get("dataset") or {}
    if not isinstance(dataset, dict):
        return ()
    paths: list[str] = []
    for key, value in dataset.items():
        if not str(key).endswith("prepared_path") and key != "prepared_source":
            continue
        values = value if isinstance(value, list) else [value]
        for item in values:
            paths.extend(part.strip() for part in str(item).split(";") if part.strip())
    paths.extend(preview_metadata(entry).get("required_inputs", []))
    return tuple(dict.fromkeys(paths))


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

    # Reject malformed numeric fields before int() can truncate or raise OverflowError.
    for mapping, prefix, keys in (
        (preview, "media.preview", ("frames", "fps", "sample_stride", "webp_quality")),
        (card, "media.preview.card", ("fps", "sample_step", "step", "mp4_crf", "webm_crf", "webp_quality", "mp4_max_crf", "video_budget_bytes")),
    ):
        for key in keys:
            if key not in mapping:
                continue
            value = mapping[key]
            if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value) or int(value) != value:
                errors.append(f"{entry_id}: {prefix}.{key} must be a finite integer")
    if errors:
        return errors

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
    max_crf = int(card.get("mp4_max_crf", MP4_MAX_CRF))
    if not 0 <= max_crf <= MP4_MAX_CRF:
        errors.append(f"{entry_id}: media.preview.card.mp4_max_crf must be between 0 and {MP4_MAX_CRF}")
    if "video_budget_bytes" in card and card["video_budget_bytes"] <= 0:
        errors.append(f"{entry_id}: media.preview.card.video_budget_bytes must be positive")
    if base_crf > max_crf:
        errors.append(f"{entry_id}: media.preview.card.mp4_crf must not exceed mp4_max_crf")
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
