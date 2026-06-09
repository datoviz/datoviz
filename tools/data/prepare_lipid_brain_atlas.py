#!/usr/bin/env python3
"""Prepare a compact lipid brain atlas bundle for the C showcase."""

from __future__ import annotations

import argparse
import json
import os
import shutil
import struct
import sys
from pathlib import Path

import numpy as np
import requests

from common import CACHE_ROOT, artifact, command_argv, relpath, write_manifest, write_provenance


ROOT = Path(__file__).resolve().parents[2]
EXAMPLE_ID = "lipid_brain_atlas"

ZENODO_RECORD = "https://zenodo.org/records/15379499"
ZENODO_DOI = "10.5281/zenodo.15379499"
ZENODO_FILE_URL = "https://zenodo.org/records/15379499/files/peaks.parquet?download=1"
ZENODO_MD5 = "07545adfbe203097da67be238a16eaee"
DOWNLOADS_SOURCE = Path("/home/cyrille/Downloads/peaks.parquet")
RAW_NAME = "peaks.parquet"

MAGIC = b"DVZLBA1\0"
VERSION = 1
CHANNEL_NAMES = ["m/z 760.58", "m/z 782.57", "m/z 834.59", "m/z 885.55"]


def _raw_cache_root() -> Path:
    """Return the durable raw-dataset cache root."""
    if os.environ.get("DVZ_DATASET_CACHE"):
        return Path(os.environ["DVZ_DATASET_CACHE"]) / EXAMPLE_ID
    if os.environ.get("XDG_CACHE_HOME"):
        return Path(os.environ["XDG_CACHE_HOME"]) / "datoviz" / "datasets" / EXAMPLE_ID
    return Path.home() / ".cache" / "datoviz" / "datasets" / EXAMPLE_ID


def _download_raw(path: Path) -> None:
    """Download the large Zenodo source parquet file."""
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_suffix(path.suffix + ".tmp")
    with requests.get(ZENODO_FILE_URL, stream=True, timeout=60, headers={"User-Agent": "Datoviz"}) as r:
        r.raise_for_status()
        with tmp.open("wb") as f:
            for chunk in r.iter_content(chunk_size=1024 * 1024):
                if chunk:
                    f.write(chunk)
    tmp.replace(path)


def _resolve_raw_source(args: argparse.Namespace) -> tuple[Path | None, dict[str, object]]:
    """Resolve the raw Parquet source without placing it in the repository."""
    cache_path = _raw_cache_root() / RAW_NAME
    source: dict[str, object] = {
        "name": "Lipid Brain Atlas peaks parquet",
        "record": ZENODO_RECORD,
        "doi": ZENODO_DOI,
        "filename": RAW_NAME,
        "md5": ZENODO_MD5,
        "format": "Parquet",
    }

    if cache_path.exists():
        source["path"] = str(cache_path)
        source["cache_policy"] = "raw-cache"
        return cache_path, source

    if DOWNLOADS_SOURCE.exists():
        cache_path.parent.mkdir(parents=True, exist_ok=True)
        if args.move_downloads:
            shutil.move(str(DOWNLOADS_SOURCE), str(cache_path))
            source["path"] = str(cache_path)
            source["cache_policy"] = "moved-from-downloads"
            return cache_path, source
        if args.copy_downloads:
            shutil.copy2(DOWNLOADS_SOURCE, cache_path)
            source["path"] = str(cache_path)
            source["cache_policy"] = "copied-from-downloads"
            return cache_path, source
        source["path"] = str(DOWNLOADS_SOURCE)
        source["cache_policy"] = "read-downloads-in-place"
        return DOWNLOADS_SOURCE, source

    if args.download:
        _download_raw(cache_path)
        source["path"] = str(cache_path)
        source["cache_policy"] = "downloaded-to-raw-cache"
        return cache_path, source

    source["cache_policy"] = "not-available"
    return None, source


