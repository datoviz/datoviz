#!/usr/bin/env python3
"""Prepare local Gothic Gaussian-splat PLY arrays for a future splat showcase example."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

import numpy as np


DEFAULT_CACHE = Path(".cache/datoviz/examples/gothic_splat")
DEFAULT_INPUT = DEFAULT_CACHE / "source" / "gothic.ply"
DEFAULT_OUTPUT = DEFAULT_CACHE / "prepared"
SH_C0 = 0.28209479177387814


def _sigmoid(values: np.ndarray) -> np.ndarray:
    values = np.nan_to_num(values, nan=-88.0, posinf=88.0, neginf=-88.0)
    return 1.0 / (1.0 + np.exp(-values))


def _read_binary_float_ply(path: Path) -> tuple[np.memmap, list[str]]:
    path = path.expanduser().resolve()
    props: list[str] = []
    vertex_count: int | None = None
    header_bytes = 0

    with path.open("rb") as file:
        first = file.readline()
        header_bytes += len(first)
        if first.strip() != b"ply":
            raise ValueError(f"{path} is not a PLY file")

        while True:
            line = file.readline()
            if not line:
                raise ValueError(f"{path} ended before end_header")

            header_bytes += len(line)
            text = line.decode("ascii", "replace").strip()
            parts = text.split()

            if len(parts) == 3 and parts[0] == "format":
                if parts[1] != "binary_little_endian" or parts[2] != "1.0":
                    raise ValueError(f"unsupported PLY format: {text}")

            if len(parts) == 3 and parts[:2] == ["element", "vertex"]:
                vertex_count = int(parts[2])

            if len(parts) == 3 and parts[0] == "property":
                if parts[1] != "float":
                    raise ValueError(f"unsupported PLY property type: {text}")
                props.append(parts[2])

            if text == "end_header":
                break

    if vertex_count is None:
        raise ValueError(f"{path} does not declare an element vertex count")
    if not props:
        raise ValueError(f"{path} does not declare float vertex properties")

    record_bytes = len(props) * np.dtype("<f4").itemsize
    payload_bytes = path.stat().st_size - header_bytes
    if payload_bytes != vertex_count * record_bytes:
        raise ValueError(
            f"unexpected payload size: {payload_bytes} bytes for {vertex_count} vertices "
            f"and {len(props)} float fields"
        )

    dtype = np.dtype([(name, "<f4") for name in props])
    data = np.memmap(path, dtype=dtype, mode="r", offset=header_bytes, shape=(vertex_count,))
    return data, props


def _require_props(props: list[str], required: set[str]) -> None:
    missing = sorted(required.difference(props))
    if missing:
        raise ValueError(f"input PLY is missing required fields: {missing}")


def prepare(
    input_path: Path,
    output_dir: Path,
    max_points: int,
    alpha_min: float,
    normalize: bool,
    flip_y: bool,
) -> None:
    input_path = input_path.expanduser().resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    data, props = _read_binary_float_ply(input_path)
    required = {
        "x",
        "y",
        "z",
        "f_dc_0",
        "f_dc_1",
        "f_dc_2",
        "opacity",
        "rot_0",
        "rot_1",
        "rot_2",
        "rot_3",
        "scale_0",
        "scale_1",
        "scale_2",
    }
    _require_props(props, required)

    opacity = np.asarray(data["opacity"], dtype=np.float32)
    alpha = _sigmoid(opacity).astype(np.float32)
    keep = np.flatnonzero(alpha >= alpha_min)

    if max_points > 0 and keep.size > max_points:
        score = alpha[keep]
        selected = np.argpartition(score, -max_points)[-max_points:]
        keep = np.sort(keep[selected])

    pos = np.column_stack([data["x"][keep], data["y"][keep], data["z"][keep]]).astype(np.float32)
    center = np.zeros(3, dtype=np.float32)
    scene_scale = np.float32(1.0)

    if normalize:
        lo = np.percentile(pos, 1.0, axis=0).astype(np.float32)
        hi = np.percentile(pos, 99.0, axis=0).astype(np.float32)
        center = ((lo + hi) * 0.5).astype(np.float32)
        scene_scale = np.float32(np.max(hi - lo))
        if not np.isfinite(scene_scale) or scene_scale <= 0:
            raise ValueError("could not compute a valid normalization scale")
        pos = np.ascontiguousarray((pos - center) / scene_scale, dtype=np.float32)
    else:
        pos = np.ascontiguousarray(pos, dtype=np.float32)

    if flip_y:
        pos[:, 1] *= -1.0

    f_dc = np.column_stack(
        [data["f_dc_0"][keep], data["f_dc_1"][keep], data["f_dc_2"][keep]]
    ).astype(np.float32)
    rgb = np.clip(0.5 + SH_C0 * f_dc, 0.0, 1.0)

    color = np.empty((keep.size, 4), dtype=np.uint8)
    color[:, :3] = np.round(rgb * 255.0).astype(np.uint8)
    color[:, 3] = np.round(np.clip(alpha[keep], 0.0, 1.0) * 255.0).astype(np.uint8)
    color = np.ascontiguousarray(color)

    scale_log = np.column_stack(
        [data["scale_0"][keep], data["scale_1"][keep], data["scale_2"][keep]]
    ).astype(np.float32)
    scale = np.exp(scale_log).astype(np.float32)
    if normalize:
        scale = np.ascontiguousarray(scale / scene_scale, dtype=np.float32)
    else:
        scale = np.ascontiguousarray(scale, dtype=np.float32)

    radius = np.cbrt(scale[:, 0] * scale[:, 1] * scale[:, 2]).astype(np.float32)
    radius = np.ascontiguousarray(radius)

    quat = np.column_stack(
        [data["rot_0"][keep], data["rot_1"][keep], data["rot_2"][keep], data["rot_3"][keep]]
    ).astype(np.float32)
    quat_norm = np.linalg.norm(quat, axis=1).astype(np.float32)
    quat = np.ascontiguousarray(quat / np.maximum(quat_norm[:, None], 1e-12), dtype=np.float32)

    np.save(output_dir / "gothic_splat_pos.npy", pos)
    np.save(output_dir / "gothic_splat_color.npy", color)
    np.save(output_dir / "gothic_splat_radius.npy", radius)
    np.save(output_dir / "gothic_splat_scale.npy", scale)
    np.save(output_dir / "gothic_splat_quat.npy", quat)

    metadata = {
        "source": str(input_path),
        "source_vertex_count": int(len(data)),
        "exported_count": int(keep.size),
        "alpha_min": float(alpha_min),
        "max_points": int(max_points),
        "normalized": bool(normalize),
        "normalization_center": center.astype(float).tolist(),
        "normalization_scale": float(scene_scale),
        "orientation_transform": {
            "flip_y": bool(flip_y),
            "note": "The Gothic 3DGS source appears vertically inverted in Datoviz scene space.",
        },
        "arrays": {
            "gothic_splat_pos.npy": "float32 Nx3 center positions",
            "gothic_splat_color.npy": "uint8 Nx4 RGBA colors, DvzColor-compatible",
            "gothic_splat_radius.npy": "float32 N scalar radius from geometric mean scale",
            "gothic_splat_scale.npy": "float32 Nx3 exp(scale_*) future 3DGS attribute",
            "gothic_splat_quat.npy": "float32 Nx4 normalized rot_* future 3DGS attribute",
        },
    }
    (output_dir / "gothic_splat_meta.json").write_text(json.dumps(metadata, indent=2))

    print(f"wrote {output_dir}")
    print(f"source vertices: {len(data):,}")
    print(f"exported splats: {keep.size:,}")
    print(f"alpha threshold: {alpha_min}")
    print(f"normalized: {normalize}")
    print(f"flip y: {flip_y}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Convert a Gaussian-splat PLY into local Datoviz-friendly .npy arrays."
    )
    parser.add_argument(
        "--input",
        type=Path,
        default=DEFAULT_INPUT,
        help=f"input Gaussian-splat PLY path, default: {DEFAULT_INPUT}",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=DEFAULT_OUTPUT,
        help=f"output directory, default: {DEFAULT_OUTPUT}",
    )
    parser.add_argument(
        "--max-points",
        type=int,
        default=500_000,
        help="maximum number of splats to export; use 0 for all retained splats",
    )
    parser.add_argument(
        "--alpha-min",
        type=float,
        default=0.05,
        help="minimum decoded alpha retained in the exported subset",
    )
    parser.add_argument(
        "--normalize",
        action="store_true",
        help="normalize positions and scales by the 1st/99th percentile scene extent",
    )
    parser.add_argument(
        "--keep-source-y",
        action="store_true",
        help="do not flip the source Y axis before writing Datoviz-ready positions",
    )
    args = parser.parse_args()

    if args.max_points < 0:
        raise ValueError("--max-points must be non-negative")
    if not 0 <= args.alpha_min <= 1:
        raise ValueError("--alpha-min must be between 0 and 1")
    if not args.input.expanduser().exists():
        raise FileNotFoundError(
            f"{args.input} does not exist. Place gothic.ply at the default source path or pass "
            "--input /path/to/gothic.ply"
        )

    prepare(
        args.input,
        args.output_dir,
        args.max_points,
        args.alpha_min,
        args.normalize,
        not args.keep_source_y,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
