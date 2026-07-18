"""Focused regression tests for installed-wheel validation."""

from __future__ import annotations

import json
import subprocess
import zipfile
from pathlib import Path
from types import SimpleNamespace

import pytest

from tools.datoviz_build_backend import validate


def test_macos_runtime_payload_is_relocatable(tmp_path: Path) -> None:
    """The macOS wheel carries a canonical loader and sibling MoltenVK manifest."""

    archive = tmp_path / "runtime.zip"
    with zipfile.ZipFile(archive, "w") as zf:
        zf.writestr("datoviz/libvulkan.1.dylib", b"vulkan")
        zf.writestr("datoviz/libMoltenVK.dylib", b"moltenvk")
        zf.writestr(
            "datoviz/MoltenVK_icd.json",
            json.dumps(
                {
                    "file_format_version": "1.0.0",
                    "ICD": {
                        "library_path": "./libMoltenVK.dylib",
                        "api_version": "1.4.0",
                        "is_portability_driver": True,
                    },
                }
            ),
        )
    with zipfile.ZipFile(archive) as zf:
        validate._validate_macos_runtime(zf, {"py3-none-macosx_15_0_arm64"})


def test_macos_runtime_payload_rejects_external_moltenvk(tmp_path: Path) -> None:
    """An ICD manifest cannot escape the installed wheel directory."""

    archive = tmp_path / "runtime.zip"
    with zipfile.ZipFile(archive, "w") as zf:
        zf.writestr("datoviz/libvulkan.1.dylib", b"vulkan")
        zf.writestr("datoviz/libMoltenVK.dylib", b"moltenvk")
        zf.writestr(
            "datoviz/MoltenVK_icd.json",
            json.dumps(
                {
                    "ICD": {
                        "library_path": "/opt/homebrew/lib/libMoltenVK.dylib",
                        "is_portability_driver": True,
                    }
                }
            ),
        )
    with zipfile.ZipFile(archive) as zf:
        with pytest.raises(RuntimeError, match=r"sibling .*libMoltenVK"):
            validate._validate_macos_runtime(zf, {"py3-none-macosx_15_0_arm64"})


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


@pytest.mark.parametrize(
    ("version", "raises"),
    [("0.4.0rc1", False), ("0.4.0rc1 (DEBUG)", True)],
)
def test_release_build_smoke_rejects_debug_native_library(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    version: str,
    raises: bool,
) -> None:
    """Release-wheel validation rejects an installed Debug native payload."""

    monkeypatch.setattr(validate.subprocess, "check_output", lambda *args, **kwargs: version)
    if raises:
        with pytest.raises(RuntimeError, match="Debug native library"):
            validate._release_build_smoke(Path("python"), tmp_path)
    else:
        validate._release_build_smoke(Path("python"), tmp_path)


def test_release_silence_smoke_checks_default_and_opt_in(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    """Release validation requires silence by default and visible explicit opt-in."""

    calls: list[dict[str, str]] = []

    def run(*args, **kwargs):
        env = kwargs["env"]
        calls.append(env)
        stderr = "datoviz-release-log-opt-in\n" if env.get("DVZ_LOG_LEVEL") == "info" else ""
        return SimpleNamespace(returncode=0, stdout="", stderr=stderr)

    monkeypatch.setattr(validate.subprocess, "run", run)
    monkeypatch.setenv("DVZ_LOG_LEVEL", "debug")

    validate._release_silence_smoke(Path("python"), tmp_path)

    assert "DVZ_LOG_LEVEL" not in calls[0]
    assert calls[1]["DVZ_LOG_LEVEL"] == "info"
    assert calls[1]["DVZ_LOG_COLOR"] == "0"


def test_release_silence_smoke_rejects_default_output(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    """Any default Release output is a packaging failure."""

    monkeypatch.setattr(
        validate.subprocess,
        "run",
        lambda *args, **kwargs: SimpleNamespace(returncode=0, stdout="", stderr="unexpected"),
    )

    with pytest.raises(RuntimeError, match="emitted output"):
        validate._release_silence_smoke(Path("python"), tmp_path)
