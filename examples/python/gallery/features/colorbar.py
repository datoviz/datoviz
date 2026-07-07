#!/usr/bin/env python3
"""Scalar sampled-field image with a retained continuous colorbar."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


def main() -> None:
    scene, figure, panel = ex.scene_panel()
    dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_X, 0.0, 1.0)
    dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_Y, 0.0, 1.0)

    width, height = 192, 144
    x = np.linspace(0.0, 1.0, width, dtype=np.float32)
    y = np.linspace(0.0, 1.0, height, dtype=np.float32)
    u, v = np.meshgrid(x, y)
    values = 0.18 + 0.35 * u + 0.28 * v
    values += 0.09 * np.sin(2.0 * np.pi * (1.7 * u + 0.25 * v))
    values += 0.34 * np.exp(-(((u - 0.68) ** 2 + 1.8 * (v - 0.54) ** 2) / (2.0 * 0.055**2)))
    values = np.clip(values, 0.0, 1.0).astype(np.float32)

    scale = ex.continuous_scale(scene, b"python_feature_colorbar")
    field = dvz.dvz_sampled_field_from_array(scene, values)
    ex.add_image(scene, panel, field, scale=scale)

    desc = dvz.dvz_colorbar_desc()
    desc.orientation = dvz.DVZ_COLORBAR_ORIENTATION_VERTICAL
    desc.anchor = dvz.DVZ_SCENE_ANCHOR_PANEL_RIGHT
    desc.title = b"intensity"
    desc.reserve_px = 110.0
    desc.ramp_width_px = 28.0
    desc.plot_gap_px = 14.0
    desc.tick_length_px = 6.0
    desc.label_gap_px = 7.0
    colorbar = dvz.dvz_colorbar(panel, scale, ctypes.byref(desc))
    if not colorbar:
        raise RuntimeError("dvz_colorbar() failed")

    ex.run(scene, figure, "Colorbar")


if __name__ == "__main__":
    main()
