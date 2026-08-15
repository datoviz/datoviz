#!/usr/bin/env python3
"""Validate the deterministic MSDF atlas manifest and textual include.

The generator writes the manifest; this module deliberately has no dependency
on Datoviz or a font/atlas generator.  It is therefore suitable for a fast
source-tree integrity check in CI and in source distributions.
"""

from __future__ import annotations

import argparse
import base64
import hashlib
import json
import re
import struct
import sys
import zlib
from pathlib import Path
from typing import Any


DEFAULT_SIZES = (32, 64, 128)
_UINT_RE = re.compile(r"static const uint32_t (DVZ_TEXT_DEFAULT_MSDF_(\d+)_(\w+)) = (\d+)u;")
_FLOAT_RE = re.compile(
    r"static const float (DVZ_TEXT_DEFAULT_MSDF_(\d+)_(\w+)) = "
    r"([-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?)f;"
)
_PAYLOAD_RE = re.compile(
    r"static const char DVZ_TEXT_DEFAULT_MSDF_(\d+)_RGBA_Z_B64\[\] =\s*"
    r"((?:\s*\"[A-Za-z0-9+/=]*\")+)\s*;",
    re.MULTILINE,
)
_GLYPHS_RE = re.compile(
    r"static const DvzTextAtlasGlyph DVZ_TEXT_DEFAULT_MSDF_(\d+)_GLYPHS\[\] = \{"
    r"(.*?)\n\};",
    re.DOTALL,
)
_CODEPOINT_RE = re.compile(r"\{(\d+)u,")


class ManifestError(ValueError):
    """Raised when a manifest or generated include is inconsistent."""


def _sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _json_sha256(value: Any) -> str:
    """Hash one value using canonical compact JSON."""
    data = json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode()
    return _sha256(data)


def _f32(value: Any) -> str:
    """Format a binary32 value as a round-trippable C floating constant."""
    text = format(float(value), ".9g")
    if "." not in text and "e" not in text.lower():
        text += ".0"
    return text + "f"


def _bits_float(value: Any) -> float:
    """Decode a neutral product's exact IEEE-754 binary32 hex-bit string."""
    if isinstance(value, str) and value.lower().startswith("0x"):
        return struct.unpack("<f", struct.pack("<I", int(value, 16)))[0]
    if isinstance(value, dict) and "bits" in value:
        return _bits_float(value["bits"])
    return float(value)


def _bits_hex(value: Any) -> str:
    if isinstance(value, str) and value.lower().startswith("0x"):
        return f"0x{int(value, 16):08x}"
    return f"0x{struct.unpack('<I', struct.pack('<f', float(value)))[0]:08x}"


def _glyph_line(glyph: dict[str, Any]) -> str:
    def scalar(name: str, *aliases: str) -> Any:
        for key in (name,) + aliases:
            if key in glyph:
                return _bits_float(glyph[key])
        raise ManifestError(f"glyph is missing {name!r}")

    def vector(name: str) -> list[float]:
        value = glyph.get(name)
        if not isinstance(value, list) or len(value) != 4:
            raise ManifestError(f"glyph field {name!r} must contain four values")
        return [_bits_float(item) for item in value]

    values = [
        f"{int(glyph['codepoint'])}u",
        f"{int(glyph.get('glyph_id', glyph.get('atlas_index', 0)))}u",
        _f32(scalar("advance", "advance_bits")),
        _f32(scalar("xoff_bits", "offset_x")),
        _f32(scalar("yoff_bits", "offset_y")),
        _f32(scalar("width_bits", "width")),
        _f32(scalar("height_bits", "height")),
    ]
    for field in ("plane_bounds_bits", "atlas_bounds_bits", "uv_bits"):
        if field not in glyph:
            field = {"plane_bounds_bits": "bounds", "atlas_bounds_bits": "atlas_bounds", "uv_bits": "uv_bounds"}[field]
        values.append("{" + ", ".join(_f32(value) for value in vector(field)) + "}")
    values.append("true" if glyph.get("valid", True) else "false")
    return "    {" + ", ".join(values) + "},"


