#!/usr/bin/env python3
"""Generate build-local WebP derivatives for gallery screenshots."""

from __future__ import annotations

import argparse
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path

import build_gallery
import gallery_media


ROOT = gallery_media.ROOT
DEFAULT_OUTPUT_DIR = ROOT / "build/gallery-webp/v0.4"
DEFAULT_QUALITY = 90


@dataclass(frozen=True)
class GalleryWebPResult:
    converted: int = 0
    skipped: int = 0
    missing: int = 0
    animated_skipped: int = 0
    selected: int = 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=gallery_media.DEFAULT_MANIFEST)
    parser.add_argument("--image-dir", type=Path, default=build_gallery.DEFAULT_IMAGE_DIR)
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR)
    parser.add_argument("--quality", type=int, default=DEFAULT_QUALITY)
    parser.add_argument("--id", action="append", default=[], help="example id; repeat or comma-separate")
    parser.add_argument("--lane", action="append", default=[], help="gallery lane")
    parser.add_argument("--dry-run", action="store_true", help="list conversions without writing WebP files")
    parser.add_argument("--force", action="store_true", help="rewrite WebP files even when newer than PNGs")
    parser.add_argument(
        "--animated-fallbacks",
        action="store_true",
        help="create missing still-image fallbacks for animated and video previews",
    )
    parser.add_argument("--strict", action="store_true", help="fail when selected PNG inputs are missing")
    parser.add_argument(
        "--require-image-dir",
        action="store_true",
        help="fail when the source PNG directory is missing",
    )
    parser.add_argument(
        "--quiet-missing",
        action="store_true",
        help="summarize missing PNG inputs instead of printing each path",
    )
    return parser.parse_args()


def split_values(values: list[str]) -> set[str]:
    return gallery_media.split_values(values)


def output_path(example: build_gallery.Example, output_dir: Path) -> Path:
    return gallery_media.gallery_webp_path(example, output_dir)


def fallback_output_path(example: build_gallery.Example, output_dir: Path) -> Path:
    """Return the documentation fallback path for an animated preview."""
    suffix = (
        ".poster.webp"
        if build_gallery.preferred_preview_media(example) == "video-mp4"
        else ".webp"
    )
    return output_dir / example.lane / f"{example.id}{suffix}"


def animated_preview_keys(manifest_data: dict) -> set[tuple[str, str]]:
    return gallery_media.animated_preview_keys(manifest_data)


def needs_update(png: Path, webp: Path, force: bool) -> bool:
    if force or not webp.exists():
        return True
    return png.stat().st_mtime_ns > webp.stat().st_mtime_ns


def prune_stale_webp(output_dir: Path, examples: list[build_gallery.Example]) -> int:
    valid = {(example.lane, example.id) for example in examples}
    removed = 0
    for lane in gallery_media.DOC_LANES:
        lane_dir = output_dir / lane
        if not lane_dir.exists():
            continue
        for webp in lane_dir.glob("*.webp"):
            stem = webp.stem
            if webp.name.endswith(".poster.webp"):
                stem = webp.name[: -len(".poster.webp")]
            if (lane, stem) in valid:
                continue
            webp.unlink()
            removed += 1
    return removed


def select_examples(
    examples: list[build_gallery.Example],
    animated_keys: set[tuple[str, str]],
    ids: set[str],
    lanes: set[str],
    animated_fallbacks: bool,
) -> tuple[list[build_gallery.Example], int]:
    selected: list[build_gallery.Example] = []
    animated_skipped = 0
    for example in examples:
        if "screenshot" not in example.validation:
            continue
        if example.lane not in gallery_media.DOC_LANES:
            continue
        if ids and example.id not in ids:
            continue
        if lanes and example.lane not in lanes:
            continue
        if (example.lane, example.id) in animated_keys:
            if not animated_fallbacks:
                animated_skipped += 1
                continue
        selected.append(example)
    selected.sort(key=lambda item: (item.lane, item.id))
    return selected, animated_skipped


