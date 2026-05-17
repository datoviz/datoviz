#!/usr/bin/env python3
"""Prepare a compact public PhysioNet ECG example excerpt."""

from __future__ import annotations

import argparse
import csv
import json
import struct
import urllib.request
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
OUT_DIR = ROOT / "data" / "examples" / "external_public" / "bio" / "mitdb_100_excerpt"

BASE_URL = "https://physionet.org/files/mitdb/1.0.0"
HEADER_URL = f"{BASE_URL}/100.hea"
DAT_URL = f"{BASE_URL}/100.dat"
ATR_URL = f"{BASE_URL}/100.atr"


def _download_bytes(url: str) -> bytes:
    with urllib.request.urlopen(url, timeout=90) as response:
        return response.read()


def _decode_212(raw: bytes, nsamp: int) -> list[tuple[int, int]]:
    samples = []
    for i in range(0, min(len(raw), nsamp * 3), 3):
        if i + 2 >= len(raw):
            break
        b0, b1, b2 = raw[i], raw[i + 1], raw[i + 2]
        s0 = b0 | ((b1 & 0x0F) << 8)
        s1 = (b1 >> 4) | (b2 << 4)
        if s0 & 0x800:
            s0 -= 0x1000
        if s1 & 0x800:
            s1 -= 0x1000
        samples.append((s0, s1))
    return samples[:nsamp]


def _decode_annotations(raw: bytes, max_sample: int) -> list[dict]:
    labels = {
        1: "N",
        2: "L",
        3: "R",
        4: "a",
        5: "V",
        6: "F",
        7: "J",
        8: "A",
        9: "S",
        10: "E",
        11: "j",
        12: "/",
        13: "Q",
        14: "~",
        16: "|",
        18: "s",
        19: "T",
        20: "*",
        21: "D",
        22: '"',
        23: "=",
        24: "p",
        25: "B",
        26: "^",
        27: "t",
        28: "+",
        31: "[",
        32: "]",
        33: "!",
        34: "x",
        35: "(",
        36: ")",
        37: "r",
    }
    out = []
    sample = 0
    i = 0
    while i + 1 < len(raw):
        value = raw[i] | (raw[i + 1] << 8)
        i += 2
        code = value >> 10
        interval = value & 0x03FF
        if code == 0:
            break
        if code == 59:
            if i + 3 >= len(raw):
                break
            interval = struct.unpack("<I", raw[i : i + 4])[0]
            i += 4
            continue
        if code in {60, 61, 62, 63}:
            i += interval
            continue
        sample += interval
        if sample > max_sample:
            break
        out.append({"sample": sample, "time_s": sample / 360.0, "code": code, "label": labels.get(code, "?")})
    return out


def prepare(out_dir: Path, seconds: float) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)

    header = _download_bytes(HEADER_URL).decode("utf-8")
    dat = _download_bytes(DAT_URL)
    atr = _download_bytes(ATR_URL)

    fs_hz = 360.0
    adc_zero = [1024, 1024]
    adc_gain = [200.0, 200.0]
    nsamp = int(seconds * fs_hz)

    samples = _decode_212(dat, nsamp)
    with (out_dir / "ecg_excerpt.csv").open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=["sample", "time_s", "mlii_mv", "v5_mv"])
        writer.writeheader()
        for sample_index, (mlii_raw, v5_raw) in enumerate(samples):
            writer.writerow(
                {
                    "sample": sample_index,
                    "time_s": sample_index / fs_hz,
                    "mlii_mv": (mlii_raw - adc_zero[0]) / adc_gain[0],
                    "v5_mv": (v5_raw - adc_zero[1]) / adc_gain[1],
                }
            )

    annotations = _decode_annotations(atr, len(samples))
    with (out_dir / "annotations.csv").open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=["sample", "time_s", "code", "label"])
        writer.writeheader()
        writer.writerows(annotations)

    (out_dir / "100.hea").write_text(header, encoding="utf-8")
    metadata = {
        "dataset": "mit-bih_arrhythmia_database_record_100_excerpt",
        "source": "PhysioNet MIT-BIH Arrhythmia Database record 100",
        "source_urls": {
            "header": HEADER_URL,
            "signal": DAT_URL,
            "annotations": ATR_URL,
        },
        "license_note": "PhysioNet MIT-BIH Arrhythmia Database terms apply.",
        "record": "100",
        "sampling_rate_hz": fs_hz,
        "seconds_prepared": seconds,
        "samples_prepared": len(samples),
        "annotation_count": len(annotations),
        "channels": ["MLII", "V5"],
    }
    (out_dir / "metadata.json").write_text(
        json.dumps(metadata, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=Path, default=OUT_DIR)
    parser.add_argument("--seconds", type=float, default=10.0)
    args = parser.parse_args()
    prepare(args.out, args.seconds)


if __name__ == "__main__":
    main()
