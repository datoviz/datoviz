#!/usr/bin/env python3
"""pytest wrapper for the raw ctypes smoke script."""

from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path

import pytest


ROOT_DIR = Path(__file__).resolve().parents[1]


def _library_exists() -> bool:
    names = ('libdatoviz.so', 'libdatoviz.dylib', 'libdatoviz.dll')
    return any((ROOT_DIR / 'build' / 'src' / name).exists() for name in names) or any(
        (ROOT_DIR / 'build' / name).exists() for name in names
    )


def test_raw_import_reports_missing_generated_binding(tmp_path, monkeypatch):
    package_dir = tmp_path / 'probe'
    package_dir.mkdir()
    (package_dir / '__init__.py').write_text('')
    shutil.copy2(ROOT_DIR / 'datoviz' / 'raw.py', package_dir / 'raw.py')
    monkeypatch.syspath_prepend(str(tmp_path))

    with pytest.raises(RuntimeError, match='just ctypes'):
        __import__('probe.raw')


@pytest.mark.skipif(not _library_exists(), reason='libdatoviz has not been built')
def test_raw_ctypes_import_time_and_scene_smoke():
    subprocess.run(
        [sys.executable, 'tools/bindings/ctypes_smoke.py'],
        cwd=ROOT_DIR,
        check=True,
    )


@pytest.mark.skipif(not _library_exists(), reason='libdatoviz has not been built')
def test_raw_ctypes_offscreen_render_smoke():
    subprocess.run(
        [sys.executable, 'tools/bindings/ctypes_render_smoke.py'],
        cwd=ROOT_DIR,
        check=True,
    )
