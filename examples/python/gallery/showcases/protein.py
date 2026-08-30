#!/usr/bin/env python3
"""Prepared PDB atom arrays rendered as lit sphere impostors."""

from __future__ import annotations

import ctypes
import math
from dataclasses import dataclass
from pathlib import Path

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


DEFAULT_PDB_ID = "6m0j"
DEFAULT_BUNDLE = Path("data/examples/proteins/6m0j/prepared")
LEGACY_FALLBACK_BUNDLE = Path("data/examples/proteins/1ubq/prepared")
ATOM_SCALE = 0.52
ROTATION_SPEED_RAD_PER_SEC = 0.18
INITIAL_ANGLES = (ctypes.c_float * 3)(0.753992, -1.025966, 2.442009)
INITIAL_ZOOM = 0.904839
INITIAL_PAN = (ctypes.c_float * 2)(0.0, 0.0)
PANEL_BG = dvz.DvzColor(22, 27, 34, 255)
GRID = dvz.DvzColor(48, 54, 61, 160)
TEXT = dvz.DvzColor(201, 209, 217, 255)
CYAN = dvz.DvzColor(76, 201, 240, 255)
GREEN = dvz.DvzColor(128, 255, 219, 255)
AMBER = dvz.DvzColor(255, 183, 3, 255)
ERROR = dvz.DvzColor(239, 71, 111, 255)


@dataclass
class ProteinAtoms:
    path: Path
    positions: np.ndarray
    radii: np.ndarray
    colors: np.ndarray

    @property
    def count(self) -> int:
        return int(self.radii.shape[0])


class ProteinState:
    def __init__(self, atoms: ProteinAtoms) -> None:
        self.atoms = atoms
        self.scaled_radii = np.ascontiguousarray(atoms.radii * ATOM_SCALE, dtype=np.float32)
        self.rotation_tracks = []
        self.animations = []

    def destroy_tracks(self) -> None:
        for track in self.rotation_tracks:
            if track:
                dvz.dvz_track_destroy(track)
        self.rotation_tracks.clear()
        self.animations.clear()


def _cache_bundle_path(pdb_id: str) -> Path | None:
    try:
        home = Path.home()
    except RuntimeError:
        return None
    return home / ".cache" / "datoviz" / "proteins" / pdb_id.lower()


def _bundle_valid(path: Path) -> bool:
    position = path / "atom_position.f32"
    radius = path / "atom_radius_vdw.f32"
    color = path / "atom_color_element.rgba8"
    if not (position.is_file() and radius.is_file() and color.is_file()):
        return False
    position_size = position.stat().st_size
    radius_size = radius.stat().st_size
    color_size = color.stat().st_size
    if position_size == 0 or position_size % (3 * np.dtype(np.float32).itemsize) != 0:
        return False
    count = position_size // (3 * np.dtype(np.float32).itemsize)
    return (
        radius_size == count * np.dtype(np.float32).itemsize
        and color_size == count * 4 * np.dtype(np.uint8).itemsize
    )


def _default_bundle_path() -> Path:
    if _bundle_valid(DEFAULT_BUNDLE):
        return DEFAULT_BUNDLE
    cache = _cache_bundle_path(DEFAULT_PDB_ID)
    if cache is not None and _bundle_valid(cache):
        return cache
    if _bundle_valid(LEGACY_FALLBACK_BUNDLE):
        return LEGACY_FALLBACK_BUNDLE
    raise RuntimeError(
        f"failed to load protein bundle at '{DEFAULT_BUNDLE}'\n"
        "prepare one with:\n"
        "  python tools/preprocess_protein.py 6M0J"
    )


def _showcase_atom_colors(element_colors: np.ndarray) -> np.ndarray:
    colors = np.empty_like(element_colors)
    for i, rgba in enumerate(element_colors):
        r, g, b = (int(v) for v in rgba[:3])
        if r > 210 and g > 210 and b > 210:
            color = TEXT
        elif r > 180 and g < 130 and b < 130:
            color = ERROR
        elif b > r + 30 and b > g + 10:
            color = CYAN
        elif g > r + 20 and g > b + 10:
            color = GREEN
        elif r > 170 and g > 120 and b < 120:
            color = AMBER
        else:
            color = GRID
        colors[i] = (color.r, color.g, color.b, 255)
    return np.ascontiguousarray(colors, dtype=np.uint8)


