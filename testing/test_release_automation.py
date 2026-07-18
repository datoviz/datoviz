"""Regression tests for the release automation CLI."""

import datetime as dt
import importlib.util
import json
import sys
from pathlib import Path

ROOT_DIR = Path(__file__).resolve().parents[1]
TOOL_PATH = ROOT_DIR / "tools" / "release_automation.py"


def _load_release_automation():
    spec = importlib.util.spec_from_file_location("release_automation", TOOL_PATH)
    assert spec is not None
    assert spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def test_utc_now_is_python_310_compatible_utc_timestamp() -> None:
    """Ensure release timestamps use APIs available on Python 3.10."""
    release_automation = _load_release_automation()
    timestamp = release_automation.utc_now()

    assert timestamp.endswith("Z")
    parsed = dt.datetime.fromisoformat(timestamp.removesuffix("Z") + "+00:00")
    assert parsed.tzinfo == dt.timezone.utc
    assert parsed.microsecond == 0


def test_conformance_campaign_matches_candidate_wheels(tmp_path, monkeypatch) -> None:
    """Accepted campaign evidence supersedes unavailable legacy physical hosts."""
    release_automation = _load_release_automation()
    monkeypatch.setattr(release_automation, "ROOT", tmp_path)
    monkeypatch.setattr(release_automation.subprocess, "call", lambda *args, **kwargs: 0)
    report_path = tmp_path / "report.json"
    report_path.write_text(
        json.dumps(
            {
                "schema": release_automation.CONFORMANCE_REPORT_SCHEMA,
                "status": "pass",
                "campaign": {
                    "wheel_run_id": "123",
                    "artifact_commit": "artifact",
                    "validator_commit": "validator",
                },
                "gates": {"required_machine_coverage": "pass"},
                "missing_machines": [],
                "evidence": [
                    {
                        "machine_id": "cloud-linux-x86_64-lavapipe",
                        "artifact_checksums": {
                            "wheel": {"name": "datoviz-0.4.0rc1-linux.whl", "sha256": "abc"}
                        },
                    }
                ],
            }
        ),
        encoding="utf8",
    )
    artifacts = [
        {"kind": "wheel", "name": "datoviz-0.4.0rc1-linux.whl", "sha256": "abc"},
        {"kind": "conformance-report", "path": "report.json"},
    ]

    result = release_automation.conformance_campaign({"commit": "release"}, artifacts)

    assert result["status"] == "pass"
    assert result["wheel_run_id"] == "123"
    assert result["machines"] == ["cloud-linux-x86_64-lavapipe"]


def test_conformance_campaign_rejects_other_wheel_checksums(tmp_path, monkeypatch) -> None:
    """A green report cannot validate a different candidate wheel payload."""
    release_automation = _load_release_automation()
    monkeypatch.setattr(release_automation, "ROOT", tmp_path)
    monkeypatch.setattr(release_automation.subprocess, "call", lambda *args, **kwargs: 0)
    report_path = tmp_path / "report.json"
    report_path.write_text(
        json.dumps(
            {
                "schema": release_automation.CONFORMANCE_REPORT_SCHEMA,
                "status": "pass",
                "campaign": {"artifact_commit": "artifact"},
                "gates": {"required_machine_coverage": "pass"},
                "missing_machines": [],
                "evidence": [
                    {
                        "machine_id": "cloud-linux",
                        "artifact_checksums": {
                            "wheel": {"name": "datoviz-0.4.0rc1-linux.whl", "sha256": "old"}
                        },
                    }
                ],
            }
        ),
        encoding="utf8",
    )
    artifacts = [
        {"kind": "wheel", "name": "datoviz-0.4.0rc1-linux.whl", "sha256": "new"},
        {"kind": "conformance-report", "path": "report.json"},
    ]

    result = release_automation.conformance_campaign({"commit": "release"}, artifacts)

    assert result["status"] == "fail"
    assert "checksums differ" in result["detail"]
