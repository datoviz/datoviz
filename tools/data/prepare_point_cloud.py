#!/usr/bin/env python3
"""Prepare the RESEPI RGB LiDAR source LAZ for the C point-cloud showcase."""

from __future__ import annotations

import argparse
import json
import re
import shutil
import struct
import sys
from pathlib import Path
from urllib.parse import urlparse

import numpy as np
import requests

from common import (
    CACHE_ROOT,
    artifact,
    command_argv,
    relpath,
    sha256_file,
    write_json,
    write_manifest,
    write_provenance,
)


ROOT = Path(__file__).resolve().parents[2]
EXAMPLE_ID = "point_cloud"

RESEPI_SAMPLE_PAGE = "https://lidarpayload.com/sample-data/"
RESEPI_VIEWER_PROJECT_ID = "689bab260195cc818197583b"
RESEPI_VIEWER_FILE_ID = "689bab84e397cd1e6892347d"
RESEPI_VIEWER_URL = f"https://app.stitch3d.io/viewer/{RESEPI_VIEWER_PROJECT_ID}"
RESEPI_DOWNLOAD_ACTION = "2f2271d525a1140efc82c0ff375e0f8417b4c7e8"
RESEPI_SOURCE_NAME = "RESEPI-GENM2X-COLORIZED-50M-10MS-STRIP-OUTLIER.laz"
RESEPI_PUBLIC_OBJECT_URL = (
    "https://original-bucket-prod.s3.amazonaws.com/public/"
    "0e04d02c-2443-41ba-8721-f7ef0243af2d--"
    "RESEPI_DASH_GENM2X_DASH_COLORIZED_DASH_50M_DASH_10MS_DASH_STRIP_OUTLIER.laz"
)
V03_PROVENANCE_NOTE = (
    "Datoviz v0.3 `examples/showcase/lidar.py` recorded "
    "`https://lidarpayload.com/sample-data/`, "
    "`RESEPI-M2X-100m10ms-FOV90-On RangerPro.laz`."
)

MAGIC = b"DVZPCD1\0"
VERSION = 2
HEADER_SIZE = struct.calcsize("<8sII6f")
DEFAULT_WEB_MAX_POINTS = 500_000

PALETTE_FRAME_BG = np.array([14, 17, 23], dtype=np.float32) / 255.0
PALETTE_PANEL_BG = np.array([22, 27, 34], dtype=np.float32) / 255.0
PALETTE_CYAN = np.array([76, 201, 240], dtype=np.float32) / 255.0
PALETTE_MINT = np.array([128, 255, 219], dtype=np.float32) / 255.0
PALETTE_AMBER = np.array([255, 183, 3], dtype=np.float32) / 255.0


def _signed_resepi_url() -> str:
    """Resolve the current public Stitch3D download URL for the RESEPI sample LAZ."""
    headers = {
        "Accept": "text/x-component",
        "Content-Type": "application/json",
        "Next-Action": RESEPI_DOWNLOAD_ACTION,
        "Origin": "https://app.stitch3d.io",
        "Referer": RESEPI_VIEWER_URL,
        "User-Agent": "Mozilla/5.0",
    }
    response = requests.post(
        RESEPI_VIEWER_URL,
        data=json.dumps([RESEPI_VIEWER_FILE_ID]),
        headers=headers,
        timeout=60,
    )
    response.raise_for_status()
    urls = re.findall(r"https?://[^\"\\\s]+", response.text)
    for url in urls:
        if "original-bucket-prod" in url and "X-Amz-Signature=" in url:
            return url.replace(r"\u0026", "&")
    raise RuntimeError("could not resolve signed RESEPI LAZ download URL")


def _download(url: str, path: Path, force: bool) -> None:
    """Download a source file when missing or explicitly refreshed."""
    if path.exists() and not force:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_suffix(path.suffix + ".tmp")
    with requests.get(url, stream=True, timeout=60, headers={"User-Agent": "Mozilla/5.0"}) as response:
        response.raise_for_status()
        with tmp.open("wb") as f:
            for chunk in response.iter_content(chunk_size=1024 * 1024):
                if chunk:
                    f.write(chunk)
    tmp.replace(path)


