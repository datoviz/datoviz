#!/usr/bin/env python3
"""Generated gyroid scalar field rendered as a 3D volume."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


FIELD_SIZE = 128
BOX_SEGMENTS = 12
ROTATION_SPEED_RAD_PER_SEC = 0.16
TAU = 2.0 * np.pi
VOLUME_BOUNDS_MIN = (-0.88, -0.74, -1.04)
VOLUME_BOUNDS_MAX = (+0.88, +0.74, +1.04)


class VolumeState:
    def __init__(self) -> None:
        self.volume_rotation = None
        self.box_rotation = None
        self.volume_animation = None
        self.box_animation = None

    def destroy_tracks(self) -> None:
        for track in (self.box_rotation, self.volume_rotation):
            if track:
                dvz.dvz_track_destroy(track)
        self.box_rotation = None
        self.volume_rotation = None


def _smoothstep(edge0: float, edge1: float, value):
    if edge0 == edge1:
        return np.where(value < edge0, 0.0, 1.0)
    t = np.clip((value - edge0) / (edge1 - edge0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def _gyroid_value(x, y, z):
    scale = 1.16 * TAU
    gx = scale * x
    gy = scale * y
    gz = scale * z
    return np.sin(gx) * np.cos(gy) + np.sin(gy) * np.cos(gz) + np.sin(gz) * np.cos(gx)


def _sample_volume(x, y, z):
    g0 = _gyroid_value(x, y, z)
    g1 = _gyroid_value(x + 0.035, y - 0.020, z + 0.045)
    sheet = np.exp(-(g0 * g0) / (2.0 * 0.115 * 0.115))
    shoulder = np.exp(-((np.abs(g1) - 0.34) ** 2) / (2.0 * 0.135 * 0.135))

    rx = x / 0.92
    ry = y / 0.86
    rz = z / 0.96
    r = np.sqrt(rx * rx + ry * ry + rz * rz)
    envelope = 1.0 - _smoothstep(0.82, 1.00, r)
    rim = _smoothstep(0.72, 0.94, r) * (1.0 - _smoothstep(0.96, 1.03, r))

    directional_light = 0.78 + 0.22 * np.clip(0.5 + 0.5 * (0.45 * x - y + z), 0.0, 1.0)
    lattice = envelope * directional_light * (0.88 * sheet + 0.16 * shoulder)
    return np.clip(0.86 * lattice + 0.12 * rim * sheet, 0.0, 1.0)


def _volume_data():
    coords = np.linspace(-1.0, 1.0, FIELD_SIZE, dtype=np.float32)
    z, y, x = np.meshgrid(coords, coords, coords, indexing="ij")
    values = _sample_volume(x, y, z)
    return np.clip(255.0 * values + 0.5, 0.0, 255.0).astype(np.uint8)


def _set_default_3d_camera(panel) -> None:
    camera = dvz.dvz_camera_desc()
    camera.view.eye[:] = (2.35, -2.95, 2.15)
    camera.view.target[:] = (0.0, 0.0, 0.0)
    camera.view.up[:] = (0.0, 0.0, 1.0)
    camera.projection.fov_y = 0.66
    camera.projection.near_clip = 0.05
    camera.projection.far_clip = 100.0
    if dvz.dvz_panel_set_camera_desc(panel, ctypes.byref(camera)) != 0:
        raise RuntimeError("dvz_panel_set_camera_desc() failed")


def _attach_transfer(scene, visual) -> None:
    desc = dvz.dvz_scale_desc()
    desc.kind = dvz.DVZ_SCALE_CONTINUOUS
    scale = dvz.dvz_scale(scene, ctypes.byref(desc))
    if not scale:
        raise RuntimeError("dvz_scale() failed")
    if dvz.dvz_scale_set_domain(scale, 0.0, 1.0) != 0:
        raise RuntimeError("dvz_scale_set_domain() failed")

    colormap = dvz.dvz_colormap(scene, None)
    if not colormap:
        raise RuntimeError("dvz_colormap() failed")
    stop_values = (
        (0.00, (14, 17, 23, 255)),
        (0.10, (14, 17, 23, 255)),
        (0.24, (18, 58, 96, 255)),
        (0.40, (44, 166, 209, 255)),
        (0.56, (76, 201, 240, 255)),
        (0.74, (128, 255, 219, 255)),
        (0.92, (238, 252, 232, 255)),
        (1.00, (255, 183, 3, 255)),
    )
    stops = (dvz.DvzColormapStop * len(stop_values))()
    for stop, (position, rgba) in zip(stops, stop_values, strict=True):
        stop.position = position
        stop.rgba[:] = rgba
    if dvz.dvz_colormap_set_stops(colormap, stops, len(stops)) != 0:
        raise RuntimeError("dvz_colormap_set_stops() failed")
    if dvz.dvz_scale_set_colormap(scale, colormap) != 0:
        raise RuntimeError("dvz_scale_set_colormap() failed")

    alpha_values = (
        (0.00, 0.00),
        (0.09, 0.00),
        (0.18, 0.08),
        (0.32, 0.32),
        (0.52, 0.62),
        (0.74, 0.86),
        (1.00, 0.98),
    )
    alpha = (dvz.DvzVolumeAlphaStop * len(alpha_values))()
    for stop, (position, opacity) in zip(alpha, alpha_values, strict=True):
        stop.position = position
        stop.alpha = opacity
    if dvz.dvz_volume_set_alpha_stops(visual, alpha, len(alpha)) != 0:
        raise RuntimeError("dvz_volume_set_alpha_stops() failed")
    if dvz.dvz_visual_set_scale(visual, b"color", scale) != 0:
        raise RuntimeError("dvz_visual_set_scale(volume) failed")


def _configure_volume(visual) -> None:
    if dvz.dvz_visual_set_alpha_mode(visual, dvz.DVZ_ALPHA_BLENDED) != 0:
        raise RuntimeError("dvz_visual_set_alpha_mode(volume) failed")
    bmin = (ctypes.c_double * 3)(*VOLUME_BOUNDS_MIN)
    bmax = (ctypes.c_double * 3)(*VOLUME_BOUNDS_MAX)
    if dvz.dvz_volume_set_bounds(visual, bmin, bmax) != 0:
        raise RuntimeError("dvz_volume_set_bounds() failed")
    if dvz.dvz_volume_set_value_range(visual, 0.0, 0.88) != 0:
        raise RuntimeError("dvz_volume_set_value_range() failed")
    if dvz.dvz_volume_set_opacity(visual, 1.0) != 0:
        raise RuntimeError("dvz_volume_set_opacity() failed")
    if dvz.dvz_volume_set_step_count(visual, 128) != 0:
        raise RuntimeError("dvz_volume_set_step_count() failed")
    if dvz.dvz_volume_set_render_mode(visual, dvz.DVZ_VOLUME_RENDER_MIP) != 0:
        raise RuntimeError("dvz_volume_set_render_mode() failed")


def _add_volume(scene, panel):
    field = dvz.dvz_sampled_field_from_array(
        scene,
        _volume_data(),
        format=dvz.DVZ_FIELD_FORMAT_R8_UNORM,
        semantic=dvz.DVZ_FIELD_SEMANTIC_SCALAR,
        dim=dvz.DVZ_FIELD_DIM_3D,
    )
    volume = dvz.dvz_volume(scene, 0)
    if not volume:
        raise RuntimeError("dvz_volume() failed")
    if dvz.dvz_visual_set_field(volume, b"field", field) != 0:
        raise RuntimeError("dvz_visual_set_field(volume) failed")
    _configure_volume(volume)
    _attach_transfer(scene, volume)
    ex.add_visual(panel, volume)
    return volume


def _add_boundary_box(scene, panel):
    xmin, ymin, zmin = VOLUME_BOUNDS_MIN
    xmax, ymax, zmax = VOLUME_BOUNDS_MAX
    corners = np.array(
        [
            [xmin, ymin, zmin],
            [xmax, ymin, zmin],
            [xmax, ymax, zmin],
            [xmin, ymax, zmin],
            [xmin, ymin, zmax],
            [xmax, ymin, zmax],
            [xmax, ymax, zmax],
            [xmin, ymax, zmax],
        ],
        dtype=np.float32,
    )
    edges = np.array(
        [
            [0, 1],
            [1, 2],
            [2, 3],
            [3, 0],
            [4, 5],
            [5, 6],
            [6, 7],
            [7, 4],
            [0, 4],
            [1, 5],
            [2, 6],
            [3, 7],
        ],
        dtype=np.uint32,
    )
    starts = corners[edges[:, 0]]
    ends = corners[edges[:, 1]]
    colors = np.tile(np.array([[ex.CYAN.r, ex.CYAN.g, ex.CYAN.b, 46]], dtype=np.uint8), (12, 1))
    widths = np.full(BOX_SEGMENTS, 1.0, dtype=np.float32)

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
        raise RuntimeError("dvz_visual_set_data_many(box) failed")
    if dvz.dvz_segment_set_caps(box, dvz.DVZ_SEGMENT_CAP_ROUND, dvz.DVZ_SEGMENT_CAP_ROUND) != 0:
        raise RuntimeError("dvz_segment_set_caps(box) failed")
    if dvz.dvz_visual_set_alpha_mode(box, dvz.DVZ_ALPHA_BLENDED) != 0:
        raise RuntimeError("dvz_visual_set_alpha_mode(box) failed")
    ex.add_visual(panel, box)
    return box


def _add_rotation_animation(scene, visual, state: VolumeState, attr: str, controller=None) -> None:
    rotation_desc = dvz.dvz_track_rotation_desc()
    rotation_desc.axis[:] = (0.0, 1.0, 0.0)
    rotation_desc.speed_rad_per_sec = 1.0
    rotation = dvz.dvz_track_rotation(ctypes.byref(rotation_desc))
    if not rotation:
        raise RuntimeError("dvz_track_rotation() failed")
    setattr(state, attr, rotation)

    transform_desc = dvz.dvz_transform_motion_desc()
    transform_desc.rotation = rotation
    animation = dvz.dvz_anim_visual_transform(scene, visual, ctypes.byref(transform_desc))
    if not animation:
        raise RuntimeError("dvz_anim_visual_transform() failed")
    if controller is not None and dvz.dvz_anim_set_interaction_policy(
        animation,
        controller,
        dvz.DVZ_ANIM_INTERACTION_PAUSE,
        0.0,
    ) != 0:
        raise RuntimeError("dvz_anim_set_interaction_policy() failed")
    if dvz.dvz_anim_set_speed(animation, ROTATION_SPEED_RAD_PER_SEC) != 0:
        raise RuntimeError("dvz_anim_set_speed() failed")
    if dvz.dvz_anim_start(animation, 0.0) != 0:
        raise RuntimeError("dvz_anim_start() failed")
    if attr == "volume_rotation":
        state.volume_animation = animation
    else:
        state.box_animation = animation


def _build_scene():
    scene, figure, panel = ex.scene_panel()
    _set_default_3d_camera(panel)
    volume = _add_volume(scene, panel)
    box = _add_boundary_box(scene, panel)
    state = VolumeState()
    return scene, figure, panel, volume, box, state


def _configure_view(view, scene, panel, volume, box, state: VolumeState) -> None:
    controller = dvz.dvz_arcball(scene, None)
    if not controller:
        raise RuntimeError("dvz_arcball() failed")
    if dvz.dvz_view_bind_controller(view, panel, controller, dvz.DVZ_DIM_MASK_XYZ) != 0:
        raise RuntimeError("dvz_view_bind_controller(arcball) failed")
    _add_rotation_animation(scene, volume, state, "volume_rotation", controller)
    _add_rotation_animation(scene, box, state, "box_rotation", controller)


def _run(scene, figure, panel, volume, box, state: VolumeState) -> None:
    app = dvz.dvz_app(scene)
    if not app:
        dvz.dvz_scene_destroy(scene)
        state.destroy_tracks()
        raise RuntimeError("dvz_app() failed")
    try:
        view = dvz.dvz_view_window(app, figure, ex.WIDTH, ex.HEIGHT, b"Volume")
        if not view:
            raise RuntimeError("dvz_view_window() failed")
        _configure_view(view, scene, panel, volume, box, state)
        dvz.dvz_app_run(app, 0)
    finally:
        dvz.dvz_app_destroy(app)
        dvz.dvz_scene_destroy(scene)
        state.destroy_tracks()


def main() -> None:
    scene, figure, panel, volume, box, state = _build_scene()
    _run(scene, figure, panel, volume, box, state)


if __name__ == "__main__":
    main()
