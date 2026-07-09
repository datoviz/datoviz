#!/usr/bin/env python3
"""Material descriptors compared on matching retained cube meshes."""

from __future__ import annotations

import ctypes

import datoviz as dvz

from examples.python.gallery import common as ex


LABELS = (b"Matte Phong", b"Glossy Phong", b"Standard rim")
INITIAL_ANGLES = (ctypes.c_float * 3)(0.58, -0.14, 0.26)


def _materials():
    matte = dvz.dvz_phong_material_desc()
    matte.phong.ambient = 0.34
    matte.phong.diffuse = 0.84
    matte.phong.specular = 0.02
    matte.phong.shininess = 8.0

    glossy = dvz.dvz_phong_material_desc()
    glossy.phong.ambient = 0.18
    glossy.phong.diffuse = 0.70
    glossy.phong.specular = 0.48
    glossy.phong.shininess = 58.0

    standard = dvz.dvz_standard_material_desc()
    standard.standard.roughness = 0.42
    standard.standard.specular = 0.46
    standard.standard.rim_strength = 0.30
    return matte, glossy, standard


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


def _add_material_cube(scene, panel, material) -> None:
    face_colors = (dvz.DvzColor * 6)(ex.CYAN, ex.GREEN, ex.YELLOW, ex.TEXT, ex.BLUE, ex.RED)

    desc = dvz.dvz_geometry_cube_desc()
    desc.size = 0.72
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

    if dvz.dvz_visual_set_material(mesh, ctypes.byref(material)) != 0:
        raise RuntimeError("dvz_visual_set_material() failed")
    ex.add_visual(panel, mesh)


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

    margins = dvz.DvzPanelReserve(34.0, 34.0, 40.0, 40.0)
    if dvz.dvz_grid_set_margins(grid, ctypes.byref(margins)) != 0:
        raise RuntimeError("dvz_grid_set_margins() failed")
    if dvz.dvz_grid_set_gutter(grid, 24.0, 0.0) != 0:
        raise RuntimeError("dvz_grid_set_gutter() failed")

    materials = _materials()
    panels = []
    for i, (label, material) in enumerate(zip(LABELS, materials, strict=True)):
        panel = dvz.dvz_grid_panel(grid, 0, i)
        if not panel:
            raise RuntimeError("dvz_grid_panel() failed")
        dvz.dvz_panel_set_background_color(panel, ex.BG)
        ex.manual_camera(panel)
        _add_label(panel, label)
        _add_material_cube(scene, panel, material)
        panels.append(panel)
    return scene, figure, panels


def _bind_linked_arcballs(view, scene, panels) -> None:
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
        _bind_linked_arcballs(view, scene, panels)

    ex.run_with_view(scene, figure, "Mesh Materials", configure)


if __name__ == "__main__":
    main()