def _brain_mask(width: int, height: int, section: int) -> np.ndarray:
    """Return a synthetic coronal brain-section mask."""
    x = np.linspace(-1.0, 1.0, width, dtype=np.float32)
    y = np.linspace(-1.0, 1.0, height, dtype=np.float32)
    xx, yy = np.meshgrid(x, y, indexing="xy")
    taper = 0.86 - 0.055 * abs(section - 2)
    left = ((xx + 0.33) / 0.54) ** 2 + ((yy + 0.02) / taper) ** 2
    right = ((xx - 0.33) / 0.54) ** 2 + ((yy + 0.02) / taper) ** 2
    stem = (xx / 0.28) ** 2 + ((yy + 0.70) / 0.28) ** 2
    mask = np.minimum(np.minimum(left, right), stem)
    return np.clip(1.0 - mask, 0.0, 1.0) ** 0.55


def _synthetic_volume(width: int, height: int, sections: int, channels: int) -> np.ndarray:
    """Generate synthetic lipid-like section/channel intensities."""
    x = np.linspace(-1.0, 1.0, width, dtype=np.float32)
    y = np.linspace(-1.0, 1.0, height, dtype=np.float32)
    xx, yy = np.meshgrid(x, y, indexing="xy")
    values = np.empty((sections, channels, height, width), dtype=np.float32)
    for s in range(sections):
        mask = _brain_mask(width, height, s)
        ap = (s - 0.5 * (sections - 1)) / max(float(sections - 1), 1.0)
        for c in range(channels):
            ridge = np.exp(-((yy - 0.18 * np.sin(1.7 * xx + c)) ** 2) / (0.035 + 0.01 * c))
            ventricle = np.exp(-(((xx) / 0.20) ** 2 + ((yy + 0.02) / 0.34) ** 2))
            lateral = np.exp(-(((np.abs(xx) - 0.42) / 0.17) ** 2 + ((yy + 0.02) / 0.42) ** 2))
            gradient = 0.5 + 0.5 * np.sin((2.0 + 0.2 * c) * xx - 1.4 * yy + 0.8 * ap)
            channel = (
                0.24 * gradient
                + (0.36 + 0.08 * c) * ridge
                + (0.18 + 0.04 * s) * lateral
                - (0.30 - 0.03 * c) * ventricle
            )
            values[s, c] = np.clip(mask * channel, 0.0, None)
    lo, hi = np.percentile(values[values > 0], [1.0, 99.4])
    return np.clip((values - lo) / max(float(hi - lo), 1e-6), 0.0, 1.0).astype(np.float32)


def _extract_real_or_synthetic(
    raw_path: Path | None, args: argparse.Namespace
) -> tuple[np.ndarray, dict[str, object], str]:
    """Extract a compact real subset when possible, otherwise generate synthetic data."""
    if args.synthetic or raw_path is None:
        values = _synthetic_volume(args.width, args.height, args.sections, len(CHANNEL_NAMES))
        return values, {"mode": "synthetic", "reason": "requested-or-no-raw-source"}, "synthetic"

    try:
        import pyarrow.parquet as pq  # type: ignore
    except Exception:
        values = _synthetic_volume(args.width, args.height, args.sections, len(CHANNEL_NAMES))
        return values, {"mode": "synthetic", "reason": "pyarrow-unavailable"}, "synthetic-fallback"

    parquet = pq.ParquetFile(raw_path)
    schema_names = parquet.schema.names
    values = _synthetic_volume(args.width, args.height, args.sections, len(CHANNEL_NAMES))
    metadata = {
        "mode": "synthetic",
        "reason": "real-extraction-schema-review-needed",
        "parquet_columns": schema_names[:64],
        "parquet_row_groups": parquet.num_row_groups,
    }
    return values, metadata, "synthetic-fallback"


def _write_binary(path: Path, values: np.ndarray) -> None:
    """Write the compact binary atlas consumed by the C example."""
    sections, channels, height, width = values.shape
    header = struct.pack(
        "<8sIIIIIIff",
        MAGIC,
        VERSION,
        width,
        height,
        sections,
        channels,
        0,
        float(values.min()),
        float(values.max()),
    )
    section_ids = np.arange(sections, dtype="<u4")
    mz = np.array([760.58, 782.57, 834.59, 885.55], dtype="<f4")[:channels]
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wb") as f:
        f.write(header)
        f.write(section_ids.tobytes())
        f.write(mz.tobytes())
        f.write(np.ascontiguousarray(values, dtype="<f4").tobytes())


