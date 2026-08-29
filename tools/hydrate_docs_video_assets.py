#!/usr/bin/env python3
"""
Hydrate optional local documentation videos from verified gallery products.

The gallery comparison pipeline owns capture, encoding, and product verification. This tool owns
only local-worker coordination and atomic materialization into a disposable documentation stage.
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import sys
import time
import uuid
from dataclasses import dataclass
from pathlib import Path

import compare_gallery_media as comparison_media
import gallery_frames
import gallery_media
import gallery_workers

ROOT = gallery_media.ROOT
DEFAULT_PRODUCT_DIR = comparison_media.DEFAULT_OUTPUT_DIR
DEFAULT_STAGING_ROOT = ROOT / 'build/docs-assets/local/gallery/v0.4'
DEFAULT_FRAME_DIR = gallery_frames.DEFAULT_FRAME_DIR
DEFAULT_FRAME_CACHE_DIR = gallery_frames.DEFAULT_CACHE_DIR
DEFAULT_PRODUCT_CACHE_DIR = comparison_media.DEFAULT_MEDIA_CACHE_DIR
DEFAULT_LOCK_DIR = ROOT / 'build/docs-assets/local-video-hydration.lock'
DEFAULT_WATCHED_SENTINEL = ROOT / 'build/docs-assets/local/.gallery-video-assets.ready'
DEFAULT_STALE_LOCK_SECONDS = 24 * 60 * 60


class LockUnavailableError(RuntimeError):
    """Raised when another hydration worker owns the directory lease."""


@dataclass(frozen=True)
class PreparedPreview:
    """One preview with a verified current frame sequence and encoding profile."""

    preview: object
    sequence: gallery_frames.FrameSequence
    profile: comparison_media.EncodingProfile


@dataclass(frozen=True)
class ProductOutcome:
    """One verified cached or freshly generated card product."""

    comparison: comparison_media.MediaComparison
    cached: bool


@dataclass(frozen=True)
class FailedPreview:
    """One optional preview that could not be prepared or encoded."""

    lane: str
    example_id: str
    error: str

    @property
    def label(self) -> str:
        """Return the stable manifest label for this preview."""
        return f'{self.lane}/{self.example_id}'


@dataclass(frozen=True)
class HydrationResult:
    """Summary of one completed local hydration batch."""

    expected: int
    available: tuple[str, ...]
    cache_hits: int
    generated: int
    materialized: int
    failures: tuple[FailedPreview, ...]
    changed: bool


class DirectoryLease:
    """Portable atomic-directory lease with bounded stale-owner recovery."""

    def __init__(self, path: Path, stale_seconds: float) -> None:
        """Create an unacquired lease for one explicit directory."""
        self.path = path
        self.stale_seconds = stale_seconds
        self.token = uuid.uuid4().hex
        self.owner_path = path / 'owner.json'
        self.acquired = False

    def _owner_payload(self) -> dict[str, object]:
        return {
            'pid': os.getpid(),
            'started_at': time.time(),
            'token': self.token,
        }

    def _write_owner(self) -> None:
        self.owner_path.write_text(
            json.dumps(self._owner_payload(), sort_keys=True) + '\n', encoding='utf8'
        )

    def _discard_owned_directory(self, path: Path) -> None:
        owner = path / 'owner.json'
        try:
            owner.unlink()
        except FileNotFoundError:
            pass
        try:
            path.rmdir()
        except FileNotFoundError:
            pass

    def _replace_stale_owner(self) -> bool:
        if self.stale_seconds <= 0:
            return False
        try:
            age = time.time() - self.path.stat().st_mtime
        except FileNotFoundError:
            return True
        if age < self.stale_seconds:
            return False
        stale = self.path.with_name(f'.{self.path.name}.stale-{self.token}')
        try:
            os.replace(self.path, stale)
        except (FileNotFoundError, OSError):
            return False
        self._discard_owned_directory(stale)
        return True

    def acquire(self) -> None:
        """Acquire the lease or raise when another current worker owns it."""
        self.path.parent.mkdir(parents=True, exist_ok=True)
        for attempt in range(2):
            try:
                self.path.mkdir()
            except FileExistsError as exc:
                if attempt == 0 and self._replace_stale_owner():
                    continue
                raise LockUnavailableError(
                    f'hydration worker already active: {self.path}'
                ) from exc
            try:
                self._write_owner()
            except Exception:
                self._discard_owned_directory(self.path)
                raise
            self.acquired = True
            return
        raise LockUnavailableError(f'hydration worker already active: {self.path}')

    def refresh(self) -> None:
        """Refresh the lease timestamp between bounded worker phases."""
        if not self.acquired:
            return
        try:
            payload = json.loads(self.owner_path.read_text(encoding='utf8'))
        except (OSError, ValueError, json.JSONDecodeError) as exc:
            raise RuntimeError(f'lost hydration worker lease: {self.path}') from exc
        if payload.get('token') != self.token:
            raise RuntimeError(f'hydration worker lease changed owner: {self.path}')
        os.utime(self.path, None)

    def release(self) -> None:
        """Release the lease only when this worker still owns it."""
        if not self.acquired:
            return
        try:
            payload = json.loads(self.owner_path.read_text(encoding='utf8'))
        except (OSError, ValueError, json.JSONDecodeError):
            return
        if payload.get('token') != self.token:
            return
        self._discard_owned_directory(self.path)
        self.acquired = False

    def __enter__(self) -> DirectoryLease:
        """Acquire and return this lease."""
        self.acquire()
        return self

    def __exit__(
        self, unused_type: object, unused_value: object, unused_traceback: object
    ) -> None:
        """Release this lease on context exit."""
        self.release()


def parse_args() -> argparse.Namespace:
    """Parse local hydration options."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', type=Path, default=gallery_media.DEFAULT_MANIFEST)
    parser.add_argument('--product-dir', type=Path, default=DEFAULT_PRODUCT_DIR)
    parser.add_argument('--product-cache-dir', type=Path, default=DEFAULT_PRODUCT_CACHE_DIR)
    parser.add_argument('--frame-dir', type=Path, default=DEFAULT_FRAME_DIR)
    parser.add_argument('--frame-cache-dir', type=Path, default=DEFAULT_FRAME_CACHE_DIR)
    parser.add_argument('--staging-root', type=Path, default=DEFAULT_STAGING_ROOT)
    parser.add_argument('--availability-manifest', type=Path)
    parser.add_argument('--watched-sentinel', type=Path, default=DEFAULT_WATCHED_SENTINEL)
    parser.add_argument('--lock-dir', type=Path, default=DEFAULT_LOCK_DIR)
    parser.add_argument(
        '--stale-lock-seconds',
        type=float,
        default=DEFAULT_STALE_LOCK_SECONDS,
        help='replace an abandoned directory lease after this age; 0 disables recovery',
    )
    parser.add_argument('--jobs', default='auto', help='parallel encoding jobs')
    parser.add_argument(
        '--capture-jobs',
        default='1' if sys.platform == 'darwin' else 'auto',
        help='parallel capture jobs; defaults to 1 on macOS',
    )
    parser.add_argument(
        '--strict',
        action='store_true',
        help='return failure when any optional video cannot be made current',
    )
    return parser.parse_args()


