#!/usr/bin/env python3
"""Vectorized real spherical-harmonic deformation of an indexed icosphere."""

from __future__ import annotations

import ctypes
import math

import numpy as np

import datoviz as dvz
from examples.python.gallery import common as ex


SUBDIVISIONS = 4
MORPH_PERIOD = 8.0
TERMS = np.array(
    [
        (7, +3, +1.00, 0.22, 1.0),
        (6, -5, +0.58, 0.25, 2.0),
        (5, +2, -0.38, 0.20, 3.0),
        (4, -1, +0.25, 0.18, 1.0),
    ],
    dtype=[("degree", "i4"), ("order", "i4"), ("base", "f8"), ("morph", "f8"), ("frequency", "f8")],
)
PALETTE = np.array(
    [[18, 47, 57], [34, 211, 238], [217, 226, 236], [250, 183, 3], [239, 71, 111]],
    dtype=np.float64,
)
INITIAL_ANGLES = (ctypes.c_float * 3)(-0.233247, +0.292663, -0.038705)


def _icosphere(subdivisions: int) -> tuple[np.ndarray, np.ndarray]:
    phi = (1.0 + np.sqrt(5.0)) / 2.0
    vertices = np.array(
        [
            [-1, +phi, 0], [+1, +phi, 0], [-1, -phi, 0], [+1, -phi, 0],
            [0, -1, +phi], [0, +1, +phi], [0, -1, -phi], [0, +1, -phi],
            [+phi, 0, -1], [+phi, 0, +1], [-phi, 0, -1], [-phi, 0, +1],
        ],
        dtype=np.float64,
    )
    vertices /= np.linalg.norm(vertices, axis=1, keepdims=True)
    faces = np.array(
        [
            [0, 11, 5], [0, 5, 1], [0, 1, 7], [0, 7, 10], [0, 10, 11],
            [1, 5, 9], [5, 11, 4], [11, 10, 2], [10, 7, 6], [7, 1, 8],
            [3, 9, 4], [3, 4, 2], [3, 2, 6], [3, 6, 8], [3, 8, 9],
            [4, 9, 5], [2, 4, 11], [6, 2, 10], [8, 6, 7], [9, 8, 1],
        ],
        dtype=np.uint32,
    )
    for _ in range(subdivisions):
        edge_cache: dict[tuple[int, int], int] = {}
        next_vertices = vertices.tolist()

        def midpoint(a: int, b: int) -> int:
            key = (min(a, b), max(a, b))
            if key not in edge_cache:
                value = vertices[a] + vertices[b]
                value /= np.linalg.norm(value)
                edge_cache[key] = len(next_vertices)
                next_vertices.append(value.tolist())
            return edge_cache[key]

        next_faces = []
        for a, b, c in faces:
            ab, bc, ca = midpoint(int(a), int(b)), midpoint(int(b), int(c)), midpoint(int(c), int(a))
            next_faces.extend(((a, ab, ca), (b, bc, ab), (c, ca, bc), (ab, bc, ca)))
        vertices = np.asarray(next_vertices, dtype=np.float64)
        faces = np.asarray(next_faces, dtype=np.uint32)
    return vertices, faces


def _associated_legendre(degree: int, order: int, x: np.ndarray) -> np.ndarray:
    p_mm = np.ones_like(x)
    if order:
        root = np.sqrt(np.maximum(0.0, 1.0 - x * x))
        for factor in range(1, 2 * order, 2):
            p_mm *= -factor * root
    if degree == order:
        return p_mm
    previous = x * (2 * order + 1) * p_mm
    if degree == order + 1:
        return previous
    previous2 = p_mm
    for level in range(order + 2, degree + 1):
        current = ((2 * level - 1) * x * previous - (level + order - 1) * previous2) / (level - order)
        previous2, previous = previous, current
    return previous


def _basis(directions: np.ndarray) -> np.ndarray:
    cos_theta = np.clip(directions[:, 2], -1.0, 1.0)
    azimuth = np.arctan2(directions[:, 1], directions[:, 0])
    columns = []
    for term in TERMS:
        degree, order = int(term["degree"]), int(term["order"])
        absolute_order = abs(order)
        normalization = np.sqrt(
            (2 * degree + 1) / (4 * np.pi)
            * np.exp(math.lgamma(degree - absolute_order + 1) - math.lgamma(degree + absolute_order + 1))
        )
        values = normalization * _associated_legendre(degree, absolute_order, cos_theta)
        if order > 0:
            values *= np.sqrt(2.0) * np.cos(absolute_order * azimuth)
        elif order < 0:
            values *= np.sqrt(2.0) * np.sin(absolute_order * azimuth)
        columns.append(values)
    return np.column_stack(columns)


