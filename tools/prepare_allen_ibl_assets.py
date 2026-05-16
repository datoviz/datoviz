"""Prepare Allen mouse brain atlas meshes in Datoviz/IBL display coordinates."""

from __future__ import annotations

import argparse
import json
import struct
import urllib.error
import urllib.request
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

import numpy as np


DEFAULT_OUTPUT = Path("data/allen_ibl_assets")
DEFAULT_CACHE = DEFAULT_OUTPUT / "cache"
DEFAULT_SCALE = 200.0
DEFAULT_RES_UM = 25.0
DEFAULT_VOLUME_SHAPE_AP_ML_DV = (528, 456, 320)
ALLEN_MESH_URL = (
    "https://download.alleninstitute.org/informatics-archive/current-release/"
    "mouse_ccf/annotation/ccf_2017/structure_meshes/{suffix}/{region_id}.{suffix}"
)


@dataclass(frozen=True)
class RegionSpec:
    """Selected Allen atlas mesh metadata used by the first C viewer bundle."""

    region_id: int
    acronym: str
    name: str
    color: tuple[int, int, int, int]


@dataclass(frozen=True)
class MeshData:
    """Minimal triangle mesh payload loaded from Allen OBJ/PLY assets."""

    vertices: np.ndarray
    faces: np.ndarray


DEFAULT_REGIONS = (
    RegionSpec(997, "root", "Whole brain", (214, 214, 214, 42)),
    RegionSpec(315, "Isocortex", "Isocortex", (112, 185, 102, 156)),
    RegionSpec(1089, "HPF", "Hippocampal formation", (126, 208, 75, 178)),
    RegionSpec(549, "TH", "Thalamus", (255, 144, 159, 190)),
    RegionSpec(294, "SC", "Superior colliculus", (255, 153, 0, 184)),
)


def _download_mesh(region_id: int, cache_dir: Path) -> Path:
    """Download one Allen CCF structure mesh into the local cache."""
    cache_dir.mkdir(parents=True, exist_ok=True)
    last_error: Exception | None = None
    for suffix in ("obj", "ply"):
        path = cache_dir / f"{region_id}.{suffix}"
        if path.exists() and path.stat().st_size > 0:
            return path
        url = ALLEN_MESH_URL.format(region_id=region_id, suffix=suffix)
        try:
            print(f"downloading {url}")
            urllib.request.urlretrieve(url, path)
            if path.stat().st_size > 0:
                return path
        except (urllib.error.URLError, OSError) as exc:
            last_error = exc
            path.unlink(missing_ok=True)
    raise RuntimeError(f"could not download mesh {region_id}") from last_error


def _load_obj(path: Path) -> MeshData:
    """Load a simple triangulated Wavefront OBJ mesh."""
    vertices: list[tuple[float, float, float]] = []
    faces: list[tuple[int, int, int]] = []
    with path.open("r", encoding="utf8", errors="replace") as stream:
        for line in stream:
            if line.startswith("v "):
                parts = line.split()
                if len(parts) >= 4:
                    vertices.append((float(parts[1]), float(parts[2]), float(parts[3])))
            elif line.startswith("f "):
                raw = line.split()[1:]
                if len(raw) < 3:
                    continue
                idx = [int(item.split("/", 1)[0]) - 1 for item in raw]
                for i in range(1, len(idx) - 1):
                    faces.append((idx[0], idx[i], idx[i + 1]))
    return MeshData(
        np.ascontiguousarray(vertices, dtype=np.float64),
        np.ascontiguousarray(faces, dtype=np.uint32),
    )


