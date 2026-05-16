#!/usr/bin/env python3
"""Prepare local LIDAR .npy arrays for the C showcase example."""

from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Convert data/misc/lidar.npz into local .npy files for showcase_lidar_glfw."
    )
    parser.add_argument(
        "--input",
        type=Path,
        default=Path("data/misc/lidar.npz"),
        help="input .npz file containing pos and color arrays",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("build/local_data/lidar"),
        help="directory where lidar_pos.npy and lidar_color.npy will be written",
    )
    args = parser.parse_args()

    data = np.load(args.input)
    pos = np.ascontiguousarray(data["pos"], dtype=np.float32)
    color = np.ascontiguousarray(data["color"], dtype=np.uint8)

    if pos.ndim != 2 or pos.shape[1] != 3:
        raise ValueError(f"expected pos shape (N, 3), got {pos.shape}")
    if color.ndim != 2 or color.shape[1] != 4:
        raise ValueError(f"expected color shape (N, 4), got {color.shape}")
    if len(pos) != len(color):
        raise ValueError(f"pos/color length mismatch: {len(pos)} != {len(color)}")

    args.output_dir.mkdir(parents=True, exist_ok=True)
    pos_path = args.output_dir / "lidar_pos.npy"
    color_path = args.output_dir / "lidar_color.npy"
    np.save(pos_path, pos)
    np.save(color_path, color)

    print(f"wrote {pos_path} ({pos.shape}, {pos.dtype})")
    print(f"wrote {color_path} ({color.shape}, {color.dtype})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
