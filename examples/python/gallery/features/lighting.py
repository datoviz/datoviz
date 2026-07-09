#!/usr/bin/env python3
"""Lighting and standard-material variants on matching sphere clusters."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


LABELS = (b"Matte key light", b"Glossy side light", b"Rim highlight")
SPHERE_COUNT = 9


def _materials():
    matte = dvz.dvz_standard_material_desc()
    matte.light_direction[:] = (0.34, 0.46, 0.82)
    matte.standard.roughness = 0.86
    matte.standard.specular = 0.12
    matte.standard.rim_strength = 0.05

    glossy = dvz.dvz_standard_material_desc()
    glossy.light_direction[:] = (0.12, 0.70, 0.62)
    glossy.standard.roughness = 0.42
    glossy.standard.specular = 0.60
    glossy.standard.rim_strength = 0.18

    rim = dvz.dvz_standard_material_desc()
    rim.light_direction[:] = (0.62, 0.18, 0.76)
    rim.standard.roughness = 0.24
    rim.standard.specular = 0.78
    rim.standard.rim_strength = 0.42
    return matte, glossy, rim


def _sphere_data():
    positions = np.array(
        [
            [-0.56, -0.20, -0.20],
            [-0.29, -0.20, +0.05],
            [+0.00, -0.20, +0.18],
            [+0.29, -0.20, +0.05],
            [+0.56, -0.20, -0.20],
            [-0.40, +0.15, -0.06],
            [-0.11, +0.15, +0.16],
            [+0.19, +0.15, +0.12],
            [+0.47, +0.15, -0.08],
        ],
        dtype=np.float32,
    )
    radii = (
        0.82
        * np.array([0.150, 0.170, 0.190, 0.170, 0.150, 0.165, 0.205, 0.185, 0.155])
    ).astype(np.float32)
    colors = ex.color_array(
        ex.CYAN, ex.GREEN, ex.YELLOW, ex.GREEN, ex.CYAN, ex.BLUE, ex.TEXT, ex.YELLOW, ex.BLUE
    )
    return positions, radii, colors


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


def _add_lit_spheres(scene, panel, material) -> None:
    sphere = dvz.dvz_sphere(scene, dvz.DVZ_SPHERE_FLAGS_LIGHTING)
    if not sphere:
        raise RuntimeError("dvz_sphere() failed")
    if dvz.dvz_sphere_set_mode(sphere, dvz.DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) != 0:
        raise RuntimeError("dvz_sphere_set_mode() failed")
    if dvz.dvz_visual_set_material(sphere, ctypes.byref(material)) != 0:
        raise RuntimeError("dvz_visual_set_material() failed")

    positions, radii, colors = _sphere_data()
    if dvz.dvz_visual_set_data_many(
        sphere,
        {
            "position": positions,
            "radius": radii,
            "color": colors,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(sphere) failed")
    ex.add_visual(panel, sphere)


def _build_scene():
    scene = dvz.dvz_scene()
    if not scene:
        raise RuntimeError("dvz_scene() failed")
    figure = dvz.dvz_figure(scene, ex.WIDTH, ex.HEIGHT, 0)
    if not figure:
        raise RuntimeError("dvz_figure() failed")

    materials = _materials()
    panel_rects = ((0.000, 0.0, 0.334, 1.0), (0.333, 0.0, 0.334, 1.0), (0.666, 0.0, 0.334, 1.0))
    panels = []
    for label, material, rect in zip(LABELS, materials, panel_rects, strict=True):
        panel = ex.panel_rect(figure, *rect)
        ex.manual_camera(panel)
        _add_label(panel, label)
        _add_lit_spheres(scene, panel, material)
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

    ex.run_with_view(scene, figure, "Lighting", configure)


if __name__ == "__main__":
    main()
