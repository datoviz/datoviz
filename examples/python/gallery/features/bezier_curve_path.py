#!/usr/bin/env python3
"""Cubic Bezier control points rendered as a retained stroked path."""

from __future__ import annotations

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


SEGMENT_COUNT = 48
CONTROLS = np.array(
    [
        [-0.82, -0.45, 0.0],
        [-0.42, +0.76, 0.0],
        [+0.38, -0.74, 0.0],
        [+0.82, +0.45, 0.0],
    ],
    dtype=np.float32,
)


def _bezier_points(controls: np.ndarray, segment_count: int = SEGMENT_COUNT) -> np.ndarray:
    t = np.linspace(0.0, 1.0, segment_count + 1, dtype=np.float32)[:, None]
    one = 1.0 - t
    return (
        one**3 * controls[0]
        + 3.0 * one**2 * t * controls[1]
        + 3.0 * one * t**2 * controls[2]
        + t**3 * controls[3]
    ).astype(np.float32)


def _add_curve(scene, panel, points: np.ndarray) -> None:
    count = len(points)
    colors = ex.color_array(*([ex.CYAN] * count))
    widths = np.full(count, 8.0, dtype=np.float32)

    path = dvz.dvz_path(scene, 0)
    if not path:
        raise RuntimeError("dvz_path() failed")
    if dvz.dvz_visual_set_data_many(
        path,
        {
            "position": points,
            "color": colors,
            "stroke_width_px": widths,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(path) failed")
    if dvz.dvz_path_set_caps(path, dvz.DVZ_SEGMENT_CAP_ROUND, dvz.DVZ_SEGMENT_CAP_ROUND) != 0:
        raise RuntimeError("dvz_path_set_caps() failed")
    if dvz.dvz_path_set_join(path, dvz.DVZ_PATH_JOIN_ROUND, 4.0) != 0:
        raise RuntimeError("dvz_path_set_join() failed")
    ex.add_visual(panel, path)


def _add_control_polygon(scene, panel, controls: np.ndarray) -> None:
    starts = controls[:-1].copy()
    ends = controls[1:].copy()
    colors = ex.color_array(
        dvz.DvzColor(ex.BLUE.r, ex.BLUE.g, ex.BLUE.b, 180),
        dvz.DvzColor(ex.BLUE.r, ex.BLUE.g, ex.BLUE.b, 180),
        dvz.DvzColor(ex.BLUE.r, ex.BLUE.g, ex.BLUE.b, 180),
    )
    widths = np.full(len(starts), 2.0, dtype=np.float32)

    segment = dvz.dvz_segment(scene, 0)
    if not segment:
        raise RuntimeError("dvz_segment() failed")
    if dvz.dvz_visual_set_data_many(
        segment,
        {
            "position_start": starts,
            "position_end": ends,
            "color": colors,
            "stroke_width_px": widths,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(segment) failed")
    if dvz.dvz_segment_set_caps(segment, dvz.DVZ_SEGMENT_CAP_ROUND, dvz.DVZ_SEGMENT_CAP_ROUND) != 0:
        raise RuntimeError("dvz_segment_set_caps() failed")
    ex.add_visual(panel, segment)


def _add_control_points(scene, panel, controls: np.ndarray) -> None:
    colors = ex.color_array(ex.YELLOW, ex.GREEN, ex.GREEN, ex.YELLOW)
    diameters = np.array([34.0, 24.0, 24.0, 34.0], dtype=np.float32)

    point = dvz.dvz_point(scene, 0)
    if not point:
        raise RuntimeError("dvz_point() failed")
    if dvz.dvz_visual_set_data_many(
        point,
        {
            "position": controls,
            "color": colors,
            "diameter_px": diameters,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(point) failed")
    ex.add_visual(panel, point)


def _build_scene():
    scene, figure, panel = ex.scene_panel()
    points = _bezier_points(CONTROLS)
    _add_control_polygon(scene, panel, CONTROLS)
    _add_curve(scene, panel, points)
    _add_control_points(scene, panel, CONTROLS)
    return scene, figure, panel


def main() -> None:
    scene, figure, _panel = _build_scene()
    ex.run(scene, figure, "Bezier Curve Path")


if __name__ == "__main__":
    main()