def _load_ply(path: Path) -> MeshData:
    """Load a simple ASCII or binary-little-endian PLY triangle mesh."""
    raw = path.read_bytes()
    header_end = raw.find(b"end_header\n")
    if header_end < 0:
        raise ValueError(f"{path} has no PLY end_header")
    header_text = raw[:header_end].decode("ascii", errors="replace")
    header_lines = header_text.splitlines()
    if not header_lines or header_lines[0] != "ply":
        raise ValueError(f"{path} is not a PLY file")

    fmt = ""
    vertex_count = 0
    face_count = 0
    vertex_properties = 0
    in_vertex = False
    for line in header_lines:
        if line.startswith("format "):
            fmt = line.split()[1]
        elif line.startswith("element vertex "):
            vertex_count = int(line.split()[2])
            in_vertex = True
        elif line.startswith("element face "):
            face_count = int(line.split()[2])
            in_vertex = False
        elif in_vertex and line.startswith("property "):
            vertex_properties += 1

    if fmt == "ascii":
        text = raw[header_end + len(b"end_header\n") :].decode("ascii", errors="replace")
        lines = iter(text.splitlines())
        vertices = []
        for _ in range(vertex_count):
            parts = next(lines).split()
            vertices.append((float(parts[0]), float(parts[1]), float(parts[2])))

        faces = []
        for _ in range(face_count):
            parts = next(lines).split()
            if not parts:
                continue
            count = int(parts[0])
            idx = [int(value) for value in parts[1 : 1 + count]]
            for i in range(1, len(idx) - 1):
                faces.append((idx[0], idx[i], idx[i + 1]))
    elif fmt == "binary_little_endian":
        offset = header_end + len(b"end_header\n")
        vertex_stride = 4 * vertex_properties
        vertices = []
        for i in range(vertex_count):
            vertices.append(struct.unpack_from("<fff", raw, offset + i * vertex_stride))
        offset += vertex_count * vertex_stride

        faces = []
        for _ in range(face_count):
            count = raw[offset]
            offset += 1
            idx = struct.unpack_from("<" + "i" * count, raw, offset)
            offset += 4 * count
            for i in range(1, len(idx) - 1):
                faces.append((idx[0], idx[i], idx[i + 1]))
    else:
        raise ValueError(f"{path} has unsupported PLY format {fmt!r}")

    return MeshData(
        np.ascontiguousarray(vertices, dtype=np.float64),
        np.ascontiguousarray(faces, dtype=np.uint32),
    )


def _load_region_mesh(region: RegionSpec, cache_dir: Path) -> MeshData:
    """Load one cached or downloaded Allen CCF region mesh."""
    path = _download_mesh(region.region_id, cache_dir)
    if path.suffix == ".obj":
        mesh = _load_obj(path)
    elif path.suffix == ".ply":
        mesh = _load_ply(path)
    else:
        raise ValueError(f"unsupported mesh suffix: {path.suffix}")
    if mesh.vertices.size == 0 or mesh.faces.size == 0:
        raise ValueError(f"empty mesh for region {region.region_id}")
    return mesh


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


def _scene_volume_bounds(ba, offset_xyz_m: np.ndarray, scale: float) -> list[list[float]]:
    """Return scene-space bounds for the 25um Allen RGBA volume."""
    ap, ml, dv = DEFAULT_VOLUME_SHAPE_AP_ML_DV
    corners = np.array(
        [
            [0.0, 0.0, 0.0],
            [ap * DEFAULT_RES_UM, 0.0, 0.0],
            [0.0, dv * DEFAULT_RES_UM, 0.0],
            [0.0, 0.0, ml * DEFAULT_RES_UM],
            [ap * DEFAULT_RES_UM, dv * DEFAULT_RES_UM, 0.0],
            [ap * DEFAULT_RES_UM, 0.0, ml * DEFAULT_RES_UM],
            [0.0, dv * DEFAULT_RES_UM, ml * DEFAULT_RES_UM],
            [ap * DEFAULT_RES_UM, dv * DEFAULT_RES_UM, ml * DEFAULT_RES_UM],
        ],
        dtype=np.float64,
    )
    xyz = ba.ccf2xyz(corners, ccf_order="apdvml")
    scene = (xyz - offset_xyz_m) * scale
    return [scene.min(axis=0).tolist(), scene.max(axis=0).tolist()]


def _region_specs(region_ids: Iterable[int] | None) -> tuple[RegionSpec, ...]:
    """Return region specifications, optionally filtered by id."""
    if region_ids is None:
        return DEFAULT_REGIONS
    by_id = {region.region_id: region for region in DEFAULT_REGIONS}
    missing = [region_id for region_id in region_ids if region_id not in by_id]
    if missing:
        raise ValueError(f"region ids need explicit color metadata first: {missing}")
    return tuple(by_id[region_id] for region_id in region_ids)


