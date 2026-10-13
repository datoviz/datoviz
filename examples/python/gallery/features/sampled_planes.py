#!/usr/bin/env python3
"""Physical sampled planes compare alpha cutout with forward/reverse WBOIT."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


SIZE = 64
LABELS = (b"Cutout: alpha >= 0.5", b"WBOIT: A then B", b"WBOIT: B then A")
ANGLES = (ctypes.c_float * 3)(0.38, -0.46, 0.12)
# Match the graphite-cyan palette and label style used by the canonical C scenario.
PANEL_BG = dvz.DvzColor(22, 27, 34, 255)
CYAN = dvz.DvzColor(76, 201, 240, 255)
AMBER = dvz.DvzColor(255, 183, 3, 255)
MESH_COLOR = dvz.DvzColor(201, 209, 217, 255)


def _texture() -> np.ndarray:
    y, x = np.mgrid[:SIZE, :SIZE]
    hole = (x % 16 - 8) ** 2 + (y % 16 - 8) ** 2 < 16
    rgba = np.empty((SIZE, SIZE, 4), dtype=np.uint8)
    rgba[..., 0] = 160 + x
    rgba[..., 1] = 184 + y // 2
    rgba[..., 2] = 224
    rgba[..., 3] = np.where(hole, 0, np.where((x // 8 + y // 8) % 2, 224, 96))
    return rgba


def _plane(scene, panel, field, perpendicular: bool, mode) -> None:
    positions = np.array(
        [[-0.82, -0.72, 0.05], [-0.82, 0.72, 0.05],
         [0.82, -0.72, 0.05], [0.82, 0.72, 0.05]], dtype=np.float32,
    )
    normals = np.zeros((4, 3), dtype=np.float32)
    normals[:, 0 if perpendicular else 2] = 1
    if perpendicular:
        positions[:, 2] = positions[:, 0]
        positions[:, 0] = -0.08
    tint = AMBER if perpendicular else CYAN
    colors = np.tile(ex.color_array(tint), (4, 1))
    uv = np.array([[0, 0], [0, 1], [1, 0], [1, 1]], dtype=np.float32)
    indices = np.array([0, 1, 2, 2, 1, 3], dtype=np.uint32)
    buffer_desc = dvz.dvz_scene_buffer_desc()
    buffer_desc.usage = dvz.DVZ_SCENE_BUFFER_USAGE_INDEX
    buffer_desc.stride = indices.itemsize
    buffer = dvz.dvz_scene_buffer(scene, ctypes.byref(buffer_desc))
    plane = dvz.dvz_mesh(scene, 0)
    if not buffer or not plane:
        raise RuntimeError("plane resource creation failed")
    if dvz.dvz_scene_buffer_set_data(buffer, indices) != 0:
        raise RuntimeError("plane index upload failed")
    for attr, data in (("position", positions), ("normal", normals), ("color", colors),
                       ("texcoords", uv)):
        if dvz.dvz_visual_set_data(plane, attr, data) != 0:
            raise RuntimeError(f"plane {attr} upload failed")
    if dvz.dvz_visual_set_buffer(plane, b"index", buffer) != 0:
        raise RuntimeError("plane index binding failed")
    if dvz.dvz_visual_set_field(plane, b"texture", field) != 0:
        raise RuntimeError("plane texture binding failed")
    sampling = dvz.dvz_field_sampling_desc()
    sampling.min_filter = dvz.DVZ_FIELD_FILTER_NEAREST
    sampling.mag_filter = dvz.DVZ_FIELD_FILTER_NEAREST
    material = dvz.dvz_material_desc()
    material.model = dvz.DVZ_MATERIAL_MODEL_UNLIT
    material.alpha_mode = mode
    material.opacity = 0.62 if mode == dvz.DVZ_ALPHA_WBOIT else 1.0
    if dvz.dvz_visual_set_field_sampling(plane, b"texture", ctypes.byref(sampling)) != 0:
        raise RuntimeError("plane sampling setup failed")
    if dvz.dvz_visual_set_material(plane, ctypes.byref(material)) != 0:
        raise RuntimeError("plane material setup failed")
    if dvz.dvz_visual_set_alpha_cutoff(plane, 0.5) != 0:
        raise RuntimeError("plane cutoff setup failed")
    ex.add_visual(panel, plane)


def _cube_and_label(scene, panel, text: bytes) -> None:
    desc = dvz.dvz_geometry_cube_desc()
    desc.size = 0.62
    desc.color = MESH_COLOR
    geometry = dvz.dvz_geometry_cube(ctypes.byref(desc))
    mesh = dvz.dvz_mesh(scene, 0)
    if not geometry or not mesh:
        raise RuntimeError("cube creation failed")
    try:
        if dvz.dvz_mesh_set_geometry(mesh, geometry) != 0:
            raise RuntimeError("cube upload failed")
    finally:
        dvz.dvz_geometry_destroy(geometry)
    ex.add_visual(panel, mesh)
    label = dvz.dvz_label_desc()
    label.text = text
    style = dvz.dvz_text_style()
    style.size_px = 23
    style.renderer = dvz.DVZ_TEXT_RENDERER_MSDF_ATLAS
    style.color[:] = (222, 236, 244, 245)
    placement = dvz.dvz_text_placement()
    placement.mode = dvz.DVZ_TEXT_PLACEMENT_SCREEN
    placement.anchor = dvz.DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT
    placement.position[:] = (16, 20, 0)
    placement.has_text_anchor = True
    annotation = dvz.dvz_annotation_label(panel, ctypes.byref(label))
    if not annotation:
        raise RuntimeError("panel annotation creation failed")
    if dvz.dvz_annotation_set_style(annotation, ctypes.byref(style)) != 0:
        raise RuntimeError("panel annotation style failed")
    if dvz.dvz_annotation_set_placement(annotation, ctypes.byref(placement)) != 0:
        raise RuntimeError("panel annotation placement failed")


def _build_scene():
    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError("scene creation failed")
    figure = dvz.dvz_figure(scene, ex.WIDTH, ex.HEIGHT, 0)
    if not figure:
        raise RuntimeError("figure creation failed")
    grid = dvz.dvz_figure_grid(figure, 1, 3)
    if not grid:
        raise RuntimeError("grid creation failed")
    margins = dvz.DvzPanelReserve(28, 28, 36, 36)
    if dvz.dvz_grid_set_margins(grid, ctypes.byref(margins)) != 0:
        raise RuntimeError("grid margins failed")
    if dvz.dvz_grid_set_gutter(grid, 20, 0) != 0:
        raise RuntimeError("grid gutter failed")
    field = dvz.dvz_sampled_field_from_array(scene, _texture())
    panels = []
    for p, text in enumerate(LABELS):
        panel = dvz.dvz_grid_panel(grid, 0, p)
        if not panel:
            raise RuntimeError("panel creation failed")
        dvz.dvz_panel_set_background_color(panel, PANEL_BG)
        camera = dvz.dvz_camera_desc()
        camera.view.eye[:] = (-0.25 * 1.15, 2 * 1.15, 4 * 1.15)
        camera.view.target[:] = (0, 0, 0)
        camera.view.up[:] = (0, 1, 0)
        camera.projection.fov_y = 0.66
        camera.projection.near_clip = 0.05
        camera.projection.far_clip = 115
        if dvz.dvz_panel_set_camera_desc(panel, ctypes.byref(camera)) != 0:
            raise RuntimeError("camera setup failed")
        mode = dvz.DVZ_ALPHA_MASK if p == 0 else dvz.DVZ_ALPHA_WBOIT
        _plane(scene, panel, field, p == 2, mode)
        _plane(scene, panel, field, p != 2, mode)
        _cube_and_label(scene, panel, text)
        panels.append(panel)
    return scene, figure, panels


def main() -> None:
    scene, figure, panels = _build_scene()

    def configure(view) -> None:
        controllers = []
        for panel in panels:
            controller = dvz.dvz_arcball(scene, None)
            if not controller:
                raise RuntimeError("arcball creation failed")
            if dvz.dvz_view_bind_controller(view, panel, controller, dvz.DVZ_DIM_MASK_XYZ) != 0:
                raise RuntimeError("arcball binding failed")
            arcball = dvz.dvz_controller_arcball(controller)
            if dvz.dvz_arcball_set(arcball, ANGLES) != 0:
                raise RuntimeError("arcball setup failed")
            controllers.append(controller)
        components = (int(dvz.DVZ_CONTROLLER_LINK_ROTATION) | int(dvz.DVZ_CONTROLLER_LINK_PAN)
                      | int(dvz.DVZ_CONTROLLER_LINK_ZOOM))
        for controller in controllers[1:]:
            if not dvz.dvz_controller_link(
                scene, controllers[0], controller, components, dvz.DVZ_CONTROLLER_LINK_TWO_WAY
            ):
                raise RuntimeError("camera link failed")

    ex.run_with_view(scene, figure, "Sampled Planes", configure)


if __name__ == "__main__":
    main()
