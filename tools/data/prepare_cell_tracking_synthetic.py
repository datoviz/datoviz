#!/usr/bin/env python3
"""Prepare deterministic synthetic cell-tracking data for napari examples."""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import zipfile
from pathlib import Path

import numpy as np


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_OUTPUT = REPO_ROOT / "data" / "examples" / "napari" / "cell_tracking_synthetic"


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


def _palette(count: int) -> np.ndarray:
    ids = np.arange(count + 1, dtype=np.uint32)
    x = ids * np.uint32(747796405) + np.uint32(2891336453)
    x ^= x >> np.uint32(16)
    colors = np.empty((count + 1, 4), dtype=np.uint8)
    colors[:, 0] = (55 + (x & 0x9F)).astype(np.uint8)
    colors[:, 1] = (70 + ((x >> np.uint32(8)) & 0x9F)).astype(np.uint8)
    colors[:, 2] = (85 + ((x >> np.uint32(16)) & 0x9F)).astype(np.uint8)
    colors[:, 3] = 190
    colors[0] = (0, 0, 0, 0)
    return colors


def _draw_disk(
    image: np.ndarray,
    labels: np.ndarray,
    track_id: int,
    y: float,
    x: float,
    radius: float,
    intensity: float,
) -> None:
    h, w = image.shape
    y0 = max(0, int(y - radius - 2))
    y1 = min(h, int(y + radius + 3))
    x0 = max(0, int(x - radius - 2))
    x1 = min(w, int(x + radius + 3))
    yy, xx = np.indices((y1 - y0, x1 - x0), dtype=np.float32)
    yy += y0
    xx += x0
    dist = ((yy - y) ** 2 + (xx - x) ** 2) / max(radius * radius, 1e-6)
    disk = dist <= 1.0
    glow = np.exp(-2.8 * dist)
    image[y0:y1, x0:x1] += intensity * glow
    target = labels[y0:y1, x0:x1]
    target[disk & (target == 0)] = np.uint32(track_id)


def prepare(output: Path, frames: int, tracks: int, size: int) -> None:
    output.mkdir(parents=True, exist_ok=True)
    rng = np.random.default_rng(424242)
    images = np.zeros((frames, size, size), dtype=np.float32)
    labels = np.zeros((frames, size, size), dtype=np.uint32)
    track_rows: list[tuple[float, float, float, float]] = []
    vector_rows: list[tuple[float, float, float, float]] = []
    edge_rows: list[tuple[int, int]] = []

    yy, xx = np.indices((size, size), dtype=np.float32)
    base = 18.0 + 5.0 * np.sin(xx / 23.0) + 4.0 * np.cos(yy / 29.0)
    starts = rng.uniform(24.0, size - 25.0, size=(tracks, 2)).astype(np.float32)
    velocities = rng.normal(0.0, 1.35, size=(tracks, 2)).astype(np.float32)
    phases = rng.uniform(0.0, 2.0 * np.pi, size=tracks).astype(np.float32)
    radii = rng.uniform(4.2, 7.5, size=tracks).astype(np.float32)

    for t in range(frames):
        images[t] = base + 2.0 * np.sin((xx + t * 3.0) / 17.0)
        for i in range(tracks):
            track_id = i + 1
            wobble = np.array(
                [
                    5.0 * np.sin(0.30 * t + phases[i]),
                    4.0 * np.cos(0.24 * t + phases[i] * 0.7),
                ],
                dtype=np.float32,
            )
            pos = starts[i] + velocities[i] * t + wobble
            pos = np.clip(pos, 12.0, size - 13.0)
            radius = float(radii[i] * (1.0 + 0.08 * np.sin(0.4 * t + phases[i])))
            intensity = float(75.0 + 85.0 * ((i % 5) / 4.0))
            _draw_disk(images[t], labels[t], track_id, float(pos[0]), float(pos[1]), radius, intensity)
            track_rows.append((float(track_id), float(t), float(pos[0]), float(pos[1])))

            if t > 0:
                prev = track_rows[-tracks - 1]
                vector_rows.append((prev[2], prev[3], float(pos[0] - prev[2]), float(pos[1] - prev[3])))
                edge_rows.append(((track_id - 1) * frames + t - 1, (track_id - 1) * frames + t))

    images += rng.normal(0.0, 3.0, size=images.shape)
    images_u8 = np.clip(images, 0.0, 255.0).astype(np.uint8)
    tracks_table = np.asarray(track_rows, dtype=np.float32)
    vectors = np.asarray(vector_rows, dtype=np.float32)
    graph_edges = np.asarray(edge_rows, dtype=np.int64)
    bundle = output / "cell_tracking_synthetic.npz"
    _savez_compressed_deterministic(
        bundle,
        images=images_u8,
        labels=labels,
        tracks=tracks_table,
        vectors=vectors,
        graph_edges=graph_edges,
        palette=_palette(tracks),
    )

    metadata = {
        "name": "cell_tracking_synthetic",
        "bundle": bundle.name,
        "frame_count": int(frames),
        "image_shape_yx": [int(size), int(size)],
        "track_count": int(tracks),
        "tracks_columns": ["track_id", "t", "y", "x"],
        "vectors_columns": ["origin_y", "origin_x", "direction_y", "direction_x"],
        "provenance": {
            "kind": "synthetic_fallback",
            "source": "deterministic CTC-like moving-cell fixture",
            "seed": 424242,
        },
        "sha256": {bundle.name: _sha256(bundle)},
    }
    (output / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--frames", type=int, default=32)
    parser.add_argument("--tracks", type=int, default=36)
    parser.add_argument("--size", type=int, default=256)
    args = parser.parse_args()
    if args.frames < 2 or args.tracks < 1 or args.size < 64:
        raise SystemExit("--frames must be >= 2, --tracks positive, and --size >= 64")
    prepare(args.output, args.frames, args.tracks, args.size)
    print(f"wrote synthetic cell-tracking bundle to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
