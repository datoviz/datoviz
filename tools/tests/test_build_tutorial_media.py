#!/usr/bin/env python3
"""Focused tests for Vulkan tutorial preview generation."""

from __future__ import annotations

import sys
import tempfile
from pathlib import Path
from unittest import TestCase, main, mock


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import build_tutorial_media  # noqa: E402


class TutorialMediaTest(TestCase):
    def _convert(self, command: list[str], **kwargs) -> None:
        del kwargs
        output = Path(command[command.index("-o") + 1])
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(b"webp")

    def test_generate_expected_previews(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source_dir = root / "source"
            output_dir = root / "output"
            source_dir.mkdir()
            for stem in build_tutorial_media.EXPECTED_PREVIEWS:
                (source_dir / f"{stem}.png").write_bytes(b"png")
            with (
                mock.patch.object(
                    build_tutorial_media.shutil, "which", return_value="/usr/bin/cwebp"
                ),
                mock.patch.object(
                    build_tutorial_media, "png_is_nonblank", return_value=(True, "ok")
                ),
                mock.patch.object(
                    build_tutorial_media.subprocess, "run", side_effect=self._convert
                ),
            ):
                rc, result = build_tutorial_media.generate_tutorial_media(
                    source_dir=source_dir, output_dir=output_dir, strict=True
                )
            self.assertEqual(rc, 0)
            self.assertEqual(result.converted, 3)
            for stem in build_tutorial_media.EXPECTED_PREVIEWS:
                self.assertTrue((output_dir / f"{stem}.webp").is_file())

    def test_strict_mode_rejects_missing_preview(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source_dir = root / "source"
            source_dir.mkdir()
            with mock.patch.object(
                build_tutorial_media.shutil, "which", return_value="/usr/bin/cwebp"
            ):
                rc, result = build_tutorial_media.generate_tutorial_media(
                    source_dir=source_dir, output_dir=root / "output", strict=True
                )
            self.assertEqual(rc, 1)
            self.assertEqual(result.missing, 3)


if __name__ == "__main__":
    main()
