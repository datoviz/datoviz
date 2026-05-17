#!/usr/bin/env python3
"""Prepare the repository LIDAR point cloud as a Datoviz example-data bundle."""

from __future__ import annotations

import argparse
import shutil
from pathlib import Path

import numpy as np

from common import artifact, command_argv, ensure_bundle, relpath, write_manifest, write_provenance


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_SOURCE = ROOT / "data" / "misc" / "lidar.npz"
EXAMPLE_ID = "lidar"


def prepare(source: Path, force: bool) -> None:
    """Prepare the LIDAR point cloud bundle.

    @param source source NPZ path
    @param force whether to replace an existing output bundle
    """
    source = source.resolve()
    bundle_root, prepared = ensure_bundle(EXAMPLE_ID)
    if force and bundle_root.exists():
        shutil.rmtree(bundle_root)
        bundle_root, prepared = ensure_bundle(EXAMPLE_ID)
    if not source.exists():
        raise FileNotFoundError(source)

    with np.load(source) as data:
        if "pos" not in data or "color" not in data:
            raise ValueError(f"{source} must contain 'pos' and 'color' arrays")
        pos = np.ascontiguousarray(data["pos"], dtype=np.float32)
        color = np.ascontiguousarray(data["color"], dtype=np.uint8)

    if pos.ndim != 2 or pos.shape[1] != 3:
        raise ValueError(f"expected pos shape (N, 3), got {pos.shape}")
    if color.ndim != 2 or color.shape[1] != 4:
        raise ValueError(f"expected color shape (N, 4), got {color.shape}")
    if pos.shape[0] != color.shape[0]:
        raise ValueError("position/color point counts do not match")

    pos_path = prepared / "lidar_pos.npy"
    color_path = prepared / "lidar_color.npy"
    np.save(pos_path, pos)
    np.save(color_path, color)

    artifacts = [
        artifact(
            pos_path,
            bundle_root,
            "position",
            "npy",
            dtype=str(pos.dtype),
            shape=list(pos.shape),
            coordinate_system="source",
        ),
        artifact(
            color_path,
            bundle_root,
            "color",
            "npy",
            dtype=str(color.dtype),
            shape=list(color.shape),
            color_space="rgba8",
        ),
    ]
    write_manifest(
        bundle_root,
        example_id=EXAMPLE_ID,
        title="Repository LIDAR Point Cloud",
        status="committed",
        script=relpath(Path(__file__), ROOT),
        command=command_argv(relpath(Path(__file__), ROOT)),
        source={
            "name": "Existing repository LIDAR NPZ",
            "path": relpath(source, ROOT),
            "format": "npz",
            "arrays": ["pos", "color"],
            "license": "Inherited from repository data provenance.",
        },
        artifacts=artifacts,
        validation={
            "point_count": int(pos.shape[0]),
            "bounds": {
                "min": pos.min(axis=0).astype(float).tolist(),
                "max": pos.max(axis=0).astype(float).tolist(),
            },
        },
    )
    write_provenance(
        bundle_root,
        title="Repository LIDAR Point Cloud",
        source_lines=[
            f"Source file: `{relpath(source, ROOT)}`.",
            "Source arrays: `pos` as float32 positions and `color` as uint8 RGBA colors.",
        ],
        processing_lines=[
            "Loaded the source NPZ with NumPy.",
            "Wrote contiguous `.npy` arrays in `prepared/` without coordinate or color conversion.",
        ],
        license_lines=["Reuse follows the repository data provenance for `data/misc/lidar.npz`."],
    )
    print(f"wrote {relpath(bundle_root, ROOT)} ({pos.shape[0]} points)")


def main() -> None:
    """Run the LIDAR preparation command."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--force", action="store_true", help="replace an existing output bundle")
    args = parser.parse_args()
    prepare(args.source, args.force)


if __name__ == "__main__":
    main()
