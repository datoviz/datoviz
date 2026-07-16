#!/usr/bin/env python3
"""Prepare a compact human cortical dSPM activity bundle from OpenNeuro ds000248."""

from __future__ import annotations

import argparse
from collections import deque
import hashlib
import json
import shutil
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import numpy as np
import requests

from common import CACHE_ROOT, artifact, command_argv, relpath, write_manifest, write_provenance


ROOT = Path(__file__).resolve().parents[2]
EXAMPLE_ID = "cortical_activity"
BUNDLE_VERSION = 3
BINARY_MAGIC = b"DVZCTA3\0"
BINARY_HEADER_FORMAT = "<8s11I5f10I"
BINARY_HEADER_SIZE = struct.calcsize(BINARY_HEADER_FORMAT)

OPENNEURO_DATASET = "ds000248"
OPENNEURO_SNAPSHOT = "1.2.4"
OPENNEURO_DOI = "10.18112/openneuro.ds000248.v1.2.4"
OPENNEURO_BASE = f"https://s3.amazonaws.com/openneuro.org/{OPENNEURO_DATASET}"

MNE_VERSION = "1.12.1"
MNE_BIDS_VERSION = "0.19.0"
NIBABEL_VERSION = "5.4.2"
NUMPY_VERSION = "2.3.4"
SCIPY_VERSION = "1.18.0"
PREPARE_COMMAND = (
    "uv run --isolated --with mne==1.12.1 --with mne-bids==0.19.0 "
    "--with nibabel==5.4.2 --with numpy==2.3.4 --with scipy==1.18.0 --with requests "
    "python tools/data/prepare_cortical_activity.py"
)

CONDITION = "Auditory/Left"
TMIN_S = -0.2
TMAX_S = 0.5
OUTPUT_TMIN_S = 0.0
OUTPUT_TMAX_S = 0.24
LOWPASS_HZ = 40.0
HIGHPASS_HZ = 1.0
SOURCE_SPACING = "oct6"
SMOOTHING_STEPS = 5
SNR = 3.0
LAMBDA2 = 1.0 / SNR**2
# Match the established MNE audiovisual sample display convention. Values below fmin remain
# neutral cortex; opacity rises through fmid and the sequential ramp saturates at fmax.
DISPLAY_LIMITS = (8.0, 12.0, 15.0)
DISPLAY_EXTENT = 1.85
INTERPOLATION_NEIGHBORS = 16
INTERPOLATION_WEIGHT_TOLERANCE = 2e-4
INTERPOLATION_ANGLE_TOLERANCE_RAD = 1e-4


@dataclass(frozen=True)
class SourceFile:
    path: str
    size: int
    digest: str


SOURCE_FILES = (
    SourceFile(
        "dataset_description.json",
        1536,
        "sha256:07fbafe6daab4385b064c8eaa2ffb5a666f483745061328ac0dce89c34cccc71",
    ),
    SourceFile(
        "sub-01/meg/sub-01_task-audiovisual_run-01_meg.fif",
        128461384,
        "md5:c1d77edb3d2f47abbe3e58befe5b6d52",
    ),
    SourceFile(
        "sub-01/meg/sub-01_task-audiovisual_run-01_events.tsv",
        13969,
        "sha256:a7f7c77010c9204ebe601d44b2af23a65e3926a855dd74553e1725a1515ef1c7",
    ),
    SourceFile(
        "sub-01/meg/sub-01_task-audiovisual_run-01_channels.tsv",
        39420,
        "sha256:c2564204af8912f03b9fd7652efb320678664869d601d96f17047bb560d137a5",
    ),
    SourceFile(
        "sub-01/meg/sub-01_task-audiovisual_run-01_meg.json",
        560,
        "sha256:7f00d1d65f3e82a5fa986e8b420722761b98df0ad06a2972b7439e3aadaaa318",
    ),
    SourceFile(
        "sub-01/meg/sub-01_coordsystem.json",
        1160,
        "sha256:366dbb66e10e5a13b31c8218c00762fe3fe24a1583bad21f93a6a36684cdf1bf",
    ),
    SourceFile(
        "sub-01/anat/sub-01_T1w.nii.gz",
        4284563,
        "md5:a06ddbdc48f0f61d54aae105288a1699",
    ),
    SourceFile(
        "sub-01/anat/sub-01_T1w.json",
        413,
        "sha256:10150bef03cd5b5341df945bea63791eda407c612d6ba0daf0740ea1559b2fa6",
    ),
    SourceFile(
        "derivatives/freesurfer/subjects/sub-01/mri/T1.mgz",
        3944794,
        "md5:f002ae5f451e6eb20a070025444cebdd",
    ),
    SourceFile(
        "derivatives/freesurfer/subjects/sub-01/bem/inner_skull.surf",
        92197,
        "md5:12f885e44ed2014a55b678c2abb1359f",
    ),
    SourceFile(
        "derivatives/freesurfer/subjects/sub-01/surf/lh.white",
        5705324,
        "md5:7a61de7bb69384614355c3853a6cd30c",
    ),
    SourceFile(
        "derivatives/freesurfer/subjects/sub-01/surf/lh.pial",
        5705324,
        "md5:36d51554f1e74eadcef4601a67de929f",
    ),
    SourceFile(
        "derivatives/freesurfer/subjects/sub-01/surf/lh.inflated",
        5706248,
        "md5:6ddbb17e160a857f6b1041addacc395e",
    ),
    SourceFile(
        "derivatives/freesurfer/subjects/sub-01/surf/lh.sphere",
        5706775,
        "md5:9feb604fa7cc509b0318810850c1439b",
    ),
    SourceFile(
        "derivatives/freesurfer/subjects/sub-01/surf/rh.white",
        5811452,
        "md5:b7c2181a6f5ee2acd103d445346f41ec",
    ),
    SourceFile(
        "derivatives/freesurfer/subjects/sub-01/surf/rh.pial",
        5811452,
        "md5:e39434a0c4e1ef7220593c15845dcc73",
    ),
    SourceFile(
        "derivatives/freesurfer/subjects/sub-01/surf/rh.inflated",
        5812376,
        "md5:4196931400f0f3c35d4073dc3378fcf6",
    ),
    SourceFile(
        "derivatives/freesurfer/subjects/sub-01/surf/rh.sphere",
        5812903,
        "md5:4e280f79abcfdde0b9453a3fa08c2462",
    ),
)


