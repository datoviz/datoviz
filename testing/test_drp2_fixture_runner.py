#!/usr/bin/env python3
"""Tests for the DRP2 fixture runner."""

from __future__ import annotations

import json
from pathlib import Path
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.drp2_fixture_runner import DRP2FixtureRunner


def test_drp2_fixture_runner_full_corpus_passes() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixtures = runner.discover([], None, [])
    results = runner.run_fixtures(fixtures)

    assert fixtures
    assert all(result.passed for result in results)


def test_drp2_fixture_runner_can_filter_schema_negatives() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixtures = runner.discover(['spec/drp2/fixtures/negative_schema'], None, ['schema'])
    results = runner.run_fixtures(fixtures)

    assert len(results) == 5
    assert all(result.passed for result in results)
    assert all(result.actual_phase == 'schema_validation' for result in results)


def test_drp2_fixture_runner_can_filter_capability_negatives() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixtures = runner.discover(['spec/drp2/fixtures/negative'], None, ['capability'])
    results = runner.run_fixtures(fixtures)
    fixture_names = {result.fixture_name for result in results}

    assert {
        'invalid_capability_shader_module_format',
        'invalid_feature_required_shader_module_fp64',
    }.issubset(fixture_names)
    assert all(result.passed for result in results)
    assert all(result.actual_phase == 'capability_validation' for result in results)
    assert {result.actual_code for result in results} == {
        'DRP2_ERR_UNSUPPORTED_CAPABILITY',
        'DRP2_ERR_FEATURE_REQUIRED',
    }


def test_drp2_fixture_runner_can_filter_handshake_fixtures() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixtures = runner.discover([], None, ['handshake'])
    results = runner.run_fixtures(fixtures)
    fixture_names = {result.fixture_name for result in results}

    assert {'session_handshake_basic', 'invalid_handshake_unsupported_major_version'}.issubset(
        fixture_names
    )
    assert all(result.passed for result in results)


def test_drp2_fixture_runner_can_filter_error_fixtures() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixtures = runner.discover([], None, ['error'])
    results = runner.run_fixtures(fixtures)
    fixture_names = {result.fixture_name for result in results}

    assert {'error_after_failed_handshake', 'error_does_not_poison_ready_session'}.issubset(
        fixture_names
    )
    assert all(result.passed for result in results)


def test_drp2_fixture_runner_can_filter_render_state_negatives() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixtures = runner.discover(['spec/drp2/fixtures/negative'], None, ['render_state'])
    results = runner.run_fixtures(fixtures)
    fixture_names = {result.fixture_name for result in results}

    assert {'invalid_set_viewport_in_compute_pass', 'invalid_set_scissor_in_compute_pass'}.issubset(
        fixture_names
    )
    assert all(result.passed for result in results)
    assert all(result.actual_phase == 'semantic_validation' for result in results)
    assert all(result.actual_code == 'DRP2_ERR_PASS_MISMATCH' for result in results)


def test_drp2_fixture_runner_can_filter_pipeline_prerequisite_negatives() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixtures = runner.discover(['spec/drp2/fixtures/negative'], None, ['pipeline'])
    results = runner.run_fixtures(fixtures)
    fixture_names = {result.fixture_name for result in results}

    assert {'invalid_draw_without_pipeline', 'invalid_dispatch_without_pipeline'}.issubset(
        fixture_names
    )
    assert all(result.passed for result in results)
    assert all(result.actual_phase == 'semantic_validation' for result in results)
    assert {result.actual_code for result in results} == {
        'DRP2_ERR_INVALID_STATE',
        'DRP2_ERR_INVALID_ARGUMENT',
    }


def test_drp2_fixture_runner_rejects_resubmitted_command_buffer() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_queue_submit_reused_command_buffer.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_queue_submit_reused_command_buffer'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_INVALID_STATE'
    assert result.actual_command_index == 5


