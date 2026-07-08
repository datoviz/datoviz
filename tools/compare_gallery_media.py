#!/usr/bin/env python3
"""Build-local gallery animation media comparison.

This tool compares existing animated WebP previews against smaller card
encodes and video alternatives. It writes only to build-local paths by default;
do not commit the generated media until the gallery media policy is settled.
"""

from __future__ import annotations

import argparse
import html
import json
import os
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
DEFAULT_SIZE = "1024x576"
DEFAULT_STEP = 2
DEFAULT_WEBP_QUALITY = 40
DEFAULT_MP4_CRF = 32
DEFAULT_WEBM_CRF = 38
DEFAULT_PREFERRED_KIND = "video-mp4"
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


@dataclass(frozen=True)
class EncodingProfile:
    preferred_kind: str
    size: str
    step: int
    webp_quality: int
    mp4_crf: int
    webm_crf: int
    fps: int = 0


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


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=gallery_media.DEFAULT_MANIFEST)
    parser.add_argument("--input-dir", type=Path, default=DEFAULT_INPUT_DIR)
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR)
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT)
    parser.add_argument("--html-report", type=Path, default=DEFAULT_HTML_REPORT)
    parser.add_argument("--id", action="append", default=[], help="example id; repeat or comma-separate")
    parser.add_argument("--lane", action="append", default=[], help="gallery lane")
    parser.add_argument("--size", default=DEFAULT_SIZE, help="card media size, for example 640x360")
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


def preview_card_metadata(preview: object, manifest_path: Path) -> dict:
    manifest = gallery_media.load_manifest(manifest_path)
    preview_key = (getattr(preview, "lane"), getattr(preview, "id"))
    for entry in manifest.get("examples", []):
        if gallery_media.entry_key(entry) != preview_key:
            continue
        metadata = gallery_media.preview_metadata(entry)
        card = metadata.get("card") or {}
        return card if isinstance(card, dict) else {}
    return {}


def profile_for(preview: object, args: argparse.Namespace) -> EncodingProfile:
    fields = {
        "preferred_kind": args.preferred_kind,
        "size": args.size,
        "step": args.step,
        "webp_quality": args.webp_quality,
        "mp4_crf": args.mp4_crf,
        "webm_crf": args.webm_crf,
        "fps": args.fps,
    }
    if not args.no_manifest_card:
        card = preview_card_metadata(preview, args.manifest)
        fields.update(
            {
                "preferred_kind": str(card.get("preferred", fields["preferred_kind"])),
                "size": str(card.get("size", fields["size"])),
                "step": int(card.get("sample_step", card.get("step", fields["step"]))),
                "webp_quality": int(card.get("webp_quality", fields["webp_quality"])),
                "mp4_crf": int(card.get("mp4_crf", fields["mp4_crf"])),
                "webm_crf": int(card.get("webm_crf", fields["webm_crf"])),
                "fps": int(card.get("fps", fields["fps"])),
            }
        )
    return EncodingProfile(**fields)


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
            "  <video muted loop playsinline preload=\"none\"",
            f'         poster="{media_base}.poster.webp" aria-label="{title}">',
            f'    <source data-src="{media_base}.mp4" type="video/mp4">',
            f'    <img src="{media_base}.poster.webp" alt="{title}">',
            "  </video>",
            "</a>",
        ]
    )


def compare_preview(preview: object, args: argparse.Namespace) -> MediaComparison | None:
    source = source_webp_path(preview, args.input_dir)
    if not source.exists():
        print(f"missing source: {getattr(preview, 'id')} -> {child_path(source)}")
        return None

    profile = profile_for(preview, args)
    base = output_base(preview, args.output_dir)
    animated_webp = base / f"{getattr(preview, 'id')}.card.webp"
    mp4 = base / f"{getattr(preview, 'id')}.card.mp4"
    webm = base / f"{getattr(preview, 'id')}.card.webm"
    poster = base / f"{getattr(preview, 'id')}.poster.webp"
    out_fps = output_fps(preview, profile)
    encoded_frames = (getattr(preview, "frames") + profile.step - 1) // profile.step

    if args.dry_run:
        print(
            f"would compare: {getattr(preview, 'id')} "
            f"source={child_path(source)} size={profile.size} "
            f"step={profile.step} fps={out_fps} preferred={profile.preferred_kind}"
        )
        return None

    with TemporaryDirectory(prefix="dvz-gallery-media-") as tmp:
        frame_root = Path(tmp)
        frames = coalesce_and_resize(source, frame_root / "frames", profile.size)
        selected = select_frames(frames, profile.step)
        selected_dir = frame_root / "selected"
        selected_dir.mkdir()
        for index, frame in enumerate(selected):
            shutil.copyfile(frame, selected_dir / f"frame_{index:04d}.png")
        selected_pattern = str(selected_dir / "frame_%04d.png")
        encode_webp(selected, animated_webp, out_fps, profile.webp_quality)
        encode_mp4(selected_pattern, mp4, out_fps, profile.mp4_crf)
        if args.webm:
            encode_webm(selected_pattern, webm, out_fps, profile.webm_crf)
        encode_poster(selected[0], poster, profile.webp_quality)
        encoded_frames = len(selected)

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
        encoded_size=profile.size,
        encoded_frames=encoded_frames,
        encoded_fps=out_fps,
        preferred_kind=profile.preferred_kind,
        variants=variants,
        webp_html_snippet=webp_html_snippet(preview),
        video_html_snippet=video_html_snippet(preview),
    )


