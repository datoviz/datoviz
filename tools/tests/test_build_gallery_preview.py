#!/usr/bin/env python3
"""Tests for generated gallery preview markup."""

from __future__ import annotations

from dataclasses import replace
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
        example = replace(
            example,
            webgpu={
                **example.webgpu,
                "status": "webgpu-deferred",
                "route": "",
                "local_route": "examples/webgpu/live.html?id=showcases_point_cloud",
            },
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

    def test_public_point_cloud_preview_uses_live_route(self) -> None:
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

        text = "\n".join(lines)
        self.assertIn("showcases_point_cloud&embedded=1", text)
        self.assertIn("Open the live WebGPU example", text)
        self.assertNotIn("dvz-local-webgpu-tabs", text)

    def test_point_cloud_run_guidance_marks_full_preprocessing_optional(self) -> None:
        manifest = build_gallery.load_manifest(build_gallery.DEFAULT_MANIFEST)
        example = next(
            example
            for example in build_gallery.collect_examples(manifest)
            if example.id == "showcases_point_cloud"
        )

        text = "\n".join(
            build_gallery.render_run_and_adapt(
                example, Path("gallery/showcases/showcases_point_cloud.md")
            )
        )

        self.assertIn('!!! info "Prepared data included"', text)
        self.assertIn("The committed prepared input is sufficient", text)
        self.assertIn("Optionally generate the larger local cache", text)
        self.assertNotIn('!!! warning "Prepared data required"', text)

    def test_required_preprocessing_keeps_warning(self) -> None:
        manifest = build_gallery.load_manifest(build_gallery.DEFAULT_MANIFEST)
        example = next(
            example
            for example in build_gallery.collect_examples(manifest)
            if example.id == "showcases_cortical_activity"
        )

        text = "\n".join(
            build_gallery.render_run_and_adapt(
                example, Path("gallery/showcases/showcases_cortical_activity.md")
            )
        )

        self.assertIn('!!! warning "Prepared data required"', text)
        self.assertNotIn('!!! info "Prepared data included"', text)


if __name__ == "__main__":
    unittest.main()
