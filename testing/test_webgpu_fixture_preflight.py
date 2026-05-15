#!/usr/bin/env python3
"""Tests for the WebGPU fixture preflight checker."""

from __future__ import annotations

import copy
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.webgpu_fixture_preflight import WebGPUFixturePreflight, WebGPUPreflightFailure


ROOT_DIR = Path(__file__).resolve().parents[1]


def _load_fixture(relative_path: str) -> dict:
    with (ROOT_DIR / relative_path).open('r', encoding='utf-8') as stream:
        return json.load(stream)


def test_webgpu_preflight_manifest_passes() -> None:
    preflight = WebGPUFixturePreflight(ROOT_DIR)
    fixtures = preflight.discover([])
    results = preflight.run_fixtures(fixtures)

    assert fixtures
    assert all(result.passed for result in results)


def test_webgpu_preflight_rejects_short_vertex_buffer() -> None:
    fixture = _load_fixture('spec/drp2/fixtures/positive/scene_static_render_from_c.json')
    broken = copy.deepcopy(fixture)
    for command in broken['commands']:
        if command['cmd'] == 'CreateBuffer' and command['id'] == 20:
            command['size'] = 16
            break

    preflight = WebGPUFixturePreflight(ROOT_DIR)
    try:
        preflight.validate_fixture(broken)
    except WebGPUPreflightFailure as exc:
        assert exc.command_index is not None
        assert 'vertex buffer slot 0 needs 36 bytes but binding has 16' in exc.message
    else:
        raise AssertionError('short vertex buffer unexpectedly passed WebGPU preflight')


def test_webgpu_preflight_rejects_missing_pipeline_metadata() -> None:
    fixture = _load_fixture('spec/drp2/fixtures/positive/scene_static_render_from_c.json')
    broken = copy.deepcopy(fixture)
    for command in broken['commands']:
        if command['cmd'] == 'CreateRenderPipeline':
            del command['color_targets']
            break

    preflight = WebGPUFixturePreflight(ROOT_DIR)
    try:
        preflight.validate_fixture(broken)
    except WebGPUPreflightFailure as exc:
        assert exc.command_index is not None
        assert 'needs explicit color_targets' in exc.message
    else:
        raise AssertionError('pipeline without color_targets unexpectedly passed WebGPU preflight')
