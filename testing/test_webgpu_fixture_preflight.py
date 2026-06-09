#!/usr/bin/env python3
"""Tests for the WebGPU fixture preflight checker."""

from __future__ import annotations

import copy
import json
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.webgpu_fixture_preflight import WebGPUFixturePreflight, WebGPUPreflightFailure


ROOT_DIR = Path(__file__).resolve().parents[1]

ACTIVE_SCHEMA_RE = re.compile(r'- `commands/([^`/]+)\.json`')
JS_CASE_RE = re.compile(r'case "([^"]+)":')


def _load_fixture(relative_path: str) -> dict:
    with (ROOT_DIR / relative_path).open('r', encoding='utf-8') as stream:
        return json.load(stream)


def _manifest() -> dict:
    return _load_fixture('examples/webgpu/fixture_manifest.json')


def _load_strict_stream(relative_path: str) -> dict:
    fixture = _load_fixture(relative_path)
    for command in fixture['commands']:
        if command['cmd'] == 'CreateRenderPipeline':
            command.setdefault('color_targets', [{'format': 'rgba8unorm'}])
    return fixture


def _active_schema_commands() -> set[str]:
    readme = (ROOT_DIR / 'spec/drp2/schema/README.md').read_text(encoding='utf-8')
    active_section = readme.split('Deferred, non-authoritative files:', maxsplit=1)[0]
    return set(ACTIVE_SCHEMA_RE.findall(active_section))


def _webgpu_js_command_cases() -> set[str]:
    source = (ROOT_DIR / 'web/drp2/webgpu.js').read_text(encoding='utf-8')
    start = source.index('export async function executeDrp2Stream')
    end = source.index('export async function executeDrp2StreamChecked')
    return set(JS_CASE_RE.findall(source[start:end]))


def test_webgpu_preflight_manifest_passes() -> None:
    preflight = WebGPUFixturePreflight(ROOT_DIR)
    fixtures = preflight.discover([])
    results = preflight.run_fixtures(fixtures)

    assert fixtures
    assert all(result.passed for result in results)


def test_webgpu_manifest_matches_fixture_tree() -> None:
    manifest = _manifest()
    positive = sorted(
        f'/spec/drp2/fixtures/positive/{path.name}'
        for path in (ROOT_DIR / 'spec/drp2/fixtures/positive').glob('*.json')
    )
    negative = sorted(
        f'/spec/drp2/fixtures/negative/{path.name}'
        for path in (ROOT_DIR / 'spec/drp2/fixtures/negative').glob('*.json')
    )

    assert manifest['positive'] == positive
    assert manifest['negative_parity'] == negative
    assert manifest['webgpu_streams'] == [
        '/examples/webgpu/streams/attachment_multi_color_wgsl.json',
        '/examples/webgpu/streams/attachment_depth_wgsl.json',
    ]


def test_webgpu_command_coverage_matches_active_schema() -> None:
    active_commands = _active_schema_commands()
    webgpu_cases = _webgpu_js_command_cases()

    missing = sorted(active_commands - webgpu_cases)

    assert active_commands
    assert not missing, (
        'active DRP2 commands need a WebGPU switch case, either implemented or explicitly '
        f'rejected: {missing}'
    )


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


def test_webgpu_preflight_rejects_unsupported_shader_format() -> None:
    fixture = _load_fixture('spec/drp2/fixtures/positive/scene_static_render_from_c.json')
    broken = copy.deepcopy(fixture)
    for command in broken['commands']:
        if command['cmd'] == 'CreateShaderModule':
            command['format'] = 'glsl'
            break

    preflight = WebGPUFixturePreflight(ROOT_DIR)
    try:
        preflight.validate_fixture(broken)
    except WebGPUPreflightFailure as exc:
        assert exc.command_index is not None
        assert 'unsupported shader format glsl' in exc.message
    else:
        raise AssertionError('unsupported shader format unexpectedly passed WebGPU preflight')


