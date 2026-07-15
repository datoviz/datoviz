#!/usr/bin/env python3
"""Scene animation tracks driving a cube transform and camera path."""

from __future__ import annotations

import ctypes
import math

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


ANIMATION_LOOP_DURATION_S = 9.6


class AnimationTracksState:
    def __init__(self) -> None:
        self.rotation = None
        self.camera_eye = None
        self.camera_target = None
        self.visual_animation = None
        self.camera_animation = None

    def destroy_tracks(self) -> None:
        for track in (self.rotation, self.camera_eye, self.camera_target):
            if track:
                dvz.dvz_track_destroy(track)
        self.rotation = None
        self.camera_eye = None
        self.camera_target = None


def _animation_camera_desc():
    camera = dvz.dvz_camera_desc()
    camera.view.eye[:] = (-2.0, 2.0, 2.0)
    camera.view.up[:] = (0.0, 1.0, 0.0)
    camera.projection.fov_y = 0.66
    camera.projection.near_clip = 0.05
    camera.projection.far_clip = 100.0
    return camera


def _add_reference_grid(panel) -> None:
    desc = dvz.dvz_reference_grid_desc()
    desc.plane = dvz.DVZ_REFERENCE_GRID_XZ
    desc.origin[1] = -0.32
    desc.size[:] = (6.0, 6.0)
    desc.spacing = 0.25
    desc.major_every = 4
    desc.minor_color = dvz.DvzColor(74, 86, 98, 95)
    desc.major_color = dvz.DvzColor(116, 132, 148, 145)
    desc.axis_color = dvz.DvzColor(176, 190, 204, 185)
    desc.minor_width_px = 1.0
    desc.major_width_px = 1.5
    desc.axis_width_px = 2.0
    desc.depth_test = True
    grid = dvz.dvz_reference_grid(panel, ctypes.byref(desc))
    if not grid:
        raise RuntimeError("dvz_reference_grid() failed")


def _add_cube_mesh(scene, panel):
    face_colors = (dvz.DvzColor * 6)(ex.CYAN, ex.GREEN, ex.YELLOW, ex.TEXT, ex.BLUE, ex.RED)

    desc = dvz.dvz_geometry_cube_desc()
    desc.size = 0.56
    desc.face_colors = face_colors
    desc.face_color_count = len(face_colors)
    geometry = dvz.dvz_geometry_cube(ctypes.byref(desc))
    if not geometry:
        raise RuntimeError("dvz_geometry_cube() failed")

    mesh = dvz.dvz_mesh(scene, 0)
    if not mesh:
        dvz.dvz_geometry_destroy(geometry)
        raise RuntimeError("dvz_mesh() failed")

    try:
        if dvz.dvz_mesh_set_geometry(mesh, geometry) != 0:
            raise RuntimeError("dvz_mesh_set_geometry() failed")
    finally:
        dvz.dvz_geometry_destroy(geometry)

    material = dvz.dvz_standard_material_desc()
    material.standard.roughness = 0.52
    material.standard.specular = 0.38
    material.standard.rim_strength = 0.18
    if dvz.dvz_visual_set_material(mesh, ctypes.byref(material)) != 0:
        raise RuntimeError("dvz_visual_set_material() failed")
    ex.add_visual(panel, mesh)
    return mesh


def _add_visual_animation(scene, mesh, state: AnimationTracksState) -> None:
    rotation_desc = dvz.dvz_track_rotation_desc()
    rotation_desc.axis[:] = (0.35, 0.85, 0.25)
    rotation_desc.speed_rad_per_sec = -math.tau / ANIMATION_LOOP_DURATION_S
    rotation = dvz.dvz_track_rotation(ctypes.byref(rotation_desc))
    if not rotation:
        raise RuntimeError("dvz_track_rotation() failed")
    state.rotation = rotation

    transform_desc = dvz.dvz_transform_motion_desc()
    transform_desc.rotation = rotation
    animation = dvz.dvz_anim_visual_transform(scene, mesh, ctypes.byref(transform_desc))
    if not animation:
        raise RuntimeError("dvz_anim_visual_transform() failed")
    state.visual_animation = animation
    if dvz.dvz_anim_set_speed(animation, 1.0) != 0:
        raise RuntimeError("dvz_anim_set_speed(visual) failed")
    if dvz.dvz_anim_start(animation, 0.0) != 0:
        raise RuntimeError("dvz_anim_start(visual) failed")


