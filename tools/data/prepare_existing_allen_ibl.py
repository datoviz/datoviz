#!/usr/bin/env python3
"""Prepare existing Allen/IBL mesh and volume assets as an example-data bundle."""

from __future__ import annotations

import argparse
import gzip
import json
import shutil
from pathlib import Path
from typing import Any

import numpy as np

from common import artifact, command_argv, ensure_bundle, relpath, write_manifest, write_provenance


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_MESH_DIR = ROOT / "data" / "allen_ibl_assets"
DEFAULT_VOLUME = ROOT / "data" / "volumes" / "allen_mouse_brain_rgba.npy.gz"
EXAMPLE_ID = "allen_ibl"


def _copy(source: Path, target: Path) -> None:
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, target)


def _load_gzip_npy_shape(path: Path) -> tuple[list[int], str]:
    with gzip.open(path, "rb") as stream:
        array = np.load(stream)
        return list(array.shape), str(array.dtype)


def prepare(mesh_dir: Path, volume: Path, force: bool) -> None:
    """Prepare the Allen/IBL example bundle from existing repository assets.

    @param mesh_dir source directory containing prepared Allen/IBL mesh arrays
    @param volume source gzip-compressed RGBA volume
    @param force whether to replace an existing output bundle
    """
    mesh_dir = mesh_dir.resolve()
    volume = volume.resolve()
    bundle_root, prepared = ensure_bundle(EXAMPLE_ID)
    if force and bundle_root.exists():
        shutil.rmtree(bundle_root)
        bundle_root, prepared = ensure_bundle(EXAMPLE_ID)
    if not mesh_dir.exists():
        raise FileNotFoundError(mesh_dir)
    if not volume.exists():
        raise FileNotFoundError(volume)

    mesh_files = {
        "position": "allen_ibl_mesh_pos.npy",
        "normal": "allen_ibl_mesh_normal.npy",
        "color": "allen_ibl_mesh_color.npy",
        "index": "allen_ibl_mesh_idx.npy",
    }
    arrays: dict[str, np.ndarray] = {}
    for role, filename in mesh_files.items():
        source_path = mesh_dir / filename
        if not source_path.exists():
            raise FileNotFoundError(source_path)
        arrays[role] = np.load(source_path)
        _copy(source_path, prepared / filename)

    source_metadata_path = mesh_dir / "metadata.json"
    source_metadata: dict[str, Any] = {}
    if source_metadata_path.exists():
        source_metadata = json.loads(source_metadata_path.read_text(encoding="utf8"))
        _copy(source_metadata_path, prepared / "metadata.json")

    volume_target = prepared / "allen_mouse_brain_rgba.npy.gz"
    _copy(volume, volume_target)
    volume_shape, volume_dtype = _load_gzip_npy_shape(volume_target)

    if arrays["position"].ndim != 2 or arrays["position"].shape[1] != 3:
        raise ValueError("mesh position array must have shape (N, 3)")
    if arrays["normal"].shape != arrays["position"].shape:
        raise ValueError("mesh normal array shape must match positions")
    if arrays["color"].ndim != 2 or arrays["color"].shape[1] != 4:
        raise ValueError("mesh color array must have shape (N, 4)")
    if arrays["color"].shape[0] != arrays["position"].shape[0]:
        raise ValueError("mesh color count must match positions")
    if arrays["index"].ndim != 1 or arrays["index"].size % 3 != 0:
        raise ValueError("mesh index array must be a flat triangle index list")

    artifacts = [
        artifact(
            prepared / mesh_files[role],
            bundle_root,
            f"mesh_{role}",
            "npy",
            dtype=str(arrays[role].dtype),
            shape=list(arrays[role].shape),
        )
        for role in ("position", "normal", "color", "index")
    ]
    artifacts.append(
        artifact(
            volume_target,
            bundle_root,
            "rgba_volume",
            "npy.gz",
            dtype=volume_dtype,
            shape=volume_shape,
        )
    )
    if source_metadata_path.exists():
        artifacts.append(artifact(prepared / "metadata.json", bundle_root, "mesh_metadata", "json"))

    write_manifest(
        bundle_root,
        example_id=EXAMPLE_ID,
        title="Allen Mouse Brain IBL Mesh And Volume",
        status="committed",
        script=relpath(Path(__file__), ROOT),
        command=command_argv(relpath(Path(__file__), ROOT)),
        source={
            "name": "Existing repository Allen/IBL assets",
            "mesh_dir": relpath(mesh_dir, ROOT),
            "volume": relpath(volume, ROOT),
            "license": "Inherited from repository data provenance.",
        },
        artifacts=artifacts,
        validation={
            "coordinate_system": source_metadata.get("coordinate_system", "IBL_ML_AP_DV"),
            "mesh_vertex_count": int(arrays["position"].shape[0]),
            "mesh_triangle_count": int(arrays["index"].size // 3),
            "volume_shape": volume_shape,
            "volume_dtype": volume_dtype,
            "regions": source_metadata.get("regions", []),
            "volume_bounds_scene": source_metadata.get("volume_bounds_scene"),
        },
    )
    write_provenance(
        bundle_root,
        title="Allen Mouse Brain IBL Mesh And Volume",
        source_lines=[
            f"Mesh arrays and metadata: `{relpath(mesh_dir, ROOT)}`.",
            f"RGBA volume: `{relpath(volume, ROOT)}`.",
        ],
        processing_lines=[
            "Copied existing mesh `.npy` assets into `prepared/` without conversion.",
            "Copied the gzip-compressed Allen mouse brain RGBA volume into `prepared/` without conversion.",
            "Copied source mesh metadata into `prepared/metadata.json` when present.",
        ],
        license_lines=[
            "Allen atlas and IBL-derived asset reuse follows the repository data provenance."
        ],
    )
    print(
        f"wrote {relpath(bundle_root, ROOT)} "
        f"({arrays['position'].shape[0]} vertices, {arrays['index'].size // 3} triangles)"
    )


def main() -> None:
    """Run the Allen/IBL preparation command."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mesh-dir", type=Path, default=DEFAULT_MESH_DIR)
    parser.add_argument("--volume", type=Path, default=DEFAULT_VOLUME)
    parser.add_argument("--force", action="store_true", help="replace an existing output bundle")
    args = parser.parse_args()
    prepare(args.mesh_dir, args.volume, args.force)


if __name__ == "__main__":
    main()
