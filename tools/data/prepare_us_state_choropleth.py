#!/usr/bin/env python3
"""Prepare flat C-readable U.S. state choropleth arrays from public Census data."""

from __future__ import annotations

import argparse
import csv
import math
import shutil
import struct
import urllib.request
import xml.etree.ElementTree as ET
import zipfile
from pathlib import Path
from typing import Any

from common import (
    CACHE_ROOT,
    REPO_ROOT,
    artifact,
    command_argv,
    relpath,
    write_manifest,
    write_provenance,
)


EXAMPLE_ID = "us_state_choropleth"
DEFAULT_OUTPUT = CACHE_ROOT / EXAMPLE_ID
BOUNDARY_URL = "https://www2.census.gov/geo/tiger/GENZ2024/shp/cb_2024_us_state_20m.zip"
POPULATION_XLSX_URL = (
    "https://www2.census.gov/programs-surveys/popest/tables/2020-2025/"
    "state/totals/NST-EST2025-POP.xlsx"
)

# Contiguous states only: keeps the first gallery target readable without inset logic.
EXCLUDED_STATEFP = {"02", "11", "15", "60", "66", "69", "72", "78"}

RAMP = (
    (26, 35, 46, 235),
    (33, 99, 126, 235),
    (50, 160, 147, 235),
    (237, 191, 94, 235),
    (221, 96, 73, 235),
)


