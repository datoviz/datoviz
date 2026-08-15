import json
import struct
import tempfile
import unittest
from pathlib import Path

from tools.check_text_atlas_manifest import ManifestError, parse_include, serialize_products, validate_manifest


def _glyph(codepoint: int) -> dict:
    return {
        "codepoint": codepoint,
        "glyph_id": codepoint - 31,
        "advance_bits": _bits(8.0),
        "xoff_bits": _bits(0.0),
        "yoff_bits": _bits(0.0),
        "width_bits": _bits(8.0),
        "height_bits": _bits(8.0),
        "plane_bounds_bits": [_bits(value) for value in (0.0, 0.0, 8.0, 8.0)],
        "atlas_bounds_bits": [_bits(value) for value in (0.0, 0.0, 8.0, 8.0)],
        "uv_bits": [_bits(value) for value in (0.0, 0.0, 1.0, 1.0)],
        "valid": True,
    }


def _bits(value: float) -> str:
    return f"0x{struct.unpack('<I', struct.pack('<f', value))[0]:08x}"


class TextAtlasManifestTests(unittest.TestCase):
    def test_existing_source_include_is_parseable(self):
        path = Path(__file__).parents[2] / "src/scene/text/generated/text_default_msdf_atlas.inc"
        products = parse_include(path)
        self.assertEqual(set(products), {32, 64, 128})
        self.assertEqual(len(products[32]["codepoints"]), 95)
        self.assertEqual(products[32]["codepoints"][0], 32)
        self.assertEqual(len(products[32]["raw"]), 214 * 214 * 4)

    def test_serialize_and_validate_neutral_products(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "font.ttf"
            source.write_bytes(b"font fixture")
            neutral_dir = root / "neutral"
            neutral_dir.mkdir()
            glyphs = [_glyph(32), _glyph(33)]
            products = []
            for size in (32, 64, 128):
                raw_path = neutral_dir / f"atlas_{size}.rgba"
                raw_path.write_bytes(bytes([size]) * (size * size * 4))
                products.append({
                    "requested": {"em_px_bits": _bits(size), "distance_range_px_bits": _bits(size / 8), "flags": 0},
                    "backend": {"name": "msdf", "value": 1},
                    "encoding": {"name": "msdf_rgb", "value": 1},
                    "glyphs": glyphs,
                    "glyph_count": 2,
                    "coverage_count": 2,
                    "fallback_mapping_count": 0,
                    "coverage": [
                        {"requested_codepoint": codepoint, "resolved_codepoint": codepoint, "glyph_index": index, "kind": "exact", "font_role": "primary"}
                        for index, codepoint in enumerate((32, 33))
                    ],
                    "metrics": {"em_px_bits": _bits(size), "distance_range_px_bits": _bits(size / 8), "ascent_bits": _bits(1.0), "descent_bits": _bits(-1.0), "line_gap_bits": _bits(0.0), "line_height_bits": _bits(2.0)},
                    "dimensions": {"width": size, "height": size, "channels": 4},
                    "rgba": {"filename": raw_path.name, "size": size * size * 4, "pixel_format": "rgba8", "row_order": "top_to_bottom"},
                })
            neutral = {
                "schema_version": 1,
                "format": "datoviz_text_atlas_product",
                "product": "default_msdf_atlas",
                "generation": {"tool": "datoviz_text_atlas_generate", "tool_version": 1, "command_template": "fixture", "canonical_thread_count": 1, "dependencies": {"msdf_atlas_gen": "1", "msdfgen": "1", "freetype": "1"}, "build": {"compiler": "fixture", "system": "fixture", "processor": "fixture"}},
                "sources": {"primary": {"path": str(source), "byte_size": len(b"font fixture"), "face_index": 0, "load_flags": 0}, "fallback": None},
                "glyph_set": {"codepoints": [32, 33]},
                "budget": {"max_glyphs": 256, "max_dimension": 4096, "max_rgba_bytes": 67108864},
                "recipe": {
                    "thread_count": 1, "fallback_codepoint": 63, "edge_coloring_seed": 0,
                    "max_corner_angle": "3", "miter_limit": "1", "overlap_support": True,
                    "scanline_pass": True, "preprocess_geometry": True, "enable_kerning": True,
                },
                "products": products,
            }
            (neutral_dir / "product.json").write_text(json.dumps(neutral), encoding="utf8")
            output = root / "output"
            manifest, include = serialize_products(neutral_dir, output, root)
            self.assertEqual(["32", "64", "128"], [str(s) for s in (32, 64, 128)])
            self.assertEqual(len(validate_manifest(manifest, output)), 3)
            self.assertEqual(parse_include(include)[128]["constants"]["WIDTH"], 128)
            self.assertFalse(include.read_bytes().endswith(b"\n\n"))

            tampered = json.loads(manifest.read_text(encoding="utf8"))
            tampered["products"][0]["rgba"]["size"] += 1
            manifest.write_text(json.dumps(tampered), encoding="utf8")
            with self.assertRaises(ManifestError):
                validate_manifest(manifest, output)


if __name__ == "__main__":
    unittest.main()
