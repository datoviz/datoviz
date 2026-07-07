#!/usr/bin/env python3
"""pytest wrapper for the raw ctypes smoke script."""

from __future__ import annotations

import ctypes
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
def test_panel_background_descriptor_layout_is_usable_from_python():
    import datoviz.raw as dvz

    desc = dvz.dvz_panel_background_desc()
    assert desc.struct_size == ctypes.sizeof(dvz.DvzPanelBackgroundDesc)
    assert isinstance(desc.gradient, dvz.DvzPanelBackgroundGradient)
    assert isinstance(desc.image, dvz.DvzPanelBackgroundImage)

    desc.type = dvz.DVZ_PANEL_BACKGROUND_LINEAR_GRADIENT
    desc.gradient.start[:] = (0.0, 0.0)
    desc.gradient.end[:] = (1.0, 1.0)
    desc.gradient.color0[:] = (0.010, 0.030, 0.065, 1.0)
    desc.gradient.color1[:] = (0.025, 0.345, 0.380, 1.0)
    assert tuple(desc.gradient.end) == (1.0, 1.0)
    assert desc.gradient.color1[2] == pytest.approx(0.380)

    pixels = (ctypes.c_uint8 * 16)()
    desc.type = dvz.DVZ_PANEL_BACKGROUND_IMAGE
    desc.image.rgba = ctypes.cast(pixels, ctypes.c_void_p)
    desc.image.width = 2
    desc.image.height = 2
    assert desc.image.rgba
    assert (desc.image.width, desc.image.height) == (2, 2)


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
