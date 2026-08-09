#!/usr/bin/env python3
"""Focused tests for the local documentation MP4 preflight."""

from __future__ import annotations

import json
import sys
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import check_docs_video_assets as check  # noqa: E402


def test_availability_manifest_lists_only_existing_mp4_assets(tmp_path: Path) -> None:
    """Only existing MP4s should be exposed to the local browser."""
    expected = [('features', 'animated'), ('showcases', 'protein')]
    video = tmp_path / 'showcases/protein.mp4'
    video.parent.mkdir(parents=True)
    video.write_bytes(b'mp4')

    available, missing = check.available_video_urls(expected, tmp_path)
    output = tmp_path / 'local-video-assets.json'
    check.write_availability_manifest(output, available, len(expected))

    assert available == ['/assets/gallery/v0.4/showcases/protein.mp4']
    assert missing == ['features/animated']
    assert json.loads(output.read_text(encoding='utf8')) == {
        'available': available,
        'expected': 2,
    }


def test_main_warns_without_failing_when_mp4_assets_are_missing(
    tmp_path: Path, monkeypatch, capsys
) -> None:
    """Missing optional MP4s should warn without blocking docs startup."""
    manifest = {
        'examples': [
            {
                'id': 'video',
                'category': 'showcase',
                'media': {
                    'preview': {
                        'kind': 'animated-webp',
                        'card': {'preferred': 'video-mp4'},
                    }
                },
            }
        ]
    }
    availability = tmp_path / 'local-video-assets.json'
    args = type(
        'Args',
        (),
        {
            'manifest': tmp_path / 'manifest.yaml',
            'output_dir': tmp_path / 'assets',
            'availability_manifest': availability,
        },
    )()
    monkeypatch.setattr(check, 'parse_args', lambda: args)
    monkeypatch.setattr(check.gallery_media, 'load_manifest', lambda unused: manifest)

    assert check.main() == 0
    assert 'WARNING: 1 gallery MP4 preview(s) are missing' in capsys.readouterr().out
    assert json.loads(availability.read_text(encoding='utf8'))['available'] == []