def _digest(path: Path, algorithm: str) -> str:
    """Return a hexadecimal file digest."""
    digest = hashlib.new(algorithm)
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _verify_source(path: Path, source: SourceFile) -> None:
    """Reject a source file that differs from the pinned snapshot payload."""
    if path.stat().st_size != source.size:
        raise ValueError(f"size mismatch for {source.path}: expected {source.size}, got {path.stat().st_size}")
    algorithm, expected = source.digest.split(":", 1)
    actual = _digest(path, algorithm)
    if actual != expected:
        raise ValueError(f"{algorithm} mismatch for {source.path}: expected {expected}, got {actual}")


def _download_source(source_root: Path, source: SourceFile, force: bool) -> Path:
    """Download and verify one pinned OpenNeuro source file."""
    destination = source_root / source.path
    if destination.exists() and not force:
        _verify_source(destination, source)
        return destination

    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = destination.with_suffix(destination.suffix + ".part")
    temporary.unlink(missing_ok=True)
    url = f"{OPENNEURO_BASE}/{source.path}"
    with requests.get(url, stream=True, timeout=120, headers={"User-Agent": "Datoviz/0.4"}) as response:
        response.raise_for_status()
        with temporary.open("wb") as stream:
            for chunk in response.iter_content(chunk_size=1024 * 1024):
                if chunk:
                    stream.write(chunk)
    temporary.replace(destination)
    _verify_source(destination, source)
    return destination


def _download_sources(source_root: Path, force: bool) -> None:
    """Download the minimal BIDS and FreeSurfer subset used by this preparation."""
    for source in SOURCE_FILES:
        path = _download_source(source_root, source, force)
        print(f"source {source.path} ({path.stat().st_size} bytes)")

    description = json.loads((source_root / "dataset_description.json").read_text(encoding="utf8"))
    if description.get("DatasetDOI") != OPENNEURO_DOI or description.get("License") != "CC0":
        raise ValueError("dataset_description.json does not match the pinned CC0 OpenNeuro snapshot")


def _load_mne() -> tuple[Any, Any, Any]:
    """Load and version-check the preparation-only neuroimaging dependencies."""
    try:
        import mne
        import mne_bids
        import nibabel
        import scipy
        import scipy.spatial
    except ImportError as exc:
        raise RuntimeError(f"missing preparation dependency; run `{PREPARE_COMMAND}`") from exc

    versions = {
        "mne": mne.__version__,
        "mne_bids": mne_bids.__version__,
        "nibabel": nibabel.__version__,
        "numpy": np.__version__,
        "scipy": scipy.__version__,
    }
    expected = {
        "mne": MNE_VERSION,
        "mne_bids": MNE_BIDS_VERSION,
        "nibabel": NIBABEL_VERSION,
        "numpy": NUMPY_VERSION,
        "scipy": SCIPY_VERSION,
    }
    if versions != expected:
        raise RuntimeError(f"preparation dependency versions must be {expected}, got {versions}; run `{PREPARE_COMMAND}`")
    return mne, mne_bids, scipy