def test_webgpu_preflight_scene_wgsl_stream_bindings_pass() -> None:
    preflight = WebGPUFixturePreflight(ROOT_DIR)
    for relative_path in (
        'examples/webgpu/streams/scene_primitive_wgsl.json',
        'examples/webgpu/streams/scene_point_wgsl.json',
        'examples/webgpu/streams/scene_image_wgsl.json',
    ):
        preflight.validate_fixture(_load_strict_stream(relative_path))


def test_webgpu_preflight_attachment_streams_pass() -> None:
    preflight = WebGPUFixturePreflight(ROOT_DIR)
    for relative_path in (
        'examples/webgpu/streams/attachment_multi_color_wgsl.json',
        'examples/webgpu/streams/attachment_depth_wgsl.json',
    ):
        preflight.validate_fixture(_load_fixture(relative_path))


def test_webgpu_preflight_wboit_fixture_passes() -> None:
    preflight = WebGPUFixturePreflight(ROOT_DIR)
    fixture = _load_fixture('spec/drp2/fixtures/positive/wboit_accumulation_resolve.json')

    preflight.validate_fixture(fixture)


def test_webgpu_preflight_rg32uint_query_fixture_passes() -> None:
    preflight = WebGPUFixturePreflight(ROOT_DIR)
    fixture = _load_fixture('spec/drp2/fixtures/positive/query_rg32uint_readback.json')

    preflight.validate_fixture(fixture)


def test_webgpu_preflight_rejects_missing_scene_common_viewport_binding() -> None:
    broken = _load_strict_stream('examples/webgpu/streams/scene_point_wgsl.json')
    for command in broken['commands']:
        if command['cmd'] == 'CreateBindGroupLayout' and command['id'] == 5000:
            command['entries'] = [entry for entry in command['entries'] if entry['binding'] != 1]
            break

    preflight = WebGPUFixturePreflight(ROOT_DIR)
    try:
        preflight.validate_fixture(broken)
    except WebGPUPreflightFailure as exc:
        assert exc.command_index is not None
        assert 'requires group 0 binding 1, missing from layout 5000' in exc.message
    else:
        raise AssertionError('scene common layout without viewport unexpectedly passed preflight')


def test_webgpu_preflight_rejects_wrong_scene_image_resource_binding() -> None:
    broken = _load_strict_stream('examples/webgpu/streams/scene_image_wgsl.json')
    for command in broken['commands']:
        if command['cmd'] == 'CreateBindGroupLayout' and command['id'] == 5000:
            command['entries'][0]['binding_type'] = 'uniform_buffer'
            break

    preflight = WebGPUFixturePreflight(ROOT_DIR)
    try:
        preflight.validate_fixture(broken)
    except WebGPUPreflightFailure as exc:
        assert exc.command_index is not None
        assert 'group 1 binding 0 uses sampled_texture, layout 5000 uses uniform_buffer' in exc.message
    else:
        raise AssertionError('scene image layout with wrong resource type unexpectedly passed preflight')


def test_webgpu_preflight_rejects_storage_access_mismatch() -> None:
    fixture = _load_fixture('spec/drp2/fixtures/positive/scene_compute_assisted_from_c.json')
    broken = copy.deepcopy(fixture)
    for command in broken['commands']:
        if command['cmd'] == 'CreateBindGroupLayout' and command['id'] == 100:
            command['entries'][0]['access'] = 'read_write'
            break

    preflight = WebGPUFixturePreflight(ROOT_DIR)
    try:
        preflight.validate_fixture(broken)
    except WebGPUPreflightFailure as exc:
        assert exc.command_index is not None
        assert 'group 0 binding 0 uses read storage access' in exc.message
    else:
        raise AssertionError('storage access mismatch unexpectedly passed WebGPU preflight')


