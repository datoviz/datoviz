#!/usr/bin/env python3
"""Shared mesh/point/sampled-plane WBOIT with opaque sites and reversed draw order."""

from __future__ import annotations

import ctypes
import math

import numpy as np

import datoviz as dvz
from examples.python.gallery import common as ex

POINT_COUNT = 768
PATH_COUNT = 48


def _point_data(time: float):
    index = np.arange(POINT_COUNT, dtype=np.float32)
    u = index / (POINT_COUNT - 1)
    phase = 2 * math.pi * time / 3
    angle = 2.39996323 * index
    y = 1 - 2 * u
    radius = np.sqrt(np.maximum(0, 1 - y * y))
    shell = 0.50 + 0.36 * (index % 11) / 10
    positions = np.column_stack(
        [
            shell * radius * np.cos(angle),
            0.72 * shell * y,
            shell * radius * np.sin(angle) + 0.06 * np.sin(phase + 8 * u),
        ]
    ).astype(np.float32)
    colors = np.column_stack(
        [
            250 - 154 * u,
            204 - 39 * u,
            21 + 229 * u,
            np.where(index % 17 == 0, 0, 64 + 96 * (0.5 + 0.5 * np.sin(phase + 2 * math.pi * u))),
        ]
    ).astype(np.uint8)
    sizes = (3 + 5 * (0.5 + 0.5 * np.cos(phase + 2 * math.pi * u))).astype(np.float32)
    return {'position': positions, 'color': colors, 'diameter_px': sizes}


def _surface(scene, center_x: float, color):
    desc = dvz.dvz_geometry_sphere_desc()
    desc.radius = 0.62
    desc.sectors = 48
    desc.rings = 24
    desc.color = dvz.DvzColor(*color)
    geometry = dvz.dvz_geometry_sphere(ctypes.byref(desc))
    mesh = dvz.dvz_mesh(scene, 0)
    if not geometry or not mesh:
        raise RuntimeError('surface creation failed')
    try:
        if dvz.dvz_mesh_set_geometry(mesh, geometry) != 0:
            raise RuntimeError('surface upload failed')
    finally:
        dvz.dvz_geometry_destroy(geometry)
    material = dvz.dvz_phong_material_desc()
    material.alpha_mode = dvz.DVZ_ALPHA_WBOIT
    material.phong.ambient = 0.5
    material.phong.diffuse = 0.8
    material.phong.specular = 0.3
    material.phong.shininess = 24
    if dvz.dvz_visual_set_material(mesh, ctypes.byref(material)) != 0:
        raise RuntimeError('surface material failed')
    transform = ((ctypes.c_float * 4) * 4)()
    for index in range(4):
        transform[index][index] = 1
    transform[3][0] = center_x
    if dvz.dvz_visual_set_transform(mesh, transform) != 0:
        raise RuntimeError('surface translation failed')
    return mesh