def test_drp2_fixture_runner_rejects_duplicate_command_buffer_ids_in_one_submit() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_queue_submit_duplicate_ids_same_submit.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_queue_submit_duplicate_ids_same_submit'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_INVALID_ARGUMENT'
    assert result.actual_command_index == 4


def test_drp2_fixture_runner_rejects_unsupported_major_version() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_handshake_unsupported_major_version.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_handshake_unsupported_major_version'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_UNSUPPORTED_VERSION'
    assert result.actual_command_index == 0


def test_drp2_fixture_runner_rejects_command_before_handshake_reply() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_handshake_command_before_reply.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_handshake_command_before_reply'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_INVALID_STATE'
    assert result.actual_command_index == 1


def test_drp2_fixture_runner_rejects_wrong_shader_stage_in_render_pipeline() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_create_render_pipeline_wrong_shader_stage.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_create_render_pipeline_wrong_shader_stage'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_INVALID_ARGUMENT'
    assert result.actual_command_index == 4


def test_drp2_fixture_runner_rejects_shader_module_fp64_without_capability() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_feature_required_shader_module_fp64.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_feature_required_shader_module_fp64'
    assert result.passed is True
    assert result.actual_phase == 'capability_validation'
    assert result.actual_code == 'DRP2_ERR_FEATURE_REQUIRED'
    assert result.actual_command_index == 2


def test_drp2_fixture_runner_rejects_command_after_failed_handshake() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_handshake_failed_then_command.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_handshake_failed_then_command'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_INVALID_STATE'
    assert result.actual_command_index == 2


def test_drp2_fixture_runner_rejects_duplicate_hello() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_handshake_duplicate_hello.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_handshake_duplicate_hello'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_INVALID_STATE'
    assert result.actual_command_index == 1


def test_drp2_fixture_runner_rejects_duplicate_reply() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_handshake_duplicate_reply.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_handshake_duplicate_reply'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_INVALID_STATE'
    assert result.actual_command_index == 2


def test_drp2_fixture_runner_rejects_error_before_hello() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_error_before_hello.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_error_before_hello'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_INVALID_STATE'
    assert result.actual_command_index == 0


def test_drp2_fixture_runner_rejects_write_texture_bad_mip_level() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_write_texture_bad_mip_level.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_write_texture_bad_mip_level'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_OUT_OF_RANGE'
    assert result.actual_command_index == 3


def test_drp2_fixture_runner_rejects_write_texture_bad_bytes_per_row() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_write_texture_bad_bytes_per_row.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_write_texture_bad_bytes_per_row'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_LAYOUT'
    assert result.actual_command_index == 3


def test_drp2_fixture_runner_rejects_write_texture_short_payload() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_write_texture_short_payload.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_write_texture_short_payload'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_LAYOUT'
    assert result.actual_command_index == 3


def test_drp2_fixture_runner_rejects_copy_buffer_to_texture_bad_rows_per_image() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_copy_buffer_to_texture_bad_rows_per_image.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_copy_buffer_to_texture_bad_rows_per_image'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_LAYOUT'
    assert result.actual_command_index == 5


def test_drp2_fixture_runner_rejects_draw_after_end_render_pass() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_draw_after_end_render_pass.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_draw_after_end_render_pass'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_INVALID_STATE'
    assert result.actual_command_index == 10


def test_drp2_fixture_runner_rejects_dispatch_after_end_compute_pass() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_dispatch_after_end_compute_pass.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_dispatch_after_end_compute_pass'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_INVALID_STATE'
    assert result.actual_command_index == 8


def test_drp2_fixture_runner_rejects_copy_after_finish_encoder() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_copy_after_finish_encoder.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_copy_after_finish_encoder'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_INVALID_STATE'
    assert result.actual_command_index == 6


def test_drp2_fixture_runner_rejects_copy_texture_to_buffer_bad_mip_level() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_copy_texture_to_buffer_bad_mip_level.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_copy_texture_to_buffer_bad_mip_level'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_OUT_OF_RANGE'
    assert result.actual_command_index == 5


