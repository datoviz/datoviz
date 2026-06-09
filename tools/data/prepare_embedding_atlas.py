#!/usr/bin/env python3
"""Prepare a compact synthetic embedding-atlas bundle for the C showcase."""

from __future__ import annotations

import argparse
import json
import shutil
import sys
from pathlib import Path

import numpy as np

from common import CACHE_ROOT, artifact, command_argv, relpath, write_manifest, write_provenance


ROOT = Path(__file__).resolve().parents[2]
EXAMPLE_ID = "embedding_atlas"

CLUSTER_NAMES = ["syntax", "vision", "audio", "planning", "retrieval", "control"]
PALETTE = np.array(
    [
        [76, 201, 240, 226],
        [128, 255, 219, 226],
        [255, 183, 3, 226],
        [239, 71, 111, 226],
        [222, 226, 230, 210],
        [141, 153, 174, 218],
    ],
    dtype=np.uint8,
)


def _cluster_points(
    rng: np.random.Generator, cluster: int, count: int
) -> tuple[np.ndarray, np.ndarray]:
    """Return clustered 2D embedding coordinates and local scores."""
    theta = np.linspace(0.0, 2.0 * np.pi, len(CLUSTER_NAMES), endpoint=False, dtype=np.float32)
    center = np.array([np.cos(theta[cluster]), np.sin(theta[cluster])], dtype=np.float32)
    center *= np.array([0.82, 0.62], dtype=np.float32)

    angle = float(theta[cluster]) + 0.45
    ca = np.cos(angle)
    sa = np.sin(angle)
    rot = np.array([[ca, -sa], [sa, ca]], dtype=np.float32)
    scale = np.array([0.18 + 0.03 * (cluster % 3), 0.055 + 0.01 * (cluster % 2)], dtype=np.float32)
    local = rng.normal(0.0, 1.0, (count, 2)).astype(np.float32)
    coords = center + (local * scale) @ rot.T

    curl = 0.040 * np.column_stack(
        [
            np.sin(5.1 * coords[:, 1] + 0.7 * cluster),
            np.cos(4.6 * coords[:, 0] - 0.5 * cluster),
        ]
    ).astype(np.float32)
    coords += curl
    scores = np.exp(-0.5 * np.sum(local * local, axis=1)).astype(np.float32)
    return coords, scores


def _normalize_xy(xy: np.ndarray) -> np.ndarray:
    """Normalize embedding coordinates into a comfortable panel domain."""
    center = np.median(xy, axis=0)
    spread = np.percentile(np.abs(xy - center), 99.4)
    return ((xy - center) / max(float(spread), 1e-6)).astype(np.float32)


def _write_arrays(prepared: Path, xy: np.ndarray, cluster: np.ndarray, color: np.ndarray) -> None:
    """Write typed render arrays in the v0.4 embedding-atlas cache layout."""
    prepared.mkdir(parents=True, exist_ok=True)
    (prepared / "xy.f32").write_bytes(np.ascontiguousarray(xy, dtype="<f4").tobytes())
    (prepared / "cluster.u16").write_bytes(np.ascontiguousarray(cluster, dtype="<u2").tobytes())
    (prepared / "color.rgba8").write_bytes(np.ascontiguousarray(color, dtype=np.uint8).tobytes())


def _write_metadata(path: Path, cluster: np.ndarray, scores: np.ndarray) -> None:
    """Write one compact JSON object per embedding item."""
    with path.open("w", encoding="utf8") as f:
        for idx, cid in enumerate(cluster.tolist()):
            payload = {
                "id": idx,
                "cluster": int(cid),
                "label": CLUSTER_NAMES[int(cid)],
                "score": round(float(scores[idx]), 5),
            }
            f.write(json.dumps(payload, sort_keys=True, separators=(",", ":")) + "\n")


def _write_neighbors(path: Path, cluster: np.ndarray) -> np.ndarray:
    """Write simple within-cluster neighbor rings for optional follow-up visuals."""
    neighbors = np.empty((cluster.shape[0], 4), dtype=np.uint32)
    for cid in range(len(CLUSTER_NAMES)):
        idx = np.flatnonzero(cluster == cid).astype(np.uint32)
        if idx.size == 0:
            continue
        for offset, item in enumerate(idx.tolist()):
            neighbors[item, 0] = idx[(offset - 1) % idx.size]
            neighbors[item, 1] = idx[(offset + 1) % idx.size]
            neighbors[item, 2] = idx[(offset + 7) % idx.size]
            neighbors[item, 3] = idx[(offset + 17) % idx.size]
    path.write_bytes(np.ascontiguousarray(neighbors, dtype="<u4").tobytes())
    return neighbors


