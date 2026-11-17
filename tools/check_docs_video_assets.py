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
    parser.add_argument('--capture-profile', choices=['homepage'])
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
        path = output_dir / lane / f'{example_id}.mp4'
        if path.is_file() and path.stat().st_size:
            available.append(f'/assets/gallery/v0.4/{lane}/{example_id}.mp4')
        else:
            missing.append(f'{lane}/{example_id}')
    return available, missing


def namespace_video_assets(manifest: dict, output_dir: Path, profile: str = '') -> tuple[list[str], list[str], int]:
    """Discover declared, nonempty videos in one served gallery namespace."""
    if profile:
        from docs_asset_stage import homepage_keys
        expected = homepage_keys(manifest)
        directory = output_dir / 'profiles/homepage'
    else:
        expected = expected_video_assets(manifest)
        directory = output_dir
    available, missing = available_video_urls(expected, directory)
    if profile:
        available = [url.replace('/v0.4/', '/v0.4/profiles/homepage/') for url in available]
        missing = [f'profiles/homepage/{label}' for label in missing]
    return available, missing, len(expected)


def combined_video_assets(manifest: dict, output_dir: Path) -> tuple[list[str], list[str], int]:
    """Rebuild browser availability from both namespaces, never from stale JSON."""
    default = namespace_video_assets(manifest, output_dir)
    homepage = namespace_video_assets(manifest, output_dir, 'homepage')
    return sorted(default[0] + homepage[0]), default[1] + homepage[1], default[2] + homepage[2]


def write_availability_manifest(path: Path, available: list[str], expected_count: int) -> None:
    """Write the local-browser MP4 availability manifest."""
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = {'available': available, 'expected': expected_count}
    path.write_text(json.dumps(payload, indent=2, sort_keys=True) + '\n', encoding='utf8')


def main() -> int:
    """Write availability metadata and report missing local MP4 previews."""
    args = parse_args()
    manifest = gallery_media.load_manifest(args.manifest)
    output_dir = args.output_dir
    availability_manifest = args.availability_manifest
    if getattr(args, "capture_profile", ""):
        if output_dir == DEFAULT_OUTPUT_DIR:
            output_dir = gallery_media.HOMEPAGE_PRODUCTS_ROOT / 'assets'
        if availability_manifest == DEFAULT_AVAILABILITY_MANIFEST:
            availability_manifest = output_dir / 'local-video-assets.json'
    available, missing, expected_count = combined_video_assets(manifest, output_dir)
    write_availability_manifest(availability_manifest, available, expected_count)
    if missing:
        print(
            f'WARNING: {len(missing)} gallery MP4 preview(s) are missing; '
            'run `just docs-video-assets` in another terminal. '
            'Posters will be shown until generation finishes.'
        )
    else:
        print(f'Local gallery MP4 previews: {len(available)}/{expected_count} available.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
