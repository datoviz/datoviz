#!/usr/bin/env python3
"""Focused tests for platform-neutral ctypes scalar mappings."""

from __future__ import annotations

import ctypes
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock


TOOLS = Path(__file__).resolve().parents[1]
ROOT = TOOLS.parent
sys.path.insert(0, str(TOOLS / "bindings"))

import generate_ctypes as ctypes_gen  # noqa: E402


class _FakeFunction:
    def __call__(self, *_args):
        return None


class _FakeLibrary:
    def __getattr__(self, _name):
        return _FakeFunction()


def _input_event_support_records() -> list[dict]:
    return [
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
    ]


def _aligned_record_api() -> dict:
    return {
        "records": [
            *_input_event_support_records(),
            {
                "name": "DvzAlignedOutput",
                "kind": "struct",
                "opaque": False,
                "size": 96,
                "align": 16,
                "fields": [
                    {
                        "name": "struct_size",
                        "type": {"qualtype": "uint32_t"},
                        "offset": 0,
                        "size": 4,
                    },
                    {
                        "name": "matrix",
                        "type": {"qualtype": "float[16]"},
                        "offset": 16,
                        "size": 64,
                    },
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


def _unsupported_record_api() -> dict:
    unavailable = {
        "name": "DvzUnavailable",
        "kind": "struct",
        "opaque": False,
        "fields": [{"name": "values", "type": {"qualtype": "float[4]"}}],
    }
    return {
        "records": [*_input_event_support_records(), unavailable],
        "functions": [
            {
                "name": "dvz_const_input",
                "result": {"qualtype": "void"},
                "parameters": [
                    {"name": "value", "type": {"qualtype": "const DvzUnavailable *"}}
                ],
            },
            {
                "name": "dvz_mutable_output",
                "result": {"qualtype": "void"},
                "parameters": [
                    {"name": "value", "type": {"qualtype": "DvzUnavailable *"}}
                ],
            },
            {
                "name": "dvz_by_value_input",
                "result": {"qualtype": "void"},
                "parameters": [{"name": "value", "type": {"qualtype": "DvzUnavailable"}}],
            },
            {
                "name": "dvz_by_value_result",
                "result": {"qualtype": "DvzUnavailable"},
                "parameters": [],
            },
        ],
    }


def _pointer_opaque_api() -> dict:
    return {
        "records": [
            *_input_event_support_records(),
            {
                "name": "DvzNativeOwned",
                "kind": "struct",
                "opaque": False,
                "fields": [{"name": "values", "type": {"qualtype": "float[4]"}}],
            },
            {
                "name": "DvzBorrowedPayload",
                "kind": "struct",
                "opaque": False,
                "fields": [{"name": "values", "type": {"qualtype": "float[4]"}}],
            },
            {
                "name": "DvzPrivateHandle",
                "kind": "struct",
                "opaque": True,
                "fields": [],
            },
        ],
        "typedefs": [
            {
                "name": "DvzBorrowedCallback",
                "type": {"qualtype": "void (*)(const DvzBorrowedPayload *)"},
            }
        ],
        "functions": [
            {
                "name": "dvz_native_owned_create",
                "result": {"qualtype": "DvzNativeOwned *"},
                "parameters": [],
            },
            {
                "name": "dvz_native_owned_destroy",
                "result": {"qualtype": "void"},
                "parameters": [
                    {"name": "value", "type": {"qualtype": "DvzNativeOwned *"}}
                ],
            },
            {
                "name": "dvz_set_borrowed_callback",
                "result": {"qualtype": "void"},
                "parameters": [
                    {"name": "callback", "type": {"qualtype": "DvzBorrowedCallback"}}
                ],
            },
            {
                "name": "dvz_private_handle_get",
                "result": {"qualtype": "DvzPrivateHandle *"},
                "parameters": [],
            },
        ],
    }


def _nested_aligned_record_api() -> dict:
    return {
        "records": [
            *_input_event_support_records(),
            {
                "name": "DvzAlignedChild",
                "kind": "struct",
                "opaque": False,
                "fields": [{"name": "matrix", "type": {"qualtype": "float[16]"}}],
            },
            {
                "name": "DvzAlignedParent",
                "kind": "struct",
                "opaque": False,
                "fields": [
                    {"name": "child", "type": {"qualtype": "DvzAlignedChild"}},
                    {"name": "value", "type": {"qualtype": "uint32_t"}},
                ],
            },
        ],
        "functions": [
            {
                "name": "dvz_nested_input",
                "result": {"qualtype": "void"},
                "parameters": [
                    {"name": "value", "type": {"qualtype": "const DvzAlignedParent *"}}
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
        self.assertEqual(ctypes.sizeof(record), 96)
        self.assertEqual(ctypes.alignment(record), 16)
        self.assertEqual(record.matrix.offset, 16)
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
        record = namespace["DvzAlignedOutput"]
        self.assertEqual(record.matrix.offset, 16)
        self.assertEqual(ctypes.sizeof(record), 96)
        self.assertEqual(
            [name for name, *_ in record._fields_],
            [
                "struct_size",
                "_ctypes_padding_0",
                "matrix",
                "_ctypes_padding_1",
            ],
        )
        self.assertEqual(namespace["_FUNCTION_LAYOUT_DEPENDENCIES"], {})
        self.assertIn("dvz_aligned_output", namespace)


class ConcreteRecordPolicyTests(unittest.TestCase):
    def test_missing_disposition_fails_for_every_unsafe_signature_shape(self) -> None:
        with self.assertRaisesRegex(ValueError, "missing concrete record disposition"):
            ctypes_gen.generate(_unsupported_record_api())

    def test_unsupported_disposition_suppresses_all_unsafe_signature_shapes(self) -> None:
        text, skipped = ctypes_gen.generate(
            _unsupported_record_api(),
            concrete_record_policy={
                "DvzUnavailable": {
                    "disposition": "unsupported",
                    "provenance": [],
                }
            },
        )
        namespace = _execute_generated(text)
        self.assertEqual(skipped, [])
        expected = {
            "dvz_const_input",
            "dvz_mutable_output",
            "dvz_by_value_input",
            "dvz_by_value_result",
        }
        self.assertEqual(set(namespace["_POLICY_UNSUPPORTED_FUNCTIONS"]), expected)
        for name in expected:
            self.assertNotIn(name, namespace)
            self.assertIn("DvzUnavailable", namespace["_UNSUPPORTED_FUNCTIONS"][name])

    def test_nested_conditional_dependency_is_suppressed_when_alignment_is_ineffective(
        self,
    ) -> None:
        text, _ = ctypes_gen.generate(
            _nested_aligned_record_api(),
            forced_layout_records={"DvzAlignedChild", "DvzAlignedParent"},
            required_alignments={"DvzAlignedChild": 16},
        )
        text = text.replace(
            "return ctypes.alignment(_AlignmentProbe) == requested",
            "return False",
        )
        namespace = _execute_generated(text)
        self.assertEqual(ctypes.sizeof(namespace["DvzAlignedChild"]), 0)
        self.assertEqual(ctypes.sizeof(namespace["DvzAlignedParent"]), 0)
        self.assertIn(
            "DvzAlignedChild",
            namespace["_UNSUPPORTED_LAYOUT_RECORDS"]["DvzAlignedParent"],
        )
        self.assertNotIn("dvz_nested_input", namespace)
        diagnostic = namespace["_UNSUPPORTED_FUNCTIONS"]["dvz_nested_input"]
        self.assertIn("DvzAlignedChild", diagnostic)

    def test_explicit_pointer_provenance_and_private_handles_remain(self) -> None:
        policy = {
            "DvzNativeOwned": {
                "disposition": "pointer-opaque",
                "provenance": ["native-returned", "native-owned-input"],
            },
            "DvzBorrowedPayload": {
                "disposition": "pointer-opaque",
                "provenance": ["callback-borrowed:DvzBorrowedCallback"],
            },
        }
        text, skipped = ctypes_gen.generate(
            _pointer_opaque_api(),
            concrete_record_policy=policy,
            callback_policy={"dvz_set_borrowed_callback": "global"},
        )
        namespace = _execute_generated(text)
        self.assertEqual(skipped, [])
        for record_name in ["DvzNativeOwned", "DvzBorrowedPayload", "DvzPrivateHandle"]:
            self.assertEqual(ctypes.sizeof(namespace[record_name]), 0)
            self.assertFalse(hasattr(namespace[record_name], "_fields_"))
        for name in [
            "dvz_native_owned_create",
            "dvz_native_owned_destroy",
            "DvzBorrowedCallback",
            "dvz_set_borrowed_callback",
            "dvz_private_handle_get",
        ]:
            self.assertIn(name, namespace)

    def test_pointer_opaque_provenance_is_direction_specific(self) -> None:
        with self.assertRaisesRegex(ValueError, "native-owned-input"):
            ctypes_gen.generate(
                _pointer_opaque_api(),
                concrete_record_policy={
                    "DvzNativeOwned": {
                        "disposition": "pointer-opaque",
                        "provenance": ["native-returned"],
                    },
                    "DvzBorrowedPayload": {
                        "disposition": "pointer-opaque",
                        "provenance": ["callback-borrowed:DvzBorrowedCallback"],
                    },
                },
            )


class RepositoryConcreteRecordTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.api = json.loads((ROOT / "build/bindings/datoviz_api.json").read_text())
        cls.policy_path = ROOT / "spec/bindings/ctypes.yml"
        cls.record_policy = ctypes_gen._concrete_record_policy_from_policy(cls.policy_path)
        cls.forced = ctypes_gen._layout_records_from_policy(cls.policy_path)
        cls.alignments = ctypes_gen._required_alignments_from_policy(cls.policy_path)

    def _generated_namespace(self, *, effective_alignment: bool) -> dict:
        text, _ = ctypes_gen.generate(
            self.api,
            forced_layout_records=self.forced,
            required_alignments=self.alignments,
            concrete_record_policy=self.record_policy,
            callback_policy=ctypes_gen._callback_policy_from_policy(self.policy_path),
            owned_string_returns=ctypes_gen._owned_string_returns_from_policy(
                self.policy_path
            ),
        )
        if not effective_alignment:
            text = text.replace(
                "return ctypes.alignment(_AlignmentProbe) == requested",
                "return False",
            )
        return _execute_generated(text)

    def test_all_audited_records_have_the_required_disposition(self) -> None:
        namespace = self._generated_namespace(effective_alignment=True)
        dispositions = namespace["_CONCRETE_RECORD_DISPOSITIONS"]
        portable = {
            "DvzBounds",
            "DvzFieldGeometry",
            "DvzGuideHit",
            "DvzGuideLayout",
            "DvzPanzoomEval",
        }
        conditional = {
            "DvzMVP",
            "DvzPanelFrameInfo",
            "DvzPanzoomResolved",
            "DvzVisualTransformDesc",
            "DvzPanelView2DState",
            "DvzPanelView3DState",
        }
        pointer_opaque = {
            "DvzGeometry",
            "DvzTessellatedPath",
            "DvzTextAtlasGlyph",
            "DvzVolumeState",
            "DvzWindowSurface",
            "DvzCanvasLiveImageFrame",
        }
        unsupported = {
            "DvzBarriers",
            "DvzDeviceConfig",
            "DvzGpuCtxConfig",
            "DvzDrp2ColorAttachment",
            "DvzDrp2RenderPassDesc",
            "DvzStreamFrame",
            "DvzWindowExternalSurfaceInfo",
        }
        for name in portable:
            self.assertEqual(dispositions[name], "layout")
        for name in conditional:
            self.assertEqual(dispositions[name], "conditional-layout")
        for name in pointer_opaque:
            self.assertEqual(dispositions[name], "pointer-opaque")
        for name in unsupported:
            self.assertEqual(dispositions[name], "unsupported")

    def test_ineffective_alignment_keeps_portable_families_and_suppresses_aligned_ones(
        self,
    ) -> None:
        namespace = self._generated_namespace(effective_alignment=False)
        portable = {
            "DvzBounds": 56,
            "DvzFieldGeometry": 104,
            "DvzGuideHit": 208,
            "DvzGuideLayout": 208,
            "DvzPanzoomEval": 24,
        }
        for name, size in portable.items():
            self.assertEqual(ctypes.sizeof(namespace[name]), size)
        for name in [
            "dvz_visual_bounds",
            "dvz_panel_visual_bounds",
            "dvz_panel_bounds",
            "dvz_ffi_field_geometry",
            "dvz_sampled_field_set_geometry",
            "dvz_field_geometry",
            "dvz_panel_frame_guide_hit",
            "dvz_panel_frame_guide_layout",
        ]:
            self.assertIn(name, namespace)

        aligned = [
            "DvzMVP",
            "DvzPanelFrameInfo",
            "DvzPanzoomResolved",
            "DvzVisualTransformDesc",
        ]
        for name in aligned:
            self.assertEqual(ctypes.sizeof(namespace[name]), 0)
            self.assertFalse(hasattr(namespace[name], "_fields_"))
        for name in [
            "dvz_arcball_mvp",
            "dvz_camera_mvp",
            "dvz_panzoom_mvp",
            "dvz_panel_frame_info",
            "dvz_panzoom_resolve",
            "dvz_ffi_visual_transform_desc",
            "dvz_visual_set_transform_desc",
            "dvz_visual_transform_desc",
        ]:
            self.assertNotIn(name, namespace)
            self.assertIn(name, namespace["_UNSUPPORTED_FUNCTIONS"])

    def test_repository_visual_transform_uses_extracted_matrix_offset(self) -> None:
        if sys.version_info < (3, 13):
            self.skipTest("effective ctypes.Structure._align_ requires Python 3.13+")
        namespace = self._generated_namespace(effective_alignment=True)
        record = namespace["DvzVisualTransformDesc"]
        self.assertEqual(record.label.offset, 32)
        self.assertEqual(record.matrix.offset, 48)
        self.assertEqual(ctypes.sizeof(record), 112)

    def test_exact_unsupported_omissions_and_pointer_opaque_sentinels(self) -> None:
        namespace = self._generated_namespace(effective_alignment=True)
        unsupported = {
            "dvz_barriers",
            "dvz_barriers_buffer",
            "dvz_barriers_buffer_count",
            "dvz_barriers_capacity",
            "dvz_barriers_dependency_flags",
            "dvz_barriers_flags",
            "dvz_barriers_image",
            "dvz_barriers_image_count",
            "dvz_barriers_memory",
            "dvz_barriers_memory_count",
            "dvz_cmd_barriers",
            "dvz_device_config",
            "dvz_device_config_enable_canvas_extensions",
            "dvz_device_config_request_extension",
            "dvz_device_config_request_queue",
            "dvz_device_config_set_features10",
            "dvz_device_config_set_features11",
            "dvz_device_config_set_features12",
            "dvz_device_config_set_features13",
            "dvz_device_config_set_gpu_index",
            "dvz_device_create",
            "dvz_gpu_ctx_config",
            "dvz_canvas_configure_gpu_ctx",
            "dvz_gpu_ctx",
            "dvz_gpu_ctx_config_add_instance_extension",
            "dvz_gpu_ctx_config_alloc",
            "dvz_gpu_ctx_config_enable_canvas_extensions",
            "dvz_gpu_ctx_config_features10",
            "dvz_gpu_ctx_config_features12",
            "dvz_gpu_ctx_config_features13",
            "dvz_gpu_ctx_config_gpu",
            "dvz_gpu_ctx_config_validation",
            "dvz_drp2_render_pass_desc",
            "dvz_drp2_stream_begin_render_pass_desc",
            "dvz_stream_start",
            "dvz_stream_update",
            "dvz_drp2_runtime_attach_frame_target",
            "dvz_drp2_runtime_copy_texture_to_frame",
            "dvz_window_external_surface_info",
            "dvz_view_external_surface",
            "dvz_view_update_external_surface",
            "dvz_window_wrap_attach_surface",
            "dvz_window_wrap_update_surface",
        }
        self.assertEqual(set(namespace["_POLICY_UNSUPPORTED_FUNCTIONS"]), unsupported)
        for name in unsupported:
            self.assertNotIn(name, namespace)
            self.assertIn(name, namespace["_UNSUPPORTED_FUNCTIONS"])

        preserved = {
            "dvz_geometry",
            "dvz_geometry_destroy",
            "dvz_tessellate_cubic_bezier",
            "dvz_tessellated_path_destroy",
            "dvz_text_atlas_glyph",
            "dvz_volume_state",
            "dvz_window_surface",
            "dvz_window_backend_surface",
            "DvzCanvasLiveImageCallback",
            "DvzCanvasDraw",
            "dvz_canvas_set_draw_callback",
            "dvz_ffi_view_external_surface",
            "dvz_ffi_view_update_external_surface",
        }
        for name in preserved:
            self.assertIn(name, namespace)
        for record_name in self.record_policy:
            if self.record_policy[record_name]["disposition"] == "pointer-opaque":
                self.assertEqual(ctypes.sizeof(namespace[record_name]), 0)
                self.assertFalse(hasattr(namespace[record_name], "_fields_"))

    def test_no_emitted_function_exposes_an_unsafe_zero_size_concrete_record(self) -> None:
        namespace = self._generated_namespace(effective_alignment=True)
        records_by_name = {
            record["name"]: record
            for record in self.api.get("records", [])
            if record.get("name")
        }
        records = set(records_by_name)
        callbacks = ctypes_gen._callback_typedefs(self.api)

        def assert_safe_reference(
            function_name: str,
            reference: tuple[str, int] | None,
            *,
            is_result: bool,
        ) -> None:
            if reference is None:
                return
            record_name, pointer_depth = reference
            record = records_by_name[record_name]
            if record.get("opaque") or not record.get("fields"):
                return
            record_type = namespace[record_name]
            if ctypes.sizeof(record_type) > 0:
                return
            entry = self.record_policy.get(record_name, {})
            self.assertEqual(
                entry.get("disposition"),
                "pointer-opaque",
                f"{function_name} exposes undisposed zero-size {record_name}",
            )
            self.assertGreater(pointer_depth, 0)
            required = "native-returned" if is_result else "native-owned-input"
            self.assertIn(required, entry.get("provenance", []))

        for function in self.api.get("functions", []):
            name = function["name"]
            if name not in namespace:
                continue
            assert_safe_reference(
                name,
                ctypes_gen._record_reference_from_type(
                    function.get("result", {"qualtype": "void"}), records
                ),
                is_result=True,
            )
            for param in function.get("parameters", []):
                type_info = param.get("type", {})
                assert_safe_reference(
                    name,
                    ctypes_gen._record_reference_from_type(type_info, records),
                    is_result=False,
                )
                callback = callbacks.get(
                    ctypes_gen._clean_type(type_info.get("qualtype", ""))
                )
                if callback is None:
                    continue
                for spelling in [callback["result"], *callback["args"]]:
                    reference = ctypes_gen._record_reference(spelling, records)
                    if reference is None:
                        continue
                    record_name, pointer_depth = reference
                    record = records_by_name[record_name]
                    if record.get("opaque") or not record.get("fields"):
                        continue
                    if ctypes.sizeof(namespace[record_name]) > 0:
                        continue
                    self.assertGreater(pointer_depth, 0)
                    self.assertIn(
                        f'callback-borrowed:{callback["name"]}',
                        self.record_policy.get(record_name, {}).get("provenance", []),
                    )


if __name__ == "__main__":
    unittest.main()
