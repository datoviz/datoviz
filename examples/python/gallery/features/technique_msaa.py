#!/usr/bin/env python3
"""Single-sample rendering compared with 8x multisample antialiasing."""

from __future__ import annotations

import ctypes

import datoviz as dvz

from examples.python.gallery import common as ex


LABELS = (b"Single sample", b"8x MSAA")
INITIAL_ANGLES = (ctypes.c_float * 3)(0.58, -0.24, 0.18)
CUBE_POSITIONS = (
    (-0.54, -0.30, -0.05),
    (+0.04, -0.08, +0.12),
    (+0.52, +0.16, +0.00),
    (-0.10, +0.45, +0.18),
)
CUBE_SIZES = (0.50, 0.62, 0.44, 0.34)


def _translation(x: float, y: float, z: float):
    transform = ((ctypes.c_float * 4) * 4)()
    transform[0][0] = 1.0
    transform[1][1] = 1.0
    transform[2][2] = 1.0
    transform[3][3] = 1.0
    transform[3][0] = float(x)
    transform[3][1] = float(y)
    transform[3][2] = float(z)
    return transform


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


def _add_cube(scene, panel, position, size: float) -> None:
    face_colors = (dvz.DvzColor * 6)(ex.CYAN, ex.GREEN, ex.YELLOW, ex.TEXT, ex.BLUE, ex.RED)

    desc = dvz.dvz_geometry_cube_desc()
    desc.size = float(size)
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
    material.standard.roughness = 0.46
    material.standard.specular = 0.34
    material.standard.rim_strength = 0.18
    if dvz.dvz_visual_set_material(mesh, ctypes.byref(material)) != 0:
        raise RuntimeError("dvz_visual_set_material() failed")

    transform = _translation(*position)
    if dvz.dvz_visual_set_transform(mesh, transform) != 0:
        raise RuntimeError("dvz_visual_set_transform() failed")
    ex.add_visual(panel, mesh)


def _add_cube_cluster(scene, panel) -> None:
    for position, size in zip(CUBE_POSITIONS, CUBE_SIZES, strict=True):
        _add_cube(scene, panel, position, size)


def _set_msaa(panel) -> None:
    desc = dvz.dvz_msaa_desc()
    desc.enabled = True
    desc.sample_count = 8
    desc.alpha_to_coverage = False
    if dvz.dvz_panel_set_msaa(panel, ctypes.byref(desc)) != 0:
        raise RuntimeError("dvz_panel_set_msaa() failed")


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
        _add_cube_cluster(scene, panel)
        panels.append(panel)

    _set_msaa(panels[1])
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

    ex.run_with_view(scene, figure, "Multisample Antialiasing", configure)


if __name__ == "__main__":
    main()