def test_webgpu_preflight_rejects_color_attachment_count_mismatch() -> None:
    broken = _load_fixture('examples/webgpu/streams/attachment_multi_color_wgsl.json')
    for command in broken['commands']:
        if command['cmd'] == 'CreateRenderPipeline':
            command['color_targets'] = command['color_targets'][:1]
            break

    preflight = WebGPUFixturePreflight(ROOT_DIR)
    try:
        preflight.validate_fixture(broken)
    except WebGPUPreflightFailure as exc:
        assert exc.command_index is not None
        assert 'color target count 1 does not match render pass' in exc.message
    else:
        raise AssertionError('color attachment count mismatch unexpectedly passed preflight')


def test_webgpu_preflight_rejects_color_attachment_format_mismatch() -> None:
    broken = _load_fixture('examples/webgpu/streams/attachment_multi_color_wgsl.json')
    for command in broken['commands']:
        if command['cmd'] == 'CreateTexture' and command['id'] == 2:
            command['format'] = 'r16float'
            break

    preflight = WebGPUFixturePreflight(ROOT_DIR)
    try:
        preflight.validate_fixture(broken)
    except WebGPUPreflightFailure as exc:
        assert exc.command_index is not None
        assert 'color target 1 format rgba8unorm does not match render pass' in exc.message
    else:
        raise AssertionError('color attachment format mismatch unexpectedly passed preflight')


def test_webgpu_preflight_rejects_depth_attachment_mismatch() -> None:
    broken = _load_fixture('examples/webgpu/streams/attachment_depth_wgsl.json')
    for command in broken['commands']:
        if command['cmd'] == 'BeginRenderPass':
            del command['depth_stencil_attachment']
            break

    preflight = WebGPUFixturePreflight(ROOT_DIR)
    try:
        preflight.validate_fixture(broken)
    except WebGPUPreflightFailure as exc:
        assert exc.command_index is not None
        assert 'depth_stencil format depth32float does not match render pass' in exc.message
    else:
        raise AssertionError('depth attachment mismatch unexpectedly passed preflight')


def test_webgpu_preflight_rejects_invalid_color_target_state() -> None:
    broken = _load_fixture('examples/webgpu/streams/attachment_multi_color_wgsl.json')
    for command in broken['commands']:
        if command['cmd'] == 'CreateRenderPipeline':
            command['color_targets'][0]['write_mask'] = ['all', 'red']
            break

    preflight = WebGPUFixturePreflight(ROOT_DIR)
    try:
        preflight.validate_fixture(broken)
    except WebGPUPreflightFailure as exc:
        assert exc.command_index is not None
        assert 'write_mask cannot combine all' in exc.message
    else:
        raise AssertionError('invalid color target state unexpectedly passed preflight')


def test_webgpu_preflight_rejects_invalid_load_store_state() -> None:
    broken = _load_fixture('examples/webgpu/streams/attachment_multi_color_wgsl.json')
    for command in broken['commands']:
        if command['cmd'] == 'BeginRenderPass':
            command['color_attachments'][0]['load_op'] = 'preserve'
            break

    preflight = WebGPUFixturePreflight(ROOT_DIR)
    try:
        preflight.validate_fixture(broken)
    except WebGPUPreflightFailure as exc:
        assert exc.command_index is not None
        assert 'unsupported load_op: preserve' in exc.message
    else:
        raise AssertionError('invalid load/store state unexpectedly passed preflight')


def test_webgpu_preflight_rejects_scissor_larger_than_attachment() -> None:
    broken = _load_fixture('examples/webgpu/streams/attachment_multi_color_wgsl.json')
    for command in broken['commands']:
        if command['cmd'] == 'SetScissor':
            command['width'] = 640
            command['height'] = 480
            break

    preflight = WebGPUFixturePreflight(ROOT_DIR)
    try:
        preflight.validate_fixture(broken)
    except WebGPUPreflightFailure as exc:
        assert exc.command_index is not None
        assert 'scissor rect 0x0 640x480 exceeds render pass extent 64x64' in exc.message
    else:
        raise AssertionError('oversized scissor unexpectedly passed preflight')