def _spherical_interpolation(
    scipy: Any,
    sphere: np.ndarray,
    source_vertices: np.ndarray,
    source_triangles: np.ndarray,
) -> tuple[np.ndarray, np.ndarray, dict[str, float]]:
    """Map every full-resolution sphere vertex into the oct6 spherical triangulation."""
    sphere_unit = np.asarray(sphere, dtype=np.float64)
    sphere_unit /= np.linalg.norm(sphere_unit, axis=1, keepdims=True)
    source_sphere = sphere_unit[source_vertices]
    triangle_points = source_sphere[source_triangles]
    centroids = triangle_points.mean(axis=1)
    centroids /= np.linalg.norm(centroids, axis=1, keepdims=True)
    tree = scipy.spatial.cKDTree(centroids)

    render_count = sphere_unit.shape[0]
    triangle_owner = np.empty(render_count, dtype=np.int64)
    weights = np.empty((render_count, 3), dtype=np.float64)
    worst_min_weight = np.inf
    batch_size = 16384
    for start in range(0, render_count, batch_size):
        stop = min(start + batch_size, render_count)
        points = sphere_unit[start:stop]
        _, candidates = tree.query(points, k=INTERPOLATION_NEIGHBORS, workers=1)
        candidate_points = triangle_points[candidates]
        a = candidate_points[:, :, 0]
        edge0 = candidate_points[:, :, 1] - a
        edge1 = candidate_points[:, :, 2] - a
        normals = np.cross(edge0, edge1)
        numerator = np.einsum("bkj,bkj->bk", normals, a)
        denominator = np.einsum("bkj,bj->bk", normals, points)
        projected = points[:, None, :] * (numerator / denominator)[:, :, None]
        edge2 = projected - a
        dot00 = np.einsum("bkj,bkj->bk", edge0, edge0)
        dot01 = np.einsum("bkj,bkj->bk", edge0, edge1)
        dot11 = np.einsum("bkj,bkj->bk", edge1, edge1)
        dot20 = np.einsum("bkj,bkj->bk", edge2, edge0)
        dot21 = np.einsum("bkj,bkj->bk", edge2, edge1)
        inverse = 1.0 / (dot00 * dot11 - dot01 * dot01)
        weight1 = (dot11 * dot20 - dot01 * dot21) * inverse
        weight2 = (dot00 * dot21 - dot01 * dot20) * inverse
        candidate_weights = np.stack((1.0 - weight1 - weight2, weight1, weight2), axis=2)
        scores = candidate_weights.min(axis=2)
        choice = np.argmax(scores, axis=1)
        rows = np.arange(stop - start)
        triangle_owner[start:stop] = candidates[rows, choice]
        weights[start:stop] = candidate_weights[rows, choice]
        worst_min_weight = min(worst_min_weight, float(scores[rows, choice].min()))

    if worst_min_weight < -INTERPOLATION_WEIGHT_TOLERANCE:
        raise ValueError(f"spherical interpolation left a triangle by {worst_min_weight:g}")
    weights = np.clip(weights, 0.0, None)
    weights /= weights.sum(axis=1, keepdims=True)
    indices = source_triangles[triangle_owner]

    # Make source-grid identity exact rather than merely numerically close at triangle corners.
    source_local = np.arange(source_vertices.size, dtype=np.int64)
    indices[source_vertices] = source_local[:, None]
    weights[source_vertices] = (1.0, 0.0, 0.0)

    reconstructed = np.einsum("vij,vi->vj", source_sphere[indices], weights)
    reconstructed /= np.linalg.norm(reconstructed, axis=1, keepdims=True)
    cosine = np.clip(np.einsum("ij,ij->i", reconstructed, sphere_unit), -1.0, 1.0)
    angular_error = np.arccos(cosine)
    max_angle = float(angular_error.max())
    if max_angle > INTERPOLATION_ANGLE_TOLERANCE_RAD:
        raise ValueError(f"spherical interpolation angular error {max_angle:g} rad")
    if not np.array_equal(indices[source_vertices, 0], source_local) or not np.all(
        weights[source_vertices, 0] == 1.0
    ):
        raise ValueError("source vertices do not reproduce identity interpolation")
    return (
        indices.astype(np.uint32),
        weights.astype(np.float32),
        {
            "minimum_unclipped_weight": worst_min_weight,
            "maximum_angular_error_rad": max_angle,
        },
    )