def _add_camera_animation(scene, camera, state: AnimationTracksState) -> None:
    times = np.array([0.0, 1.2, 2.4, 3.6, 4.8], dtype=np.float64)
    eyes = np.array(
        [
            [-2.0, 2.0, +2.0],
            [+2.0, 2.0, +2.0],
            [+2.0, 2.0, -2.0],
            [-2.0, 2.0, -2.0],
            [-2.0, 2.0, +2.0],
        ],
        dtype=np.float32,
    )

    eye_desc = dvz.dvz_track_keyframes_desc()
    eye_desc.type = dvz.DVZ_TRACK_VEC3
    eye_desc.count = len(times)
    eye_desc.times = times.ctypes.data_as(ctypes.POINTER(ctypes.c_double))
    eye_desc.values = eyes.ctypes.data_as(ctypes.c_void_p)
    eye_desc.topology = dvz.DVZ_TRACK_TOPOLOGY_CLOSED
    eye_desc.repeat = dvz.DVZ_TRACK_REPEAT_LOOP
    eye_desc.interpolation = dvz.DVZ_TRACK_INTERP_CATMULL_ROM
    camera_eye = dvz.dvz_track_keyframes(ctypes.byref(eye_desc))
    if not camera_eye:
        raise RuntimeError("dvz_track_keyframes(camera eye) failed")
    state.camera_eye = camera_eye

    target_value = (ctypes.c_float * 3)(0.0, 0.0, 0.0)
    target_desc = dvz.dvz_track_constant_desc()
    target_desc.type = dvz.DVZ_TRACK_VEC3
    target_desc.value = ctypes.cast(target_value, ctypes.c_void_p)
    camera_target = dvz.dvz_track_constant(ctypes.byref(target_desc))
    if not camera_target:
        raise RuntimeError("dvz_track_constant(camera target) failed")
    state.camera_target = camera_target

    camera_motion = dvz.dvz_camera_motion_desc()
    camera_motion.eye = camera_eye
    camera_motion.target = camera_target
    camera_motion.up_mode = dvz.DVZ_CAMERA_UP_WORLD
    camera_motion.up[:] = (0.0, 1.0, 0.0)
    animation = dvz.dvz_anim_camera_motion(scene, camera, ctypes.byref(camera_motion))
    if not animation:
        raise RuntimeError("dvz_anim_camera_motion() failed")
    state.camera_animation = animation
    if dvz.dvz_anim_set_speed(animation, 0.5) != 0:
        raise RuntimeError("dvz_anim_set_speed(camera) failed")
    if dvz.dvz_anim_start(animation, 0.0) != 0:
        raise RuntimeError("dvz_anim_start(camera) failed")


def _build_scene():
    scene, figure, panel = ex.scene_panel()
    state = AnimationTracksState()

    camera_desc = _animation_camera_desc()
    if dvz.dvz_panel_set_camera_desc(panel, ctypes.byref(camera_desc)) != 0:
        raise RuntimeError("dvz_panel_set_camera_desc() failed")
    camera = dvz.dvz_panel_camera(panel)
    if not camera:
        raise RuntimeError("dvz_panel_camera() failed")

    _add_reference_grid(panel)
    mesh = _add_cube_mesh(scene, panel)
    _add_visual_animation(scene, mesh, state)
    _add_camera_animation(scene, camera, state)
    return scene, figure, panel, state


def _configure_view(view, scene, panel, state: AnimationTracksState) -> None:
    controller = dvz.dvz_turntable(scene, None)
    if not controller:
        raise RuntimeError("dvz_turntable() failed")
    if dvz.dvz_view_bind_controller(view, panel, controller, dvz.DVZ_DIM_MASK_XYZ) != 0:
        raise RuntimeError("dvz_view_bind_controller() failed")
    if (
        dvz.dvz_anim_set_interaction_policy(
            state.camera_animation,
            controller,
            dvz.DVZ_ANIM_INTERACTION_PAUSE,
            0.0,
        )
        != 0
    ):
        raise RuntimeError("dvz_anim_set_interaction_policy() failed")


def _run(scene, figure, panel, state: AnimationTracksState) -> None:
    app = dvz.dvz_app(scene)
    if not app:
        dvz.dvz_scene_destroy(scene)
        state.destroy_tracks()
        raise RuntimeError("dvz_app() failed")
    try:
        view = dvz.dvz_view_window(app, figure, ex.WIDTH, ex.HEIGHT, b"Animation Tracks")
        if not view:
            raise RuntimeError("dvz_view_window() failed")
        _configure_view(view, scene, panel, state)
        ex.run_app(app, view)
    finally:
        dvz.dvz_app_destroy(app)
        dvz.dvz_scene_destroy(scene)
        state.destroy_tracks()


def main() -> None:
    scene, figure, panel, state = _build_scene()
    _run(scene, figure, panel, state)


if __name__ == "__main__":
    main()