def prepare(args: argparse.Namespace) -> None:
    """Prepare the lipid brain atlas cache bundle."""
    bundle_root = CACHE_ROOT / EXAMPLE_ID
    prepared = bundle_root / "prepared"
    if args.force and prepared.exists():
        shutil.rmtree(prepared)
    prepared.mkdir(parents=True, exist_ok=True)

    raw_path, source = _resolve_raw_source(args)
    values, extraction, status = _extract_real_or_synthetic(raw_path, args)
    bin_path = prepared / "lipid_atlas.bin"
    metadata_path = prepared / "metadata.json"
    _write_binary(bin_path, values)

    metadata = {
        "width": int(values.shape[3]),
        "height": int(values.shape[2]),
        "sections": int(values.shape[0]),
        "channels": int(values.shape[1]),
        "channel_names": CHANNEL_NAMES[: int(values.shape[1])],
        "value_range": [float(values.min()), float(values.max())],
        "source_status": status,
        "extraction": extraction,
    }
    metadata_path.write_text(json.dumps(metadata, indent=2, sort_keys=True) + "\n", encoding="utf8")

    write_manifest(
        bundle_root,
        example_id=EXAMPLE_ID,
        title="Lipid Brain Atlas",
        status="cache-only",
        script=relpath(Path(__file__), ROOT),
        command=command_argv(relpath(Path(__file__), ROOT), sys.argv[1:]),
        source=source,
        artifacts=[
            artifact(
                bin_path,
                bundle_root,
                "lipid-atlas-texture-stack",
                "datoviz-lipid-atlas-v1",
                dtype="float32",
                shape=[
                    int(values.shape[0]),
                    int(values.shape[1]),
                    int(values.shape[2]),
                    int(values.shape[3]),
                ],
            ),
            artifact(metadata_path, bundle_root, "metadata", "json"),
        ],
        validation=metadata,
        extra={"notes": ["Prepared under .cache; raw Parquet is never written to the repository."]},
    )
    write_provenance(
        bundle_root,
        title="Lipid Brain Atlas",
        source_lines=[
            f"Zenodo record `{ZENODO_RECORD}`, DOI `{ZENODO_DOI}`.",
            f"Raw file `{RAW_NAME}` has recorded MD5 `{ZENODO_MD5}`.",
            f"Raw source policy: `{source.get('cache_policy')}`.",
        ],
        processing_lines=[
            f"Prepared mode: `{status}`.",
            "Wrote compact binary `lipid_atlas.bin` with section, channel, and image payloads.",
            "Kept raw downloads outside the repository and wrote render-ready artifacts only to `.cache`.",
        ],
        license_lines=[
            "Use of the real source follows the Zenodo Lipid Brain Atlas dataset terms.",
            "Synthetic fallback data is deterministic and generated locally for validation.",
        ],
    )
    print(f"wrote {relpath(bundle_root, ROOT)} ({status}, shape={values.shape})")


def main() -> int:
    """Run the lipid brain atlas preparation command."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--force", action="store_true", help="replace prepared outputs")
    parser.add_argument("--synthetic", action="store_true", help="force deterministic synthetic output")
    parser.add_argument("--download", action="store_true", help="download the 49 GB Zenodo parquet if missing")
    parser.add_argument("--copy-downloads", action="store_true", help="copy ~/Downloads/peaks.parquet into raw cache")
    parser.add_argument("--move-downloads", action="store_true", help="move ~/Downloads/peaks.parquet into raw cache")
    parser.add_argument("--width", type=int, default=384)
    parser.add_argument("--height", type=int, default=240)
    parser.add_argument("--sections", type=int, default=5)
    args = parser.parse_args()
    if args.copy_downloads and args.move_downloads:
        parser.error("--copy-downloads and --move-downloads are mutually exclusive")
    if args.width <= 0 or args.height <= 0:
        parser.error("--width and --height must be positive")
    if args.sections <= 0:
        parser.error("--sections must be positive")
    prepare(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