def _surface_bundle(mne: Any, scipy: Any, source_root: Path, src: Any) -> dict[str, Any]:
    """Build full-resolution render surfaces and an oct6 scientific sampling domain."""
    render_inflated_parts: list[np.ndarray] = []
    render_pial_parts: list[np.ndarray] = []
    render_index_parts: list[np.ndarray] = []
    source_index_parts: list[np.ndarray] = []
    interpolation_index_parts: list[np.ndarray] = []
    interpolation_weight_parts: list[np.ndarray] = []
    source_render_vertex_parts: list[np.ndarray] = []
    render_hemispheres: list[dict[str, int]] = []
    source_hemispheres: list[dict[str, int]] = []
    sampling: list[tuple[np.ndarray, np.ndarray]] = []
    interpolation_validation: list[dict[str, float]] = []
    render_vertex_offset = 0
    render_index_offset = 0
    source_vertex_offset = 0
    source_index_offset = 0

    for hemi_index, hemi in enumerate(("lh", "rh")):
        surface_root = source_root / "derivatives/freesurfer/subjects/sub-01/surf"
        inflated, inflated_triangles = mne.read_surface(
            surface_root / f"{hemi}.inflated", verbose="ERROR"
        )
        pial, pial_triangles = mne.read_surface(surface_root / f"{hemi}.pial", verbose="ERROR")
        sphere, sphere_triangles = mne.read_surface(
            surface_root / f"{hemi}.sphere", verbose="ERROR"
        )
        if not (
            np.array_equal(inflated_triangles, pial_triangles)
            and np.array_equal(inflated_triangles, sphere_triangles)
        ):
            raise ValueError(f"{hemi} FreeSurfer surface topology differs across geometries")

        source_vertices = np.asarray(src[hemi_index]["vertno"], dtype=np.int64)
        full_to_source = np.full(inflated.shape[0], -1, dtype=np.int64)
        full_to_source[source_vertices] = np.arange(source_vertices.size, dtype=np.int64)
        source_triangles = full_to_source[np.asarray(src[hemi_index]["use_tris"], dtype=np.int64)]
        source_triangles = source_triangles[np.all(source_triangles >= 0, axis=1)]
        if source_triangles.size == 0 or int(source_triangles.max()) >= source_vertices.size:
            raise ValueError(f"could not derive compact {hemi} oct6 triangles")

        interpolation_indices, interpolation_weights, validation = _spherical_interpolation(
            scipy, sphere, source_vertices, source_triangles
        )
        interpolation_indices += source_vertex_offset
        interpolation_validation.append(validation)

        render_inflated_parts.append(np.asarray(inflated, dtype=np.float64))
        render_pial_parts.append(np.asarray(pial, dtype=np.float64))
        render_index_parts.append(
            np.asarray(inflated_triangles, dtype=np.uint32) + render_vertex_offset
        )
        source_index_parts.append(source_triangles.astype(np.uint32) + source_vertex_offset)
        interpolation_index_parts.append(interpolation_indices)
        interpolation_weight_parts.append(interpolation_weights)
        source_render_vertex_parts.append(
            source_vertices.astype(np.uint32) + render_vertex_offset
        )
        sampling.append((source_vertices, source_triangles))
        render_hemispheres.append(
            {
                "vertex_offset": render_vertex_offset,
                "vertex_count": int(inflated.shape[0]),
                "index_offset": render_index_offset,
                "index_count": int(inflated_triangles.size),
            }
        )
        source_hemispheres.append(
            {
                "vertex_offset": source_vertex_offset,
                "vertex_count": int(source_vertices.size),
                "index_offset": source_index_offset,
                "index_count": int(source_triangles.size),
            }
        )
        render_vertex_offset += int(inflated.shape[0])
        render_index_offset += int(inflated_triangles.size)
        source_vertex_offset += int(source_vertices.size)
        source_index_offset += int(source_triangles.size)

    inflated = np.concatenate(render_inflated_parts, axis=0)
    pial = np.concatenate(render_pial_parts, axis=0)
    reference = np.concatenate((inflated, pial), axis=0)
    center = 0.5 * (reference.min(axis=0) + reference.max(axis=0))
    scale = DISPLAY_EXTENT / float(np.max(np.ptp(reference, axis=0)))
    return {
        "inflated": ((inflated - center) * scale).astype(np.float32),
        "pial": ((pial - center) * scale).astype(np.float32),
        "render_indices": np.concatenate(render_index_parts).reshape(-1).astype(np.uint32),
        "source_indices": np.concatenate(source_index_parts).reshape(-1).astype(np.uint32),
        "interpolation_indices": np.concatenate(interpolation_index_parts).astype(np.uint32),
        "interpolation_weights": np.concatenate(interpolation_weight_parts).astype(np.float32),
        "source_render_vertices": np.concatenate(source_render_vertex_parts).astype(np.uint32),
        "render_hemispheres": render_hemispheres,
        "source_hemispheres": source_hemispheres,
        "sampling": sampling,
        "interpolation_validation": interpolation_validation,
    }


