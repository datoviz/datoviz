"""Regression tests for the release automation CLI."""

import datetime as dt
import importlib.util
import json
import sys
from pathlib import Path
from types import SimpleNamespace

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


def test_rc2_machine_matrix_records_physical_host_exceptions() -> None:
    """RC2 requires the available M3 without pretending hosted CI is physical evidence."""
    release_automation = _load_release_automation()

    rows = {
        row["class"]: row
        for row in release_automation.machine_matrix([], version="0.4.0rc2")
    }

    assert rows["macos-arm64"]["status"] == "missing"
    assert rows["linux-x86_64-vulkan"]["status"] == "excluded-unavailable"
    assert rows["windows-amd64"]["status"] == "excluded-unavailable"
    assert "hosted Linux" in rows["linux-x86_64-vulkan"]["exception"]
    assert "hosted Windows" in rows["windows-amd64"]["exception"]


def test_rc2_machine_exceptions_do_not_weaken_rc3() -> None:
    """The narrow RC2 exception must not change later-candidate physical requirements."""
    release_automation = _load_release_automation()

    rows = {
        row["class"]: row
        for row in release_automation.machine_matrix([], version="0.4.0rc3")
    }

    assert rows["macos-arm64"]["status"] == "missing"
    assert rows["linux-x86_64-vulkan"]["status"] == "missing"
    assert rows["windows-amd64"]["status"] == "missing"


def test_rc2_command_plan_discloses_machine_exceptions() -> None:
    """Maintainer-facing plans make the reduced physical matrix explicit."""
    release_automation = _load_release_automation()

    plan = release_automation.command_plan("0.4.0rc2")

    assert "linux-x86_64-vulkan: required=excluded-unavailable" in plan
    assert "windows-amd64: required=excluded-unavailable" in plan
    assert "No physical Linux host is available for RC2" in plan
    assert "No physical Windows host is available for RC2" in plan

    machine_plan = release_automation.machine_plan_text("0.4.0rc2")
    assert "no physical action; satisfy the hosted evidence" in machine_plan


def test_github_draft_marks_release_candidate_as_prerelease(tmp_path, monkeypatch) -> None:
    """A draft for an RC must retain GitHub's prerelease classification."""
    release_automation = _load_release_automation()
    monkeypatch.setattr(release_automation, "ROOT", tmp_path)
    (tmp_path / "notes.md").write_text("RC notes\n", encoding="utf8")
    (tmp_path / "candidate.whl").write_bytes(b"wheel")
    state = {
        "tag": "v0.4.0rc2",
        "artifacts": [
            {"kind": "release-notes", "path": "notes.md"},
            {"kind": "wheel", "path": "candidate.whl"},
        ],
    }
    monkeypatch.setattr(
        release_automation,
        "require_rehearsal_ready",
        lambda version, allow_incomplete: {"state": state},
    )
    commands = []
    monkeypatch.setattr(
        release_automation,
        "run_checked",
        lambda argv, dry_run: commands.append(argv) or 0,
    )

    result = release_automation.github_draft(
        SimpleNamespace(
            version="0.4.0rc2",
            confirm="no",
            dry_run=True,
            allow_incomplete=False,
            allow_missing_tag=True,
            notes_file=None,
        )
    )

    assert result == 0
    assert commands[0][:4] == ["gh", "release", "create", "v0.4.0rc2"]
    assert "--prerelease" in commands[0]


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
