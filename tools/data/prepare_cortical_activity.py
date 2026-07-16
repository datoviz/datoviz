#!/usr/bin/env python3
"""Prepare a compact human cortical dSPM activity bundle from OpenNeuro ds000248."""

from __future__ import annotations

import argparse
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
BUNDLE_VERSION = 1
BINARY_MAGIC = b"DVZCTA1\0"
BINARY_HEADER_FORMAT = "<8s8I5f5I"
BINARY_HEADER_SIZE = struct.calcsize(BINARY_HEADER_FORMAT)

OPENNEURO_DATASET = "ds000248"
OPENNEURO_SNAPSHOT = "1.2.4"
OPENNEURO_DOI = "10.18112/openneuro.ds000248.v1.2.4"
OPENNEURO_BASE = f"https://s3.amazonaws.com/openneuro.org/{OPENNEURO_DATASET}"

MNE_VERSION = "1.12.1"
MNE_BIDS_VERSION = "0.19.0"
NIBABEL_VERSION = "5.4.2"
PREPARE_COMMAND = (
    "uv run --isolated --with mne==1.12.1 --with mne-bids==0.19.0 "
    "--with nibabel==5.4.2 --with requests --with numpy "
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
SNR = 3.0
LAMBDA2 = 1.0 / SNR**2
# Match the established MNE audiovisual sample display convention. Values below fmin remain
# neutral cortex; opacity rises through fmid and the sequential ramp saturates at fmax.
DISPLAY_LIMITS = (8.0, 12.0, 15.0)
HEMISPHERE_GAP_MM = 8.0


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


def _load_mne() -> tuple[Any, Any]:
    """Load and version-check the preparation-only neuroimaging dependencies."""
    try:
        import mne
        import mne_bids
        import nibabel
    except ImportError as exc:
        raise RuntimeError(f"missing preparation dependency; run `{PREPARE_COMMAND}`") from exc

    versions = {
        "mne": mne.__version__,
        "mne_bids": mne_bids.__version__,
        "nibabel": nibabel.__version__,
    }
    expected = {
        "mne": MNE_VERSION,
        "mne_bids": MNE_BIDS_VERSION,
        "nibabel": NIBABEL_VERSION,
    }
    if versions != expected:
        raise RuntimeError(f"preparation dependency versions must be {expected}, got {versions}; run `{PREPARE_COMMAND}`")
    return mne, mne_bids


def _vertex_normals(positions: np.ndarray, triangles: np.ndarray) -> np.ndarray:
    """Compute normalized area-weighted vertex normals."""
    normals = np.zeros_like(positions, dtype=np.float64)
    face_normals = np.cross(
        positions[triangles[:, 1]] - positions[triangles[:, 0]],
        positions[triangles[:, 2]] - positions[triangles[:, 0]],
    )
    for corner in range(3):
        np.add.at(normals, triangles[:, corner], face_normals)
    lengths = np.linalg.norm(normals, axis=1)
    valid = lengths > 0
    normals[valid] /= lengths[valid, None]
    return normals.astype(np.float32)


def _source_mesh(
    mne: Any, source_root: Path, src: Any, stc: Any
) -> tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray, list[dict[str, int]]]:
    """Build a compact two-hemisphere mesh matching the SourceEstimate vertex order."""
    inflated_parts: list[np.ndarray] = []
    pial_parts: list[np.ndarray] = []
    normal_parts: list[np.ndarray] = []
    index_parts: list[np.ndarray] = []
    hemispheres: list[dict[str, int]] = []
    vertex_offset = 0
    index_offset = 0

    for hemi_index, hemi in enumerate(("lh", "rh")):
        vertices = np.asarray(stc.vertices[hemi_index], dtype=np.int64)
        inflated_full, _ = mne.read_surface(
            source_root / f"derivatives/freesurfer/subjects/sub-01/surf/{hemi}.inflated",
            verbose="ERROR",
        )
        pial_full, _ = mne.read_surface(
            source_root / f"derivatives/freesurfer/subjects/sub-01/surf/{hemi}.pial",
            verbose="ERROR",
        )
        full_to_local = np.full(inflated_full.shape[0], -1, dtype=np.int64)
        full_to_local[vertices] = np.arange(vertices.size, dtype=np.int64)
        full_triangles = np.asarray(src[hemi_index]["use_tris"], dtype=np.int64)
        local_triangles = full_to_local[full_triangles]
        local_triangles = local_triangles[np.all(local_triangles >= 0, axis=1)]
        if local_triangles.size == 0 or int(local_triangles.max()) >= vertices.size:
            raise ValueError(f"could not derive compact {hemi} source-space triangles")

        inflated = np.asarray(inflated_full[vertices], dtype=np.float64)
        pial = np.asarray(pial_full[vertices], dtype=np.float64)
        inflated[:, 0] += -HEMISPHERE_GAP_MM if hemi == "lh" else HEMISPHERE_GAP_MM
        pial[:, 0] += -HEMISPHERE_GAP_MM if hemi == "lh" else HEMISPHERE_GAP_MM
        normals = _vertex_normals(inflated, local_triangles)

        inflated_parts.append(inflated)
        pial_parts.append(pial)
        normal_parts.append(normals)
        index_parts.append(local_triangles.astype(np.uint32) + vertex_offset)
        hemispheres.append(
            {
                "vertex_offset": vertex_offset,
                "vertex_count": int(vertices.size),
                "index_offset": index_offset,
                "index_count": int(local_triangles.size),
            }
        )
        vertex_offset += int(vertices.size)
        index_offset += int(local_triangles.size)

    inflated = np.concatenate(inflated_parts, axis=0)
    pial = np.concatenate(pial_parts, axis=0)
    center = 0.5 * (inflated.min(axis=0) + inflated.max(axis=0))
    scale = float(np.max(inflated.max(axis=0) - inflated.min(axis=0)))
    inflated = (inflated - center) * (1.85 / scale)
    pial = (pial - center) * (1.85 / scale)
    return (
        inflated.astype(np.float32),
        pial.astype(np.float32),
        np.concatenate(normal_parts, axis=0).astype(np.float32),
        np.concatenate(index_parts, axis=0).reshape(-1).astype(np.uint32),
        hemispheres,
    )


