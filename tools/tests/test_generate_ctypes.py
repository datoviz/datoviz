#!/usr/bin/env python3
"""Focused tests for platform-neutral ctypes scalar mappings."""

from __future__ import annotations

from pathlib import Path
import sys
import unittest


TOOLS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS / "bindings"))

import generate_ctypes as ctypes_gen  # noqa: E402


class FixedWidthTypeTests(unittest.TestCase):
    def test_public_and_vulkan_aliases_ignore_platform_canonical_type(self) -> None:
        cases = {
            "DvzCallbackId": "ctypes.c_uint64",
            "DvzCategoryId": "ctypes.c_int64",
            "DvzSize": "ctypes.c_uint64",
            "DvzTimestamp": "ctypes.c_int64",
            "VkAccessFlags2": "ctypes.c_uint64",
            "VkDeviceSize": "ctypes.c_uint64",
            "VkPipelineStageFlags2": "ctypes.c_uint64",
        }
        for alias, expected in cases.items():
            with self.subTest(alias=alias):
                actual = ctypes_gen._ctype_for_type(
                    {"qualtype": alias, "canonical": "unsigned long"}, set(), set()
                )
                self.assertEqual(actual, expected)

    def test_callback_parser_prefers_fixed_width_spelling(self) -> None:
        api = {
            "typedefs": [
                {
                    "name": "DvzCallback",
                    "type": {
                        "qualtype": "void (*)(uint64_t)",
                        "canonical": "void (*)(unsigned long)",
                    },
                }
            ]
        }
        callback = ctypes_gen._callback_typedefs(api)["DvzCallback"]
        self.assertEqual(callback["args"], ["uint64_t"])


if __name__ == "__main__":
    unittest.main()
