#!/usr/bin/env python3
"""Point values mapped through a custom continuous color scale."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


def main() -> None:
    scene, figure, panel = ex.scene_panel()

    colors = (dvz.DvzColor * 4)(ex.BG, ex.CYAN, ex.GREEN, ex.YELLOW)
    colormap = dvz.dvz_colormap_custom(scene, b"python_colormap_scale", colors, 4)
    if not colormap:
        raise RuntimeError("dvz_colormap_custom() failed")

    scale_desc = dvz.dvz_scale_desc()
    scale_desc.kind = dvz.DVZ_SCALE_CONTINUOUS
    scale_desc.label = b"scalar value"
    scale = dvz.dvz_scale(scene, ctypes.byref(scale_desc))
    if not scale:
        raise RuntimeError("dvz_scale() failed")
    dvz.dvz_scale_set_domain(scale, 0.0, 1.0)
    dvz.dvz_scale_set_colormap(scale, colormap)

    positions = np.array(
        [
            [-0.62, -0.18, 0.0],
            [-0.30, 0.18, 0.0],
            [0.00, -0.08, 0.0],
            [0.30, 0.24, 0.0],
            [0.62, -0.14, 0.0],
        ],
        dtype=np.float32,
    )
    values = np.array([0.05, 0.30, 0.52, 0.74, 0.96], dtype=np.float32)
    diameters = np.array([48.0, 56.0, 64.0, 56.0, 48.0], dtype=np.float32)

    point = dvz.dvz_point(scene, 0)
    if not point:
        raise RuntimeError("dvz_point() failed")
    dvz.dvz_visual_set_attr_format(point, b"color", dvz.DVZ_VISUAL_ATTR_FORMAT_SCALAR_F32)
    dvz.dvz_visual_set_scale(point, b"color", scale)
    dvz.dvz_visual_set_data_many(
        point,
        {
            "position": positions,
            "color": values,
            "diameter_px": diameters,
        },
    )
    ex.set_filled_point_style(point)
    dvz.dvz_visual_set_depth_test(point, False)
    ex.add_visual(panel, point)

    ex.run(scene, figure, "Scalar Color Scale")


if __name__ == "__main__":
    main()
