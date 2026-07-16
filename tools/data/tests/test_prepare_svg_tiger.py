"""Focused tests for the cache-only SVG tiger preparation tool."""

from __future__ import annotations

import json
import struct
import sys
import tempfile
import unittest
from pathlib import Path


TOOLS_DATA = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DATA))

import prepare_svg_tiger as tiger  # noqa: E402


SVG = """\
<svg xmlns="http://www.w3.org/2000/svg" width="100" height="80">
  <g transform="matrix(2,0,0,2,4,6)" style="fill:#123456;stroke:#abcdef;stroke-width:3">
    <path d="m 1,2 l 5,0 0,4 z"/>
    <path style="fill:none;stroke:#000000" d="M 0,0 C 0,8 8,8 8,0"/>
  </g>
</svg>
"""


class SvgTigerTests(unittest.TestCase):
    """Exercise inheritance, transforms, cubic flattening, and deterministic output."""

    def _svg(self, root: Path, text: str = SVG) -> Path:
        path = root / "fixture.svg"
        path.write_text(text, encoding="utf8")
        return path

    def test_parse_supported_subset(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            document = tiger.parse_svg(self._svg(Path(tmp)), tolerance=0.1)
        self.assertEqual((document.width, document.height), (100.0, 80.0))
        self.assertEqual(len(document.paths), 2)
        polygon, curve = document.paths
        self.assertTrue(polygon.closed)
        self.assertEqual(polygon.points, ((6.0, 10.0), (16.0, 10.0), (16.0, 18.0)))
        self.assertEqual(polygon.paint.fill, (0x12, 0x34, 0x56, 255))
        self.assertEqual(polygon.paint.stroke, (0xAB, 0xCD, 0xEF, 255))
        self.assertEqual(polygon.paint.stroke_width, 3.0)
        self.assertFalse(curve.closed)
        self.assertIsNone(curve.paint.fill)
        self.assertEqual(curve.paint.stroke, (0, 0, 0, 255))
        self.assertGreater(len(curve.points), 4)
        self.assertEqual(curve.points[0], (4.0, 6.0))
        self.assertEqual(curve.points[-1], (20.0, 6.0))

    def test_bundle_is_deterministic_and_self_describing(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = self._svg(root)
            document = tiger.parse_svg(source, tolerance=0.25)
            first = tiger.write_bundle(document, root / "one", source, 0.25)
            second = tiger.write_bundle(document, root / "two", source, 0.25)
            self.assertEqual(first.read_bytes(), second.read_bytes())
            metadata = json.loads((first.parent / "metadata.json").read_text(encoding="utf8"))
            self.assertEqual(metadata["document"]["path_count"], 2)
            self.assertEqual(metadata["artifact"]["bytes"], first.stat().st_size)

            header = tiger.HEADER.unpack_from(first.read_bytes())
            self.assertEqual(header[0], tiger.MAGIC)
            self.assertEqual(header[1], tiger.VERSION)
            self.assertEqual(header[2], 2)
            self.assertEqual(header[4], tiger.PATH_RECORD.size)
            first_record = tiger.PATH_RECORD.unpack_from(first.read_bytes(), tiger.HEADER.size)
            self.assertEqual(first_record[0:2], (0, 3))
            self.assertEqual(first_record[2:5], (1, 1, 1))

    def test_rejects_unsupported_elements_and_commands(self) -> None:
        cases = (
            '<svg width="1" height="1"><circle cx="0" cy="0" r="1"/></svg>',
            '<svg width="1" height="1"><path d="M0,0 Q1,1 2,0"/></svg>',
            '<svg width="1" height="1"><path d="M0,0 M1,1 L2,2"/></svg>',
        )
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for index, text in enumerate(cases):
                path = root / f"bad-{index}.svg"
                path.write_text(text, encoding="utf8")
                with self.subTest(index=index), self.assertRaises(tiger.SvgTigerError):
                    tiger.parse_svg(path)


if __name__ == "__main__":
    unittest.main()
