#!/usr/bin/env python3
"""Prepare compact public geoscience example data."""

from __future__ import annotations

import argparse
import csv
import json
import math
import urllib.request
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
OUT_DIR = ROOT / "data" / "examples" / "external_public" / "geo" / "ridgecrest_usgs"

USGS_GEOJSON_URL = (
    "https://earthquake.usgs.gov/fdsnws/event/1/query?"
    "format=geojson&starttime=2019-07-04&endtime=2019-08-04"
    "&latitude=35.770&longitude=-117.599&maxradiuskm=120"
    "&minmagnitude=1.0&orderby=time-asc&limit=20000"
)

USGS_CSV_URL = USGS_GEOJSON_URL.replace("format=geojson", "format=csv")

CENTER_LON = -117.599
CENTER_LAT = 35.770


def _download_text(url: str) -> str:
    with urllib.request.urlopen(url, timeout=60) as response:
        return response.read().decode("utf-8")


def _local_xy_km(lon: float, lat: float) -> tuple[float, float]:
    lat_scale = 111.32
    lon_scale = 111.32 * math.cos(math.radians(CENTER_LAT))
    return ((lon - CENTER_LON) * lon_scale, (lat - CENTER_LAT) * lat_scale)


def prepare(out_dir: Path) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)

    geojson_text = _download_text(USGS_GEOJSON_URL)
    csv_text = _download_text(USGS_CSV_URL)

    (out_dir / "ridgecrest_usgs_comcat.geojson").write_text(geojson_text, encoding="utf-8")
    (out_dir / "ridgecrest_usgs_comcat.csv").write_text(csv_text, encoding="utf-8")

    collection = json.loads(geojson_text)
    rows = []
    for feature in collection["features"]:
        props = feature["properties"]
        lon, lat, depth_km = feature["geometry"]["coordinates"][:3]
        x_km, y_km = _local_xy_km(float(lon), float(lat))
        rows.append(
            {
                "event_id": feature["id"],
                "time_ms": int(props["time"]),
                "elapsed_s": 0.0,
                "lon": float(lon),
                "lat": float(lat),
                "depth_km": float(depth_km),
                "x_km": x_km,
                "y_km": y_km,
                "magnitude": "" if props.get("mag") is None else float(props["mag"]),
                "place": props.get("place") or "",
            }
        )

    rows.sort(key=lambda row: (row["time_ms"], row["event_id"]))
    if rows:
        t0 = rows[0]["time_ms"]
        for row in rows:
            row["elapsed_s"] = (row["time_ms"] - t0) / 1000.0

    fieldnames = [
        "event_id",
        "time_ms",
        "elapsed_s",
        "lon",
        "lat",
        "depth_km",
        "x_km",
        "y_km",
        "magnitude",
        "place",
    ]
    with (out_dir / "ridgecrest_aftershocks_prepared.csv").open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    metadata = {
        "dataset": "ridgecrest_2019_aftershocks",
        "source": "USGS ComCat FDSN Event Web Service",
        "source_geojson_url": USGS_GEOJSON_URL,
        "source_csv_url": USGS_CSV_URL,
        "license_note": "USGS-authored data are public domain unless otherwise noted by USGS.",
        "query": {
            "starttime": "2019-07-04",
            "endtime": "2019-08-04",
            "latitude": CENTER_LAT,
            "longitude": CENTER_LON,
            "maxradiuskm": 120,
            "minmagnitude": 1.0,
            "orderby": "time-asc",
            "limit": 20000,
        },
        "event_count": len(rows),
        "prepared_files": [
            "ridgecrest_usgs_comcat.geojson",
            "ridgecrest_usgs_comcat.csv",
            "ridgecrest_aftershocks_prepared.csv",
        ],
    }
    (out_dir / "metadata.json").write_text(
        json.dumps(metadata, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )

    blockers = """# Geoscience Public Data Blockers

Prepared now:

- `ridgecrest_usgs`: direct unauthenticated USGS ComCat GeoJSON/CSV query.

Deferred planned sources:

- ERA5: requires Copernicus/ECMWF data access setup and can easily become a large download.
- OpenSky historical tracks: bulk historical access is account/API-token mediated for many uses.
- Movebank: many datasets require Movebank account access, permissions, or dataset-specific terms.
"""
    (out_dir.parent / "BLOCKERS.md").write_text(blockers, encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=Path, default=OUT_DIR)
    args = parser.parse_args()
    prepare(args.out)


if __name__ == "__main__":
    main()