def _colors(amplitude: np.ndarray) -> np.ndarray:
    coordinate = 2.0 * np.clip(amplitude + 1.0, 0.0, 2.0)
    segment = np.minimum(coordinate.astype(np.int32), 3)
    local = (coordinate - segment)[:, None]
    rgb = (1.0 - local) * PALETTE[segment] + local * PALETTE[segment + 1]
    return np.column_stack((np.clip(rgb + 0.5, 0, 255).astype(np.uint8), np.full(len(rgb), 255, np.uint8)))


def _geometry(directions: np.ndarray, faces: np.ndarray, basis: np.ndarray, phase: float):
    weights = TERMS["base"] + TERMS["morph"] * np.sin(2.0 * np.pi * TERMS["frequency"] * phase)
    amplitude = basis @ weights
    amplitude /= max(float(np.max(np.abs(amplitude))), 1e-12)
    smooth = (np.sqrt(amplitude * amplitude + 0.18**2) - 0.18) / (np.sqrt(1.0 + 0.18**2) - 0.18)
    breathing = 0.96 + 0.04 * np.cos(2.0 * np.pi * phase)
    positions = directions * (0.54 + 0.94 * breathing * smooth)[:, None]

    triangles = positions[faces]
    face_normals = np.cross(triangles[:, 1] - triangles[:, 0], triangles[:, 2] - triangles[:, 0])
    normals = np.zeros_like(positions)
    np.add.at(normals, faces[:, 0], face_normals)
    np.add.at(normals, faces[:, 1], face_normals)
    np.add.at(normals, faces[:, 2], face_normals)
    normals /= np.maximum(np.linalg.norm(normals, axis=1, keepdims=True), 1e-12)
    return positions.astype(np.float32), normals.astype(np.float32), _colors(amplitude)


def _build_scene():
    directions, faces = _icosphere(SUBDIVISIONS)
    basis = _basis(directions)
    positions, normals, colors = _geometry(directions, faces, basis, 0.0)
    scene, figure, panel = ex.scene_panel()
    ex.set_panel_directional_light(scene, panel, (-0.48, +0.62, +0.72))

    camera = dvz.dvz_camera_desc()
    camera.view.eye[:] = (-0.46, +2.20, +3.42)
    camera.view.target[:] = (0.0, 0.0, 0.0)
    camera.view.up[:] = (0.0, 1.0, 0.0)
    camera.projection.fov_y = 0.58
    camera.projection.near_clip = 0.05
    camera.projection.far_clip = 100.0
    if dvz.dvz_panel_set_camera_desc(panel, ctypes.byref(camera)) != 0:
        raise RuntimeError("dvz_panel_set_camera_desc() failed")

    mesh = dvz.dvz_mesh(scene, 0)
    if not mesh:
        raise RuntimeError("dvz_mesh() failed")
    if dvz.dvz_visual_set_data_many(mesh, {"position": positions, "normal": normals, "color": colors}) != 0:
        raise RuntimeError("dvz_visual_set_data_many(mesh) failed")
    if dvz.dvz_visual_set_index_data(mesh, faces.reshape(-1)) != 0:
        raise RuntimeError("dvz_visual_set_index_data() failed")
    material = dvz.dvz_standard_material_desc()
    material.standard.roughness = 0.58
    material.standard.specular = 0.16
    material.standard.rim_strength = 0.12
    if dvz.dvz_visual_set_material(mesh, ctypes.byref(material)) != 0:
        raise RuntimeError("dvz_visual_set_material() failed")
    ex.add_visual(panel, mesh)
    return scene, figure, panel, mesh, directions, faces, basis


def main() -> None:
    scene, figure, panel, mesh, directions, faces, basis = _build_scene()

    def configure(view) -> None:
        arcball = dvz.dvz_view_arcball(view, panel, None)
        if not arcball or dvz.dvz_arcball_set(arcball, INITIAL_ANGLES) != 0:
            raise RuntimeError("arcball setup failed")
        dvz.dvz_arcball_zoom(arcball, 0.606531)

    def on_frame(_view, _frame_index: int, elapsed: float) -> None:
        positions, normals, colors = _geometry(directions, faces, basis, (elapsed / MORPH_PERIOD) % 1.0)
        if dvz.dvz_visual_set_data_many(mesh, {"position": positions, "normal": normals, "color": colors}) != 0:
            raise RuntimeError("spherical-harmonics update failed")

    ex.run_with_frame_callback(scene, figure, "Spherical Harmonics", on_frame, configure)


if __name__ == "__main__":
    main()
