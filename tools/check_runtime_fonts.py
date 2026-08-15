#!/usr/bin/env python3
"""Validate admitted runtime font identities, metadata, licenses, and coverage."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import sys
import unicodedata

try:
    from fontTools.ttLib import TTFont
except ImportError as exc:
    raise SystemExit("fontTools is required for the runtime font admission check") from exc


ROOT = Path(__file__).resolve().parents[1]
FONT_DIR = ROOT / "assets" / "runtime" / "fonts"


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _font_name(font: TTFont, name_id: int) -> str:
    names = font["name"]
    value = names.getName(name_id, 3, 1) or names.getName(name_id, 1, 0)
    return value.toUnicode() if value is not None else ""


def _font_codepoints(font: TTFont) -> set[int]:
    codepoints: set[int] = set()
    for table in font["cmap"].tables:
        if table.isUnicode():
            codepoints.update(table.cmap)
    return codepoints


def _parse_codepoint(value: str) -> int:
    if not value.startswith("U+"):
        raise ValueError(f"invalid codepoint {value!r}")
    return int(value[2:], 16)


def _parse_range(value: str) -> range:
    start, end = value.split("-", 1)
    return range(_parse_codepoint(start), _parse_codepoint(end) + 1)


def main() -> int:
    manifest = json.loads((FONT_DIR / "manifest.json").read_text(encoding="utf-8"))
    coverage = json.loads((FONT_DIR / "coverage.json").read_text(encoding="utf-8"))
    errors: list[str] = []
    role_codepoints: dict[str, set[int]] = {}
    declared_files: set[str] = set()

    for entry in manifest["files"]:
        relative = entry["path"]
        declared_files.add(relative)
        path = FONT_DIR / relative
        if not path.is_file():
            errors.append(f"missing font: {relative}")
            continue
        size = path.stat().st_size
        if size != entry["size"]:
            errors.append(f"size mismatch for {relative}: {size} != {entry['size']}")
        digest = _sha256(path)
        if digest != entry["sha256"]:
            errors.append(f"SHA-256 mismatch for {relative}: {digest}")
        font = TTFont(path, lazy=True)
        family = _font_name(font, 1)
        style = _font_name(font, 2)
        if family != entry["family"]:
            errors.append(f"family mismatch for {relative}: {family!r}")
        if style != entry["style"]:
            errors.append(f"style mismatch for {relative}: {style!r}")
        role_codepoints[entry["role"]] = _font_codepoints(font)
        font.close()

    actual_files = {path.name for path in FONT_DIR.glob("*.ttf")}
    if actual_files != declared_files:
        errors.append(f"manifest font set mismatch: actual={sorted(actual_files)} declared={sorted(declared_files)}")

    for source in manifest["sources"].values():
        license_path = FONT_DIR / source["license_path"]
        if not license_path.is_file():
            errors.append(f"missing license: {source['license_path']}")
        elif _sha256(license_path) != source["license_sha256"]:
            errors.append(f"license SHA-256 mismatch: {source['license_path']}")

    primary = role_codepoints.get(coverage["primary_role"], set())
    combined = set(primary)
    for role in coverage["fallback_roles"]:
        if role not in role_codepoints:
            errors.append(f"unknown fallback role: {role}")
        combined.update(role_codepoints.get(role, set()))

    for string in coverage["required_primary_strings"]:
        missing = [character for character in string if ord(character) not in primary]
        if missing:
            errors.append(f"primary coverage missing: {''.join(missing)}")
    for value in coverage["required_combined_ranges"]:
        missing = [
            codepoint
            for codepoint in _parse_range(value)
            if unicodedata.category(chr(codepoint)) != "Cn" and codepoint not in combined
        ]
        if missing:
            errors.append(f"combined coverage missing {len(missing)} codepoints from {value}")
    for string in coverage["required_combined_strings"]:
        missing = [character for character in string if ord(character) not in combined]
        if missing:
            errors.append(f"combined coverage missing: {''.join(missing)}")

    fallback = _parse_codepoint(coverage["missing_glyph"])
    if fallback not in primary:
        errors.append(f"primary font lacks visible fallback {coverage['missing_glyph']}")

    if errors:
        for error in errors:
            print(f"ERROR {error}", file=sys.stderr)
        return 1
    print(f"runtime font admission OK: {len(declared_files)} fonts, {len(combined)} combined codepoints")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
