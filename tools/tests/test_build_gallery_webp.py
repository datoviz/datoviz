#!/usr/bin/env python3
"""Focused tests for clean-build gallery media fallbacks."""

from __future__ import annotations

import sys
import tempfile
from pathlib import Path
from types import SimpleNamespace
from unittest import TestCase, main, mock


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import build_gallery_webp  # noqa: E402


def _example(example_id: str, preview: dict | None = None) -> SimpleNamespace:
    return SimpleNamespace(
        id=example_id,
        lane="features",
        validation="smoke+screenshot",
        media={"preview": preview} if preview else {},
    )


def _manifest_entry(example: SimpleNamespace) -> dict:
    return {
        "id": example.id,
        "category": "feature",
        "media": example.media,
    }


class GalleryWebPFallbackTest(TestCase):
    """Validate clean fallbacks without invoking external encoders."""

    def _convert(self, cmd: list[str], **kwargs) -> None:
        del kwargs
        output = Path(cmd[cmd.index("-o") + 1])
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(b"generated fallback")

    def _run(
        self,
        image_dir: Path,
        output_dir: Path,
        examples: list[SimpleNamespace],
        *,
        force: bool = False,
    ) -> build_gallery_webp.GalleryWebPResult:
        manifest = {"examples": [_manifest_entry(example) for example in examples]}
        with (
            mock.patch.object(
                build_gallery_webp.gallery_media, "load_manifest", return_value=manifest
            ),
            mock.patch.object(
                build_gallery_webp.build_gallery, "collect_examples", return_value=examples
            ),
            mock.patch.object(build_gallery_webp.shutil, "which", return_value="/usr/bin/cwebp"),
            mock.patch.object(build_gallery_webp.subprocess, "run", side_effect=self._convert),
        ):
            rc, result = build_gallery_webp.generate_gallery_webp(
                image_dir=image_dir,
                output_dir=output_dir,
                animated_fallbacks=True,
                force=force,
            )
        self.assertEqual(rc, 0)
        return result

    def test_clean_output_generates_all_documentation_fallbacks(self) -> None:
        static = _example("static")
        animated = _example("animated", {"kind": "animated-webp"})
        video = _example(
            "video",
            {"kind": "animated-webp", "card": {"preferred": "video-mp4"}},
        )
        examples = [static, animated, video]

        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            image_dir = root / "images"
            output_dir = root / "output"
            for example in examples:
                png = image_dir / example.lane / f"{example.id}.png"
                png.parent.mkdir(parents=True, exist_ok=True)
                png.write_bytes(b"png")

            result = self._run(image_dir, output_dir, examples)

            self.assertEqual(result.converted, 3)
            self.assertTrue((output_dir / "features/static.webp").is_file())
            self.assertTrue((output_dir / "features/animated.webp").is_file())
            self.assertTrue((output_dir / "features/video.poster.webp").is_file())

    def test_existing_rich_media_is_never_overwritten(self) -> None:
        animated = _example("animated", {"kind": "animated-webp"})
        video = _example(
            "video",
            {"kind": "animated-webp", "card": {"preferred": "video-mp4"}},
        )
        examples = [animated, video]

        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            image_dir = root / "images"
            output_dir = root / "output"
            for example in examples:
                png = image_dir / example.lane / f"{example.id}.png"
                png.parent.mkdir(parents=True, exist_ok=True)
                png.write_bytes(b"new png")
            animation = output_dir / "features/animated.webp"
            poster = output_dir / "features/video.poster.webp"
            animation.parent.mkdir(parents=True)
            animation.write_bytes(b"rich animation")
            poster.write_bytes(b"rich poster")

            result = self._run(image_dir, output_dir, examples, force=True)

            self.assertEqual(result.converted, 0)
            self.assertEqual(result.skipped, 2)
            self.assertEqual(animation.read_bytes(), b"rich animation")
            self.assertEqual(poster.read_bytes(), b"rich poster")


if __name__ == "__main__":
    main()
