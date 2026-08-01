#!/usr/bin/env python3
"""Build-local gallery animation media comparison.

This tool compares existing animated WebP previews against smaller card
encodes and video alternatives. It writes only to build-local paths by default;
do not commit the generated media until the gallery media policy is settled.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import hashlib
import html
import json
import os
import shutil
import subprocess
import sys
import time
from collections.abc import Callable
from dataclasses import asdict, dataclass
from fractions import Fraction
from pathlib import Path
from tempfile import TemporaryDirectory
from typing import TypeVar, cast

import gallery_frames
import gallery_media


ROOT = gallery_media.ROOT
DEFAULT_INPUT_DIR = ROOT / "build/gallery-webp/v0.4"
DEFAULT_OUTPUT_DIR = ROOT / "build/gallery-media-compare/v0.4"
DEFAULT_SITE_OUTPUT_DIR = ROOT / "build/gallery-webp/v0.4"
DEFAULT_FRAME_DIR = gallery_frames.DEFAULT_FRAME_DIR
DEFAULT_FRAME_CACHE_DIR = gallery_frames.DEFAULT_CACHE_DIR
DEFAULT_MEDIA_CACHE_DIR = ROOT / "build/gallery-cache/cards"
DEFAULT_STEP = 2
DEFAULT_WEBP_QUALITY = 40
DEFAULT_MP4_CRF = 32
DEFAULT_WEBM_CRF = 38
DEFAULT_PREFERRED_KIND = gallery_media.ANIMATED_WEBP_KIND
DEFAULT_REPORT = ROOT / "build/gallery-media-compare/report.json"
DEFAULT_HTML_REPORT = ROOT / "build/gallery-media-compare/index.html"

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
MAX_AUTO_JOBS = 4

T = TypeVar("T")
R = TypeVar("R")


@dataclass(frozen=True)
class EncodingProfile:
    preferred_kind: str
    step: int
    webp_quality: int
    mp4_crf: int
    webm_crf: int
    fps: int = 0
    compare: bool = False


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
    encoded_size: str
    encoded_frames: int
    encoded_fps: int
    preferred_kind: str
    variants: list[MediaVariant]
    webp_html_snippet: str
    video_html_snippet: str
    sample_step: int = 1
    encoded_crf: int = 0
    source_duration: float = 0.0
    encoded_duration: float = 0.0
    notes: tuple[str, ...] = ()


@dataclass(frozen=True)
class FrameItem:
    path: Path
    delay_ms: int


@dataclass(frozen=True)
class Mp4Attempt:
    fps: int
    sample_step: int
    crf: int


@dataclass(frozen=True)
class MediaProbe:
    width: int
    height: int
    fps: float = 0.0
    frames: int = 0
    duration: float = 0.0


@dataclass(frozen=True)
class ComparisonOutcome:
    comparison: MediaComparison
    cached: bool


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=gallery_media.DEFAULT_MANIFEST)
    parser.add_argument("--input-dir", type=Path, default=DEFAULT_INPUT_DIR)
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR)
    parser.add_argument("--site-output-dir", type=Path, default=DEFAULT_SITE_OUTPUT_DIR)
    parser.add_argument("--frame-dir", type=Path, default=DEFAULT_FRAME_DIR)
    parser.add_argument("--frame-cache-dir", type=Path, default=DEFAULT_FRAME_CACHE_DIR)
    parser.add_argument("--cache-dir", type=Path, default=DEFAULT_MEDIA_CACHE_DIR)
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT)
    parser.add_argument("--html-report", type=Path, default=DEFAULT_HTML_REPORT)
    parser.add_argument("--id", action="append", default=[], help="example id; repeat or comma-separate")
    parser.add_argument("--lane", action="append", default=[], help="gallery lane")
    parser.add_argument("--step", type=int, default=DEFAULT_STEP, help="keep every Nth source frame")
    parser.add_argument("--webp-quality", type=int, default=DEFAULT_WEBP_QUALITY)
    parser.add_argument("--mp4-crf", type=int, default=DEFAULT_MP4_CRF)
    parser.add_argument("--webm-crf", type=int, default=DEFAULT_WEBM_CRF)
    parser.add_argument("--fps", type=int, default=0, help="override output fps; default derives from preview fps")
    parser.add_argument("--preferred-kind", default=DEFAULT_PREFERRED_KIND)
    parser.add_argument("--webm", action="store_true", help="also encode a VP9 WebM candidate")
    parser.add_argument("--all-animated", action="store_true", help="compare every animated preview")
    parser.add_argument("--no-manifest-card", action="store_true", help="ignore media.preview.card")
    parser.add_argument("--dry-run", action="store_true", help="list selected inputs without encoding")
    parser.add_argument(
        "--force",
        action="store_true",
        help="bypass the media cache and replace existing comparison outputs",
    )
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
        "--write-site-assets",
        action="store_true",
        help="copy preferred MP4 candidates and posters into the build-local gallery asset tree",
    )
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


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as file:
        for chunk in iter(lambda: file.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def run(cmd: list[str]) -> None:
    subprocess.run(cmd, cwd=ROOT, check=True)


def parse_jobs(value: str) -> int:
    """Parse a worker count with an automatic CPU-bounded default."""
    if value == "auto":
        return max(1, min(MAX_AUTO_JOBS, os.cpu_count() or 1))
    try:
        jobs = int(value)
    except ValueError as exc:
        raise ValueError(f"invalid worker count: {value!r}") from exc
    if jobs < 1:
        raise ValueError("worker count must be at least 1")
    return jobs


def bounded_parallel_map(
    items: list[T],
    worker: Callable[[T], R],
    jobs: int,
    label: Callable[[T], str] = str,
) -> list[R]:
    """Run bounded work, preserve input order, and stop scheduling after failure."""
    if jobs < 1:
        raise ValueError("jobs must be at least 1")
    if jobs == 1 or len(items) <= 1:
        results = []
        for item in items:
            try:
                results.append(worker(item))
            except Exception as exc:
                raise RuntimeError(
                    f"gallery media worker failed: {label(item)}: {exc}"
                ) from exc
        return results

    results: list[R | None] = [None] * len(items)
    failures: list[tuple[int, Exception]] = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as executor:
        next_index = 0
        active: dict[concurrent.futures.Future[R], int] = {}

        def submit_one() -> None:
            nonlocal next_index
            if next_index >= len(items):
                return
            index = next_index
            next_index += 1
            active[executor.submit(worker, items[index])] = index

        for _ in range(min(jobs, len(items))):
            submit_one()

        while active and not failures:
            done, _ = concurrent.futures.wait(
                active, return_when=concurrent.futures.FIRST_COMPLETED
            )
            for future in sorted(done, key=active.__getitem__):
                index = active.pop(future)
                try:
                    results[index] = future.result()
                except Exception as exc:
                    failures.append((index, exc))
            if failures:
                for future in active:
                    future.cancel()
                concurrent.futures.wait(active)
                for future, index in active.items():
                    if future.cancelled():
                        continue
                    try:
                        results[index] = future.result()
                    except Exception as exc:
                        failures.append((index, exc))
                break
            for _ in range(len(done)):
                submit_one()

    if failures:
        details = "; ".join(
            f"{label(items[index])}: {exc}" for index, exc in sorted(failures)
        )
        raise RuntimeError(f"gallery media workers failed: {details}")
    if any(result is None for result in results):
        raise RuntimeError("gallery media workers returned an incomplete output set")
    return [cast(R, result) for result in results]


def media_workspace(example_id: str) -> TemporaryDirectory[str]:
    """Create an isolated workspace for one media worker."""
    safe_id = "".join(character if character.isalnum() else "-" for character in example_id)
    return TemporaryDirectory(prefix=f"dvz-gallery-media-{safe_id}-")


def source_webp_path(preview: object, input_dir: Path) -> Path:
    return input_dir / getattr(preview, "lane") / f"{getattr(preview, 'id')}.webp"


def output_base(preview: object, output_dir: Path) -> Path:
    return output_dir / getattr(preview, "lane") / getattr(preview, "id")


def profile_for(
    preview: object,
    args: argparse.Namespace,
    manifest_entries: dict[tuple[str, str], dict] | None = None,
) -> EncodingProfile:
    fields = {
        "preferred_kind": args.preferred_kind,
        "step": args.step,
        "webp_quality": args.webp_quality,
        "mp4_crf": args.mp4_crf,
        "webm_crf": args.webm_crf,
        "fps": args.fps,
        "compare": False,
    }
    if not args.no_manifest_card:
        preview_key = (getattr(preview, "lane"), getattr(preview, "id"))
        if manifest_entries is None:
            manifest = gallery_media.load_manifest(args.manifest)
            manifest_entries = {
                gallery_media.entry_key(item): item
                for item in manifest.get("examples", [])
            }
        entry = manifest_entries.get(preview_key)
        if entry is None:
            raise ValueError(f"{getattr(preview, 'id')}: missing manifest entry")
        gallery_media.validate_animation_media_policy(entry)
        card = gallery_media.preview_metadata(entry).get("card") or {}
        fields.update(
            {
                "preferred_kind": str(card.get("preferred", fields["preferred_kind"])),
                "step": int(card.get("sample_step", card.get("step", fields["step"]))),
                "webp_quality": int(card.get("webp_quality", fields["webp_quality"])),
                "mp4_crf": int(card.get("mp4_crf", fields["mp4_crf"])),
                "webm_crf": int(card.get("webm_crf", fields["webm_crf"])),
                "fps": int(card.get("fps", fields["fps"])),
                "compare": bool(card.get("compare", fields["compare"])),
            }
        )
    preview_fps = int(getattr(preview, "fps"))
    if fields["preferred_kind"] == "video-mp4" and fields["fps"] > 0:
        expected_source_fps = fields["fps"] * fields["step"]
        if preview_fps != expected_source_fps:
            raise ValueError(
                f"{getattr(preview, 'id')}: preview fps {preview_fps} must equal card fps "
                f"{fields['fps']} * sample_step {fields['step']} to preserve duration"
            )
    return EncodingProfile(**fields)


def toolchain_fingerprint() -> str:
    """Fingerprint encoder identities and gallery implementation inputs once per run."""
    digest = hashlib.sha256()
    for name in ("img2webp", "cwebp", "ffmpeg"):
        executable = require_tool(name)
        version_flag = "-version"
        result = subprocess.run(
            [executable, version_flag],
            cwd=ROOT,
            check=True,
            capture_output=True,
            text=True,
        )
        digest.update(name.encode("utf8"))
        digest.update(b"\0")
        digest.update(result.stdout.encode("utf8"))
        digest.update(result.stderr.encode("utf8"))
        digest.update(b"\0")
    for path in (
        ROOT / "tools/compare_gallery_media.py",
        ROOT / "tools/gallery_frames.py",
        ROOT / "tools/gallery_media.py",
    ):
        digest.update(path.relative_to(ROOT).as_posix().encode("utf8"))
        digest.update(b"\0")
        digest.update(path.read_bytes())
        digest.update(b"\0")
    return digest.hexdigest()


def comparison_input_hash(
    preview: object,
    frame_sequence: gallery_frames.FrameSequence,
    profile: EncodingProfile,
    args: argparse.Namespace,
    encoder_fingerprint: str,
) -> str:
    """Return the content fingerprint for one card/poster output set."""
    payload = {
        "id": getattr(preview, "id"),
        "lane": getattr(preview, "lane"),
        "frame_input_hash": frame_sequence.input_hash,
        "frames_hash": frame_sequence.frames_hash,
        "profile": asdict(profile),
        "generated_kinds": sorted(generated_variant_kinds(profile, args.webm)),
        "output_dir": str(args.output_dir.resolve()),
        "encoder_fingerprint": encoder_fingerprint,
    }
    return hashlib.sha256(
        json.dumps(payload, sort_keys=True, separators=(",", ":")).encode("utf8")
    ).hexdigest()


def comparison_cache_path(preview: object, cache_dir: Path) -> Path:
    return cache_dir / getattr(preview, "lane") / f"{getattr(preview, 'id')}.json"


def comparison_from_dict(item: dict) -> MediaComparison:
    fields = dict(item)
    fields["variants"] = [
        MediaVariant(**variant)
        for variant in fields.get("variants", [])
        if isinstance(variant, dict)
    ]
    if isinstance(fields.get("notes"), list):
        fields["notes"] = tuple(fields["notes"])
    return MediaComparison(**fields)


def current_comparison_cache(
    preview: object,
    cache_dir: Path,
    input_hash: str,
) -> MediaComparison | None:
    path = comparison_cache_path(preview, cache_dir)
    try:
        payload = json.loads(path.read_text(encoding="utf8"))
        if payload.get("input_hash") != input_hash:
            return None
        comparison = comparison_from_dict(payload["comparison"])
        variant_hashes = payload["variant_hashes"]
    except (KeyError, OSError, TypeError, ValueError, json.JSONDecodeError):
        return None
    if set(variant_hashes) != {variant.kind for variant in comparison.variants}:
        return None
    for variant_item in comparison.variants:
        output = ROOT / variant_item.path
        try:
            if output.stat().st_size != variant_item.bytes:
                return None
            if file_sha256(output) != variant_hashes[variant_item.kind]:
                return None
        except OSError:
            return None
    return comparison


def write_comparison_cache(
    preview: object,
    cache_dir: Path,
    input_hash: str,
    comparison: MediaComparison,
) -> None:
    path = comparison_cache_path(preview, cache_dir)
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = {
        "input_hash": input_hash,
        "comparison": asdict(comparison),
        "variant_hashes": {
            variant_item.kind: file_sha256(ROOT / variant_item.path)
            for variant_item in comparison.variants
        },
    }
    path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf8")


def output_fps(preview: object, profile: EncodingProfile) -> int:
    if profile.fps > 0:
        return profile.fps
    return max(1, int(round(getattr(preview, "fps") / profile.step)))


def selected_previews(args: argparse.Namespace) -> list[object]:
    ids = split_values(args.id)
    lanes = split_values(args.lane)
    if not ids and not lanes and not args.all_animated:
        ids = set(DEFAULT_CANDIDATE_IDS)
    selected = []
    for preview in gallery_frames.collect_previews(
        args.manifest, ROOT / "build/examples/c", ids, lanes
    ):
        if not args.all_animated and ids and getattr(preview, "id") not in ids:
            continue
        selected.append(preview)
    selected.sort(key=lambda item: (getattr(item, "lane"), getattr(item, "id")))
    return selected


def cached_frames(preview: object, source_dir: Path) -> list[FrameItem]:
    """Return canonical cached frames without a resize or conversion stage."""
    delay_ms = max(1, int(round(1000.0 / max(1, getattr(preview, "fps")))))
    frames = []
    for index in range(getattr(preview, "frames")):
        source = gallery_frames.frame_path(source_dir, index)
        if not source.exists():
            raise FileNotFoundError(source)
        frames.append(FrameItem(path=source, delay_ms=delay_ms))
    return frames


def select_frames(frames: list[FrameItem], step: int) -> list[FrameItem]:
    if step <= 0:
        raise ValueError("--step must be positive")
    selected = [
        FrameItem(
            path=frames[index].path,
            delay_ms=sum(frame.delay_ms for frame in frames[index : index + step]),
        )
        for index in range(0, len(frames), step)
    ]
    if not selected:
        raise ValueError("no frames selected")
    return selected


def encode_webp(frames: list[FrameItem], output: Path, quality: int) -> None:
    img2webp = require_tool("img2webp")
    output.parent.mkdir(parents=True, exist_ok=True)
    cmd = [img2webp, "-loop", "0"]
    for frame in frames:
        cmd.extend(
            ["-d", str(max(1, frame.delay_ms)), "-lossy", "-q", str(quality), str(frame.path)])
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


def mp4_attempt_plan(source_fps: int, initial_step: int, base_crf: int) -> list[Mp4Attempt]:
    """Plan the deterministic FPS and CRF fallback ladder for one MP4 card."""
    if source_fps <= 0:
        raise ValueError("source fps must be positive")
    if initial_step <= 0 or source_fps % initial_step != 0:
        raise ValueError("source fps must be divisible by the initial sample step")
    if not 0 <= base_crf <= gallery_media.MP4_MAX_CRF:
        raise ValueError(
            f"base MP4 CRF must be between 0 and {gallery_media.MP4_MAX_CRF}"
        )

    initial_fps = source_fps // initial_step
    if initial_fps > 60:
        if initial_fps % 60 != 0:
            raise ValueError("capture rates above 60 fps must be divisible by 60")
        initial_step *= initial_fps // 60
        initial_fps = 60

    attempts = [Mp4Attempt(initial_fps, initial_step, base_crf)]
    fallback_fps = initial_fps
    fallback_step = initial_step
    if initial_fps == 60:
        fallback_fps = 30
        fallback_step = initial_step * 2
        attempts.append(Mp4Attempt(fallback_fps, fallback_step, base_crf))

    crf = base_crf
    while crf < gallery_media.MP4_MAX_CRF:
        crf = min(crf + 4, gallery_media.MP4_MAX_CRF)
        attempt = Mp4Attempt(fallback_fps, fallback_step, crf)
        if attempt != attempts[-1]:
            attempts.append(attempt)
    return attempts


def execute_mp4_attempts(
    attempts: list[Mp4Attempt],
    encode: Callable[[Mp4Attempt], int],
    budget: int = BUDGETS["video_card"],
) -> Mp4Attempt:
    """Execute attempts in order and return the first one within the byte budget."""
    if not attempts:
        raise ValueError("MP4 attempt plan must not be empty")
    for attempt in attempts:
        if encode(attempt) <= budget:
            return attempt
    last = attempts[-1]
    raise RuntimeError(
        f"MP4 remains over the {budget}-byte budget at "
        f"{last.fps} fps and CRF {last.crf}"
    )


def probe_media(path: Path) -> MediaProbe:
    """Read encoded dimensions, timing, and frame metadata with ffprobe."""
    ffprobe = require_tool("ffprobe")
    result = subprocess.run(
        [
            ffprobe,
            "-v",
            "error",
            "-select_streams",
            "v:0",
            "-show_entries",
            "stream=width,height,avg_frame_rate,nb_frames:format=duration",
            "-of",
            "json",
            str(path),
        ],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    payload = json.loads(result.stdout)
    streams = payload.get("streams") or []
    if not streams:
        raise ValueError(f"{path}: no video/image stream")
    stream = streams[0]
    rate = str(stream.get("avg_frame_rate", "0/1"))
    fps = float(Fraction(rate)) if rate not in {"", "0/0"} else 0.0
    frames_raw = stream.get("nb_frames")
    frames = int(frames_raw) if str(frames_raw).isdigit() else 0
    duration = float((payload.get("format") or {}).get("duration") or 0.0)
    width = int(stream.get("width", 0))
    height = int(stream.get("height", 0))
    if path.suffix.lower() == ".webp" and (width <= 0 or height <= 0):
        try:
            from PIL import Image
        except ImportError as exc:
            raise RuntimeError(
                f"{path}: Pillow is required when ffprobe cannot read animated WebP dimensions"
            ) from exc
        with Image.open(path) as image:
            width, height = image.size
    return MediaProbe(
        width=width,
        height=height,
        fps=fps,
        frames=frames,
        duration=duration,
    )


def write_selected_frames(frames: list[FrameItem], selected_dir: Path, step: int) -> int:
    selected = select_frames(frames, step)
    if selected_dir.exists():
        shutil.rmtree(selected_dir)
    selected_dir.mkdir()
    for index, frame in enumerate(selected):
        shutil.copyfile(frame.path, selected_dir / f"frame_{index:04d}.png")
    return len(selected)


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
            "-vf",
            "format=yuv420p",
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


def webp_html_snippet(preview: object) -> str:
    lane = getattr(preview, "lane")
    id_ = getattr(preview, "id")
    title = getattr(preview, "title")
    media_base = f"/assets/gallery/v0.4/{lane}/{id_}"
    return "\n".join(
        [
            f'<a class="dvz-gallery-media" href="gallery/{lane}/{id_}/">',
            f'  <img class="dvz-gallery-poster" src="{media_base}.poster.webp" alt="{title}">',
            f'  <img class="dvz-gallery-animated" data-src="{media_base}.webp" alt="" aria-hidden="true">',
            "</a>",
        ]
    )


def video_html_snippet(preview: object) -> str:
    lane = getattr(preview, "lane")
    id_ = getattr(preview, "id")
    title = getattr(preview, "title")
    media_base = f"/assets/gallery/v0.4/{lane}/{id_}"
    return "\n".join(
        [
            f'<a class="dvz-gallery-media" href="gallery/{lane}/{id_}/">',
            f'  <img class="dvz-gallery-poster" src="{media_base}.poster.webp" alt="{title}">',
            "  <video muted loop playsinline preload=\"none\"",
            f'         poster="{media_base}.poster.webp" aria-label="{title}">',
            f'    <source data-src="{media_base}.mp4" type="video/mp4">',
            "  </video>",
            "</a>",
        ]
    )


def compare_preview(
    preview: object,
    args: argparse.Namespace,
    frame_sequence: gallery_frames.FrameSequence | None = None,
    profile: EncodingProfile | None = None,
) -> MediaComparison | None:
    source = source_webp_path(preview, args.input_dir)
    profile = profile or profile_for(preview, args)
    base = output_base(preview, args.output_dir)
    if args.force and base.exists():
        shutil.rmtree(base)
    animated_webp = base / f"{getattr(preview, 'id')}.card.webp"
    mp4 = base / f"{getattr(preview, 'id')}.card.mp4"
    webm = base / f"{getattr(preview, 'id')}.card.webm"
    poster = base / f"{getattr(preview, 'id')}.poster.webp"
    out_fps = output_fps(preview, profile)
    preferred_fps = out_fps
    encoded_frames = (getattr(preview, "frames") + profile.step - 1) // profile.step
    generated_kinds = generated_variant_kinds(profile, args.webm)

    stale_variants = {
        "animated-webp-card": animated_webp,
        "mp4-card": mp4,
        "webm-card": webm,
    }
    for kind, path in stale_variants.items():
        if kind not in generated_kinds:
            path.unlink(missing_ok=True)

    if args.dry_run:
        variants = ",".join(sorted(generated_kinds | {"poster"}))
        print(
            f"would compare: {getattr(preview, 'id')} "
            f"frames={child_path(args.frame_dir / getattr(preview, 'lane') / getattr(preview, 'id'))} "
            f"size={gallery_media.CANONICAL_ANIMATION_SIZE} "
            f"step={profile.step} fps={out_fps} preferred={profile.preferred_kind} "
            f"compare={profile.compare} variants={variants}"
        )
        return None

    if frame_sequence is None:
        frame_sequence = gallery_frames.ensure_frames(
            preview, args.manifest, args.frame_dir, args.frame_cache_dir, force=args.force
        )

    notes: list[str] = []
    with media_workspace(getattr(preview, "id")) as tmp:
        frame_root = Path(tmp)
        frames = cached_frames(preview, frame_sequence.frame_dir)
        selected = select_frames(frames, profile.step)
        selected_dir = frame_root / "selected"
        write_selected_frames(frames, selected_dir, profile.step)
        selected_pattern = str(selected_dir / "frame_%04d.png")
        selected_step = profile.step
        selected_crf = 0
        if "animated-webp-card" in generated_kinds:
            encode_webp(selected, animated_webp, profile.webp_quality)
        if "mp4-card" in generated_kinds:
            attempts = mp4_attempt_plan(
                int(getattr(preview, "fps")), profile.step, profile.mp4_crf
            )

            def encode_attempt(attempt: Mp4Attempt) -> int:
                nonlocal encoded_frames
                encoded_frames = write_selected_frames(
                    frames, selected_dir, attempt.sample_step
                )
                encode_mp4(selected_pattern, mp4, attempt.fps, attempt.crf)
                return mp4.stat().st_size

            selected_attempt = execute_mp4_attempts(attempts, encode_attempt)
            out_fps = selected_attempt.fps
            selected_step = selected_attempt.sample_step
            selected_crf = selected_attempt.crf
            if selected_attempt != attempts[0]:
                notes.append(
                    f"{getattr(preview, 'id')}: earlier MP4 attempts exceeded the video budget; "
                    f"using {out_fps} fps at CRF {selected_crf}"
                )
        if "webm-card" in generated_kinds:
            encode_webm(selected_pattern, webm, out_fps, profile.webm_crf)
        encode_poster(selected[0].path, poster, profile.webp_quality)
        if out_fps == preferred_fps:
            encoded_frames = len(selected)

    variants = [variant("poster", poster, BUDGETS["poster"])]
    if "animated-webp-card" in generated_kinds:
        variants.append(variant("animated-webp-card", animated_webp, BUDGETS["animated_webp_card"]))
    if "mp4-card" in generated_kinds:
        variants.append(variant("mp4-card", mp4, BUDGETS["video_card"]))
    if "webm-card" in generated_kinds:
        variants.append(variant("webm-card", webm, BUDGETS["video_card"]))
    return MediaComparison(
        id=getattr(preview, "id"),
        lane=getattr(preview, "lane"),
        title=getattr(preview, "title"),
        source_webp=child_path(source),
        source_bytes=source.stat().st_size if source.exists() else 0,
        source_frames=getattr(preview, "frames"),
        source_size=getattr(preview, "size"),
        encoded_size=gallery_media.CANONICAL_ANIMATION_SIZE,
        encoded_frames=encoded_frames,
        encoded_fps=out_fps,
        preferred_kind=profile.preferred_kind,
        variants=variants,
        webp_html_snippet=webp_html_snippet(preview),
        video_html_snippet=video_html_snippet(preview),
        sample_step=selected_step,
        encoded_crf=selected_crf,
        source_duration=getattr(preview, "frames") / getattr(preview, "fps"),
        encoded_duration=encoded_frames / out_fps,
        notes=tuple(notes),
    )


def compare_preview_cached(
    preview: object,
    args: argparse.Namespace,
    frame_sequence: gallery_frames.FrameSequence,
    profile: EncodingProfile,
    encoder_fingerprint: str,
) -> ComparisonOutcome:
    """Return one comparison, reusing verified encoded outputs when possible."""
    input_hash = comparison_input_hash(
        preview, frame_sequence, profile, args, encoder_fingerprint
    )
    if not args.force:
        cached = current_comparison_cache(preview, args.cache_dir, input_hash)
        if cached is not None:
            return ComparisonOutcome(cached, True)
    comparison = compare_preview(preview, args, frame_sequence, profile)
    if comparison is None:
        raise RuntimeError(f"{getattr(preview, 'id')}: comparison produced no result")
    write_comparison_cache(preview, args.cache_dir, input_hash, comparison)
    return ComparisonOutcome(comparison, False)


def print_report(comparisons: list[MediaComparison]) -> None:
    for item in comparisons:
        for note in item.notes:
            print(note)
        print(
            f"{item.id}: source={item.source_bytes / 1024:.1f}KB "
            f"frames={item.source_frames} size={item.source_size} -> "
            f"{item.encoded_frames} frames {item.encoded_fps}fps {item.encoded_size} "
            f"preferred={item.preferred_kind}"
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


def read_report(path: Path) -> list[MediaComparison]:
    if not path.exists():
        return []
    payload = json.loads(path.read_text(encoding="utf8"))
    items = payload.get("comparisons", [])
    if not isinstance(items, list):
        return []
    comparisons = []
    for item in items:
        if not isinstance(item, dict):
            continue
        comparisons.append(comparison_from_dict(item))
    return comparisons


def should_merge_report(args: argparse.Namespace) -> bool:
    return bool(args.id or args.lane) and not args.all_animated


def merge_comparisons(
    existing: list[MediaComparison], updates: list[MediaComparison]) -> list[MediaComparison]:
    merged = {(item.lane, item.id): item for item in existing}
    for item in updates:
        merged[(item.lane, item.id)] = item
    return sorted(merged.values(), key=lambda item: (item.lane, item.id))


def relative_report_url(report: Path, media_path: str) -> str:
    source = ROOT / media_path
    return Path(os.path.relpath(source, report.parent)).as_posix()


def media_element(report: Path, path: str, kind: str, title: str) -> str:
    src = html.escape(relative_report_url(report, path), quote=True)
    alt = html.escape(f"{title} {kind}", quote=True)
    if kind in {"mp4-card", "webm-card"}:
        media_type = "video/mp4" if kind == "mp4-card" else "video/webm"
        return (
            f'<video muted loop playsinline controls preload="none">'
            f'<source data-src="{src}" type="{media_type}"></video>'
        )
    return f'<img data-src="{src}" alt="{alt}" loading="lazy">'


def variant_for(item: MediaComparison, kind: str) -> MediaVariant:
    for media in item.variants:
        if media.kind == kind:
            return media
    raise ValueError(f"{item.id}: missing {kind} variant")


def optional_variant_for(item: MediaComparison, kind: str) -> MediaVariant | None:
    for media in item.variants:
        if media.kind == kind:
            return media
    return None


def preferred_variant_kind(preferred_kind: str) -> str:
    return {
        "animated-webp": "animated-webp-card",
        "video-mp4": "mp4-card",
        "video-webm": "webm-card",
    }.get(preferred_kind, preferred_kind)


def generated_variant_kinds(profile: EncodingProfile, include_webm: bool) -> set[str]:
    preferred = preferred_variant_kind(profile.preferred_kind)
    if preferred not in {"animated-webp-card", "mp4-card", "webm-card"}:
        raise ValueError(f"unsupported preferred media kind: {profile.preferred_kind}")

    kinds = {preferred}
    if profile.compare:
        kinds.update({"animated-webp-card", "mp4-card"})
        if include_webm:
            kinds.add("webm-card")
    return kinds


def budgeted_variants(item: MediaComparison) -> list[MediaVariant]:
    variants = [variant_for(item, "poster")]
    preferred = optional_variant_for(item, preferred_variant_kind(item.preferred_kind))
    if preferred is not None:
        variants.append(preferred)
    return variants


def site_video_keys(manifest_path: Path) -> set[tuple[str, str]]:
    keys: set[tuple[str, str]] = set()
    for entry in gallery_media.load_manifest(manifest_path).get("examples", []):
        preview = gallery_media.preview_metadata(entry)
        card = preview.get("card") or {}
        if not isinstance(card, dict) or card.get("preferred") != "video-mp4":
            continue
        keys.add(gallery_media.entry_key(entry))
    return keys


def write_site_assets(
    output_dir: Path,
    comparisons: list[MediaComparison],
    manifest_path: Path = gallery_media.DEFAULT_MANIFEST,
) -> int:
    copied = 0
    video_keys = site_video_keys(manifest_path)
    for item in comparisons:
        if (item.lane, item.id) not in video_keys:
            continue
        (output_dir / item.lane / f"{item.id}.webp").unlink(missing_ok=True)
        mp4 = variant_for(item, "mp4-card")
        poster = variant_for(item, "poster")
        for source, target in (
            (ROOT / mp4.path, output_dir / item.lane / f"{item.id}.mp4"),
            (ROOT / poster.path, output_dir / item.lane / f"{item.id}.poster.webp"),
        ):
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, target)
            copied += 1
    return copied


def lazy_webp_card(report: Path, item: MediaComparison) -> str:
    title = html.escape(item.title)
    poster = variant_for(item, "poster")
    animated = variant_for(item, "animated-webp-card")
    poster_src = html.escape(relative_report_url(report, poster.path), quote=True)
    animated_src = html.escape(relative_report_url(report, animated.path), quote=True)
    return "\n".join(
        [
            '<article class="integration-card" data-gallery-lazy="webp">',
            f"<h3>{title} <span>lazy animated WebP</span></h3>",
            '<a class="dvz-gallery-media" href="#">',
            f'  <img class="dvz-gallery-poster" src="{poster_src}" alt="{title}">',
            "  <img class=\"dvz-gallery-animated\"",
            f'       data-src="{animated_src}" alt="" aria-hidden="true">',
            "</a>",
            f"<p>{animated.bytes / 1024:.1f} KB animation, {poster.bytes / 1024:.1f} KB poster</p>",
            "</article>",
        ]
    )


def lazy_video_card(report: Path, item: MediaComparison) -> str:
    title = html.escape(item.title)
    poster = variant_for(item, "poster")
    video = variant_for(item, "mp4-card")
    poster_src = html.escape(relative_report_url(report, poster.path), quote=True)
    video_src = html.escape(relative_report_url(report, video.path), quote=True)
    return "\n".join(
        [
            '<article class="integration-card" data-gallery-lazy="video">',
            f"<h3>{title} <span>lazy MP4 video</span></h3>",
            '<a class="dvz-gallery-media" href="#">',
            f'  <img class="dvz-gallery-poster" src="{poster_src}" alt="{title}">',
            "  <video class=\"dvz-gallery-video\" muted loop playsinline preload=\"none\" data-autoplay=\"1\"",
            f'         poster="{poster_src}" aria-label="{title}">',
            f'    <source data-src="{video_src}" type="video/mp4">',
            "  </video>",
            "</a>",
            f"<p>{video.bytes / 1024:.1f} KB video, {poster.bytes / 1024:.1f} KB poster</p>",
            "</article>",
        ]
    )


def write_html_report(path: Path, comparisons: list[MediaComparison]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    examples = []
    summary_rows = []
    for item in comparisons:
        title = html.escape(item.title)
        cards = []
        snippets = []
        if optional_variant_for(item, "animated-webp-card") is not None:
            cards.append(lazy_webp_card(path, item))
            snippets.extend(
                [
                    "<details><summary>lazy animated WebP card HTML</summary>",
                    f"<pre>{html.escape(item.webp_html_snippet)}</pre></details>",
                ]
            )
        if optional_variant_for(item, "mp4-card") is not None:
            cards.append(lazy_video_card(path, item))
            snippets.extend(
                [
                    "<details><summary>lazy MP4 video card HTML</summary>",
                    f"<pre>{html.escape(item.video_html_snippet)}</pre></details>",
                ]
            )
        examples.append(
            "\n".join(
                [
                    '<section class="integration-example">',
                    f"<h2>{title}</h2>",
                    (
                        f"<p><code>{html.escape(item.id)}</code> source "
                        f"{item.source_frames} frames at {html.escape(item.source_size)}; "
                        f"candidate {item.encoded_frames} frames at "
                        f"{item.encoded_fps} fps within {html.escape(item.encoded_size)}.</p>"
                    ),
                    '<div class="integration-row">',
                    *cards,
                    "</div>",
                    *snippets,
                    "</section>",
                ]
            )
        )

        poster = variant_for(item, "poster")
        animated = optional_variant_for(item, "animated-webp-card")
        video = optional_variant_for(item, "mp4-card")
        webm = optional_variant_for(item, "webm-card")
        animated_cell = (
            f"<td class=\"{animated.status}\">{animated.bytes / 1024:.1f} KB</td>"
            if animated
            else "<td class=\"skipped\">skipped</td>"
        )
        video_cell = (
            f"<td class=\"{video.status}\">{video.bytes / 1024:.1f} KB</td>"
            if video
            else "<td class=\"skipped\">skipped</td>"
        )
        webm_cell = f"<td>{webm.bytes / 1024:.1f} KB</td>" if webm else "<td></td>"
        summary_rows.append(
            "<tr>"
            f"<td><code>{html.escape(item.id)}</code></td>"
            f"<td>{html.escape(item.preferred_kind)}</td>"
            f"<td>{html.escape(item.encoded_size)}</td>"
            f"<td>{item.encoded_frames} @ {item.encoded_fps} fps</td>"
            f"{animated_cell}"
            f"{video_cell}"
            f"<td>{poster.bytes / 1024:.1f} KB</td>"
            f"{webm_cell}"
            "</tr>"
        )

    document = "\n".join(
        [
            "<!doctype html>",
            "<html lang=\"en\">",
            "<head>",
            "<meta charset=\"utf-8\">",
            "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">",
            "<title>Datoviz Gallery Media Comparison</title>",
            "<style>",
            ":root{color-scheme:dark;--bg:#0e1117;--bg-elevated:#161b22;--bg-subtle:#0a0d12;--fg:#c9d1d9;--fg-muted:#8b949e;--border:#30363d;--accent:#4cc9f0;--ok:#7ee787;--warn:#ff7b72}",
            "body{font-family:system-ui,sans-serif;margin:2rem;color:var(--fg);background:var(--bg)}",
            "a{color:var(--accent)}",
            "h1,h2,h3{color:var(--fg)}",
            "p{color:var(--fg-muted)}",
            "code{color:var(--fg);background:var(--bg-subtle);border:1px solid var(--border);border-radius:4px;padding:.08rem .25rem}",
            "section{margin:0 0 3rem}",
            ".integration-row{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:1rem;margin:1rem 0}",
            ".integration-card{background:var(--bg-elevated);border:1px solid var(--border);border-radius:6px;padding:.75rem;box-shadow:0 1px 0 rgba(255,255,255,.03)}",
            ".integration-card h3{font-size:1rem;margin:.1rem 0 .6rem}",
            ".integration-card h3 span{display:block;font-weight:400;color:var(--fg-muted)}",
            ".integration-card p{font-size:.85rem;margin:.5rem 0 0}",
            ".dvz-gallery-media{display:block;position:relative;overflow:hidden;background:#05070a;border:1px solid var(--border);border-radius:4px;aspect-ratio:16/9}",
            ".dvz-gallery-poster,.dvz-gallery-animated,.dvz-gallery-video{position:absolute;inset:0;width:100%;height:100%;object-fit:contain}",
            ".dvz-gallery-animated,.dvz-gallery-video{opacity:0;transition:opacity .18s ease}",
            ".dvz-gallery-media.is-ready .dvz-gallery-animated,.dvz-gallery-media.is-ready .dvz-gallery-video{opacity:1}",
            ".ok{color:var(--ok)}.over-budget{color:var(--warn)}.skipped{color:var(--fg-muted)}",
            "details{color:var(--fg-muted)}",
            "summary{cursor:pointer;color:var(--fg)}",
            "pre{overflow:auto;padding:1rem;background:var(--bg-subtle);color:var(--fg);border:1px solid var(--border);border-radius:6px}",
            "table{width:100%;border-collapse:collapse;background:var(--bg-elevated);border:1px solid var(--border)}",
            "th,td{text-align:left;border:1px solid var(--border);padding:.45rem;font-size:.9rem}",
            "th{background:var(--bg-subtle);color:var(--fg)}",
            "td{color:var(--fg)}",
            "@media (max-width:720px){.integration-row{grid-template-columns:1fr}}",
            "@media (prefers-reduced-motion: reduce){.dvz-gallery-animated,.dvz-gallery-video{display:none}}",
            "</style>",
            "</head>",
            "<body>",
            "<h1>Datoviz Gallery Media Comparison</h1>",
            "<p>Generated build-local media. Do not commit these binary outputs.</p>",
            "<p>Rows show the manifest-selected build-local media. Settled examples generate only their preferred asset plus a poster; examples with <code>card.compare: true</code> show WebP and MP4 side by side. Posters display immediately; animation/video sources attach only near the viewport unless reduced motion or Save-Data is active.</p>",
            *examples,
            "<section>",
            "<h2>Size Summary</h2>",
            "<table>",
            "<thead><tr><th>Example</th><th>Preferred</th><th>Max size</th><th>Frames</th><th>WebP</th><th>MP4</th><th>Poster</th><th>WebM</th></tr></thead>",
            "<tbody>",
            *summary_rows,
            "</tbody>",
            "</table>",
            "</section>",
            "<script>",
            "(() => {",
            "  const reduceMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;",
            "  const saveData = navigator.connection && navigator.connection.saveData;",
            "  if (reduceMotion || saveData) return;",
            "  const ready = (el) => el.closest('.dvz-gallery-media')?.classList.add('is-ready');",
            "  const loadCard = (card) => {",
            "    if (card.dataset.loaded === '1') return;",
            "    card.dataset.loaded = '1';",
            "    const img = card.querySelector('img[data-src]');",
            "    if (img) {",
            "      img.addEventListener('load', () => ready(img), { once: true });",
            "      img.src = img.dataset.src;",
            "      return;",
            "    }",
            "    const video = card.querySelector('video.dvz-gallery-video');",
            "    if (!video) return;",
            "    for (const source of video.querySelectorAll('source[data-src]')) {",
            "      source.src = source.dataset.src;",
            "    }",
            "    video.addEventListener('canplay', () => ready(video), { once: true });",
            "    video.load();",
            "    if (video.dataset.autoplay === '1') video.play().catch(() => {});",
            "  };",
            "  if (!('IntersectionObserver' in window)) {",
            "    document.querySelectorAll('[data-gallery-lazy]').forEach(loadCard);",
            "    return;",
            "  }",
            "  const observer = new IntersectionObserver((entries) => {",
            "    for (const entry of entries) {",
            "      if (!entry.isIntersecting) continue;",
            "      loadCard(entry.target);",
            "      observer.unobserve(entry.target);",
            "    }",
            "  }, { rootMargin: '400px 0px' });",
            "  document.querySelectorAll('[data-gallery-lazy]').forEach((card) => observer.observe(card));",
            "})();",
            "</script>",
            "</body>",
            "</html>",
        ]
    )
    path.write_text(document + "\n", encoding="utf8")


def main() -> int:
    started_at = time.perf_counter()
    args = parse_args()
    try:
        jobs = parse_jobs(args.jobs)
        capture_jobs = parse_jobs(args.capture_jobs)
        if args.step <= 0:
            raise ValueError("--step must be positive")
        for value, label in (
            (args.webp_quality, "--webp-quality"),
            (args.mp4_crf, "--mp4-crf"),
            (args.webm_crf, "--webm-crf"),
        ):
            if not 0 <= value <= 100:
                raise ValueError(f"{label} must be between 0 and 100")
        require_tool("img2webp")
        require_tool("cwebp")
        require_tool("ffmpeg")
        require_tool("ffprobe")

        previews = selected_previews(args)
        if not previews:
            print("No matching animated previews.")
            return 1
        manifest = gallery_media.load_manifest(args.manifest)
        manifest_entries = {
            gallery_media.entry_key(item): item
            for item in manifest.get("examples", [])
        }
        profiles = {
            (getattr(preview, "lane"), getattr(preview, "id")): profile_for(
                preview, args, manifest_entries
            )
            for preview in previews
        }

        if args.dry_run:
            for preview in previews:
                compare_preview(
                    preview,
                    args,
                    profile=profiles[(getattr(preview, "lane"), getattr(preview, "id"))],
                )
            print(f"gallery media compare: selected={len(previews)}")
            return 0

        encoder_fingerprint = toolchain_fingerprint()

        capture_started_at = time.perf_counter()
        frame_sequences = bounded_parallel_map(
            previews,
            lambda preview: gallery_frames.ensure_frames(
                preview,
                args.manifest,
                args.frame_dir,
                args.frame_cache_dir,
                force=args.force,
            ),
            capture_jobs,
            lambda preview: getattr(preview, "id"),
        )
        capture_seconds = time.perf_counter() - capture_started_at
        for sequence in frame_sequences:
            if sequence.generated:
                print(
                    f"captured frames: {sequence.preview.id} -> "
                    f"{child_path(sequence.frame_dir)}"
                )

        sequences_by_key = {
            (sequence.preview.lane, sequence.preview.id): sequence
            for sequence in frame_sequences
        }
        encode_started_at = time.perf_counter()
        outcomes = bounded_parallel_map(
            previews,
            lambda preview: compare_preview_cached(
                preview,
                args,
                sequences_by_key[(getattr(preview, "lane"), getattr(preview, "id"))],
                profiles[(getattr(preview, "lane"), getattr(preview, "id"))],
                encoder_fingerprint,
            ),
            jobs,
            lambda preview: getattr(preview, "id"),
        )
        comparisons = [outcome.comparison for outcome in outcomes]
        encode_seconds = time.perf_counter() - encode_started_at

        publish_started_at = time.perf_counter()
        report_comparisons = comparisons
        if should_merge_report(args):
            report_comparisons = merge_comparisons(read_report(args.report), comparisons)

        print_report(comparisons)
        write_report(args.report, report_comparisons)
        write_html_report(args.html_report, report_comparisons)
        copied = 0
        if args.write_site_assets:
            copied = write_site_assets(args.site_output_dir, comparisons, args.manifest)
        publish_seconds = time.perf_counter() - publish_started_at
        over_budget = sum(
            1
            for item in comparisons
            for media in budgeted_variants(item)
            if media.status != "ok"
        )
        print(
            f"gallery media compare: compared={len(comparisons)} "
            f"jobs={jobs} capture_jobs={capture_jobs} "
            f"over_budget={over_budget} report={child_path(args.report)} "
            f"html={child_path(args.html_report)} site_assets={copied}"
        )
        frame_cache_misses = sum(sequence.generated for sequence in frame_sequences)
        media_cache_hits = sum(outcome.cached for outcome in outcomes)
        print(
            "gallery media timing: "
            f"capture={capture_seconds:.3f}s encode={encode_seconds:.3f}s "
            f"publish={publish_seconds:.3f}s total={time.perf_counter() - started_at:.3f}s "
            f"frame_cache_hits={len(frame_sequences) - frame_cache_misses} "
            f"frame_cache_misses={frame_cache_misses} "
            f"media_cache_hits={media_cache_hits} "
            f"media_cache_misses={len(outcomes) - media_cache_hits}"
        )
        return 1 if over_budget else 0
    except (OSError, RuntimeError, ValueError, subprocess.CalledProcessError) as exc:
        print(f"gallery media compare: {exc}")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
