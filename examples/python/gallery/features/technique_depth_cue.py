#!/usr/bin/env python3
"""Uniform sphere lattice compared with depth-dependent fading."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


LABELS = (b"Plain depth", b"Depth cue")
LATTICE_SIDE = 3
INITIAL_ANGLES = (ctypes.c_float * 3)(0.48, -0.18, 0.20)


def _sphere_lattice_data():
    positions = []
    colors = []
    radii = []
    for z in range(LATTICE_SIDE):
        for y in range(LATTICE_SIDE):
            for x in range(LATTICE_SIDE):
                positions.append((-0.58 + 0.58 * x, -0.44 + 0.44 * y, -0.72 + 0.72 * z))
                colors.append(ex.CYAN)
                radii.append(0.115)
    return (
        np.array(positions, dtype=np.float32),
        ex.color_array(*colors),
        np.array(radii, dtype=np.float32),
    )


def _depth_cue_desc():
    desc = dvz.dvz_depth_cue_desc()
    desc.mode = dvz.DVZ_DEPTH_CUE_FADE_TO_BACKGROUND
    desc.metric = dvz.DVZ_DEPTH_CUE_METRIC_EYE_DISTANCE
    desc.falloff = dvz.DVZ_DEPTH_CUE_FALLOFF_LINEAR
    desc.near_depth = 3.80
    desc.far_depth = 5.80
    desc.strength = 1.0
    desc.density = 0.45
    desc.background_color[:] = (0.035, 0.047, 0.067, 1.0)
    return desc


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


def _add_sphere_lattice(scene, panel, cue=None) -> None:
    sphere = dvz.dvz_sphere(scene, dvz.DVZ_SPHERE_FLAGS_LIGHTING)
    if not sphere:
        raise RuntimeError("dvz_sphere() failed")
    if dvz.dvz_sphere_set_mode(sphere, dvz.DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) != 0:
        raise RuntimeError("dvz_sphere_set_mode() failed")

    positions, colors, radii = _sphere_lattice_data()
    if dvz.dvz_visual_set_data_many(
        sphere,
        {
            "position": positions,
            "color": colors,
            "radius": radii,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(sphere) failed")

    material = dvz.dvz_standard_material_desc()
    material.light_direction[:] = (-0.32, 0.55, 0.76)
    material.standard.roughness = 0.46
    material.standard.specular = 0.44
    material.standard.rim_strength = 0.18
    if dvz.dvz_visual_set_material(sphere, ctypes.byref(material)) != 0:
        raise RuntimeError("dvz_visual_set_material() failed")
    if cue is not None and dvz.dvz_visual_set_depth_cue(sphere, ctypes.byref(cue)) != 0:
        raise RuntimeError("dvz_visual_set_depth_cue() failed")
    ex.add_visual(panel, sphere)


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
        panels.append(panel)

    _add_sphere_lattice(scene, panels[0], None)
    _add_sphere_lattice(scene, panels[1], _depth_cue_desc())
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

    ex.run_with_view(scene, figure, "Depth Cue", configure)


if __name__ == "__main__":
    main()