def test_drp2_fixture_runner_rejects_copy_texture_to_buffer_bad_bytes_per_row() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_copy_texture_to_buffer_bad_bytes_per_row.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_copy_texture_to_buffer_bad_bytes_per_row'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_LAYOUT'
    assert result.actual_command_index == 5


def test_drp2_fixture_runner_rejects_copy_texture_to_buffer_bad_rows_per_image() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_copy_texture_to_buffer_bad_rows_per_image.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_copy_texture_to_buffer_bad_rows_per_image'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_LAYOUT'
    assert result.actual_command_index == 5


def test_drp2_fixture_runner_can_filter_vertex_binding_negatives() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixtures = runner.discover(['spec/drp2/fixtures/negative'], None, ['vertex_binding'])
    results = runner.run_fixtures(fixtures)
    fixture_names = {result.fixture_name for result in results}

    assert {'invalid_draw_missing_vertex_buffer'}.issubset(fixture_names)
    assert all(result.passed for result in results)
    assert all(result.actual_phase == 'semantic_validation' for result in results)
    assert all(result.actual_code == 'DRP2_ERR_INVALID_STATE' for result in results)


def test_drp2_fixture_runner_rejects_missing_index_buffer_binding() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_drawindexed_missing_index_buffer.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_drawindexed_missing_index_buffer'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_INVALID_STATE'
    assert result.actual_command_index == 11


def test_drp2_fixture_runner_can_filter_bind_group_negatives() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixtures = runner.discover(['spec/drp2/fixtures/negative'], None, ['bind_group'])
    results = runner.run_fixtures(fixtures)
    fixture_names = {result.fixture_name for result in results}

    assert {
        'invalid_bind_group_entries_mismatch_layout',
        'invalid_set_bind_group_dynamic_offsets_missing',
        'invalid_bind_group_wrong_resource_usage',
    }.issubset(fixture_names)
    assert all(result.passed for result in results)
    assert {result.actual_phase for result in results} == {
        'schema_validation',
        'semantic_validation',
    }


def test_drp2_fixture_runner_rejects_wrong_bind_group_resource_kind() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_bind_group_wrong_resource_kind.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_bind_group_wrong_resource_kind'
    assert result.passed is True
    assert result.actual_phase == 'schema_validation'
    assert result.actual_code == 'DRP2_ERR_INVALID_ARGUMENT'
    assert result.actual_command_index == 4


def test_drp2_fixture_runner_rejects_wrong_bind_group_resource_usage() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_bind_group_wrong_resource_usage.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_bind_group_wrong_resource_usage'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_USAGE'
    assert result.actual_command_index == 4


def test_drp2_fixture_runner_rejects_set_scissor_in_compute_pass() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_set_scissor_in_compute_pass.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_set_scissor_in_compute_pass'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_PASS_MISMATCH'
    assert result.actual_command_index == 4


def test_drp2_fixture_runner_rejects_set_blend_constant_in_compute_pass() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_set_blend_constant_in_compute_pass.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_set_blend_constant_in_compute_pass'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_PASS_MISMATCH'
    assert result.actual_command_index == 4


def test_drp2_fixture_runner_rejects_set_stencil_reference_in_compute_pass() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_set_stencil_reference_in_compute_pass.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_set_stencil_reference_in_compute_pass'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_PASS_MISMATCH'
    assert result.actual_command_index == 4


def test_drp2_fixture_runner_rejects_bind_group_entries_mismatch_layout() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_bind_group_entries_mismatch_layout.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_bind_group_entries_mismatch_layout'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_INVALID_ARGUMENT'
    assert result.actual_command_index == 4


def test_drp2_fixture_runner_rejects_wrong_bind_group_layout_slot() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_set_bind_group_wrong_layout_slot.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_set_bind_group_wrong_layout_slot'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_INVALID_STATE'
    assert result.actual_command_index == 14