def prepare(output_dir: Path, cache_dir: Path, scale: float, region_ids: Iterable[int] | None) -> None:
    """Prepare the combined atlas mesh bundle consumed by the C example."""
    try:
        from iblatlas.atlas import AllenAtlas
    except ModuleNotFoundError as exc:
        raise RuntimeError(
            "iblatlas is required for authoritative Allen CCF -> IBL xyz conversion; "
            "install it in this Python environment before preparing assets"
        ) from exc

    output_dir.mkdir(parents=True, exist_ok=True)
    regions = _region_specs(region_ids)
    ba = AllenAtlas(res_um=int(DEFAULT_RES_UM))

    root_mesh = _load_region_mesh(regions[0], cache_dir)
    root_xyz = ba.ccf2xyz(np.asarray(root_mesh.vertices, dtype=np.float64), ccf_order="apdvml")
    offset_xyz_m = root_xyz.mean(axis=0)

    pos_chunks: list[np.ndarray] = []
    idx_chunks: list[np.ndarray] = []
    color_chunks: list[np.ndarray] = []
    metadata_regions = []
    vertex_offset = 0

    for region in regions:
        mesh = _load_region_mesh(region, cache_dir)
        ccf = np.asarray(mesh.vertices, dtype=np.float64)
        xyz = ba.ccf2xyz(ccf, ccf_order="apdvml")
        pos = np.ascontiguousarray((xyz - offset_xyz_m) * scale, dtype=np.float32)
        idx = np.ascontiguousarray(np.asarray(mesh.faces, dtype=np.uint32).reshape(-1) + vertex_offset)
        color = np.empty((pos.shape[0], 4), dtype=np.uint8)
        color[:] = np.asarray(region.color, dtype=np.uint8)

        pos_chunks.append(pos)
        idx_chunks.append(idx)
        color_chunks.append(color)
        metadata_regions.append(
            {
                "id": region.region_id,
                "acronym": region.acronym,
                "name": region.name,
                "rgba": list(region.color),
                "vertex_start": vertex_offset,
                "vertex_count": int(pos.shape[0]),
                "index_start": int(sum(chunk.size for chunk in idx_chunks[:-1])),
                "index_count": int(idx.size),
            }
        )
        vertex_offset += pos.shape[0]

    atlas_pos = np.ascontiguousarray(np.concatenate(pos_chunks, axis=0), dtype=np.float32)
    atlas_idx = np.ascontiguousarray(np.concatenate(idx_chunks, axis=0), dtype=np.uint32)
    atlas_color = np.ascontiguousarray(np.concatenate(color_chunks, axis=0), dtype=np.uint8)
    atlas_normal = _compute_vertex_normals(atlas_pos.astype(np.float64), atlas_idx)

    np.save(output_dir / "allen_ibl_mesh_pos.npy", atlas_pos)
    np.save(output_dir / "allen_ibl_mesh_normal.npy", atlas_normal)
    np.save(output_dir / "allen_ibl_mesh_color.npy", atlas_color)
    np.save(output_dir / "allen_ibl_mesh_idx.npy", atlas_idx)

    metadata = {
        "coordinate_system": "IBL_ML_AP_DV",
        "source_ccf_order": "apdvml",
        "res_um": DEFAULT_RES_UM,
        "offset_xyz_m": offset_xyz_m.tolist(),
        "scale": scale,
        "volume_shape_ap_ml_dv": list(DEFAULT_VOLUME_SHAPE_AP_ML_DV),
        "texture_axis_for_ml_ap_dv": {"ML": "X", "AP": "Y", "DV": "Z"},
        "volume_bounds_scene": _scene_volume_bounds(ba, offset_xyz_m, scale),
        "mesh_files": {
            "position": "allen_ibl_mesh_pos.npy",
            "normal": "allen_ibl_mesh_normal.npy",
            "color": "allen_ibl_mesh_color.npy",
            "index": "allen_ibl_mesh_idx.npy",
        },
        "regions": metadata_regions,
    }
    (output_dir / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n")

    print(f"wrote {output_dir}")
    print(f"mesh: {atlas_pos.shape[0]} vertices, {atlas_idx.size // 3} triangles")
    print(f"offset_xyz_m: {offset_xyz_m.tolist()}")


def main() -> None:
    """Run the Allen/IBL mesh asset preparation command."""
    parser = argparse.ArgumentParser(
        description="Prepare selected Allen atlas meshes in IBL display coordinates."
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=DEFAULT_OUTPUT,
        help=f"output directory, default: {DEFAULT_OUTPUT}",
    )
    parser.add_argument(
        "--cache-dir",
        type=Path,
        default=DEFAULT_CACHE,
        help=f"download cache directory, default: {DEFAULT_CACHE}",
    )
    parser.add_argument("--scale", type=float, default=DEFAULT_SCALE, help="display scale")
    parser.add_argument(
        "--regions",
        type=int,
        nargs="+",
        default=None,
        help="subset of supported region ids, default: root/isocortex/HPF/TH/SC",
    )
    args = parser.parse_args()
    prepare(args.output_dir, args.cache_dir, args.scale, args.regions)


if __name__ == "__main__":
    main()
