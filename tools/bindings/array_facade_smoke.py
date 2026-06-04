#!/usr/bin/env python3
"""Smoke-test the top-level array-aware facade against libdatoviz."""

from __future__ import annotations

import sys
from pathlib import Path

import numpy as np


ROOT_DIR = Path(__file__).resolve().parents[2]


def main() -> int:
    sys.path.insert(0, str(ROOT_DIR))

    import datoviz as dvz  # noqa: PLC0415
    import datoviz.raw as raw  # noqa: PLC0415

    assert hasattr(dvz, 'dvz_scene')
    assert hasattr(raw, 'dvz_scene')
    assert dvz.dvz_visual_set_data is not raw.dvz_visual_set_data

    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError('dvz_scene() failed')

    try:
        visual = dvz.dvz_point(scene, 0)
        if not visual:
            raise RuntimeError('dvz_point() failed')

        positions = np.array(
            [
                [-0.5, -0.4, 0.0],
                [+0.5, -0.4, 0.0],
                [0.0, +0.5, 0.0],
            ],
            dtype=np.float32,
        )
        colors = np.array(
            [
                [255, 80, 80, 255],
                [80, 220, 120, 255],
                [90, 150, 255, 255],
            ],
            dtype=np.uint8,
        )
        diameters = np.array([18.0, 18.0, 18.0], dtype=np.float32)
        updated = np.array([[0.0, 0.0, 0.0]], dtype=np.float32)

        if dvz.dvz_visual_set_data(visual, 'position', positions) != 0:
            raise RuntimeError('facade dvz_visual_set_data(position) failed')
        if dvz.dvz_visual_set_data(visual, 'color', colors) != 0:
            raise RuntimeError('facade dvz_visual_set_data(color) failed')
        if dvz.dvz_visual_set_data(visual, 'diameter', diameters) != 0:
            raise RuntimeError('facade dvz_visual_set_data(diameter) failed')
        if dvz.dvz_visual_set_data_range(visual, 'position', updated, 1) != 0:
            raise RuntimeError('facade dvz_visual_set_data_range(position) failed')
    finally:
        dvz.dvz_scene_destroy(scene)

    print('array facade smoke: OK')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
