"""Prepare local IBL BWM arrays for the C brain mesh example."""

from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np


DEFAULT_INPUT = Path("/home/cyrille/GIT/Viz/experiments/ibl/bwm.npz")
DEFAULT_OUTPUT = Path("build/local_data/ibl_bwm")


def _compute_vertex_normals(positions: np.ndarray, indices: np.ndarray) -> np.ndarray:
    """Compute area-weighted vertex normals for an indexed triangle mesh."""
    normals = np.zeros_like(positions, dtype=np.float64)
    triangles = indices.reshape(-1, 3)
    p0 = positions[triangles[:, 0]]
    p1 = positions[triangles[:, 1]]
    p2 = positions[triangles[:, 2]]
    face_normals = np.cross(p1 - p0, p2 - p0)
    np.add.at(normals, triangles[:, 0], face_normals)
    np.add.at(normals, triangles[:, 1], face_normals)
    np.add.at(normals, triangles[:, 2], face_normals)

    lengths = np.linalg.norm(normals, axis=1)
    valid = lengths > 0
    normals[valid] /= lengths[valid, None]
    normals[~valid] = (0.0, 0.0, 1.0)
    return np.ascontiguousarray(normals, dtype=np.float32)


def prepare(input_path: Path, output_dir: Path) -> None:
    input_path = input_path.expanduser().resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    data = np.load(input_path)
    required = {"cluster_pos", "cluster_color", "mesh_pos", "mesh_idx", "mesh_color"}
    missing = required.difference(data.files)
    if missing:
        raise ValueError(f"{input_path} is missing keys: {sorted(missing)}")

    cluster_pos = np.ascontiguousarray(data["cluster_pos"], dtype=np.float32)
    cluster_color = np.ascontiguousarray(data["cluster_color"], dtype=np.uint8)
    mesh_pos = np.ascontiguousarray(data["mesh_pos"], dtype=np.float32)
    mesh_idx = np.ascontiguousarray(data["mesh_idx"], dtype=np.uint32).reshape(-1, 3)
    mesh_color = np.tile(np.asarray(data["mesh_color"], dtype=np.uint8), (mesh_pos.shape[0], 1))
    mesh_color[:, 3] = 32
    mesh_color = np.ascontiguousarray(mesh_color, dtype=np.uint8)
    mesh_normal = _compute_vertex_normals(mesh_pos.astype(np.float64), mesh_idx)
    cluster_size = np.full(cluster_pos.shape[0], 5.0, dtype=np.float32)

    if cluster_pos.ndim != 2 or cluster_pos.shape[1] != 3:
        raise ValueError("cluster_pos must have shape (n, 3)")
    if cluster_color.shape != (cluster_pos.shape[0], 4):
        raise ValueError("cluster_color must have shape (n, 4)")
    if mesh_pos.ndim != 2 or mesh_pos.shape[1] != 3:
        raise ValueError("mesh_pos must have shape (n, 3)")
    if mesh_idx.size == 0 or int(mesh_idx.max()) >= mesh_pos.shape[0]:
        raise ValueError("mesh_idx contains an out-of-range vertex index")

    np.save(output_dir / "bwm_cluster_pos.npy", cluster_pos)
    np.save(output_dir / "bwm_cluster_color.npy", cluster_color)
    np.save(output_dir / "bwm_cluster_size.npy", cluster_size)
    np.save(output_dir / "bwm_mesh_pos.npy", mesh_pos)
    np.save(output_dir / "bwm_mesh_normal.npy", mesh_normal)
    np.save(output_dir / "bwm_mesh_color.npy", mesh_color)
    np.save(output_dir / "bwm_mesh_idx.npy", mesh_idx.reshape(-1))

    print(f"wrote {output_dir}")
    print(f"clusters: {cluster_pos.shape[0]}")
    print(f"mesh: {mesh_pos.shape[0]} vertices, {mesh_idx.shape[0]} triangles")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Extract the cached IBL BWM Datoviz v0.3 dataset into C-friendly .npy arrays."
    )
    parser.add_argument(
        "--input",
        type=Path,
        default=DEFAULT_INPUT,
        help=f"input bwm.npz path, default: {DEFAULT_INPUT}",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=DEFAULT_OUTPUT,
        help=f"output directory, default: {DEFAULT_OUTPUT}",
    )
    args = parser.parse_args()
    prepare(args.input, args.output_dir)


if __name__ == "__main__":
    main()
