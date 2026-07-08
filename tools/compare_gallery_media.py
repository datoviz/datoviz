#!/usr/bin/env python3
"""Build-local gallery animation media comparison.

This tool compares existing animated WebP previews against smaller card
encodes and video alternatives. It writes only to build-local paths by default;
do not commit the generated media until the gallery media policy is settled.
"""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
from dataclasses import asdict, dataclass
from pathlib import Path
from tempfile import TemporaryDirectory

import build_gallery_animations
import gallery_media


ROOT = gallery_media.ROOT
DEFAULT_INPUT_DIR = ROOT / "build/gallery-webp/v0.4"
DEFAULT_OUTPUT_DIR = ROOT / "build/gallery-media-compare/v0.4"
DEFAULT_SIZE = "640x360"
DEFAULT_STEP = 4
DEFAULT_WEBP_QUALITY = 50
DEFAULT_MP4_CRF = 28
DEFAULT_WEBM_CRF = 34
DEFAULT_REPORT = ROOT / "build/gallery-media-compare/report.json"

DEFAULT_CANDIDATE_IDS = (
    "showcases_point_cloud",
    "showcases_wind_field",
    "showcases_protein",
    "showcases_gpu_particle_smoke",
    "showcases_textured_planet",
    "features_animation_tracks",
    "features_user_scale",
    "showcases_panel_linked_axes",
)

BUDGETS = {
    "animated_webp_card": 1_000_000,
    "video_card": 1_000_000,
    "poster": 500_000,
}


@dataclass(frozen=True)
class MediaVariant:
    kind: str
    path: str
    bytes: int
    budget_bytes: int
    status: str


@dataclass(frozen=True)
class MediaComparison:
    id: str
    lane: str
    title: str
    source_webp: str
    source_bytes: int
    source_frames: int
    source_size: str
    variants: list[MediaVariant]
    html_snippet: str


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=gallery_media.DEFAULT_MANIFEST)
    parser.add_argument("--input-dir", type=Path, default=DEFAULT_INPUT_DIR)
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR)
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT)
    parser.add_argument("--id", action="append", default=[], help="example id; repeat or comma-separate")
    parser.add_argument("--lane", action="append", default=[], help="gallery lane")
    parser.add_argument("--size", default=DEFAULT_SIZE, help="card media size, for example 640x360")
    parser.add_argument("--step", type=int, default=DEFAULT_STEP, help="keep every Nth source frame")
    parser.add_argument("--webp-quality", type=int, default=DEFAULT_WEBP_QUALITY)
    parser.add_argument("--mp4-crf", type=int, default=DEFAULT_MP4_CRF)
    parser.add_argument("--webm-crf", type=int, default=DEFAULT_WEBM_CRF)
    parser.add_argument("--fps", type=int, default=0, help="override output fps; default derives from preview fps")
    parser.add_argument("--webm", action="store_true", help="also encode a VP9 WebM candidate")
    parser.add_argument("--all-animated", action="store_true", help="compare every animated preview")
    parser.add_argument("--dry-run", action="store_true", help="list selected inputs without encoding")
    parser.add_argument("--force", action="store_true", help="replace existing comparison outputs")
    return parser.parse_args()


def child_path(path: Path) -> str:
    try:
        return path.relative_to(ROOT).as_posix()
    except ValueError:
        return path.as_posix()


def split_values(values: list[str]) -> set[str]:
    return gallery_media.split_values(values)


def require_tool(name: str) -> str:
    path = shutil.which(name)
    if path is None:
        raise RuntimeError(f"{name} not found")
    return path


def run(cmd: list[str]) -> None:
    subprocess.run(cmd, cwd=ROOT, check=True)


def source_webp_path(preview: object, input_dir: Path) -> Path:
    return input_dir / getattr(preview, "lane") / f"{getattr(preview, 'id')}.webp"


def output_base(preview: object, output_dir: Path) -> Path:
    return output_dir / getattr(preview, "lane") / getattr(preview, "id")


def output_fps(preview: object, step: int, fps_override: int) -> int:
    if fps_override > 0:
        return fps_override
    return max(1, int(round(getattr(preview, "fps") / step)))


def selected_previews(args: argparse.Namespace) -> list[object]:
    ids = split_values(args.id)
    lanes = split_values(args.lane)
    if not ids and not lanes and not args.all_animated:
        ids = set(DEFAULT_CANDIDATE_IDS)
    selected = []
    for preview in build_gallery_animations.collect_previews(
        args.manifest, ROOT / "build/examples/c", ids, lanes
    ):
        if not args.all_animated and ids and getattr(preview, "id") not in ids:
            continue
        selected.append(preview)
    selected.sort(key=lambda item: (getattr(item, "lane"), getattr(item, "id")))
    return selected


