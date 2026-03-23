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
