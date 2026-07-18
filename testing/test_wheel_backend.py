from __future__ import annotations

import json
import zipfile
from pathlib import Path

import pytest

from tools.datoviz_build_backend import native_payload, repair
from tools.datoviz_build_backend import wheel as wheel_backend
from tools.datoviz_build_backend.config import parse_config_settings
from tools.datoviz_build_backend.manifest import PayloadEntry, write_manifest
from tools.datoviz_build_backend.native_payload import _stage_native
from tools.datoviz_build_backend.tags import repair_input_platform_tag, wheel_tags
from tools.datoviz_build_backend.validate import validate_dist, validate_wheel
from tools.datoviz_build_backend.wheel import build_from_stage, write_wheel_from_stage


def _write_project(root: Path) -> None:
    (root / "README.md").write_text("# Datoviz test\n", encoding="utf8")
    (root / "pyproject.toml").write_text(
        """
[project]
name = "datoviz"
version = "0.4.0.dev0"
requires-python = ">=3.10"
dependencies = ["numpy"]
description = "test"
readme = "README.md"
license = { text = "MIT" }

[project.scripts]
datoviz-config = "datoviz.cli:main"
""".lstrip(),
        encoding="utf8",
    )


def _write_stage(root: Path) -> Path:
    stage = root / "stage"
    package = stage / "datoviz"
    package.mkdir(parents=True)
    (package / "__init__.py").write_text("__version__ = '0.4.0.dev0'\n", encoding="utf8")
    (package / "cli.py").write_text("def main(): return 0\n", encoding="utf8")
    manifest = package / "_wheel_payload.json"
    write_manifest(
        [
            PayloadEntry(
                source=str(package / "__init__.py"),
                wheel_path="datoviz/__init__.py",
                kind="python",
                required=True,
                reason="python-package",
            ),
            PayloadEntry(
                source=str(package / "cli.py"),
                wheel_path="datoviz/cli.py",
                kind="python",
                required=True,
                reason="python-package",
            ),
            PayloadEntry(
                source=str(manifest),
                wheel_path="datoviz/_wheel_payload.json",
                kind="metadata",
                required=True,
                reason="payload-manifest",
            ),
        ],
        manifest,
    )
    return stage


def test_parse_release_config_namespaced(tmp_path: Path) -> None:
    _write_project(tmp_path)
    config = parse_config_settings(
        {
            "datoviz.release-wheel": "true",
            "datoviz.platform-tag": "manylinux_2_34_x86_64",
            "datoviz.native-build-dir": "native",
            "datoviz.include-qtbridge": "yes",
            "datoviz.skip-repair": "1",
        },
        root=tmp_path,
    )

    assert config.release_wheel is True
    assert config.platform_tag == "manylinux_2_34_x86_64"
    assert config.native_build_dir == tmp_path / "native"
    assert config.include_qtbridge is True
    assert config.skip_repair is True


def test_parse_config_rejects_unknown_datoviz_setting(tmp_path: Path) -> None:
    _write_project(tmp_path)
    with pytest.raises(ValueError, match="unknown Datoviz"):
        parse_config_settings({"datoviz.unknown": "1"}, root=tmp_path)


def test_wheel_cmake_config_requires_c11() -> None:
    root = Path(__file__).resolve().parents[1]
    config = (root / "cmake" / "DatovizConfig.cmake.wheel").read_text(encoding="utf8")

    assert 'INTERFACE_COMPILE_FEATURES "c_std_11"' in config


