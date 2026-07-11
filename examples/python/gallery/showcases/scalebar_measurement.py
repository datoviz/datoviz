#!/usr/bin/env python3
"""Scale-bar measurement workflow across image and 3D specimen panels."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


OVERVIEW_WIDTH = 224
OVERVIEW_HEIGHT = 160
DETAIL_WIDTH = 180
DETAIL_HEIGHT = 136
DETAIL_POINTS = 120
CLOUD_COUNT = 125
ROTATION_SPEED_RAD_PER_SEC = 0.35
TAU = 2.0 * np.pi


class ScalebarMeasurementState:
    def __init__(self) -> None:
        self.specimen_rotation = None
        self.specimen_animation = None

    def destroy_tracks(self) -> None:
        if self.specimen_rotation:
            dvz.dvz_track_destroy(self.specimen_rotation)
        self.specimen_rotation = None


def _u8(value):
    return np.clip(255.0 * value + 0.5, 0.0, 255.0).astype(np.uint8)


def _sample_field(x, y, detail: bool):
    value = 0.10 + 0.10 * np.sin(TAU * (2.2 * x + 0.7 * y))
    value += 0.06 * np.cos(TAU * (0.4 * x - 3.8 * y))
    for i in range(9):
        cx = np.mod(0.17 + 0.137 * i, 0.96)
        cy = np.mod(0.21 + 0.219 * i, 0.94)
        dx = x - cx
        dy = y - cy
        sigma = 0.030 + 0.004 * (i % 3) if detail else 0.044
        d2 = (dx * dx + 1.35 * dy * dy) / (2.0 * sigma * sigma)
        value += (0.33 + 0.05 * (i % 4)) * np.exp(-d2)
    return np.clip(value, 0.0, 1.0)


def _field_texture(width: int, height: int, *, detail: bool):
    x = np.linspace(0.0, 1.0, width, dtype=np.float32)
    y = np.linspace(0.0, 1.0, height, dtype=np.float32)
    u, v = np.meshgrid(x, y)
    sample = _sample_field(u, v, detail)
    ridge = 0.5 + 0.5 * np.sin(TAU * (7.0 * u + 2.4 * v))

    rgba = np.empty((height, width, 4), dtype=np.uint8)
    rgba[..., 0] = _u8(0.07 + 0.16 * sample + 0.05 * ridge)
    rgba[..., 1] = _u8(0.12 + 0.55 * sample + 0.06 * ridge)
    rgba[..., 2] = _u8(0.16 + 0.74 * sample)
    rgba[..., 3] = 255
    return rgba


def _set_domain(panel, xmin: float, xmax: float, ymin: float, ymax: float) -> None:
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_X, xmin, xmax) != 0:
        raise RuntimeError("dvz_panel_set_domain(X) failed")
    if dvz.dvz_panel_set_domain(panel, dvz.DVZ_DIM_Y, ymin, ymax) != 0:
        raise RuntimeError("dvz_panel_set_domain(Y) failed")


def _configure_panel(panel) -> None:
    dvz.dvz_panel_set_background_color(panel, ex.BG)


def _add_image(scene, panel, rgba, xmin: float, xmax: float, ymin: float, ymax: float) -> None:
    positions = np.array(
        [[xmin, ymin, 0.0], [xmin, ymax, 0.0], [xmax, ymin, 0.0], [xmax, ymax, 0.0]],
        dtype=np.float32,
    )
    texcoords = np.array(
        [[0.0, 0.0], [0.0, 1.0], [1.0, 0.0], [1.0, 1.0]],
        dtype=np.float32,
    )
    image = dvz.dvz_image(scene, 0)
    if not image:
        raise RuntimeError("dvz_image() failed")
    if dvz.dvz_visual_set_data_many(image, {"position": positions, "texcoords": texcoords}) != 0:
        raise RuntimeError("dvz_visual_set_data_many(image) failed")
    field = dvz.dvz_sampled_field_from_array(scene, rgba)
    if dvz.dvz_visual_set_field(image, b"field", field) != 0:
        raise RuntimeError("dvz_visual_set_field(image) failed")
    if dvz.dvz_visual_set_depth_test(image, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(image) failed")
    ex.add_visual(panel, image)


def _add_zoom_box(scene, panel) -> None:
    starts = np.array(
        [[3.10, 2.25, 0.0], [5.10, 2.25, 0.0], [5.10, 3.65, 0.0], [3.10, 3.65, 0.0]],
        dtype=np.float32,
    )
    ends = np.array(
        [[5.10, 2.25, 0.0], [5.10, 3.65, 0.0], [3.10, 3.65, 0.0], [3.10, 2.25, 0.0]],
        dtype=np.float32,
    )
    colors = np.tile(np.array([[ex.GREEN.r, ex.GREEN.g, ex.GREEN.b, 230]], dtype=np.uint8), (4, 1))
    widths = np.full(4, 2.2, dtype=np.float32)

    box = dvz.dvz_segment(scene, 0)
    if not box:
        raise RuntimeError("dvz_segment() failed")
    if dvz.dvz_visual_set_data_many(
        box,
        {
            "position_start": starts,
            "position_end": ends,
            "color": colors,
            "stroke_width_px": widths,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(zoom box) failed")
    if dvz.dvz_segment_set_caps(box, dvz.DVZ_SEGMENT_CAP_SQUARE, dvz.DVZ_SEGMENT_CAP_SQUARE) != 0:
        raise RuntimeError("dvz_segment_set_caps(zoom box) failed")
    if dvz.dvz_visual_set_depth_test(box, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(zoom box) failed")
    ex.add_visual(panel, box)


def _add_detail_points(scene, panel) -> None:
    t = np.linspace(0.0, 1.0, DETAIL_POINTS, dtype=np.float32)
    a = TAU * (9.0 * t + 0.07 * np.sin(21.0 * t))
    r = 0.10 + 0.78 * np.sqrt(t)

    positions = np.zeros((DETAIL_POINTS, 3), dtype=np.float32)
    positions[:, 0] = 4.10 + 0.54 * r * np.cos(a)
    positions[:, 1] = 2.95 + 0.34 * r * np.sin(a)

    pulse = np.sin(TAU * (3.0 * t + 0.11))
    colors = np.empty((DETAIL_POINTS, 4), dtype=np.uint8)
    colors[:, 0] = _u8(0.20 + 0.20 * t)
    colors[:, 1] = _u8(0.66 + 0.22 * t)
    colors[:, 2] = _u8(0.76 + 0.12 * pulse * pulse)
    colors[:, 3] = 210
    diameters = (4.4 + 5.4 * pulse * pulse).astype(np.float32)

    points = dvz.dvz_point(scene, 0)
    if not points:
        raise RuntimeError("dvz_point() failed")
    if dvz.dvz_visual_set_data_many(
        points,
        {
            "position": positions,
            "color": colors,
            "diameter_px": diameters,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(detail points) failed")
    if dvz.dvz_visual_set_depth_test(points, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(detail points) failed")
    ex.set_filled_point_style(points)
    ex.add_visual(panel, points)


def _add_panel_scalebar(panel, anchor, color, label_position) -> None:
    desc = dvz.dvz_scale_bar_desc()
    desc.dimension = dvz.DVZ_DIM_X
    desc.anchor = anchor
    desc.label_position = label_position
    desc.target_length_px = 132.0
    desc.min_length_px = 82.0
    desc.max_length_px = 190.0
    desc.offset_px[:] = (26.0, 24.0)
    desc.tick_length_px = 8.0
    desc.line_width_px = 2.0
    desc.unit = b"m"
    desc.data_to_unit = 0.001
    desc.line_color[:] = (color.r, color.g, color.b, 255)

    scalebar = dvz.dvz_scale_bar(panel, ctypes.byref(desc))
    if not scalebar:
        raise RuntimeError("dvz_scale_bar(panel) failed")

    style = dvz.dvz_text_style()
    style.size_px = 16.0
    style.renderer = dvz.DVZ_TEXT_RENDERER_MSDF_ATLAS
    style.color[:] = (color.r, color.g, color.b, 255)
    if dvz.dvz_scale_bar_set_label_style(scalebar, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_scale_bar_set_label_style(panel) failed")


def _add_world_scalebar(panel) -> None:
    desc = dvz.dvz_scale_bar_desc()
    desc.dimension = dvz.DVZ_DIM_X
    desc.reference_mode = dvz.DVZ_SCALEBAR_REFERENCE_VIEW_PLANE
    desc.reference_position[:] = (0.0, 0.0, 0.0)
    desc.anchor = dvz.DVZ_SCENE_ANCHOR_BOTTOM_RIGHT
    desc.label_position = dvz.DVZ_SCALEBAR_LABEL_ABOVE
    desc.target_length_px = 122.0
    desc.min_length_px = 78.0
    desc.max_length_px = 176.0
    desc.offset_px[:] = (24.0, 24.0)
    desc.tick_length_px = 8.0
    desc.line_width_px = 2.0
    desc.unit = b"m"
    desc.data_to_unit = 0.001
    desc.line_color[:] = (ex.TEXT.r, ex.TEXT.g, ex.TEXT.b, 255)

    scalebar = dvz.dvz_scale_bar(panel, ctypes.byref(desc))
    if not scalebar:
        raise RuntimeError("dvz_scale_bar(world) failed")

    style = dvz.dvz_text_style()
    style.size_px = 16.0
    style.renderer = dvz.DVZ_TEXT_RENDERER_MSDF_ATLAS
    style.color[:] = (ex.TEXT.r, ex.TEXT.g, ex.TEXT.b, 255)
    if dvz.dvz_scale_bar_set_label_style(scalebar, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_scale_bar_set_label_style(world) failed")


def _add_3d_cloud(scene, panel):
    camera = dvz.dvz_camera_desc()
    camera.view.eye[:] = (0.18, -0.10, 3.35)
    camera.view.target[:] = (0.0, 0.0, 0.0)
    camera.view.up[:] = (0.0, 1.0, 0.0)
    camera.projection.fov_y = 0.70
    camera.projection.near_clip = 0.1
    camera.projection.far_clip = 100.0
    if dvz.dvz_panel_set_camera_desc(panel, ctypes.byref(camera)) != 0:
        raise RuntimeError("dvz_panel_set_camera_desc() failed")

    positions = []
    colors = []
    diameters = []
    for z in range(5):
        for y in range(5):
            for x in range(5):
                fx = -1.0 + 0.5 * x
                fy = -1.0 + 0.5 * y
                fz = -1.0 + 0.5 * z
                d = np.sqrt(fx * fx + fy * fy + fz * fz)
                if d > 1.24 or len(positions) >= CLOUD_COUNT:
                    continue
                positions.append((fx, fy, fz))
                colors.append(
                    (
                        int(np.clip(255.0 * (0.22 + 0.15 * x / 4.0) + 0.5, 0, 255)),
                        int(np.clip(255.0 * (0.58 + 0.30 * y / 4.0) + 0.5, 0, 255)),
                        int(np.clip(255.0 * (0.84 + 0.12 * (1.0 - z / 4.0)) + 0.5, 0, 255)),
                        230,
                    )
                )
                diameters.append(11.0 + 8.0 * (1.24 - d))

    points = dvz.dvz_point(scene, 0)
    if not points:
        raise RuntimeError("dvz_point(3D cloud) failed")
    if dvz.dvz_visual_set_data_many(
        points,
        {
            "position": np.array(positions, dtype=np.float32),
            "color": np.array(colors, dtype=np.uint8),
            "diameter_px": np.array(diameters, dtype=np.float32),
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(3D cloud) failed")
    ex.set_filled_point_style(points)
    ex.add_visual(panel, points)
    return points


def _add_specimen_animation(scene, specimen_cloud, state: ScalebarMeasurementState) -> None:
    rotation_desc = dvz.dvz_track_rotation_desc()
    rotation_desc.axis[:] = (0.0, 1.0, 0.0)
    rotation_desc.speed_rad_per_sec = 1.0
    rotation = dvz.dvz_track_rotation(ctypes.byref(rotation_desc))
    if not rotation:
        raise RuntimeError("dvz_track_rotation() failed")
    state.specimen_rotation = rotation

    transform_desc = dvz.dvz_transform_motion_desc()
    transform_desc.rotation = rotation
    animation = dvz.dvz_anim_visual_transform(scene, specimen_cloud, ctypes.byref(transform_desc))
    if not animation:
        raise RuntimeError("dvz_anim_visual_transform() failed")
    state.specimen_animation = animation
    if dvz.dvz_anim_set_speed(animation, ROTATION_SPEED_RAD_PER_SEC) != 0:
        raise RuntimeError("dvz_anim_set_speed(specimen) failed")
    if dvz.dvz_anim_start(animation, 0.0) != 0:
        raise RuntimeError("dvz_anim_start(specimen) failed")


def _build_scene():
    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError("dvz_scene() failed")
    figure = dvz.dvz_figure(scene, ex.WIDTH, ex.HEIGHT, 0)
    if not figure:
        raise RuntimeError("dvz_figure() failed")
    grid = dvz.dvz_figure_grid(figure, 2, 2)
    if not grid:
        raise RuntimeError("dvz_figure_grid() failed")

    margins = dvz.DvzPanelReserve(36.0, 30.0, 24.0, 30.0)
    if dvz.dvz_grid_set_margins(grid, ctypes.byref(margins)) != 0:
        raise RuntimeError("dvz_grid_set_margins() failed")
    if dvz.dvz_grid_set_gutter(grid, 28.0, 26.0) != 0:
        raise RuntimeError("dvz_grid_set_gutter() failed")

    overview = dvz.dvz_grid_panel_span(grid, 0, 0, 2, 1)
    detail = dvz.dvz_grid_panel(grid, 0, 1)
    specimen = dvz.dvz_grid_panel(grid, 1, 1)
    if not overview or not detail or not specimen:
        raise RuntimeError("dvz_grid_panel() failed")

    for panel in (overview, detail, specimen):
        _configure_panel(panel)

    _set_domain(overview, 0.0, 12.0, 0.0, 8.0)
    _set_domain(detail, 3.10, 5.10, 2.25, 3.65)

    overview_pixels = _field_texture(OVERVIEW_WIDTH, OVERVIEW_HEIGHT, detail=False)
    detail_pixels = _field_texture(DETAIL_WIDTH, DETAIL_HEIGHT, detail=True)
    _add_image(scene, overview, overview_pixels, 0.0, 12.0, 0.0, 8.0)
    _add_image(scene, detail, detail_pixels, 3.10, 5.10, 2.25, 3.65)
    _add_zoom_box(scene, overview)
    _add_detail_points(scene, detail)
    specimen_cloud = _add_3d_cloud(scene, specimen)

    _add_panel_scalebar(
        overview,
        dvz.DVZ_SCENE_ANCHOR_BOTTOM_LEFT,
        ex.CYAN,
        dvz.DVZ_SCALEBAR_LABEL_ABOVE,
    )
    _add_panel_scalebar(
        detail,
        dvz.DVZ_SCENE_ANCHOR_BOTTOM_RIGHT,
        ex.GREEN,
        dvz.DVZ_SCALEBAR_LABEL_ABOVE,
    )
    _add_world_scalebar(specimen)

    state = ScalebarMeasurementState()
    _add_specimen_animation(scene, specimen_cloud, state)
    return scene, figure, (overview, detail, specimen), state


def _configure_view(view, scene, panels, state: ScalebarMeasurementState) -> None:
    overview, detail, specimen = panels
    ex.bind_panzoom(view, scene, overview, dvz.DVZ_DIM_MASK_XY)
    ex.bind_panzoom(view, scene, detail, dvz.DVZ_DIM_MASK_XY)

    controller = dvz.dvz_arcball(scene, None)
    if not controller:
        raise RuntimeError("dvz_arcball() failed")
    if dvz.dvz_view_bind_controller(view, specimen, controller, dvz.DVZ_DIM_MASK_XYZ) != 0:
        raise RuntimeError("dvz_view_bind_controller(arcball) failed")
    arcball = dvz.dvz_controller_arcball(controller)
    if not arcball:
        raise RuntimeError("dvz_controller_arcball() failed")
    if dvz.dvz_arcball_set(arcball, (ctypes.c_float * 3)(0.56, -0.18, 0.30)) != 0:
        raise RuntimeError("dvz_arcball_set() failed")
    if state.specimen_animation and dvz.dvz_anim_set_interaction_policy(
        state.specimen_animation,
        controller,
        dvz.DVZ_ANIM_INTERACTION_PAUSE,
        0.0,
    ) != 0:
        raise RuntimeError("dvz_anim_set_interaction_policy() failed")


def _run(scene, figure, panels, state: ScalebarMeasurementState) -> None:
    app = dvz.dvz_app(scene)
    if not app:
        dvz.dvz_scene_destroy(scene)
        state.destroy_tracks()
        raise RuntimeError("dvz_app() failed")
    try:
        view = dvz.dvz_view_window(app, figure, ex.WIDTH, ex.HEIGHT, b"Scale Bar Measurement")
        if not view:
            raise RuntimeError("dvz_view_window() failed")
        _configure_view(view, scene, panels, state)
        ex.run_app(app, view)
    finally:
        dvz.dvz_app_destroy(app)
        dvz.dvz_scene_destroy(scene)
        state.destroy_tracks()


def main() -> None:
    scene, figure, panels, state = _build_scene()
    _run(scene, figure, panels, state)


if __name__ == "__main__":
    main()