def coalesce_and_resize(source: Path, frame_dir: Path, size: str) -> list[Path]:
    frame_dir.mkdir(parents=True, exist_ok=True)
    pattern = frame_dir / "frame_%04d.png"
    run(["magick", str(source), "-coalesce", "-resize", size, str(pattern)])
    return sorted(frame_dir.glob("frame_*.png"))


def select_frames(frames: list[Path], step: int) -> list[Path]:
    if step <= 0:
        raise ValueError("--step must be positive")
    selected = frames[::step]
    if not selected:
        raise ValueError("no frames selected")
    return selected


def encode_webp(frames: list[Path], output: Path, fps: int, quality: int) -> None:
    img2webp = require_tool("img2webp")
    output.parent.mkdir(parents=True, exist_ok=True)
    delay_ms = max(1, int(round(1000.0 / fps)))
    cmd = [img2webp, "-loop", "0"]
    for frame in frames:
        cmd.extend(["-d", str(delay_ms), "-lossy", "-q", str(quality), str(frame)])
    cmd.extend(["-o", str(output)])
    run(cmd)


def encode_mp4(frame_pattern: str, output: Path, fps: int, crf: int) -> None:
    ffmpeg = require_tool("ffmpeg")
    output.parent.mkdir(parents=True, exist_ok=True)
    run(
        [
            ffmpeg,
            "-hide_banner",
            "-loglevel",
            "error",
            "-y",
            "-framerate",
            str(fps),
            "-i",
            frame_pattern,
            "-an",
            "-vf",
            "format=yuv420p",
            "-c:v",
            "libx264",
            "-preset",
            "slow",
            "-crf",
            str(crf),
            "-movflags",
            "+faststart",
            str(output),
        ]
    )


def encode_webm(frame_pattern: str, output: Path, fps: int, crf: int) -> None:
    ffmpeg = require_tool("ffmpeg")
    output.parent.mkdir(parents=True, exist_ok=True)
    run(
        [
            ffmpeg,
            "-hide_banner",
            "-loglevel",
            "error",
            "-y",
            "-framerate",
            str(fps),
            "-i",
            frame_pattern,
            "-an",
            "-c:v",
            "libvpx-vp9",
            "-b:v",
            "0",
            "-crf",
            str(crf),
            str(output),
        ]
    )


def encode_poster(frame: Path, output: Path, quality: int) -> None:
    cwebp = require_tool("cwebp")
    output.parent.mkdir(parents=True, exist_ok=True)
    run([cwebp, "-quiet", "-q", str(quality), str(frame), "-o", str(output)])


def variant(kind: str, path: Path, budget: int) -> MediaVariant:
    size = path.stat().st_size
    return MediaVariant(
        kind=kind,
        path=child_path(path),
        bytes=size,
        budget_bytes=budget,
        status="ok" if size <= budget else "over-budget",
    )


def html_snippet(preview: object, include_webm: bool) -> str:
    lane = getattr(preview, "lane")
    id_ = getattr(preview, "id")
    title = getattr(preview, "title")
    media_base = f"/assets/gallery/v0.4/{lane}/{id_}"
    sources = []
    if include_webm:
        sources.append(f'    <source src="{media_base}.webm" type="video/webm">')
    sources.append(f'    <source src="{media_base}.mp4" type="video/mp4">')
    return "\n".join(
        [
            f'<a class="dvz-gallery-media" href="gallery/{lane}/{id_}/">',
            "  <video autoplay muted loop playsinline preload=\"metadata\"",
            f'         poster="{media_base}.poster.webp" aria-label="{title}">',
            *sources,
            f'    <img src="{media_base}.webp" alt="{title}">',
            "  </video>",
            "</a>",
        ]
    )