def test_stage_macos_native_normalizes_vulkan_runtime(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    _write_project(tmp_path)
    native = tmp_path / "native"
    (native / "src").mkdir(parents=True)
    (native / "src" / "libdatoviz.dylib").write_bytes(b"datoviz")
    runtime = tmp_path / "runtime"
    runtime.mkdir()
    (runtime / "libshaderc_shared.1.dylib").write_bytes(b"shaderc")
    (runtime / "libvulkan.1.4.350.dylib").write_bytes(b"vulkan")
    (runtime / "libMoltenVK.dylib").write_bytes(b"moltenvk")

    config = parse_config_settings(
        {
            "datoviz.release-wheel": "true",
            "datoviz.native-build-dir": "native",
            "datoviz.stage-dir": "stage",
        },
        root=tmp_path,
    )
    package = config.stage_dir / "datoviz"
    monkeypatch.setattr(native_payload.platform, "system", lambda: "Darwin")
    monkeypatch.setenv("DVZ_WHEEL_RUNTIME_DIRS", str(runtime))

    entries = _stage_native(config, package)

    assert {path.name for path in package.iterdir()} == {
        "MoltenVK_icd.json",
        "libMoltenVK.dylib",
        "libdatoviz.dylib",
        "libshaderc_shared.1.dylib",
        "libvulkan.1.dylib",
    }
    icd = json.loads((package / "MoltenVK_icd.json").read_text(encoding="utf8"))
    assert icd == {
        "file_format_version": "1.0.0",
        "ICD": {
            "library_path": "./libMoltenVK.dylib",
            "api_version": "1.4.0",
            "is_portability_driver": True,
        },
    }
    reasons = {entry.wheel_path: entry.reason for entry in entries}
    assert reasons["datoviz/libvulkan.1.dylib"] == "vulkan-loader"
    assert reasons["datoviz/libMoltenVK.dylib"] == "moltenvk"
    assert reasons["datoviz/MoltenVK_icd.json"] == "moltenvk-icd"


def test_stage_linux_native_normalizes_shaderc_runtime_name(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    """Linux staging uses the runtime basename baked into libdatoviz, not a devel alias."""
    _write_project(tmp_path)
    native = tmp_path / "native"
    (native / "src").mkdir(parents=True)
    (native / "src" / "libdatoviz.so").write_bytes(b"datoviz")
    (native / "CMakeCache.txt").write_text(
        "DVZ_SHADERC_RUNTIME_LIBRARY:STRING=libshaderc_shared.so.1\n", encoding="utf8"
    )
    runtime = tmp_path / "runtime"
    runtime.mkdir()
    (runtime / "libshaderc_shared.so").write_bytes(b"shaderc")
    config = parse_config_settings(
        {
            "datoviz.release-wheel": "true",
            "datoviz.native-build-dir": "native",
            "datoviz.stage-dir": "stage",
        },
        root=tmp_path,
    )
    package = config.stage_dir / "datoviz"
    monkeypatch.setattr(native_payload.platform, "system", lambda: "Linux")
    monkeypatch.setenv("DVZ_WHEEL_RUNTIME_DIRS", str(runtime))

    entries = _stage_native(config, package)

    assert {path.name for path in package.iterdir()} == {
        "libdatoviz.so",
        "libshaderc_shared.so.1",
    }
    shaderc = next(entry for entry in entries if entry.reason == "shaderc-runtime")
    assert shaderc.source == str(runtime / "libshaderc_shared.so")
    assert shaderc.wheel_path == "datoviz/libshaderc_shared.so.1"


def test_stage_macos_native_requires_moltenvk(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    _write_project(tmp_path)
    native = tmp_path / "native"
    (native / "src").mkdir(parents=True)
    (native / "src" / "libdatoviz.dylib").write_bytes(b"datoviz")
    runtime = tmp_path / "runtime"
    runtime.mkdir()
    (runtime / "libshaderc_shared.1.dylib").write_bytes(b"shaderc")
    (runtime / "libvulkan.1.dylib").write_bytes(b"vulkan")

    config = parse_config_settings(
        {
            "datoviz.release-wheel": "true",
            "datoviz.native-build-dir": "native",
            "datoviz.stage-dir": "stage",
        },
        root=tmp_path,
    )
    monkeypatch.setattr(native_payload.platform, "system", lambda: "Darwin")
    monkeypatch.setenv("DVZ_WHEEL_RUNTIME_DIRS", str(runtime))

    with pytest.raises(FileNotFoundError, match="libMoltenVK"):
        _stage_native(config, config.stage_dir / "datoviz")


def test_stage_windows_native_uses_configured_build_dir(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    _write_project(tmp_path)
    native = tmp_path / "native"
    (native / "src").mkdir(parents=True)
    (native / "src" / "datoviz.dll").write_bytes(b"msvc dll")
    (native / "src" / "datoviz.lib").write_bytes(b"msvc import library")
    (native / "CMakeCache.txt").write_text(
        "DVZ_HAS_SHADERC:INTERNAL=1\nDVZ_SHADERC_STATIC:INTERNAL=1\n", encoding="utf8"
    )
    runtime = tmp_path / "runtime"
    runtime.mkdir(parents=True)
    (runtime / "runtime.dll").write_bytes(b"runtime")

    stale = tmp_path / "build" / "src"
    stale.mkdir(parents=True)
    (stale / "libdatoviz.dll").write_bytes(b"stale mingw dll")
    (stale / "libdatoviz.dll.a").write_bytes(b"stale mingw import library")

    mingw = tmp_path / "mingw" / "bin"
    mingw.mkdir(parents=True)
    (mingw / "gcc.exe").write_bytes(b"")
    for name in ("libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll"):
        (mingw / name).write_bytes(b"mingw runtime")

    config = parse_config_settings(
        {
            "datoviz.release-wheel": "true",
            "datoviz.native-build-dir": "native",
            "datoviz.stage-dir": "stage",
        },
        root=tmp_path,
    )
    package = tmp_path / "stage" / "datoviz"
    monkeypatch.setattr(native_payload.platform, "system", lambda: "Windows")
    monkeypatch.setattr(native_payload.shutil, "which", lambda name: str(mingw / "gcc.exe"))
    monkeypatch.setenv("DVZ_WHEEL_RUNTIME_DIRS", str(runtime))

    entries = _stage_native(config, package)

    assert {path.name for path in package.iterdir()} == {
        "datoviz.dll",
        "datoviz.lib",
        "runtime.dll",
    }
    assert {entry.wheel_path for entry in entries} == {
        "datoviz/datoviz.dll",
        "datoviz/datoviz.lib",
        "datoviz/runtime.dll",
    }


def test_stage_windows_native_rejects_missing_shared_shaderc(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    _write_project(tmp_path)
    native = tmp_path / "native"
    (native / "src").mkdir(parents=True)
    (native / "src" / "datoviz.dll").write_bytes(b"msvc dll")
    (native / "CMakeCache.txt").write_text(
        "DVZ_HAS_SHADERC:INTERNAL=1\nDVZ_SHADERC_STATIC:INTERNAL=0\n", encoding="utf8"
    )

    config = parse_config_settings(
        {
            "datoviz.release-wheel": "true",
            "datoviz.native-build-dir": "native",
            "datoviz.stage-dir": "stage",
        },
        root=tmp_path,
    )
    monkeypatch.setattr(native_payload.platform, "system", lambda: "Windows")
    monkeypatch.setattr(native_payload, "_stage_c_integration", lambda *_: [])

    with pytest.raises(FileNotFoundError, match="requires shaderc"):
        native_payload.stage_payload(config, clean=True)


def test_stage_windows_native_records_static_shaderc_policy(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    _write_project(tmp_path)
    native = tmp_path / "native"
    (native / "src").mkdir(parents=True)
    (native / "src" / "datoviz.dll").write_bytes(b"msvc dll")
    (native / "CMakeCache.txt").write_text(
        "DVZ_HAS_SHADERC:INTERNAL=1\nDVZ_SHADERC_STATIC:INTERNAL=1\n", encoding="utf8"
    )

    config = parse_config_settings(
        {
            "datoviz.release-wheel": "true",
            "datoviz.native-build-dir": "native",
            "datoviz.stage-dir": "stage",
        },
        root=tmp_path,
    )
    monkeypatch.setattr(native_payload.platform, "system", lambda: "Windows")
    monkeypatch.setattr(native_payload, "_stage_c_integration", lambda *_: [])

    native_payload.stage_payload(config, clean=True)

    manifest = json.loads((config.stage_dir / "datoviz" / "_wheel_payload.json").read_text())
    assert manifest["metadata"]["shaderc"] == {"mode": "static"}


def test_direct_wheel_writer_validates_record_and_manifest(tmp_path: Path) -> None:
    _write_project(tmp_path)
    stage = _write_stage(tmp_path)

    wheel = write_wheel_from_stage(
        stage,
        tmp_path / "dist",
        "manylinux_2_34_x86_64",
        root=tmp_path,
    )

    validate_wheel(wheel)
    with zipfile.ZipFile(wheel) as zf:
        names = set(zf.namelist())
    assert "datoviz-0.4.0.dev0.dist-info/METADATA" in names
    assert "datoviz-0.4.0.dev0.dist-info/RECORD" in names
    assert "datoviz/_wheel_payload.json" in names


def test_manylinux_build_uses_neutral_input_tag(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    _write_project(tmp_path)
    stage = _write_stage(tmp_path)
    captured: dict[str, object] = {}

    def fake_repair(
        wheel: Path, *, skip: bool = False, platform_tag: str | None = None
    ) -> Path:
        captured.update(wheel=wheel, skip=skip, platform_tag=platform_tag)
        return wheel

    monkeypatch.setattr(wheel_backend, "repair_wheel", fake_repair)
    built = build_from_stage(
        stage,
        tmp_path / "dist",
        "manylinux_2_34_x86_64",
        root=tmp_path,
    )

    assert built.name.endswith("-py3-none-linux_x86_64.whl")
    assert captured == {
        "wheel": built,
        "skip": False,
        "platform_tag": "manylinux_2_34_x86_64",
    }
    assert repair_input_platform_tag("manylinux_2_34_aarch64") == "linux_aarch64"


def test_linux_repair_requests_exact_manylinux_policy(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    _write_project(tmp_path)
    stage = _write_stage(tmp_path)
    wheel = write_wheel_from_stage(stage, tmp_path / "dist", "linux_x86_64", root=tmp_path)
    commands: list[list[str]] = []

    monkeypatch.setattr(repair.shutil, "which", lambda name: f"/usr/bin/{name}")

    def fake_run(command: list[str], *, check: bool) -> None:
        assert check is True
        commands.append(command)
        if command[1] != "repair":
            return
        output = Path(command[command.index("-w") + 1])
        repaired = output / wheel.name.replace("linux_x86_64", "manylinux_2_34_x86_64")
        repaired.write_bytes(wheel.read_bytes())

    monkeypatch.setattr(repair.subprocess, "run", fake_run)
    repaired = repair._repair_linux(wheel, platform_tag="manylinux_2_34_x86_64")

    assert commands[1][1:4] == ["repair", "--plat", "manylinux_2_34_x86_64"]
    assert repaired.name.endswith("-py3-none-manylinux_2_34_x86_64.whl")


def test_compressed_wheel_tags_are_valid_but_not_release_evidence(tmp_path: Path) -> None:
    _write_project(tmp_path)
    stage = _write_stage(tmp_path)
    dist = tmp_path / "dist"
    wheel = write_wheel_from_stage(stage, dist, "manylinux_2_34_x86_64", root=tmp_path)
    compressed = wheel.with_name(
        wheel.name.replace(
            "manylinux_2_34_x86_64", "manylinux_2_34_x86_64.manylinux_2_39_x86_64"
        )
    )
    with zipfile.ZipFile(wheel) as zf:
        files = {
            info.filename: zf.read(info.filename)
            for info in zf.infolist()
            if not info.filename.endswith("/")
        }
    wheel_meta = next(name for name in files if name.endswith(".dist-info/WHEEL"))
    files[wheel_meta] = files[wheel_meta].replace(
        b"Tag: py3-none-manylinux_2_34_x86_64\n",
        b"Tag: py3-none-manylinux_2_34_x86_64\n"
        b"Tag: py3-none-manylinux_2_39_x86_64\n",
    )
    record = next(name for name in files if name.endswith(".dist-info/RECORD"))
    files[record] = repair._record_bytes(files, record)
    repair._write_wheel_files(compressed, files)
    wheel.unlink()

    assert wheel_tags(compressed) == {
        "py3-none-manylinux_2_34_x86_64",
        "py3-none-manylinux_2_39_x86_64",
    }
    validate_wheel(compressed)
    with pytest.raises(RuntimeError, match="release artifact must have exactly one tag"):
        validate_dist(
            dist,
            version="0.4.0.dev0",
            platform_tags=["manylinux_2_34_x86_64"],
        )
