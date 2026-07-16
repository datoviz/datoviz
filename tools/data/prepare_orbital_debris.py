#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.10"
# dependencies = ["sgp4==2.25"]
# ///
"""Prepare a cache-only SGP4 ephemeris for catalogued orbital-debris objects."""

from __future__ import annotations

import argparse
import csv
import io
import json
import math
import shutil
import struct
import sys
import urllib.error
import urllib.parse
import urllib.request
from dataclasses import dataclass
from datetime import datetime, timedelta, timezone
from pathlib import Path
from typing import Any

from common import CACHE_ROOT, REPO_ROOT, artifact, relpath, write_json, write_manifest, write_provenance

try:
    from sgp4 import omm
    from sgp4.api import Satrec, jday
except ImportError as exc:  # pragma: no cover - exercised only without the inline dependency runner.
    raise SystemExit(
        "missing `sgp4`; run `uv run tools/data/prepare_orbital_debris.py`"
    ) from exc


EXAMPLE_ID = "orbital_debris"
DEFAULT_OUTPUT = CACHE_ROOT / EXAMPLE_ID
CELESTRAK_GP_URL = "https://celestrak.org/NORAD/elements/gp.php"
SOURCE_CACHE_MIN_AGE = timedelta(hours=2)

MAGIC = b"DVZORB1\0"
VERSION = 1
EARTH_EQUATORIAL_RADIUS_KM = 6378.137
SNAPSHOT_TEXT_SIZE = 32


@dataclass(frozen=True)
class DebrisEvent:
    key: str
    label: str
    query_name: str

    @property
    def url(self) -> str:
        query = urllib.parse.urlencode({"NAME": self.query_name, "FORMAT": "CSV"})
        return f"{CELESTRAK_GP_URL}?{query}"


EVENTS = (
    DebrisEvent("fengyun_1c", "FENGYUN 1C", "FENGYUN 1C DEB"),
    DebrisEvent("iridium_33", "IRIDIUM 33", "IRIDIUM 33 DEB"),
    DebrisEvent("cosmos_2251", "COSMOS 2251", "COSMOS 2251 DEB"),
)


def _utc_now() -> datetime:
    return datetime.now(timezone.utc).replace(microsecond=0)


def _iso_utc(value: datetime) -> str:
    return value.astimezone(timezone.utc).isoformat().replace("+00:00", "Z")


def _parse_utc(value: str) -> datetime:
    return datetime.fromisoformat(value.strip().replace("Z", "+00:00")).astimezone(timezone.utc)


def _raw_paths(bundle_root: Path, event: DebrisEvent) -> tuple[Path, Path]:
    source = bundle_root / "source" / f"{event.key}.csv"
    metadata = bundle_root / "source" / f"{event.key}.download.json"
    return source, metadata


def _cached_source_is_fresh(metadata_path: Path, now: datetime) -> bool:
    if not metadata_path.exists():
        return False
    try:
        metadata = json.loads(metadata_path.read_text(encoding="utf8"))
        retrieved_at = _parse_utc(str(metadata["retrieved_at_utc"]))
    except (KeyError, TypeError, ValueError, json.JSONDecodeError):
        return False
    return now - retrieved_at < SOURCE_CACHE_MIN_AGE


def _download_event(
    bundle_root: Path,
    event: DebrisEvent,
    *,
    now: datetime,
    refresh_source: bool,
    offline: bool,
) -> tuple[Path, dict[str, Any]]:
    source_path, metadata_path = _raw_paths(bundle_root, event)
    if source_path.exists() and (offline or (not refresh_source and _cached_source_is_fresh(metadata_path, now))):
        metadata = json.loads(metadata_path.read_text(encoding="utf8"))
        return source_path, metadata
    if offline:
        raise RuntimeError(f"offline mode requested but cached source is missing: {source_path}")

    request = urllib.request.Request(
        event.url,
        headers={
            "Accept": "text/csv",
            "User-Agent": "Datoviz-v0.4-orbital-debris-example/1.0",
        },
    )
    try:
        with urllib.request.urlopen(request, timeout=120) as response:
            if response.status != 200:
                raise RuntimeError(f"CelesTrak returned HTTP {response.status}; stopping")
            payload = response.read()
    except urllib.error.HTTPError as exc:
        raise RuntimeError(f"CelesTrak returned HTTP {exc.code}; stopping without retry") from exc
    except urllib.error.URLError as exc:
        raise RuntimeError(f"CelesTrak request failed; stopping without retry: {exc.reason}") from exc

    if not payload.startswith(b"OBJECT_NAME,"):
        preview = payload[:160].decode("utf8", errors="replace")
        raise RuntimeError(f"unexpected CelesTrak CSV response for {event.label}: {preview!r}")

    source_path.parent.mkdir(parents=True, exist_ok=True)
    tmp_path = source_path.with_suffix(".csv.tmp")
    tmp_path.write_bytes(payload)
    tmp_path.replace(source_path)
    metadata = {
        "event_key": event.key,
        "event_label": event.label,
        "query_name": event.query_name,
        "source_url": event.url,
        "retrieved_at_utc": _iso_utc(now),
        "bytes": len(payload),
    }
    write_json(metadata_path, metadata)
    return source_path, metadata


