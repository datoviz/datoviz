#!/usr/bin/env python3
"""View-space ambient occlusion on a protein-like synthetic aggregate."""

from __future__ import annotations

import ctypes

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


LABELS = (b"Plain lighting", b"Ambient occlusion")
INITIAL_ANGLES = (ctypes.c_float * 3)(-0.239547, -0.407204, +0.824558)
INITIAL_PAN = (ctypes.c_float * 2)(0.0, -0.020)
INITIAL_ZOOM = 0.505017
AGGREGATE_TARGET_SPHERES = 2600
AGGREGATE_MAX_CANDIDATES = 200_000
AGGREGATE_HASH_STEP = 0.080


def _hash_u32(value: int) -> int:
    value &= 0xFFFFFFFF
    value ^= value >> 16
    value = (value * 0x7FEB352D) & 0xFFFFFFFF
    value ^= value >> 15
    value = (value * 0x846CA68B) & 0xFFFFFFFF
    value ^= value >> 16
    return value & 0xFFFFFFFF


def _hash_unit(value: int) -> float:
    return (_hash_u32(value) & 0x00FFFFFF) / 16777215.0


def _sphere_data():
    positions = []
    radii = []
    spatial_hash: dict[tuple[int, int, int], list[int]] = {}
    lobe_centers = (
        (-0.40, +0.12, -0.02),
        (+0.18, +0.17, +0.06),
        (+0.55, -0.24, -0.08),
        (-0.26, -0.38, +0.10),
        (+0.08, -0.18, -0.30),
    )
    lobe_radii = (
        (0.58, 0.45, 0.43),
        (0.54, 0.49, 0.46),
        (0.40, 0.34, 0.36),
        (0.43, 0.32, 0.35),
        (0.38, 0.32, 0.34),
    )

    for candidate in range(AGGREGATE_MAX_CANDIDATES):
        if len(positions) >= AGGREGATE_TARGET_SPHERES:
            break
        px = -1.05 + 2.10 * _hash_unit(4 * candidate + 0)
        py = -0.90 + 1.80 * _hash_unit(4 * candidate + 1)
        pz = -0.80 + 1.60 * _hash_unit(4 * candidate + 2)
        radius = 0.024 + 0.012 * _hash_unit(4 * candidate + 3)
        inside = any(
            ((px - cx) / rx) ** 2 + ((py - cy) / ry) ** 2 + ((pz - cz) / rz) ** 2 <= 1
            for (cx, cy, cz), (rx, ry, rz) in zip(lobe_centers, lobe_radii)
        )
        pocket0 = (px + 0.03) ** 2 + (py - 0.31) ** 2 + (pz - 0.48) ** 2 < 0.070
        pocket1 = (px - 0.48) ** 2 + (py - 0.08) ** 2 + (pz - 0.24) ** 2 < 0.030
        if not inside or pocket0 or pocket1:
            continue

        cell = (
            int(np.floor((px + 1.05) / AGGREGATE_HASH_STEP)),
            int(np.floor((py + 0.90) / AGGREGATE_HASH_STEP)),
            int(np.floor((pz + 0.80) / AGGREGATE_HASH_STEP)),
        )
        overlaps = False
        for dz in range(-1, 2):
            for dy in range(-1, 2):
                for dx in range(-1, 2):
                    for index in spatial_hash.get(
                        (cell[0] + dx, cell[1] + dy, cell[2] + dz), ()
                    ):
                        qx, qy, qz = positions[index]
                        minimum = radius + radii[index] + 0.002
                        if (px - qx) ** 2 + (py - qy) ** 2 + (pz - qz) ** 2 < minimum**2:
                            overlaps = True
                            break
                    if overlaps:
                        break
                if overlaps:
                    break
            if overlaps:
                break
        if overlaps:
            continue
        spatial_hash.setdefault(cell, []).append(len(positions))
        positions.append((px, py, pz))
        radii.append(radius)

    count = len(positions)
    colors = np.tile(np.array([[ex.CYAN.r, ex.CYAN.g, ex.CYAN.b, ex.CYAN.a]]), (count, 1))
    return (
        np.asarray(positions, dtype=np.float32),
        np.asarray(radii, dtype=np.float32),
        colors.astype(np.uint8),
    )


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


def _set_ao(panel) -> None:
    desc = dvz.dvz_ao_desc()
    desc.radius = 0.560
    desc.intensity = 3.318
    desc.thickness = 0.128
    desc.min_visibility = 0.000
    desc.quality = dvz.DVZ_AO_QUALITY_ULTRA
    desc.debug_mode = dvz.DVZ_AO_DEBUG_NONE
    if dvz.dvz_panel_set_ao(panel, ctypes.byref(desc)) != 0:
        raise RuntimeError("dvz_panel_set_ao() failed")


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

    _set_ao(panels[1])
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
        if dvz.dvz_arcball_initial(arcball, INITIAL_ANGLES) != 0:
            raise RuntimeError("dvz_arcball_initial() failed")
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
