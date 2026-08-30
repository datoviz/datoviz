from pathlib import Path
import sys
from unittest.mock import patch


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import gallery_media  # noqa: E402


def test_prepared_dataset_fingerprint_tracks_declared_content(tmp_path: Path) -> None:
    prepared = tmp_path / "prepared"
    prepared.mkdir()
    payload = prepared / "values.f32"
    payload.write_bytes(b"first")
    entry = {
        "data": {"kind": "real"},
        "dataset": {"prepared_path": "prepared"},
    }

    first = gallery_media.prepared_dataset_fingerprint(entry, tmp_path)
    payload.write_bytes(b"second")
    second = gallery_media.prepared_dataset_fingerprint(entry, tmp_path)

    assert first != second


def test_prepared_dataset_fingerprint_tracks_missing_fallback_appearance(tmp_path: Path) -> None:
    entry = {
        "data": {"kind": "prepared"},
        "dataset": {
            "promoted_prepared_path": "data/prepared",
            "fallback_prepared_path": ".cache/prepared",
        },
    }

    missing = gallery_media.prepared_dataset_fingerprint(entry, tmp_path)
    fallback = tmp_path / ".cache/prepared"
    fallback.mkdir(parents=True)
    (fallback / "metadata.tsv").write_text("ready\n", encoding="utf8")
    available = gallery_media.prepared_dataset_fingerprint(entry, tmp_path)

    assert missing != available


def test_prepared_dataset_without_paths_uses_data_gitlink(tmp_path: Path) -> None:
    entry = {"data": {"kind": "prepared"}}
    first_result = type("Result", (), {"returncode": 0, "stdout": "a" * 40 + "\n"})()
    second_result = type("Result", (), {"returncode": 0, "stdout": "b" * 40 + "\n"})()

    with patch.object(gallery_media.subprocess, "run", return_value=first_result):
        first = gallery_media.prepared_dataset_fingerprint(entry, tmp_path)
    with patch.object(gallery_media.subprocess, "run", return_value=second_result):
        second = gallery_media.prepared_dataset_fingerprint(entry, tmp_path)

    assert first != second