def comparison_args(args: argparse.Namespace) -> argparse.Namespace:
    """Build the argument contract consumed by the gallery comparison primitives."""
    return argparse.Namespace(
        manifest=args.manifest,
        input_dir=comparison_media.DEFAULT_INPUT_DIR,
        output_dir=args.product_dir,
        site_output_dir=args.staging_root,
        frame_dir=args.frame_dir,
        frame_cache_dir=args.frame_cache_dir,
        cache_dir=args.product_cache_dir,
        report=comparison_media.DEFAULT_REPORT,
        html_report=comparison_media.DEFAULT_HTML_REPORT,
        id=[],
        lane=[],
        step=comparison_media.DEFAULT_STEP,
        webp_quality=comparison_media.DEFAULT_WEBP_QUALITY,
        mp4_crf=comparison_media.DEFAULT_MP4_CRF,
        webm_crf=comparison_media.DEFAULT_WEBM_CRF,
        fps=0,
        preferred_kind=comparison_media.DEFAULT_PREFERRED_KIND,
        webm=False,
        all_animated=False,
        site_video_previews=True,
        no_manifest_card=False,
        dry_run=False,
        force=False,
        jobs=args.jobs,
        capture_jobs=args.capture_jobs,
        write_site_assets=False,
    )


def preview_failure(preview: object, exc: BaseException) -> FailedPreview:
    """Describe one optional product failure without hiding its example identity."""
    return FailedPreview(
        lane=str(preview.lane),
        example_id=str(preview.id),
        error=f'{type(exc).__name__}: {exc}',
    )


