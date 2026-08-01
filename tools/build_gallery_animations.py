#!/usr/bin/env python3
"""Generate build-local animated WebP previews for gallery examples."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path

import gallery_frames
import gallery_media
import gallery_workers


ROOT = gallery_media.ROOT
DEFAULT_FRAME_DIR = gallery_frames.DEFAULT_FRAME_DIR
DEFAULT_OUTPUT_DIR = ROOT / "build/gallery-webp/v0.4"
DEFAULT_BUILD_EXAMPLES_DIR = gallery_frames.DEFAULT_BUILD_EXAMPLES_DIR
DEFAULT_FRAME_CACHE_DIR = gallery_frames.DEFAULT_CACHE_DIR
DEFAULT_CACHE_DIR = ROOT / "build/gallery-cache/animations"
DEFAULT_QUALITY = 90

AnimatedPreview = gallery_frames.AnimatedPreview
collect_previews = gallery_frames.collect_previews
frame_path = gallery_frames.frame_path
capture_sequence = gallery_frames.capture_sequence


@dataclass(frozen=True)
class AnimationResult:
    generated: bool
    messages: tuple[str, ...]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=gallery_media.DEFAULT_MANIFEST)
    parser.add_argument("--build-examples-dir", type=Path, default=DEFAULT_BUILD_EXAMPLES_DIR)
    parser.add_argument("--frame-dir", type=Path, default=DEFAULT_FRAME_DIR)
    parser.add_argument("--frame-cache-dir", type=Path, default=DEFAULT_FRAME_CACHE_DIR)
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR)
    parser.add_argument("--cache-dir", type=Path, default=DEFAULT_CACHE_DIR)
    parser.add_argument("--id", action="append", default=[], help="example id; repeat or comma-separate")
    parser.add_argument("--lane", action="append", default=[], help="gallery lane")
    parser.add_argument("--dry-run", action="store_true", help="list work without rendering frames")
    parser.add_argument(
        "--keep-frames",
        action="store_true",
        help="deprecated; PNG preview frames are always cached in build/gallery-frames",
    )
    parser.add_argument("--quality", type=int, default=DEFAULT_QUALITY, help="lossy WebP quality")
    parser.add_argument("--force", action="store_true", help="regenerate even when the cache is current")
    parser.add_argument(
        "--jobs",
        default="auto",
        help="parallel encoding jobs, 'auto' for a CPU-bounded default, or 1 for serial",
    )
    parser.add_argument(
        "--capture-jobs",
        default="1" if sys.platform == "darwin" else "auto",
        help="parallel capture jobs; defaults to 1 on macOS and CPU-bounded elsewhere",
    )
    parser.add_argument(
        "--include-video-previews",
        action="store_true",
        help="also build animated WebPs for entries whose public card is MP4-owned",
    )
    return parser.parse_args()


def split_values(values: list[str]) -> set[str]:
    return gallery_media.split_values(values)


def select_owned_previews(
    previews: list[AnimatedPreview],
    manifest: dict,
    explicit_ids: set[str],
    include_video_previews: bool,
) -> list[AnimatedPreview]:
    """Select WebP-owned previews while honoring explicit comparison requests."""
    if include_video_previews or explicit_ids:
        return previews
    video_keys = gallery_media.video_preview_keys(manifest)
    return [preview for preview in previews if (preview.lane, preview.id) not in video_keys]


def child_path(path: Path) -> str:
    return gallery_frames.child_path(path)


def file_sha256(path: Path) -> str:
    return gallery_frames.file_sha256(path)


def _hash_file(digest: "hashlib._Hash", path: Path) -> None:
    try:
        resolved = path.resolve()
        rel = resolved.relative_to(ROOT)
        data = resolved.read_bytes()
    except (OSError, ValueError):
        return
    digest.update(str(rel).encode("utf8"))
    digest.update(b"\0")
    digest.update(data)
    digest.update(b"\0")


def input_hash_for(
    preview: AnimatedPreview,
    frames: gallery_frames.FrameSequence,
    output_root: Path,
    quality: int,
) -> str:
    digest = hashlib.sha256()
    fields = (
        preview.id,
        preview.lane,
        frames.input_hash,
        frames.frames_hash,
        str(output_root.relative_to(ROOT)) if output_root.is_relative_to(ROOT) else str(output_root),
        str(quality),
    )
    for field in fields:
        digest.update(field.encode("utf8"))
        digest.update(b"\0")

    for path in (
        ROOT / "tools/build_gallery_animations.py",
        ROOT / "tools/gallery_frames.py",
        ROOT / "tools/gallery_media.py",
    ):
        _hash_file(digest, path)
    return digest.hexdigest()


def cache_path(preview: AnimatedPreview, cache_dir: Path) -> Path:
    return cache_dir / preview.lane / f"{preview.id}.json"


def load_cache(path: Path) -> dict:
    try:
        with path.open("r", encoding="utf8") as f:
            payload = json.load(f)
    except (OSError, json.JSONDecodeError):
        return {}
    return payload if isinstance(payload, dict) else {}


def write_cache(path: Path, payload: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf8")


def current_cache_hit(
    preview: AnimatedPreview,
    output_path: Path,
    cache_dir: Path,
    input_hash: str,
) -> tuple[bool, str]:
    payload = load_cache(cache_path(preview, cache_dir))
    if not output_path.exists():
        return False, "missing output"
    if payload.get("input_hash") != input_hash:
        return False, "stale inputs"
    try:
        output_hash = file_sha256(output_path)
    except OSError:
        return False, "missing output"
    if payload.get("webp_hash") != output_hash:
        return False, "changed outside cache"
    return True, "current"


def encode_webp(
    preview: AnimatedPreview, frame_dir: Path, output_path: Path, quality: int) -> None:
    img2webp = shutil.which("img2webp")
    if img2webp is None:
        raise RuntimeError("img2webp not found; install the WebP tools package")

    output_path.parent.mkdir(parents=True, exist_ok=True)
    delay_ms = max(1, int(round(1000.0 / preview.fps)))
    cmd = [img2webp, "-loop", "0"]
    for frame in range(preview.frames):
        png = frame_path(frame_dir, frame)
        if not png.exists():
            raise FileNotFoundError(png)
        cmd.extend(["-d", str(delay_ms), "-lossy", "-q", str(quality), str(png)])
    cmd.extend(["-o", str(output_path)])
    subprocess.run(cmd, cwd=ROOT, check=True, capture_output=True)


def dry_run_preview(
    preview: AnimatedPreview,
    manifest_path: Path,
    frame_root: Path,
    frame_cache_dir: Path,
    output_path: Path,
    force: bool,
) -> None:
    frame_input_hash = gallery_frames.input_hash_for(preview, manifest_path, frame_root)
    frame_hit, frame_reason, _ = gallery_frames.current_cache_hit(
        preview, frame_root / preview.lane / preview.id, frame_cache_dir, frame_input_hash
    )
    action = "would replace" if output_path.exists() else "would animate"
    if force:
        action = "would recapture frames and replace"
    elif not frame_hit:
        action = f"{action} (frames: {frame_reason})"
    motion = ""
    if preview.motion_type:
        motion = (
            f" motion={preview.motion_type}"
            f" target={preview.motion_target or 'auto'}"
            f" axis={preview.motion_axis or 'default'}"
            f" cycles={preview.motion_cycles:g}"
            f" phase={preview.motion_phase or 'default'}"
        )
    timeline = f" timeline={preview.timeline_spec}" if preview.timeline_spec else ""
    rel_out = output_path.relative_to(ROOT) if output_path.is_relative_to(ROOT) else output_path
    print(
        f"{action}: {preview.id} frames={preview.frames} fps={preview.fps} "
        f"sample_stride={preview.sample_stride} time_scale={preview.time_scale:g} "
        f"size={preview.size}{timeline}{motion} -> {rel_out}"
    )


def encode_preview(
    preview: AnimatedPreview,
    frames: gallery_frames.FrameSequence,
    output_root: Path,
    cache_dir: Path,
    quality: int,
    force: bool,
) -> AnimationResult:
    quality = preview.webp_quality or quality
    output_path = output_root / preview.lane / f"{preview.id}.webp"
    rel_out = output_path.relative_to(ROOT) if output_path.is_relative_to(ROOT) else output_path
    input_hash = input_hash_for(preview, frames, output_root, quality)
    cache_hit, cache_reason = current_cache_hit(preview, output_path, cache_dir, input_hash)
    if cache_hit and not force:
        return AnimationResult(False, (f"cached animated webp: {preview.id} -> {rel_out}",))

    messages = []
    if frames.generated:
        messages.append(f"captured frames: {preview.id} -> {child_path(frames.frame_dir)}")
    elif cache_reason != "current":
        messages.append(f"animated webp cache stale: {preview.id} ({cache_reason})")
    encode_webp(preview, frames.frame_dir, output_path, quality)
    write_cache(
        cache_path(preview, cache_dir),
        {
            "input_hash": input_hash,
            "webp_hash": file_sha256(output_path),
            "path": child_path(output_path),
            "frame_input_hash": frames.input_hash,
            "frames_hash": frames.frames_hash,
        },
    )
    messages.append(f"animated webp: {preview.id} -> {rel_out}")
    return AnimationResult(True, tuple(messages))


def main() -> int:
    started_at = time.perf_counter()
    args = parse_args()
    ids = split_values(args.id)
    lanes = split_values(args.lane)
    try:
        manifest = gallery_media.load_manifest(args.manifest)
        previews = collect_previews(args.manifest, args.build_examples_dir, ids, lanes)
        previews = select_owned_previews(
            previews, manifest, ids, args.include_video_previews
        )
        if not 0 <= args.quality <= 100:
            raise ValueError("--quality must be between 0 and 100")
        if not previews:
            print("No matching animated WebP previews.")
            return 1
        jobs = gallery_workers.parse_jobs(args.jobs)
        capture_jobs = gallery_workers.parse_jobs(args.capture_jobs)

        if args.dry_run:
            for preview in previews:
                output_path = args.output_dir / preview.lane / f"{preview.id}.webp"
                dry_run_preview(
                    preview,
                    args.manifest,
                    args.frame_dir,
                    args.frame_cache_dir,
                    output_path,
                    args.force,
                )
            print(f"gallery animations: would_generate={len(previews)} selected={len(previews)}")
            return 0

        capture_started_at = time.perf_counter()
        frame_sequences = gallery_workers.bounded_parallel_map(
            previews,
            lambda preview: gallery_frames.ensure_frames(
                preview,
                args.manifest,
                args.frame_dir,
                args.frame_cache_dir,
                force=args.force,
            ),
            capture_jobs,
            lambda preview: preview.id,
        )
        capture_seconds = time.perf_counter() - capture_started_at
        frames_by_key = {
            (sequence.preview.lane, sequence.preview.id): sequence
            for sequence in frame_sequences
        }

        encode_started_at = time.perf_counter()
        results = gallery_workers.bounded_parallel_map(
            previews,
            lambda preview: encode_preview(
                preview,
                frames_by_key[(preview.lane, preview.id)],
                args.output_dir,
                args.cache_dir,
                args.quality,
                args.force,
            ),
            jobs,
            lambda preview: preview.id,
        )
        encode_seconds = time.perf_counter() - encode_started_at
        for result in results:
            for message in result.messages:
                print(message)
        generated = sum(result.generated for result in results)
        frame_cache_misses = sum(sequence.generated for sequence in frame_sequences)
        print(
            f"gallery animations: generated={generated} selected={len(previews)} "
            f"jobs={jobs} capture_jobs={capture_jobs}"
        )
        print(
            "gallery animation timing: "
            f"capture={capture_seconds:.3f}s encode={encode_seconds:.3f}s "
            f"total={time.perf_counter() - started_at:.3f}s "
            f"frame_cache_hits={len(frame_sequences) - frame_cache_misses} "
            f"frame_cache_misses={frame_cache_misses} "
            f"animation_cache_hits={len(results) - generated} "
            f"animation_cache_misses={generated}"
        )
        return 0
    except (FileNotFoundError, RuntimeError, ValueError, subprocess.CalledProcessError) as e:
        print(f"gallery animations: {e}")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