def prepare(args: argparse.Namespace) -> None:
    """Prepare the embedding-atlas cache bundle."""
    bundle_root = CACHE_ROOT / EXAMPLE_ID
    prepared = bundle_root / "prepared"
    if args.force and prepared.exists():
        shutil.rmtree(prepared)
    prepared.mkdir(parents=True, exist_ok=True)

    rng = np.random.default_rng(args.seed)
    base = args.count // len(CLUSTER_NAMES)
    remainder = args.count % len(CLUSTER_NAMES)
    xy_parts: list[np.ndarray] = []
    score_parts: list[np.ndarray] = []
    cluster_parts: list[np.ndarray] = []
    for cid in range(len(CLUSTER_NAMES)):
        n = base + (1 if cid < remainder else 0)
        coords, scores = _cluster_points(rng, cid, n)
        xy_parts.append(coords)
        score_parts.append(scores)
        cluster_parts.append(np.full(n, cid, dtype=np.uint16))

    xy = _normalize_xy(np.vstack(xy_parts))
    scores = np.concatenate(score_parts).astype(np.float32)
    cluster = np.concatenate(cluster_parts).astype(np.uint16)

    order = rng.permutation(xy.shape[0])
    xy = xy[order]
    scores = scores[order]
    cluster = cluster[order]

    color = PALETTE[cluster].copy()
    alpha = np.clip(170.0 + 70.0 * scores, 0.0, 245.0).astype(np.uint8)
    color[:, 3] = alpha

    _write_arrays(prepared, xy, cluster, color)
    _write_metadata(prepared / "metadata.jsonl", cluster, scores)
    neighbors = _write_neighbors(prepared / "neighbors.u32", cluster)

    artifacts = [
        artifact(
            prepared / "xy.f32",
            bundle_root,
            "embedding-coordinates",
            "f32",
            dtype="float32",
            shape=[int(xy.shape[0]), 2],
            columns=["x", "y"],
        ),
        artifact(
            prepared / "cluster.u16",
            bundle_root,
            "cluster-id",
            "u16",
            dtype="uint16",
            shape=[int(cluster.shape[0])],
        ),
        artifact(
            prepared / "color.rgba8",
            bundle_root,
            "direct-color",
            "rgba8",
            dtype="uint8",
            shape=[int(color.shape[0]), 4],
        ),
        artifact(prepared / "metadata.jsonl", bundle_root, "metadata", "jsonl"),
        artifact(
            prepared / "neighbors.u32",
            bundle_root,
            "nearest-neighbor-rings",
            "u32",
            dtype="uint32",
            shape=[int(neighbors.shape[0]), 4],
        ),
    ]

    write_manifest(
        bundle_root,
        example_id=EXAMPLE_ID,
        title="Synthetic Embedding Atlas",
        status="cache-only",
        script=relpath(Path(__file__), ROOT),
        command=command_argv(relpath(Path(__file__), ROOT), sys.argv[1:]),
        source={
            "kind": "synthetic",
            "license": "Generated deterministically by the Datoviz preparation script.",
        },
        artifacts=artifacts,
        validation={
            "point_count": int(xy.shape[0]),
            "cluster_count": len(CLUSTER_NAMES),
            "cluster_names": CLUSTER_NAMES,
            "seed": args.seed,
            "bounds": {"min": xy.min(axis=0).astype(float).tolist(),
                       "max": xy.max(axis=0).astype(float).tolist()},
        },
        extra={"notes": ["Prepared under .cache; not committed to the data submodule."]},
    )
    write_provenance(
        bundle_root,
        title="Synthetic Embedding Atlas",
        source_lines=[
            "Deterministic synthetic embedding generated locally; no external source dataset.",
            f"Cluster labels: `{', '.join(CLUSTER_NAMES)}`.",
        ],
        processing_lines=[
            f"Generated `{xy.shape[0]}` clustered 2D points with NumPy seed `{args.seed}`.",
            "Wrote separate `xy.f32`, `cluster.u16`, `color.rgba8`, `metadata.jsonl`, and `neighbors.u32` artifacts.",
            "Normalized coordinates into the showcase panzoom domain.",
        ],
        license_lines=[
            "Generated data may be redistributed with the repository under the project license.",
        ],
    )
    print(f"wrote {relpath(bundle_root, ROOT)} ({xy.shape[0]} points)")


def main() -> int:
    """Run the embedding-atlas preparation command."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--force", action="store_true", help="replace prepared outputs")
    parser.add_argument("--count", type=int, default=4096, help="number of embedding points")
    parser.add_argument("--seed", type=int, default=20260609, help="deterministic RNG seed")
    args = parser.parse_args()
    if args.count < len(CLUSTER_NAMES):
        parser.error("--count must be at least the cluster count")
    if args.count > 200000:
        parser.error("--count is too large for the default showcase bundle")
    prepare(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
