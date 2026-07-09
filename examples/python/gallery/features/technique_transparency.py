#!/usr/bin/env python3
"""Source-over transparency compared with weighted OIT and depth peeling."""

from __future__ import annotations

import ctypes

import datoviz as dvz

from examples.python.gallery import common as ex


LABELS = (b"source-over", b"weighted OIT", b"depth peel")
ALPHA_MODES = (dvz.DVZ_ALPHA_BLENDED, dvz.DVZ_ALPHA_WBOIT, dvz.DVZ_ALPHA_DEPTH_PEEL)
INITIAL_ANGLES = (ctypes.c_float * 3)(0.50, -0.18, 0.22)


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


def _rgba(color, alpha: int):
    return dvz.DvzColor(color.r, color.g, color.b, int(alpha))


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


def _cube_face_colors(alpha: int, primary):
    return (dvz.DvzColor * 6)(
        _rgba(primary, alpha),
        _rgba(ex.CYAN, alpha),
        _rgba(ex.GREEN, alpha),
        _rgba(ex.YELLOW, alpha),
        _rgba(ex.TEXT, alpha),
        _rgba(ex.BLUE, alpha),
    )


def _add_transparent_cube(scene, panel, *, size: float, position, alpha: int, mode, primary) -> None:
    face_colors = _cube_face_colors(alpha, primary)

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
    material.standard.roughness = 0.42
    material.standard.specular = 0.30
    material.standard.rim_strength = 0.20
    material.alpha_mode = mode
    if dvz.dvz_visual_set_material(mesh, ctypes.byref(material)) != 0:
        raise RuntimeError("dvz_visual_set_material() failed")
    if dvz.dvz_visual_set_alpha_mode(mesh, mode) != 0:
        raise RuntimeError("dvz_visual_set_alpha_mode() failed")

    if dvz.dvz_visual_set_transform(mesh, _translation(*position)) != 0:
        raise RuntimeError("dvz_visual_set_transform() failed")
    ex.add_visual(panel, mesh)


def _add_transparent_cubes(scene, panel, mode) -> None:
    _add_transparent_cube(
        scene,
        panel,
        size=1.06,
        position=(-0.20, -0.02, +0.00),
        alpha=112,
        mode=mode,
        primary=ex.CYAN,
    )
    _add_transparent_cube(
        scene,
        panel,
        size=0.78,
        position=(+0.26, +0.10, +0.18),
        alpha=146,
        mode=mode,
        primary=ex.YELLOW,
    )


def _build_scene():
    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError("dvz_scene() failed")
    figure = dvz.dvz_figure(scene, ex.WIDTH, ex.HEIGHT, 0)
    if not figure:
        raise RuntimeError("dvz_figure() failed")

    grid = dvz.dvz_figure_grid(figure, 1, 3)
    if not grid:
        raise RuntimeError("dvz_figure_grid() failed")
    margins = dvz.DvzPanelReserve(42.0, 42.0, 38.0, 38.0)
    if dvz.dvz_grid_set_margins(grid, ctypes.byref(margins)) != 0:
        raise RuntimeError("dvz_grid_set_margins() failed")
    if dvz.dvz_grid_set_gutter(grid, 24.0, 0.0) != 0:
        raise RuntimeError("dvz_grid_set_gutter() failed")

    panels = []
    for col, (label, mode) in enumerate(zip(LABELS, ALPHA_MODES, strict=True)):
        panel = dvz.dvz_grid_panel(grid, 0, col)
        if not panel:
            raise RuntimeError("dvz_grid_panel() failed")
        dvz.dvz_panel_set_background_color(panel, ex.BG)
        ex.manual_camera(panel)
        _add_label(panel, label)
        _add_transparent_cubes(scene, panel, mode)
        panels.append(panel)
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
    for controller in controllers[1:]:
        link = dvz.dvz_controller_link(
            scene, controllers[0], controller, components, dvz.DVZ_CONTROLLER_LINK_TWO_WAY
        )
        if not link:
            raise RuntimeError("dvz_controller_link() failed")


def main() -> None:
    scene, figure, panels = _build_scene()

    def configure(view) -> None:
        _configure_view(view, scene, panels)

    ex.run_with_view(scene, figure, "Transparency Order", configure)


if __name__ == "__main__":
    main()