def _load_atoms(path: Path | None = None) -> ProteinAtoms:
    path = _default_bundle_path() if path is None else path
    if not _bundle_valid(path):
        raise RuntimeError(f"failed to load protein bundle at '{path}'")

    positions = np.fromfile(path / "atom_position.f32", dtype=np.float32).reshape((-1, 3))
    radii = np.fromfile(path / "atom_radius_vdw.f32", dtype=np.float32)
    element_colors = np.fromfile(path / "atom_color_element.rgba8", dtype=np.uint8).reshape((-1, 4))
    if not (positions.shape[0] == radii.shape[0] == element_colors.shape[0]):
        raise RuntimeError("protein: inconsistent atom array lengths")
    return ProteinAtoms(
        path=path,
        positions=np.ascontiguousarray(positions, dtype=np.float32),
        radii=np.ascontiguousarray(radii, dtype=np.float32),
        colors=_showcase_atom_colors(element_colors),
    )


def _selected_atom(atoms: ProteinAtoms) -> int:
    score = (
        atoms.positions[:, 0]
        + 0.42 * atoms.positions[:, 2]
        - 0.10 * np.abs(atoms.positions[:, 1])
        + 0.12 * atoms.radii
    )
    return int(np.argmax(score))


def _setup_camera(panel) -> None:
    camera = dvz.dvz_camera_desc()
    camera.projection.type = dvz.DVZ_CAMERA_PERSPECTIVE
    camera.view.eye[:] = (0.519, -0.08, 2.95)
    camera.view.target[:] = (0.0, 0.0, 0.0)
    camera.view.up[:] = (0.0, 1.0, 0.0)
    camera.projection.fov_y = 0.57
    camera.projection.near_clip = 0.05
    camera.projection.far_clip = 100.0
    camera.projection.ortho_height = 0.0
    if dvz.dvz_panel_set_camera_desc(panel, ctypes.byref(camera)) != 0:
        raise RuntimeError("dvz_panel_set_camera_desc() failed")


def _set_ao(panel) -> None:
    desc = dvz.dvz_ao_desc()
    desc.radius = 1.296
    desc.intensity = 5.899
    desc.thickness = 0.367
    desc.min_visibility = 0.157
    desc.quality = dvz.DVZ_AO_QUALITY_ULTRA
    desc.debug_mode = dvz.DVZ_AO_DEBUG_NONE
    if dvz.dvz_panel_set_ao(panel, ctypes.byref(desc)) != 0:
        raise RuntimeError("dvz_panel_set_ao() failed")


def _material():
    material = dvz.dvz_material_desc()
    material.model = dvz.DVZ_MATERIAL_MODEL_STANDARD
    material.alpha_mode = dvz.DVZ_ALPHA_OPAQUE
    material.opacity = 1.0
    material.base_color_factor[:] = (1.0, 1.0, 1.0, 1.0)
    material.phong.ambient = 0.24
    material.phong.diffuse = 0.82
    material.phong.specular = 0.24
    material.phong.shininess = 26.0
    material.standard.roughness = 0.404
    material.standard.specular = 0.659
    material.standard.metallic = 0.0
    material.standard.emissive[:] = (0.0, 0.0, 0.0)
    material.standard.rim_strength = 0.074
    return material


