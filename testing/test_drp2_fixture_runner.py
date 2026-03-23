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

    assert len(results) == 3
    assert all(result.passed for result in results)
    assert all(result.actual_phase == 'schema_validation' for result in results)


def test_drp2_fixture_runner_can_filter_capability_negatives() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixtures = runner.discover(['spec/drp2/fixtures/negative'], None, ['capability'])
    results = runner.run_fixtures(fixtures)

    assert len(results) == 4
    assert all(result.passed for result in results)
    assert all(result.actual_phase == 'capability_validation' for result in results)
    assert all(result.actual_code == 'DRP2_ERR_UNSUPPORTED_CAPABILITY' for result in results)


def test_drp2_fixture_runner_can_filter_pipeline_prerequisite_negatives() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixtures = runner.discover(['spec/drp2/fixtures/negative'], None, ['pipeline'])
    results = runner.run_fixtures(fixtures)

    assert len(results) == 2
    assert all(result.passed for result in results)
    assert all(result.actual_phase == 'semantic_validation' for result in results)
    assert all(result.actual_code == 'DRP2_ERR_INVALID_STATE' for result in results)


def test_drp2_fixture_runner_rejects_resubmitted_command_buffer() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_queue_submit_reused_command_buffer.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_queue_submit_reused_command_buffer'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_INVALID_STATE'
    assert result.actual_command_index == 3


def test_drp2_fixture_runner_can_filter_vertex_binding_negatives() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixtures = runner.discover(['spec/drp2/fixtures/negative'], None, ['vertex_binding'])
    results = runner.run_fixtures(fixtures)

    assert len(results) == 1
    assert results[0].passed is True
    assert results[0].actual_phase == 'semantic_validation'
    assert results[0].actual_code == 'DRP2_ERR_INVALID_STATE'
    assert results[0].actual_command_index == 5


def test_drp2_fixture_runner_rejects_missing_index_buffer_binding() -> None:
    runner = DRP2FixtureRunner(Path(__file__).resolve().parents[1])
    fixture = Path('spec/drp2/fixtures/negative/invalid_drawindexed_missing_index_buffer.json')
    result = runner.run_fixture(runner.root_dir / fixture)

    assert result.fixture_name == 'invalid_drawindexed_missing_index_buffer'
    assert result.passed is True
    assert result.actual_phase == 'semantic_validation'
    assert result.actual_code == 'DRP2_ERR_INVALID_STATE'
    assert result.actual_command_index == 7


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
    fixture = 'spec/drp2/fixtures/negative/invalid_capability_compute_disabled.json'
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
    assert result['fixture_name'] == 'invalid_capability_compute_disabled'
    assert result['fixture_path'] == fixture
    assert result['actual_outcome'] == 'error'
    assert result['actual_phase'] == 'capability_validation'
    assert result['actual_code'] == 'DRP2_ERR_UNSUPPORTED_CAPABILITY'
    assert result['actual_command_index'] == 1
    assert result['passed'] is True
