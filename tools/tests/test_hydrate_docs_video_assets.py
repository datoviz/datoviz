#!/usr/bin/env python3
"""Focused tests for supervised local documentation video hydration."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import pytest

TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import compare_gallery_media as comparison_media  # noqa: E402
import hydrate_docs_video_assets as hydration  # noqa: E402


def make_args(tmp_path: Path) -> argparse.Namespace:
    """Return an isolated hydration argument set."""
    return argparse.Namespace(
        manifest=tmp_path / 'manifest.yaml',
        product_dir=tmp_path / 'products',
        product_cache_dir=tmp_path / 'product-cache',
        frame_dir=tmp_path / 'frames',
        frame_cache_dir=tmp_path / 'frame-cache',
        staging_root=tmp_path / 'stage',
        availability_manifest=None,
        watched_sentinel=tmp_path / 'watched/sentinel',
        lock_dir=tmp_path / 'hydrate.lock',
        stale_lock_seconds=3600,
        jobs='1',
        capture_jobs='1',
        strict=False,
    )


def comparison(tmp_path: Path, example_id: str = 'video') -> comparison_media.MediaComparison:
    """Create one verified-product-shaped comparison fixture."""
    base = tmp_path / 'products/showcases/video'
    base.mkdir(parents=True, exist_ok=True)
    mp4 = base / f'{example_id}.card.mp4'
    poster = base / f'{example_id}.poster.webp'
    mp4.write_bytes(b'complete-mp4')
    poster.write_bytes(b'complete-poster')
    return comparison_media.MediaComparison(
        id=example_id,
        lane='showcases',
        title='Video',
        source_webp='',
        source_bytes=0,
        source_frames=1,
        source_size='1280x720',
        encoded_size='1280x720',
        encoded_frames=1,
        encoded_fps=30,
        preferred_kind='video-mp4',
        variants=[
            comparison_media.variant('poster', poster, comparison_media.BUDGETS['poster']),
            comparison_media.variant('mp4-card', mp4, comparison_media.BUDGETS['video_card']),
        ],
        webp_html_snippet='',
        video_html_snippet='',
    )


def test_directory_lease_excludes_duplicate_worker_and_releases(tmp_path: Path) -> None:
    """The portable directory lease should exclude only concurrent owners."""
    path = tmp_path / 'worker.lock'
    with hydration.DirectoryLease(path, 3600):
        with pytest.raises(hydration.LockUnavailableError):
            hydration.DirectoryLease(path, 3600).acquire()
    with hydration.DirectoryLease(path, 3600):
        assert path.is_dir()


def test_stage_product_atomically_materializes_pair_without_rewriting(
    tmp_path: Path, monkeypatch
) -> None:
    """Verified products should materialize completely and warm staging should be unchanged."""
    item = comparison(tmp_path)
    monkeypatch.setattr(hydration, 'ROOT', tmp_path)
    stage = tmp_path / 'stage'

    url, changed = hydration.stage_product(item, stage)

    assert url == '/assets/gallery/v0.4/showcases/video.mp4'
    assert changed == 2
    assert (stage / 'showcases/video.mp4').read_bytes() == b'complete-mp4'
    assert (stage / 'showcases/video.poster.webp').read_bytes() == b'complete-poster'
    assert not list(stage.rglob('*.tmp-*'))
    assert hydration.stage_product(item, stage)[1] == 0


def test_ensure_product_reuses_verified_cache_without_encoding(
    tmp_path: Path, monkeypatch
) -> None:
    """A verified product hit must not enter the shared encoding path."""
    item = comparison(tmp_path)
    preview = type('Preview', (), {'id': 'video', 'lane': 'showcases'})()
    sequence = type('Sequence', (), {})()
    profile = comparison_media.EncodingProfile('video-mp4', 1, 40, 32, 38, 30)
    prepared = hydration.PreparedPreview(preview, sequence, profile)
    args = argparse.Namespace(
        cache_dir=tmp_path / 'cache', output_dir=tmp_path / 'products', webm=False
    )
    monkeypatch.setattr(comparison_media, 'comparison_input_hash', lambda *unused: 'current')
    monkeypatch.setattr(comparison_media, 'current_comparison_cache', lambda *unused: item)
    monkeypatch.setattr(
        comparison_media,
        'compare_preview_cached',
        lambda *unused: pytest.fail('warm cache must not encode'),
    )

    result = hydration.ensure_product(prepared, args, 'encoders')

    assert isinstance(result, hydration.ProductOutcome)
    assert result.cached


def test_hydrate_publishes_one_atomic_manifest_and_touches_once(
    tmp_path: Path, monkeypatch
) -> None:
    """One completed batch should expose only successful products and signal once."""
    args = make_args(tmp_path)
    product = comparison(tmp_path)
    preview_ok = type('Preview', (), {'id': 'video', 'lane': 'showcases'})()
    preview_bad = type('Preview', (), {'id': 'broken', 'lane': 'showcases'})()
    manifest = {
        'examples': [
            {
                'id': example_id,
                'category': 'showcase',
                'media': {
                    'preview': {
                        'kind': 'animated-webp',
                        'card': {'preferred': 'video-mp4'},
                    }
                },
            }
            for example_id in ('video', 'broken')
        ]
    }
    monkeypatch.setattr(hydration.gallery_media, 'load_manifest', lambda unused: manifest)
    monkeypatch.setattr(
        hydration.comparison_media,
        'selected_previews',
        lambda unused: [preview_ok, preview_bad],
    )
    monkeypatch.setattr(
        hydration,
        'prepare_preview',
        lambda preview, unused_args, unused_entries: (
            hydration.PreparedPreview(preview, object(), object())
            if preview.id == 'video'
            else hydration.FailedPreview('showcases', 'broken', 'capture failed')
        ),
    )
    monkeypatch.setattr(hydration.comparison_media, 'toolchain_fingerprint', lambda: 'tools')
    monkeypatch.setattr(
        hydration,
        'ensure_product',
        lambda unused_item, unused_args, unused_fingerprint: hydration.ProductOutcome(
            product, True
        ),
    )
    monkeypatch.setattr(hydration, 'ROOT', tmp_path)
    touch_calls = []
    original_touch = Path.touch

    def record_touch(path: Path, *call_args, **call_kwargs) -> None:
        touch_calls.append(path)
        original_touch(path, *call_args, **call_kwargs)

    monkeypatch.setattr(Path, 'touch', record_touch)
    with hydration.DirectoryLease(args.lock_dir, 3600) as lease:
        result = hydration.hydrate(args, lease)

    payload = json.loads(
        (args.staging_root / 'local-video-assets.json').read_text(encoding='utf8')
    )
    assert payload == {
        'available': ['/assets/gallery/v0.4/showcases/video.mp4'],
        'expected': 2,
        'failed': ['showcases/broken'],
    }
    assert result.cache_hits == 1
    assert result.generated == 0
    assert result.materialized == 2
    assert [failure.label for failure in result.failures] == ['showcases/broken']
    assert touch_calls == [args.watched_sentinel]


def test_unchanged_warm_batch_does_not_retouch_sentinel(tmp_path: Path, monkeypatch) -> None:
    """Warm staging should not cause an unnecessary MkDocs reload."""
    args = make_args(tmp_path)
    product = comparison(tmp_path)
    preview = type('Preview', (), {'id': 'video', 'lane': 'showcases'})()
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
    monkeypatch.setattr(hydration.gallery_media, 'load_manifest', lambda unused: manifest)
    monkeypatch.setattr(hydration.comparison_media, 'selected_previews', lambda unused: [preview])
    monkeypatch.setattr(
        hydration,
        'prepare_preview',
        lambda selected, unused_args, unused_entries: hydration.PreparedPreview(
            selected, object(), object()
        ),
    )
    monkeypatch.setattr(hydration.comparison_media, 'toolchain_fingerprint', lambda: 'tools')
    monkeypatch.setattr(
        hydration,
        'ensure_product',
        lambda unused_item, unused_args, unused_fingerprint: hydration.ProductOutcome(
            product, True
        ),
    )
    monkeypatch.setattr(hydration, 'ROOT', tmp_path)
    with hydration.DirectoryLease(args.lock_dir, 3600) as lease:
        first = hydration.hydrate(args, lease)
    first_mtime = args.watched_sentinel.stat().st_mtime_ns
    with hydration.DirectoryLease(args.lock_dir, 3600) as lease:
        second = hydration.hydrate(args, lease)

    assert first.changed
    assert not second.changed
    assert args.watched_sentinel.stat().st_mtime_ns == first_mtime


def test_optional_failure_keeps_existing_poster_and_main_is_soft_by_default(
    tmp_path: Path, monkeypatch, capsys
) -> None:
    """A failed optional video should keep its poster and only fail in explicit strict mode."""
    args = make_args(tmp_path)
    poster = args.staging_root / 'showcases/broken.poster.webp'
    poster.parent.mkdir(parents=True)
    poster.write_bytes(b'usable-poster')
    failure = hydration.FailedPreview('showcases', 'broken', 'encoder unavailable')
    result = hydration.HydrationResult(
        expected=1,
        available=(),
        cache_hits=0,
        generated=0,
        materialized=0,
        failures=(failure,),
        changed=True,
    )
    monkeypatch.setattr(hydration, 'parse_args', lambda: args)
    monkeypatch.setattr(hydration, 'hydrate', lambda unused_args, unused_lease: result)

    assert hydration.main() == 0
    assert poster.read_bytes() == b'usable-poster'
    assert 'WARNING: showcases/broken: encoder unavailable' in capsys.readouterr().err

    args.strict = True
    assert hydration.main() == 1
