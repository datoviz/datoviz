#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.10"
# dependencies = [
#   "numpy==2.2.6",
#   "pillow==11.2.1",
#   "pyproj==3.7.1",
#   "requests==2.32.4",
# ]
# ///
"""Prepare aligned USGS 3DEP elevation and NAIP imagery for the terrain showcase."""

from __future__ import annotations

import argparse
import json
import shutil
import struct
from pathlib import Path
from typing import Any

import numpy as np
import requests
from PIL import Image, ImageEnhance, ImageOps
from pyproj import Transformer

from common import (
    CACHE_ROOT,
    REPO_ROOT,
    artifact,
    command_argv,
    write_json,
    write_manifest,
    write_provenance,
)


EXAMPLE_ID = "terrain_relief"
DEFAULT_OUTPUT = CACHE_ROOT / EXAMPLE_ID

ELEVATION_SERVICE = (
    "https://elevation.nationalmap.gov/arcgis/rest/services/3DEPElevation/ImageServer"
)
IMAGERY_SERVICE = (
    "https://imagery.nationalmap.gov/arcgis/rest/services/USGSNAIPPlus/ImageServer"
)

# McHenrys Peak, Glacier Gorge, and the Continental Divide in Rocky Mountain National Park.
WGS84_BBOX = (-105.705, 40.245, -105.625, 40.310)
OUTPUT_CRS = 26913  # NAD83 / UTM zone 13N.
ELEVATION_ROWS = 512
TEXTURE_ROWS = 2048

MAGIC = b"DVZTRN1\0"
VERSION = 1
USER_AGENT = "Datoviz-v0.4-terrain-relief-example/1.0"


def _service_metadata(session: requests.Session, service_url: str) -> dict[str, Any]:
    """Fetch the public ArcGIS image-service metadata."""
    response = session.get(service_url, params={"f": "json"}, timeout=60)
    response.raise_for_status()
    payload = response.json()
    if "error" in payload:
        raise RuntimeError(f"ArcGIS service metadata error: {payload['error']}")
    return payload


def _download_file(session: requests.Session, url: str, path: Path) -> None:
    """Download one response body atomically."""
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_suffix(path.suffix + ".tmp")
    with session.get(url, stream=True, timeout=180) as response:
        response.raise_for_status()
        with tmp.open("wb") as file:
            for chunk in response.iter_content(chunk_size=1024 * 1024):
                if chunk:
                    file.write(chunk)
    tmp.replace(path)


def _export_image(
    session: requests.Session,
    service_url: str,
    path: Path,
    metadata_path: Path,
    *,
    cols: int,
    rows: int,
    image_format: str,
    pixel_type: str | None = None,
    refresh: bool,
) -> dict[str, Any]:
    """Export and cache one projected ArcGIS image-service raster."""
    if path.exists() and metadata_path.exists() and not refresh:
        return json.loads(metadata_path.read_text(encoding="utf8"))["export"]

    params: dict[str, str] = {
        "bbox": ",".join(f"{value:.9f}" for value in WGS84_BBOX),
        "bboxSR": "4326",
        "imageSR": str(OUTPUT_CRS),
        "size": f"{cols},{rows}",
        "format": image_format,
        "interpolation": "RSP_BilinearInterpolation",
        "f": "json",
    }
    if pixel_type is not None:
        params["pixelType"] = pixel_type

    endpoint = f"{service_url}/exportImage"
    response = session.get(endpoint, params=params, timeout=180)
    response.raise_for_status()
    export = response.json()
    if "error" in export:
        raise RuntimeError(f"ArcGIS export error: {export['error']}")
    href = export.get("href")
    if not isinstance(href, str) or not href.startswith("https://"):
        raise RuntimeError(f"ArcGIS export did not return an HTTPS download: {export}")

    _download_file(session, href, path)
    write_json(
        metadata_path,
        {
            "endpoint": endpoint,
            "parameters": params,
            "export": export,
        },
    )
    return export


def _projected_dimensions() -> tuple[float, float]:
    """Return the requested WGS84 crop dimensions in UTM meters."""
    west, south, east, north = WGS84_BBOX
    transformer = Transformer.from_crs(4326, OUTPUT_CRS, always_xy=True)
    xmin, ymin = transformer.transform(west, south)
    xmax, ymax = transformer.transform(east, north)
    return float(xmax - xmin), float(ymax - ymin)


def _cols_for_rows(rows: int, width_m: float, depth_m: float) -> int:
    """Choose a column count that keeps projected pixels approximately square."""
    return max(2, int(round(rows * width_m / depth_m)))


