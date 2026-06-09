#!/usr/bin/env python3
"""Prepare real Allen CCF volume, label, palette, and mesh data for the brain showcase."""

from __future__ import annotations

import argparse
import gzip
import json
import shutil
import struct
import sys
import urllib.request
from pathlib import Path
from typing import Any

import numpy as np

from common import CACHE_ROOT, artifact, command_argv, relpath, write_manifest, write_provenance


ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = ROOT / "tools"
EXAMPLE_ID = "brain_volume"
VERSION = 1
MAGIC = b"DVZBRN1\0"
DEFAULT_RESOLUTION_UM = 50
SELECTED_REGION_ID = 549
ALLEN_CCF_BASE = (
    "https://download.alleninstitute.org/informatics-archive/current-release/mouse_ccf"
)
STRUCTURE_GRAPH_URL = "http://api.brain-map.org/api/v2/structure_graph_download/1.json"
MESH_REGION_IDS = (997, 315, 1089, 549, 294)


def _download(url: str, target: Path, force: bool) -> None:
    """Download one source file unless it already exists."""
    target.parent.mkdir(parents=True, exist_ok=True)
    if target.exists() and target.stat().st_size > 0 and not force:
        return
    tmp = target.with_suffix(target.suffix + ".tmp")
    tmp.unlink(missing_ok=True)
    print(f"downloading {url}")
    urllib.request.urlretrieve(url, tmp)
    tmp.replace(target)


def _parse_nrrd(path: Path) -> tuple[np.ndarray, dict[str, str]]:
    """Parse the simple raw/gzip NRRD files used by Allen CCF volumes."""
    raw = path.read_bytes()
    header_end = raw.find(b"\n\n")
    separator_len = 2
    if header_end < 0:
        header_end = raw.find(b"\r\n\r\n")
        separator_len = 4
    if header_end < 0:
        raise ValueError(f"{path} has no NRRD header terminator")

    header_text = raw[:header_end].decode("ascii", errors="replace")
    meta: dict[str, str] = {}
    for line in header_text.splitlines():
        line = line.strip()
        if not line or line.startswith("#") or line.startswith("NRRD"):
            continue
        if ":" not in line:
            continue
        key, value = line.split(":", 1)
        meta[key.strip().lower()] = value.strip()

    sizes = tuple(int(part) for part in meta["sizes"].split())
    encoding = meta.get("encoding", "raw").lower()
    type_name = meta["type"].lower()
    dtype_map = {
        "uchar": np.uint8,
        "unsigned char": np.uint8,
        "uint8": np.uint8,
        "uint8_t": np.uint8,
        "ushort": np.uint16,
        "unsigned short": np.uint16,
        "uint16": np.uint16,
        "uint16_t": np.uint16,
        "uint": np.uint32,
        "unsigned int": np.uint32,
        "uint32": np.uint32,
        "uint32_t": np.uint32,
        "int": np.int32,
        "float": np.float32,
    }
    if type_name not in dtype_map:
        raise ValueError(f"unsupported NRRD type {type_name!r} in {path}")
    dtype = np.dtype(dtype_map[type_name])
    if dtype.itemsize > 1 and meta.get("endian", "little").lower() == "little":
        dtype = dtype.newbyteorder("<")

    payload = raw[header_end + separator_len :]
    if encoding in {"gzip", "gz"}:
        payload = gzip.decompress(payload)
    elif encoding not in {"raw", "txt", "text"}:
        raise ValueError(f"unsupported NRRD encoding {encoding!r} in {path}")

    array = np.frombuffer(payload, dtype=dtype)
    expected = int(np.prod(sizes))
    if array.size != expected:
        raise ValueError(f"{path} contains {array.size} values, expected {expected}")
    return np.ascontiguousarray(array.reshape(sizes)), meta


def _flatten_structures(node: dict[str, Any], out: dict[int, dict[str, Any]]) -> None:
    """Flatten Allen structure graph nodes by structure id."""
    sid = int(node["id"])
    out[sid] = node
    for child in node.get("children", []) or []:
        _flatten_structures(child, out)


def _load_structure_graph(path: Path) -> dict[int, dict[str, Any]]:
    """Load Allen structure graph JSON by structure id."""
    payload = json.loads(path.read_text(encoding="utf8"))
    structures: dict[int, dict[str, Any]] = {}
    for root in payload.get("msg", []):
        _flatten_structures(root, structures)
    return structures


