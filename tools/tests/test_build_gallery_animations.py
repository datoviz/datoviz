import argparse
from pathlib import Path
import sys


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import build_gallery_animations as animations  # noqa: E402


def _preview(tmp_path: Path, example_id: str) -> animations.AnimatedPreview:
    return animations.AnimatedPreview(
        id=example_id,
        title=example_id,
        lane="features",
        source=f"examples/c/features/{example_id}.c",
        executable=tmp_path / example_id,
        frames=2,
        fps=30,
        sample_stride=1,
        time_scale=1.0,
        size="1280x720",
    )


def test_main_uses_separate_worker_limits_and_preserves_output_order(
    tmp_path: Path, monkeypatch, capsys
) -> None:
    previews = [_preview(tmp_path, "first"), _preview(tmp_path, "second")]
    args = argparse.Namespace(
        manifest=tmp_path / "manifest.yaml",
        build_examples_dir=tmp_path / "examples",
        frame_dir=tmp_path / "frames",
        frame_cache_dir=tmp_path / "frame-cache",
        output_dir=tmp_path / "output",
        cache_dir=tmp_path / "cache",
        id=[],
        lane=[],
        dry_run=False,
        keep_frames=False,
        quality=90,
        force=False,
        jobs="3",
        capture_jobs="2",
    )
    worker_counts = []

    def bounded(items, worker, jobs, label):
        worker_counts.append(jobs)
        return [worker(item) for item in items]

    def ensure_frames(preview, *unused, **unused_kwargs):
        return animations.gallery_frames.FrameSequence(
            preview=preview,
            frame_dir=tmp_path / preview.id,
            input_hash=f"input-{preview.id}",
            frames_hash=f"frames-{preview.id}",
            generated=False,
            reason="current",
        )

    def encode_preview(preview, *unused):
        return animations.AnimationResult(True, (f"encoded {preview.id}",))

    monkeypatch.setattr(animations, "parse_args", lambda: args)
    monkeypatch.setattr(animations, "collect_previews", lambda *unused: previews)
    monkeypatch.setattr(animations.gallery_workers, "bounded_parallel_map", bounded)
    monkeypatch.setattr(animations.gallery_frames, "ensure_frames", ensure_frames)
    monkeypatch.setattr(animations, "encode_preview", encode_preview)

    assert animations.main() == 0

    output = capsys.readouterr().out
    assert worker_counts == [2, 3]
    assert output.index("encoded first") < output.index("encoded second")
    assert "jobs=3 capture_jobs=2" in output
    assert "animation_cache_misses=2" in output


def test_dry_run_does_not_start_workers(tmp_path: Path, monkeypatch) -> None:
    preview = _preview(tmp_path, "example")
    args = argparse.Namespace(
        manifest=tmp_path / "manifest.yaml",
        build_examples_dir=tmp_path / "examples",
        frame_dir=tmp_path / "frames",
        frame_cache_dir=tmp_path / "frame-cache",
        output_dir=tmp_path / "output",
        cache_dir=tmp_path / "cache",
        id=[],
        lane=[],
        dry_run=True,
        keep_frames=False,
        quality=90,
        force=False,
        jobs="3",
        capture_jobs="2",
    )
    monkeypatch.setattr(animations, "parse_args", lambda: args)
    monkeypatch.setattr(animations, "collect_previews", lambda *unused: [preview])
    monkeypatch.setattr(animations, "dry_run_preview", lambda *unused: None)
    monkeypatch.setattr(
        animations.gallery_workers,
        "bounded_parallel_map",
        lambda *unused: (_ for _ in ()).throw(AssertionError("workers started")),
    )

    assert animations.main() == 0
