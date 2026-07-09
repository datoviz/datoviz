#!/usr/bin/env python3
"""Prepared RGB LiDAR point cloud rendered with a pixel visual."""

from __future__ import annotations

import ctypes
import struct
from dataclasses import dataclass
from pathlib import Path

import numpy as np

import datoviz as dvz

from examples.python.gallery import common as ex


DATA_PATH = Path(".cache/datoviz/examples/point_cloud/prepared/point_cloud.bin")
MAGIC = b"DVZPCD1\x00"
VERSION = 2
HEADER_SIZE = 40
RECORD_SIZE = 32
MAX_POINTS = 8_000_000

PANEL_BG = dvz.DvzColor(13, 18, 24, 255)


@dataclass
class PointCloudData:
    positions: np.ndarray
    colors: np.ndarray
    pixel_sizes: np.ndarray
    bounds_min: tuple[float, float, float]
    bounds_max: tuple[float, float, float]


def _u8(values):
    return np.clip(values * 255.0 + 0.5, 0.0, 255.0).astype(np.uint8)


def _load_header(path: Path):
    if not path.exists():
        raise FileNotFoundError(
            f"{path} is missing; run `python tools/data/prepare_point_cloud.py --force`"
        )
    with path.open("rb") as f:
        header = f.read(HEADER_SIZE)
    if len(header) != HEADER_SIZE:
        raise ValueError(f"{path} is too small for a point-cloud header")

    magic, version, count = struct.unpack_from("<8sII", header, 0)
    bounds_min = struct.unpack_from("<3f", header, 16)
    bounds_max = struct.unpack_from("<3f", header, 28)
    if magic != MAGIC or version != VERSION or count == 0 or count > MAX_POINTS:
        raise ValueError(f"invalid point-cloud header in {path}")
    expected_size = HEADER_SIZE + count * RECORD_SIZE
    actual_size = path.stat().st_size
    if actual_size != expected_size:
        raise ValueError(f"invalid point-cloud size: expected {expected_size}, got {actual_size}")
    return count, bounds_min, bounds_max


def _load_point_cloud(path: Path = DATA_PATH) -> PointCloudData:
    count, bounds_min, bounds_max = _load_header(path)
    record_dtype = np.dtype(
        [
            ("x", "<f4"),
            ("y", "<f4"),
            ("z", "<f4"),
            ("r", "<f4"),
            ("g", "<f4"),
            ("b", "<f4"),
            ("a", "<f4"),
            ("pixel_size_px", "<f4"),
        ]
    )
    records = np.memmap(path, dtype=record_dtype, mode="r", offset=HEADER_SIZE, shape=(count,))

    positions = np.empty((count, 3), dtype=np.float32)
    positions[:, 0] = records["x"]
    positions[:, 1] = records["z"]
    positions[:, 2] = records["y"]

    colors = np.empty((count, 4), dtype=np.uint8)
    colors[:, 0] = _u8(records["r"])
    colors[:, 1] = _u8(records["g"])
    colors[:, 2] = _u8(records["b"])
    colors[:, 3] = _u8(records["a"])

    pixel_sizes = np.array(records["pixel_size_px"], dtype=np.float32, copy=True)
    return PointCloudData(positions, colors, pixel_sizes, bounds_min, bounds_max)


def _camera_desc():
    camera = dvz.dvz_camera_desc()
    camera.projection.type = dvz.DVZ_CAMERA_PERSPECTIVE
    camera.view.eye[:] = (+0.791911, +0.472144, -0.891266)
    camera.view.target[:] = (+0.207716, +0.133955, -0.153469)
    camera.view.up[:] = (-0.209938, +0.941078, +0.265137)
    camera.projection.fov_y = 0.700000
    camera.projection.near_clip = 0.020000
    camera.projection.far_clip = 100.000000
    camera.projection.ortho_height = 2.000000
    return camera


def _set_edl(panel) -> None:
    desc = dvz.dvz_edl_desc()
    desc.radius = 1.8
    desc.strength = 34.0
    desc.depth_scale = 1.0
    if dvz.dvz_panel_set_edl(panel, ctypes.byref(desc)) != 0:
        raise RuntimeError("dvz_panel_set_edl() failed")


def _add_pixels(scene, panel, data: PointCloudData):
    pixel = dvz.dvz_pixel(scene, 0)
    if not pixel:
        raise RuntimeError("dvz_pixel() failed")
    if dvz.dvz_visual_set_data_many(
        pixel,
        {
            "position": data.positions,
            "color": data.colors,
            "pixel_size_px": data.pixel_sizes,
        },
    ) != 0:
        raise RuntimeError("dvz_visual_set_data_many(point cloud) failed")
    if dvz.dvz_visual_set_depth_test(pixel, True) != 0:
        raise RuntimeError("dvz_visual_set_depth_test() failed")
    ex.add_visual(panel, pixel)
    return pixel


def _build_scene(path: Path = DATA_PATH):
    data = _load_point_cloud(path)
    scene, figure, panel = ex.scene_panel()
    dvz.dvz_panel_set_background_color(panel, PANEL_BG)
    camera = _camera_desc()
    if dvz.dvz_panel_set_camera_desc(panel, ctypes.byref(camera)) != 0:
        raise RuntimeError("dvz_panel_set_camera_desc() failed")
    _set_edl(panel)
    pixel = _add_pixels(scene, panel, data)
    return scene, figure, panel, camera, pixel, data.positions.shape[0]


def _configure_view(view, panel, camera) -> None:
    desc = dvz.dvz_fly_desc()
    desc.mode = dvz.DVZ_FLY_MODE_PLANE
    desc.controller_flags = int(dvz.DVZ_FLY_FLAGS_FIXED_UP) | int(dvz.DVZ_FLY_FLAGS_DISABLE_ROLL)
    desc.initial_view = camera.view
    desc.initial_view.up[:] = (0.0, 1.0, 0.0)
    fly = dvz.dvz_view_fly(view, panel, ctypes.byref(desc))
    if not fly:
        raise RuntimeError("dvz_view_fly() failed")


def main() -> None:
    scene, figure, panel, camera, _pixel, count = _build_scene()
    print(f"point_cloud: {count} points (prepared real data)")

    def configure(view) -> None:
        _configure_view(view, panel, camera)

    ex.run_with_view(scene, figure, "Point Cloud", configure)


if __name__ == "__main__":
    main()