def _rgba_from_hex(hex_color: str | None, alpha: int = 255) -> tuple[int, int, int, int]:
    """Parse Allen RGB hex colors."""
    if not hex_color:
        return (128, 128, 128, alpha)
    value = hex_color.strip().lstrip("#")
    if len(value) != 6:
        return (128, 128, 128, alpha)
    return (int(value[0:2], 16), int(value[2:4], 16), int(value[4:6], 16), alpha)


def _gallery_color(allen_rgba: tuple[int, int, int, int], label_id: int) -> tuple[int, int, int, int]:
    """Return a Datoviz-gallery color for one Allen label."""
    if label_id == 0:
        return (0, 0, 0, 0)
    if label_id == SELECTED_REGION_ID:
        return (255, 183, 3, 230)

    graphite = np.array([14.0, 17.0, 23.0])
    cyan = np.array([76.0, 201.0, 240.0])
    source = np.array(allen_rgba[:3], dtype=np.float64)
    gray = np.array([source.mean(), source.mean(), source.mean()])
    muted = 0.56 * gray + 0.24 * source + 0.20 * cyan
    rgb = np.clip(0.72 * muted + 0.28 * graphite, 0.0, 255.0).astype(np.uint8)
    return (int(rgb[0]), int(rgb[1]), int(rgb[2]), 172)


def _mesh_color(region_id: int) -> tuple[int, int, int, int]:
    """Return the canonical gallery mesh color for one selected Allen region mesh."""
    if region_id == SELECTED_REGION_ID:
        return (255, 183, 3, 190)
    if region_id == 997:
        return (76, 201, 240, 54)
    return (88, 180, 210, 86)


def _to_dv_ml_ap(array: np.ndarray) -> np.ndarray:
    """Convert Allen AP,DV,ML arrays to the existing Datoviz texture order DV,ML,AP."""
    if array.ndim != 3:
        raise ValueError("Allen CCF volume must be 3D")
    return np.ascontiguousarray(np.transpose(array, (1, 2, 0)))


def _prepare_mesh(source_dir: Path, work_dir: Path, force: bool) -> tuple[dict[str, np.ndarray], dict[str, Any]]:
    """Prepare selected Allen meshes into IBL display coordinates."""
    output = work_dir / "mesh"
    if force and output.exists():
        shutil.rmtree(output)
    if not (output / "metadata.json").exists():
        sys.path.insert(0, str(TOOLS_ROOT))
        from prepare_allen_ibl_assets import prepare as prepare_mesh

        prepare_mesh(output, source_dir / "structure_meshes", 200.0, MESH_REGION_IDS)

    arrays = {
        "position": np.load(output / "allen_ibl_mesh_pos.npy").astype("<f4", copy=False),
        "normal": np.load(output / "allen_ibl_mesh_normal.npy").astype("<f4", copy=False),
        "index": np.load(output / "allen_ibl_mesh_idx.npy").astype("<u4", copy=False),
    }
    metadata = json.loads((output / "metadata.json").read_text(encoding="utf8"))
    return arrays, metadata


def _write_binary(
    path: Path,
    resolution_um: int,
    average: np.ndarray,
    annotation: np.ndarray,
    categories: list[dict[str, Any]],
    mesh: dict[str, np.ndarray],
    mesh_metadata: dict[str, Any],
) -> None:
    """Write the native compact binary consumed by the C showcase."""
    path.parent.mkdir(parents=True, exist_ok=True)
    bounds = mesh_metadata.get("volume_bounds_scene") or [[-1.0, -1.0, -1.0], [1.0, 1.0, 1.0]]
    bounds_min = [float(x) for x in bounds[0]]
    bounds_max = [float(x) for x in bounds[1]]
    mesh_pos = np.ascontiguousarray(mesh["position"], dtype="<f4")
    mesh_normal = np.ascontiguousarray(mesh["normal"], dtype="<f4")
    mesh_idx = np.ascontiguousarray(mesh["index"], dtype="<u4")

    with path.open("wb") as f:
        f.write(MAGIC)
        f.write(
            struct.pack(
                "<IIIIIfIIII6f",
                VERSION,
                int(average.shape[0]),
                int(average.shape[1]),
                int(average.shape[2]),
                int(resolution_um),
                float(resolution_um),
                len(categories),
                mesh_pos.shape[0],
                mesh_idx.size,
                SELECTED_REGION_ID,
                *(bounds_min + bounds_max),
            )
        )
        f.write(np.ascontiguousarray(average, dtype=np.uint8).tobytes(order="C"))
        f.write(np.ascontiguousarray(annotation, dtype="<u4").tobytes(order="C"))
        for category in categories:
            acronym = category["acronym"].encode("utf8")[:31]
            name = category["name"].encode("utf8")[:95]
            f.write(
                struct.pack(
                    "<IIIIIIII",
                    int(category["label_id"]),
                    int(category["structure_id"]),
                    int(category["order"]),
                    int(category["flags"]),
                    int(category["vertex_start"]),
                    int(category["vertex_count"]),
                    int(category["index_start"]),
                    int(category["index_count"]),
                )
            )
            f.write(bytes(category["allen_rgba"]))
            f.write(bytes(category["gallery_rgba"]))
            f.write(bytes(category["mesh_rgba"]))
            f.write(acronym + b"\0" * (32 - len(acronym)))
            f.write(name + b"\0" * (96 - len(name)))
        f.write(mesh_pos.tobytes(order="C"))
        f.write(mesh_normal.tobytes(order="C"))
        f.write(mesh_idx.tobytes(order="C"))


