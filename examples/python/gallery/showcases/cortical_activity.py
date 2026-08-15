#!/usr/bin/env python3
"""Measured auditory dSPM activity animated on the prepared cortical surface."""

from __future__ import annotations

import ctypes
import struct
from dataclasses import dataclass
from pathlib import Path

import numpy as np

import datoviz as dvz
from examples.python.gallery import common as ex


PATHS = (Path("data/examples/cortical_activity/prepared/cortical_activity.bin"), Path(".cache/datoviz/examples/cortical_activity/prepared/cortical_activity.bin"))
HEADER_SIZE = 112
ACTIVITY_MIN, ACTIVITY_MID, ACTIVITY_MAX = 5.668, 10.387, 15.783
ANATOMY = np.array([65, 69, 74], dtype=np.float32)
MAGMA = np.array([[0, 0, 4], [81, 18, 124], [183, 55, 121], [252, 137, 97], [252, 253, 191]], dtype=np.float32)
INITIAL_ANGLES = (ctypes.c_float * 3)(+0.065367, -1.124700, +0.325712)


@dataclass
class CorticalData:
    times: np.ndarray
    positions: np.ndarray
    indices: np.ndarray
    interpolation_indices: np.ndarray
    interpolation_weights: np.ndarray
    values: np.ndarray


def _load() -> CorticalData:
    path = next((candidate for candidate in PATHS if candidate.exists()), None)
    if path is None:
        raise FileNotFoundError("missing cortical activity bundle; run `python tools/data/prepare_cortical_activity.py`")
    payload = path.read_bytes()
    if payload[:7] != b"DVZCTA4":
        raise ValueError(f"invalid cortical activity bundle: {path}")
    u32 = lambda offset: struct.unpack_from("<I", payload, offset)[0]
    version, header_size, hemisphere_count = u32(8), u32(12), u32(16)
    time_count, source_count, _source_index_count = u32(20), u32(24), u32(28)
    render_count, render_index_count = u32(32), u32(36)
    if version != 4 or header_size != HEADER_SIZE or hemisphere_count != 2:
        raise ValueError(f"unsupported cortical activity bundle: {path}")
    offset = HEADER_SIZE

    def array(dtype, count, shape):
        nonlocal offset
        result = np.frombuffer(payload, dtype, count, offset).reshape(shape).copy()
        offset += result.nbytes
        return result

    times = array("<f4", time_count, (time_count,))
    pial = array("<f4", 3 * render_count, (render_count, 3))
    indices = array("<u4", render_index_count, (render_index_count,))
    interpolation_indices = array("<u4", 3 * render_count, (render_count, 3))
    interpolation_weights = array("<f4", 3 * render_count, (render_count, 3))
    values = array("<f4", time_count * source_count, (time_count, source_count))
    if offset != len(payload):
        raise ValueError(f"unexpected cortical activity bundle size: {path}")
    positions = np.column_stack((-pial[:, 0], pial[:, 2], pial[:, 1])).astype(np.float32)
    return CorticalData(times, positions, indices, interpolation_indices, interpolation_weights, values)


def _normals(positions: np.ndarray, indices: np.ndarray) -> np.ndarray:
    faces = indices.reshape(-1, 3)
    triangles = positions[faces]
    face_normals = np.cross(triangles[:, 1] - triangles[:, 0], triangles[:, 2] - triangles[:, 0])
    normals = np.zeros_like(positions)
    np.add.at(normals, faces[:, 0], face_normals)
    np.add.at(normals, faces[:, 1], face_normals)
    np.add.at(normals, faces[:, 2], face_normals)
    normals /= np.maximum(np.linalg.norm(normals, axis=1, keepdims=True), 1e-12)
    return normals


def _magma(values: np.ndarray) -> np.ndarray:
    coordinate = np.clip(values, 0.0, 1.0) * (len(MAGMA) - 1)
    lower = coordinate.astype(np.int32)
    upper = np.minimum(lower + 1, len(MAGMA) - 1)
    local = (coordinate - lower)[:, None]
    return (1.0 - local) * MAGMA[lower] + local * MAGMA[upper]


def _colors(data: CorticalData, time_ms: float) -> np.ndarray:
    upper = int(np.clip(np.searchsorted(data.times, time_ms, side="left"), 1, len(data.times) - 1))
    lower = upper - 1
    alpha = float((time_ms - data.times[lower]) / (data.times[upper] - data.times[lower]))
    source = (1.0 - alpha) * data.values[lower] + alpha * data.values[upper]
    render = np.sum(data.interpolation_weights * source[data.interpolation_indices], axis=1)
    normalized = np.clip((render - ACTIVITY_MIN) / (ACTIVITY_MAX - ACTIVITY_MIN), 0.0, 1.0)
    visibility = np.clip((render - ACTIVITY_MIN) / (ACTIVITY_MID - ACTIVITY_MIN), 0.0, 1.0)[:, None]
    rgb = (1.0 - visibility) * ANATOMY + visibility * _magma(normalized)
    return np.column_stack((np.clip(rgb + 0.5, 0, 255).astype(np.uint8), np.full(len(rgb), 255, np.uint8)))


def _build_scene():
    data = _load()
    normals = _normals(data.positions, data.indices)
    peak_time = float(data.times[np.unravel_index(np.argmax(data.values), data.values.shape)[0]])
    scene, figure, panel = ex.scene_panel()
    ex.set_panel_directional_light(scene, panel, (-0.35, +0.55, +0.75))
    camera = dvz.dvz_camera_desc()
    camera.view.eye[:] = (-0.125, +1.0, +2.0)
    camera.view.target[:] = (0.0, 0.0, 0.0)
    camera.view.up[:] = (+0.210683, +0.897238, -0.388042)
    dvz.dvz_panel_set_camera_desc(panel, ctypes.byref(camera))

    mesh = dvz.dvz_mesh(scene, 0)
    dvz.dvz_visual_set_data_many(mesh, {"position": data.positions, "normal": normals, "color": _colors(data, peak_time)})
    dvz.dvz_visual_set_index_data(mesh, data.indices)
    material = dvz.dvz_phong_material_desc()
    material.phong.ambient = 0.225
    material.phong.diffuse = 0.864
    material.phong.specular = 0.129
    material.phong.shininess = 56.687
    dvz.dvz_visual_set_material(mesh, ctypes.byref(material))
    ex.add_visual(panel, mesh)
    return scene, figure, panel, mesh, data, peak_time


def main() -> None:
    scene, figure, panel, mesh, data, peak_time = _build_scene()
    print(f"cortical_activity: {len(data.positions)} render vertices, {len(data.times)} time samples")
    first, last = float(data.times[0]), float(data.times[-1])
    rate = (last - first) / 3.6

    def configure(view) -> None:
        arcball = dvz.dvz_view_arcball(view, panel, None)
        if not arcball or dvz.dvz_arcball_set(arcball, INITIAL_ANGLES) != 0:
            raise RuntimeError("arcball setup failed")
        dvz.dvz_arcball_zoom(arcball, 0.904838)
        pan = (ctypes.c_float * 2)(+0.103135, +0.038889)
        dvz.dvz_arcball_pan(arcball, pan)

    def on_frame(_view, frame_index: int, elapsed: float) -> None:
        if frame_index % 2:
            return
        time_ms = first + ((peak_time - first + rate * elapsed) % (last - first))
        if dvz.dvz_visual_set_data(mesh, "color", _colors(data, time_ms)) != 0:
            raise RuntimeError("cortical activity update failed")

    ex.run_with_frame_callback(scene, figure, "Human Auditory Cortical Activity", on_frame, configure)


if __name__ == "__main__":
    main()
