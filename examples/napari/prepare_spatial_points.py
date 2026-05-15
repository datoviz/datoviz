#!/usr/bin/env python3
"""Prepare dense spatial-points caches for the Datoviz napari demo.

The default path generates a deterministic synthetic spatial-omics-like point cloud using only
NumPy. Optional SpatialData extraction is intentionally best-effort so the render-time C demo does
not depend on the SpatialData Python stack.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

import numpy as np


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_CACHE = REPO_ROOT / ".cache" / "datoviz-napari-demos" / "spatial_points" / "synthetic"


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


def _synthetic_points(count: int, category_count: int, seed: int) -> tuple[np.ndarray, ...]:
    rng = np.random.default_rng(seed)
    cluster_count = max(64, min(512, count // 4096))
    centers = rng.uniform(-0.82, 0.82, size=(cluster_count, 2)).astype(np.float32)
    cluster_category = rng.integers(0, category_count, size=cluster_count, dtype=np.uint32)

    idx = np.arange(count, dtype=np.uint32)
    cluster = _hash_u32(idx + np.uint32(seed)) % np.uint32(cluster_count)
    jitter_x = ((_hash_u32(idx * np.uint32(3) + np.uint32(11)) & 0xFFFF) / 65535.0) - 0.5
    jitter_y = ((_hash_u32(idx * np.uint32(5) + np.uint32(23)) & 0xFFFF) / 65535.0) - 0.5
    radius = 0.018 + 0.055 * ((cluster % np.uint32(17)).astype(np.float32) / 16.0)

    positions = np.zeros((count, 3), dtype=np.float32)
    positions[:, 0] = centers[cluster, 0] + jitter_x.astype(np.float32) * radius
    positions[:, 1] = centers[cluster, 1] + jitter_y.astype(np.float32) * radius
    positions[:, 0:2] = np.clip(positions[:, 0:2], -0.92, 0.92)

    category = cluster_category[cluster]
    value = ((_hash_u32(idx + np.uint32(101)) & 0xFFFF) / 65535.0).astype(np.float32)
    return positions, category, value


def _synthetic_image(width: int, height: int, seed: int) -> np.ndarray:
    y, x = np.mgrid[0:height, 0:width].astype(np.float32)
    nx = (x / max(1, width - 1)) - 0.5
    ny = (y / max(1, height - 1)) - 0.5
    vignette = np.clip(1.0 - 1.65 * (np.abs(nx) + np.abs(ny)), 0.0, 1.0)
    texture = ((_hash_u32((x.astype(np.uint32) * 1973) ^ (y.astype(np.uint32) * 9277)) & 63) / 63.0)
    base = np.clip(34.0 + 150.0 * vignette + 22.0 * texture, 0, 255)
    image = np.empty((height, width, 4), dtype=np.uint8)
    image[:, :, 0] = np.clip(base * 0.82, 0, 255).astype(np.uint8)
    image[:, :, 1] = np.clip(base * 0.92, 0, 255).astype(np.uint8)
    image[:, :, 2] = np.clip(base * 1.08, 0, 255).astype(np.uint8)
    image[:, :, 3] = 255
    return image


def _load_spatialdata(dataset: str) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    try:
        import spatialdata.datasets as datasets
    except ImportError as exc:
        raise RuntimeError(
            "SpatialData extraction requires the optional 'spatialdata' package. "
            "Use --dataset synthetic to generate a cache with only NumPy."
        ) from exc

    helper = getattr(datasets, dataset, None)
    if helper is None:
        raise RuntimeError(f"SpatialData helper '{dataset}' was not found")
    sdata = helper()

    tables = getattr(sdata, "tables", {})
    for table in tables.values():
        obs = getattr(table, "obs", None)
        obsm = getattr(table, "obsm", {})
        coords = obsm.get("spatial") if hasattr(obsm, "get") else None
        if coords is None and obs is not None:
            columns = getattr(obs, "columns", [])
            if "x" in columns and "y" in columns:
                coords = np.column_stack([np.asarray(obs["x"]), np.asarray(obs["y"])])
        if coords is None:
            continue

        points = np.asarray(coords, dtype=np.float32)
        if points.ndim != 2 or points.shape[1] < 2:
            continue
        points = points[:, :2]

        category = np.zeros(points.shape[0], dtype=np.uint32)
        value = np.zeros(points.shape[0], dtype=np.float32)
        if obs is not None:
            for name in ("cell_type", "cluster", "annotation", "gene"):
                if name in getattr(obs, "columns", []):
                    _, inverse = np.unique(np.asarray(obs[name]).astype(str), return_inverse=True)
                    category = inverse.astype(np.uint32)
                    break
            for name in ("intensity", "expression", "score", "quality", "area"):
                if name in getattr(obs, "columns", []):
                    raw = np.asarray(obs[name], dtype=np.float32)
                    lo = float(np.nanmin(raw))
                    hi = float(np.nanmax(raw))
                    value = (raw - lo) / max(hi - lo, 1e-6)
                    break

        out = np.zeros((points.shape[0], 3), dtype=np.float32)
        lo = np.nanmin(points, axis=0)
        hi = np.nanmax(points, axis=0)
        span = np.maximum(hi - lo, 1e-6)
        out[:, :2] = -0.92 + 1.84 * ((points - lo) / span)
        return out, category, value

    raise RuntimeError(f"no usable spatial coordinate table found in SpatialData dataset {dataset!r}")


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
    parser.add_argument("--dataset", default="synthetic", choices=("synthetic", "merfish", "mibitof"))
    parser.add_argument("--output", type=Path, default=DEFAULT_CACHE)
    parser.add_argument("--count", type=int, default=1_000_000)
    parser.add_argument("--categories", type=int, default=16)
    parser.add_argument("--image-size", type=int, default=1024)
    parser.add_argument("--seed", type=int, default=42)
    args = parser.parse_args()

    if args.dataset == "synthetic":
        positions, category, value = _synthetic_points(args.count, args.categories, args.seed)
    else:
        positions, category, value = _load_spatialdata(args.dataset)

    image = _synthetic_image(args.image_size, args.image_size, args.seed)
    _write_cache(args.output, args.dataset, positions, category, value, image)
    print(f"wrote {positions.shape[0]} points to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
