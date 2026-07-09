#!/usr/bin/env python3
"""Generate build-local animated WebP previews for gallery examples."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path

import gallery_media


ROOT = gallery_media.ROOT
DEFAULT_FRAME_DIR = ROOT / "build/gallery-frames/v0.4"
DEFAULT_OUTPUT_DIR = ROOT / "build/gallery-webp/v0.4"
DEFAULT_BUILD_EXAMPLES_DIR = ROOT / "build/examples/c"
DEFAULT_CACHE_DIR = ROOT / "build/gallery-cache/animations"
DEFAULT_SIZE = "1280x720"
DEFAULT_FRAMES = 16
DEFAULT_FPS = 12
DEFAULT_QUALITY = 90


@dataclass(frozen=True)
class AnimatedPreview:
    id: str
    title: str
    lane: str
    source: str
    executable: Path
    frames: int
    fps: int
    sample_stride: int
    time_scale: float
    size: str
    timeline_spec: str = ""
    motion_type: str = ""
    motion_target: str = ""
    motion_axis: str = ""
    motion_cycles: float = 1.0
    motion_phase: str = ""

    @property
    def frame_dir(self) -> Path:
        return DEFAULT_FRAME_DIR / self.lane / self.id

    @property
    def output_path(self) -> Path:
        return DEFAULT_OUTPUT_DIR / self.lane / f"{self.id}.webp"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=gallery_media.DEFAULT_MANIFEST)
    parser.add_argument("--build-examples-dir", type=Path, default=DEFAULT_BUILD_EXAMPLES_DIR)
    parser.add_argument("--frame-dir", type=Path, default=DEFAULT_FRAME_DIR)
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR)
    parser.add_argument("--cache-dir", type=Path, default=DEFAULT_CACHE_DIR)
    parser.add_argument("--id", action="append", default=[], help="example id; repeat or comma-separate")
    parser.add_argument("--lane", action="append", default=[], help="gallery lane")
    parser.add_argument("--dry-run", action="store_true", help="list work without rendering frames")
    parser.add_argument("--keep-frames", action="store_true", help="keep temporary PNG frame sequences")
    parser.add_argument("--quality", type=int, default=DEFAULT_QUALITY, help="lossy WebP quality")
    parser.add_argument("--force", action="store_true", help="regenerate even when the cache is current")
    return parser.parse_args()


def split_values(values: list[str]) -> set[str]:
    return gallery_media.split_values(values)


def collect_previews(
    manifest_path: Path,
    build_examples_dir: Path,
    ids: set[str],
    lanes: set[str],
) -> list[AnimatedPreview]:
    manifest = gallery_media.load_manifest(manifest_path)
    previews: list[AnimatedPreview] = []
    for entry in manifest.get("examples", []):
        id_ = str(entry.get("id", ""))
        source = str(entry.get("source", ""))
        lane = gallery_media.lane_for_entry(entry)
        if not id_ or not source.startswith("examples/c/"):
            continue
        if lane not in gallery_media.MEDIA_LANES:
            continue
        if ids and id_ not in ids:
            continue
        if lanes and lane not in lanes:
            continue

        preview = gallery_media.preview_metadata(entry)
        if preview.get("kind") != gallery_media.ANIMATED_WEBP_KIND:
            continue

        frames = int(preview.get("frames", DEFAULT_FRAMES))
        fps = int(preview.get("fps", DEFAULT_FPS))
        sample_stride = int(preview.get("sample_stride", 1))
        time_scale = float(preview.get("time_scale", 1.0))
        size = str(preview.get("size", entry.get("capture", {}).get("size", DEFAULT_SIZE)))
        if frames <= 0 or fps <= 0:
            raise ValueError(f"{id_}: media.preview frames and fps must be positive")
        if sample_stride <= 0:
            raise ValueError(f"{id_}: media.preview sample_stride must be positive")
        if time_scale <= 0.0:
            raise ValueError(f"{id_}: media.preview time_scale must be positive")

        timeline_spec = ""
        timeline = preview.get("timeline", {})
        if timeline is None:
            timeline = {}
        if not isinstance(timeline, dict):
            raise ValueError(f"{id_}: media.preview.timeline must be a mapping")
        segments = timeline.get("segments", [])
        if segments is None:
            segments = []
        if not isinstance(segments, list):
            raise ValueError(f"{id_}: media.preview.timeline.segments must be a list")
        if segments:
            timeline_parts = []
            timeline_frames = 0
            for segment in segments:
                if not isinstance(segment, dict):
                    raise ValueError(f"{id_}: timeline segments must be mappings")
                segment_id = str(segment.get("id", ""))
                segment_kind = str(segment.get("kind", "state"))
                segment_frames = int(segment.get("frames", 0))
                if not segment_id or ":" in segment_id or "," in segment_id:
                    raise ValueError(f"{id_}: timeline segment id is invalid")
                if not segment_kind or ":" in segment_kind or "," in segment_kind:
                    raise ValueError(f"{id_}: timeline segment kind is invalid")
                if segment_frames <= 0:
                    raise ValueError(f"{id_}: timeline segment frames must be positive")
                timeline_parts.append(f"{segment_id}:{segment_kind}:{segment_frames}")
                timeline_frames += segment_frames
            if timeline_frames != frames:
                raise ValueError(
                    f"{id_}: timeline segment frames ({timeline_frames}) must equal frames ({frames})"
                )
            timeline_spec = ",".join(timeline_parts)

        motion_items = entry.get("motion", {}).get("preview", [])
        if isinstance(motion_items, dict):
            motion_items = [motion_items]
        motion = motion_items[0] if motion_items else {}
        if not isinstance(motion, dict):
            raise ValueError(f"{id_}: motion.preview entries must be mappings")
        motion_type = str(motion.get("type", ""))
        motion_target = str(motion.get("target", ""))
        motion_axis = str(motion.get("axis", ""))
        motion_cycles = float(motion.get("cycles", 1.0))
        motion_phase = str(motion.get("phase", ""))
        if motion_type and motion_type != "visual-spin":
            raise ValueError(f"{id_}: unsupported preview motion type {motion_type!r}")
        if motion_type and motion_cycles <= 0.0:
            raise ValueError(f"{id_}: preview motion cycles must be positive")

        previews.append(
            AnimatedPreview(
                id=id_,
                title=str(entry.get("title", id_)),
                lane=lane,
                source=source,
                executable=gallery_media.source_executable_path(source, build_examples_dir),
                frames=frames,
                fps=fps,
                sample_stride=sample_stride,
                time_scale=time_scale,
                size=size,
                timeline_spec=timeline_spec,
                motion_type=motion_type,
                motion_target=motion_target,
                motion_axis=motion_axis,
                motion_cycles=motion_cycles,
                motion_phase=motion_phase,
            )
        )
    previews.sort(key=lambda item: (item.lane, item.id))
    return previews


def frame_path(frame_dir: Path, frame: int) -> Path:
    return frame_dir / f"frame_{frame:04d}.png"


def child_path(path: Path) -> str:
    try:
        return str(path.relative_to(ROOT))
    except ValueError:
        return str(path)


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


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
    manifest_path: Path,
    frame_root: Path,
    output_root: Path,
    quality: int,
) -> str:
    digest = hashlib.sha256()
    fields = (
        preview.id,
        preview.lane,
        preview.source,
        str(preview.executable.relative_to(ROOT))
        if preview.executable.is_relative_to(ROOT)
        else str(preview.executable),
        str(frame_root.relative_to(ROOT)) if frame_root.is_relative_to(ROOT) else str(frame_root),
        str(output_root.relative_to(ROOT)) if output_root.is_relative_to(ROOT) else str(output_root),
        str(preview.frames),
        str(preview.fps),
        str(preview.sample_stride),
        f"{preview.time_scale:g}",
        preview.size,
        preview.timeline_spec,
        preview.motion_type,
        preview.motion_target,
        preview.motion_axis,
        f"{preview.motion_cycles:g}",
        preview.motion_phase,
        str(quality),
    )
    for field in fields:
        digest.update(field.encode("utf8"))
        digest.update(b"\0")

    for path in (
        manifest_path,
        ROOT / preview.source,
        preview.executable,
        ROOT / "tools/build_gallery_animations.py",
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


def capture_sequence(preview: AnimatedPreview, frame_dir: Path) -> None:
    frame_dir.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env["DVZ_CAPTURE_DIR"] = child_path(frame_dir)
    env["DVZ_CAPTURE_BASENAME"] = "frame"
    cmd = [
        str(preview.executable),
        "--preview",
        "--preview-sequence",
        "--preview-frames",
        str(preview.frames),
        "--preview-fps",
        str(preview.fps),
        "--preview-sample-stride",
        str(preview.sample_stride),
        "--preview-time-scale",
        f"{preview.time_scale:g}",
        "--png",
        "--size",
        preview.size,
    ]
    if preview.timeline_spec:
        cmd.extend(["--preview-timeline", preview.timeline_spec])
    if preview.motion_type:
        cmd.extend(["--preview-motion", preview.motion_type])
        if preview.motion_target:
            cmd.extend(["--preview-motion-target", preview.motion_target])
        if preview.motion_axis:
            cmd.extend(["--preview-motion-axis", preview.motion_axis])
        cmd.extend(["--preview-motion-cycles", f"{preview.motion_cycles:g}"])
        if preview.motion_phase:
            cmd.extend(["--preview-motion-phase", preview.motion_phase])
    subprocess.run(cmd, cwd=ROOT, env=env, check=True)


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
    subprocess.run(cmd, cwd=ROOT, check=True)


def generate_preview(
    preview: AnimatedPreview,
    manifest_path: Path,
    frame_root: Path,
    output_root: Path,
    cache_dir: Path,
    dry_run: bool,
    keep_frames: bool,
    quality: int,
    force: bool,
) -> bool:
    frame_dir = frame_root / preview.lane / preview.id
    output_path = output_root / preview.lane / f"{preview.id}.webp"
    rel_out = output_path.relative_to(ROOT) if output_path.is_relative_to(ROOT) else output_path
    input_hash = input_hash_for(preview, manifest_path, frame_root, output_root, quality)
    cache_hit, cache_reason = current_cache_hit(preview, output_path, cache_dir, input_hash)
    if cache_hit and not force:
        print(f"cached animated webp: {preview.id} -> {rel_out}")
        return False
    if dry_run:
        action = "would replace" if output_path.exists() else "would animate"
        if not force and output_path.exists() and not cache_hit:
            action = f"{action} ({cache_reason})"
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
        print(
            f"{action}: {preview.id} frames={preview.frames} fps={preview.fps} "
            f"sample_stride={preview.sample_stride} time_scale={preview.time_scale:g} "
            f"size={preview.size}{timeline}{motion} -> {rel_out}"
        )
        return True
    if not preview.executable.exists():
        raise FileNotFoundError(f"example executable not found: {preview.executable}")

    if frame_dir.exists():
        shutil.rmtree(frame_dir)
    capture_sequence(preview, frame_dir)
    encode_webp(preview, frame_dir, output_path, quality)
    write_cache(
        cache_path(preview, cache_dir),
        {
            "input_hash": input_hash,
            "webp_hash": file_sha256(output_path),
            "path": child_path(output_path),
        },
    )
    if not keep_frames:
        shutil.rmtree(frame_dir)
    print(f"animated webp: {preview.id} -> {rel_out}")
    return True


def main() -> int:
    args = parse_args()
    ids = split_values(args.id)
    lanes = split_values(args.lane)
    try:
        previews = collect_previews(args.manifest, args.build_examples_dir, ids, lanes)
        if not 0 <= args.quality <= 100:
            raise ValueError("--quality must be between 0 and 100")
        if not previews:
            print("No matching animated WebP previews.")
            return 1

        generated = 0
        for preview in previews:
            if generate_preview(
                preview,
                args.manifest,
                args.frame_dir,
                args.output_dir,
                args.cache_dir,
                args.dry_run,
                args.keep_frames,
                args.quality,
                args.force,
            ):
                generated += 1
        action = "would_generate" if args.dry_run else "generated"
        print(f"gallery animations: {action}={generated} selected={len(previews)}")
        return 0
    except (FileNotFoundError, RuntimeError, ValueError, subprocess.CalledProcessError) as e:
        print(f"gallery animations: {e}")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
