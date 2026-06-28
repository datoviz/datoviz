#!/usr/bin/env python3
"""Convert compact MSDF atlas payloads to direct C byte arrays."""

from __future__ import annotations

import argparse
import base64
import re
from pathlib import Path


PAYLOAD_RE = re.compile(
    r"static const char (DVZ_TEXT_DEFAULT_MSDF_[0-9]+_RGBA_Z)_B64\[\] =\n"
    r"((?:\s*\"[A-Za-z0-9+/=]*\")+)\s*;",
    re.MULTILINE,
)


def _format_bytes(data: bytes) -> str:
    lines = []
    for offset in range(0, len(data), 12):
        chunk = data[offset : offset + 12]
        values = ", ".join(f"0x{value:02x}" for value in chunk)
        lines.append(f"    {values},")
    return "\n".join(lines)


def _convert_payload(match: re.Match[str]) -> str:
    symbol = match.group(1)
    literals = "".join(re.findall(r'"([A-Za-z0-9+/=]*)"', match.group(2)))
    data = base64.b64decode(literals, validate=True)
    return f"static const uint8_t {symbol}[] = {{\n{_format_bytes(data)}\n}};"


def convert(input_path: Path, output_path: Path) -> None:
    text = input_path.read_text(encoding="utf8")
    text = text.replace(
        "Pixel payloads are base64 zlib-compressed RGBA8.",
        "Pixel payloads are zlib-compressed RGBA8 byte arrays.",
    )
    text, count = PAYLOAD_RE.subn(_convert_payload, text)
    if count == 0:
        raise RuntimeError(f"no MSDF atlas payloads found in {input_path}")
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(text, encoding="utf8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    convert(args.input, args.output)


if __name__ == "__main__":
    main()