def _expand_activity(stc: Any, sampling: list[tuple[np.ndarray, np.ndarray]]) -> np.ndarray:
    """Fill the complete oct6 mesh and apply short graph-neighbor display smoothing."""
    from scipy import sparse

    output: list[np.ndarray] = []
    for hemi_index, (vertices, triangles) in enumerate(sampling):
        stc_vertices = np.asarray(stc.vertices[hemi_index], dtype=np.int64)
        full_to_local = {int(vertex): i for i, vertex in enumerate(vertices)}
        known_local = np.asarray(
            [full_to_local[int(vertex)] for vertex in stc_vertices], dtype=np.int64
        )
        vertex_count = vertices.size

        adjacency: list[list[int]] = [[] for _ in range(vertex_count)]
        edge_pairs = np.concatenate(
            (triangles[:, [0, 1]], triangles[:, [1, 2]], triangles[:, [2, 0]]), axis=0
        )
        edge_pairs = np.concatenate((edge_pairs, edge_pairs[:, ::-1]), axis=0)
        edge_pairs = np.unique(edge_pairs, axis=0)
        for source, target in edge_pairs:
            adjacency[int(source)].append(int(target))

        owner = np.full(vertex_count, -1, dtype=np.int64)
        owner[known_local] = np.arange(known_local.size, dtype=np.int64)
        queue: deque[int] = deque(int(index) for index in known_local)
        while queue:
            source = queue.popleft()
            for target in adjacency[source]:
                if owner[target] >= 0:
                    continue
                owner[target] = owner[source]
                queue.append(target)
        if np.any(owner < 0):
            raise ValueError(f"disconnected vertices remain in hemisphere {hemi_index}")

        start = 0 if hemi_index == 0 else stc.vertices[0].size
        stop = start + stc_vertices.size
        values = np.asarray(stc.data[start:stop], dtype=np.float64).T[:, owner]
        rows = edge_pairs[:, 0]
        cols = edge_pairs[:, 1]
        graph = sparse.csr_matrix(
            (np.ones(rows.size, dtype=np.float64), (rows, cols)),
            shape=(vertex_count, vertex_count),
        )
        degree = np.asarray(graph.sum(axis=1)).reshape(-1)
        for _ in range(SMOOTHING_STEPS):
            values = (values + graph.dot(values.T).T) / (degree[None, :] + 1.0)
        output.append(values.astype(np.float32))
    return np.concatenate(output, axis=1)


def _write_binary(
    path: Path,
    times_ms: np.ndarray,
    inflated: np.ndarray,
    pial: np.ndarray,
    render_indices: np.ndarray,
    source_indices: np.ndarray,
    interpolation_indices: np.ndarray,
    interpolation_weights: np.ndarray,
    source_render_vertices: np.ndarray,
    values: np.ndarray,
    render_hemispheres: list[dict[str, int]],
    source_hemispheres: list[dict[str, int]],
) -> None:
    """Write the v3 full-render-mesh/scientific-source-grid runtime bundle."""
    render_vertex_count = inflated.shape[0]
    source_vertex_count = source_render_vertices.size
    time_count = times_ms.size
    if values.shape != (time_count, source_vertex_count):
        raise ValueError(
            f"activity shape {values.shape} does not match {(time_count, source_vertex_count)}"
        )
    if interpolation_indices.shape != (render_vertex_count, 3):
        raise ValueError("interpolation indices must have three entries per render vertex")
    if interpolation_weights.shape != interpolation_indices.shape:
        raise ValueError("interpolation weights do not match interpolation indices")
    if len(render_hemispheres) != 2 or len(source_hemispheres) != 2:
        raise ValueError("exactly two hemispheres are required")

    header = struct.pack(
        BINARY_HEADER_FORMAT,
        BINARY_MAGIC,
        BUNDLE_VERSION,
        BINARY_HEADER_SIZE,
        2,
        time_count,
        source_vertex_count,
        source_indices.size,
        render_vertex_count,
        render_indices.size,
        3,
        3,
        0,
        float(times_ms[0]),
        float(times_ms[1] - times_ms[0]),
        *DISPLAY_LIMITS,
        source_hemispheres[0]["vertex_count"],
        source_hemispheres[0]["index_count"],
        source_hemispheres[1]["vertex_count"],
        source_hemispheres[1]["index_count"],
        render_hemispheres[0]["vertex_count"],
        render_hemispheres[0]["index_count"],
        render_hemispheres[1]["vertex_count"],
        render_hemispheres[1]["index_count"],
        0,
        0,
    )
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wb") as stream:
        stream.write(header)
        stream.write(np.asarray(times_ms, dtype="<f4").tobytes(order="C"))
        stream.write(np.asarray(inflated, dtype="<f4").tobytes(order="C"))
        stream.write(np.asarray(pial, dtype="<f4").tobytes(order="C"))
        stream.write(np.asarray(render_indices, dtype="<u4").tobytes(order="C"))
        stream.write(np.asarray(source_indices, dtype="<u4").tobytes(order="C"))
        stream.write(np.asarray(interpolation_indices, dtype="<u4").tobytes(order="C"))
        stream.write(np.asarray(interpolation_weights, dtype="<f4").tobytes(order="C"))
        stream.write(np.asarray(source_render_vertices, dtype="<u4").tobytes(order="C"))
        stream.write(np.asarray(values, dtype="<f4").tobytes(order="C"))


