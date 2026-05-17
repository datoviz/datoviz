#!/usr/bin/env python3
"""Prepare dense spatial-points caches for the Datoviz napari demo."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

import numpy as np


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_ZARR_ROOT = REPO_ROOT / ".cache" / "datoviz-napari-demos" / "spatialdata" / "data.zarr"


def _hash_u32(values: np.ndarray) -> np.ndarray:
    values = values.astype(np.uint32, copy=False)
    values ^= values >> np.uint32(16)
    values *= np.uint32(0x7FEB352D)
    values ^= values >> np.uint32(15)
    values *= np.uint32(0x846CA68B)
    values ^= values >> np.uint32(16)
    return values


def _categorical_colors(category: np.ndarray, category_count: int) -> np.ndarray:
    h = _hash_u32(category + np.uint32(17))
    colors = np.empty((category.size, 4), dtype=np.uint8)
    colors[:, 0] = 55 + (h & 0x9F).astype(np.uint8)
    colors[:, 1] = 70 + ((h >> np.uint32(8)) & 0x9F).astype(np.uint8)
    colors[:, 2] = 85 + ((h >> np.uint32(16)) & 0x9F).astype(np.uint8)
    colors[:, 3] = 220
    if category_count > 0:
        colors[category == 0, 3] = 170
    return colors


def _continuous_colors(value: np.ndarray) -> np.ndarray:
    t = np.clip(value, 0.0, 1.0).astype(np.float32, copy=False)
    colors = np.empty((value.size, 4), dtype=np.uint8)
    colors[:, 0] = np.clip(255.0 * np.minimum(1.0, 1.7 * t), 0, 255).astype(np.uint8)
    colors[:, 1] = np.clip(255.0 * (1.0 - np.abs(t - 0.55) * 1.7), 0, 255).astype(np.uint8)
    colors[:, 2] = np.clip(255.0 * np.minimum(1.0, 1.8 * (1.0 - t)), 0, 255).astype(np.uint8)
    colors[:, 3] = 215
    return colors


def _density_colors(value: np.ndarray) -> np.ndarray:
    colors = np.empty((value.size, 4), dtype=np.uint8)
    colors[:, 0] = 245
    colors[:, 1] = 248
    colors[:, 2] = 255
    colors[:, 3] = np.clip(16.0 + 30.0 * value, 12, 54).astype(np.uint8)
    return colors


def _normalize_points(points: np.ndarray) -> np.ndarray:
    out = np.zeros((points.shape[0], 3), dtype=np.float32)
    lo = np.nanmin(points, axis=0)
    hi = np.nanmax(points, axis=0)
    span = np.maximum(hi - lo, 1e-6)
    out[:, :2] = -0.92 + 1.84 * ((points - lo) / span)
    return out


def _rgba_from_image(image: np.ndarray) -> np.ndarray:
    array = np.asarray(image)
    if array.ndim == 3 and array.shape[0] in (1, 3, 4):
        array = np.moveaxis(array, 0, -1)
    if array.ndim == 3 and array.shape[-1] == 1:
        array = array[:, :, 0]

    if array.ndim == 2:
        raw = array.astype(np.float32, copy=False)
        lo = float(np.nanmin(raw))
        hi = float(np.nanmax(raw))
        gray = np.clip(255.0 * (raw - lo) / max(hi - lo, 1e-6), 0, 255).astype(np.uint8)
        rgba = np.empty((*gray.shape, 4), dtype=np.uint8)
        rgba[:, :, 0] = gray
        rgba[:, :, 1] = gray
        rgba[:, :, 2] = gray
        rgba[:, :, 3] = 255
        return rgba

    if array.ndim == 3 and array.shape[-1] in (3, 4):
        raw = array.astype(np.float32, copy=False)
        lo = float(np.nanmin(raw))
        hi = float(np.nanmax(raw))
        rgb = np.clip(255.0 * (raw - lo) / max(hi - lo, 1e-6), 0, 255).astype(np.uint8)
        rgba = np.empty((array.shape[0], array.shape[1], 4), dtype=np.uint8)
        rgba[:, :, : min(3, rgb.shape[-1])] = rgb[:, :, : min(3, rgb.shape[-1])]
        rgba[:, :, 3] = 255 if rgb.shape[-1] == 3 else rgb[:, :, 3]
        return rgba

    raise RuntimeError(f"unsupported image shape {array.shape}")


def _load_spatialdata_zarr(path: Path) -> tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    import spatialdata as sd

    sdata = sd.read_zarr(path)

    if getattr(sdata, "points", {}):
        name, points_df = next(iter(sdata.points.items()))
        print(f"using SpatialData points element {name!r}")
        df = points_df.compute() if hasattr(points_df, "compute") else points_df
        points = np.column_stack([np.asarray(df["x"]), np.asarray(df["y"])]).astype(np.float32)
        if "cell_type" in df:
            _, inverse = np.unique(np.asarray(df["cell_type"]).astype(str), return_inverse=True)
            category = inverse.astype(np.uint32)
        else:
            category = np.zeros(points.shape[0], dtype=np.uint32)
        value = ((_hash_u32(np.arange(points.shape[0], dtype=np.uint32) + np.uint32(101)) & 0xFFFF) / 65535.0).astype(
            np.float32
        )
    elif getattr(sdata, "shapes", {}):
        name, shapes = next(iter(sdata.shapes.items()))
        print(f"using SpatialData shapes element {name!r}")
        centroids = shapes.geometry.centroid
        points = np.column_stack([centroids.x.to_numpy(), centroids.y.to_numpy()]).astype(np.float32)
        category = np.zeros(points.shape[0], dtype=np.uint32)
        value = np.zeros(points.shape[0], dtype=np.float32)
    else:
        raise RuntimeError(f"no points or shapes found in SpatialData Zarr {path}")

    if getattr(sdata, "images", {}):
        image_name, image_data = next(iter(sdata.images.items()))
        print(f"using SpatialData image element {image_name!r}")
        data = image_data.data
        data = data.compute() if hasattr(data, "compute") else data
        image = _rgba_from_image(data)
    else:
        raise RuntimeError(f"no image found in SpatialData Zarr {path}")

    return _normalize_points(points), category, value, image


def _write_cache(
    output: Path,
    dataset: str,
    positions: np.ndarray,
    category: np.ndarray,
    value: np.ndarray,
    image: np.ndarray,
) -> None:
    output.mkdir(parents=True, exist_ok=True)
    category_count = int(category.max()) + 1 if category.size else 0

    colors_category = _categorical_colors(category, category_count)
    colors_continuous = _continuous_colors(value)
    colors_density = _density_colors(value)

    positions.astype("<f4", copy=False).tofile(output / "positions_f32.bin")
    category.astype("<u4", copy=False).tofile(output / "category_u32.bin")
    value.astype("<f4", copy=False).tofile(output / "value_f32.bin")
    colors_category.tofile(output / "colors_category_rgba8.bin")
    colors_continuous.tofile(output / "colors_continuous_rgba8.bin")
    colors_density.tofile(output / "colors_density_rgba8.bin")
    image.tofile(output / "image_rgba8.bin")

    lod_targets = (
        ("real", positions.shape[0]),
        ("1m", 1_000_000),
        ("5m", 5_000_000),
        ("10m", 10_000_000),
    )
    for name, target in lod_targets:
        count = min(int(positions.shape[0]), target)
        np.arange(count, dtype="<u4").tofile(output / f"lod_{name}_u32.bin")

    metadata = {
        "dataset": dataset,
        "point_count": int(positions.shape[0]),
        "image_width": int(image.shape[1]),
        "image_height": int(image.shape[0]),
        "x_min": -0.92,
        "x_max": 0.92,
        "y_min": -0.92,
        "y_max": 0.92,
        "category_count": category_count,
        "category_names": [f"category_{i}" for i in range(category_count)],
    }
    (output / "metadata.json").write_text(json.dumps(metadata, indent=2), encoding="utf8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dataset", default="merfish", choices=("merfish", "mibitof"))
    parser.add_argument("--output", type=Path)
    parser.add_argument("--zarr", type=Path, default=DEFAULT_ZARR_ROOT)
    args = parser.parse_args()

    output = args.output
    if output is None:
        output = REPO_ROOT / ".cache" / "datoviz-napari-demos" / "spatial_points" / args.dataset

    positions, category, value, image = _load_spatialdata_zarr(args.zarr)

    _write_cache(output, args.dataset, positions, category, value, image)
    print(f"wrote {positions.shape[0]} points to {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
