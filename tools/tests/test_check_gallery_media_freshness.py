import argparse
import os
from pathlib import Path
import sys


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import check_gallery_media_freshness as freshness  # noqa: E402
import compare_gallery_media  # noqa: E402
import gallery_frames  # noqa: E402


def _preview(tmp_path: Path) -> gallery_frames.AnimatedPreview:
    return gallery_frames.AnimatedPreview(
        id="example",
        title="Example",
        lane="showcases",
        source="examples/c/showcases/example.c",
        executable=tmp_path / "example",
        frames=2,
        fps=30,
        sample_stride=1,
        time_scale=1.0,
        size="1280x720",
    )


def _comparison(candidate_dir: Path) -> compare_gallery_media.MediaComparison:
    candidate_dir = candidate_dir / "showcases" / "example"
    variants = []
    for kind, filename in (("poster", "example.poster.webp"), ("mp4-card", "example.card.mp4")):
        path = candidate_dir / filename
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(kind.encode("utf8"))
        variants.append(
            compare_gallery_media.MediaVariant(
                kind=kind,
                path=str(path),
                bytes=path.stat().st_size,
                budget_bytes=1_000_000,
                status="ok",
            )
        )
    return compare_gallery_media.MediaComparison(
        id="example",
        lane="showcases",
        title="Example",
        source_webp="source.webp",
        source_bytes=1,
        source_frames=2,
        source_size="1280x720",
        encoded_size="1280x720",
        encoded_frames=2,
        encoded_fps=30,
        preferred_kind="video-mp4",
        variants=variants,
        webp_html_snippet="",
        video_html_snippet="",
        sample_step=1,
        encoded_crf=32,
        source_duration=2 / 30,
        encoded_duration=2 / 30,
    )


def test_video_card_current(tmp_path: Path, monkeypatch) -> None:
    preview = _preview(tmp_path)
    candidate_dir = tmp_path / "cards"
    item = _comparison(candidate_dir)
    monkeypatch.setattr(freshness, "ROOT", Path("/"))
    monkeypatch.setattr(
        compare_gallery_media,
        "probe_media",
        lambda path: compare_gallery_media.MediaProbe(
            width=1280,
            height=720,
            fps=30 if path.suffix == ".mp4" else 0,
            frames=2 if path.suffix == ".mp4" else 0,
            duration=2 / 30 if path.suffix == ".mp4" else 0,
        ),
    )
    cache = tmp_path / "cache.json"
    cache.write_text("{}", encoding="utf8")
    past = min((Path("/") / variant.path).stat().st_mtime_ns for variant in item.variants) - 1_000_000
    os.utime(cache, ns=(past, past))
    site = tmp_path / "site"
    (site / "showcases").mkdir(parents=True)
    for variant, target in zip(
        item.variants,
        (site / "showcases" / "example.poster.webp", site / "showcases" / "example.mp4"),
        strict=True,
    ):
        target.write_bytes((Path("/") / variant.path).read_bytes())

    assert freshness.validate_video_card(preview, cache, item, candidate_dir, site) == []


def test_video_card_rejects_candidate_older_than_frames(tmp_path: Path, monkeypatch) -> None:
    preview = _preview(tmp_path)
    item = _comparison(tmp_path / "cards")
    monkeypatch.setattr(freshness, "ROOT", Path("/"))
    monkeypatch.setattr(
        compare_gallery_media,
        "probe_media",
        lambda unused: compare_gallery_media.MediaProbe(width=1280, height=720),
    )
    cache = tmp_path / "cache.json"
    cache.write_text("{}", encoding="utf8")
    future = max(Path("/").joinpath(variant.path).stat().st_mtime_ns for variant in item.variants) + 1_000_000
    os.utime(cache, ns=(future, future))

    errors = freshness.validate_video_card(preview, cache, item, tmp_path / "cards", tmp_path / "site")

    assert any("predates the current frame cache" in error for error in errors)


def test_preview_rejects_stale_frame_inputs(tmp_path: Path, monkeypatch) -> None:
    preview = _preview(tmp_path)
    args = argparse.Namespace(
        manifest=tmp_path / "manifest.yaml",
        frame_dir=tmp_path / "frames",
        frame_cache_dir=tmp_path / "cache",
    )
    monkeypatch.setattr(gallery_frames, "input_hash_for", lambda *unused: "input")
    monkeypatch.setattr(
        gallery_frames,
        "current_cache_hit",
        lambda *unused: (False, "stale inputs", ""),
    )

    errors = freshness.validate_preview(preview, args, {}, set())

    assert errors == ["showcases/example: frame cache is not current (stale inputs)"]


def test_video_card_rejects_report_policy_drift(tmp_path: Path, monkeypatch) -> None:
    preview = _preview(tmp_path)
    item = _comparison(tmp_path / "cards")
    item = compare_gallery_media.MediaComparison(
        **{
            **item.__dict__,
            "encoded_size": "1024x576",
            "encoded_duration": 1.0,
        }
    )
    monkeypatch.setattr(freshness, "ROOT", Path("/"))
    monkeypatch.setattr(
        compare_gallery_media,
        "probe_media",
        lambda unused: compare_gallery_media.MediaProbe(width=1024, height=576),
    )
    cache = tmp_path / "cache.json"
    cache.write_text("{}", encoding="utf8")

    errors = freshness.validate_video_card(
        preview, cache, item, tmp_path / "cards", tmp_path / "site"
    )

    assert any("encoded size is 1024x576" in error for error in errors)
    assert any("report encoded duration" in error for error in errors)
    assert any("generated poster: encoded dimensions are 1024x576" in error for error in errors)
