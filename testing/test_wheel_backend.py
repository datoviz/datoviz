from __future__ import annotations

import zipfile
from pathlib import Path

import pytest

from tools.datoviz_build_backend.config import parse_config_settings
from tools.datoviz_build_backend.manifest import PayloadEntry, write_manifest
from tools.datoviz_build_backend.wheel import write_wheel_from_stage
from tools.datoviz_build_backend.validate import validate_wheel


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


def test_direct_wheel_writer_validates_record_and_manifest(tmp_path: Path) -> None:
    _write_project(tmp_path)
    stage = tmp_path / "stage"
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

