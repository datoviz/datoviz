#!/usr/bin/env python3
"""Checked public-API scatter plot used by the Quickstart and homepage."""

import ctypes
import os

import numpy as np
import datoviz as dvz


WIDTH, HEIGHT = 1280, 720
N = 10_000
TITLE = "Datoviz Quickstart"


def _check(result, call):
    if result != 0:
        raise RuntimeError(f"{call} failed with code {result}")


def main():
    rng = np.random.default_rng(12345)
    positions = np.zeros((N, 3), dtype=np.float32)
    positions[:, :2] = rng.uniform(-1, 1, (N, 2)).astype(np.float32)
    colors = rng.integers(0, 256, (N, 4), dtype=np.uint8)
    colors[:, 3] = 200
    diameters = rng.uniform(4, 12, N).astype(np.float32)

    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError("dvz_scene() failed")

    session = None
    try:
        figure = dvz.dvz_figure(scene, WIDTH, HEIGHT, 0)
        panel = dvz.dvz_panel_full(figure) if figure else None
        if not figure or not panel:
            raise RuntimeError("figure or panel creation failed")

        _check(
            dvz.dvz_panel_set_background_color(panel, dvz.DvzColor(13, 18, 25, 255)),
            "dvz_panel_set_background_color()",
        )
        controller = dvz.dvz_panzoom(scene, None)
        if not controller:
            raise RuntimeError("dvz_panzoom() failed")
        _check(
            dvz.dvz_panel_bind_controller(panel, controller, dvz.DVZ_DIM_MASK_XY),
            "dvz_panel_bind_controller()",
        )

        points = dvz.dvz_point(scene, 0)
        if not points:
            raise RuntimeError("dvz_point() failed")
        _check(
            dvz.dvz_visual_set_data_many(
                points,
                {"position": positions, "color": colors, "diameter_px": diameters},
            ),
            "dvz_visual_set_data_many()",
        )

        style = dvz.dvz_point_style_desc()
        style.aspect = dvz.DVZ_SHAPE_ASPECT_FILLED
        style.stroke_width_px = 0.0
        _check(dvz.dvz_point_set_style(points, ctypes.byref(style)), "dvz_point_set_style()")
        _check(dvz.dvz_visual_set_depth_test(points, False), "dvz_visual_set_depth_test()")
        _check(
            dvz.dvz_visual_set_alpha_mode(points, dvz.DVZ_ALPHA_BLENDED),
            "dvz_visual_set_alpha_mode()",
        )
        _check(dvz.dvz_panel_add_visual(panel, points, None), "dvz_panel_add_visual()")

        test_mode = os.environ.get("DVZ_QUICKSTART_TEST") == "1"
        session = dvz.run(scene, figure, title=TITLE, blocking=False if test_mode else None)
        if test_mode:
            if session is None:
                raise RuntimeError("nonblocking dvz.run() returned no session")
            status = session.render_once()
            if status != dvz.DVZ_CANVAS_FRAME_READY:
                raise RuntimeError(f"first frame failed with status {status}")
    finally:
        if session is not None:
            session.close()
        dvz.dvz_scene_destroy(scene)


if __name__ == "__main__":
    main()
