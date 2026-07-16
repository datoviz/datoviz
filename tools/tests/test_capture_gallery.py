#!/usr/bin/env python3
"""Focused tests for canonical gallery capture dimensions."""

from __future__ import annotations

from pathlib import Path
import sys
import tempfile
import unittest

from PIL import Image


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import capture_gallery  # noqa: E402


class NormalizePngSizeTest(unittest.TestCase):
    """Validate exact, HiDPI, and invalid capture dimensions."""

    def _image(self, root: Path, size: tuple[int, int]) -> Path:
        path = root / "capture.png"
        image = Image.new("RGBA", size, (18, 24, 33, 255))
        image.paste(
            (22, 210, 230, 255),
            (size[0] // 4, size[1] // 4, size[0] // 2, size[1] // 2),
        )
        image.save(path)
        return path

    def test_exact_size_is_unchanged(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = self._image(Path(tmp), (1280, 720))
            before = path.read_bytes()

            ok, detail = capture_gallery.normalize_png_size(path, (1280, 720))

            self.assertTrue(ok, detail)
            self.assertEqual(detail, "1280x720")
            self.assertEqual(path.read_bytes(), before)

    def test_fractional_hidpi_capture_is_downsampled(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = self._image(Path(tmp), (1920, 1080))

            ok, detail = capture_gallery.normalize_png_size(path, (1280, 720))

            self.assertTrue(ok, detail)
            self.assertEqual(detail, "normalized 1920x1080 -> 1280x720")
            with Image.open(path) as image:
                self.assertEqual(image.size, (1280, 720))

    def test_retina_capture_is_downsampled(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = self._image(Path(tmp), (2560, 1440))

            ok, detail = capture_gallery.normalize_png_size(path, (1280, 720))

            self.assertTrue(ok, detail)
            self.assertEqual(detail, "normalized 2560x1440 -> 1280x720")
            with Image.open(path) as image:
                self.assertEqual(image.size, (1280, 720))

    def test_aspect_mismatch_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = self._image(Path(tmp), (1920, 1200))

            ok, detail = capture_gallery.normalize_png_size(path, (1280, 720))

            self.assertFalse(ok)
            self.assertIn("does not match canonical aspect ratio", detail)
            with Image.open(path) as image:
                self.assertEqual(image.size, (1920, 1200))

    def test_undersized_capture_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = self._image(Path(tmp), (640, 360))

            ok, detail = capture_gallery.normalize_png_size(path, (1280, 720))

            self.assertFalse(ok)
            self.assertIn("is smaller than canonical", detail)
            with Image.open(path) as image:
                self.assertEqual(image.size, (640, 360))


if __name__ == "__main__":
    unittest.main()