def _source_path(args: argparse.Namespace, bundle_root: Path) -> tuple[Path, dict[str, object]]:
    """Resolve or download the raw LAZ source."""
    if args.source is not None:
        parsed = urlparse(args.source)
        if parsed.scheme in {"http", "https"}:
            name = Path(parsed.path).name or RESEPI_SOURCE_NAME
            path = bundle_root / "source" / name
            _download(args.source, path, args.refresh_source)
            source_url = args.source
        else:
            path = Path(args.source)
            if not path.is_absolute():
                path = ROOT / path
            source_url = None
        if not path.exists():
            raise RuntimeError(f"missing source LAZ: {path}")
        return path, {
            "name": path.name,
            "url": source_url,
            "path": relpath(path, ROOT) if path.is_relative_to(ROOT) else str(path),
            "format": "LAZ",
        }

    path = bundle_root / "source" / RESEPI_SOURCE_NAME
    if args.refresh_source or not path.exists():
        signed_url = _signed_resepi_url()
        _download(signed_url, path, True)
    return path, {
        "name": RESEPI_SOURCE_NAME,
        "sample_page": RESEPI_SAMPLE_PAGE,
        "viewer_url": RESEPI_VIEWER_URL,
        "file_id": RESEPI_VIEWER_FILE_ID,
        "public_object_url": RESEPI_PUBLIC_OBJECT_URL,
        "format": "LAZ",
    }