def _extent(export: dict[str, Any]) -> tuple[float, float, float, float]:
    """Read one projected extent from an ArcGIS export response."""
    extent = export.get("extent")
    if not isinstance(extent, dict):
        raise RuntimeError("ArcGIS export response is missing its projected extent")
    try:
        keys = ("xmin", "ymin", "xmax", "ymax")
        return tuple(float(extent[key]) for key in keys)  # type: ignore[return-value]
    except (KeyError, TypeError, ValueError) as exc:
        raise RuntimeError(f"invalid ArcGIS export extent: {extent}") from exc


def _assert_aligned(elevation_export: dict[str, Any], imagery_export: dict[str, Any]) -> None:
    """Verify that elevation and imagery cover the same projected rectangle."""
    elevation_extent = np.asarray(_extent(elevation_export), dtype=np.float64)
    imagery_extent = np.asarray(_extent(imagery_export), dtype=np.float64)
    if not np.allclose(elevation_extent, imagery_extent, rtol=0.0, atol=0.02):
        raise RuntimeError(
            "USGS elevation and imagery exports are not aligned: "
            f"{elevation_extent.tolist()} != {imagery_extent.tolist()}"
        )


def _load_elevation(path: Path, rows: int, cols: int) -> np.ndarray:
    """Load and validate the exported float32 elevation grid."""
    with Image.open(path) as image:
        elevation = np.asarray(image, dtype=np.float32)
    if elevation.shape != (rows, cols):
        raise RuntimeError(f"unexpected elevation shape {elevation.shape}; expected {(rows, cols)}")
    if not np.all(np.isfinite(elevation)):
        raise RuntimeError("elevation export contains non-finite samples")
    return np.ascontiguousarray(elevation, dtype="<f4")


def _prepare_texture(source: Path, output: Path, rows: int, cols: int) -> None:
    """Apply a restrained gallery grade while preserving the aligned NAIP pixels."""
    with Image.open(source) as image:
        texture = ImageOps.exif_transpose(image).convert("RGB")
        if texture.size != (cols, rows):
            raise RuntimeError(
                f"unexpected imagery size {texture.size}; expected {(cols, rows)}"
            )
        texture = ImageOps.autocontrast(texture, cutoff=(0.25, 0.25))
        texture = ImageEnhance.Color(texture).enhance(1.08)
        texture = ImageEnhance.Contrast(texture).enhance(1.06)
        texture = ImageEnhance.Brightness(texture).enhance(0.96)
        output.parent.mkdir(parents=True, exist_ok=True)
        texture.save(output, format="JPEG", quality=92, subsampling=0, optimize=True)


def _write_elevation(
    path: Path,
    elevation: np.ndarray,
    *,
    width_m: float,
    depth_m: float,
) -> None:
    """Write the little-endian terrain-grid binary consumed by the C example."""
    rows, cols = elevation.shape
    header = struct.pack(
        "<8sIIIffff",
        MAGIC,
        VERSION,
        rows,
        cols,
        width_m,
        depth_m,
        float(np.min(elevation)),
        float(np.max(elevation)),
    )
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wb") as file:
        file.write(header)
        file.write(elevation.tobytes(order="C"))


