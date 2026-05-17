#!/usr/bin/env python3
"""Prepare a compact cells3d-style volume bundle for napari examples."""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import zipfile
from pathlib import Path

import numpy as np


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_OUTPUT = REPO_ROOT / "data" / "examples" / "napari" / "cells3d"
DEFAULT_SHAPE = (64, 128, 128)


def _as_u8(array: np.ndarray) -> np.ndarray:
    raw = np.asarray(array, dtype=np.float32)
    lo = float(np.nanpercentile(raw, 0.5))
    hi = float(np.nanpercentile(raw, 99.5))
    if not np.isfinite(lo) or not np.isfinite(hi) or hi <= lo:
        lo = float(np.nanmin(raw))
        hi = float(np.nanmax(raw))
    scaled = np.clip((raw - lo) / max(hi - lo, 1e-6), 0.0, 1.0)
    return np.ascontiguousarray(np.round(255.0 * scaled), dtype=np.uint8)


def _center_crop_or_stride(volume: np.ndarray, shape: tuple[int, int, int]) -> np.ndarray:
    out = volume
    for axis, target in enumerate(shape):
        if out.shape[axis] > target:
            step = max(1, out.shape[axis] // target)
            indices = np.linspace(0, out.shape[axis] - 1, target, dtype=np.int64)
            if step > 1:
                out = np.take(out, indices, axis=axis)
            else:
                start = (out.shape[axis] - target) // 2
                out = np.take(out, np.arange(start, start + target), axis=axis)
        elif out.shape[axis] < target:
            pad = target - out.shape[axis]
            before = pad // 2
            after = pad - before
            pads = [(0, 0), (0, 0), (0, 0)]
            pads[axis] = (before, after)
            out = np.pad(out, pads, mode="edge")
    return np.ascontiguousarray(out)


def _load_skimage_cells3d(shape: tuple[int, int, int]) -> tuple[np.ndarray, str] | None:
    try:
        from skimage import data
    except Exception:
        return None

    try:
        cells = np.asarray(data.cells3d())
    except Exception:
        return None

    source_shape = tuple(int(v) for v in cells.shape)
    if cells.ndim == 4 and cells.shape[1] >= 2:
        nuclei = cells[:, 1, :, :]
    elif cells.ndim == 4:
        nuclei = cells[:, 0, :, :]
    elif cells.ndim == 3:
        nuclei = cells
    else:
        return None

    volume = _center_crop_or_stride(_as_u8(nuclei), shape)
    return volume, f"skimage.data.cells3d source_shape={source_shape}"


def _synthetic_cells3d(shape: tuple[int, int, int]) -> np.ndarray:
    rng = np.random.default_rng(20260517)
    z, y, x = np.indices(shape, dtype=np.float32)
    volume = np.zeros(shape, dtype=np.float32)
    volume += 18.0 + 8.0 * np.sin(x / 13.0) + 7.0 * np.cos((y + z) / 17.0)

    for _ in range(120):
        cz = rng.uniform(4.0, shape[0] - 5.0)
        cy = rng.uniform(8.0, shape[1] - 9.0)
        cx = rng.uniform(8.0, shape[2] - 9.0)
        rz = rng.uniform(1.8, 5.0)
        ry = rng.uniform(3.0, 9.0)
        rx = rng.uniform(3.0, 9.0)
        amp = rng.uniform(80.0, 210.0)
        dist = ((z - cz) / rz) ** 2 + ((y - cy) / ry) ** 2 + ((x - cx) / rx) ** 2
        volume += amp * np.exp(-1.8 * dist)

    volume += rng.normal(0.0, 4.0, size=shape)
    return _as_u8(volume)


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


def prepare(output: Path, shape: tuple[int, int, int]) -> None:
    output.mkdir(parents=True, exist_ok=True)
    loaded = _load_skimage_cells3d(shape)
    if loaded is None:
        volume = _synthetic_cells3d(shape)
        provenance = {
            "kind": "synthetic_fallback",
            "source": "deterministic ellipsoid microscopy-like volume",
            "seed": 20260517,
        }
    else:
        volume, source = loaded
        provenance = {
            "kind": "skimage_cells3d",
            "source": source,
            "note": "Loaded through scikit-image when available; no Datoviz network fetch.",
        }

    z, y, x = np.indices(volume.shape, dtype=np.float32)
    threshold = np.percentile(volume, 97.0)
    mask = volume >= threshold
    if np.count_nonzero(mask) > 0:
        points = np.column_stack([x[mask], y[mask], z[mask]]).astype(np.float32)
        values = volume[mask].astype(np.float32) / 255.0
        if points.shape[0] > 8192:
            idx = np.linspace(0, points.shape[0] - 1, 8192, dtype=np.int64)
            points = points[idx]
            values = values[idx]
    else:
        points = np.zeros((0, 3), dtype=np.float32)
        values = np.zeros((0,), dtype=np.float32)

    bundle = output / "cells3d_volume.npz"
    _savez_compressed_deterministic(
        bundle,
        volume=volume,
        points=points,
        point_value=values.astype(np.float32),
        voxel_size=np.asarray([1.0, 1.0, 1.0], dtype=np.float32),
    )

    metadata = {
        "name": "cells3d",
        "bundle": bundle.name,
        "shape_zyx": [int(v) for v in volume.shape],
        "dtype": str(volume.dtype),
        "point_count": int(points.shape[0]),
        "intensity_min": int(volume.min()),
        "intensity_max": int(volume.max()),
        "provenance": provenance,
        "sha256": {bundle.name: _sha256(bundle)},
    }
    (output / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--shape", default="64,128,128", help="output Z,Y,X shape")
    args = parser.parse_args()

    shape = tuple(int(part) for part in args.shape.split(","))
    if len(shape) != 3 or min(shape) <= 0:
        raise SystemExit("--shape must contain three positive integers")
    prepare(args.output, shape)  # type: ignore[arg-type]
    print(f"wrote cells3d bundle to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