def compare_preview(preview: object, args: argparse.Namespace) -> MediaComparison | None:
    source = source_webp_path(preview, args.input_dir)
    if not source.exists():
        print(f"missing source: {getattr(preview, 'id')} -> {child_path(source)}")
        return None

    base = output_base(preview, args.output_dir)
    animated_webp = base / f"{getattr(preview, 'id')}.card.webp"
    mp4 = base / f"{getattr(preview, 'id')}.card.mp4"
    webm = base / f"{getattr(preview, 'id')}.card.webm"
    poster = base / f"{getattr(preview, 'id')}.poster.webp"
    out_fps = output_fps(preview, args.step, args.fps)

    if args.dry_run:
        print(
            f"would compare: {getattr(preview, 'id')} "
            f"source={child_path(source)} size={args.size} step={args.step} fps={out_fps}"
        )
        return None

    if not args.force and animated_webp.exists() and mp4.exists() and poster.exists():
        variants = [
            variant("animated-webp-card", animated_webp, BUDGETS["animated_webp_card"]),
            variant("mp4-card", mp4, BUDGETS["video_card"]),
            variant("poster", poster, BUDGETS["poster"]),
        ]
        if args.webm and webm.exists():
            variants.append(variant("webm-card", webm, BUDGETS["video_card"]))
        return MediaComparison(
            id=getattr(preview, "id"),
            lane=getattr(preview, "lane"),
            title=getattr(preview, "title"),
            source_webp=child_path(source),
            source_bytes=source.stat().st_size,
            source_frames=getattr(preview, "frames"),
            source_size=getattr(preview, "size"),
            variants=variants,
            html_snippet=html_snippet(preview, args.webm),
        )

    with TemporaryDirectory(prefix="dvz-gallery-media-") as tmp:
        frame_root = Path(tmp)
        frames = coalesce_and_resize(source, frame_root / "frames", args.size)
        selected = select_frames(frames, args.step)
        selected_dir = frame_root / "selected"
        selected_dir.mkdir()
        for index, frame in enumerate(selected):
            shutil.copyfile(frame, selected_dir / f"frame_{index:04d}.png")
        selected_pattern = str(selected_dir / "frame_%04d.png")
        encode_webp(selected, animated_webp, out_fps, args.webp_quality)
        encode_mp4(selected_pattern, mp4, out_fps, args.mp4_crf)
        if args.webm:
            encode_webm(selected_pattern, webm, out_fps, args.webm_crf)
        encode_poster(selected[0], poster, args.webp_quality)

    variants = [
        variant("animated-webp-card", animated_webp, BUDGETS["animated_webp_card"]),
        variant("mp4-card", mp4, BUDGETS["video_card"]),
        variant("poster", poster, BUDGETS["poster"]),
    ]
    if args.webm:
        variants.append(variant("webm-card", webm, BUDGETS["video_card"]))
    return MediaComparison(
        id=getattr(preview, "id"),
        lane=getattr(preview, "lane"),
        title=getattr(preview, "title"),
        source_webp=child_path(source),
        source_bytes=source.stat().st_size,
        source_frames=getattr(preview, "frames"),
        source_size=getattr(preview, "size"),
        variants=variants,
        html_snippet=html_snippet(preview, args.webm),
    )


def print_report(comparisons: list[MediaComparison]) -> None:
    for item in comparisons:
        print(
            f"{item.id}: source={item.source_bytes / 1024:.1f}KB "
            f"frames={item.source_frames} size={item.source_size}"
        )
        for media in item.variants:
            print(
                f"  {media.kind}: {media.bytes / 1024:.1f}KB "
                f"budget={media.budget_bytes / 1024:.1f}KB {media.status}"
            )


def write_report(path: Path, comparisons: list[MediaComparison]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = {
        "budgets": BUDGETS,
        "comparisons": [asdict(item) for item in comparisons],
    }
    path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf8")


def main() -> int:
    args = parse_args()
    try:
        if args.step <= 0:
            raise ValueError("--step must be positive")
        for value, label in (
            (args.webp_quality, "--webp-quality"),
            (args.mp4_crf, "--mp4-crf"),
            (args.webm_crf, "--webm-crf"),
        ):
            if not 0 <= value <= 100:
                raise ValueError(f"{label} must be between 0 and 100")
        require_tool("magick")
        require_tool("img2webp")
        require_tool("cwebp")
        require_tool("ffmpeg")

        previews = selected_previews(args)
        if not previews:
            print("No matching animated previews.")
            return 1

        comparisons = []
        for preview in previews:
            comparison = compare_preview(preview, args)
            if comparison is not None:
                comparisons.append(comparison)
        if args.dry_run:
            print(f"gallery media compare: selected={len(previews)}")
            return 0

        print_report(comparisons)
        write_report(args.report, comparisons)
        over_budget = sum(
            1
            for item in comparisons
            for media in item.variants
            if media.status != "ok"
        )
        print(
            f"gallery media compare: compared={len(comparisons)} "
            f"over_budget={over_budget} report={child_path(args.report)}"
        )
        return 1 if over_budget else 0
    except (OSError, RuntimeError, ValueError, subprocess.CalledProcessError) as exc:
        print(f"gallery media compare: {exc}")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