def _write_binary(
    path: Path,
    times_ms: np.ndarray,
    inflated: np.ndarray,
    pial: np.ndarray,
    normals: np.ndarray,
    indices: np.ndarray,
    values: np.ndarray,
    hemispheres: list[dict[str, int]],
) -> None:
    """Write the compact little-endian runtime bundle."""
    vertex_count = inflated.shape[0]
    time_count = times_ms.size
    if values.shape != (time_count, vertex_count):
        raise ValueError(f"activity shape {values.shape} does not match {(time_count, vertex_count)}")
    if len(hemispheres) != 2:
        raise ValueError("exactly two hemispheres are required")

    header = struct.pack(
        BINARY_HEADER_FORMAT,
        BINARY_MAGIC,
        BUNDLE_VERSION,
        BINARY_HEADER_SIZE,
        2,
        time_count,
        vertex_count,
        indices.size,
        3,
        1,
        float(times_ms[0]),
        float(times_ms[1] - times_ms[0]),
        *DISPLAY_LIMITS,
        hemispheres[0]["vertex_count"],
        hemispheres[0]["index_count"],
        hemispheres[1]["vertex_count"],
        hemispheres[1]["index_count"],
        0,
    )
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wb") as stream:
        stream.write(header)
        stream.write(np.asarray(times_ms, dtype="<f4").tobytes(order="C"))
        stream.write(np.asarray(inflated, dtype="<f4").tobytes(order="C"))
        stream.write(np.asarray(pial, dtype="<f4").tobytes(order="C"))
        stream.write(np.asarray(normals, dtype="<f4").tobytes(order="C"))
        stream.write(np.asarray(indices, dtype="<u4").tobytes(order="C"))
        stream.write(np.asarray(values, dtype="<f4").tobytes(order="C"))


def _compute(source_root: Path) -> dict[str, Any]:
    """Compute the auditory evoked dSPM estimate and matching compact surface mesh."""
    mne, mne_bids = _load_mne()
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
    values = np.asarray(stc.data.T, dtype=np.float32)
    if not np.all(np.isfinite(values)) or float(values.max()) < DISPLAY_LIMITS[1]:
        raise ValueError(f"unexpected dSPM activity range: {float(values.min())} .. {float(values.max())}")

    inflated, pial, normals, indices, hemispheres = _source_mesh(
        mne, source_root, inverse["src"], stc
    )
    return {
        "times_ms": np.asarray(stc.times * 1000.0, dtype=np.float32),
        "inflated": inflated,
        "pial": pial,
        "normals": normals,
        "indices": indices,
        "values": values,
        "hemispheres": hemispheres,
        "accepted_epochs": len(epochs),
        "available_events": event_id,
        "peak_dspm": float(values.max()),
        "peak_time_ms": float(stc.times[np.unravel_index(np.argmax(stc.data), stc.data.shape)[1]] * 1000.0),
        "head_mri_transform": np.asarray(trans["trans"]).tolist(),
        "versions": {
            "mne": mne.__version__,
            "mne_bids": mne_bids.__version__,
            "numpy": np.__version__,
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
        result["normals"],
        result["indices"],
        result["values"],
        result["hemispheres"],
    )

    metadata = {
        "schema": "datoviz.cortical-activity.v1",
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
            "snr": SNR,
            "lambda2": LAMBDA2,
            "accepted_epochs": result["accepted_epochs"],
            "display_limits_dspm": list(DISPLAY_LIMITS),
            "peak_dspm": result["peak_dspm"],
            "peak_time_ms": result["peak_time_ms"],
        },
        "mesh": {
            "surface": "inflated",
            "alternate_surface": "pial",
            "hemisphere_gap_mm_before_normalization": HEMISPHERE_GAP_MM,
            "hemispheres": result["hemispheres"],
            "vertex_count": int(result["inflated"].shape[0]),
            "triangle_count": int(result["indices"].size // 3),
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
            vertex_count=int(result["inflated"].shape[0]),
            triangle_count=int(result["indices"].size // 3),
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
        f"activity: {result['inflated'].shape[0]} vertices, {result['indices'].size // 3} triangles, "
        f"{result['times_ms'].size} frames, peak {result['peak_dspm']:.2f} dSPM at {result['peak_time_ms']:.1f} ms"
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
