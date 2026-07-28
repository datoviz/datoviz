"""CPU-only checks for the hardware-gated CuPy image smoke oracle."""

from __future__ import annotations

import importlib.util
from pathlib import Path

import numpy as np
import pytest

_SMOKE_PATH = Path(__file__).resolve().parents[1] / 'tools' / 'bindings' / 'ctypes_cupy_smoke.py'
_SMOKE_SPEC = importlib.util.spec_from_file_location('ctypes_cupy_smoke', _SMOKE_PATH)
assert _SMOKE_SPEC is not None and _SMOKE_SPEC.loader is not None
_SMOKE = importlib.util.module_from_spec(_SMOKE_SPEC)
_SMOKE_SPEC.loader.exec_module(_SMOKE)
_validate_image_capture = _SMOKE._validate_image_capture


def test_cupy_image_smoke_oracle_accepts_red_to_green() -> None:
    before = np.zeros((64, 64, 4), dtype=np.uint8)
    after = np.zeros_like(before)
    before[..., :] = (255, 0, 0, 255)
    after[..., :] = (0, 255, 0, 255)

    red, green = _validate_image_capture(before, after)

    assert red == (255, 0, 0)
    assert green == (0, 255, 0)


@pytest.mark.parametrize(
    ('before_color', 'after_color'),
    (
        ((0, 0, 255, 255), (0, 255, 0, 255)),
        ((255, 0, 0, 255), (255, 0, 0, 255)),
    ),
)
def test_cupy_image_smoke_oracle_rejects_wrong_or_unchanged_frames(
    before_color, after_color
) -> None:
    before = np.zeros((64, 64, 4), dtype=np.uint8)
    after = np.zeros_like(before)
    before[..., :] = before_color
    after[..., :] = after_color

    with pytest.raises(RuntimeError):
        _validate_image_capture(before, after)
