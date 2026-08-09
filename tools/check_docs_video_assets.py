#!/usr/bin/env python3
"""Report local documentation MP4 availability and write its browser manifest."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

import gallery_media

ROOT = gallery_media.ROOT
DEFAULT_OUTPUT_DIR = ROOT / 'build/gallery-webp/v0.4'
DEFAULT_AVAILABILITY_MANIFEST = DEFAULT_OUTPUT_DIR / 'local-video-assets.json'


def parse_args() -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, default=gallery_media.DEFAULT_MANIFEST)
    parser.add_argument('--output-dir', type=Path, default=DEFAULT_OUTPUT_DIR)
    parser.add_argument(
        '--availability-manifest',
        type=Path,
        default=DEFAULT_AVAILABILITY_MANIFEST,
    )
    return parser.parse_args()


def expected_video_assets(manifest: dict) -> list[tuple[str, str]]:
    """Return sorted lane/example keys for documentation cards that require MP4 previews."""
    return sorted(gallery_media.video_preview_keys(manifest))


def available_video_urls(
    expected: list[tuple[str, str]], output_dir: Path
) -> tuple[list[str], list[str]]:
    """Return available site URLs and missing lane/example labels."""
    available = []
    missing = []
    for lane, example_id in expected:
        if (output_dir / lane / f'{example_id}.mp4').is_file():
            available.append(f'/assets/gallery/v0.4/{lane}/{example_id}.mp4')
        else:
            missing.append(f'{lane}/{example_id}')
    return available, missing


def write_availability_manifest(path: Path, available: list[str], expected_count: int) -> None:
    """Write the local-browser MP4 availability manifest."""
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = {'available': available, 'expected': expected_count}
    path.write_text(json.dumps(payload, indent=2, sort_keys=True) + '\n', encoding='utf8')


def main() -> int:
    """Write availability metadata and report missing local MP4 previews."""
    args = parse_args()
    manifest = gallery_media.load_manifest(args.manifest)
    expected = expected_video_assets(manifest)
    available, missing = available_video_urls(expected, args.output_dir)
    write_availability_manifest(args.availability_manifest, available, len(expected))
    if missing:
        print(
            f'WARNING: {len(missing)} gallery MP4 preview(s) are missing; '
            'run `just docs-video-assets` in another terminal. '
            'Posters will be shown until generation finishes.'
        )
    else:
        print(f'Local gallery MP4 previews: {len(available)}/{len(expected)} available.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
