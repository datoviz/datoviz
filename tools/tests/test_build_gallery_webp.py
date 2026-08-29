#!/usr/bin/env python3
"""Focused tests for verified static gallery WebP products and fallbacks."""

from __future__ import annotations

import json
import subprocess
import sys
import tempfile
from pathlib import Path
from types import SimpleNamespace
from unittest import TestCase, main, mock

from PIL import Image


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


def _write_image(path: Path, size: tuple[int, int], color: tuple[int, int, int]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    image_format = "PNG" if path.suffix == ".png" else "WEBP"
    Image.new("RGB", size, color).save(path, format=image_format, quality=90)


class GalleryWebPTest(TestCase):
    """Validate cache repair and clean fallbacks without invoking cwebp."""

    def _convert(self, cmd: list[str], **kwargs) -> None:
        del kwargs
        source = Path(cmd[cmd.index("-o") - 1])
        output = Path(cmd[cmd.index("-o") + 1])
        output.parent.mkdir(parents=True, exist_ok=True)
        with Image.open(source) as image:
            image.save(output, format="WEBP", quality=int(cmd[cmd.index("-q") + 1]))

    def _run(
        self,
        image_dir: Path,
        output_dir: Path,
        examples: list[SimpleNamespace],
        *,
        force: bool = False,
        quality: int = build_gallery_webp.DEFAULT_QUALITY,
        encoder_identity: str = "test-cwebp-a",
        implementation_identity: str = "test-implementation-a",
        encoder: object | None = None,
    ) -> build_gallery_webp.GalleryWebPResult:
        manifest = {"examples": [_manifest_entry(example) for example in examples]}
        cache_dir = output_dir.parent / "cache"
        with (
            mock.patch.object(
                build_gallery_webp.gallery_media, "load_manifest", return_value=manifest
            ),
            mock.patch.object(
                build_gallery_webp.build_gallery, "collect_examples", return_value=examples
            ),
            mock.patch.object(build_gallery_webp.shutil, "which", return_value="/usr/bin/cwebp"),
            mock.patch.object(
                build_gallery_webp, "cwebp_identity", return_value=encoder_identity
            ),
            mock.patch.object(
                build_gallery_webp,
                "implementation_identity",
                return_value=implementation_identity,
            ),
            mock.patch.object(
                build_gallery_webp.subprocess,
                "run",
                side_effect=encoder or self._convert,
            ),
        ):
            rc, result = build_gallery_webp.generate_gallery_webp(
                image_dir=image_dir,
                output_dir=output_dir,
                cache_dir=cache_dir,
                animated_fallbacks=True,
                force=force,
                quality=quality,
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
                _write_image(png, (1280, 720), (40, 80, 120))

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
                _write_image(png, (1280, 720), (120, 80, 40))
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

    def test_newer_wrong_dimension_placeholder_is_repaired_without_force(self) -> None:
        example = _example("scientific_plotting")

        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            image_dir = root / "images"
            output_dir = root / "output"
            png = image_dir / "features/scientific_plotting.png"
            webp = output_dir / "features/scientific_plotting.webp"
            _write_image(png, (1280, 720), (10, 70, 140))
            self.assertEqual(self._run(image_dir, output_dir, [example]).converted, 1)
            _write_image(webp, (640, 360), (238, 238, 238))
            webp.touch()
            self.assertGreaterEqual(webp.stat().st_mtime_ns, png.stat().st_mtime_ns)

            result = self._run(image_dir, output_dir, [example])

            self.assertEqual(result.converted, 1)
            self.assertEqual(build_gallery_webp.image_dimensions(webp, "WEBP"), (1280, 720))
            record = json.loads((root / "cache/features/scientific_plotting.json").read_text())
            self.assertEqual(record["dimensions"], [1280, 720])
            self.assertEqual(record["webp_sha256"], build_gallery_webp.file_sha256(webp))

    def test_cache_identity_tracks_source_quality_encoder_and_implementation(self) -> None:
        example = _example("static")

        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            image_dir = root / "images"
            output_dir = root / "output"
            png = image_dir / "features/static.png"
            _write_image(png, (1280, 720), (20, 40, 60))

            self.assertEqual(self._run(image_dir, output_dir, [example]).converted, 1)
            cached = self._run(image_dir, output_dir, [example])
            self.assertEqual(cached.converted, 0)
            self.assertEqual(cached.skipped, 1)
            self.assertEqual(
                self._run(image_dir, output_dir, [example], quality=80).converted, 1
            )
            self.assertEqual(
                self._run(
                    image_dir,
                    output_dir,
                    [example],
                    quality=80,
                    encoder_identity="test-cwebp-b",
                ).converted,
                1,
            )
            self.assertEqual(
                self._run(
                    image_dir,
                    output_dir,
                    [example],
                    quality=80,
                    encoder_identity="test-cwebp-b",
                    implementation_identity="test-implementation-b",
                ).converted,
                1,
            )

            _write_image(png, (1280, 720), (90, 30, 10))
            self.assertEqual(
                self._run(
                    image_dir,
                    output_dir,
                    [example],
                    quality=80,
                    encoder_identity="test-cwebp-b",
                    implementation_identity="test-implementation-b",
                ).converted,
                1,
            )

    def test_changed_cached_output_is_regenerated_even_at_the_expected_size(self) -> None:
        example = _example("static")

        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            image_dir = root / "images"
            output_dir = root / "output"
            png = image_dir / "features/static.png"
            webp = output_dir / "features/static.webp"
            _write_image(png, (1280, 720), (20, 40, 60))
            self.assertEqual(self._run(image_dir, output_dir, [example]).converted, 1)
            expected_hash = build_gallery_webp.file_sha256(webp)

            _write_image(webp, (1280, 720), (220, 210, 200))
            self.assertNotEqual(build_gallery_webp.file_sha256(webp), expected_hash)
            self.assertEqual(self._run(image_dir, output_dir, [example]).converted, 1)
            self.assertEqual(build_gallery_webp.file_sha256(webp), expected_hash)

    def test_encoder_failure_preserves_previous_valid_product_and_record(self) -> None:
        example = _example("static")

        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            image_dir = root / "images"
            output_dir = root / "output"
            png = image_dir / "features/static.png"
            webp = output_dir / "features/static.webp"
            record = root / "cache/features/static.json"
            _write_image(png, (1280, 720), (20, 40, 60))
            self.assertEqual(self._run(image_dir, output_dir, [example]).converted, 1)
            previous_webp = webp.read_bytes()
            previous_record = record.read_bytes()

            def fail(cmd: list[str], **kwargs) -> None:
                del kwargs
                raise subprocess.CalledProcessError(1, cmd)

            with self.assertRaises(subprocess.CalledProcessError):
                self._run(image_dir, output_dir, [example], force=True, encoder=fail)

            self.assertEqual(webp.read_bytes(), previous_webp)
            self.assertEqual(record.read_bytes(), previous_record)
            self.assertEqual(list(webp.parent.glob(".*.tmp")), [])
            self.assertEqual(list(record.parent.glob(".*.tmp")), [])


if __name__ == "__main__":
    main()