def prepare_preview(
    preview: object,
    args: argparse.Namespace,
    manifest_entries: dict[tuple[str, str], dict],
) -> PreparedPreview | FailedPreview:
    """Verify or capture current frames for one optional card."""
    try:
        sequence = gallery_frames.ensure_frames(
            preview,
            args.manifest,
            args.frame_dir,
            args.frame_cache_dir,
            force=False,
        )
        profile = comparison_media.profile_for(preview, args, manifest_entries)
        return PreparedPreview(preview, sequence, profile)
    except Exception as exc:
        return preview_failure(preview, exc)


def ensure_product(
    item: PreparedPreview,
    args: argparse.Namespace,
    encoder_fingerprint: str,
) -> ProductOutcome | FailedPreview:
    """Reuse a verified product or generate a stale/missing one with the shared encoder."""
    preview = item.preview
    try:
        input_hash = comparison_media.comparison_input_hash(
            preview,
            item.sequence,
            item.profile,
            args,
            encoder_fingerprint,
        )
        cached = comparison_media.current_comparison_cache(preview, args.cache_dir, input_hash)
        if cached is not None:
            return ProductOutcome(cached, True)
        outcome = comparison_media.compare_preview_cached(
            preview,
            args,
            item.sequence,
            item.profile,
            encoder_fingerprint,
        )
        return ProductOutcome(outcome.comparison, outcome.cached)
    except Exception as exc:
        return preview_failure(preview, exc)


def files_equal(first: Path, second: Path) -> bool:
    """Return whether two regular files have identical verified content."""
    try:
        if first.stat().st_size != second.stat().st_size:
            return False
    except OSError:
        return False
    return comparison_media.file_sha256(first) == comparison_media.file_sha256(second)


def atomic_copy_if_changed(source: Path, target: Path) -> bool:
    """Atomically replace one staged asset only when its bytes changed."""
    if files_equal(source, target):
        return False
    target.parent.mkdir(parents=True, exist_ok=True)
    temporary = target.with_name(f'.{target.name}.tmp-{uuid.uuid4().hex}')
    try:
        source_hash = comparison_media.file_sha256(source)
        with source.open('rb') as input_file, temporary.open('xb') as output_file:
            shutil.copyfileobj(input_file, output_file)
            output_file.flush()
            os.fsync(output_file.fileno())
        if comparison_media.file_sha256(temporary) != source_hash:
            raise RuntimeError(f'source changed while staging: {source}')
        if comparison_media.file_sha256(source) != source_hash:
            raise RuntimeError(f'source changed while staging: {source}')
        os.replace(temporary, target)
    finally:
        temporary.unlink(missing_ok=True)
    return True


def atomic_json_if_changed(path: Path, payload: dict[str, object]) -> bool:
    """Atomically replace a JSON file only when its canonical content changed."""
    content = json.dumps(payload, indent=2, sort_keys=True) + '\n'
    try:
        if path.read_text(encoding='utf8') == content:
            return False
    except OSError:
        pass
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(f'.{path.name}.tmp-{uuid.uuid4().hex}')
    try:
        with temporary.open('x', encoding='utf8') as output:
            output.write(content)
            output.flush()
            os.fsync(output.fileno())
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)
    return True


def stage_product(item: comparison_media.MediaComparison, staging_root: Path) -> tuple[str, int]:
    """Atomically stage one verified poster/MP4 product pair."""
    if item.preferred_kind != 'video-mp4':
        raise ValueError(f'{item.id}: expected video-mp4 product, got {item.preferred_kind}')
    mp4 = comparison_media.variant_for(item, 'mp4-card')
    poster = comparison_media.variant_for(item, 'poster')
    if mp4.status != 'ok' or poster.status != 'ok':
        raise ValueError(f'{item.id}: verified product exceeds its media budget')
    changed = 0
    for variant, target in (
        (mp4, staging_root / item.lane / f'{item.id}.mp4'),
        (poster, staging_root / item.lane / f'{item.id}.poster.webp'),
    ):
        source = ROOT / variant.path
        changed += atomic_copy_if_changed(source, target)
    url = f'/assets/gallery/v0.4/{item.lane}/{item.id}.mp4'
    return url, changed


