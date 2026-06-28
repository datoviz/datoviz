#!/usr/bin/env python3
"""Smoke-test the top-level array-aware facade against libdatoviz."""

from __future__ import annotations

import sys
import ctypes
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
    assert dvz.dvz_axis_set_ticks is not raw.dvz_axis_set_ticks

    required_symbols = [
        'DvzAxisTicks',
        'DvzColorbarDesc',
        'DvzColorbarTicks',
        'DvzGeometry',
        'DvzQueryResult',
        'DvzTextItem',
        'DvzTextLayout',
        'DvzTextPlacement',
        'DvzTextStyle',
        'dvz_panel_set_domain',
        'dvz_panel_visible_domain',
        'dvz_panel_axis',
        'dvz_axis_set_grid',
        'dvz_axis_set_label',
        'dvz_axis_set_tick_policy',
        'dvz_axis_set_ticks',
        'dvz_axis_clear_ticks',
        'dvz_colorbar',
        'dvz_colorbar_desc',
        'dvz_colorbar_ticks',
        'dvz_colorbar_set_title',
        'dvz_colorbar_set_format',
        'dvz_colorbar_set_orientation',
        'dvz_colorbar_set_anchor',
        'dvz_colorbar_set_layout',
        'dvz_colorbar_set_ticks',
        'dvz_colorbar_clear_ticks',
        'dvz_text',
        'dvz_text_style',
        'dvz_text_placement',
        'dvz_text_layout',
        'dvz_text_set_items',
        'dvz_text_set_string',
        'dvz_text_set_position',
        'dvz_text_set_layout',
        'dvz_text_set_positions',
        'dvz_text_set_sizes',
        'dvz_text_set_colors',
        'dvz_text_set_angles',
        'dvz_text_set_style',
        'dvz_text_set_placement',
        'dvz_mesh',
        'dvz_mesh_set_geometry',
        'dvz_visual_set_index_data',
        'dvz_panel_query',
        'dvz_scene_poll_query',
    ]
    missing = [name for name in required_symbols if not hasattr(dvz, name)]
    if missing:
        raise RuntimeError('array facade is missing symbols: ' + ', '.join(missing))

    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError('dvz_scene() failed')

    try:
        figure = dvz.dvz_figure(scene, 320, 240, 0)
        if not figure:
            raise RuntimeError('dvz_figure() failed')
        panel = dvz.dvz_panel_full(figure)
        if not panel:
            raise RuntimeError('dvz_panel_full() failed')
        axis = dvz.dvz_panel_axis(panel, dvz.DvzDim.DVZ_DIM_X)
        if not axis:
            raise RuntimeError('dvz_panel_axis() failed')
        if not dvz.dvz_axis_set_ticks(
            axis, np.array([0.0, 1.0], dtype=np.float64), ['zero', 'one']
        ):
            raise RuntimeError('facade dvz_axis_set_ticks() failed')
        if not dvz.dvz_axis_clear_ticks(axis):
            raise RuntimeError('dvz_axis_clear_ticks() failed')

        scale_desc = dvz.dvz_scale_desc()
        scale_desc.kind = dvz.DvzScaleKind.DVZ_SCALE_CONTINUOUS
        scale = dvz.dvz_scale(scene, ctypes.byref(scale_desc))
        if not scale:
            raise RuntimeError('dvz_scale() failed')
        dvz.dvz_scale_set_domain(scale, 0.0, 1.0)
        cmap = dvz.dvz_colormap_builtin(scene, dvz.DvzBuiltinColormap.DVZ_BUILTIN_COLORMAP_VIRIDIS)
        if not cmap:
            raise RuntimeError('dvz_colormap_builtin() failed')
        dvz.dvz_scale_set_colormap(scale, cmap)
        colorbar = dvz.dvz_colorbar(panel, scale, None)
        if not colorbar:
            raise RuntimeError('dvz_colorbar() failed')
        if not dvz.dvz_colorbar_set_ticks(
            colorbar, np.array([0.0, 0.5, 1.0], dtype=np.float64), ['low', 'mid', 'high']
        ):
            raise RuntimeError('facade dvz_colorbar_set_ticks() failed')
        if not dvz.dvz_colorbar_clear_ticks(colorbar):
            raise RuntimeError('dvz_colorbar_clear_ticks() failed')

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
        if dvz.dvz_visual_set_data(visual, 'diameter_px', diameters) != 0:
            raise RuntimeError('facade dvz_visual_set_data(diameter_px) failed')
        if dvz.dvz_visual_set_data_range(visual, 'position', updated, 1) != 0:
            raise RuntimeError('facade dvz_visual_set_data_range(position) failed')
    finally:
        dvz.dvz_scene_destroy(scene)

    print('array facade smoke: OK')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