def _compute(source_root: Path) -> dict[str, Any]:
    """Compute the auditory evoked dSPM estimate and matching display surface mesh."""
    mne, mne_bids, scipy = _load_mne()
    mne.set_log_level("WARNING")
    subjects_dir = source_root / "derivatives/freesurfer/subjects"

    bids_path = mne_bids.BIDSPath(
        root=source_root,
        subject="01",
        task="audiovisual",
        run="01",
        datatype="meg",
        suffix="meg",
        extension=".fif",
    )
    t1_bids_path = mne_bids.BIDSPath(
        root=source_root,
        subject="01",
        datatype="anat",
        suffix="T1w",
        extension=".nii.gz",
    )
    raw = mne_bids.read_raw_bids(bids_path, extra_params={"allow_maxshield": "yes"}, verbose="ERROR")
    raw.load_data()
    events, event_id = mne.events_from_annotations(raw, verbose="ERROR")
    if CONDITION not in event_id:
        raise ValueError(f"condition {CONDITION!r} is absent; available conditions: {sorted(event_id)}")

    raw.filter(HIGHPASS_HZ, LOWPASS_HZ, picks="meg", n_jobs=1, verbose="ERROR")
    picks = mne.pick_types(raw.info, meg=True, eog=True, stim=False, exclude=[])
    epochs = mne.Epochs(
        raw,
        events,
        event_id={CONDITION: event_id[CONDITION]},
        tmin=TMIN_S,
        tmax=TMAX_S,
        baseline=(None, 0.0),
        picks=picks,
        reject={"grad": 4000e-13, "mag": 4e-12, "eog": 150e-6},
        proj=True,
        preload=True,
        verbose="ERROR",
    )
    if len(epochs) < 20:
        raise ValueError(f"too few accepted {CONDITION} epochs: {len(epochs)}")
    noise_cov = mne.compute_covariance(
        epochs, tmax=0.0, method="empirical", rank="info", verbose="ERROR"
    )
    evoked = epochs.average().pick("meg")

    trans = mne_bids.get_head_mri_trans(
        bids_path,
        t1_bids_path=t1_bids_path,
        fs_subject="sub-01",
        fs_subjects_dir=subjects_dir,
        verbose="ERROR",
    )
    src = mne.setup_source_space(
        "sub-01", spacing=SOURCE_SPACING, subjects_dir=subjects_dir, add_dist=False, verbose="ERROR"
    )
    bem_model = mne.make_bem_model(
        "sub-01", ico=4, conductivity=(0.3,), subjects_dir=subjects_dir, verbose="ERROR"
    )
    bem = mne.make_bem_solution(bem_model, verbose="ERROR")
    forward = mne.make_forward_solution(
        evoked.info,
        trans=trans,
        src=src,
        bem=bem,
        meg=True,
        eeg=False,
        mindist=5.0,
        n_jobs=1,
        verbose="ERROR",
    )
    inverse = mne.minimum_norm.make_inverse_operator(
        evoked.info, forward, noise_cov, loose=0.2, depth=0.8, verbose="ERROR"
    )
    stc = mne.minimum_norm.apply_inverse(
        evoked,
        inverse,
        lambda2=LAMBDA2,
        method="dSPM",
        pick_ori=None,
        verbose="ERROR",
    )
    stc.crop(OUTPUT_TMIN_S, OUTPUT_TMAX_S)
    surfaces = _surface_bundle(mne, scipy, source_root, src)
    values = _expand_activity(stc, surfaces["sampling"])
    if not np.all(np.isfinite(values)) or float(values.max()) < DISPLAY_LIMITS[1]:
        raise ValueError(f"unexpected dSPM activity range: {float(values.min())} .. {float(values.max())}")
    return {
        "times_ms": np.asarray(stc.times * 1000.0, dtype=np.float32),
        **surfaces,
        "values": values,
        "accepted_epochs": len(epochs),
        "available_events": event_id,
        "peak_dspm": float(values.max()),
        "peak_time_ms": float(stc.times[np.unravel_index(np.argmax(stc.data), stc.data.shape)[1]] * 1000.0),
        "head_mri_transform": np.asarray(trans["trans"]).tolist(),
        "versions": {
            "mne": mne.__version__,
            "mne_bids": mne_bids.__version__,
            "numpy": np.__version__,
            "scipy": scipy.__version__,
        },
    }


