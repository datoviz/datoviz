#!/usr/bin/env python3
"""Tests for generated gallery preview markup."""

from __future__ import annotations

from pathlib import Path
import sys
import unittest

TOOLS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS))

import build_gallery  # noqa: E402


class GalleryPreviewTests(unittest.TestCase):
    def test_local_webgpu_preview_emits_unwrapped_two_tab_block(self) -> None:
        manifest = build_gallery.load_manifest(build_gallery.DEFAULT_MANIFEST)
        example = next(
            example
            for example in build_gallery.collect_examples(manifest)
            if example.id == "showcases_point_cloud"
        )
        lines = build_gallery.render_preview(
            example,
            Path("gallery/showcases/showcases_point_cloud.md"),
            build_gallery.DEFAULT_IMAGE_DIR,
            build_gallery.DEFAULT_IMAGE_URL_BASE,
        )
        marker = '<div class="dvz-local-webgpu-tabs" hidden></div>'
        marker_index = lines.index(marker)

        self.assertEqual(lines[marker_index + 2], '=== "Screenshot"')
        self.assertEqual(lines.count('=== "Screenshot"'), 1)
        self.assertEqual(lines.count('=== "Live WebGPU"'), 1)
        self.assertNotIn('<div class="dvz-local-webgpu-tabs" hidden markdown="1">', lines)


if __name__ == "__main__":
    unittest.main()
