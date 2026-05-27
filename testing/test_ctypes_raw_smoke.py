#!/usr/bin/env python3
"""pytest wrapper for the raw ctypes smoke script."""

from __future__ import annotations

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


@pytest.mark.skipif(not _library_exists(), reason='libdatoviz has not been built')
def test_raw_ctypes_import_time_and_scene_smoke():
    subprocess.run(
        [sys.executable, 'tools/bindings/ctypes_smoke.py'],
        cwd=ROOT_DIR,
        check=True,
    )
