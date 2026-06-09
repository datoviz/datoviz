#!/usr/bin/env python3
"""Prepare a compact render-ready synthetic mouse animation bundle."""

from __future__ import annotations

import argparse
import json
import shutil
import struct
import sys
from pathlib import Path

import numpy as np

from common import CACHE_ROOT, artifact, command_argv, relpath, write_manifest, write_provenance


ROOT = Path(__file__).resolve().parents[2]
EXAMPLE_ID = "synthetic_mouse"

OSF_SOURCE = "https://osf.io/h3ec5/"
MAGIC = b"DVZMSYN\0"
VERSION = 1


def _topology(rings: int, sectors: int) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    """Create a UV sphere-like topology."""
    positions: list[list[float]] = []
    normals: list[list[float]] = []
    uvs: list[list[float]] = []
    for r in range(rings + 1):
        v = r / rings
        phi = -0.5 * np.pi + np.pi * v
        cp = np.cos(phi)
        sp = np.sin(phi)
        for s in range(sectors):
            u = s / sectors
            theta = 2.0 * np.pi * u
            n = np.array([cp * np.cos(theta), cp * np.sin(theta), sp], dtype=np.float32)
            normals.append(n.tolist())
            positions.append(n.tolist())
            uvs.append([u, v])

    indices: list[int] = []
    for r in range(rings):
        for s in range(sectors):
            a = r * sectors + s
            b = r * sectors + ((s + 1) % sectors)
            c = (r + 1) * sectors + s
            d = (r + 1) * sectors + ((s + 1) % sectors)
            indices.extend([a, c, b, b, c, d])
    return (
        np.asarray(positions, dtype=np.float32),
        np.asarray(normals, dtype=np.float32),
        np.asarray(uvs, dtype=np.float32),
        np.asarray(indices, dtype=np.uint32),
    )


def _mouse_frame(base: np.ndarray, frame: int, frames: int) -> tuple[np.ndarray, np.ndarray]:
    """Return one baked vertex-position frame and keypoints."""
    t = frame / frames
    phase = 2.0 * np.pi * t
    gait = np.sin(phase)
    x = base[:, 0]
    y = base[:, 1]
    z = base[:, 2]

    body = np.empty_like(base)
    body[:, 0] = 1.00 * x + 0.10 * np.sin(phase + 2.3 * y) * (1.0 - np.abs(z))
    body[:, 1] = 0.42 * y
    body[:, 2] = 0.30 * z + 0.020 * np.cos(phase + 4.0 * x)
    body[:, 0] += 0.045 * frame
    body[:, 2] += 0.34

    nose = np.array([1.18 + 0.045 * frame, 0.0, 0.38 + 0.025 * gait], dtype=np.float32)
    tail = np.array([-1.10 + 0.045 * frame, 0.02, 0.42 - 0.020 * gait], dtype=np.float32)
    spine = np.array([0.00 + 0.045 * frame, 0.0, 0.54 + 0.018 * np.cos(phase)], dtype=np.float32)
    hip = np.array([-0.52 + 0.045 * frame, 0.0, 0.42], dtype=np.float32)
    paws = np.array(
        [
            [0.55 + 0.045 * frame, -0.34, 0.08 + 0.08 * max(gait, 0.0)],
            [0.55 + 0.045 * frame, +0.34, 0.08 + 0.08 * max(-gait, 0.0)],
            [-0.45 + 0.045 * frame, -0.31, 0.08 + 0.07 * max(-gait, 0.0)],
            [-0.45 + 0.045 * frame, +0.31, 0.08 + 0.07 * max(gait, 0.0)],
        ],
        dtype=np.float32,
    )
    keypoints = np.vstack([nose, spine, hip, tail, paws])
    return body.astype(np.float32), keypoints.astype(np.float32)


def _texture(width: int, height: int) -> np.ndarray:
    """Generate a small fur-like RGBA texture."""
    x = np.linspace(0.0, 1.0, width, dtype=np.float32)
    y = np.linspace(0.0, 1.0, height, dtype=np.float32)
    xx, yy = np.meshgrid(x, y, indexing="xy")
    fur = 0.5 + 0.5 * np.sin(42.0 * xx + 11.0 * np.sin(7.0 * yy))
    saddle = np.exp(-((yy - 0.56) ** 2) / 0.020)
    base = 0.60 + 0.18 * fur - 0.16 * saddle
    tex = np.empty((height, width, 4), dtype=np.uint8)
    tex[..., 0] = np.clip(180 * base, 0, 255).astype(np.uint8)
    tex[..., 1] = np.clip(168 * base, 0, 255).astype(np.uint8)
    tex[..., 2] = np.clip(148 * base, 0, 255).astype(np.uint8)
    tex[..., 3] = 255
    return tex