def _serialize_final_products(neutral: dict[str, Any], input_dir: Path, output_dir: Path, repo_root: Path | None) -> tuple[Path, Path]:
    """Serialize the version-1 neutral product emitted by the C++ generator."""
    if neutral.get("schema_version") != 1:
        raise ManifestError("neutral product schema_version must be 1")
    if neutral.get("format") != "datoviz_text_atlas_product":
        raise ManifestError("neutral product format is unsupported")
    if neutral.get("product") != "default_msdf_atlas":
        raise ManifestError("neutral product identity is unsupported")
    glyph_set = dict(_required(neutral, "glyph_set", "neutral product"))
    glyphs = list(_required(glyph_set, "codepoints", "glyph_set"))
    if glyphs != sorted(set(glyphs)):
        raise ManifestError("glyph_set codepoints must be sorted and unique")
    recipe = dict(_required(neutral, "recipe", "neutral product"))
    products = _required(neutral, "products", "neutral product")
    if not isinstance(products, list):
        raise ManifestError("neutral products must be an array")
    sizes = []
    for item in products:
        requested = item.get("requested", {})
        sizes.append(int(round(_bits_float(requested["em_px_bits"]))))
    if sorted(sizes) != list(DEFAULT_SIZES):
        raise ManifestError("products must encode requested sizes 32/64/128")
    ranges = sorted(
        int(round(_bits_float(item["requested"]["distance_range_px_bits"])))
        for item in products
    )
    if ranges != [4, 8, 16]:
        raise ManifestError("products must encode requested ranges 4/8/16")
    output_dir.mkdir(parents=True, exist_ok=True)
    include_lines = ["/* Generated deterministic MSDF atlas. */", "/* Pixel payloads are base64 zlib-compressed RGBA8. */", ""]
    manifest_products = []
    for product in sorted(products, key=lambda item: int(round(_bits_float(item["requested"]["em_px_bits"])))):
        size = int(round(_bits_float(product["requested"]["em_px_bits"])))
        dims = product["dimensions"]
        width, height = int(dims["width"]), int(dims["height"])
        if int(dims.get("channels", 0)) != 4:
            raise ManifestError(f"product {size} must contain four RGBA channels")
        if product.get("backend", {}).get("name") != "msdf":
            raise ManifestError(f"product {size} backend must be msdf")
        if product.get("encoding", {}).get("name") != "msdf_rgb":
            raise ManifestError(f"product {size} encoding must be msdf_rgb")
        rgba = dict(_required(product, "rgba", f"product {size}"))
        if rgba.get("pixel_format") != "rgba8" or rgba.get("row_order") != "top_to_bottom":
            raise ManifestError(f"product {size} has unsupported RGBA encoding")
        raw_path = input_dir / _check_string(_required(rgba, "filename", f"product {size} rgba"), f"product {size} rgba.filename")
        raw = raw_path.read_bytes()
        if len(raw) != width * height * 4 or int(_required(rgba, "size", f"product {size} rgba")) != len(raw):
            raise ManifestError(f"product {size} RGBA size does not match dimensions")
        glyph_records = product.get("glyphs", [])
        if [int(g["codepoint"]) for g in glyph_records] != glyphs:
            raise ManifestError(f"product {size} glyph records differ from glyph_set")
        coverage = product.get("coverage", [])
        if not isinstance(coverage, list) or len(coverage) != len(glyphs):
            raise ManifestError(f"product {size} coverage must contain every requested glyph")
        if int(product.get("glyph_count", -1)) != len(glyph_records):
            raise ManifestError(f"product {size} glyph_count is inconsistent")
        if int(product.get("coverage_count", -1)) != len(coverage):
            raise ManifestError(f"product {size} coverage_count is inconsistent")
        if int(product.get("fallback_mapping_count", -1)) != 0:
            raise ManifestError(f"product {size} printable ASCII must not use fallback mappings")
        for index, item in enumerate(coverage):
            if (
                int(item.get("requested_codepoint", -1)) != glyphs[index]
                or int(item.get("resolved_codepoint", -1)) != glyphs[index]
                or item.get("kind") != "exact"
                or item.get("font_role") != "primary"
            ):
                raise ManifestError(f"product {size} coverage record {index} is not exact primary coverage")
        prefix = f"DVZ_TEXT_DEFAULT_MSDF_{size}"
        metrics = dict(product.get("metrics", {}))
        include_lines += [f"static const uint32_t {prefix}_WIDTH = {width}u;", f"static const uint32_t {prefix}_HEIGHT = {height}u;", f"static const uint32_t {prefix}_GLYPH_COUNT = {len(glyph_records)}u;"]
        for key, output_key in (("em_px_bits", "EM_PX"), ("distance_range_px_bits", "RANGE_PX"), ("ascent_bits", "ASCENT"), ("descent_bits", "DESCENT"), ("line_gap_bits", "LINE_GAP"), ("line_height_bits", "LINE_HEIGHT")):
            if key in metrics:
                include_lines.append(f"static const float {prefix}_{output_key} = {_f32(_bits_float(metrics[key]))};")
        include_lines.append(f"static const DvzTextAtlasGlyph {prefix}_GLYPHS[] = {{")
        include_lines.extend(_glyph_line(glyph) for glyph in glyph_records)
        compressed = zlib.compress(raw, level=9)
        include_lines += ["};", f"static const uint32_t {prefix}_RGBA_SIZE = {len(raw)}u;", f"static const uint32_t {prefix}_RGBA_Z_SIZE = {len(compressed)}u;", f"static const char {prefix}_RGBA_Z_B64[] ="]
        encoded = base64.b64encode(compressed).decode("ascii")
        include_lines.extend(f'"{encoded[offset:offset + 120]}"' for offset in range(0, len(encoded), 120))
        include_lines += [";", ""]
        manifest_products.append(
            {
                "requested": product["requested"],
                "backend": product["backend"],
                "encoding": product["encoding"],
                "dimensions": dims,
                "metrics": metrics,
                "glyph_count": len(glyph_records),
                "coverage_count": len(coverage),
                "fallback_mapping_count": 0,
                "glyph_records_sha256": _json_sha256(glyph_records),
                "coverage_records_sha256": _json_sha256(coverage),
                "rgba": {**rgba, "sha256": _sha256(raw)},
                "compressed": {"size": len(compressed), "sha256": _sha256(compressed)},
                "include_symbol": f"{prefix}_RGBA_Z_B64",
            }
        )
    include_bytes = ("\n".join(include_lines) + "\n").encode("utf8")
    include_path = output_dir / "src/scene/text/generated/text_default_msdf_atlas.inc"
    include_path.parent.mkdir(parents=True, exist_ok=True)
    include_path.write_bytes(include_bytes)
    sources = dict(_required(neutral, "sources", "neutral product"))
    for source in sources.values():
        if not isinstance(source, dict):
            continue
        path = Path(source["path"])
        if not path.is_absolute():
            path = (repo_root or input_dir.parent) / path
        data = path.read_bytes()
        if int(source.get("byte_size", -1)) != len(data):
            raise ManifestError(f"source {source.get('path', '<unknown>')} byte_size is inconsistent")
        source["size"], source["sha256"] = len(data), _sha256(data)
    manifest = {"schema_version": 1, "format": "datoviz-msdf-atlas", "product": "default_msdf_atlas", "generation": neutral["generation"], "sources": sources, "glyph_set": glyph_set, "budget": neutral["budget"], "recipe": recipe, "products": manifest_products, "include": {"path": "src/scene/text/generated/text_default_msdf_atlas.inc", "sha256": _sha256(include_bytes)}}
    manifest_path = output_dir / "assets/runtime/text/default_msdf_atlas.json"
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf8")
    return manifest_path, include_path


