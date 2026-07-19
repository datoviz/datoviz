#!/usr/bin/env python3
"""Reject stale generated gallery previews and deployed card media."""

from __future__ import annotations

import argparse
import hashlib
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
        if candidate.stat().st_mtime_ns < cache_mtime:
            errors.append(f"{label}: generated {kind} predates the current frame cache")
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
    return []


def main() -> int:
    args = parse_args()
    try:
        previews = gallery_frames.collect_previews(
            args.manifest, args.build_examples_dir, set(), set()
        )
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
