#!/usr/bin/env python3
"""Requested view size policies resolved into concrete view sizes."""

from __future__ import annotations

import argparse
import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


POINT_COUNT = 5
DEFAULT_VIEW_KIND = dvz.DVZ_VIEW_WINDOW


def _policy_name(policy: int) -> str:
    labels = {
        dvz.DVZ_VIEW_SIZE_FRAMEBUFFER_PX: "pixel_exact",
        dvz.DVZ_VIEW_SIZE_HOST_LOGICAL_PX: "host_logical_px",
        dvz.DVZ_VIEW_SIZE_REFERENCE_PX: "reference_px",
        dvz.DVZ_VIEW_SIZE_PHYSICAL_MM: "physical_mm",
    }
    return labels.get(policy, "unknown")


def _size_desc(policy: str):
    if policy in {"pixel", "pixel_exact"}:
        return dvz.dvz_view_size_desc_framebuffer_px(ex.WIDTH, ex.HEIGHT)
    if policy in {"host", "host_logical", "host_logical_px"}:
        return dvz.dvz_view_size_desc_host_logical_px(ex.WIDTH, ex.HEIGHT)
    if policy in {"physical", "physical_mm"}:
        return dvz.dvz_view_size_desc_physical_mm(338.7, 190.5, 96.0)
    return dvz.dvz_view_size_desc_reference_px(float(ex.WIDTH), float(ex.HEIGHT), 96.0)


def _add_points(scene, panel) -> None:
    positions = np.array(
        [
            [-0.70, -0.35, 0.0],
            [-0.35, +0.26, 0.0],
            [+0.00, +0.00, 0.0],
            [+0.35, -0.24, 0.0],
            [+0.70, +0.36, 0.0],
        ],
        dtype=np.float32,
    )
    colors = ex.color_array(ex.WHITE, ex.CYAN, ex.YELLOW, ex.RED, ex.BLUE)
    diameters = np.array([26.0, 34.0, 46.0, 58.0, 70.0], dtype=np.float32)

    point = dvz.dvz_point(scene, 0)
    if not point:
        raise RuntimeError("dvz_point() failed")
    if dvz.dvz_visual_set_data_many(
        point,
        {
            "position": positions,
            "color": colors,
            "diameter_px": diameters,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(point) failed")
    ex.set_filled_point_style(point)
    ex.add_visual(panel, point)


def _print_resolved(resolved) -> None:
    print(f"policy: {_policy_name(resolved.requested_policy)}")
    print(f"canvas_px: {resolved.canvas_width_px:.1f}x{resolved.canvas_height_px:.1f}")
    print(f"host_logical_px: {resolved.host_logical_width}x{resolved.host_logical_height}")
    print(f"framebuffer_px: {resolved.framebuffer_width}x{resolved.framebuffer_height}")
    print(
        "framebuffer_per_canvas_px: "
        f"{resolved.framebuffer_per_canvas_px_x:.3f}x{resolved.framebuffer_per_canvas_px_y:.3f}"
    )
    if resolved.target_width_mm > 0.0 and resolved.target_height_mm > 0.0:
        print(f"target_physical_mm: {resolved.target_width_mm:.1f}x{resolved.target_height_mm:.1f}")


def _build_scene(size, kind=DEFAULT_VIEW_KIND):
    initial = dvz.dvz_view_size_resolve(ctypes.byref(size), kind)
    width = int(initial.host_logical_width or ex.WIDTH)
    height = int(initial.host_logical_height or ex.HEIGHT)

    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError("dvz_scene() failed")
    figure = dvz.dvz_figure(scene, width, height, 0)
    if not figure:
        dvz.dvz_scene_destroy(scene)
        raise RuntimeError("dvz_figure() failed")
    panel = dvz.dvz_panel_full(figure)
    if not panel:
        dvz.dvz_scene_destroy(scene)
        raise RuntimeError("dvz_panel_full() failed")
    dvz.dvz_panel_set_background_color(panel, ex.BG)
    _add_points(scene, panel)
    return scene, figure, panel


def _view_desc(kind, size):
    desc = dvz.dvz_view_desc(kind)
    desc.size_policy = size.policy
    desc.size_width = size.width
    desc.size_height = size.height
    desc.size_reference_dpi = size.reference_dpi
    desc.size_requested_device_scale = size.requested_device_scale
    desc.size_monitor_dpi_x_override = size.monitor_dpi_x_override
    desc.size_monitor_dpi_y_override = size.monitor_dpi_y_override
    desc.size_strict_framebuffer_size = size.strict_framebuffer_size
    desc.title = b"View Size Policies"
    return desc


def _run_view(policy: str = "reference", *, kind=DEFAULT_VIEW_KIND) -> tuple:
    size = _size_desc(policy)
    scene, figure, panel = _build_scene(size, kind)
    app = dvz.dvz_app(scene)
    if not app:
        dvz.dvz_scene_destroy(scene)
        raise RuntimeError("dvz_app() failed")
    try:
        desc = _view_desc(kind, size)
        view = dvz.dvz_view(app, figure, ctypes.byref(desc))
        if not view:
            raise RuntimeError("dvz_view() failed")
        if not dvz.dvz_view_panzoom(view, panel, None):
            raise RuntimeError("dvz_view_panzoom() failed")
        return scene, app, view, dvz.dvz_view_resolved_size(view)
    except Exception:
        dvz.dvz_app_destroy(app)
        dvz.dvz_scene_destroy(scene)
        raise


def _run_smoke(policy: str = "pixel"):
    scene, app, view, resolved = _run_view(policy, kind=dvz.DVZ_VIEW_OFFSCREEN)
    try:
        status = dvz.dvz_view_render_once(view)
        if status != dvz.DVZ_CANVAS_FRAME_READY:
            raise RuntimeError("dvz_view_render_once() failed")
        ex.capture_smoke(view)
        if resolved.framebuffer_width <= 0 or resolved.framebuffer_height <= 0:
            raise RuntimeError("resolved framebuffer size is empty")
        return resolved
    finally:
        dvz.dvz_app_destroy(app)
        dvz.dvz_scene_destroy(scene)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--policy",
        choices=(
            "pixel",
            "pixel_exact",
            "host",
            "host_logical",
            "host_logical_px",
            "reference",
            "physical",
            "physical_mm",
        ),
        default="reference",
    )
    parser.add_argument("--frames", type=int, default=0)
    parser.add_argument("--smoke", action="store_true")
    args = parser.parse_args()

    if args.smoke or ex.SMOKE_MODE:
        resolved = _run_smoke(args.policy)
        _print_resolved(resolved)
        return

    scene, app, view, resolved = _run_view(args.policy, kind=dvz.DVZ_VIEW_WINDOW)
    try:
        _print_resolved(resolved)
        if args.frames > 0 and not ex.SMOKE_MODE:
            dvz.dvz_app_run(app, args.frames)
        else:
            ex.run_app(app, view)
    finally:
        dvz.dvz_app_destroy(app)
        dvz.dvz_scene_destroy(scene)


if __name__ == "__main__":
    main()
