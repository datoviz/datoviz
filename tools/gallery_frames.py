#!/usr/bin/env python3
"""Shared cached PNG frame sequences for gallery preview media."""

from __future__ import annotations

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
DEFAULT_BUILD_EXAMPLES_DIR = ROOT / "build/examples/c"
DEFAULT_CACHE_DIR = ROOT / "build/gallery-cache/frames"
DEFAULT_SIZE = gallery_media.CANONICAL_ANIMATION_SIZE
DEFAULT_FRAMES = 16
DEFAULT_FPS = 12


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
    webp_quality: int = 0
    timeline_spec: str = ""
    motion_type: str = ""
    motion_target: str = ""
    motion_axis: str = ""
    motion_cycles: float = 1.0
    motion_phase: str = ""
    manifest_input_hash: str = ""
    dataset_input_hash: str = ""

    @property
    def frame_dir(self) -> Path:
        return DEFAULT_FRAME_DIR / self.lane / self.id


@dataclass(frozen=True)
class FrameSequence:
    preview: AnimatedPreview
    frame_dir: Path
    input_hash: str
    frames_hash: str
    generated: bool
    reason: str


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
        webp_quality = int(preview.get("webp_quality", 0))
        if frames <= 0 or fps <= 0:
            raise ValueError(f"{id_}: media.preview frames and fps must be positive")
        if sample_stride <= 0:
            raise ValueError(f"{id_}: media.preview sample_stride must be positive")
        if time_scale <= 0.0:
            raise ValueError(f"{id_}: media.preview time_scale must be positive")
        if not 0 <= webp_quality <= 100:
            raise ValueError(f"{id_}: media.preview.webp_quality must be between 0 and 100")

        timeline_spec = _timeline_spec(id_, preview)
        motion = _preview_motion(id_, entry)

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
                webp_quality=webp_quality,
                timeline_spec=timeline_spec,
                motion_type=motion["type"],
                motion_target=motion["target"],
                motion_axis=motion["axis"],
                motion_cycles=motion["cycles"],
                motion_phase=motion["phase"],
                manifest_input_hash=capture_manifest_hash(entry),
                dataset_input_hash=gallery_media.prepared_dataset_fingerprint(entry),
            )
        )
    previews.sort(key=lambda item: (item.lane, item.id))
    return previews


def capture_manifest_hash(entry: dict) -> str:
    """Hash only manifest fields that can affect one preview capture."""
    motion = entry.get("motion") or {}
    payload = {
        "id": entry.get("id"),
        "source": entry.get("source"),
        "capture": entry.get("capture") or {},
        "preview": gallery_media.preview_metadata(entry),
        "preview_motion": motion.get("preview") or [],
    }
    return hashlib.sha256(
        json.dumps(payload, sort_keys=True, separators=(",", ":")).encode("utf8")
    ).hexdigest()


def _timeline_spec(id_: str, preview: dict) -> str:
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
    if not segments:
        return ""

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
    if timeline_frames != int(preview.get("frames", DEFAULT_FRAMES)):
        raise ValueError(
            f"{id_}: timeline segment frames ({timeline_frames}) must equal frames "
            f"({preview.get('frames', DEFAULT_FRAMES)})"
        )
    return ",".join(timeline_parts)


def _preview_motion(id_: str, entry: dict) -> dict:
    motion_items = entry.get("motion", {}).get("preview", [])
    if isinstance(motion_items, dict):
        motion_items = [motion_items]
    motion = motion_items[0] if motion_items else {}
    if not isinstance(motion, dict):
        raise ValueError(f"{id_}: motion.preview entries must be mappings")

    motion_type = str(motion.get("type", ""))
    motion_cycles = float(motion.get("cycles", 1.0))
    if motion_type and motion_type != "visual-spin":
        raise ValueError(f"{id_}: unsupported preview motion type {motion_type!r}")
    if motion_type and motion_cycles <= 0.0:
        raise ValueError(f"{id_}: preview motion cycles must be positive")
    return {
        "type": motion_type,
        "target": str(motion.get("target", "")),
        "axis": str(motion.get("axis", "")),
        "cycles": motion_cycles,
        "phase": str(motion.get("phase", "")),
    }


def frame_path(frame_dir: Path, frame: int) -> Path:
    return frame_dir / f"frame_{frame:04d}.png"


def frame_paths(preview: AnimatedPreview, frame_dir: Path) -> list[Path]:
    return [frame_path(frame_dir, frame) for frame in range(preview.frames)]


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


def frames_sha256(preview: AnimatedPreview, frame_dir: Path) -> str:
    digest = hashlib.sha256()
    for path in frame_paths(preview, frame_dir):
        if not path.exists():
            raise FileNotFoundError(path)
        digest.update(path.name.encode("utf8"))
        digest.update(b"\0")
        with path.open("rb") as f:
            for chunk in iter(lambda: f.read(1024 * 1024), b""):
                digest.update(chunk)
        digest.update(b"\0")
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
    _manifest_path: Path,
    frame_root: Path,
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
        str(preview.frames),
        str(preview.fps),
        str(preview.sample_stride),
        f"{preview.time_scale:g}",
        preview.size,
        str(preview.webp_quality),
        preview.timeline_spec,
        preview.motion_type,
        preview.motion_target,
        preview.motion_axis,
        f"{preview.motion_cycles:g}",
        preview.motion_phase,
        preview.manifest_input_hash,
        preview.dataset_input_hash,
    )
    for field in fields:
        digest.update(field.encode("utf8"))
        digest.update(b"\0")

    for path in (
        ROOT / preview.source,
        preview.executable,
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
    frame_dir: Path,
    cache_dir: Path,
    input_hash: str,
) -> tuple[bool, str, str]:
    payload = load_cache(cache_path(preview, cache_dir))
    if payload.get("input_hash") != input_hash:
        return False, "stale inputs", ""
    try:
        frames_hash = frames_sha256(preview, frame_dir)
    except OSError:
        return False, "missing frames", ""
    if payload.get("frames_hash") != frames_hash:
        return False, "changed outside cache", frames_hash
    return True, "current", frames_hash


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


def ensure_frames(
    preview: AnimatedPreview,
    manifest_path: Path,
    frame_root: Path = DEFAULT_FRAME_DIR,
    cache_dir: Path = DEFAULT_CACHE_DIR,
    force: bool = False,
) -> FrameSequence:
    frame_dir = frame_root / preview.lane / preview.id
    input_hash = input_hash_for(preview, manifest_path, frame_root)
    cache_hit, reason, frames_hash = current_cache_hit(preview, frame_dir, cache_dir, input_hash)
    if cache_hit and not force:
        return FrameSequence(preview, frame_dir, input_hash, frames_hash, False, reason)
    if not preview.executable.exists():
        raise FileNotFoundError(f"example executable not found: {preview.executable}")

    if frame_dir.exists():
        shutil.rmtree(frame_dir)
    capture_sequence(preview, frame_dir)
    frames_hash = frames_sha256(preview, frame_dir)
    write_cache(
        cache_path(preview, cache_dir),
        {
            "input_hash": input_hash,
            "frames_hash": frames_hash,
            "frame_dir": child_path(frame_dir),
            "frames": preview.frames,
            "fps": preview.fps,
            "size": preview.size,
        },
    )
    return FrameSequence(preview, frame_dir, input_hash, frames_hash, True, reason)
