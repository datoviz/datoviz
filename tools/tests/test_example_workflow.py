from __future__ import annotations

import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

import example_workflow as workflow


def entry(**overrides):
    value = {
        "id": "features_example",
        "source": "examples/c/features/example.c",
        "validation": "smoke+interaction+screenshot",
        "portability": "portable-scenario",
        "webgpu": {
            "status": "webgpu-live",
            "route": "examples/webgpu/live.html?id=features_example",
        },
    }
    value.update(overrides)
    return value


class ExampleWorkflowTests(unittest.TestCase):
    def test_check_is_build_local_and_includes_webgpu_proof(self):
        result = workflow.build_workflow(entry(), "check", False)
        flattened = [" ".join(command) for command in result.commands]
        self.assertTrue(
            any("build/example-check/features_example/gallery" in command for command in flattened)
        )
        self.assertFalse(any("--approve-data-update" in command for command in flattened))
        self.assertIn("just wasm-scene-smoke", flattened)
        self.assertIn("just webgpu-browser-smoke --route=features_example", flattened)
        self.assertTrue(
            any(
                "tools/check_example_manifests.py --examples build/example-check/features_example/docs/examples.json"
                in command
                for command in flattened
            )
        )
        self.assertEqual(
            result.warnings,
            (
                "features_example: interaction or motion is declared but no deterministic preview/video is configured",
            ),
        )

    def test_promote_requires_explicit_data_approval(self):
        with self.assertRaisesRegex(ValueError, "--approve-data-update"):
            workflow.build_workflow(entry(), "promote", False)

    def test_promote_captures_verifies_and_generates_video(self):
        promoted = entry(
            validation="smoke+interaction+screenshot+video",
            media={
                "preview": {
                    "kind": "animated-webp",
                    "frames": 60,
                    "fps": 30,
                    "card": {"preferred": "video-mp4", "fps": 30},
                }
            },
        )
        result = workflow.build_workflow(promoted, "promote", True)
        flattened = [" ".join(command) for command in result.commands]
        self.assertTrue(
            any(
                "tools/capture_gallery.py --id features_example --force" in command
                for command in flattened
            )
        )
        self.assertTrue(any("--verify-existing" in command for command in flattened))
        self.assertTrue(any("--include-video-previews" in command for command in flattened))
        self.assertTrue(
            any("--site-video-previews --write-site-assets" in command for command in flattened)
        )
        self.assertIn("just docs-generate", flattened)
        self.assertIn("just docs-assets", flattened)
        self.assertEqual(result.warnings, ())

    def test_public_example_requires_screenshot_and_webgpu_classification(self):
        with self.assertRaisesRegex(ValueError, "screenshot validation"):
            workflow.build_workflow(entry(validation="smoke"), "check", False)
        with self.assertRaisesRegex(ValueError, "WebGPU status"):
            workflow.build_workflow(entry(webgpu={}), "check", False)


if __name__ == "__main__":
    unittest.main()