def print_report(comparisons: list[MediaComparison]) -> None:
    for item in comparisons:
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


def budgeted_variants(item: MediaComparison) -> list[MediaVariant]:
    variants = [variant_for(item, "poster")]
    preferred = optional_variant_for(item, preferred_variant_kind(item.preferred_kind))
    if preferred is not None:
        variants.append(preferred)
    return variants


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
            "  <video class=\"dvz-gallery-video\" muted loop playsinline preload=\"none\" data-autoplay=\"1\"",
            f'         poster="{poster_src}" aria-label="{title}">',
            f'    <source data-src="{video_src}" type="video/mp4">',
            f'    <img src="{poster_src}" alt="{title}">',
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
                    lazy_webp_card(path, item),
                    lazy_video_card(path, item),
                    "</div>",
                    "<details><summary>candidate lazy animated WebP card HTML</summary>",
                    f"<pre>{html.escape(item.webp_html_snippet)}</pre></details>",
                    "<details><summary>candidate lazy MP4 video card HTML</summary>",
                    f"<pre>{html.escape(item.video_html_snippet)}</pre></details>",
                    "</section>",
                ]
            )
        )

        animated = variant_for(item, "animated-webp-card")
        video = variant_for(item, "mp4-card")
        poster = variant_for(item, "poster")
        webm = optional_variant_for(item, "webm-card")
        webm_cell = f"<td>{webm.bytes / 1024:.1f} KB</td>" if webm else "<td></td>"
        summary_rows.append(
            "<tr>"
            f"<td><code>{html.escape(item.id)}</code></td>"
            f"<td>{html.escape(item.preferred_kind)}</td>"
            f"<td>{html.escape(item.encoded_size)}</td>"
            f"<td>{item.encoded_frames} @ {item.encoded_fps} fps</td>"
            f"<td class=\"{animated.status}\">{animated.bytes / 1024:.1f} KB</td>"
            f"<td class=\"{video.status}\">{video.bytes / 1024:.1f} KB</td>"
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
            "body{font-family:system-ui,sans-serif;margin:2rem;color:#202124;background:#fafafa}",
            "section{margin:0 0 3rem}",
            ".integration-row{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:1rem;margin:1rem 0}",
            ".integration-card{background:white;border:1px solid #ddd;padding:.75rem}",
            ".integration-card h3{font-size:1rem;margin:.1rem 0 .6rem}",
            ".integration-card h3 span{display:block;font-weight:400;color:#5f6368}",
            ".integration-card p{font-size:.85rem;margin:.5rem 0 0}",
            ".dvz-gallery-media{display:block;position:relative;overflow:hidden;background:#111;aspect-ratio:16/9}",
            ".dvz-gallery-poster,.dvz-gallery-animated,.dvz-gallery-video{position:absolute;inset:0;width:100%;height:100%;object-fit:contain}",
            ".dvz-gallery-animated,.dvz-gallery-video{opacity:0;transition:opacity .18s ease}",
            ".dvz-gallery-media.is-ready .dvz-gallery-animated,.dvz-gallery-media.is-ready .dvz-gallery-video{opacity:1}",
            ".ok{color:#0a7a2f}.over-budget{color:#b00020}",
            "pre{overflow:auto;padding:1rem;background:#111;color:#f5f5f5}",
            "table{width:100%;border-collapse:collapse;background:white}",
            "th,td{text-align:left;border:1px solid #ddd;padding:.45rem;font-size:.9rem}",
            "@media (max-width:720px){.integration-row{grid-template-columns:1fr}}",
            "@media (prefers-reduced-motion: reduce){.dvz-gallery-animated,.dvz-gallery-video{display:none}}",
            "</style>",
            "</head>",
            "<body>",
            "<h1>Datoviz Gallery Media Comparison</h1>",
            "<p>Generated build-local media. Do not commit these binary outputs.</p>",
            "<p>Each example row shows exactly two candidate integrations: lazy animated WebP and lazy MP4. Posters display immediately; animation/video sources attach only near the viewport unless reduced motion or Save-Data is active.</p>",
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
        write_html_report(args.html_report, comparisons)
        over_budget = sum(
            1
            for item in comparisons
            for media in budgeted_variants(item)
            if media.status != "ok"
        )
        print(
            f"gallery media compare: compared={len(comparisons)} "
            f"over_budget={over_budget} report={child_path(args.report)} "
            f"html={child_path(args.html_report)}"
        )
        return 1 if over_budget else 0
    except (OSError, RuntimeError, ValueError, subprocess.CalledProcessError) as exc:
        print(f"gallery media compare: {exc}")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