def _sampled_plane(scene):
    size = 32
    y, x = np.mgrid[:size, :size]
    hole = (x % 8 - 4) ** 2 + (y % 8 - 4) ** 2 < 5
    pixels = np.empty((size, size, 4), dtype=np.uint8)
    pixels[..., :3] = (244, 114, 182)
    pixels[..., 3] = np.where(hole, 0, np.where((x // 8 + y // 8) % 2, 144, 208))
    field = dvz.dvz_sampled_field_from_array(scene, pixels)
    if not field:
        raise RuntimeError('sampled-plane field upload failed')
    positions = np.array(
        [
            [-0.68, -0.60, -0.0704],
            [-0.68, 0.60, -0.0704],
            [0.68, -0.60, 0.3104],
            [0.68, 0.60, 0.3104],
        ],
        dtype=np.float32,
    )
    normals = np.tile(np.array([-0.26963, 0, 0.96296], dtype=np.float32), (4, 1))
    uv = np.array([[0, 0], [0, 1], [1, 0], [1, 1]], dtype=np.float32)
    colors = np.full((4, 4), 255, dtype=np.uint8)
    indices = np.array([0, 2, 1, 2, 3, 1], dtype=np.uint32)
    index_desc = dvz.dvz_scene_buffer_desc()
    index_desc.usage = dvz.DVZ_SCENE_BUFFER_USAGE_INDEX
    index_desc.stride = indices.itemsize
    index = dvz.dvz_scene_buffer(scene, ctypes.byref(index_desc))
    plane = dvz.dvz_mesh(scene, 0)
    if not index or not plane or dvz.dvz_scene_buffer_set_data(index, indices) != 0:
        raise RuntimeError('sampled-plane geometry creation failed')
    for attr, data in (
        ('position', positions),
        ('normal', normals),
        ('color', colors),
        ('texcoords', uv),
    ):
        if dvz.dvz_visual_set_data(plane, attr, data) != 0:
            raise RuntimeError(f'sampled-plane {attr} upload failed')
    if (
        dvz.dvz_visual_set_buffer(plane, b'index', index) != 0
        or dvz.dvz_visual_set_field(plane, b'texture', field) != 0
    ):
        raise RuntimeError('sampled-plane resource binding failed')
    sampling = dvz.dvz_field_sampling_desc()
    sampling.min_filter = sampling.mag_filter = dvz.DVZ_FIELD_FILTER_NEAREST
    material = dvz.dvz_material_desc()
    material.model = dvz.DVZ_MATERIAL_MODEL_UNLIT
    material.alpha_mode = dvz.DVZ_ALPHA_WBOIT
    material.opacity = 0.50
    if (
        dvz.dvz_visual_set_field_sampling(plane, b'texture', ctypes.byref(sampling)) != 0
        or dvz.dvz_visual_set_material(plane, ctypes.byref(material)) != 0
    ):
        raise RuntimeError('sampled-plane material setup failed')
    return plane


def _opaque_context(scene, panel) -> None:
    spheres = dvz.dvz_sphere(scene, 0)
    dvz.dvz_visual_set_data_many(
        spheres,
        {
            'position': np.array([[-0.52, -0.14, 0.45], [0.52, 0.14, -0.45]], dtype=np.float32),
            'color': np.array([[250, 204, 21, 255], [248, 113, 113, 255]], dtype=np.uint8),
            'radius': np.array([0.12, 0.12], dtype=np.float32),
        },
    )
    ex.add_visual(panel, spheres)
    u = np.linspace(0, 1, PATH_COUNT, dtype=np.float32)
    for trajectory in range(2):
        positions = np.column_stack(
            [
                (-0.22 if trajectory == 0 else 0.22) + 0.10 * np.sin(2 * math.pi * u),
                -0.86 + 1.72 * u,
                0.72 * np.cos(2 * math.pi * u + trajectory * 1.8),
            ]
        ).astype(np.float32)
        color = [217, 226, 236, 255] if trajectory == 0 else [74, 222, 128, 255]
        path = dvz.dvz_path(scene, 0)
        dvz.dvz_visual_set_data_many(
            path,
            {
                'position': positions,
                'color': np.tile(np.array(color, dtype=np.uint8), (PATH_COUNT, 1)),
                'stroke_width_px': np.full(
                    PATH_COUNT, 4 if trajectory == 0 else 6, dtype=np.float32
                ),
            },
        )
        dvz.dvz_path_set_caps(path, dvz.DVZ_SEGMENT_CAP_ROUND, dvz.DVZ_SEGMENT_CAP_ROUND)
        dvz.dvz_path_set_join(path, dvz.DVZ_PATH_JOIN_ROUND, 4)
        ex.add_visual(panel, path)


def _label(panel, text: bytes) -> None:
    label = dvz.dvz_label_desc()
    label.text = text
    annotation = dvz.dvz_annotation_label(panel, ctypes.byref(label))
    style = dvz.dvz_text_style()
    style.size_px = 23
    style.renderer = dvz.DVZ_TEXT_RENDERER_MSDF_ATLAS
    style.color[:] = (222, 236, 244, 245)
    placement = dvz.dvz_text_placement()
    placement.mode = dvz.DVZ_TEXT_PLACEMENT_SCREEN
    placement.anchor = dvz.DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT
    placement.position[:] = (18, 18, 0)
    placement.text_anchor[:] = (0, 0)
    placement.has_text_anchor = True
    if (
        not annotation
        or dvz.dvz_annotation_set_style(annotation, ctypes.byref(style)) != 0
        or dvz.dvz_annotation_set_placement(annotation, ctypes.byref(placement)) != 0
    ):
        raise RuntimeError('panel label failed')


def main() -> None:
    scene = dvz.dvz_scene()
    figure = dvz.dvz_figure(scene, ex.WIDTH, ex.HEIGHT, 0)
    grid = dvz.dvz_figure_grid(figure, 1, 2)
    margins = dvz.DvzPanelReserve(32, 32, 32, 32)
    dvz.dvz_grid_set_margins(grid, ctypes.byref(margins))
    dvz.dvz_grid_set_gutter(grid, 24, 0)
    panels = []
    points = []
    for index, label in enumerate((b'Forward order', b'Reverse order')):
        panel = dvz.dvz_grid_panel(grid, 0, index)
        dvz.dvz_panel_set_background_color(panel, dvz.DvzColor(22, 27, 34, 255))
        camera = dvz.dvz_camera_desc()
        camera.view.eye[:] = (-0.25 * 0.82, 2 * 0.82, 4 * 0.82)
        camera.projection.fov_y = 0.66
        camera.projection.near_clip = 0.05
        camera.projection.far_clip = 82
        dvz.dvz_panel_set_camera_desc(panel, ctypes.byref(camera))
        _label(panel, label)
        surfaces = [
            _surface(scene, -0.23, (34, 211, 238, 68)),
            _surface(scene, 0.23, (96, 165, 250, 92)),
        ]
        plane = _sampled_plane(scene)
        point = dvz.dvz_point(scene, 0)
        dvz.dvz_visual_set_data_many(point, _point_data(0))
        dvz.dvz_visual_set_alpha_mode(point, dvz.DVZ_ALPHA_WBOIT)
        ex.add_visual(panel, surfaces[index])
        if index == 1:
            ex.add_visual(panel, plane)
        ex.add_visual(panel, point)
        if index == 0:
            ex.add_visual(panel, plane)
        ex.add_visual(panel, surfaces[1 - index])
        _opaque_context(scene, panel)
        panels.append(panel)
        points.append(point)

    def configure(view) -> None:
        controllers = []
        for panel in panels:
            controller = dvz.dvz_arcball(scene, None)
            if (
                not controller
                or dvz.dvz_view_bind_controller(view, panel, controller, dvz.DVZ_DIM_MASK_XYZ) != 0
            ):
                raise RuntimeError('arcball binding failed')
            controllers.append(controller)
        components = (
            dvz.DVZ_CONTROLLER_LINK_ROTATION
            | dvz.DVZ_CONTROLLER_LINK_PAN
            | dvz.DVZ_CONTROLLER_LINK_ZOOM
        )
        if not dvz.dvz_controller_link(
            scene, controllers[0], controllers[1], components, dvz.DVZ_CONTROLLER_LINK_TWO_WAY
        ):
            raise RuntimeError('camera linking failed')

    def on_frame(_view, frame_index: int, elapsed: float) -> None:
        data = _point_data(frame_index / 60 if ex.SMOKE_MODE else elapsed)
        for point in points:
            dvz.dvz_visual_set_data_many(point, data)

    ex.run_with_frame_callback(scene, figure, 'Scientific 3D Transparency', on_frame, configure)


if __name__ == '__main__':
    main()
