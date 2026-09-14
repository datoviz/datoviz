#!/usr/bin/env python3
"""Focused tests for Vulkan course preview generation."""

from __future__ import annotations

import os
import sys
import tempfile
import time
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
        for index in range(1, 16):
            executable = directory / f"step{index:02d}"
            executable.write_bytes(b"executable")
            future = time.time() + 3600
            os.utime(executable, (future, future))
        return directory

    def _run_step(
        self, executable: Path, arguments: list[str], cwd: Path = build_tutorial_media.ROOT
    ) -> str:
        del cwd
        if executable.name == "step01":
            return "Datoviz 0.4.0-dev\n"
        png = Path(arguments[arguments.index("--png") + 1])
        if executable.name == "step02":
            rgba = (89, 97, 118, 255)
        elif executable.name == "step03":
            time_s = float(arguments[arguments.index("--time") + 1])
            frame_index = round(time_s * build_tutorial_media.ANIMATION_FPS)
            rgba = build_tutorial_media.EXPECTED_STEP03_RGBA[frame_index]
        else:
            image = Image.new("RGBA", build_tutorial_media.SIZE, (40, 45, 55, 255))
            accent = 240
            if executable.name == "step15" and "--time" in arguments:
                time_s = float(arguments[arguments.index("--time") + 1])
                accent = 100 + round(time_s * 50)
            image.putpixel((400, 300), (accent, 80, 60, 255))
            image.save(png)
            return "rendered 1 frames\nvalidation errors: 0\n"
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
                path = output_dir / name
                if name == "02-window.webp":
                    image = Image.new("RGBA", build_tutorial_media.SIZE, (89, 97, 118, 255))
                    image.save(path, format="WEBP", lossless=True)
                elif name == "03-frame.webp":
                    frames = [
                        Image.new("RGBA", build_tutorial_media.SIZE, rgba)
                        for rgba in build_tutorial_media.EXPECTED_STEP03_RGBA
                    ]
                    frames[0].save(
                        path, save_all=True, append_images=frames[1:], format="WEBP", lossless=True
                    )
                elif name == "15-mesh-animated.webp":
                    frames = []
                    for index in range(len(build_tutorial_media.ANIMATION_TIMES)):
                        frame = Image.new("RGBA", build_tutorial_media.SIZE, (40, 45, 55, 255))
                        frame.putpixel((400, 300), (100 + index * 10, 80, 60, 255))
                        frames.append(frame)
                    frames[0].save(
                        path, save_all=True, append_images=frames[1:], format="WEBP", lossless=True
                    )
                else:
                    rgba = (
                        build_tutorial_media.EXPECTED_STEP03_RGBA[3]
                        if name == "03-frame-still.webp"
                        else (40, 45, 55, 255)
                    )
                    image = Image.new("RGBA", build_tutorial_media.SIZE, rgba)
                    if name != "03-frame-still.webp":
                        image.putpixel((400, 300), (240, 80, 60, 255))
                    image.save(path, format="WEBP", lossless=True)
                future = time.time() + 3600
                os.utime(path, (future, future))
            rc, result = build_tutorial_media.generate_tutorial_media(
                executables_dir=executables, output_dir=output_dir, strict=True
            )
            self.assertEqual(rc, 0)
            self.assertEqual(result.skipped, len(build_tutorial_media.EXPECTED_OUTPUTS))

            flat = output_dir / "04-triangle.webp"
            Image.new("RGBA", build_tutorial_media.SIZE, (40, 45, 55, 255)).save(
                flat, format="WEBP", lossless=True
            )
            future = time.time() + 3600
            os.utime(flat, (future, future))
            outputs = [output_dir / name for name in build_tutorial_media.EXPECTED_OUTPUTS]
            self.assertFalse(
                build_tutorial_media._current(list(executables.iterdir()), outputs, False)
            )

    def test_strict_mode_rejects_missing_executables(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            rc, result = build_tutorial_media.generate_tutorial_media(
                executables_dir=Path(tmp), output_dir=Path(tmp) / "output", strict=True
            )
        self.assertEqual(rc, 2)
        self.assertEqual(result.missing, 15)

    def test_uniform_rgba_accepts_one_level_rgb_delta(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "close.png"
            Image.new("RGBA", build_tutorial_media.SIZE, (90, 96, 119, 255)).save(path)
            build_tutorial_media._validate_uniform_rgba(path, (89, 97, 118, 255))

    def test_uniform_rgba_rejects_larger_rgb_delta(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "wrong.png"
            Image.new("RGBA", build_tutorial_media.SIZE, (89, 97, 120, 255)).save(path)
            with self.assertRaisesRegex(RuntimeError, "channel deltas"):
                build_tutorial_media._validate_uniform_rgba(path, (89, 97, 118, 255))

    def test_uniform_rgba_rejects_nonuniform_image(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "gradient.png"
            image = Image.new("RGBA", build_tutorial_media.SIZE, (89, 97, 118, 255))
            image.putpixel((1, 0), (90, 97, 118, 255))
            image.save(path)
            with self.assertRaisesRegex(RuntimeError, "uniform RGBA image"):
                build_tutorial_media._validate_uniform_rgba(path, (89, 97, 118, 255))


if __name__ == "__main__":
    main()
