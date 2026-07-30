from pathlib import Path
import shutil
import subprocess
import sys

import pytest


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import compare_gallery_media as media  # noqa: E402


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
