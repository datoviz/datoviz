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
