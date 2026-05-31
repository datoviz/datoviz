#!/usr/bin/env python3
"""Tests for the test inventory helper."""

from __future__ import annotations

import importlib.util
import sys
from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parents[1]
TOOL_PATH = ROOT_DIR / "tools" / "test_inventory.py"


def _load_tool():
    spec = importlib.util.spec_from_file_location("test_inventory", TOOL_PATH)
    assert spec is not None
    assert spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def test_test_inventory_parses_default_and_named_groups() -> None:
    tool = _load_tool()
    list_text = "\n".join(
        [
            (
                "app/config_defaults  function=test_app_config_defaults resources=cpu "
                "isolation=serial fixture= fixture_scope=none"
            ),
            (
                "drp2/vklite-runtime/draws_render_pass  "
                "function=test_drp2_runtime_vklite_draws_render_pass "
                "resources=cpu,gpu,vulkan isolation=serial "
                "fixture=drp2-vklite-runtime fixture_scope=process"
            ),
        ]
    )
    groups_text = "\n".join(["app/default", "drp2/vklite-runtime"])

    cases = tool.parse_list_output(list_text, groups_text)

    assert [(case.module, case.group, case.name) for case in cases] == [
        ("app", "default", "config_defaults"),
        ("drp2", "vklite-runtime", "draws_render_pass"),
    ]
    assert cases[0].lane == "fast-cpu"
    assert cases[1].lane == "runtime-vklite"
    assert cases[1].case_id == "drp2/vklite-runtime/test_drp2_runtime_vklite_draws_render_pass"
    assert cases[1].resources == ["cpu", "gpu", "vulkan"]
    assert cases[1].fixture == "drp2-vklite-runtime"


def test_test_inventory_classifies_scene_semantic_and_render_cases() -> None:
    tool = _load_tool()
    list_text = "\n".join(
        [
            (
                "scene/scene-graph/point_emit  function=test_scene_point_emit "
                "resources=cpu isolation=serial fixture= fixture_scope=none"
            ),
            (
                "scene/app-offscreen/clear  function=test_scene_app_clear "
                "resources=cpu,gpu,vulkan isolation=serial "
                "fixture=scene-app-gpu fixture_scope=process"
            ),
            (
                "scene/query/item_pick  function=test_scene_query_item_pick "
                "resources=cpu,gpu,vulkan isolation=serial "
                "fixture=scene-app-gpu fixture_scope=process"
            ),
        ]
    )
    groups_text = "\n".join(["scene/scene-graph", "scene/app-offscreen", "scene/query"])

    cases = tool.parse_list_output(list_text, groups_text)

    assert [case.lane for case in cases] == [
        "scene-semantic",
        "render-smoke",
        "render-conformance",
    ]


def test_test_inventory_keeps_cpu_resize_cases_semantic() -> None:
    tool = _load_tool()
    list_text = "\n".join(
        [
            (
                "scene/axis/panzoom_resize_visual_smoke  "
                "function=test_axis_panzoom_resize_visual_smoke "
                "resources=cpu isolation=serial fixture= fixture_scope=none"
            ),
            (
                "canvas/resize_recreate_refreshes_state  "
                "function=test_canvas_resize_recreate_refreshes_state "
                "resources=cpu,gpu,vulkan,glfw isolation=process "
                "fixture= fixture_scope=none"
            ),
        ]
    )
    groups_text = "\n".join(["scene/axis", "canvas/default"])

    cases = tool.parse_list_output(list_text, groups_text)

    assert [case.lane for case in cases] == ["scene-semantic", "slow-churn"]


def test_test_inventory_keeps_video_only_cases_out_of_render_smoke() -> None:
    tool = _load_tool()
    list_text = (
        "video/offline_headless_encode  function=test_video_offline_headless_encode "
        "resources=filesystem,video isolation=process fixture= fixture_scope=none"
    )
    groups_text = "video/default"

    cases = tool.parse_list_output(list_text, groups_text)

    assert cases[0].lane == "fast-cpu"


def test_test_inventory_routes_gpu_metadata_out_of_cpu_lanes() -> None:
    tool = _load_tool()
    list_text = "\n".join(
        [
            (
                "app/resources_owned_defaults  function=test_app_resources_owned_defaults "
                "resources=cpu,gpu,vulkan isolation=process fixture= fixture_scope=none"
            ),
            (
                "drp2/runtime-lifecycle/download_buffer_rejects_out_of_range  "
                "function=test_drp2_runtime_download_buffer_rejects_out_of_range "
                "resources=cpu,gpu,vulkan isolation=process fixture= fixture_scope=none"
            ),
        ]
    )
    groups_text = "\n".join(["app/default", "drp2/runtime-lifecycle"])

    cases = tool.parse_list_output(list_text, groups_text)

    assert [case.lane for case in cases] == ["render-smoke", "runtime-vklite"]


def test_test_inventory_filters_lanes() -> None:
    tool = _load_tool()
    list_text = "\n".join(
        [
            (
                "common/alloc/basic  function=test_alloc_basic resources=cpu "
                "isolation=serial fixture= fixture_scope=none"
            ),
            (
                "scene/query/resolves_sample  function=test_scene_query_resolves_sample "
                "resources=cpu,gpu,vulkan isolation=serial fixture=scene-app-gpu "
                "fixture_scope=process"
            ),
        ]
    )
    groups_text = "\n".join(["common/alloc", "scene/query"])

    cases = tool.parse_list_output(list_text, groups_text)
    filtered = tool._filter_cases(cases, "render-conformance")

    assert [case.case_id for case in filtered] == [
        "scene/query/test_scene_query_resolves_sample"
    ]
