#!/usr/bin/env python3
"""Prepare compact public CMS event-display example data."""

from __future__ import annotations

import argparse
import csv
import io
import json
import math
import re
import urllib.request
import zipfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
OUT_DIR = ROOT / "data" / "examples" / "external_public" / "physics" / "cms_singleelectron_2012c"

RECORD_URL = "https://opendata.cern.ch/record/7144"
API_URL = "https://opendata.cern.ch/api/records/7144"
IG_URL = "https://opendata.cern.ch/record/7144/files/SingleElectron_Run2012C_0.ig"


def _download_bytes(url: str) -> bytes:
    with urllib.request.urlopen(url, timeout=120) as response:
        return response.read()


def _load_ig_event(raw: bytes, max_events: int) -> tuple[list[str], list[dict]]:
    archive = zipfile.ZipFile(io.BytesIO(raw))
    event_names = sorted(name for name in archive.namelist() if name.startswith("Events/"))
    events = []
    for name in event_names[:max_events]:
        text = archive.read(name).decode("utf-8")
        text = re.sub(r"\bnan\b", "NaN", text)
        events.append(json.loads(text))
    return event_names, events


def _fields(types: dict, collection: str) -> list[str]:
    return [field[0] for field in types.get(collection, [])]


def _field_index(types: dict, collection: str, field: str) -> int | None:
    names = _fields(types, collection)
    try:
        return names.index(field)
    except ValueError:
        return None


def _first_number(row: list, *indices: int | None) -> float:
    for index in indices:
        if index is None or index >= len(row):
            continue
        value = row[index]
        if isinstance(value, (int, float)) and math.isfinite(float(value)):
            return float(value)
    return float("nan")


def prepare(out_dir: Path, max_events: int) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)

    record = json.loads(_download_bytes(API_URL).decode("utf-8"))
    ig_raw = _download_bytes(IG_URL)
    (out_dir / "SingleElectron_Run2012C_0.ig").write_bytes(ig_raw)

    event_names, events = _load_ig_event(ig_raw, max_events)

    event_rows = []
    track_rows = []
    calo_rows = []
    jet_rows = []
    met_rows = []

    for event_index, event in enumerate(events):
        types = event["Types"]
        collections = event.get("Collections", {})
        event_info = collections.get("Event_V2", [[]])[0]
        run = int(event_info[0]) if len(event_info) > 0 else -1
        event_id = int(event_info[1]) if len(event_info) > 1 else -1
        event_rows.append(
            {
                "event_index": event_index,
                "run": run,
                "event": event_id,
                "source_path": event_names[event_index],
            }
        )

        for collection in ["Tracks_V3", "GsfTracks_V1", "GlobalMuons_V1", "TrackerMuons_V1"]:
            eta_i = _field_index(types, collection, "eta")
            phi_i = _field_index(types, collection, "phi")
            pt_i = _field_index(types, collection, "pt")
            charge_i = _field_index(types, collection, "charge")
            for row in collections.get(collection, []):
                track_rows.append(
                    {
                        "event_index": event_index,
                        "family": collection,
                        "eta": _first_number(row, eta_i),
                        "phi": _first_number(row, phi_i),
                        "pt": _first_number(row, pt_i),
                        "charge": _first_number(row, charge_i),
                    }
                )

        for collection in ["EBRecHits_V2", "EERecHits_V2", "HBRecHits_V2", "HERecHits_V2"]:
            eta_i = _field_index(types, collection, "eta")
            phi_i = _field_index(types, collection, "phi")
            energy_i = _field_index(types, collection, "energy")
            for row in collections.get(collection, [])[:2000]:
                calo_rows.append(
                    {
                        "event_index": event_index,
                        "family": collection,
                        "eta": _first_number(row, eta_i),
                        "phi": _first_number(row, phi_i),
                        "energy": _first_number(row, energy_i),
                    }
                )

        for collection in ["Jets_V1", "PFJets_V1"]:
            eta_i = _field_index(types, collection, "eta")
            phi_i = _field_index(types, collection, "phi")
            pt_i = _field_index(types, collection, "pt")
            et_i = _field_index(types, collection, "et")
            energy_i = _field_index(types, collection, "energy")
            for row in collections.get(collection, []):
                jet_rows.append(
                    {
                        "event_index": event_index,
                        "family": collection,
                        "eta": _first_number(row, eta_i),
                        "phi": _first_number(row, phi_i),
                        "pt": _first_number(row, pt_i, et_i),
                        "energy": _first_number(row, energy_i),
                    }
                )

        for collection in ["METs_V1", "PFMETs_V1"]:
            phi_i = _field_index(types, collection, "phi")
            energy_i = _field_index(types, collection, "energy")
            pt_i = _field_index(types, collection, "pt")
            for row in collections.get(collection, []):
                met_rows.append(
                    {
                        "event_index": event_index,
                        "family": collection,
                        "phi": _first_number(row, phi_i),
                        "magnitude": _first_number(row, energy_i, pt_i),
                    }
                )

    def write_csv(name: str, rows: list[dict], fieldnames: list[str]) -> None:
        with (out_dir / name).open("w", newline="", encoding="utf-8") as f:
            writer = csv.DictWriter(f, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(rows)

    write_csv("events.csv", event_rows, ["event_index", "run", "event", "source_path"])
    write_csv("tracks.csv", track_rows, ["event_index", "family", "eta", "phi", "pt", "charge"])
    write_csv("calo_hits.csv", calo_rows, ["event_index", "family", "eta", "phi", "energy"])
    write_csv("jets.csv", jet_rows, ["event_index", "family", "eta", "phi", "pt", "energy"])
    write_csv("met.csv", met_rows, ["event_index", "family", "phi", "magnitude"])

    metadata = {
        "dataset": "cms_singleelectron_2012c_event_display",
        "source": "CERN Open Data Portal record 7144",
        "source_record_url": RECORD_URL,
        "source_api_url": API_URL,
        "source_file_url": IG_URL,
        "license": record["metadata"]["license"]["attribution"],
        "experiment": "CMS",
        "collision_energy": "8TeV",
        "run_period": "Run2012C",
        "source_file": "SingleElectron_Run2012C_0.ig",
        "source_file_size": len(ig_raw),
        "events_available": len(event_names),
        "events_prepared": len(events),
        "track_rows": len(track_rows),
        "calo_hit_rows": len(calo_rows),
        "jet_rows": len(jet_rows),
        "met_rows": len(met_rows),
    }
    (out_dir / "metadata.json").write_text(
        json.dumps(metadata, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=Path, default=OUT_DIR)
    parser.add_argument("--max-events", type=int, default=3)
    args = parser.parse_args()
    prepare(args.out, args.max_events)


if __name__ == "__main__":
    main()