def _write_binary(path: Path, payload: dict[str, np.ndarray]) -> None:
    """Write the native synthetic mouse binary format."""
    positions = payload["positions"]
    normals = payload["normals"]
    uvs = payload["uvs"]
    indices = payload["indices"]
    keypoints = payload["keypoints"]
    edges = payload["edges"]
    texture = payload["texture"]
    frames, vertices, _ = positions.shape
    keypoint_count = keypoints.shape[1]
    edge_count = edges.shape[0]
    height, width, _ = texture.shape
    header = struct.pack(
        "<8sIIIIIIII",
        MAGIC,
        VERSION,
        vertices,
        indices.shape[0],
        frames,
        keypoint_count,
        edge_count,
        width,
        height,
    )
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wb") as f:
        f.write(header)
        f.write(np.ascontiguousarray(uvs, dtype="<f4").tobytes())
        f.write(np.ascontiguousarray(normals, dtype="<f4").tobytes())
        f.write(np.ascontiguousarray(positions, dtype="<f4").tobytes())
        f.write(np.ascontiguousarray(indices, dtype="<u4").tobytes())
        f.write(np.ascontiguousarray(keypoints, dtype="<f4").tobytes())
        f.write(np.ascontiguousarray(edges, dtype="<u4").tobytes())
        f.write(np.ascontiguousarray(texture, dtype=np.uint8).tobytes())


def prepare(args: argparse.Namespace) -> None:
    """Prepare the synthetic mouse cache bundle."""
    bundle_root = CACHE_ROOT / EXAMPLE_ID
    prepared = bundle_root / "prepared"
    if args.force and prepared.exists():
        shutil.rmtree(prepared)
    prepared.mkdir(parents=True, exist_ok=True)

    base, normals, uvs, indices = _topology(args.rings, args.sectors)
    positions = np.empty((args.frames, base.shape[0], 3), dtype=np.float32)
    keypoints = np.empty((args.frames, 8, 3), dtype=np.float32)
    for frame in range(args.frames):
        positions[frame], keypoints[frame] = _mouse_frame(base, frame, args.frames)
    edges = np.array(
        [[0, 1], [1, 2], [2, 3], [1, 4], [1, 5], [2, 6], [2, 7]], dtype=np.uint32
    )
    texture = _texture(args.texture_size, args.texture_size)

    payload = {
        "positions": positions,
        "normals": normals,
        "uvs": uvs,
        "indices": indices,
        "keypoints": keypoints,
        "edges": edges,
        "texture": texture,
    }
    bin_path = prepared / "synthetic_mouse.bin"
    metadata_path = prepared / "metadata.json"
    _write_binary(bin_path, payload)
    metadata = {
        "frames": args.frames,
        "vertices": int(base.shape[0]),
        "indices": int(indices.shape[0]),
        "keypoints": int(keypoints.shape[1]),
        "skeleton_edges": int(edges.shape[0]),
        "texture_size": [args.texture_size, args.texture_size],
        "source": OSF_SOURCE,
    }
    metadata_path.write_text(json.dumps(metadata, indent=2, sort_keys=True) + "\n", encoding="utf8")

    write_manifest(
        bundle_root,
        example_id=EXAMPLE_ID,
        title="Synthetic Mouse Animation",
        status="cache-only",
        script=relpath(Path(__file__), ROOT),
        command=command_argv(relpath(Path(__file__), ROOT), sys.argv[1:]),
        source={
            "kind": "synthetic",
            "upstream_reference": OSF_SOURCE,
            "license": "Generated validation asset; upstream OSF terms must be reviewed before real conversion.",
        },
        artifacts=[
            artifact(
                bin_path,
                bundle_root,
                "synthetic-mouse-animation",
                "datoviz-synthetic-mouse-v1",
                frames=args.frames,
                vertices=int(base.shape[0]),
                indices=int(indices.shape[0]),
            ),
            artifact(metadata_path, bundle_root, "metadata", "json"),
        ],
        validation=metadata,
        extra={"notes": ["Prepared under .cache; no Blender or raw OSF payload is committed."]},
    )
    write_provenance(
        bundle_root,
        title="Synthetic Mouse Animation",
        source_lines=[
            f"Recommended upstream dataset: `{OSF_SOURCE}`.",
            "This checkpoint uses deterministic generated geometry rather than downloading OSF assets.",
        ],
        processing_lines=[
            "Generated static topology, UVs, normals, texture, baked vertex-position frames, keypoints, skeleton edges, and trajectory-ready keypoint tracks.",
            "Wrote one compact binary payload consumed directly by the C showcase.",
        ],
        license_lines=[
            "Generated fallback data is local validation data.",
            "Review upstream OSF terms before preparing real redistributed assets.",
        ],
    )
    print(f"wrote {relpath(bundle_root, ROOT)} ({args.frames} frames)")


def main() -> int:
    """Run the synthetic mouse preparation command."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--force", action="store_true", help="replace prepared outputs")
    parser.add_argument("--frames", type=int, default=120)
    parser.add_argument("--rings", type=int, default=18)
    parser.add_argument("--sectors", type=int, default=32)
    parser.add_argument("--texture-size", type=int, default=64)
    args = parser.parse_args()
    if args.frames <= 1:
        parser.error("--frames must be greater than one")
    if args.rings < 4 or args.sectors < 8:
        parser.error("--rings/--sectors are too small")
    if args.texture_size <= 0:
        parser.error("--texture-size must be positive")
    prepare(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