def hydrate(args: argparse.Namespace, lease: DirectoryLease) -> HydrationResult:
    """Run one failure-isolated, atomic local hydration batch."""
    media_args = comparison_args(args)
    jobs = gallery_workers.parse_jobs(args.jobs)
    capture_jobs = gallery_workers.parse_jobs(args.capture_jobs)
    manifest = gallery_media.load_manifest(args.manifest)
    expected_keys = sorted(gallery_media.video_preview_keys(manifest))
    previews = comparison_media.selected_previews(media_args)
    selected_keys = {(str(preview.lane), str(preview.id)) for preview in previews}
    manifest_entries = {
        gallery_media.entry_key(entry): entry for entry in manifest.get('examples', [])
    }

    prepared_items = gallery_workers.bounded_parallel_map(
        previews,
        lambda preview: prepare_preview(preview, media_args, manifest_entries),
        capture_jobs,
        lambda preview: str(preview.id),
    )
    failures = [item for item in prepared_items if isinstance(item, FailedPreview)]
    failures.extend(
        FailedPreview(lane, example_id, 'manifest video has no capturable animated preview')
        for lane, example_id in expected_keys
        if (lane, example_id) not in selected_keys
    )
    prepared = [item for item in prepared_items if isinstance(item, PreparedPreview)]
    lease.refresh()

    products: list[ProductOutcome | FailedPreview] = []
    if prepared:
        try:
            encoder_fingerprint = comparison_media.toolchain_fingerprint()
        except Exception as exc:
            products.extend(preview_failure(item.preview, exc) for item in prepared)
        else:
            products = gallery_workers.bounded_parallel_map(
                prepared,
                lambda item: ensure_product(item, media_args, encoder_fingerprint),
                jobs,
                lambda item: str(item.preview.id),
            )
    failures.extend(item for item in products if isinstance(item, FailedPreview))
    successful = [item for item in products if isinstance(item, ProductOutcome)]
    lease.refresh()

    available: list[str] = []
    materialized = 0
    for product in successful:
        try:
            url, changed_files = stage_product(product.comparison, args.staging_root)
        except Exception as exc:
            failures.append(
                FailedPreview(
                    product.comparison.lane,
                    product.comparison.id,
                    f'{type(exc).__name__}: {exc}',
                )
            )
            continue
        available.append(url)
        materialized += changed_files

    availability_manifest = (
        args.availability_manifest
        if args.availability_manifest is not None
        else args.staging_root / 'local-video-assets.json'
    )
    payload: dict[str, object] = {
        'available': sorted(available),
        'expected': len(expected_keys),
    }
    if failures:
        payload['failed'] = sorted({failure.label for failure in failures})
    manifest_changed = atomic_json_if_changed(availability_manifest, payload)
    changed = materialized > 0 or manifest_changed
    if changed:
        args.watched_sentinel.parent.mkdir(parents=True, exist_ok=True)
        args.watched_sentinel.touch()

    return HydrationResult(
        expected=len(expected_keys),
        available=tuple(sorted(available)),
        cache_hits=sum(item.cached for item in successful),
        generated=sum(not item.cached for item in successful),
        materialized=materialized,
        failures=tuple(sorted(failures, key=lambda failure: failure.label)),
        changed=changed,
    )


def main() -> int:
    """Coordinate one local hydration worker and report optional failures."""
    args = parse_args()
    if args.stale_lock_seconds < 0:
        print('ERROR: --stale-lock-seconds must be non-negative', file=sys.stderr)
        return 2
    try:
        with DirectoryLease(args.lock_dir, args.stale_lock_seconds) as lease:
            result = hydrate(args, lease)
    except LockUnavailableError as exc:
        print(f'Local gallery video hydration skipped: {exc}')
        return 0
    except Exception as exc:
        print(f'ERROR: local gallery video hydration failed: {exc}', file=sys.stderr)
        return 1

    for failure in result.failures:
        print(f'WARNING: {failure.label}: {failure.error}', file=sys.stderr)
    print(
        'Local gallery video hydration: '
        f'available={len(result.available)}/{result.expected} '
        f'cache_hits={result.cache_hits} generated={result.generated} '
        f'materialized={result.materialized} failures={len(result.failures)} '
        f'changed={str(result.changed).lower()}'
    )
    return 1 if args.strict and result.failures else 0


if __name__ == '__main__':
    raise SystemExit(main())
