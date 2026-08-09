import shutil
import subprocess
import sys
import threading
import time
from argparse import Namespace
from pathlib import Path

import pytest
from PIL import Image


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import compare_gallery_media as media  # noqa: E402
import gallery_workers  # noqa: E402


def test_mp4_plan_60_fps_then_30_fps_crf_ladder() -> None:
    attempts = media.mp4_attempt_plan(60, 1, 32)

    assert attempts == [
        media.Mp4Attempt(fps=60, sample_step=1, crf=32),
        media.Mp4Attempt(fps=30, sample_step=2, crf=32),
        media.Mp4Attempt(fps=30, sample_step=2, crf=36),
        media.Mp4Attempt(fps=30, sample_step=2, crf=40),
    ]


def test_mp4_plan_native_30_fps_is_not_upsampled() -> None:
    assert media.mp4_attempt_plan(30, 1, 38) == [
        media.Mp4Attempt(fps=30, sample_step=1, crf=38),
        media.Mp4Attempt(fps=30, sample_step=1, crf=40),
    ]


def test_mp4_plan_lower_native_rate_keeps_fps_and_uses_crf_ladder() -> None:
    assert media.mp4_attempt_plan(24, 1, 36) == [
        media.Mp4Attempt(fps=24, sample_step=1, crf=36),
        media.Mp4Attempt(fps=24, sample_step=1, crf=40),
    ]


def test_mp4_executor_selects_first_attempt_within_budget() -> None:
    attempts = media.mp4_attempt_plan(60, 1, 32)
    sizes = iter((1_200_000, 1_100_000, 900_000))
    visited = []

    selected = media.execute_mp4_attempts(
        attempts,
        lambda attempt: visited.append(attempt) or next(sizes),
    )

    assert selected == attempts[2]
    assert visited == attempts[:3]


def test_mp4_executor_rejects_impossible_budget() -> None:
    attempts = media.mp4_attempt_plan(30, 1, 40)

    try:
        media.execute_mp4_attempts(attempts, lambda unused: 1_000_001)
    except RuntimeError as exc:
        assert "CRF 40" in str(exc)
    else:
        raise AssertionError("expected an impossible-budget failure")


def test_cached_frames_use_canonical_sources_without_resize(tmp_path: Path) -> None:
    class Preview:
        frames = 2
        fps = 30

    for index in range(2):
        media.gallery_frames.frame_path(tmp_path, index).write_bytes(b"png")

    frames = media.cached_frames(Preview(), tmp_path)

    assert [frame.path for frame in frames] == [
        tmp_path / "frame_0000.png",
        tmp_path / "frame_0001.png",
    ]


@pytest.mark.skipif(
    shutil.which("ffmpeg") is None or shutil.which("ffprobe") is None,
    reason="ffmpeg tools are unavailable",
)
def test_probe_canonical_synthetic_mp4(tmp_path: Path) -> None:
    output = tmp_path / "synthetic.mp4"
    subprocess.run(
        [
            "ffmpeg",
            "-hide_banner",
            "-loglevel",
            "error",
            "-f",
            "lavfi",
            "-i",
            "color=c=black:s=1280x720:r=30:d=0.2",
            "-an",
            "-c:v",
            "libx264",
            "-pix_fmt",
            "yuv420p",
            str(output),
        ],
        check=True,
    )

    probe = media.probe_media(output)

    assert (probe.width, probe.height) == (1280, 720)
    assert probe.fps == 30
    assert probe.frames == 6
    assert probe.duration == pytest.approx(0.2, abs=0.02)


@pytest.mark.skipif(
    shutil.which("ffprobe") is None,
    reason="ffprobe is unavailable",
)
def test_probe_animated_webp_falls_back_when_ffprobe_omits_dimensions(tmp_path: Path) -> None:
    output = tmp_path / "synthetic.webp"
    frames = [
        Image.new("RGB", (1280, 720), color)
        for color in ("black", "white")
    ]
    frames[0].save(
        output,
        save_all=True,
        append_images=frames[1:],
        duration=100,
        loop=0,
    )

    probe = media.probe_media(output)

    assert (probe.width, probe.height) == (1280, 720)


def test_parse_jobs_caps_automatic_workers(monkeypatch) -> None:
    monkeypatch.setattr(gallery_workers.os, "cpu_count", lambda: 64)

    assert gallery_workers.parse_jobs("auto") == 4
    assert gallery_workers.parse_jobs("1") == 1


def test_bounded_parallel_map_preserves_order_and_worker_bound() -> None:
    lock = threading.Lock()
    active = 0
    max_active = 0

    def worker(value: int) -> int:
        nonlocal active, max_active
        with lock:
            active += 1
            max_active = max(max_active, active)
        time.sleep(0.01 * (4 - value))
        with lock:
            active -= 1
        return value * 10

    results = gallery_workers.bounded_parallel_map([1, 2, 3], worker, jobs=2)

    assert results == [10, 20, 30]
    assert max_active == 2


def test_bounded_parallel_map_explicit_serial_mode() -> None:
    visited = []

    results = gallery_workers.bounded_parallel_map(
        [3, 1, 2],
        lambda value: visited.append(value) or value * 10,
        jobs=1,
    )

    assert visited == [3, 1, 2]
    assert results == [30, 10, 20]