def generate_gallery_webp(
    *,
    manifest: Path = gallery_media.DEFAULT_MANIFEST,
    image_dir: Path = build_gallery.DEFAULT_IMAGE_DIR,
    output_dir: Path = DEFAULT_OUTPUT_DIR,
    quality: int = DEFAULT_QUALITY,
    ids: set[str] | None = None,
    lanes: set[str] | None = None,
    dry_run: bool = False,
    force: bool = False,
    strict: bool = False,
    require_image_dir: bool = False,
    quiet_missing: bool = False,
    prune_stale: bool = True,
    animated_fallbacks: bool = False,
) -> tuple[int, GalleryWebPResult]:
    if not 0 <= quality <= 100:
        print("--quality must be between 0 and 100")
        return 2, GalleryWebPResult()
    if not image_dir.exists():
        print(f"gallery PNG source directory not found: {image_dir}")
        print("Run: git submodule update --init --recursive data")
        return (2 if strict or require_image_dir else 0), GalleryWebPResult()

    cwebp = shutil.which("cwebp")
    if cwebp is None and not dry_run:
        print("cwebp not found; install the WebP tools package or rerun with --dry-run")
        return 2, GalleryWebPResult()

    manifest_data = gallery_media.load_manifest(manifest)
    all_examples = build_gallery.collect_examples(manifest_data)
    animated_keys = animated_preview_keys(manifest_data)
    ids = ids or set()
    lanes = lanes or set()
    examples, animated_skipped = select_examples(
        all_examples, animated_keys, ids, lanes, animated_fallbacks
    )
    if not examples:
        if animated_skipped:
            print("No static WebP conversions; animated previews are owned by build_gallery_animations.py")
            return 0, GalleryWebPResult(animated_skipped=animated_skipped, selected=animated_skipped)
        print("No matching gallery examples.")
        return 1, GalleryWebPResult()
    can_prune = prune_stale and not dry_run and not ids and not lanes
    valid_for_prune = [
        example
        for example in all_examples
        if "screenshot" in example.validation and example.lane in gallery_media.DOC_LANES
    ]
    removed = prune_stale_webp(output_dir, valid_for_prune) if can_prune else 0

    converted = 0
    skipped = 0
    missing = 0
    for example in examples:
        png = gallery_media.gallery_png_path(example, image_dir)
        is_animated = (example.lane, example.id) in animated_keys
        webp = (
            fallback_output_path(example, output_dir)
            if is_animated and animated_fallbacks
            else output_path(example, output_dir)
        )
        if not png.exists():
            missing += 1
            if not quiet_missing:
                rel_png = png.relative_to(ROOT) if png.is_relative_to(ROOT) else png
                print(f"missing: {example.id} -> {rel_png}")
            continue
        # A richer animation or video pipeline owns existing animated outputs. The documentation
        # fallback only fills missing paths and must never replace those assets, even with --force.
        if is_animated and animated_fallbacks and webp.exists():
            skipped += 1
            continue
        if not needs_update(png, webp, force):
            skipped += 1
            continue
        converted += 1
        rel_webp = webp.relative_to(ROOT) if webp.is_relative_to(ROOT) else webp
        if dry_run:
            print(f"would convert: {example.id} -> {rel_webp}")
            continue
        webp.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(
            [cwebp, "-quiet", "-q", str(quality), str(png), "-o", str(webp)],
            check=True,
        )

    result = GalleryWebPResult(
        converted=converted,
        skipped=skipped,
        missing=missing,
        animated_skipped=animated_skipped,
        selected=len(examples),
    )
    action = "would_convert" if dry_run else "converted"
    print(
        f"gallery webp: {action}={converted} skipped={skipped} "
        f"animated_skipped={animated_skipped} missing={missing}"
    )
    if removed:
        print(f"gallery webp: removed_stale={removed}")
    if missing and strict:
        print("Missing committed gallery PNGs. If this is a fresh clone, run:")
        print("  git submodule update --init --recursive data")
    return (1 if missing and strict else 0), result


def main() -> int:
    args = parse_args()
    ids = split_values(args.id)
    lanes = split_values(args.lane)
    rc, _ = generate_gallery_webp(
        manifest=args.manifest,
        image_dir=args.image_dir,
        output_dir=args.output_dir,
        quality=args.quality,
        ids=ids,
        lanes=lanes,
        dry_run=args.dry_run,
        force=args.force,
        strict=args.strict,
        require_image_dir=args.require_image_dir,
        quiet_missing=args.quiet_missing,
        animated_fallbacks=args.animated_fallbacks,
    )
    return rc


if __name__ == "__main__":
    raise SystemExit(main())