def serialize_products(input_dir: Path, output_dir: Path, repo_root: Path | None = None) -> tuple[Path, Path]:
    """Serialize a neutral C++ product directory into an include and manifest.

    The directory contains ``product.json`` and one raw RGBA file per size as
    named by each product's ``raw_path``.  The neutral JSON is intentionally
    the hand-off between the developer-only C++ generator and this Python
    serializer; no scene or GPU code is imported here.
    """
    neutral_path = input_dir / "product.json"
    neutral = _read_json(neutral_path)
    return _serialize_final_products(neutral, input_dir, output_dir, repo_root)


def _required(mapping: dict[str, Any], key: str, context: str) -> Any:
    if key not in mapping:
        raise ManifestError(f"{context} is missing {key!r}")
    return mapping[key]


def _read_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ManifestError(f"cannot read manifest {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise ManifestError("manifest root must be an object")
    return value


def parse_include(path: Path) -> dict[int, dict[str, Any]]:
    """Extract dimensions, glyph coverage, and compressed RGBA payloads."""
    try:
        text = path.read_text(encoding="utf8")
    except OSError as exc:
        raise ManifestError(f"cannot read include {path}: {exc}") from exc
    products: dict[int, dict[str, Any]] = {}
    for match in _UINT_RE.finditer(text):
        symbol, size_text, field, value_text = match.groups()
        size = int(size_text)
        products.setdefault(size, {}).setdefault("constants", {})[field] = int(value_text)
    for match in _FLOAT_RE.finditer(text):
        _, size_text, field, value_text = match.groups()
        size = int(size_text)
        products.setdefault(size, {}).setdefault("constants", {})[field] = float(value_text)
    for match in _GLYPHS_RE.finditer(text):
        size = int(match.group(1))
        codepoints = [int(value) for value in _CODEPOINT_RE.findall(match.group(2))]
        products.setdefault(size, {})["codepoints"] = codepoints
    for match in _PAYLOAD_RE.finditer(text):
        size = int(match.group(1))
        literals = "".join(re.findall(r'"([A-Za-z0-9+/=]*)"', match.group(2)))
        try:
            compressed = base64.b64decode(literals, validate=True)
            raw = zlib.decompress(compressed)
        except (ValueError, zlib.error) as exc:
            raise ManifestError(f"invalid payload for atlas size {size}: {exc}") from exc
        products.setdefault(size, {})["compressed"] = compressed
        products[size]["raw"] = raw
    return products


def _check_string(value: Any, field: str) -> str:
    if not isinstance(value, str) or not value:
        raise ManifestError(f"{field} must be a non-empty string")
    return value


def validate_manifest(manifest_path: Path, repo_root: Path, include_root: Path | None = None) -> list[str]:
    """Validate *manifest_path* and return human-readable product summaries."""
    manifest = _read_json(manifest_path)
    if _required(manifest, "schema_version", "manifest") != 1:
        raise ManifestError("unsupported manifest schema_version")
    if _required(manifest, "product", "manifest") != "default_msdf_atlas":
        raise ManifestError("manifest product must be default_msdf_atlas")
    if manifest.get("format") != "datoviz-msdf-atlas":
        raise ManifestError("manifest format must be datoviz-msdf-atlas")
    generation = _required(manifest, "generation", "manifest")
    if not isinstance(generation, dict):
        raise ManifestError("generation must be an object")
    for field in ("tool", "tool_version", "command_template", "canonical_thread_count", "dependencies", "build"):
        _required(generation, field, "generation")
    if generation["tool"] != "datoviz_text_atlas_generate" or int(generation["tool_version"]) != 1:
        raise ManifestError("generation tool identity is unsupported")
    if int(generation["canonical_thread_count"]) != 1:
        raise ManifestError("canonical generation must use one thread")
    dependencies = generation["dependencies"]
    if not isinstance(dependencies, dict):
        raise ManifestError("generation dependencies must be an object")
    for field in ("msdf_atlas_gen", "msdfgen", "freetype"):
        _check_string(_required(dependencies, field, "generation dependencies"), f"generation.dependencies.{field}")
    build = generation["build"]
    if not isinstance(build, dict):
        raise ManifestError("generation build identity must be an object")
    for field in ("compiler", "system", "processor"):
        _check_string(_required(build, field, "generation build"), f"generation.build.{field}")
    sources = _required(manifest, "sources", "manifest")
    if not isinstance(sources, dict) or not isinstance(sources.get("primary"), dict):
        raise ManifestError("sources.primary must be an object")
    source = sources["primary"]
    glyph_set = _required(manifest, "glyph_set", "manifest")
    if not isinstance(glyph_set, dict):
        raise ManifestError("glyph_set must be an object")
    recipe = _required(manifest, "recipe", "manifest")
    declared_products = _required(manifest, "products", "manifest")
    include = _required(manifest, "include", "manifest")
    if not isinstance(source, dict) or not isinstance(recipe, dict) or not isinstance(include, dict):
        raise ManifestError("source, recipe, and include must be objects")
    font_path = repo_root / _check_string(_required(source, "path", "source"), "source.path")
    if not font_path.is_file():
        raise ManifestError(f"font does not exist: {font_path}")
    font_bytes = font_path.read_bytes()
    if len(font_bytes) != int(_required(source, "size", "source")):
        raise ManifestError("font byte size differs from manifest")
    if _sha256(font_bytes) != _check_string(_required(source, "sha256", "source"), "source.sha256"):
        raise ManifestError("font SHA-256 differs from manifest")
    for role, candidate in sources.items():
        if candidate is None:
            continue
        if not isinstance(candidate, dict):
            raise ManifestError(f"source {role} must be an object")
        candidate_path = repo_root / _check_string(_required(candidate, "path", f"source {role}"), f"source {role}.path")
        if not candidate_path.is_file():
            raise ManifestError(f"source {role} does not exist: {candidate_path}")
        candidate_bytes = candidate_path.read_bytes()
        if len(candidate_bytes) != int(_required(candidate, "byte_size", f"source {role}")):
            raise ManifestError(f"source {role} generator byte_size differs from manifest")
        if len(candidate_bytes) != int(_required(candidate, "size", f"source {role}")):
            raise ManifestError(f"source {role} byte size differs from manifest")
        if _sha256(candidate_bytes) != _check_string(_required(candidate, "sha256", f"source {role}"), f"source {role}.sha256"):
            raise ManifestError(f"source {role} SHA-256 differs from manifest")
    sizes = DEFAULT_SIZES
    for field in ("thread_count", "fallback_codepoint", "edge_coloring_seed", "max_corner_angle", "miter_limit", "overlap_support", "scanline_pass", "preprocess_geometry", "enable_kerning"):
        _required(recipe, field, "recipe")
    if (
        int(recipe["thread_count"]) != 1
        or int(recipe["fallback_codepoint"]) != 63
        or int(recipe["edge_coloring_seed"]) != 0
        or float(recipe["max_corner_angle"]) != 3.0
        or float(recipe["miter_limit"]) != 1.0
        or not all(
            bool(recipe[field])
            for field in ("overlap_support", "scanline_pass", "preprocess_geometry", "enable_kerning")
        )
    ):
        raise ManifestError("generation recipe differs from the approved canonical recipe")
    budget = _required(manifest, "budget", "manifest")
    if not isinstance(budget, dict) or int(_required(budget, "max_dimension", "manifest budget")) != 4096:
        raise ManifestError("recipe budget max_dimension must be 4096")
    if int(_required(budget, "max_rgba_bytes", "manifest budget")) != 64 * 1024 * 1024:
        raise ManifestError("recipe budget max_rgba_bytes must be 67108864")
    if int(_required(budget, "max_glyphs", "manifest budget")) != 256:
        raise ManifestError("recipe budget max_glyphs must be 256")
    for field in ("face_index", "load_flags"):
        _required(source, field, "source")
    glyphs = _required(glyph_set, "codepoints", "glyph_set")
    if not isinstance(glyphs, list) or glyphs != sorted(set(glyphs)):
        raise ManifestError("recipe glyphs must be sorted and unique")
    include_path = (include_root or repo_root) / _check_string(_required(include, "path", "include"), "include.path")
    include_bytes = include_path.read_bytes()
    if _sha256(include_bytes) != _check_string(_required(include, "sha256", "include"), "include.sha256"):
        raise ManifestError("textual include SHA-256 differs from manifest")
    parsed = parse_include(include_path)
    if set(parsed) != set(sizes):
        raise ManifestError(f"include atlas sizes {sorted(parsed)} differ from recipe {list(sizes)}")
    if not isinstance(declared_products, list) or len(declared_products) != len(sizes):
        raise ManifestError("products must contain one record per recipe size")
    by_size = {}
    for product in declared_products:
        if not isinstance(product, dict):
            raise ManifestError("each product record must be an object")
        requested = _required(product, "requested", "product")
        size = int(round(_bits_float(_required(requested, "em_px_bits", "product requested"))))
        if size in by_size:
            raise ManifestError(f"duplicate product size {size}")
        by_size[size] = product
    if set(by_size) != set(sizes):
        raise ManifestError("product sizes differ from recipe sizes")
    summaries = []
    for size in sizes:
        actual = parsed[size]
        product = by_size[size]
        for field in (
            "metrics",
            "glyph_records_sha256",
            "coverage_records_sha256",
            "rgba",
            "compressed",
        ):
            _required(product, field, f"product {size}")
        constants = actual.get("constants", {})
        codepoints = actual.get("codepoints", [])
        compressed = actual.get("compressed")
        raw = actual.get("raw")
        if compressed is None or raw is None or "codepoints" not in actual:
            raise ManifestError(f"atlas size {size} is missing constants, glyphs, or payload")
        if codepoints != glyphs:
            raise ManifestError(f"atlas size {size} glyph coverage differs from recipe")
        dimensions = _required(product, "dimensions", f"product {size}")
        width = int(_required(dimensions, "width", f"product {size} dimensions"))
        height = int(_required(dimensions, "height", f"product {size} dimensions"))
        if int(_required(dimensions, "channels", f"product {size} dimensions")) != 4:
            raise ManifestError(f"atlas size {size} channel count differs from manifest")
        if constants.get("WIDTH") != width or constants.get("HEIGHT") != height:
            raise ManifestError(f"atlas size {size} dimensions differ from manifest")
        if constants.get("GLYPH_COUNT") != len(codepoints):
            raise ManifestError(f"atlas size {size} glyph count constant is incorrect")
        requested = product["requested"]
        if int(round(_bits_float(requested["distance_range_px_bits"]))) != size // 8:
            raise ManifestError(f"atlas size {size} requested range is incorrect")
        if product.get("backend", {}).get("name") != "msdf":
            raise ManifestError(f"atlas size {size} backend differs from manifest")
        if product.get("encoding", {}).get("name") != "msdf_rgb":
            raise ManifestError(f"atlas size {size} encoding differs from manifest")
        metric_constants = {
            "em_px_bits": "EM_PX",
            "distance_range_px_bits": "RANGE_PX",
            "ascent_bits": "ASCENT",
            "descent_bits": "DESCENT",
            "line_gap_bits": "LINE_GAP",
            "line_height_bits": "LINE_HEIGHT",
        }
        for metric, constant in metric_constants.items():
            metric_value = _required(product["metrics"], metric, f"product {size} metrics")
            if _bits_hex(constants.get(constant)) != _bits_hex(metric_value):
                raise ManifestError(f"atlas size {size} metric {metric} differs from include")
        if int(product.get("glyph_count", -1)) != len(glyphs):
            raise ManifestError(f"atlas size {size} glyph count is inconsistent")
        if int(product.get("coverage_count", -1)) != len(glyphs):
            raise ManifestError(f"atlas size {size} coverage count is inconsistent")
        if int(product.get("fallback_mapping_count", -1)) != 0:
            raise ManifestError(f"atlas size {size} unexpectedly uses fallback mappings")
        _check_string(product["glyph_records_sha256"], f"product {size}.glyph_records_sha256")
        _check_string(product["coverage_records_sha256"], f"product {size}.coverage_records_sha256")
        rgba_record = _required(product, "rgba", f"product {size}")
        if rgba_record.get("pixel_format") != "rgba8" or rgba_record.get("row_order") != "top_to_bottom":
            raise ManifestError(f"atlas size {size} RGBA encoding differs from manifest")
        raw_record = rgba_record
        compressed_record = _required(product, "compressed", f"product {size}")
        if int(_required(raw_record, "size", f"product {size} raw")) != len(raw):
            raise ManifestError(f"atlas size {size} raw byte size differs from manifest")
        if constants.get("RGBA_SIZE") != len(raw):
            raise ManifestError(f"atlas size {size} raw byte size constant is incorrect")
        if _check_string(_required(raw_record, "sha256", f"product {size} raw"), f"product {size} raw.sha256") != _sha256(raw):
            raise ManifestError(f"atlas size {size} raw SHA-256 differs from manifest")
        if int(_required(compressed_record, "size", f"product {size} compressed")) != len(compressed):
            raise ManifestError(f"atlas size {size} compressed byte size differs from manifest")
        if constants.get("RGBA_Z_SIZE") != len(compressed):
            raise ManifestError(f"atlas size {size} compressed byte size constant is incorrect")
        if _check_string(_required(compressed_record, "sha256", f"product {size} compressed"), f"product {size} compressed.sha256") != _sha256(compressed):
            raise ManifestError(f"atlas size {size} compressed SHA-256 differs from manifest")
        expected_raw_size = width * height * 4
        if len(raw) != expected_raw_size:
            raise ManifestError(f"atlas size {size} raw payload is not width*height*4")
        summaries.append(f"{size}: {width}x{height}, {len(codepoints)} glyphs, {len(raw)} raw bytes")
    return summaries


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path)
    parser.add_argument("--serialize", action="store_true", help="serialize a neutral product directory")
    parser.add_argument("--input-dir", type=Path, help="neutral product directory containing product.json")
    parser.add_argument("--output-dir", type=Path, help="candidate output directory")
    parser.add_argument("--repo-root", type=Path, default=Path.cwd())
    parser.add_argument("--include-root", type=Path, help="root for the generated include (defaults to --repo-root)")
    args = parser.parse_args(argv)
    try:
        if args.serialize:
            if args.input_dir is None or args.output_dir is None:
                parser.error("--serialize requires --input-dir and --output-dir")
            manifest_path, include_path = serialize_products(args.input_dir, args.output_dir, args.repo_root)
            print(f"text-atlas serialization: OK ({manifest_path}, {include_path})")
            return 0
        if args.manifest is None:
            parser.error("--manifest is required unless --serialize is used")
        summaries = validate_manifest(args.manifest, args.repo_root, args.include_root)
    except (ManifestError, OSError, ValueError) as exc:
        print(f"text-atlas manifest check: FAIL: {exc}", file=sys.stderr)
        return 1
    print(f"text-atlas manifest check: OK ({args.manifest})")
    for summary in summaries:
        print(f"  {summary}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