def prepare(force_download: bool, download_only: bool) -> Path:
    """Prepare the cortical-activity bundle in the local ignored cache."""
    bundle_root = CACHE_ROOT / EXAMPLE_ID
    source_root = bundle_root / "source"
    prepared = bundle_root / "prepared"
    source_root.mkdir(parents=True, exist_ok=True)
    prepared.mkdir(parents=True, exist_ok=True)
    _download_sources(source_root, force_download)
    if download_only:
        return bundle_root

    result = _compute(source_root)
    binary_path = prepared / "cortical_activity.bin"
    metadata_path = prepared / "cortical_activity.json"
    _write_binary(
        binary_path,
        result["times_ms"],
        result["inflated"],
        result["pial"],
        result["render_indices"],
        result["source_indices"],
        result["interpolation_indices"],
        result["interpolation_weights"],
        result["source_render_vertices"],
        result["values"],
        result["render_hemispheres"],
        result["source_hemispheres"],
    )

    metadata = {
        "schema": "datoviz.cortical-activity.v3",
        "dataset": {
            "accession": OPENNEURO_DATASET,
            "snapshot": OPENNEURO_SNAPSHOT,
            "doi": OPENNEURO_DOI,
            "license": "CC0",
            "participant": "sub-01",
        },
        "analysis": {
            "condition": CONDITION,
            "modality": "MEG",
            "estimate": "dSPM",
            "orientation": "free-orientation magnitude",
            "epoch_seconds": [TMIN_S, TMAX_S],
            "output_seconds": [OUTPUT_TMIN_S, OUTPUT_TMAX_S],
            "filter_hz": [HIGHPASS_HZ, LOWPASS_HZ],
            "baseline_seconds": [TMIN_S, 0.0],
            "source_spacing": SOURCE_SPACING,
            "display_smoothing_steps": SMOOTHING_STEPS,
            "snr": SNR,
            "lambda2": LAMBDA2,
            "accepted_epochs": result["accepted_epochs"],
            "display_limits_dspm": list(DISPLAY_LIMITS),
            "peak_dspm": result["peak_dspm"],
            "peak_time_ms": result["peak_time_ms"],
        },
        "render_mesh": {
            "surfaces": ["pial", "inflated"],
            "coordinate_space": "participant-native FreeSurfer surface RAS",
            "normalization": "one shared translation and uniform scale across axes and surfaces",
            "display_extent": DISPLAY_EXTENT,
            "hemispheres": result["render_hemispheres"],
            "vertex_count": int(result["inflated"].shape[0]),
            "triangle_count": int(result["render_indices"].size // 3),
        },
        "scientific_mesh": {
            "spacing": SOURCE_SPACING,
            "hemispheres": result["source_hemispheres"],
            "vertex_count": int(result["values"].shape[1]),
            "triangle_count": int(result["source_indices"].size // 3),
        },
        "interpolation": {
            "domain": "FreeSurfer spherical surface",
            "method": "containing oct6 spherical triangle with barycentric weights",
            "components": 3,
            "candidate_triangles": INTERPOLATION_NEIGHBORS,
            "validation": result["interpolation_validation"],
        },
        "time_ms": result["times_ms"].tolist(),
        "head_mri_transform": result["head_mri_transform"],
        "versions": result["versions"],
        "source_files": [source.__dict__ for source in SOURCE_FILES],
    }
    metadata_path.write_text(json.dumps(metadata, indent=2, sort_keys=True) + "\n", encoding="utf8")

    artifacts = [
        artifact(
            binary_path,
            bundle_root,
            "cortical_activity_binary",
            "bin",
            version=BUNDLE_VERSION,
            render_vertex_count=int(result["inflated"].shape[0]),
            render_triangle_count=int(result["render_indices"].size // 3),
            source_vertex_count=int(result["values"].shape[1]),
            source_triangle_count=int(result["source_indices"].size // 3),
            time_count=int(result["times_ms"].size),
            value_dtype="float32",
            estimate="dSPM",
        ),
        artifact(metadata_path, bundle_root, "cortical_activity_metadata", "json"),
    ]
    write_manifest(
        bundle_root,
        example_id=EXAMPLE_ID,
        title="Human Auditory Cortical Activity",
        status="local-cache",
        script=relpath(Path(__file__), ROOT),
        command=command_argv(relpath(Path(__file__), ROOT)),
        source={
            "name": "MNE audiovisual sample data",
            "repository": "OpenNeuro",
            "accession": OPENNEURO_DATASET,
            "snapshot": OPENNEURO_SNAPSHOT,
            "doi": OPENNEURO_DOI,
            "license": "CC0",
            "participant": "sub-01",
        },
        artifacts=artifacts,
        validation={
            "condition": CONDITION,
            "accepted_epochs": result["accepted_epochs"],
            "peak_dspm": result["peak_dspm"],
            "peak_time_ms": result["peak_time_ms"],
            "display_limits_dspm": list(DISPLAY_LIMITS),
        },
    )
    write_provenance(
        bundle_root,
        title="Human Auditory Cortical Activity",
        source_lines=[
            f"OpenNeuro `{OPENNEURO_DATASET}` snapshot `{OPENNEURO_SNAPSHOT}` ({OPENNEURO_DOI}).",
            "Participant `sub-01`; simultaneous audiovisual MEG/EEG experiment; this derivative uses MEG only.",
            "Every downloaded source file is checked against a pinned byte count and MD5 or SHA-256 digest.",
        ],
        processing_lines=[
            f"Filtered MEG channels from {HIGHPASS_HZ:g} to {LOWPASS_HZ:g} Hz.",
            f"Epoched `{CONDITION}` trials from {TMIN_S:g} to {TMAX_S:g} seconds and baseline-corrected through stimulus onset.",
            "Estimated baseline noise covariance, participant-native BEM/forward solution, and a loose-orientation minimum-norm inverse.",
            f"Applied dSPM with SNR={SNR:g} and lambda2={LAMBDA2:.9f}; exported {OUTPUT_TMIN_S:g} to {OUTPUT_TMAX_S:g} seconds.",
            f"Sampled activity on the `{SOURCE_SPACING}` source mesh and preserved both inflated and pial coordinates.",
            "Kept the complete FreeSurfer pial/inflated meshes as the independent render domain.",
            "Mapped every render vertex to a hemisphere-local oct6 spherical triangle with three barycentric weights.",
            "Centered both hemispheres together and applied one shared uniform scale, preserving the anatomical aspect ratio.",
            f"Filled inverse-excluded mesh vertices by nearest graph propagation and applied {SMOOTHING_STEPS} neighbor-averaging display steps.",
        ],
        license_lines=[
            "The pinned OpenNeuro dataset declares CC0.",
            "Acknowledge Alexandre Gramfort and Matti Hämäläinen and cite the MNE publications requested by the dataset metadata.",
        ],
        notes=[
            "dSPM is a model-derived, dimensionless, noise-normalized source estimate; it is not a direct measurement of neuronal firing.",
            "This script writes only to `.cache/datoviz/examples/cortical_activity` and does not modify the `data` submodule.",
        ],
    )
    print(f"wrote {bundle_root}")
    print(
        f"activity: {result['values'].shape[1]} source vertices, "
        f"{result['inflated'].shape[0]} render vertices, "
        f"{result['render_indices'].size // 3} render triangles, {result['times_ms'].size} frames, "
        f"peak {result['peak_dspm']:.2f} dSPM at {result['peak_time_ms']:.1f} ms"
    )
    return bundle_root


def main() -> None:
    """Run the preparation script."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--force-download", action="store_true", help="redownload pinned source inputs")
    parser.add_argument("--download-only", action="store_true", help="download and verify sources without computing the derivative")
    parser.add_argument("--clear", action="store_true", help="remove the local bundle before preparation")
    args = parser.parse_args()
    bundle_root = CACHE_ROOT / EXAMPLE_ID
    if args.clear and bundle_root.exists():
        shutil.rmtree(bundle_root)
    prepare(args.force_download, args.download_only)


if __name__ == "__main__":
    main()