def test_bounded_parallel_map_stops_scheduling_after_failure() -> None:
    second_started = threading.Event()
    started = []
    lock = threading.Lock()

    def worker(value: int) -> int:
        with lock:
            started.append(value)
        if value == 0:
            assert second_started.wait(timeout=1)
            raise ValueError("broken")
        second_started.set()
        time.sleep(0.05)
        return value

    with pytest.raises(RuntimeError, match="item-0: broken"):
        gallery_workers.bounded_parallel_map(
            list(range(8)),
            worker,
            jobs=2,
            label=lambda value: f"item-{value}",
        )

    assert sorted(started) == [0, 1]


def test_parallel_media_workspaces_are_isolated() -> None:
    barrier = threading.Barrier(2)

    def worker(example_id: str) -> str:
        with media.media_workspace(example_id) as workspace:
            barrier.wait()
            path = Path(workspace)
            (path / "sentinel").write_text(example_id, encoding="utf8")
            return str(path)

    workspaces = gallery_workers.bounded_parallel_map(["a", "b"], worker, jobs=2)

    assert len(set(workspaces)) == 2
    assert all(not Path(path).exists() for path in workspaces)


def test_comparison_cache_reuses_verified_outputs(tmp_path: Path, monkeypatch) -> None:
    class Preview:
        id = "example"
        lane = "showcases"

    output = tmp_path / "cards" / "example.poster.webp"
    output.parent.mkdir(parents=True)
    output.write_bytes(b"poster")
    comparison = media.MediaComparison(
        id="example",
        lane="showcases",
        title="Example",
        source_webp="source.webp",
        source_bytes=0,
        source_frames=1,
        source_size="1280x720",
        encoded_size="1280x720",
        encoded_frames=1,
        encoded_fps=30,
        preferred_kind="animated-webp",
        variants=[media.variant("poster", output, media.BUDGETS["poster"])],
        webp_html_snippet="",
        video_html_snippet="",
    )
    monkeypatch.setattr(media, "ROOT", tmp_path)

    media.write_comparison_cache(Preview(), tmp_path / "cache", "input", comparison)

    assert media.current_comparison_cache(
        Preview(), tmp_path / "cache", "input"
    ) == comparison


def test_comparison_cache_rejects_changed_output(tmp_path: Path, monkeypatch) -> None:
    class Preview:
        id = "example"
        lane = "showcases"

    output = tmp_path / "cards" / "example.poster.webp"
    output.parent.mkdir(parents=True)
    output.write_bytes(b"poster")
    comparison = media.MediaComparison(
        id="example",
        lane="showcases",
        title="Example",
        source_webp="source.webp",
        source_bytes=0,
        source_frames=1,
        source_size="1280x720",
        encoded_size="1280x720",
        encoded_frames=1,
        encoded_fps=30,
        preferred_kind="animated-webp",
        variants=[media.variant("poster", output, media.BUDGETS["poster"])],
        webp_html_snippet="",
        video_html_snippet="",
    )
    monkeypatch.setattr(media, "ROOT", tmp_path)
    cache_dir = tmp_path / "cache"
    media.write_comparison_cache(Preview(), cache_dir, "input", comparison)
    output.write_bytes(b"changed")

    assert media.current_comparison_cache(Preview(), cache_dir, "input") is None


def test_comparison_input_hash_tracks_profile_and_frames(tmp_path: Path) -> None:
    class Preview:
        id = "example"
        lane = "showcases"

    sequence = media.gallery_frames.FrameSequence(
        preview=Preview(),
        frame_dir=tmp_path,
        input_hash="frame-input",
        frames_hash="frames-a",
        generated=False,
        reason="current",
    )
    profile = media.EncodingProfile("video-mp4", 1, 40, 32, 38, 30)
    args = Namespace(webm=False, output_dir=tmp_path / "output")
    first = media.comparison_input_hash(Preview(), sequence, profile, args, "tools")
    changed_profile = media.EncodingProfile("video-mp4", 1, 40, 36, 38, 30)
    second = media.comparison_input_hash(
        Preview(), sequence, changed_profile, args, "tools"
    )
    changed_frames = media.gallery_frames.FrameSequence(
        **{**sequence.__dict__, "frames_hash": "frames-b"}
    )
    third = media.comparison_input_hash(
        Preview(), changed_frames, profile, args, "tools"
    )

    assert len({first, second, third}) == 3


def test_selected_previews_limits_docs_assets_to_manifest_mp4_cards(monkeypatch) -> None:
    previews = [
        type("Preview", (), {"id": "webp", "lane": "features"})(),
        type("Preview", (), {"id": "video", "lane": "showcases"})(),
    ]
    args = Namespace(
        id=[],
        lane=[],
        all_animated=False,
        site_video_previews=True,
        manifest=Path("manifest.yaml"),
    )
    manifest = {
        "examples": [
            {
                "id": "webp",
                "category": "feature",
                "media": {"preview": {"kind": "animated-webp"}},
            },
            {
                "id": "video",
                "category": "showcase",
                "media": {
                    "preview": {
                        "kind": "animated-webp",
                        "card": {"preferred": "video-mp4"},
                    }
                },
            },
        ]
    }
    monkeypatch.setattr(media.gallery_media, "load_manifest", lambda unused: manifest)
    collected_ids = []

    def collect(unused_manifest, unused_build_dir, ids, unused_lanes):
        collected_ids.extend(ids)
        return [preview for preview in previews if preview.id in ids]

    monkeypatch.setattr(media.gallery_frames, "collect_previews", collect)

    assert media.selected_previews(args) == [previews[1]]
    assert collected_ids == ["video"]
