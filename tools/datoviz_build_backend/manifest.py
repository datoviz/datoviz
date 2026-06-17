"""Payload manifest support for Datoviz wheels."""

from __future__ import annotations

import json
from dataclasses import asdict, dataclass
from pathlib import Path


@dataclass(frozen=True)
class PayloadEntry:
    """One file included in a Datoviz release wheel."""

    source: str
    wheel_path: str
    kind: str
    required: bool
    reason: str
    repair_status: str = "not-repaired"


def write_manifest(entries: list[PayloadEntry], path: Path) -> None:
    """Write the payload manifest."""

    path.parent.mkdir(parents=True, exist_ok=True)
    data = {
        "schema": 1,
        "entries": [asdict(entry) for entry in sorted(entries, key=lambda item: item.wheel_path)],
    }
    path.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf8")


def read_manifest(path: Path) -> list[PayloadEntry]:
    """Read a payload manifest."""

    data = json.loads(path.read_text(encoding="utf8"))
    return [PayloadEntry(**entry) for entry in data.get("entries", [])]

