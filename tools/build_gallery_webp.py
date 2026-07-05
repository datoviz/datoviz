#!/usr/bin/env python3
"""Generate build-local WebP derivatives for gallery screenshots."""

from __future__ import annotations

import argparse
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path

import build_gallery


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT_DIR = ROOT / "build/gallery-webp/v0.4"
DEFAULT_QUALITY = 90


@dataclass(frozen=True)
class GalleryWebPResult:
    converted: int = 0
    skipped: int = 0
    missing: int = 0
    selected: int = 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=build_gallery.DEFAULT_MANIFEST)
    parser.add_argument("--image-dir", type=Path, default=build_gallery.DEFAULT_IMAGE_DIR)
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR)
    parser.add_argument("--quality", type=int, default=DEFAULT_QUALITY)
    parser.add_argument("--id", action="append", default=[], help="example id; repeat or comma-separate")
    parser.add_argument("--lane", action="append", default=[], help="gallery lane")
    parser.add_argument("--dry-run", action="store_true", help="list conversions without writing WebP files")
    parser.add_argument("--force", action="store_true", help="rewrite WebP files even when newer than PNGs")
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
    out: set[str] = set()
    for value in values:
        out.update(part.strip() for part in value.split(",") if part.strip())
    return out


def output_path(example: build_gallery.Example, output_dir: Path) -> Path:
    return output_dir / example.lane / f"{example.id}.webp"


def needs_update(png: Path, webp: Path, force: bool) -> bool:
    if force or not webp.exists():
        return True
    return png.stat().st_mtime_ns > webp.stat().st_mtime_ns


def prune_stale_webp(output_dir: Path, examples: list[build_gallery.Example]) -> int:
    valid = {(example.lane, example.id) for example in examples}
    removed = 0
    for lane in build_gallery.PUBLIC_LANES:
        lane_dir = output_dir / lane
        if not lane_dir.exists():
            continue
        for webp in lane_dir.glob("*.webp"):
            if (lane, webp.stem) in valid:
                continue
            webp.unlink()
            removed += 1
    return removed


def generate_gallery_webp(
    *,
    manifest: Path = build_gallery.DEFAULT_MANIFEST,
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

    manifest_data = build_gallery.load_manifest(manifest)
    examples = build_gallery.collect_examples(manifest_data)
    ids = ids or set()
    lanes = lanes or set()
    selected = []
    for example in examples:
        if ids and example.id not in ids:
            continue
        if lanes and example.lane not in lanes:
            continue
        selected.append(example)
    selected.sort(key=lambda item: (item.lane, item.id))
    examples = selected
    if not examples:
        print("No matching gallery examples.")
        return 1, GalleryWebPResult()
    can_prune = prune_stale and not dry_run and not ids and not lanes
    removed = prune_stale_webp(output_dir, examples) if can_prune else 0

    converted = 0
    skipped = 0
    missing = 0
    for example in examples:
        png = image_dir / example.lane / f"{example.id}.png"
        webp = output_path(example, output_dir)
        if not png.exists():
            missing += 1
            if not quiet_missing:
                rel_png = png.relative_to(ROOT) if png.is_relative_to(ROOT) else png
                print(f"missing: {example.id} -> {rel_png}")
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
        selected=len(examples),
    )
    action = "would_convert" if dry_run else "converted"
    print(f"gallery webp: {action}={converted} skipped={skipped} missing={missing}")
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
    )
    return rc


if __name__ == "__main__":
    raise SystemExit(main())