def _load_rows(path: Path, event_index: int) -> list[dict[str, Any]]:
    text = path.read_text(encoding="utf-8-sig")
    rows: list[dict[str, Any]] = []
    for source_row in csv.DictReader(io.StringIO(text)):
        try:
            catalog_id = int(source_row["NORAD_CAT_ID"])
            epoch = _parse_utc(source_row["EPOCH"])
        except (KeyError, TypeError, ValueError):
            continue
        rows.append(
            {
                "catalog_id": catalog_id,
                "event_index": event_index,
                "epoch": epoch,
                "name": source_row.get("OBJECT_NAME", "").strip(),
                "object_id": source_row.get("OBJECT_ID", "").strip(),
                "omm": source_row,
            }
        )
    rows.sort(key=lambda row: row["catalog_id"])
    return rows


def _balanced_limit(rows_by_event: list[list[dict[str, Any]]], max_objects: int) -> list[dict[str, Any]]:
    if max_objects <= 0 or sum(len(rows) for rows in rows_by_event) <= max_objects:
        return [row for rows in rows_by_event for row in rows]

    selected: list[dict[str, Any]] = []
    remaining = max_objects
    for event_index, rows in enumerate(rows_by_event):
        events_left = len(rows_by_event) - event_index
        target = min(len(rows), max(1, remaining // events_left))
        if target == len(rows):
            chosen = rows
        else:
            chosen = [rows[(i * len(rows)) // target] for i in range(target)]
        selected.extend(chosen)
        remaining -= len(chosen)
    return selected


def _gmst_radians(julian_date: float) -> float:
    centuries = (julian_date - 2451545.0) / 36525.0
    seconds = (
        67310.54841
        + (876600.0 * 3600.0 + 8640184.812866) * centuries
        + 0.093104 * centuries * centuries
        - 6.2e-6 * centuries * centuries * centuries
    )
    return (seconds % 86400.0) * (2.0 * math.pi / 86400.0)


def _teme_to_visual(position_km: tuple[float, float, float], julian_date: float) -> tuple[float, float, float]:
    theta = _gmst_radians(julian_date)
    cosine = math.cos(theta)
    sine = math.sin(theta)
    x_teme, y_teme, z_teme = position_km
    x_ecef = cosine * x_teme + sine * y_teme
    y_ecef = -sine * x_teme + cosine * y_teme
    z_ecef = z_teme
    scale = 1.0 / EARTH_EQUATORIAL_RADIUS_KM
    return -y_ecef * scale, z_ecef * scale, -x_ecef * scale


def _propagate(
    rows: list[dict[str, Any]],
    frame_times: list[datetime],
    max_element_age: timedelta,
) -> tuple[list[dict[str, Any]], list[list[tuple[float, float, float]]], dict[str, int]]:
    frame_jd = []
    for timestamp in frame_times:
        second = timestamp.second + timestamp.microsecond / 1_000_000.0
        jd, fraction = jday(
            timestamp.year,
            timestamp.month,
            timestamp.day,
            timestamp.hour,
            timestamp.minute,
            second,
        )
        frame_jd.append((jd, fraction))

    accepted: list[dict[str, Any]] = []
    tracks: list[list[tuple[float, float, float]]] = []
    rejected = {"stale": 0, "initialization": 0, "propagation": 0, "duplicate": 0}
    seen_catalog_ids: set[int] = set()
    snapshot = frame_times[0]
    for row in rows:
        catalog_id = int(row["catalog_id"])
        if catalog_id in seen_catalog_ids:
            rejected["duplicate"] += 1
            continue
        seen_catalog_ids.add(catalog_id)
        if abs(snapshot - row["epoch"]) > max_element_age:
            rejected["stale"] += 1
            continue

        satellite = Satrec()
        try:
            omm.initialize(satellite, row["omm"])
        except (KeyError, TypeError, ValueError):
            rejected["initialization"] += 1
            continue

        track: list[tuple[float, float, float]] = []
        failed = False
        for jd, fraction in frame_jd:
            error, position_km, _velocity_km_s = satellite.sgp4(jd, fraction)
            if error != 0 or not all(math.isfinite(value) for value in position_km):
                failed = True
                break
            track.append(_teme_to_visual(position_km, jd + fraction))
        if failed:
            rejected["propagation"] += 1
            continue
        accepted.append(row)
        tracks.append(track)
    return accepted, tracks, rejected


def _write_binary(
    path: Path,
    rows: list[dict[str, Any]],
    tracks: list[list[tuple[float, float, float]]],
    frame_times: list[datetime],
    step_seconds: float,
) -> dict[str, Any]:
    object_count = len(rows)
    frame_count = len(frame_times)
    event_count = len(EVENTS)
    snapshot_text = _iso_utc(frame_times[0]).encode("ascii")
    if len(snapshot_text) >= SNAPSHOT_TEXT_SIZE:
        raise RuntimeError("snapshot timestamp exceeds binary field")
    snapshot_field = snapshot_text + b"\0" * (SNAPSHOT_TEXT_SIZE - len(snapshot_text))
    start_unix_s = frame_times[0].timestamp()
    max_radius = max(math.sqrt(sum(value * value for value in position)) for track in tracks for position in track)

    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wb") as f:
        f.write(
            struct.pack(
                "<8sIIIIIddff32s",
                MAGIC,
                VERSION,
                object_count,
                frame_count,
                event_count,
                0,
                start_unix_s,
                step_seconds,
                EARTH_EQUATORIAL_RADIUS_KM,
                max_radius,
                snapshot_field,
            )
        )
        f.write(bytes(int(row["event_index"]) for row in rows))
        f.write(struct.pack(f"<{object_count}I", *(int(row["catalog_id"]) for row in rows)))
        for frame_index in range(frame_count):
            for track in tracks:
                f.write(struct.pack("<3f", *track[frame_index]))

    return {
        "object_count": object_count,
        "frame_count": frame_count,
        "event_count": event_count,
        "start_utc": _iso_utc(frame_times[0]),
        "step_seconds": step_seconds,
        "duration_seconds": step_seconds * (frame_count - 1),
        "earth_equatorial_radius_km": EARTH_EQUATORIAL_RADIUS_KM,
        "max_radius_earth_radii": max_radius,
        "binary_layout": "header; event_id u8[N]; NORAD catalog_id u32[N]; position f32[F,N,3]",
    }


def _write_metadata(
    path: Path,
    rows: list[dict[str, Any]],
    binary_metadata: dict[str, Any],
    source_metadata: list[dict[str, Any]],
    source_counts: list[int],
    rejected: dict[str, int],
) -> None:
    event_counts = [sum(int(row["event_index"]) == i for row in rows) for i in range(len(EVENTS))]
    write_json(
        path,
        {
            **binary_metadata,
            "coordinate_frame": (
                "TEME positions propagated with SGP4, rotated to approximate Earth-fixed coordinates "
                "using GMST, then mapped to Datoviz Y-up planet coordinates"
            ),
            "events": [
                {
                    "index": index,
                    "key": event.key,
                    "label": event.label,
                    "query_name": event.query_name,
                    "source_count": source_counts[index],
                    "prepared_count": event_counts[index],
                    "source_url": event.url,
                }
                for index, event in enumerate(EVENTS)
            ],
            "objects": [
                {
                    "catalog_id": int(row["catalog_id"]),
                    "event_index": int(row["event_index"]),
                    "name": row["name"],
                    "object_id": row["object_id"],
                    "element_epoch_utc": _iso_utc(row["epoch"]),
                }
                for row in rows
            ],
            "source_downloads": source_metadata,
            "rejected": rejected,
        },
    )


def prepare(args: argparse.Namespace) -> Path:
    bundle_root = args.output.resolve()
    prepared_root = bundle_root / "prepared"
    now = _utc_now()
    source_paths: list[Path] = []
    source_metadata: list[dict[str, Any]] = []
    rows_by_event: list[list[dict[str, Any]]] = []
    for event_index, event in enumerate(EVENTS):
        source_path, download_metadata = _download_event(
            bundle_root,
            event,
            now=now,
            refresh_source=args.refresh_source,
            offline=args.offline,
        )
        source_paths.append(source_path)
        source_metadata.append(download_metadata)
        rows_by_event.append(_load_rows(source_path, event_index))

    rows = _balanced_limit(rows_by_event, args.max_objects)
    snapshot = max(
        _parse_utc(str(metadata["retrieved_at_utc"])) for metadata in source_metadata
    ).replace(second=0, microsecond=0)
    frame_count = int(round(args.duration_minutes * 60.0 / args.step_seconds)) + 1
    frame_times = [snapshot + timedelta(seconds=i * args.step_seconds) for i in range(frame_count)]
    accepted, tracks, rejected = _propagate(
        rows,
        frame_times,
        timedelta(days=args.max_element_age_days),
    )
    if not accepted:
        raise RuntimeError("no valid catalogued objects remained after SGP4 validation")

    if prepared_root.exists() and args.force:
        shutil.rmtree(prepared_root)
    prepared_root.mkdir(parents=True, exist_ok=True)
    binary_path = prepared_root / "orbital_debris.bin"
    metadata_path = prepared_root / "metadata.json"
    binary_metadata = _write_binary(binary_path, accepted, tracks, frame_times, args.step_seconds)
    _write_metadata(
        metadata_path,
        accepted,
        binary_metadata,
        source_metadata,
        [len(rows) for rows in rows_by_event],
        rejected,
    )

    command = [
        "uv",
        "run",
        "tools/data/prepare_orbital_debris.py",
        "--output",
        relpath(bundle_root, REPO_ROOT),
    ]
    write_manifest(
        bundle_root,
        example_id=EXAMPLE_ID,
        title="Catalogued Orbital Debris",
        status="prepared-cache",
        script="tools/data/prepare_orbital_debris.py",
        command=command,
        source={
            "provider": "CelesTrak",
            "format": "OMM-compatible CSV GP element sets",
            "retrieved_at_utc": _iso_utc(now),
            "queries": [event.url for event in EVENTS],
            "raw_artifacts": [
                artifact(path, bundle_root, "source_gp_elements", "CSV") for path in source_paths
            ],
        },
        artifacts=[
            artifact(
                binary_path,
                bundle_root,
                "render_ready_ephemeris",
                "DVZORB1",
                dtype="mixed header + uint8 + uint32 + float32",
                shape=[len(frame_times), len(accepted), 3],
            ),
            artifact(metadata_path, bundle_root, "catalog_metadata", "JSON"),
        ],
        validation={
            "sgp4_valid_objects": len(accepted),
            "frame_count": len(frame_times),
            "rejected": rejected,
        },
        extra={"notes": ["Prepared under .cache; the data submodule is not modified."]},
    )
    write_provenance(
        bundle_root,
        title="Catalogued Orbital Debris",
        source_lines=[
            "CelesTrak OMM-compatible GP element sets for FENGYUN 1C, IRIDIUM 33, and COSMOS 2251 debris.",
            f"Snapshot retrieval time: {_iso_utc(now)}.",
            "CelesTrak GP documentation: https://celestrak.org/NORAD/documentation/gp-data-formats.php",
        ],
        processing_lines=[
            "Propagated each accepted GP element set with python-sgp4 2.25.",
            "Converted TEME positions to approximate Earth-fixed coordinates using GMST and normalized by 6378.137 km.",
            f"Generated {len(frame_times)} frames for {len(accepted)} catalogued objects at {args.step_seconds:g}-second intervals.",
        ],
        license_lines=[
            "CelesTrak data is freely provided subject to its usage policy: https://celestrak.org/usage-policy.php",
            "SGP4 implementation follows Vallado et al.; cite https://celestrak.org/publications/AIAA/2006-6753/",
        ],
        notes=[
            "These are tracked catalog objects from three selected debris events, not the full debris environment.",
            "The Earth-fixed conversion omits polar motion and other high-precision Earth-orientation corrections.",
            "Point sizes in the visualization are exaggerated and do not encode physical object size.",
        ],
    )
    return bundle_root


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--duration-minutes", type=float, default=120.0)
    parser.add_argument("--step-seconds", type=float, default=60.0)
    parser.add_argument("--max-element-age-days", type=float, default=14.0)
    parser.add_argument("--max-objects", type=int, default=0, help="zero keeps every valid object")
    parser.add_argument("--refresh-source", action="store_true")
    parser.add_argument("--offline", action="store_true")
    parser.add_argument("--force", action="store_true")
    return parser


def main() -> int:
    args = _parser().parse_args()
    if args.duration_minutes <= 0 or args.step_seconds <= 0 or args.max_element_age_days <= 0:
        raise SystemExit("duration, timestep, and maximum element age must be positive")
    try:
        bundle_root = prepare(args)
    except RuntimeError as exc:
        print(f"prepare_orbital_debris: {exc}", file=sys.stderr)
        return 1
    print(f"prepared real orbital-debris bundle: {relpath(bundle_root, REPO_ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