def _load_laz_sample(path: Path, max_points: int, seed: int) -> tuple[np.ndarray, np.ndarray, dict[str, object]]:
    """Load and sample positions and RGB colors from a raw LAZ file."""
    try:
        import laspy
    except Exception as exc:
        raise RuntimeError("install LAZ support with `python3 -m pip install --user laspy lazrs`") from exc

    las = laspy.read(path)
    source_count = int(las.header.point_count)
    if source_count <= 0:
        raise RuntimeError("source LAZ has no points")

    rng = np.random.default_rng(seed)
    if source_count > max_points:
        idx = np.sort(rng.choice(source_count, size=max_points, replace=False))
    else:
        idx = slice(None)

    x = np.asarray(las.x[idx], dtype=np.float64)
    y = np.asarray(las.y[idx], dtype=np.float64)
    z = np.asarray(las.z[idx], dtype=np.float64)
    positions = np.column_stack([x, y, z]).astype(np.float64, copy=False)

    dims = set(las.point_format.dimension_names)
    if {"red", "green", "blue"}.issubset(dims):
        rgb16 = np.column_stack(
            [
                np.asarray(las.red[idx], dtype=np.uint16),
                np.asarray(las.green[idx], dtype=np.uint16),
                np.asarray(las.blue[idx], dtype=np.uint16),
            ]
        )
        divisor = 256 if int(np.max(rgb16)) > 255 else 1
        colors = np.clip(rgb16 // divisor, 0, 255).astype(np.uint8)
    elif "intensity" in dims:
        intensity = np.asarray(las.intensity[idx], dtype=np.float32)
        lo, hi = np.percentile(intensity, [1.0, 99.5])
        gray = np.clip((intensity - lo) / max(float(hi - lo), 1e-6), 0.0, 1.0)
        colors = np.repeat((255.0 * gray[:, None]).astype(np.uint8), 3, axis=1)
    else:
        raise RuntimeError("source LAZ must contain RGB or intensity attributes")

    metadata = {
        "source_point_count": source_count,
        "source_point_count_after_sampling": int(positions.shape[0]),
        "las_version": f"{las.header.version.major}.{las.header.version.minor}",
        "point_format": int(las.header.point_format.id),
        "source_bounds": {
            "min": [float(las.header.mins[0]), float(las.header.mins[1]), float(las.header.mins[2])],
            "max": [float(las.header.maxs[0]), float(las.header.maxs[1]), float(las.header.maxs[2])],
        },
        "source_dimensions": sorted(dims),
    }
    return positions, colors, metadata


def _grade_real_colors(rgb8: np.ndarray, height: np.ndarray) -> np.ndarray:
    """Apply the shared graphite/cyan gallery palette while preserving real RGB differences."""
    rgb = rgb8.astype(np.float32) / 255.0
    luminance = rgb @ np.array([0.2126, 0.7152, 0.0722], dtype=np.float32)
    l_lo, l_hi = np.percentile(luminance, [0.5, 99.5])
    l_norm = np.clip((luminance - l_lo) / max(float(l_hi - l_lo), 1e-6), 0.0, 1.0)

    gray = luminance[:, None]
    graded = 0.76 * rgb + 0.24 * gray

    shadow_mix = np.clip(0.20 * (1.0 - l_norm), 0.0, 0.20)
    graded = graded * (1.0 - shadow_mix[:, None]) + PALETTE_PANEL_BG * shadow_mix[:, None]

    h_lo, h_hi = np.percentile(height, [2.0, 99.0])
    h_norm = np.clip((height - h_lo) / max(float(h_hi - h_lo), 1e-6), 0.0, 1.0)
    cyan_mix = np.clip(0.04 + 0.10 * h_norm + 0.06 * l_norm, 0.0, 0.20)
    graded = graded * (1.0 - cyan_mix[:, None]) + PALETTE_CYAN * cyan_mix[:, None]

    green_bias = np.clip(rgb[:, 1] - 0.5 * (rgb[:, 0] + rgb[:, 2]), 0.0, 1.0)
    foliage_mix = np.clip(0.14 * green_bias, 0.0, 0.14)
    graded = graded * (1.0 - foliage_mix[:, None]) + PALETTE_MINT * foliage_mix[:, None]

    warm_bias = np.clip(rgb[:, 0] - np.maximum(rgb[:, 1], rgb[:, 2]), 0.0, 1.0)
    warm_mix = np.clip(0.08 * warm_bias, 0.0, 0.08)
    graded = graded * (1.0 - warm_mix[:, None]) + PALETTE_AMBER * warm_mix[:, None]

    graded = 0.5 + 1.06 * (graded - 0.5)
    return np.clip(graded, 0.0, 1.0)


def _normalize_points(
    source_positions: np.ndarray, source_colors: np.ndarray, z_exaggeration: float
) -> tuple[np.ndarray, dict[str, object]]:
    """Normalize sampled LAZ points into the point-cloud v2 cache layout."""
    xy = source_positions[:, :2].astype(np.float64, copy=True)
    z = source_positions[:, 2].astype(np.float64, copy=True)

    center_xy = np.percentile(xy, 50.0, axis=0)
    xy -= center_xy
    covariance = np.cov(xy, rowvar=False)
    eigvals, eigvecs = np.linalg.eigh(covariance)
    forward_vec = eigvecs[:, int(np.argmax(eigvals))]
    if forward_vec[1] < 0:
        forward_vec *= -1.0
    side_vec = np.array([forward_vec[1], -forward_vec[0]], dtype=np.float64)

    forward = xy @ forward_vec
    side = xy @ side_vec
    ground = float(np.percentile(z, 0.5))
    height = z - ground

    horizontal_scale = max(float(np.percentile(np.abs(np.column_stack([side, forward])), 99.4)), 1e-6)
    coordinate_scale = horizontal_scale
    vertical_scale = coordinate_scale / z_exaggeration

    positions = np.empty((source_positions.shape[0], 3), dtype=np.float32)
    positions[:, 0] = (side / horizontal_scale).astype(np.float32)
    positions[:, 1] = (forward / horizontal_scale).astype(np.float32)
    positions[:, 2] = (height / vertical_scale).astype(np.float32)

    colors = _grade_real_colors(source_colors, height)
    alpha = np.ones((positions.shape[0], 1), dtype=np.float32)
    luminance = colors @ np.array([0.2126, 0.7152, 0.0722], dtype=np.float32)
    pixel_size = np.clip(1.00 + 0.70 * np.power(luminance, 0.65), 1.00, 1.75).astype(np.float32)

    records = np.column_stack([positions, colors, alpha, pixel_size]).astype(np.float32)
    metadata: dict[str, object] = {
        "point_count": int(records.shape[0]),
        "center_source_xy": center_xy.astype(float).tolist(),
        "ground_source_z": ground,
        "horizontal_scale": horizontal_scale,
        "vertical_scale": vertical_scale,
        "coordinate_scale_m_per_unit": coordinate_scale,
        "z_exaggeration": z_exaggeration,
        "forward_vector_source_xy": forward_vec.astype(float).tolist(),
        "side_vector_source_xy": side_vec.astype(float).tolist(),
        "bounds_normalized": {
            "min": positions.min(axis=0).astype(float).tolist(),
            "max": positions.max(axis=0).astype(float).tolist(),
        },
        "source_color_percentiles": {
            "r": np.percentile(source_colors[:, 0], [0, 25, 50, 75, 100]).astype(float).tolist(),
            "g": np.percentile(source_colors[:, 1], [0, 25, 50, 75, 100]).astype(float).tolist(),
            "b": np.percentile(source_colors[:, 2], [0, 25, 50, 75, 100]).astype(float).tolist(),
        },
        "color_channel_percentiles": {
            "r": np.percentile(colors[:, 0], [0, 25, 50, 75, 100]).astype(float).tolist(),
            "g": np.percentile(colors[:, 1], [0, 25, 50, 75, 100]).astype(float).tolist(),
            "b": np.percentile(colors[:, 2], [0, 25, 50, 75, 100]).astype(float).tolist(),
        },
    }
    return records, metadata


def _write_binary(path: Path, records: np.ndarray) -> None:
    """Write the native point-cloud binary format consumed by the C example."""
    path.parent.mkdir(parents=True, exist_ok=True)
    mins = records[:, :3].min(axis=0)
    maxs = records[:, :3].max(axis=0)
    header = struct.pack("<8sII6f", MAGIC, VERSION, records.shape[0], *mins, *maxs)
    with path.open("wb") as f:
        f.write(header)
        f.write(np.ascontiguousarray(records, dtype="<f4").tobytes())


def _web_records(records: np.ndarray, max_points: int) -> np.ndarray:
    """Return a deterministic, bounded subset for browser delivery."""
    if records.shape[0] <= max_points:
        return np.ascontiguousarray(records, dtype=np.float32)
    indices = np.linspace(0, records.shape[0] - 1, max_points, dtype=np.int64)
    return np.ascontiguousarray(records[indices], dtype=np.float32)


def _write_web_bundle(bundle_root: Path, records: np.ndarray, max_points: int) -> Path:
    """Write the cache-local browser point-cloud artifact and staging metadata."""
    web_records = _web_records(records, max_points)
    web_prepared = bundle_root / "web" / "prepared"
    binary_path = web_prepared / "point_cloud.bin"
    _write_binary(binary_path, web_records)
    write_json(
        web_prepared / "metadata.json",
        {
            "schema": "datoviz.point-cloud-web-cache.v1",
            "artifact": {
                "path": "prepared/point_cloud.bin",
                "bytes": binary_path.stat().st_size,
                "sha256": sha256_file(binary_path),
            },
            "point_count": int(web_records.shape[0]),
            "source_point_count": int(records.shape[0]),
        },
    )
    return binary_path


def _read_prepared_records(path: Path) -> np.ndarray:
    """Read an existing v2 prepared binary without loading the raw LAZ source."""
    with path.open("rb") as f:
        header = f.read(HEADER_SIZE)
    if len(header) != HEADER_SIZE:
        raise RuntimeError(f"invalid point-cloud header: {path}")
    magic, version, count, *_ = struct.unpack("<8sII6f", header)
    if magic != MAGIC or version != VERSION or count <= 0:
        raise RuntimeError(f"unsupported point-cloud binary: {path}")
    expected_size = HEADER_SIZE + count * 8 * np.dtype("<f4").itemsize
    if path.stat().st_size != expected_size:
        raise RuntimeError(f"invalid point-cloud byte size: {path}")
    return np.memmap(path, mode="r", dtype="<f4", offset=HEADER_SIZE, shape=(count, 8))


def _write_preview(path: Path, records: np.ndarray) -> None:
    """Write a quick top-down preview for source-selection iteration."""
    try:
        import matplotlib.pyplot as plt
    except Exception:
        return

    x = records[:, 0]
    y = records[:, 1]
    colors = records[:, 3:7]
    order = np.argsort(records[:, 2])
    frame_bg = tuple(float(c) for c in PALETTE_FRAME_BG)
    fig, ax = plt.subplots(figsize=(12, 7), facecolor=frame_bg)
    ax.set_facecolor(frame_bg)
    ax.scatter(x[order], y[order], s=0.08, c=colors[order], linewidths=0)
    ax.set_aspect("equal", adjustable="box")
    ax.set_axis_off()
    fig.tight_layout(pad=0)
    path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(path, dpi=180, facecolor=frame_bg)
    plt.close(fig)


def prepare(args: argparse.Namespace) -> None:
    """Prepare the point-cloud cache bundle."""
    bundle_root = CACHE_ROOT / EXAMPLE_ID
    prepared = bundle_root / "prepared"
    if args.force and bundle_root.exists() and args.source is not None:
        shutil.rmtree(bundle_root)
    elif args.force and prepared.exists():
        shutil.rmtree(prepared)
    prepared.mkdir(parents=True, exist_ok=True)

    source_path, source_info = _source_path(args, bundle_root)
    source_positions, source_colors, source_metadata = _load_laz_sample(
        source_path, args.max_points, args.seed
    )
    records, metadata = _normalize_points(source_positions, source_colors, args.z_exaggeration)

    bin_path = prepared / "point_cloud.bin"
    preview_path = prepared / "preview.png"
    _write_binary(bin_path, records)
    web_path = _write_web_bundle(bundle_root, records, args.web_max_points)
    _write_preview(preview_path, records)

    artifacts = [
        artifact(
            bin_path,
            bundle_root,
            "point-cloud-records",
            "datoviz-point-cloud-v2",
            dtype="float32",
            shape=[int(records.shape[0]), 8],
            columns=["x", "y", "z", "r", "g", "b", "a", "pixel_size"],
        )
    ]
    artifacts.append(
        artifact(
            web_path,
            bundle_root,
            "browser-point-cloud-records",
            "datoviz-point-cloud-v2",
            dtype="float32",
            shape=[min(int(records.shape[0]), args.web_max_points), 8],
            columns=["x", "y", "z", "r", "g", "b", "a", "pixel_size"],
        )
    )
    if preview_path.exists():
        artifacts.append(artifact(preview_path, bundle_root, "preview", "png"))
    artifacts.append(artifact(source_path, bundle_root, "raw-source", "laz"))

    write_manifest(
        bundle_root,
        example_id=EXAMPLE_ID,
        title="RESEPI RGB LiDAR Point Cloud",
        status="cache-only",
        script=relpath(Path(__file__), ROOT),
        command=command_argv(relpath(Path(__file__), ROOT), sys.argv[1:]),
        source={
            **source_info,
            "license": "Redistribution permission received; permission details pending.",
            "v03_provenance_note": V03_PROVENANCE_NOTE,
        },
        artifacts=artifacts,
        validation={
            **source_metadata,
            **metadata,
            "seed": args.seed,
        },
        extra={
            "notes": [
                "The raw source and six-million-point native bundle remain cache-local.",
                "The deterministic 500,000-point browser derivative is published separately in the data submodule.",
            ]
        },
    )
    write_provenance(
        bundle_root,
        title="RESEPI RGB LiDAR Point Cloud",
        source_lines=[
            V03_PROVENANCE_NOTE,
            f"Current source page: `{RESEPI_SAMPLE_PAGE}`.",
            f"Current public viewer: `{RESEPI_VIEWER_URL}`.",
            f"Raw LAZ file used: `{source_info['name']}`.",
            "The source contains real per-point RGB color attributes.",
        ],
        processing_lines=[
            f"Downloaded/read the raw LAZ source; did not read `data/misc/lidar.npz`.",
            f"Deterministically sampled up to `{args.max_points}` source points with seed `{args.seed}`.",
            "Centered the point cloud and aligned the horizontal long axis with scene depth.",
            f"Preserved metric XY/Z aspect ratio with z exaggeration `{args.z_exaggeration}`.",
            "Preserved real per-point RGB variation and graded it with the shared graphite/cyan palette tokens.",
            "Wrote binary `point_cloud.bin` records in the v2 direct-color layout.",
        ],
        license_lines=[
            "The current source is public RESEPI sample data downloadable through Stitch3D.",
            "Redistribution permission was received; grantor, date, scope, and durable reference details are pending.",
            "The raw LAZ and six-million-point native bundle remain cache-local; only the deterministic 500,000-point browser derivative is published.",
        ],
    )
    print(f"wrote {relpath(bundle_root, ROOT)} ({records.shape[0]} points)")


def main() -> int:
    """Run the point-cloud preparation command."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--force", action="store_true", help="replace prepared outputs")
    parser.add_argument("--refresh-source", action="store_true", help="redownload the raw LAZ source")
    parser.add_argument("--max-points", type=int, default=6_000_000)
    parser.add_argument("--web-max-points", type=int, default=DEFAULT_WEB_MAX_POINTS)
    parser.add_argument(
        "--web-only",
        action="store_true",
        help="derive only the browser bundle from the existing prepared binary",
    )
    parser.add_argument("--seed", type=int, default=12345)
    parser.add_argument(
        "--z-exaggeration",
        type=float,
        default=1.0,
        help="vertical scale multiplier; 1.0 preserves metric XY/Z aspect ratio",
    )
    parser.add_argument("--source", help="optional local or HTTP(S) LAZ source override")
    args = parser.parse_args()
    if args.max_points <= 0:
        parser.error("--max-points must be positive")
    if args.web_max_points <= 0:
        parser.error("--web-max-points must be positive")
    if args.z_exaggeration <= 0:
        parser.error("--z-exaggeration must be positive")
    if args.web_only:
        bundle_root = CACHE_ROOT / EXAMPLE_ID
        records = _read_prepared_records(bundle_root / "prepared" / "point_cloud.bin")
        path = _write_web_bundle(bundle_root, records, args.web_max_points)
        print(f"wrote {relpath(path, ROOT)} ({min(records.shape[0], args.web_max_points)} points)")
        return 0
    prepare(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
