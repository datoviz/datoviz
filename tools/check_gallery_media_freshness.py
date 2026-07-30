#!/usr/bin/env python3
"""Reject stale generated gallery previews and deployed card media."""

from __future__ import annotations

import argparse
import hashlib
import math
from pathlib import Path

import build_gallery_animations
import compare_gallery_media
import gallery_frames
import gallery_media


ROOT = gallery_media.ROOT
DEFAULT_ANIMATION_DIR = build_gallery_animations.DEFAULT_OUTPUT_DIR
DEFAULT_ANIMATION_CACHE_DIR = build_gallery_animations.DEFAULT_CACHE_DIR
DEFAULT_CARD_DIR = compare_gallery_media.DEFAULT_OUTPUT_DIR
DEFAULT_REPORT = compare_gallery_media.DEFAULT_REPORT
DEFAULT_SITE_DIR = compare_gallery_media.DEFAULT_SITE_OUTPUT_DIR


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=gallery_media.DEFAULT_MANIFEST)
    parser.add_argument("--build-examples-dir", type=Path, default=gallery_frames.DEFAULT_BUILD_EXAMPLES_DIR)
    parser.add_argument("--frame-dir", type=Path, default=gallery_frames.DEFAULT_FRAME_DIR)
    parser.add_argument("--frame-cache-dir", type=Path, default=gallery_frames.DEFAULT_CACHE_DIR)
    parser.add_argument("--animation-dir", type=Path, default=DEFAULT_ANIMATION_DIR)
    parser.add_argument("--animation-cache-dir", type=Path, default=DEFAULT_ANIMATION_CACHE_DIR)
    parser.add_argument("--card-dir", type=Path, default=DEFAULT_CARD_DIR)
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT)
    parser.add_argument("--site-dir", type=Path, default=DEFAULT_SITE_DIR)
    return parser.parse_args()


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as file:
        for chunk in iter(lambda: file.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def validate_canonical_dimensions(path: Path, label: str) -> list[str]:
    """Validate one encoded animation or poster against the canonical size."""
    try:
        probe = compare_gallery_media.probe_media(path)
    except (OSError, RuntimeError, ValueError) as exc:
        return [f"{label}: unable to inspect encoded dimensions: {exc}"]
    return canonical_dimension_errors(probe, label)


def canonical_dimension_errors(
    probe: compare_gallery_media.MediaProbe, label: str
) -> list[str]:
    """Validate already-probed dimensions against the canonical size."""
    expected_width, expected_height = (
        int(value)
        for value in gallery_media.CANONICAL_ANIMATION_SIZE.split("x", maxsplit=1)
    )
    if (probe.width, probe.height) == (expected_width, expected_height):
        return []
    return [
        f"{label}: encoded dimensions are {probe.width}x{probe.height}, "
        f"expected {gallery_media.CANONICAL_ANIMATION_SIZE}"
    ]


def validate_video_card(
    preview: gallery_frames.AnimatedPreview,
    frame_cache_path: Path,
    item: compare_gallery_media.MediaComparison | None,
    card_dir: Path,
    site_dir: Path,
) -> list[str]:
    errors: list[str] = []
    label = f"{preview.lane}/{preview.id}"
    if item is None:
        return [f"{label}: missing gallery media comparison report entry"]
    if item.preferred_kind != "video-mp4":
        return [f"{label}: comparison report preferred kind is {item.preferred_kind!r}, expected 'video-mp4'"]
    if item.encoded_size != gallery_media.CANONICAL_ANIMATION_SIZE:
        errors.append(
            f"{label}: encoded size is {item.encoded_size}, "
            f"expected {gallery_media.CANONICAL_ANIMATION_SIZE}"
        )
    if item.sample_step <= 0 or item.encoded_fps * item.sample_step != preview.fps:
        errors.append(f"{label}: report FPS and sample_step do not preserve the source rate")
    expected_frames = math.ceil(preview.frames / max(1, item.sample_step))
    if item.encoded_frames != expected_frames:
        errors.append(
            f"{label}: report has {item.encoded_frames} encoded frames, "
            f"expected {expected_frames}"
        )
    expected_duration = preview.frames / preview.fps
    if not math.isclose(item.source_duration, expected_duration, abs_tol=1e-6):
        errors.append(f"{label}: report source duration is inconsistent")
    if not math.isclose(item.encoded_duration, expected_duration, abs_tol=1e-6):
        errors.append(f"{label}: report encoded duration does not preserve source duration")

    variants = {variant.kind: variant for variant in item.variants}
    targets = {
        "poster": (
            card_dir / preview.lane / preview.id / f"{preview.id}.poster.webp",
            site_dir / preview.lane / f"{preview.id}.poster.webp",
        ),
        "mp4-card": (
            card_dir / preview.lane / preview.id / f"{preview.id}.card.mp4",
            site_dir / preview.lane / f"{preview.id}.mp4",
        ),
    }
    cache_mtime = frame_cache_path.stat().st_mtime_ns if frame_cache_path.is_file() else 0
    for kind, (candidate, target) in targets.items():
        variant = variants.get(kind)
        if variant is None:
            errors.append(f"{label}: comparison report is missing {kind}")
            continue
        reported_candidate = ROOT / variant.path
        if reported_candidate.resolve() != candidate.resolve():
            errors.append(f"{label}: comparison report has an unexpected {kind} path")
        if not candidate.is_file():
            errors.append(f"{label}: missing generated {kind}: {candidate}")
            continue
        if candidate.stat().st_size != variant.bytes:
            errors.append(f"{label}: generated {kind} differs from its comparison report")
        if variant.status != "ok" or variant.bytes > variant.budget_bytes:
            errors.append(f"{label}: generated {kind} exceeds its declared budget")
        if candidate.stat().st_mtime_ns < cache_mtime:
            errors.append(f"{label}: generated {kind} predates the current frame cache")
        try:
            probe = compare_gallery_media.probe_media(candidate)
        except (OSError, RuntimeError, ValueError) as exc:
            errors.append(f"{label}: unable to inspect generated {kind}: {exc}")
            probe = None
        if probe is not None:
            errors.extend(canonical_dimension_errors(probe, f"{label}: generated {kind}"))
        if probe is not None and kind == "mp4-card":
            if not math.isclose(probe.fps, item.encoded_fps, abs_tol=0.01):
                errors.append(
                    f"{label}: generated MP4 is {probe.fps:g} fps, "
                    f"report says {item.encoded_fps} fps"
                )
            if probe.frames and probe.frames != item.encoded_frames:
                errors.append(
                    f"{label}: generated MP4 has {probe.frames} frames, "
                    f"report says {item.encoded_frames}"
                )
            tolerance = 1.0 / max(1, item.encoded_fps) + 0.02
            if not math.isclose(probe.duration, expected_duration, abs_tol=tolerance):
                errors.append(
                    f"{label}: generated MP4 duration {probe.duration:g}s "
                    f"does not preserve {expected_duration:g}s"
                )
        if not target.is_file():
            errors.append(f"{label}: missing site {kind}: {target}")
        elif file_sha256(candidate) != file_sha256(target):
            errors.append(f"{label}: site {kind} differs from the current generated candidate")
    return errors


def validate_preview(
    preview: gallery_frames.AnimatedPreview,
    args: argparse.Namespace,
    reports: dict[tuple[str, str], compare_gallery_media.MediaComparison],
    video_keys: set[tuple[str, str]],
) -> list[str]:
    label = f"{preview.lane}/{preview.id}"
    frame_path = args.frame_dir / preview.lane / preview.id
    frame_input_hash = gallery_frames.input_hash_for(preview, args.manifest, args.frame_dir)
    frame_hit, frame_reason, frames_hash = gallery_frames.current_cache_hit(
        preview, frame_path, args.frame_cache_dir, frame_input_hash
    )
    if not frame_hit:
        return [f"{label}: frame cache is not current ({frame_reason})"]

    key = (preview.lane, preview.id)
    if key in video_keys:
        return validate_video_card(
            preview,
            gallery_frames.cache_path(preview, args.frame_cache_dir),
            reports.get(key),
            args.card_dir,
            args.site_dir,
        )

    frames = gallery_frames.FrameSequence(
        preview=preview,
        frame_dir=frame_path,
        input_hash=frame_input_hash,
        frames_hash=frames_hash,
        generated=False,
        reason="current",
    )
    quality = preview.webp_quality or build_gallery_animations.DEFAULT_QUALITY
    animation_path = args.animation_dir / preview.lane / f"{preview.id}.webp"
    animation_input_hash = build_gallery_animations.input_hash_for(
        preview, frames, args.animation_dir, quality
    )
    animation_hit, animation_reason = build_gallery_animations.current_cache_hit(
        preview, animation_path, args.animation_cache_dir, animation_input_hash
    )
    if not animation_hit:
        return [f"{label}: animated preview is not current ({animation_reason})"]
    return validate_canonical_dimensions(animation_path, f"{label}: animated preview")


def main() -> int:
    args = parse_args()
    try:
        previews = gallery_frames.collect_previews(
            args.manifest, args.build_examples_dir, set(), set()
        )
        policy_errors = gallery_media.manifest_media_policy_errors(
            gallery_media.load_manifest(args.manifest)
        )
        if policy_errors:
            print("gallery media freshness failed:")
            for error in policy_errors:
                print(f"  {error}")
            return 1
        reports = {
            (item.lane, item.id): item
            for item in compare_gallery_media.read_report(args.report)
        }
        video_keys = compare_gallery_media.site_video_keys(args.manifest)
        errors = [
            error
            for preview in previews
            for error in validate_preview(preview, args, reports, video_keys)
        ]
        if errors:
            print("gallery media freshness failed:")
            for error in errors:
                print(f"  {error}")
            return 1
        print(
            f"gallery media freshness: current={len(previews)} "
            f"video_cards={len(video_keys)}"
        )
        return 0
    except (OSError, RuntimeError, ValueError) as exc:
        print(f"gallery media freshness: {exc}")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