def test_drp2_fixture_runner_rejects_missing_dynamic_offsets() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_set_bind_group_dynamic_offsets_missing.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_set_bind_group_dynamic_offsets_missing'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_INVALID_STATE'
    assert result.actual_command_index == 14


def test_drp2_fixture_runner_rejects_misordered_dynamic_offsets() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_set_bind_group_dynamic_offsets_misordered.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_set_bind_group_dynamic_offsets_misordered'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_OUT_OF_RANGE'
    assert result.actual_command_index == 15


def test_drp2_fixture_runner_rejects_destroying_bind_group_layout_still_referenced_by_recorded_work() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path(
        'spec/drp2/fixtures/negative/invalid_destroy_bind_group_layout_still_referenced_by_recorded_work.json'
    )
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_destroy_bind_group_layout_still_referenced_by_recorded_work'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_USAGE'
    assert result.actual_command_index == 18


def test_drp2_fixture_runner_rejects_destroying_bind_group_layout_still_referenced_by_submitted_work() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path(
        'spec/drp2/fixtures/negative/invalid_destroy_bind_group_layout_still_referenced_by_submitted_work.json'
    )
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_destroy_bind_group_layout_still_referenced_by_submitted_work'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_USAGE'
    assert result.actual_command_index == 19


def test_drp2_fixture_runner_rejects_set_bind_group_after_pipeline_rebind_without_required_dynamic_offsets() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path(
        'spec/drp2/fixtures/negative/invalid_set_bind_group_after_pipeline_rebind_missing_dynamic_offsets.json'
    )
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_set_bind_group_after_pipeline_rebind_missing_dynamic_offsets'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_INVALID_STATE'
    assert result.actual_command_index == 19


def test_drp2_fixture_runner_rejects_draw_after_pipeline_rebind_missing_new_vertex_slot() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path(
        'spec/drp2/fixtures/negative/invalid_draw_after_pipeline_rebind_missing_new_vertex_slot.json'
    )
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_draw_after_pipeline_rebind_missing_new_vertex_slot'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_INVALID_STATE'
    assert result.actual_command_index == 15


def test_drp2_fixture_runner_cli_json_output_shape() -> None:
    root = Path(__file__).resolve().parents[1]
    fixture = 'spec/drp2/fixtures/positive/write_buffer_basic.json'
    proc = subprocess.run(
        [sys.executable, 'tools/drp2_fixture_runner.py', '--json', fixture],
        cwd=root,
        check=True,
        capture_output=True,
        text=True,
    )
    payload = json.loads(proc.stdout)

    assert isinstance(payload, list)
    assert len(payload) == 1
    result = payload[0]
    assert result['fixture_name'] == 'write_buffer_basic'
    assert result['fixture_path'] == fixture
    assert result['actual_outcome'] == 'success'
    assert result['actual_phase'] is None
    assert result['actual_code'] is None
    assert result['passed'] is True


def test_drp2_fixture_runner_cli_json_output_for_capability_negative() -> None:
    root = Path(__file__).resolve().parents[1]
    fixture = 'spec/drp2/fixtures/negative/invalid_capability_texture_format.json'
    proc = subprocess.run(
        [sys.executable, 'tools/drp2_fixture_runner.py', '--json', fixture],
        cwd=root,
        check=True,
        capture_output=True,
        text=True,
    )
    payload = json.loads(proc.stdout)

    assert isinstance(payload, list)
    assert len(payload) == 1
    result = payload[0]
    assert result['fixture_name'] == 'invalid_capability_texture_format'
    assert result['fixture_path'] == fixture
    assert result['actual_outcome'] == 'error'
    assert result['actual_phase'] == 'capability_validation'
    assert result['actual_code'] == 'DRP2_ERR_UNSUPPORTED_CAPABILITY'
    assert result['actual_command_index'] == 2
    assert result['passed'] is True
