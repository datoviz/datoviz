#!/usr/bin/env python3
"""Focused tests for platform-neutral ctypes scalar mappings."""

from __future__ import annotations

import ctypes
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock


TOOLS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS / "bindings"))

import generate_ctypes as ctypes_gen  # noqa: E402


class _FakeFunction:
    def __call__(self, *_args):
        return None


class _FakeLibrary:
    def __getattr__(self, _name):
        return _FakeFunction()


def _aligned_record_api() -> dict:
    return {
        "records": [
            *[
                {
                    "name": name,
                    "kind": "struct",
                    "opaque": False,
                    "fields": [
                        {"name": "value", "type": {"qualtype": "uint32_t"}},
                    ],
                }
                for name in [
                    "DvzPointerEvent",
                    "DvzKeyboardEvent",
                    "DvzInputResizeEvent",
                    "DvzInputScaleEvent",
                ]
            ],
            {
                "name": "DvzAlignedOutput",
                "kind": "struct",
                "opaque": False,
                "fields": [
                    {"name": "struct_size", "type": {"qualtype": "uint32_t"}},
                    {"name": "matrix", "type": {"qualtype": "float[16]"}},
                ],
            }
        ],
        "functions": [
            {
                "name": "dvz_aligned_output",
                "result": {"qualtype": "bool"},
                "parameters": [
                    {
                        "name": "out",
                        "type": {"qualtype": "DvzAlignedOutput *"},
                    }
                ],
            }
        ],
    }


def _execute_generated(text: str) -> dict:
    with tempfile.NamedTemporaryFile() as library:
        with mock.patch.dict(os.environ, {"DATOVIZ_LIBRARY": library.name}):
            with mock.patch.object(ctypes.cdll, "LoadLibrary", return_value=_FakeLibrary()):
                namespace = {"__file__": str(Path(library.name).with_suffix(".py"))}
                exec(compile(text, namespace["__file__"], "exec"), namespace)
    return namespace


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


class AlignedLayoutTests(unittest.TestCase):
    def test_policy_required_alignment_parser(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            policy = Path(tmp) / "ctypes.yml"
            policy.write_text(
                "layout_records:\n"
                "  include:\n"
                "    - DvzAlignedOutput\n"
                "  required_alignment:\n"
                "    DvzAlignedOutput: 16\n"
            )
            self.assertEqual(
                ctypes_gen._required_alignments_from_policy(policy),
                {"DvzAlignedOutput": 16},
            )

    def test_effective_alignment_emits_record_and_output_function(self) -> None:
        if sys.version_info < (3, 13):
            self.skipTest("effective ctypes.Structure._align_ requires Python 3.13+")
        text, skipped = ctypes_gen.generate(
            _aligned_record_api(),
            forced_layout_records={"DvzAlignedOutput"},
            required_alignments={"DvzAlignedOutput": 16},
        )
        namespace = _execute_generated(text)
        record = namespace["DvzAlignedOutput"]
        self.assertEqual(skipped, [])
        self.assertGreater(ctypes.sizeof(record), 0)
        self.assertEqual(ctypes.alignment(record), 16)
        self.assertIn("DvzAlignedOutput", namespace["_DATOVIZ_CTYPES_LAYOUT_RECORDS"])
        self.assertIn("dvz_aligned_output", namespace)
        self.assertEqual(namespace["_UNSUPPORTED_FUNCTIONS"], {})

    def test_ineffective_alignment_keeps_record_opaque_and_output_unavailable(self) -> None:
        text, _ = ctypes_gen.generate(
            _aligned_record_api(),
            forced_layout_records={"DvzAlignedOutput"},
            required_alignments={"DvzAlignedOutput": 16},
        )
        text = text.replace(
            "return ctypes.alignment(_AlignmentProbe) == requested",
            "return False",
        )
        namespace = _execute_generated(text)
        record = namespace["DvzAlignedOutput"]
        self.assertFalse(hasattr(record, "_fields_"))
        self.assertEqual(ctypes.sizeof(record), 0)
        self.assertNotIn("DvzAlignedOutput", namespace["_DATOVIZ_CTYPES_LAYOUT_RECORDS"])
        self.assertNotIn("dvz_aligned_output", namespace)
        self.assertIn("DvzAlignedOutput", namespace["_UNSUPPORTED_LAYOUT_RECORDS"])
        self.assertIn("dvz_aligned_output", namespace["_UNSUPPORTED_FUNCTIONS"])

    def test_no_conditional_output_dependencies_imports_cleanly(self) -> None:
        text, _ = ctypes_gen.generate(
            _aligned_record_api(),
            forced_layout_records={"DvzAlignedOutput"},
            required_alignments={},
        )
        namespace = _execute_generated(text)
        self.assertEqual(namespace["_OUTPUT_RECORD_DEPENDENCIES"], {})
        self.assertIn("dvz_aligned_output", namespace)


if __name__ == "__main__":
    unittest.main()
