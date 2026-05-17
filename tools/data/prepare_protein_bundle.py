#!/usr/bin/env python3
"""Prepare the repository protein point cloud as a Datoviz example-data bundle."""

from __future__ import annotations

import argparse
import shutil
from pathlib import Path

import numpy as np

from common import artifact, command_argv, ensure_bundle, relpath, write_manifest, write_provenance


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_SOURCE = ROOT / "data" / "misc" / "molecule" / "mol.npz"
DEFAULT_PDB_DIR = ROOT / "data" / "misc" / "molecule"
EXAMPLE_ID = "proteins/6qzp"


def _source_pdbs(pdb_dir: Path) -> list[Path]:
    return sorted(pdb_dir.glob("6qzp-pdb-bundle*.pdb"))


def prepare(source: Path, pdb_dir: Path, force: bool) -> None:
    """Prepare the protein point bundle.

    @param source source NPZ path
    @param pdb_dir directory containing source PDB bundle files
    @param force whether to replace an existing output bundle
    """
    source = source.resolve()
    pdb_dir = pdb_dir.resolve()
    bundle_root, prepared = ensure_bundle(EXAMPLE_ID)
    if force and bundle_root.exists():
        shutil.rmtree(bundle_root)
        bundle_root, prepared = ensure_bundle(EXAMPLE_ID)
    if not source.exists():
        raise FileNotFoundError(source)

    with np.load(source) as data:
        required = {"position", "color", "size"}
        missing = sorted(required.difference(data.files))
        if missing:
            raise ValueError(f"{source} is missing arrays: {missing}")
        position = np.ascontiguousarray(data["position"], dtype=np.float32)
        color = np.ascontiguousarray(data["color"], dtype=np.uint8)
        size = np.ascontiguousarray(data["size"], dtype=np.float32)

    if position.ndim != 2 or position.shape[1] != 3:
        raise ValueError(f"expected position shape (N, 3), got {position.shape}")
    if color.ndim != 2 or color.shape[1] != 4:
        raise ValueError(f"expected color shape (N, 4), got {color.shape}")
    if size.ndim != 1:
        raise ValueError(f"expected size shape (N,), got {size.shape}")
    if position.shape[0] != color.shape[0] or position.shape[0] != size.shape[0]:
        raise ValueError("position/color/size atom counts do not match")

    position_path = prepared / "atom_position.npy"
    color_path = prepared / "atom_color.npy"
    size_path = prepared / "atom_size.npy"
    np.save(position_path, position)
    np.save(color_path, color)
    np.save(size_path, size)

    artifacts = [
        artifact(position_path, bundle_root, "atom_position", "npy", dtype=str(position.dtype),
                 shape=list(position.shape), coordinate_system="source"),
        artifact(color_path, bundle_root, "atom_color", "npy", dtype=str(color.dtype),
                 shape=list(color.shape), color_space="rgba8"),
        artifact(size_path, bundle_root, "atom_size", "npy", dtype=str(size.dtype),
                 shape=list(size.shape)),
    ]
    pdb_sources = [
        {"path": relpath(path, ROOT), "bytes": path.stat().st_size}
        for path in _source_pdbs(pdb_dir)
    ]
    write_manifest(
        bundle_root,
        example_id=EXAMPLE_ID,
        title="6QZP Protein Point Cloud",
        status="committed",
        script=relpath(Path(__file__), ROOT),
        command=command_argv(relpath(Path(__file__), ROOT)),
        source={
            "name": "Existing repository 6QZP molecule bundle",
            "path": relpath(source, ROOT),
            "format": "npz",
            "arrays": ["position", "color", "size"],
            "pdb_bundles": pdb_sources,
            "license": "Inherited from repository data provenance.",
        },
        artifacts=artifacts,
        validation={
            "atom_count": int(position.shape[0]),
            "bounds": {
                "min": position.min(axis=0).astype(float).tolist(),
                "max": position.max(axis=0).astype(float).tolist(),
            },
            "size_range": [float(size.min()), float(size.max())],
        },
    )
    write_provenance(
        bundle_root,
        title="6QZP Protein Point Cloud",
        source_lines=[
            f"Prepared source file: `{relpath(source, ROOT)}`.",
            f"Adjacent PDB bundle files: {', '.join(item['path'] for item in pdb_sources)}.",
        ],
        processing_lines=[
            "Loaded `position`, `color`, and `size` from the source NPZ with NumPy.",
            "Wrote contiguous `.npy` arrays in `prepared/` without coordinate, color, or size conversion.",
            "Recorded the PDB bundle files as upstream provenance; they are not duplicated here.",
        ],
        license_lines=["Reuse follows the repository data provenance for `data/misc/molecule`."],
    )
    print(f"wrote {relpath(bundle_root, ROOT)} ({position.shape[0]} atoms)")


def main() -> None:
    """Run the protein preparation command."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--pdb-dir", type=Path, default=DEFAULT_PDB_DIR)
    parser.add_argument("--force", action="store_true", help="replace an existing output bundle")
    args = parser.parse_args()
    prepare(args.source, args.pdb_dir, args.force)


if __name__ == "__main__":
    main()
