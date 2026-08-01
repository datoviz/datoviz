#!/usr/bin/env python3
"""Focused tests for Vulkan course preview generation."""

from __future__ import annotations

import sys
import tempfile
from pathlib import Path
from unittest import TestCase, main, mock

from PIL import Image


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import build_tutorial_media  # noqa: E402


class TutorialMediaTest(TestCase):
    def _executables(self, root: Path) -> Path:
        directory = root / "executables"
        directory.mkdir()
        for index in range(1, 4):
            (directory / f"step0{index}").write_bytes(b"executable")
        return directory

    def _run_step(self, executable: Path, arguments: list[str]) -> str:
        if executable.name == "step01":
            return "Datoviz 0.4.0-dev\n"
        png = Path(arguments[arguments.index("--png") + 1])
        if executable.name == "step02":
            rgba = (89, 97, 118, 255)
        else:
            time_s = float(arguments[arguments.index("--time") + 1])
            frame_index = round(time_s * build_tutorial_media.ANIMATION_FPS)
            rgba = build_tutorial_media.EXPECTED_STEP03_RGBA[frame_index]
        Image.new("RGBA", build_tutorial_media.SIZE, rgba).save(png)
        return "rendered 1 frames\nvalidation errors: 0\n"

    @staticmethod
    def _encode_static(source: Path, output: Path, quality: int) -> None:
        del source, quality
        output.write_bytes(b"static-webp")

    @staticmethod
    def _encode_animation(frames: list[Path], output: Path, quality: int) -> None:
        del quality
        output.write_bytes(b"animated-webp:" + b"|".join(path.read_bytes() for path in frames))

    def test_generate_expected_previews(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            executables = self._executables(root)
            output_dir = root / "output"
            with (
                mock.patch.object(build_tutorial_media, "_run_step", side_effect=self._run_step),
                mock.patch.object(
                    build_tutorial_media, "_encode_static", side_effect=self._encode_static
                ),
                mock.patch.object(
                    build_tutorial_media, "_encode_animation", side_effect=self._encode_animation
                ),
            ):
                rc, result = build_tutorial_media.generate_tutorial_media(
                    executables_dir=executables, output_dir=output_dir, strict=True
                )
            self.assertEqual(rc, 0)
            self.assertEqual(result.generated, len(build_tutorial_media.EXPECTED_OUTPUTS))
            for name in build_tutorial_media.EXPECTED_OUTPUTS:
                self.assertTrue((output_dir / name).is_file())

    def test_current_outputs_are_skipped(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            executables = self._executables(root)
            output_dir = root / "output"
            output_dir.mkdir()
            for name in build_tutorial_media.EXPECTED_OUTPUTS:
                (output_dir / name).write_bytes(b"webp")
            with mock.patch.object(build_tutorial_media, "_current", return_value=True):
                rc, result = build_tutorial_media.generate_tutorial_media(
                    executables_dir=executables, output_dir=output_dir, strict=True
                )
            self.assertEqual(rc, 0)
            self.assertEqual(result.skipped, len(build_tutorial_media.EXPECTED_OUTPUTS))

    def test_strict_mode_rejects_missing_executables(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            rc, result = build_tutorial_media.generate_tutorial_media(
                executables_dir=Path(tmp), output_dir=Path(tmp) / "output", strict=True
            )
        self.assertEqual(rc, 2)
        self.assertEqual(result.missing, 3)

    def test_exact_rgba_rejects_wrong_flat_color(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "wrong.png"
            Image.new("RGBA", build_tutorial_media.SIZE, (1, 2, 3, 255)).save(path)
            with self.assertRaisesRegex(RuntimeError, "expected exact RGBA"):
                build_tutorial_media._validate_exact_rgba(path, (89, 97, 118, 255))


if __name__ == "__main__":
    main()
