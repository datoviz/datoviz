#!/usr/bin/env python3
"""Check static/animated gallery media ownership derived from the manifest."""

from __future__ import annotations

import argparse
from pathlib import Path

import build_gallery
import build_gallery_animations
import build_gallery_webp
import gallery_media


DEFAULT_BUILD_EXAMPLES_DIR = gallery_media.ROOT / "build/examples/c"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=gallery_media.DEFAULT_MANIFEST)
    parser.add_argument("--build-examples-dir", type=Path, default=DEFAULT_BUILD_EXAMPLES_DIR)
    return parser.parse_args()


def _keys(examples: list[build_gallery.Example]) -> set[tuple[str, str]]:
    return {(example.lane, example.id) for example in examples}


def _preview_keys(previews: list[build_gallery_animations.AnimatedPreview]) -> set[tuple[str, str]]:
    return {(preview.lane, preview.id) for preview in previews}


def main() -> int:
    args = parse_args()
    manifest = gallery_media.load_manifest(args.manifest)
    examples = build_gallery.collect_examples(manifest)
    animated_keys = gallery_media.animated_preview_keys(manifest)
    static_examples, animated_skipped = build_gallery_webp.select_static_examples(
        examples, animated_keys, set(), set()
    )
    static_keys = _keys(static_examples)
    overlap = static_keys & animated_keys
    if overlap:
        labels = ", ".join(f"{lane}/{example_id}" for lane, example_id in sorted(overlap))
        print(f"animated previews selected for static conversion: {labels}")
        return 1

    expected_animated_keys = {
        key for key in animated_keys if key[0] in gallery_media.MEDIA_LANES
    }
    previews = build_gallery_animations.collect_previews(
        args.manifest, args.build_examples_dir, set(), set()
    )
    collected_preview_keys = _preview_keys(previews)
    missing = expected_animated_keys - collected_preview_keys
    extra = collected_preview_keys - expected_animated_keys
    if missing or extra:
        if missing:
            labels = ", ".join(f"{lane}/{example_id}" for lane, example_id in sorted(missing))
            print(f"animated previews missing from animation generator: {labels}")
        if extra:
            labels = ", ".join(f"{lane}/{example_id}" for lane, example_id in sorted(extra))
            print(f"unexpected animated previews from animation generator: {labels}")
        return 1

    if animated_skipped != len(expected_animated_keys):
        print(
            "static converter skipped "
            f"{animated_skipped} animated previews, expected {len(expected_animated_keys)}"
        )
        return 1

    print(
        "gallery media pipeline: "
        f"static={len(static_examples)} animated={len(previews)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
