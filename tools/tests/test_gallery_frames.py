from dataclasses import replace
from pathlib import Path
import sys


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import gallery_frames  # noqa: E402


def _entry(example_id: str, fps: int = 30) -> dict:
    return {
        "id": example_id,
        "title": "Editorial title",
        "source": f"examples/c/features/{example_id}.c",
        "tags": ["editorial"],
        "capture": {"size": "1280x720"},
        "media": {
            "preview": {
                "kind": "animated-webp",
                "frames": 60,
                "fps": fps,
            }
        },
    }


def test_capture_manifest_hash_ignores_editorial_metadata() -> None:
    first = _entry("example")
    second = {**first, "title": "Changed title", "tags": ["changed"]}

    assert gallery_frames.capture_manifest_hash(first) == gallery_frames.capture_manifest_hash(
        second
    )


def test_capture_manifest_hash_tracks_preview_inputs() -> None:
    first = _entry("example", fps=30)
    second = _entry("example", fps=60)

    assert gallery_frames.capture_manifest_hash(first) != gallery_frames.capture_manifest_hash(
        second
    )


def test_frame_input_hash_does_not_hash_whole_manifest(tmp_path: Path) -> None:
    executable = tmp_path / "example"
    executable.write_bytes(b"executable")
    preview = gallery_frames.AnimatedPreview(
        id="example",
        title="Example",
        lane="features",
        source="examples/c/features/missing.c",
        executable=executable,
        frames=2,
        fps=30,
        sample_stride=1,
        time_scale=1.0,
        size="1280x720",
        manifest_input_hash="entry-a",
    )
    first_manifest = tmp_path / "first.yaml"
    second_manifest = tmp_path / "second.yaml"
    first_manifest.write_text("unrelated: first\n", encoding="utf8")
    second_manifest.write_text("unrelated: second\n", encoding="utf8")

    first = gallery_frames.input_hash_for(preview, first_manifest, tmp_path / "frames")
    second = gallery_frames.input_hash_for(preview, second_manifest, tmp_path / "frames")
    changed = gallery_frames.input_hash_for(
        replace(preview, manifest_input_hash="entry-b"),
        first_manifest,
        tmp_path / "frames",
    )

    assert first == second
    assert first != changed


def test_frame_input_hash_tracks_prepared_dataset_content(tmp_path: Path) -> None:
    executable = tmp_path / "example"
    executable.write_bytes(b"executable")
    preview = gallery_frames.AnimatedPreview(
        id="example",
        title="Example",
        lane="showcases",
        source="examples/c/showcases/missing.c",
        executable=executable,
        frames=2,
        fps=30,
        sample_stride=1,
        time_scale=1.0,
        size="1280x720",
        manifest_input_hash="entry",
        dataset_input_hash="dataset-a",
    )

    first = gallery_frames.input_hash_for(preview, tmp_path / "manifest", tmp_path / "frames")
    changed = gallery_frames.input_hash_for(
        replace(preview, dataset_input_hash="dataset-b"),
        tmp_path / "manifest",
        tmp_path / "frames",
    )

    assert first != changed
