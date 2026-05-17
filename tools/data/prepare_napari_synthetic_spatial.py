#!/usr/bin/env python3
"""Prepare deterministic dense spatial-points data for napari examples."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

import numpy as np


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_OUTPUT = REPO_ROOT / "data" / "examples" / "napari" / "spatial_points" / "synthetic"


def _sha256(path: Path) -> str:
    h = hashlib.sha256()
    h.update(path.read_bytes())
    return h.hexdigest()


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
    colors[:, 3] = 218
    if category_count > 0:
        colors[category == 0, 3] = 150
    return colors


def _continuous_colors(value: np.ndarray) -> np.ndarray:
    t = np.clip(value, 0.0, 1.0).astype(np.float32, copy=False)
    colors = np.empty((value.size, 4), dtype=np.uint8)
    colors[:, 0] = np.clip(255.0 * np.minimum(1.0, 1.7 * t), 0, 255).astype(np.uint8)
    colors[:, 1] = np.clip(255.0 * (1.0 - np.abs(t - 0.55) * 1.65), 0, 255).astype(np.uint8)
    colors[:, 2] = np.clip(255.0 * np.minimum(1.0, 1.8 * (1.0 - t)), 0, 255).astype(np.uint8)
    colors[:, 3] = 215
    return colors


def _density_colors(value: np.ndarray) -> np.ndarray:
    colors = np.empty((value.size, 4), dtype=np.uint8)
    colors[:, 0] = 245
    colors[:, 1] = 248
    colors[:, 2] = 255
    colors[:, 3] = np.clip(14.0 + 36.0 * value, 12, 58).astype(np.uint8)
    return colors


def _background(width: int, height: int) -> np.ndarray:
    y, x = np.indices((height, width), dtype=np.float32)
    cx = width * 0.52
    cy = height * 0.48
    r = ((x - cx) / (0.48 * width)) ** 2 + ((y - cy) / (0.40 * height)) ** 2
    tissue = np.clip(1.0 - r, 0.0, 1.0)
    texture = 0.12 * np.sin(x / 19.0) + 0.10 * np.cos(y / 23.0)
    gray = np.clip(28.0 + 145.0 * tissue + 35.0 * texture, 0.0, 255.0).astype(np.uint8)
    image = np.empty((height, width, 4), dtype=np.uint8)
    image[:, :, 0] = gray
    image[:, :, 1] = np.clip(gray.astype(np.int16) + 8, 0, 255).astype(np.uint8)
    image[:, :, 2] = np.clip(gray.astype(np.int16) + 18, 0, 255).astype(np.uint8)
    image[:, :, 3] = 255
    return image


def _generate_points(count: int, category_count: int) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    rng = np.random.default_rng(17052026)
    centers = np.array(
        [
            [-0.50, -0.25],
            [-0.12, -0.44],
            [0.24, -0.32],
            [0.49, 0.04],
            [0.18, 0.36],
            [-0.28, 0.31],
            [-0.58, 0.11],
            [0.02, 0.02],
        ],
        dtype=np.float32,
    )
    weights = np.array([0.11, 0.10, 0.12, 0.15, 0.13, 0.14, 0.10, 0.15], dtype=np.float32)
    weights /= weights.sum()
    cluster = rng.choice(centers.shape[0], size=count, p=weights)
    jitter = rng.normal(0.0, 0.085, size=(count, 2)).astype(np.float32)
    points2 = centers[cluster] + jitter
    points2[:, 0] += 0.10 * np.sin(points2[:, 1] * 9.0)
    points2[:, 1] += 0.07 * np.cos(points2[:, 0] * 11.0)
    points2 = np.clip(points2, -0.92, 0.92)

    z = (0.06 * np.sin(points2[:, 0] * 8.0) + 0.04 * np.cos(points2[:, 1] * 7.0)).astype(np.float32)
    positions = np.column_stack([points2, z]).astype(np.float32)
    category = ((cluster + rng.integers(0, 3, size=count)) % category_count).astype(np.uint32)
    distance = np.linalg.norm(points2 - centers[cluster], axis=1)
    value = np.clip(1.0 - distance / 0.28 + 0.12 * rng.random(count), 0.0, 1.0).astype(np.float32)
    return positions, category, value


def prepare(output: Path, count: int, category_count: int, image_size: int) -> None:
    output.mkdir(parents=True, exist_ok=True)
    positions, category, value = _generate_points(count, category_count)
    image = _background(image_size, image_size)

    files: dict[str, Path] = {
        "positions": output / "positions_f32.bin",
        "category": output / "category_u32.bin",
        "value": output / "value_f32.bin",
        "colors_category": output / "colors_category_rgba8.bin",
        "colors_continuous": output / "colors_continuous_rgba8.bin",
        "colors_density": output / "colors_density_rgba8.bin",
        "image": output / "image_rgba8.bin",
    }
    positions.astype("<f4", copy=False).tofile(files["positions"])
    category.astype("<u4", copy=False).tofile(files["category"])
    value.astype("<f4", copy=False).tofile(files["value"])
    _categorical_colors(category, category_count).tofile(files["colors_category"])
    _continuous_colors(value).tofile(files["colors_continuous"])
    _density_colors(value).tofile(files["colors_density"])
    image.tofile(files["image"])

    for name, target in (("real", count), ("100k", 100_000), ("250k", 250_000), ("1m", 1_000_000)):
        lod = output / f"lod_{name}_u32.bin"
        np.arange(min(count, target), dtype="<u4").tofile(lod)
        files[f"lod_{name}"] = lod

    metadata = {
        "dataset": "synthetic_spatial_points",
        "point_count": int(count),
        "image_width": int(image_size),
        "image_height": int(image_size),
        "x_min": -0.92,
        "x_max": 0.92,
        "y_min": -0.92,
        "y_max": 0.92,
        "category_count": int(category_count),
        "category_names": [f"cell_type_{i:02d}" for i in range(category_count)],
        "provenance": {
            "kind": "synthetic_fallback",
            "source": "deterministic tissue-like clustered point cloud",
            "seed": 17052026,
        },
        "files": {key: path.name for key, path in files.items()},
        "sha256": {path.name: _sha256(path) for path in files.values()},
    }
    (output / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--count", type=int, default=200_000)
    parser.add_argument("--categories", type=int, default=12)
    parser.add_argument("--image-size", type=int, default=512)
    args = parser.parse_args()
    if args.count < 1 or args.categories < 1 or args.image_size < 16:
        raise SystemExit("--count, --categories, and --image-size must be positive")
    prepare(args.output, args.count, args.categories, args.image_size)
    print(f"wrote {args.count} synthetic spatial points to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
