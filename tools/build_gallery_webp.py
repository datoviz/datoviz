#!/usr/bin/env python3
"""Generate build-local WebP derivatives for gallery screenshots."""

from __future__ import annotations

import argparse
import shutil
import subprocess
from pathlib import Path

import build_gallery


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT_DIR = ROOT / "build/gallery-webp/v0.4"
DEFAULT_QUALITY = 90


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
    return parser.parse_args()


def split_values(values: list[str]) -> set[str]:
    out: set[str] = set()
    for value in values:
        out.update(part.strip() for part in value.split(",") if part.strip())
    return out


def selected_examples(args: argparse.Namespace) -> list[build_gallery.Example]:
    manifest = build_gallery.load_manifest(args.manifest)
    examples = build_gallery.collect_examples(manifest)
    ids = split_values(args.id)
    lanes = split_values(args.lane)
    selected = []
    for example in examples:
        if ids and example.id not in ids:
            continue
        if lanes and example.lane not in lanes:
            continue
        selected.append(example)
    selected.sort(key=lambda item: (item.lane, item.id))
    return selected


def output_path(example: build_gallery.Example, output_dir: Path) -> Path:
    return output_dir / example.lane / f"{example.id}.webp"


def needs_update(png: Path, webp: Path, force: bool) -> bool:
    if force or not webp.exists():
        return True
    return png.stat().st_mtime_ns > webp.stat().st_mtime_ns


def main() -> int:
    args = parse_args()
    if not 0 <= args.quality <= 100:
        print("--quality must be between 0 and 100")
        return 2

    cwebp = shutil.which("cwebp")
    if cwebp is None and not args.dry_run:
        print("cwebp not found; install the WebP tools package or rerun with --dry-run")
        return 2

    examples = selected_examples(args)
    if not examples:
        print("No matching gallery examples.")
        return 1

    converted = 0
    skipped = 0
    missing = 0
    for example in examples:
        png = args.image_dir / example.lane / f"{example.id}.png"
        webp = output_path(example, args.output_dir)
        if not png.exists():
            missing += 1
            rel_png = png.relative_to(ROOT) if png.is_relative_to(ROOT) else png
            print(f"missing: {example.id} -> {rel_png}")
            continue
        if not needs_update(png, webp, args.force):
            skipped += 1
            continue
        converted += 1
        rel_webp = webp.relative_to(ROOT) if webp.is_relative_to(ROOT) else webp
        if args.dry_run:
            print(f"would convert: {example.id} -> {rel_webp}")
            continue
        webp.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(
            [cwebp, "-quiet", "-q", str(args.quality), str(png), "-o", str(webp)],
            check=True,
        )

    action = "would_convert" if args.dry_run else "converted"
    print(f"gallery webp: {action}={converted} skipped={skipped} missing={missing}")
    return 1 if missing and args.strict else 0


if __name__ == "__main__":
    raise SystemExit(main())