def prepare(resolution_um: int, force: bool) -> Path:
    """Prepare the brain-volume-mesh bundle into the local cache."""
    if resolution_um not in {10, 25, 50, 100}:
        raise ValueError("--resolution must be one of 10, 25, 50, or 100")

    bundle_root = CACHE_ROOT / EXAMPLE_ID
    source_dir = bundle_root / "source"
    work_dir = bundle_root / "work"
    prepared = bundle_root / "prepared"
    if force and bundle_root.exists():
        shutil.rmtree(bundle_root)
    source_dir.mkdir(parents=True, exist_ok=True)
    work_dir.mkdir(parents=True, exist_ok=True)
    prepared.mkdir(parents=True, exist_ok=True)

    average_path = source_dir / f"average_template_{resolution_um}.nrrd"
    annotation_path = source_dir / f"annotation_{resolution_um}.nrrd"
    graph_path = source_dir / "structure_graph_1.json"
    _download(
        f"{ALLEN_CCF_BASE}/average_template/average_template_{resolution_um}.nrrd",
        average_path,
        force,
    )
    _download(
        f"{ALLEN_CCF_BASE}/annotation/ccf_2017/annotation_{resolution_um}.nrrd",
        annotation_path,
        force,
    )
    _download(STRUCTURE_GRAPH_URL, graph_path, force)

    average_raw, average_meta = _parse_nrrd(average_path)
    annotation_raw, annotation_meta = _parse_nrrd(annotation_path)
    if average_raw.shape != annotation_raw.shape:
        raise ValueError(f"average and annotation shapes differ: {average_raw.shape} vs {annotation_raw.shape}")

    average = _to_dv_ml_ap(average_raw.astype(np.uint8, copy=False))
    annotation = _to_dv_ml_ap(annotation_raw.astype(np.uint32, copy=False))
    structures = _load_structure_graph(graph_path)
    mesh, mesh_metadata = _prepare_mesh(source_dir, work_dir, force)
    mesh_regions = {int(region["id"]): region for region in mesh_metadata.get("regions", [])}

    unique_labels = sorted(int(value) for value in np.unique(annotation) if int(value) != 0)
    categories: list[dict[str, Any]] = [
        {
            "label_id": 0,
            "structure_id": 0,
            "order": 0,
            "flags": 0,
            "acronym": "background",
            "name": "Background",
            "allen_rgba": (0, 0, 0, 0),
            "gallery_rgba": (0, 0, 0, 0),
            "mesh_rgba": (0, 0, 0, 0),
            "vertex_start": 0,
            "vertex_count": 0,
            "index_start": 0,
            "index_count": 0,
        }
    ]
    for order, label_id in enumerate(unique_labels, start=1):
        structure = structures.get(label_id, {})
        allen_rgba = _rgba_from_hex(structure.get("color_hex_triplet"), 255)
        mesh_region = mesh_regions.get(label_id, {})
        categories.append(
            {
                "label_id": label_id,
                "structure_id": label_id,
                "order": order,
                "flags": 1 if label_id == SELECTED_REGION_ID else 0,
                "acronym": structure.get("acronym", f"id{label_id}"),
                "name": structure.get("name", f"Allen structure {label_id}"),
                "allen_rgba": allen_rgba,
                "gallery_rgba": _gallery_color(allen_rgba, label_id),
                "mesh_rgba": _mesh_color(label_id) if label_id in mesh_regions else (0, 0, 0, 0),
                "vertex_start": int(mesh_region.get("vertex_start", 0)),
                "vertex_count": int(mesh_region.get("vertex_count", 0)),
                "index_start": int(mesh_region.get("index_start", 0)),
                "index_count": int(mesh_region.get("index_count", 0)),
            }
        )

    binary_path = prepared / "brain_volume.bin"
    structures_path = prepared / "structures.json"
    _write_binary(binary_path, resolution_um, average, annotation, categories, mesh, mesh_metadata)
    structures_path.write_text(
        json.dumps(
            {
                "source": {
                    "average_template": average_path.name,
                    "annotation": annotation_path.name,
                    "structure_graph": graph_path.name,
                    "resolution_um": resolution_um,
                },
                "texture_order": "DV_ML_AP",
                "labels": categories,
                "mesh": mesh_metadata,
                "nrrd": {"average": average_meta, "annotation": annotation_meta},
            },
            indent=2,
            sort_keys=True,
        )
        + "\n",
        encoding="utf8",
    )

    artifacts = [
        artifact(
            binary_path,
            bundle_root,
            "brain_volume_binary",
            "bin",
            version=VERSION,
            average_dtype="uint8",
            annotation_dtype="uint32",
            texture_order="DV_ML_AP",
            shape=list(average.shape),
            category_count=len(categories),
            mesh_vertex_count=int(mesh["position"].shape[0]),
            mesh_triangle_count=int(mesh["index"].size // 3),
        ),
        artifact(structures_path, bundle_root, "structure_metadata", "json"),
    ]
    write_manifest(
        bundle_root,
        example_id=EXAMPLE_ID,
        title="Allen Mouse Brain Volume And Atlas Mesh",
        status="local-cache",
        script=relpath(Path(__file__), ROOT),
        command=command_argv(relpath(Path(__file__), ROOT), [f"--resolution={resolution_um}"]),
        source={
            "name": "Allen Mouse Brain Common Coordinate Framework",
            "average_template": f"{ALLEN_CCF_BASE}/average_template/average_template_{resolution_um}.nrrd",
            "annotation": f"{ALLEN_CCF_BASE}/annotation/ccf_2017/annotation_{resolution_um}.nrrd",
            "structure_graph": STRUCTURE_GRAPH_URL,
            "mesh_regions": list(MESH_REGION_IDS),
            "license": "Allen Institute Terms of Use",
        },
        artifacts=artifacts,
        validation={
            "resolution_um": resolution_um,
            "texture_order": "DV_ML_AP",
            "unique_label_count": len(unique_labels),
            "selected_region_id": SELECTED_REGION_ID,
            "selected_region_name": structures.get(SELECTED_REGION_ID, {}).get("name"),
            "selected_region_acronym": structures.get(SELECTED_REGION_ID, {}).get("acronym"),
        },
    )
    write_provenance(
        bundle_root,
        title="Allen Mouse Brain Volume And Atlas Mesh",
        source_lines=[
            f"Average anatomical template: `{average_path.name}` from Allen CCF at {resolution_um} um.",
            f"Annotation label volume: `{annotation_path.name}` from Allen CCF 2017 at {resolution_um} um.",
            "Structure ontology: Allen structure graph 1 JSON.",
            "Selected atlas meshes: Allen CCF structure meshes for root, isocortex, hippocampal formation, thalamus, and superior colliculus.",
        ],
        processing_lines=[
            "Converted NRRD source volumes into Datoviz texture order `DV_ML_AP`.",
            "Stored anatomical density as `uint8` and preserved annotation labels as `uint32` Allen structure ids.",
            "Generated a Datoviz gallery palette while preserving original Allen RGB colors in metadata.",
            "Prepared selected structure meshes in IBL `ML_AP_DV` scene coordinates with the existing Allen/IBL mesh helper.",
            "Wrote a compact binary consumed by the C showcase plus JSON structure metadata.",
        ],
        license_lines=[
            "Allen CCF and Allen Brain Atlas content is used under Allen Institute Terms of Use.",
            "Generated gallery colors are a Datoviz presentation layer; original Allen colors remain recorded in `structures.json`.",
        ],
        notes=[
            "This script writes to `.cache/datoviz/examples/brain_volume` by default and does not modify the `data` submodule.",
        ],
    )
    print(f"wrote {bundle_root}")
    print(
        f"volume: {average.shape[0]}x{average.shape[1]}x{average.shape[2]}, "
        f"{len(categories)} categories, {mesh['index'].size // 3} mesh triangles"
    )
    return bundle_root


def main() -> None:
    """Run the preparation script."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--resolution",
        type=int,
        default=DEFAULT_RESOLUTION_UM,
        choices=(10, 25, 50, 100),
        help="Allen CCF isotropic resolution in microns; 50 is the compact gallery default",
    )
    parser.add_argument("--force", action="store_true", help="redownload and regenerate outputs")
    args = parser.parse_args()
    prepare(args.resolution, args.force)


if __name__ == "__main__":
    main()
