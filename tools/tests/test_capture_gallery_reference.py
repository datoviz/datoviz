#!/usr/bin/env python3
"""Focused tests for Linux reference screenshot candidate reporting."""

from __future__ import annotations

from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

from PIL import Image


TOOLS_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS_DIR))

import capture_gallery_reference as reference  # noqa: E402


class ReferenceOutputSafetyTest(unittest.TestCase):
    def test_accepts_build_child(self) -> None:
        path = reference.ensure_build_output(reference.ROOT / "build/gallery-reference")

        self.assertEqual(path, (reference.ROOT / "build/gallery-reference").resolve())

    def test_rejects_data_path(self) -> None:
        with self.assertRaisesRegex(ValueError, "proper child"):
            reference.ensure_build_output(reference.ROOT / "data/gallery/v0.4")

    def test_rejects_build_root(self) -> None:
        with self.assertRaisesRegex(ValueError, "proper child"):
            reference.ensure_build_output(reference.ROOT / "build")


class VulkanSummaryTest(unittest.TestCase):
    def test_parses_selected_gpu_identity(self) -> None:
        summary = """
Vulkan Instance Version: 1.4.328
Devices:
========
GPU0:
    deviceName = NVIDIA GeForce RTX 5090
    driverInfo = 595.84
    deviceUUID = first
GPU1:
    deviceName = llvmpipe
    driverInfo = Mesa
"""

        devices = reference.parse_vulkan_devices(summary)

        self.assertEqual(len(devices), 2)
        self.assertEqual(devices[0]["index"], "0")
        self.assertEqual(devices[0]["deviceName"], "NVIDIA GeForce RTX 5090")
        self.assertEqual(devices[1]["deviceUUID"] if "deviceUUID" in devices[1] else None, None)


class ReferenceComparisonTest(unittest.TestCase):
    def _image(self, path: Path, color: tuple[int, int, int, int]) -> Path:
        path.parent.mkdir(parents=True, exist_ok=True)
        Image.new("RGBA", (32, 32), color).save(path)
        return path

    def test_classifies_exact_and_bounded_differences(self) -> None:
        self.assertEqual(reference.classification(0, 0), "identical")
        self.assertEqual(reference.classification(8, 0.001), "pixel-equivalent")
        self.assertEqual(reference.classification(9, 0.001), "different")
        self.assertEqual(reference.classification(8, 0.0011), "different")

    def test_writes_enhanced_difference(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            first = self._image(root / "first.png", (10, 20, 30, 255))
            second = self._image(root / "second.png", (12, 20, 30, 255))
            target = root / "diff/diff.png"

            reference.write_enhanced_diff(first, second, target)

            with Image.open(target) as image:
                self.assertEqual(image.size, (32, 32))
                self.assertEqual(image.getpixel((0, 0)), (16, 0, 0))


class ProvenancePolicyTest(unittest.TestCase):
    @patch.object(reference.platform, "system", return_value="Darwin")
    def test_rejects_non_linux_host(self, unused_system) -> None:
        with self.assertRaisesRegex(RuntimeError, "must be generated on Linux"):
            reference.collect_provenance(
                "reference", [["capture", "--jobs", "1"]]
            )


if __name__ == "__main__":
    unittest.main()
