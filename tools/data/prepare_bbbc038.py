#!/usr/bin/env python3
"""Prepare a compact BBBC038-like nuclei image and label bundle."""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import zipfile
from pathlib import Path

import numpy as np


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_OUTPUT = REPO_ROOT / "data" / "examples" / "napari" / "bbbc038"


def _sha256(path: Path) -> str:
    h = hashlib.sha256()
    h.update(path.read_bytes())
    return h.hexdigest()


def _savez_compressed_deterministic(path: Path, **arrays: np.ndarray) -> None:
    with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, array in arrays.items():
            buffer = io.BytesIO()
            np.save(buffer, np.asarray(array), allow_pickle=False)
            info = zipfile.ZipInfo(f"{name}.npy", date_time=(1980, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            archive.writestr(info, buffer.getvalue())


def _make_palette(count: int) -> np.ndarray:
    ids = np.arange(count + 1, dtype=np.uint32)
    x = ids + np.uint32(0x9E3779B9)
    x ^= x >> np.uint32(16)
    x *= np.uint32(0x7FEB352D)
    x ^= x >> np.uint32(15)
    colors = np.empty((count + 1, 4), dtype=np.uint8)
    colors[:, 0] = (50 + (x & 0xAF)).astype(np.uint8)
    colors[:, 1] = (65 + ((x >> np.uint32(8)) & 0xAF)).astype(np.uint8)
    colors[:, 2] = (80 + ((x >> np.uint32(16)) & 0xAF)).astype(np.uint8)
    colors[:, 3] = 180
    colors[0] = (0, 0, 0, 0)
    return colors


def _synthetic_nuclei(size: int, count: int) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    rng = np.random.default_rng(38038)
    yy, xx = np.indices((size, size), dtype=np.float32)
    image = 18.0 + 10.0 * np.sin(xx / 27.0) + 8.0 * np.cos(yy / 31.0)
    labels = np.zeros((size, size), dtype=np.uint32)
    centroids: list[tuple[float, float, float]] = []

    label_id = 1
    attempts = 0
    while label_id <= count and attempts < count * 8:
        attempts += 1
        cy = rng.uniform(18.0, size - 19.0)
        cx = rng.uniform(18.0, size - 19.0)
        ry = rng.uniform(5.0, 13.0)
        rx = rng.uniform(5.0, 14.0)
        angle = rng.uniform(0.0, np.pi)
        ca = np.cos(angle)
        sa = np.sin(angle)
        dx = xx - cx
        dy = yy - cy
        u = (dx * ca + dy * sa) / rx
        v = (-dx * sa + dy * ca) / ry
        nucleus = u * u + v * v <= 1.0
        overlap = np.count_nonzero(labels[nucleus])
        area = int(np.count_nonzero(nucleus))
        if area < 40 or overlap > area * 0.16:
            continue
        core = np.exp(-2.4 * (u * u + v * v))
        image += nucleus * rng.uniform(65.0, 140.0) * core
        image += (u * u + v * v <= 0.12) * rng.uniform(25.0, 70.0)
        labels[nucleus & (labels == 0)] = label_id
        centroids.append((float(label_id), cy, cx))
        label_id += 1

    image += rng.normal(0.0, 5.0, size=(size, size))
    image_u8 = np.clip(image, 0.0, 255.0).astype(np.uint8)
    return image_u8, labels, np.asarray(centroids, dtype=np.float32)


def prepare(output: Path, size: int, count: int) -> None:
    output.mkdir(parents=True, exist_ok=True)
    image, labels, centroids = _synthetic_nuclei(size, count)
    label_count = int(labels.max())
    palette = _make_palette(label_count)
    boundary = np.zeros_like(labels, dtype=np.uint8)
    boundary[1:, :] |= labels[1:, :] != labels[:-1, :]
    boundary[:, 1:] |= labels[:, 1:] != labels[:, :-1]

    bundle = output / "bbbc038_labels.npz"
    _savez_compressed_deterministic(
        bundle,
        image=image,
        labels=labels,
        centroids=centroids,
        palette=palette,
        boundary=boundary,
    )

    metadata = {
        "name": "bbbc038_labels",
        "bundle": bundle.name,
        "image_shape_yx": [int(v) for v in image.shape],
        "label_count": label_count,
        "centroid_columns": ["label_id", "y", "x"],
        "provenance": {
            "kind": "synthetic_fallback",
            "source": "deterministic BBBC038/Kaggle-DSB nuclei-like fixture",
            "seed": 38038,
            "note": "No external BBBC038 download was performed for this compact repo bundle.",
            "reference": "https://bbbc.broadinstitute.org/BBBC038",
        },
        "sha256": {bundle.name: _sha256(bundle)},
    }
    (output / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--size", type=int, default=512)
    parser.add_argument("--count", type=int, default=180)
    args = parser.parse_args()
    if args.size < 64 or args.count < 1:
        raise SystemExit("--size must be >= 64 and --count must be positive")
    prepare(args.output, args.size, args.count)
    print(f"wrote BBBC038-style bundle to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