def _add_spheres(scene, panel, state: ProteinState, material):
    spheres = dvz.dvz_sphere(scene, dvz.DVZ_SPHERE_FLAGS_LIGHTING)
    if not spheres:
        raise RuntimeError("dvz_sphere() failed")
    if dvz.dvz_sphere_set_mode(spheres, dvz.DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) != 0:
        raise RuntimeError("dvz_sphere_set_mode() failed")
    if dvz.dvz_visual_set_material(spheres, ctypes.byref(material)) != 0:
        raise RuntimeError("dvz_visual_set_material(spheres) failed")
    if dvz.dvz_visual_set_data_many(
        spheres,
        {
            "position": state.atoms.positions,
            "color": state.atoms.colors,
            "radius": state.scaled_radii,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(spheres) failed")
    ex.add_visual(panel, spheres)
    return spheres


def _selection_arrays(atoms: ProteinAtoms, atom_index: int):
    p = atoms.positions[atom_index]
    radius = float(atoms.radii[atom_index] * ATOM_SCALE * 1.32)
    halo_position = np.ascontiguousarray(p.reshape((1, 3)), dtype=np.float32)
    halo_radius = np.array([radius], dtype=np.float32)
    halo_color = np.array([[AMBER.r, AMBER.g, AMBER.b, 255]], dtype=np.uint8)

    r = radius * 2.15
    gap = radius * 1.18
    starts = np.array(
        [
            [p[0] - r, p[1], p[2]],
            [p[0] + gap, p[1], p[2]],
            [p[0], p[1] - r, p[2]],
            [p[0], p[1] + gap, p[2]],
            [p[0], p[1], p[2] - r],
            [p[0], p[1], p[2] + gap],
        ],
        dtype=np.float32,
    )
    ends = np.array(
        [
            [p[0] - gap, p[1], p[2]],
            [p[0] + r, p[1], p[2]],
            [p[0], p[1] - gap, p[2]],
            [p[0], p[1] + r, p[2]],
            [p[0], p[1], p[2] - gap],
            [p[0], p[1], p[2] + r],
        ],
        dtype=np.float32,
    )
    colors = np.array(
        [
            [AMBER.r, AMBER.g, AMBER.b, 245],
            [AMBER.r, AMBER.g, AMBER.b, 245],
            [CYAN.r, CYAN.g, CYAN.b, 220],
            [CYAN.r, CYAN.g, CYAN.b, 220],
            [CYAN.r, CYAN.g, CYAN.b, 180],
            [CYAN.r, CYAN.g, CYAN.b, 180],
        ],
        dtype=np.uint8,
    )
    widths = np.array([2.8, 2.8, 2.4, 2.4, 1.8, 1.8], dtype=np.float32)
    return halo_position, halo_radius, halo_color, starts, ends, colors, widths


def _add_selection(scene, panel, state: ProteinState, material):
    selection = dvz.dvz_sphere(scene, dvz.DVZ_SPHERE_FLAGS_LIGHTING)
    crosshair = dvz.dvz_segment(scene, 0)
    if not selection or not crosshair:
        raise RuntimeError("selection visual creation failed")
    if dvz.dvz_sphere_set_mode(selection, dvz.DVZ_SPHERE_MODE_RAYCAST_IMPOSTOR) != 0:
        raise RuntimeError("dvz_sphere_set_mode(selection) failed")
    if dvz.dvz_visual_set_material(selection, ctypes.byref(material)) != 0:
        raise RuntimeError("dvz_visual_set_material(selection) failed")

    atom_index = _selected_atom(state.atoms)
    halo_position, halo_radius, halo_color, starts, ends, colors, widths = _selection_arrays(
        state.atoms, atom_index
    )
    if dvz.dvz_visual_set_data_many(
        selection,
        {
            "position": halo_position,
            "color": halo_color,
            "radius": halo_radius,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(selection) failed")
    if (
        dvz.dvz_segment_set_caps(
            crosshair, dvz.DVZ_SEGMENT_CAP_ROUND, dvz.DVZ_SEGMENT_CAP_ROUND
        )
        != 0
    ):
        raise RuntimeError("dvz_segment_set_caps(crosshair) failed")
    if dvz.dvz_visual_set_depth_test(crosshair, False) != 0:
        raise RuntimeError("dvz_visual_set_depth_test(crosshair) failed")
    if dvz.dvz_visual_set_alpha_mode(crosshair, dvz.DVZ_ALPHA_BLENDED) != 0:
        raise RuntimeError("dvz_visual_set_alpha_mode(crosshair) failed")
    if dvz.dvz_visual_set_data_many(
        crosshair,
        {
            "position_start": starts,
            "position_end": ends,
            "color": colors,
            "stroke_width_px": widths,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(crosshair) failed")
    ex.add_visual(panel, selection)
    ex.add_visual(panel, crosshair)
    return selection, crosshair


def _normalize(v: np.ndarray) -> np.ndarray:
    norm = float(np.linalg.norm(v))
    if norm <= 0.0 or not math.isfinite(norm):
        raise RuntimeError("protein: invalid spin axis")
    return v / norm


def _preview_spin_axis() -> np.ndarray:
    eye = np.array([0.18, -0.08, 2.95], dtype=np.float32)
    target = np.array([0.0, 0.0, 0.0], dtype=np.float32)
    up = np.array([0.0, 1.0, 0.0], dtype=np.float32)
    forward = _normalize(target - eye)
    right = _normalize(np.cross(forward, up))
    screen_up = _normalize(np.cross(right, forward))

    sx, sy, sz = (math.sin(value) for value in INITIAL_ANGLES)
    cx, cy, cz = (math.cos(value) for value in INITIAL_ANGLES)
    arcball_cols = np.array(
        [
            [cy * cz, sx * sy * cz + cx * sz, sx * sz - cx * sy * cz],
            [-cy * sz, cx * cz - sx * sy * sz, sx * cz + cx * sy * sz],
            [sy, -cy * sx, cx * cy],
        ],
        dtype=np.float32,
    )
    return _normalize(arcball_cols @ screen_up).astype(np.float32)


def _attach_rotation(scene, visual, state: ProteinState, axis: np.ndarray) -> None:
    desc = dvz.dvz_track_rotation_desc()
    desc.axis[:] = tuple(float(v) for v in axis)
    desc.speed_rad_per_sec = 1.0
    track = dvz.dvz_track_rotation(ctypes.byref(desc))
    if not track:
        raise RuntimeError("dvz_track_rotation() failed")
    state.rotation_tracks.append(track)

    transform = dvz.dvz_transform_motion_desc()
    transform.rotation = track
    animation = dvz.dvz_anim_visual_transform(scene, visual, ctypes.byref(transform))
    if not animation:
        raise RuntimeError("dvz_anim_visual_transform() failed")
    state.animations.append(animation)
    if dvz.dvz_anim_set_speed(animation, ROTATION_SPEED_RAD_PER_SEC) != 0:
        raise RuntimeError("dvz_anim_set_speed() failed")
    if dvz.dvz_anim_start(animation, 0.0) != 0:
        raise RuntimeError("dvz_anim_start() failed")


def _build_scene(path: Path | None = None):
    atoms = _load_atoms(path)
    state = ProteinState(atoms)
    scene, figure, panel = ex.scene_panel()
    directional = ex.set_panel_directional_light(scene, panel, (0.25, 0.65, 0.72))
    ambient = dvz.dvz_scene_default_ambient(scene)
    if (
        not ambient
        or dvz.dvz_light_set_intensity(ambient, 0.50) != 0
        or dvz.dvz_light_set_intensity(directional, 0.72) != 0
    ):
        raise RuntimeError("failed to configure protein lights")
    dvz.dvz_panel_set_background_color(panel, PANEL_BG)
    _setup_camera(panel)
    _set_ao(panel)

    material = _material()
    spheres = _add_spheres(scene, panel, state, material)
    selection, crosshair = _add_selection(scene, panel, state, material)
    axis = _preview_spin_axis()
    for visual in (spheres, selection, crosshair):
        _attach_rotation(scene, visual, state, axis)
    return scene, figure, panel, state


def _configure_view(view, scene, panel) -> None:
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


def _run(scene, figure, panel, state: ProteinState) -> None:
    print(f"loaded {state.atoms.count} atoms from {state.atoms.path}")
    app = dvz.dvz_app(scene)
    if not app:
        dvz.dvz_scene_destroy(scene)
        state.destroy_tracks()
        raise RuntimeError("dvz_app() failed")

    try:
        view = dvz.dvz_view_window(app, figure, ex.WIDTH, ex.HEIGHT, b"Protein")
        if not view:
            raise RuntimeError("dvz_view_window() failed")
        _configure_view(view, scene, panel)
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
