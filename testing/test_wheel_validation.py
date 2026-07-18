"""Focused regression tests for installed-wheel validation."""

from __future__ import annotations

import subprocess
from pathlib import Path
from types import SimpleNamespace

import pytest

from tools.datoviz_build_backend import validate


def test_render_smoke_accepts_capability_skip(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    """Exit code 77 represents an unavailable graphics capability."""
    monkeypatch.setattr(
        validate.subprocess, "run", lambda *args, **kwargs: SimpleNamespace(returncode=77)
    )

    validate._render_smoke(Path("python"), tmp_path)


def test_render_smoke_rejects_process_failure(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    """Unexpected render-process failures remain fatal."""
    monkeypatch.setattr(
        validate.subprocess, "run", lambda *args, **kwargs: SimpleNamespace(returncode=1)
    )

    with pytest.raises(subprocess.CalledProcessError):
        validate._render_smoke(Path("python"), tmp_path)


def test_render_smoke_requires_output_after_success(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    """A successful process must produce a nonempty capture."""
    monkeypatch.setattr(
        validate.subprocess, "run", lambda *args, **kwargs: SimpleNamespace(returncode=0)
    )

    with pytest.raises(RuntimeError, match="render smoke output is missing"):
        validate._render_smoke(Path("python"), tmp_path)


def test_render_smoke_accepts_nonempty_output(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    """A successful process and nonempty capture pass validation."""

    def run(*args, **kwargs):
        (tmp_path / "python_render_example.png").write_bytes(b"PNG")
        return SimpleNamespace(returncode=0)

    monkeypatch.setattr(validate.subprocess, "run", run)

    validate._render_smoke(Path("python"), tmp_path)


@pytest.mark.parametrize("required", [False, True])
def test_shader_resource_smoke_propagates_precompiled_requirement(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, required: bool
) -> None:
    """Release callers can require a real precompiled shader payload."""
    commands: list[list[str]] = []
    monkeypatch.setattr(validate, "_run", lambda command, **kwargs: commands.append(command))

    validate._builtin_shader_resource_smoke(
        Path("python"), tmp_path, require_precompiled_shaders=required
    )

    assert f"required={required!r}" in commands[0][2]
