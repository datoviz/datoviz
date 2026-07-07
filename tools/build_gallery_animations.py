#!/usr/bin/env python3
"""Generate build-local animated WebP previews for gallery examples."""

from __future__ import annotations

import argparse
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
    parser.add_argument("--id", action="append", default=[], help="example id; repeat or comma-separate")
    parser.add_argument("--lane", action="append", default=[], help="gallery lane")
    parser.add_argument("--dry-run", action="store_true", help="list work without rendering frames")
    parser.add_argument("--keep-frames", action="store_true", help="keep temporary PNG frame sequences")
    parser.add_argument("--quality", type=int, default=DEFAULT_QUALITY, help="lossy WebP quality")
    parser.add_argument("--force", action="store_true", help="accepted for parity with other media tools")
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
            )
        )
    previews.sort(key=lambda item: (item.lane, item.id))
    return previews


def frame_path(frame_dir: Path, frame: int) -> Path:
    return frame_dir / f"frame_{frame:04d}.png"


def capture_sequence(preview: AnimatedPreview, frame_dir: Path) -> None:
    frame_dir.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env["DVZ_CAPTURE_DIR"] = str(frame_dir)
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
    frame_root: Path,
    output_root: Path,
    dry_run: bool,
    keep_frames: bool,
    quality: int,
    force: bool,
) -> bool:
    frame_dir = frame_root / preview.lane / preview.id
    output_path = output_root / preview.lane / f"{preview.id}.webp"
    rel_out = output_path.relative_to(ROOT) if output_path.is_relative_to(ROOT) else output_path
    if dry_run:
        action = "would replace" if output_path.exists() else "would animate"
        print(
            f"{action}: {preview.id} frames={preview.frames} fps={preview.fps} "
            f"sample_stride={preview.sample_stride} time_scale={preview.time_scale:g} "
            f"size={preview.size} -> {rel_out}"
        )
        return True
    if not preview.executable.exists():
        raise FileNotFoundError(f"example executable not found: {preview.executable}")

    if frame_dir.exists():
        shutil.rmtree(frame_dir)
    capture_sequence(preview, frame_dir)
    encode_webp(preview, frame_dir, output_path, quality)
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
                args.frame_dir,
                args.output_dir,
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
