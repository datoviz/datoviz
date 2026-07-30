#!/usr/bin/env python3
"""Focused tests for canonical gallery capture dimensions."""

from __future__ import annotations

import argparse
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

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


class VerifyExistingCaptureTest(unittest.TestCase):
    def _example(self) -> capture_gallery.CaptureExample:
        return capture_gallery.CaptureExample(
            id="features_example",
            title="Example",
            lane="features",
            source="examples/c/features/example.c",
            validation="screenshot",
            capture_mode="scenario",
            capture_reason="",
            capture_args=(),
            expected_width=1280,
            expected_height=720,
        )

    def _args(self, root: Path) -> argparse.Namespace:
        return argparse.Namespace(
            image_dir=root / "canonical",
            verification_dir=root / "verify",
            cache_dir=root / "cache",
            build_dir=root / "build",
            cache=True,
            force=False,
            dry_run=False,
            verify_existing=True,
            skip_nonblank_check=False,
        )

    def test_populates_cache_only_for_byte_identical_capture(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            example = self._example()
            args = self._args(root)
            canonical = capture_gallery.output_path(example, args.image_dir)
            canonical.parent.mkdir(parents=True)
            Image.new("RGBA", (1280, 720), (10, 20, 30, 255)).save(canonical)

            def fake_capture(
                captured_example: capture_gallery.CaptureExample,
                captured_args: argparse.Namespace,
                unused_hash: str | None,
            ) -> tuple[bool, str, str]:
                candidate = capture_gallery.output_path(
                    captured_example, captured_args.image_dir
                )
                candidate.parent.mkdir(parents=True)
                candidate.write_bytes(canonical.read_bytes())
                return True, "new", "1280x720"

            with patch.object(capture_gallery, "capture_one", side_effect=fake_capture):
                ok, status, detail = capture_gallery.verify_existing_capture(
                    example, args, "input-hash"
                )

            self.assertTrue(ok, detail)
            self.assertEqual(status, "verified")
            cache = capture_gallery.load_cache(
                capture_gallery.cache_path(example, args.cache_dir)
            )
            self.assertEqual(cache["input_hash"], "input-hash")
            self.assertEqual(cache["png_hash"], capture_gallery.file_sha256(canonical))

    def test_rejects_different_capture_without_cache_record(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            example = self._example()
            args = self._args(root)
            canonical = capture_gallery.output_path(example, args.image_dir)
            canonical.parent.mkdir(parents=True)
            Image.new("RGBA", (1280, 720), (10, 20, 30, 255)).save(canonical)

            def fake_capture(
                captured_example: capture_gallery.CaptureExample,
                captured_args: argparse.Namespace,
                unused_hash: str | None,
            ) -> tuple[bool, str, str]:
                candidate = capture_gallery.output_path(
                    captured_example, captured_args.image_dir
                )
                candidate.parent.mkdir(parents=True)
                Image.new("RGBA", (1280, 720), (30, 20, 10, 255)).save(candidate)
                return True, "new", "1280x720"

            with patch.object(capture_gallery, "capture_one", side_effect=fake_capture):
                ok, status, _ = capture_gallery.verify_existing_capture(
                    example, args, "input-hash"
                )

            self.assertFalse(ok)
            self.assertEqual(status, "different")
            self.assertFalse(
                capture_gallery.cache_path(example, args.cache_dir).exists()
            )

    def test_accepts_tightly_bounded_pixel_variation(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            example = self._example()
            args = self._args(root)
            canonical = capture_gallery.output_path(example, args.image_dir)
            canonical.parent.mkdir(parents=True)
            Image.new("RGBA", (1280, 720), (10, 20, 30, 255)).save(canonical)

            def fake_capture(
                captured_example: capture_gallery.CaptureExample,
                captured_args: argparse.Namespace,
                unused_hash: str | None,
            ) -> tuple[bool, str, str]:
                candidate = capture_gallery.output_path(
                    captured_example, captured_args.image_dir
                )
                candidate.parent.mkdir(parents=True)
                image = Image.new("RGBA", (1280, 720), (10, 20, 30, 255))
                image.putpixel((0, 0), (11, 20, 30, 255))
                image.save(candidate)
                return True, "new", "1280x720"

            with patch.object(capture_gallery, "capture_one", side_effect=fake_capture):
                ok, status, detail = capture_gallery.verify_existing_capture(
                    example, args, "input-hash"
                )

            self.assertTrue(ok, detail)
            self.assertEqual(status, "verified-pixels")
            self.assertIn("max_delta=1", detail)


if __name__ == "__main__":
    unittest.main()
