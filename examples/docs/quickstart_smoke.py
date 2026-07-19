#!/usr/bin/env python3
"""Automated one-frame smoke test for the public Python Quickstart."""

import ctypes
from pathlib import Path

import numpy as np
import datoviz as dvz


WIDTH, HEIGHT = 1280, 720
N = 10_000
TITLE = "Datoviz Quickstart"


def _check_documented_source():
    root = Path(__file__).resolve().parents[2]
    source = (root / "examples/docs/quickstart.py").read_text(encoding="utf8")
    page = (root / "docs/start/quickstart.md").read_text(encoding="utf8")
    if "dvz.App" in source or "dvz.App" in page:
        raise RuntimeError("v0.4 Quickstart must not use the removed dvz.App API")
    if '--8<-- "examples/docs/quickstart.py"' not in page:
        raise RuntimeError("Python Quickstart must include the checked canonical fixture")


def _check(result, call):
    if result != 0:
        raise RuntimeError(f"{call} failed with code {result}")


def main():
    _check_documented_source()
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

        session = dvz.run(scene, figure, title=TITLE, blocking=False)
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
