#!/usr/bin/env python3
"""Check native gallery screenshots against the manifest and cache metadata."""

from __future__ import annotations

import argparse
from pathlib import Path

import yaml

import capture_gallery


ROOT = Path(__file__).resolve().parents[1]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=capture_gallery.DEFAULT_MANIFEST)
    parser.add_argument("--build-dir", type=Path, default=capture_gallery.DEFAULT_BUILD_DIR)
    parser.add_argument("--image-dir", type=Path, default=capture_gallery.DEFAULT_IMAGE_DIR)
    parser.add_argument("--cache-dir", type=Path, default=capture_gallery.DEFAULT_CACHE_DIR)
    parser.add_argument("--id", action="append", default=[], help="example id; repeat or comma-separate")
    parser.add_argument("--lane", action="append", default=[], help="gallery lane")
    parser.add_argument(
        "--ignore-cache",
        action="store_true",
        help="only check that PNGs exist and are nonblank",
    )
    parser.add_argument(
        "--full-pixel-check",
        action="store_true",
        help="scan PNG pixels for nonblank validation instead of checking headers and cached hashes",
    )
    return parser.parse_args()


def selected_examples(args: argparse.Namespace) -> list[capture_gallery.CaptureExample]:
    manifest = capture_gallery.load_manifest(args.manifest)
    examples = capture_gallery.collect_examples(manifest)
    ids = capture_gallery.split_values(args.id)
    lanes = capture_gallery.split_values(args.lane)
    selected = []
    for example in examples:
        if ids and example.id not in ids:
            continue
        if lanes and example.lane not in lanes:
            continue
        if not ids and not lanes and "screenshot" not in example.validation:
            continue
        selected.append(example)
    selected.sort(key=lambda item: (item.lane, item.id))
    return selected


def main() -> int:
    args = parse_args()
    try:
        examples = selected_examples(args)
    except (OSError, ValueError, yaml.YAMLError) as exc:
        print(str(exc))
        return 2

    if not examples:
        print("No matching gallery examples.")
        return 1

    input_hashes: dict[str, str] = {}
    if not args.ignore_cache:
        global_hash = capture_gallery.hash_files(
            list(capture_gallery.GLOBAL_FINGERPRINT_PATHS)
            + list(capture_gallery.SHARED_EXAMPLE_PATHS)
        )
        input_hashes = {
            example.id: capture_gallery.input_hash_for(example, global_hash, args.build_dir)
            for example in examples
        }

    counts: dict[str, int] = {}
    failures = 0
    for example in examples:
        png = capture_gallery.output_path(example, args.image_dir)
        if args.full_pixel_check:
            ok, detail = capture_gallery.png_is_nonblank(
                png, (example.expected_width, example.expected_height)
            )
        else:
            ok, detail = capture_gallery.png_dimensions(png)
            expected = f"{example.expected_width}x{example.expected_height}"
            if ok and detail != expected:
                ok = False
                detail = f"expected {expected}, got {detail}"
        status = "ok" if ok else "invalid"
        if ok and not args.ignore_cache:
            cache = capture_gallery.load_cache(capture_gallery.cache_path(example, args.cache_dir))
            if not cache:
                status = "uncached"
            elif cache.get("input_hash") != input_hashes[example.id]:
                status = "stale"
            else:
                try:
                    png_hash = capture_gallery.file_sha256(png)
                except OSError:
                    status = "invalid"
                else:
                    if cache.get("png_hash") != png_hash:
                        status = "changed-outside-cache"
        counts[status] = counts.get(status, 0) + 1
        if status != "ok":
            failures += 1
            rel_png = png.relative_to(ROOT) if png.is_relative_to(ROOT) else png
            print(f"{status}: {example.id} -> {rel_png} ({detail})")

    summary = ", ".join(f"{key}={counts[key]}" for key in sorted(counts))
    print(f"gallery media: {summary}")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