def prepare(args: argparse.Namespace) -> None:
    """Prepare the cache-only McHenrys Peak terrain bundle."""
    bundle_root = args.output.resolve()
    source = bundle_root / "source"
    prepared = bundle_root / "prepared"
    if args.refresh_source and source.exists():
        shutil.rmtree(source)
    if args.force and prepared.exists():
        shutil.rmtree(prepared)
    source.mkdir(parents=True, exist_ok=True)
    prepared.mkdir(parents=True, exist_ok=True)

    approximate_width_m, approximate_depth_m = _projected_dimensions()
    elevation_cols = _cols_for_rows(
        ELEVATION_ROWS, approximate_width_m, approximate_depth_m
    )
    texture_cols = _cols_for_rows(TEXTURE_ROWS, approximate_width_m, approximate_depth_m)

    session = requests.Session()
    session.headers.update({"User-Agent": USER_AGENT})
    elevation_service = _service_metadata(session, ELEVATION_SERVICE)
    imagery_service = _service_metadata(session, IMAGERY_SERVICE)

    elevation_source = source / "mchenrys_peak_3dep.tif"
    imagery_source = source / "mchenrys_peak_naip.jpg"
    elevation_export = _export_image(
        session,
        ELEVATION_SERVICE,
        elevation_source,
        source / "mchenrys_peak_3dep.export.json",
        cols=elevation_cols,
        rows=ELEVATION_ROWS,
        image_format="tiff",
        pixel_type="F32",
        refresh=args.refresh_source,
    )
    imagery_export = _export_image(
        session,
        IMAGERY_SERVICE,
        imagery_source,
        source / "mchenrys_peak_naip.export.json",
        cols=texture_cols,
        rows=TEXTURE_ROWS,
        image_format="jpgpng",
        refresh=args.refresh_source,
    )
    _assert_aligned(elevation_export, imagery_export)

    xmin, ymin, xmax, ymax = _extent(elevation_export)
    width_m = xmax - xmin
    depth_m = ymax - ymin
    elevation = _load_elevation(elevation_source, ELEVATION_ROWS, elevation_cols)

    elevation_output = prepared / "terrain.bin"
    texture_output = prepared / "terrain.jpg"
    _write_elevation(elevation_output, elevation, width_m=width_m, depth_m=depth_m)
    _prepare_texture(imagery_source, texture_output, TEXTURE_ROWS, texture_cols)

    source_metadata = {
        "name": "USGS McHenrys Peak terrain and orthoimagery",
        "bbox_wgs84": list(WGS84_BBOX),
        "output_crs": f"EPSG:{OUTPUT_CRS}",
        "elevation": {
            "service": ELEVATION_SERVICE,
            "description": elevation_service.get("serviceDescription", ""),
            "copyright": elevation_service.get("copyrightText", ""),
        },
        "imagery": {
            "service": IMAGERY_SERVICE,
            "description": imagery_service.get("serviceDescription", ""),
            "copyright": imagery_service.get("copyrightText", ""),
        },
    }
    command = command_argv(
        "tools/data/prepare_terrain_relief.py",
        ["--output", str(bundle_root.relative_to(REPO_ROOT))]
        if bundle_root.is_relative_to(REPO_ROOT)
        else ["--output", str(bundle_root)],
    )
    write_manifest(
        bundle_root,
        example_id=EXAMPLE_ID,
        title="McHenrys Peak Terrain Relief",
        status="cache-only",
        script="tools/data/prepare_terrain_relief.py",
        command=command,
        source=source_metadata,
        artifacts=[
            artifact(
                elevation_output,
                bundle_root,
                "elevation-grid",
                "DVZTRN1 little-endian float32",
                rows=ELEVATION_ROWS,
                cols=elevation_cols,
                elevation_min_m=float(np.min(elevation)),
                elevation_max_m=float(np.max(elevation)),
            ),
            artifact(
                texture_output,
                bundle_root,
                "surface-texture",
                "JPEG RGB8",
                width=texture_cols,
                height=TEXTURE_ROWS,
            ),
        ],
        validation={
            "aligned_projected_extent": [xmin, ymin, xmax, ymax],
            "width_m": width_m,
            "depth_m": depth_m,
            "finite_elevation": True,
        },
        extra={
            "encoding": {
                "terrain_header": "<8sIIIffff",
                "row_order": "north-to-south",
                "column_order": "west-to-east",
                "elevation_units": "meters NAVD88",
                "horizontal_crs": f"EPSG:{OUTPUT_CRS}",
            }
        },
    )
    write_provenance(
        bundle_root,
        title="McHenrys Peak Terrain Relief",
        source_lines=[
            "Bare-earth elevation: USGS 3D Elevation Program dynamic image service.",
            "Natural-color texture: USGS The National Map NAIP Plus image service.",
            f"WGS84 crop: {WGS84_BBOX} around McHenrys Peak and Glacier Gorge, Colorado.",
        ],
        processing_lines=[
            f"Both rasters were exported into EPSG:{OUTPUT_CRS} over the same projected extent.",
            f"Elevation was sampled to {elevation_cols}x{ELEVATION_ROWS} float32 meters.",
            f"Imagery was sampled to {texture_cols}x{TEXTURE_ROWS} RGB pixels.",
            "The imagery received restrained autocontrast, saturation, contrast, and "
            "brightness grading.",
        ],
        license_lines=[
            "USGS 3DEP products are public domain and available without use restrictions.",
            "NAIP imagery acquired by USDA has been placed in the public domain.",
            "Credit: U.S. Geological Survey 3DEP and USDA National Agriculture Imagery Program.",
        ],
        notes=[
            "The ArcGIS service descriptions and exact export requests are retained under source/.",
            "Service-backed source mosaics evolve; the manifest records checksums for this "
            "prepared snapshot.",
        ],
    )

    print(f"prepared terrain relief in {bundle_root}")
    print(
        f"grid {elevation_cols}x{ELEVATION_ROWS}, texture {texture_cols}x{TEXTURE_ROWS}, "
        f"elevation {float(np.min(elevation)):.1f}-{float(np.max(elevation)):.1f} m"
    )


def main() -> int:
    """Parse command-line arguments and prepare the terrain cache."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--force", action="store_true", help="replace prepared outputs")
    parser.add_argument(
        "--refresh-source", action="store_true", help="redownload both USGS service exports"
    )
    args = parser.parse_args()
    prepare(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
