#!/usr/bin/env python3
"""Screen-space ambient occlusion on a porous close-packed sphere aggregate."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


LABELS = (b"Plain lighting", b"Ambient occlusion")
INITIAL_ANGLES = (ctypes.c_float * 3)(0.180, -0.120, 0.000)
INITIAL_PAN = (ctypes.c_float * 2)(0.0, -0.020)
INITIAL_ZOOM = 0.82
AGGREGATE_SIDE = 11


def _sphere_data():
    positions = []
    center = AGGREGATE_SIDE // 2
    for z in range(AGGREGATE_SIDE):
        for y in range(AGGREGATE_SIDE):
            for x in range(AGGREGATE_SIDE):
                ix, iy, iz = x - center, y - center, z - center
                stagger = 0.0668 if (x + y + z) & 1 else 0.0
                px, py, pz = 0.1336 * ix + stagger, 0.1196 * iy, 0.1133 * iz
                distance2 = px * px + py * py + pz * pz
                outside = distance2 > 0.47
                cavity = pz > -0.08 and px * px + py * py < 0.045
                pore = (7 * x + 11 * y + 13 * z) % 23 == 0
                if outside or cavity or pore:
                    continue
                positions.append((px, py, pz))

    count = len(positions)
    radii = np.full(count, 0.0687, dtype=np.float32)
    colors = np.tile(np.array([[ex.CYAN.r, ex.CYAN.g, ex.CYAN.b, ex.CYAN.a]]), (count, 1))
    return np.asarray(positions, dtype=np.float32), radii, colors.astype(np.uint8)


def _add_label(panel, label: bytes) -> None:
    desc = dvz.dvz_label_desc()
    desc.text = label

    style = dvz.dvz_text_style()
    style.size_px = 18.0
    style.renderer = dvz.DVZ_TEXT_RENDERER_MSDF_ATLAS
    style.color[:] = (ex.TEXT.r, ex.TEXT.g, ex.TEXT.b, ex.TEXT.a)

    placement = dvz.dvz_text_placement()
    placement.mode = dvz.DVZ_TEXT_PLACEMENT_SCREEN
    placement.anchor = dvz.DVZ_SCENE_ANCHOR_PANEL_TOP_LEFT
    placement.position[:] = (20.0, 20.0, 0.0)
    placement.text_anchor[:] = (0.0, 0.0)
    placement.has_text_anchor = True

    annotation = dvz.dvz_annotation_label(panel, ctypes.byref(desc))
    if not annotation:
        raise RuntimeError("dvz_annotation_label() failed")
    if dvz.dvz_annotation_set_style(annotation, ctypes.byref(style)) != 0:
        raise RuntimeError("dvz_annotation_set_style() failed")
    if dvz.dvz_annotation_set_placement(annotation, ctypes.byref(placement)) != 0:
        raise RuntimeError("dvz_annotation_set_placement() failed")


def _add_sphere_cluster(scene, panel) -> None:
    spheres = dvz.dvz_sphere(scene, dvz.DVZ_SPHERE_FLAGS_LIGHTING)
    if not spheres:
        raise RuntimeError("dvz_sphere() failed")
    if dvz.dvz_sphere_set_mode(spheres, dvz.DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) != 0:
        raise RuntimeError("dvz_sphere_set_mode() failed")

    positions, radii, colors = _sphere_data()
    if dvz.dvz_visual_set_data_many(
        spheres,
        {
            "position": positions,
            "radius": radii,
            "color": colors,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(spheres) failed")
    material = dvz.dvz_standard_material_desc()
    material.light_direction[:] = (-0.38, 0.52, 0.76)
    material.standard.roughness = 0.72
    material.standard.specular = 0.22
    material.standard.rim_strength = 0.10
    if dvz.dvz_visual_set_material(spheres, ctypes.byref(material)) != 0:
        raise RuntimeError("dvz_visual_set_material() failed")
    ex.add_visual(panel, spheres)


def _set_ssao(panel) -> None:
    desc = dvz.dvz_ssao_desc()
    desc.radius = 0.28
    desc.strength = 4.0
    desc.bias = 0.0
    desc.power = 1.25
    desc.min_visibility = 0.30
    desc.sample_count = 32
    desc.blur_radius = 3.0
    desc.blur_depth_sigma = 0.65
    desc.blur_normal_sigma = 0.35
    desc.blur_enabled = True
    desc.debug_view = False
    if dvz.dvz_panel_set_ssao(panel, ctypes.byref(desc)) != 0:
        raise RuntimeError("dvz_panel_set_ssao() failed")


def _build_scene():
    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError("dvz_scene() failed")
    figure = dvz.dvz_figure(scene, ex.WIDTH, ex.HEIGHT, 0)
    if not figure:
        raise RuntimeError("dvz_figure() failed")

    grid = dvz.dvz_figure_grid(figure, 1, 2)
    if not grid:
        raise RuntimeError("dvz_figure_grid() failed")
    margins = dvz.DvzPanelReserve(42.0, 42.0, 38.0, 38.0)
    if dvz.dvz_grid_set_margins(grid, ctypes.byref(margins)) != 0:
        raise RuntimeError("dvz_grid_set_margins() failed")
    if dvz.dvz_grid_set_gutter(grid, 30.0, 0.0) != 0:
        raise RuntimeError("dvz_grid_set_gutter() failed")

    panels = []
    for col, label in enumerate(LABELS):
        panel = dvz.dvz_grid_panel(grid, 0, col)
        if not panel:
            raise RuntimeError("dvz_grid_panel() failed")
        dvz.dvz_panel_set_background_color(panel, ex.BG)
        ex.manual_camera(panel)
        _add_label(panel, label)
        _add_sphere_cluster(scene, panel)
        panels.append(panel)

    _set_ssao(panels[1])
    return scene, figure, panels


def _configure_view(view, scene, panels) -> None:
    controllers = []
    for panel in panels:
        controller = dvz.dvz_arcball(scene, None)
        if not controller:
            raise RuntimeError("dvz_arcball() failed")
        if dvz.dvz_view_bind_controller(view, panel, controller, dvz.DVZ_DIM_MASK_XYZ) != 0:
            raise RuntimeError("dvz_view_bind_controller() failed")
        arcball = dvz.dvz_controller_arcball(controller)
        if not arcball:
            raise RuntimeError("dvz_controller_arcball() failed")
        if dvz.dvz_arcball_set(arcball, INITIAL_ANGLES) != 0:
            raise RuntimeError("dvz_arcball_set() failed")
        if dvz.dvz_arcball_zoom(arcball, INITIAL_ZOOM) != 0:
            raise RuntimeError("dvz_arcball_zoom() failed")
        if dvz.dvz_arcball_pan(arcball, INITIAL_PAN) != 0:
            raise RuntimeError("dvz_arcball_pan() failed")
        controllers.append(controller)

    components = (
        int(dvz.DVZ_CONTROLLER_LINK_ROTATION)
        | int(dvz.DVZ_CONTROLLER_LINK_PAN)
        | int(dvz.DVZ_CONTROLLER_LINK_ZOOM)
    )
    link = dvz.dvz_controller_link(
        scene, controllers[0], controllers[1], components, dvz.DVZ_CONTROLLER_LINK_TWO_WAY
    )
    if not link:
        raise RuntimeError("dvz_controller_link() failed")


def main() -> None:
    scene, figure, panels = _build_scene()

    def configure(view) -> None:
        _configure_view(view, scene, panels)

    ex.run_with_view(scene, figure, "Screen-Space Ambient Occlusion", configure)


if __name__ == "__main__":
    main()