def _download(url: str, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with urllib.request.urlopen(url, timeout=120) as response:
        path.write_bytes(response.read())


def _read_dbf(path: Path) -> list[dict[str, str]]:
    data = path.read_bytes()
    record_count = struct.unpack_from("<I", data, 4)[0]
    header_len = struct.unpack_from("<H", data, 8)[0]
    record_len = struct.unpack_from("<H", data, 10)[0]

    fields: list[tuple[str, int, int]] = []
    offset = 32
    cursor = 1
    while offset < header_len and data[offset] != 0x0D:
        raw_name = data[offset : offset + 11].split(b"\0", 1)[0]
        name = raw_name.decode("ascii", errors="ignore")
        length = data[offset + 16]
        fields.append((name, cursor, length))
        cursor += length
        offset += 32

    records: list[dict[str, str]] = []
    base = header_len
    for i in range(record_count):
        rec = data[base + i * record_len : base + (i + 1) * record_len]
        if not rec or rec[0:1] == b"*":
            continue
        row: dict[str, str] = {}
        for name, start, length in fields:
            raw = rec[start : start + length].strip()
            row[name] = raw.decode("utf-8", errors="replace")
        records.append(row)
    return records


def _read_shp_polygons(path: Path) -> list[list[list[tuple[float, float]]]]:
    data = path.read_bytes()
    offset = 100
    records: list[list[list[tuple[float, float]]]] = []
    while offset + 8 <= len(data):
        content_words = struct.unpack_from(">i", data, offset + 4)[0]
        content_size = content_words * 2
        content = offset + 8
        shape_type = struct.unpack_from("<i", data, content)[0]
        if shape_type == 0:
            records.append([])
            offset = content + content_size
            continue
        if shape_type != 5:
            raise ValueError(f"expected Polygon shapefile records, got shape type {shape_type}")

        num_parts = struct.unpack_from("<i", data, content + 36)[0]
        num_points = struct.unpack_from("<i", data, content + 40)[0]
        parts_offset = content + 44
        parts = list(struct.unpack_from(f"<{num_parts}i", data, parts_offset))
        points_offset = parts_offset + 4 * num_parts
        points = [
            struct.unpack_from("<dd", data, points_offset + 16 * i) for i in range(num_points)
        ]

        rings: list[list[tuple[float, float]]] = []
        for i, first in enumerate(parts):
            last = parts[i + 1] if i + 1 < len(parts) else num_points
            ring = points[first:last]
            if len(ring) >= 2 and ring[0] == ring[-1]:
                ring = ring[:-1]
            if len(ring) >= 3:
                rings.append(ring)
        records.append(rings)
        offset = content + content_size
    return records


def _shared_strings(zf: zipfile.ZipFile) -> list[str]:
    ns = {"m": "http://schemas.openxmlformats.org/spreadsheetml/2006/main"}
    root = ET.fromstring(zf.read("xl/sharedStrings.xml"))
    values = []
    for si in root.findall("m:si", ns):
        values.append("".join(t.text or "" for t in si.findall(".//m:t", ns)))
    return values


def _cell_value(cell: ET.Element, shared_strings: list[str]) -> str:
    ns = {"m": "http://schemas.openxmlformats.org/spreadsheetml/2006/main"}
    value = cell.find("m:v", ns)
    text = "" if value is None or value.text is None else value.text
    if cell.attrib.get("t") == "s" and text:
        return shared_strings[int(text)]
    return text


def _population_by_state(work: Path) -> dict[str, int]:
    xlsx_path = work / "NST-EST2025-POP.xlsx"
    _download(POPULATION_XLSX_URL, xlsx_path)

    ns = {"m": "http://schemas.openxmlformats.org/spreadsheetml/2006/main"}
    populations: dict[str, int] = {}
    with zipfile.ZipFile(xlsx_path) as zf:
        shared_strings = _shared_strings(zf)
        sheet = ET.fromstring(zf.read("xl/worksheets/sheet1.xml"))
        for row in sheet.findall(".//m:row", ns):
            values = {
                cell.attrib.get("r", "")[0]: _cell_value(cell, shared_strings)
                for cell in row.findall("m:c", ns)
            }
            name = values.get("A", "")
            if not name.startswith("."):
                continue
            clean_name = name.lstrip(".").strip()
            population_text = values.get("H", "")
            if clean_name and population_text.isdigit():
                populations[clean_name] = int(population_text)
    return populations


def _albers_contiguous_us(lon: float, lat: float) -> tuple[float, float]:
    # Spherical Albers equal-area parameters used for the continental United States.
    radius = 6371007.181
    phi = math.radians(lat)
    lam = math.radians(lon)
    phi1 = math.radians(29.5)
    phi2 = math.radians(45.5)
    phi0 = math.radians(23.0)
    lam0 = math.radians(-96.0)
    n = 0.5 * (math.sin(phi1) + math.sin(phi2))
    c = math.cos(phi1) ** 2 + 2.0 * n * math.sin(phi1)
    rho = radius * math.sqrt(max(0.0, c - 2.0 * n * math.sin(phi))) / n
    rho0 = radius * math.sqrt(max(0.0, c - 2.0 * n * math.sin(phi0))) / n
    theta = n * (lam - lam0)
    return rho * math.sin(theta), rho0 - rho * math.cos(theta)


def _lerp(a: int, b: int, t: float) -> int:
    return int(round(a + (b - a) * t))


def _ramp_color(t: float) -> tuple[int, int, int, int]:
    t = min(1.0, max(0.0, t))
    scaled = t * (len(RAMP) - 1)
    i = min(len(RAMP) - 2, int(math.floor(scaled)))
    f = scaled - i
    return tuple(_lerp(RAMP[i][c], RAMP[i + 1][c], f) for c in range(4))  # type: ignore[return-value]


def _prepare_records(work: Path) -> tuple[list[dict[str, Any]], list[dict[str, Any]], list[tuple[float, float]]]:
    zip_path = work / "cb_2024_us_state_20m.zip"
    _download(BOUNDARY_URL, zip_path)
    with zipfile.ZipFile(zip_path) as zf:
        zf.extractall(work / "boundary")

    shp_path = next((work / "boundary").glob("*.shp"))
    dbf_path = next((work / "boundary").glob("*.dbf"))
    attributes = _read_dbf(dbf_path)
    polygons = _read_shp_polygons(shp_path)
    populations = _population_by_state(work)
    if len(attributes) != len(polygons):
        raise ValueError("shapefile record count does not match DBF record count")

    projected: list[dict[str, Any]] = []
    all_points: list[tuple[float, float]] = []
    for attrs, rings_lonlat in zip(attributes, polygons):
        statefp = attrs["STATEFP"]
        if statefp in EXCLUDED_STATEFP:
            continue
        population = populations.get(attrs["NAME"])
        if population is None:
            continue
        aland_m2 = float(attrs["ALAND"])
        if aland_m2 <= 0:
            continue

        rings_projected: list[list[tuple[float, float]]] = []
        for ring in rings_lonlat:
            out_ring = [_albers_contiguous_us(lon, lat) for lon, lat in ring]
            if len(out_ring) >= 3:
                rings_projected.append(out_ring)
                all_points.extend(out_ring)
        if not rings_projected:
            continue

        area_km2 = aland_m2 / 1_000_000.0
        density = population / area_km2
        projected.append(
            {
                "statefp": statefp,
                "name": attrs["NAME"],
                "aland_m2": aland_m2,
                "area_km2": area_km2,
                "population": population,
                "density_people_km2": density,
                "density_log10": math.log10(density),
                "rings": rings_projected,
            }
        )
    projected.sort(key=lambda row: row["statefp"])
    return projected, [], all_points


def _normalize(records: list[dict[str, Any]], all_points: list[tuple[float, float]]) -> None:
    xs = [p[0] for p in all_points]
    ys = [p[1] for p in all_points]
    xmin, xmax = min(xs), max(xs)
    ymin, ymax = min(ys), max(ys)
    cx = 0.5 * (xmin + xmax)
    cy = 0.5 * (ymin + ymax)
    scale = 3.20 / max(xmax - xmin, ymax - ymin)

    for rec in records:
        normalized_rings = []
        weighted_x = 0.0
        weighted_y = 0.0
        weight = 0
        for ring in rec["rings"]:
            out_ring = [((x - cx) * scale, (y - cy) * scale) for x, y in ring]
            normalized_rings.append(out_ring)
            for x, y in out_ring:
                weighted_x += x
                weighted_y += y
                weight += 1
        rec["rings"] = normalized_rings
        rec["centroid"] = (weighted_x / weight, weighted_y / weight)


def _write_bundle(records: list[dict[str, Any]], bundle_root: Path) -> None:
    prepared = bundle_root / "prepared"
    if prepared.exists():
        shutil.rmtree(prepared)
    prepared.mkdir(parents=True, exist_ok=True)

    values = [rec["density_log10"] for rec in records]
    value_min = min(values)
    value_max = max(values)

    ring_records: list[tuple[int, int, int]] = []
    point_records: list[tuple[float, float]] = []
    ring_fill_records: list[tuple[int, int, int, int]] = []
    ring_stroke_records: list[tuple[int, int, int, int]] = []
    ring_width_records: list[float] = []
    ring_id_records: list[int] = []
    region_records: list[dict[str, Any]] = []
    stroke_color = (14, 24, 31, 230)
    for region_index, rec in enumerate(records):
        ring_first = len(ring_records)
        point_count = 0
        t = (rec["density_log10"] - value_min) / (value_max - value_min)
        color = _ramp_color(t)
        for ring in rec["rings"]:
            point_first = len(point_records)
            point_records.extend(ring)
            ring_records.append((region_index, point_first, len(ring)))
            ring_fill_records.append(color)
            ring_stroke_records.append(stroke_color)
            ring_width_records.append(1.35)
            ring_id_records.append(int(rec["statefp"]))
            point_count += len(ring)
        region_records.append(
            {
                **rec,
                "ring_first": ring_first,
                "ring_count": len(rec["rings"]),
                "point_count": point_count,
                "color": color,
            }
        )

    xs = [p[0] for p in point_records]
    ys = [p[1] for p in point_records]

    metadata_path = prepared / "metadata.tsv"
    metadata = {
        "region_count": len(region_records),
        "ring_count": len(ring_records),
        "point_count": len(point_records),
        "xmin": min(xs),
        "xmax": max(xs),
        "ymin": min(ys),
        "ymax": max(ys),
        "value_min": value_min,
        "value_max": value_max,
        "density_min": min(rec["density_people_km2"] for rec in records),
        "density_max": max(rec["density_people_km2"] for rec in records),
    }
    with metadata_path.open("w", encoding="utf-8") as f:
        for key, value in metadata.items():
            f.write(f"{key}\t{value}\n")

    points_path = prepared / "points_xy_f64.bin"
    with points_path.open("wb") as f:
        for x, y in point_records:
            f.write(struct.pack("<dd", x, y))

    rings_path = prepared / "rings_u32.bin"
    with rings_path.open("wb") as f:
        for region_index, point_first, point_count in ring_records:
            f.write(struct.pack("<III", region_index, point_first, point_count))

    fill_path = prepared / "ring_fill_rgba8.bin"
    with fill_path.open("wb") as f:
        for color in ring_fill_records:
            f.write(struct.pack("<4B", *color))

    stroke_path = prepared / "ring_stroke_rgba8.bin"
    with stroke_path.open("wb") as f:
        for color in ring_stroke_records:
            f.write(struct.pack("<4B", *color))

    width_path = prepared / "ring_width_f32.bin"
    with width_path.open("wb") as f:
        for width in ring_width_records:
            f.write(struct.pack("<f", width))

    id_path = prepared / "ring_id_u64.bin"
    with id_path.open("wb") as f:
        for region_id in ring_id_records:
            f.write(struct.pack("<Q", region_id))

    with (prepared / "regions.tsv").open("w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f, delimiter="\t")
        writer.writerow(
            [
                "statefp",
                "name",
                "population_2025",
                "area_km2",
                "density_people_km2",
                "density_log10",
                "ring_count",
                "point_count",
            ]
        )
        for rec in region_records:
            writer.writerow(
                [
                    rec["statefp"],
                    rec["name"],
                    rec["population"],
                    f"{rec['area_km2']:.6f}",
                    f"{rec['density_people_km2']:.9f}",
                    f"{rec['density_log10']:.9f}",
                    rec["ring_count"],
                    rec["point_count"],
                ]
            )

    artifacts = [
        artifact(
            metadata_path,
            bundle_root,
            "choropleth_metadata",
            "tsv",
            region_count=len(region_records),
            ring_count=len(ring_records),
            point_count=len(point_records),
        ),
        artifact(points_path, bundle_root, "points_xy", "binary_f64", columns=2),
        artifact(rings_path, bundle_root, "rings", "binary_u32", columns=3),
        artifact(fill_path, bundle_root, "ring_fill", "binary_rgba8", columns=4),
        artifact(stroke_path, bundle_root, "ring_stroke", "binary_rgba8", columns=4),
        artifact(width_path, bundle_root, "ring_width", "binary_f32"),
        artifact(id_path, bundle_root, "ring_id", "binary_u64"),
        artifact(prepared / "regions.tsv", bundle_root, "region_metadata", "tsv"),
    ]
    status = "committed" if bundle_root.is_relative_to(REPO_ROOT / "data") else "cache"
    output_arg = ["--output", relpath(bundle_root, REPO_ROOT)]
    write_manifest(
        bundle_root,
        example_id=EXAMPLE_ID,
        title="Contiguous U.S. State Population Density Choropleth",
        status=status,
        script=relpath(Path(__file__), REPO_ROOT),
        command=command_argv(relpath(Path(__file__), REPO_ROOT), output_arg),
        source={
            "boundary": {
                "name": "U.S. Census Bureau 2024 Cartographic Boundary File, States, 1:20m",
                "url": BOUNDARY_URL,
            },
            "population": {
                "name": "U.S. Census Bureau Vintage 2025 state resident population estimates",
                "url": POPULATION_XLSX_URL,
            },
            "license": "U.S. Census Bureau public data; cite the Census Bureau as source.",
        },
        artifacts=artifacts,
        validation={
            "region_count": len(region_records),
            "ring_count": len(ring_records),
            "point_count": len(point_records),
            "value": "log10(population_2025 / land_area_km2)",
            "excluded_statefp": sorted(EXCLUDED_STATEFP),
            "prepared_layout": "flat typed arrays: metadata.tsv, points_xy_f64.bin, rings_u32.bin, ring style arrays",
        },
    )
    write_provenance(
        bundle_root,
        title="Contiguous U.S. State Population Density Choropleth",
        source_lines=[
            f"Boundaries: U.S. Census Bureau 2024 Cartographic Boundary File, states, 1:20m, `{BOUNDARY_URL}`.",
            f"Population: U.S. Census Bureau Vintage 2025 state resident population estimates, `{POPULATION_XLSX_URL}`.",
        ],
        processing_lines=[
            "Filtered to the 48 contiguous U.S. states; Alaska, Hawaii, District of Columbia, Puerto Rico, and territories are excluded to avoid inset layout in the first gallery target.",
            "Projected longitude/latitude rings with a spherical Albers equal-area transform for the contiguous United States.",
            "Normalized projected coordinates into scene space and encoded each shapefile ring as one polygon-set region.",
            "Computed population density from 2025 resident population estimates divided by Census `ALAND` square meters.",
            "Stored `log10(people per km2)` as the displayed scalar value.",
            "Wrote flat typed arrays in `prepared/` so the C example only loads render-ready polygon-set data.",
        ],
        license_lines=[
            "U.S. Census Bureau public data may be reused; cite the Census Bureau as the source of original data.",
            "Generated Datoviz prepared files are derived from those public source datasets.",
        ],
        notes=[
            "Interior holes are not preserved in the prepared polygon-set bundle; source rings are rendered as independent filled regions.",
            "Generated media should cite the U.S. Census Bureau boundary and population sources.",
        ],
    )


def prepare(output: Path, keep_work: bool) -> None:
    work = output / "source"
    if work.exists():
        shutil.rmtree(work)
    work.mkdir(parents=True, exist_ok=True)
    records, _, all_points = _prepare_records(work)
    _normalize(records, all_points)
    _write_bundle(records, output)
    if not keep_work:
        shutil.rmtree(work)
    print(f"wrote {relpath(output, REPO_ROOT)}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--keep-work", action="store_true")
    args = parser.parse_args()
    prepare(args.output, args.keep_work)


if __name__ == "__main__":
    main()
