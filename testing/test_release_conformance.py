"""Tests for installed-wheel release conformance evidence."""

from __future__ import annotations

import json
import struct
import tarfile
import zlib
from argparse import Namespace
from pathlib import Path

from tools.release_wheels import conformance


def _chunk(kind: bytes, payload: bytes) -> bytes:
    return (
        struct.pack('>I', len(payload))
        + kind
        + payload
        + struct.pack('>I', zlib.crc32(kind + payload) & 0xFFFFFFFF)
    )


def _png(path: Path, rgba: tuple[int, int, int, int]) -> None:
    width, height = 2, 2
    pixels = bytes(rgba) * width
    raw = b''.join(b'\0' + pixels for _ in range(height))
    path.write_bytes(
        b'\x89PNG\r\n\x1a\n'
        + _chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0))
        + _chunk(b'IDAT', zlib.compress(raw))
        + _chunk(b'IEND', b'')
    )


def _evidence(
    root: Path, machine_id: str, status: str = 'pass', *, physical: bool = False
) -> Path:
    evidence_dir = root / machine_id
    captures = evidence_dir / 'captures'
    captures.mkdir(parents=True)
    _png(captures / 'point.png', (20, 40, 60, 255))
    environment = {
        'platform': {'system': 'TestOS', 'machine': 'test64'},
        'execution_class': 'physical-interactive' if physical else 'github-hosted-software-gpu',
    }
    (evidence_dir / 'environment.json').write_text(json.dumps(environment), encoding='utf8')
    manual = {
        'state': 'approved' if physical else 'not-applicable',
        'scenarios': [],
    }
    (evidence_dir / 'manual-observations.json').write_text(json.dumps(manual), encoding='utf8')
    evidence = {
        'schema': conformance.EVIDENCE_SCHEMA,
        'version': '0.4.0rc1',
        'campaign': {
            'wheel_run_id': '123',
            'artifact_commit': 'artifact',
            'validator_commit': 'validator',
        },
        'machine_id': machine_id,
        'execution_class': environment['execution_class'],
        'mode': 'physical' if physical else 'unattended',
        'artifact_checksums': {'wheel': {'name': 'wheel.whl', 'sha256': 'abc'}},
        'environment': 'environment.json',
        'results': [],
        'captures': [
            {
                'scenario': 'point',
                'path': 'captures/point.png',
                'sha256': conformance.sha256(captures / 'point.png'),
                'deterministic': True,
                'stats': conformance.png_stats(captures / 'point.png'),
            }
        ],
        'capture_comparisons': [
            {
                'id': 'c-python-point-render',
                'status': 'pass',
                'scenarios': ['point'],
            }
        ],
        'manual': manual,
        'failures': [] if status == 'pass' else [{'id': 'test'}],
        'status': status,
    }
    (evidence_dir / 'evidence.json').write_text(json.dumps(evidence), encoding='utf8')
    return evidence_dir


def test_png_stats_rgba(tmp_path: Path) -> None:
    """RGBA PNG statistics expose deterministic channel facts."""
    path = tmp_path / 'capture.png'
    _png(path, (20, 40, 60, 255))

    stats = conformance.png_stats(path)

    assert stats['width'] == 2
    assert stats['height'] == 2
    assert stats['channel_mean'] == [20.0, 40.0, 60.0, 255.0]
    assert stats['nontransparent_fraction'] == 1.0
    assert len(stats['pixel_sha256']) == 64


def test_capture_parity_compares_decoded_pixels() -> None:
    """Equivalent C and Python scenarios require identical decoded pixels."""
    scenarios = conformance.CAPTURE_PARITY_GROUPS['c-python-point-render']
    captures = [
        {
            'scenario': scenario,
            'stats': {
                'width': 2,
                'height': 2,
                'channels': 4,
                'pixel_sha256': 'same-pixels',
            },
        }
        for scenario in scenarios
    ]

    comparisons = conformance.capture_parity_records(captures)

    assert comparisons[0]['status'] == 'pass'
    captures[-1]['stats']['pixel_sha256'] = 'different-pixels'
    assert conformance.capture_parity_records(captures)[0]['status'] == 'fail'


def test_validate_wheels_run_uses_workflow_path_not_display_case() -> None:
    """Run identity follows the canonical file even when its API name is title-cased."""
    run = {
        'name': 'Wheels',
        'path': '.github/workflows/wheels.yml',
        'status': 'completed',
        'conclusion': 'success',
    }

    conformance.validate_wheels_run(run, '123')


def test_run_parser_records_capability_skips() -> None:
    """Hosted lanes can disable unavailable checks without changing shared defaults."""
    args = conformance.parse_args(
        [
            'run',
            '--wheel',
            'wheel.whl',
            '--output-dir',
            'evidence',
            '--version',
            '0.4.0rc1',
            '--wheel-run-id',
            '123',
            '--artifact-commit',
            'artifact',
            '--machine-id',
            'windows-arm64',
            '--execution-class',
            'github-hosted-no-gpu',
            '--no-render',
            '--no-cmake-consumer',
            '--no-examples',
        ]
    )

    assert args.render is False
    assert args.cmake_consumer is False
    assert args.examples is False
    assert args.shaderc is True


def test_aggregate_builds_self_contained_report(tmp_path: Path) -> None:
    """Aggregation preserves captures and writes HTML, JSON, and a manifest."""
    inputs = tmp_path / 'inputs'
    _evidence(inputs, 'linux-ci')
    _evidence(inputs, 'macbook-m3')
    output = tmp_path / 'report'
    args = Namespace(
        input=[inputs],
        output_dir=output,
        replace=False,
        strict=True,
        expected_machine=['linux-ci', 'macbook-m3'],
    )

    assert conformance.aggregate(args) == 0

    report = json.loads((output / 'report.json').read_text(encoding='utf8'))
    assert report['status'] == 'pass'
    assert len(report['evidence']) == 2
    assert (output / 'index.html').is_file()
    assert (output / 'manifest.json').is_file()
    assert (output / 'platforms/linux-ci/captures/point.png').is_file()
    report_html = (output / 'index.html').read_text(encoding='utf8')
    assert 'Full environment and Vulkan metadata' in report_html
    assert 'Capture statistics' in report_html
    assert 'Release gates' in report_html
    assert report['gates']['cross_frontend_render_parity'] == 'pass'
    assert 'Cross-frontend capture parity' in report_html


def test_aggregate_writes_report_before_strict_failure(tmp_path: Path) -> None:
    """A failing platform still produces the diagnostic report."""
    inputs = tmp_path / 'inputs'
    _evidence(inputs, 'windows-ci', status='fail')
    output = tmp_path / 'report'
    args = Namespace(
        input=[inputs],
        output_dir=output,
        replace=False,
        strict=True,
        expected_machine=['windows-ci'],
    )

    assert conformance.aggregate(args) == 1
    assert (output / 'index.html').is_file()
    assert json.loads((output / 'report.json').read_text())['status'] == 'fail'


def test_aggregate_rejects_cross_frontend_capture_mismatch(tmp_path: Path) -> None:
    """A frontend parity mismatch fails the report even when each repeat is deterministic."""
    evidence_dir = _evidence(tmp_path / 'inputs', 'macos-ci')
    evidence_path = evidence_dir / 'evidence.json'
    evidence = json.loads(evidence_path.read_text(encoding='utf8'))
    evidence['capture_comparisons'][0]['status'] = 'fail'
    evidence_path.write_text(json.dumps(evidence), encoding='utf8')
    output = tmp_path / 'report'
    args = Namespace(
        input=[evidence_dir],
        output_dir=output,
        replace=False,
        strict=True,
        expected_machine=['macos-ci'],
    )

    assert conformance.aggregate(args) == 1
    report = json.loads((output / 'report.json').read_text())
    assert report['gates']['cross_frontend_render_parity'] == 'fail'


def test_aggregate_rejects_missing_expected_machine(tmp_path: Path) -> None:
    """Missing matrix evidence is a report failure rather than an implicit pass."""
    inputs = tmp_path / 'inputs'
    _evidence(inputs, 'linux-ci')
    output = tmp_path / 'report'
    args = Namespace(
        input=[inputs],
        output_dir=output,
        replace=False,
        strict=True,
        expected_machine=['linux-ci', 'windows-ci'],
    )

    assert conformance.aggregate(args) == 1
    report = json.loads((output / 'report.json').read_text())
    assert report['missing_machines'] == ['windows-ci']


def test_aggregate_keeps_pending_physical_human_gate_non_green(tmp_path: Path) -> None:
    """Passing unattended work cannot promote pending human observations."""
    inputs = tmp_path / 'inputs'
    evidence_dir = _evidence(inputs, 'macbook-m3', physical=True)
    evidence_path = evidence_dir / 'evidence.json'
    evidence = json.loads(evidence_path.read_text(encoding='utf8'))
    evidence['manual']['state'] = 'pending'
    evidence_path.write_text(json.dumps(evidence), encoding='utf8')
    (evidence_dir / 'manual-observations.json').write_text(
        json.dumps(evidence['manual']), encoding='utf8'
    )
    output = tmp_path / 'report'
    args = Namespace(
        input=[inputs],
        output_dir=output,
        replace=False,
        strict=True,
        expected_machine=['macbook-m3'],
    )

    assert conformance.aggregate(args) == 1
    report = json.loads((output / 'report.json').read_text())
    assert report['status'] == 'pending'
    assert report['gates']['physical_unattended'] == 'pass'
    assert report['gates']['physical_human_interaction'] == 'pending'


def test_verify_bundle_accepts_approved_physical_evidence(tmp_path: Path) -> None:
    """Intake accepts one approved bundle with matching immutable identity."""
    evidence_dir = _evidence(tmp_path / 'source', 'macbook-m3', physical=True)
    archive = tmp_path / 'evidence.tar.gz'
    with tarfile.open(archive, 'w:gz') as tar:
        tar.add(evidence_dir, arcname='macbook-m3')
    output = tmp_path / 'accepted'
    args = Namespace(
        archive=archive,
        output_dir=output,
        wheel_run_id='123',
        machine_id='macbook-m3',
        version='0.4.0rc1',
        replace=False,
    )

    assert conformance.verify_bundle(args) == 0
    assert (output / 'macbook-m3/evidence.json').is_file()


def test_verify_bundle_rejects_tampered_capture(tmp_path: Path) -> None:
    """Intake verifies every recorded image checksum after extraction."""
    evidence_dir = _evidence(tmp_path / 'source', 'macbook-m3', physical=True)
    _png(evidence_dir / 'captures/point.png', (200, 40, 60, 255))
    archive = tmp_path / 'evidence.tar.gz'
    with tarfile.open(archive, 'w:gz') as tar:
        tar.add(evidence_dir, arcname='macbook-m3')
    args = Namespace(
        archive=archive,
        output_dir=tmp_path / 'accepted',
        wheel_run_id='123',
        machine_id='macbook-m3',
        version='0.4.0rc1',
        replace=False,
    )

    try:
        conformance.verify_bundle(args)
    except ValueError as exc:
        assert 'capture checksum mismatch' in str(exc)
    else:
        raise AssertionError('tampered capture was accepted')


def test_safe_extract_rejects_path_traversal(tmp_path: Path) -> None:
    """Evidence archive extraction cannot write outside its intake directory."""
    payload = tmp_path / 'payload'
    payload.write_text('unsafe', encoding='utf8')
    archive = tmp_path / 'unsafe.tar.gz'
    with tarfile.open(archive, 'w:gz') as tar:
        tar.add(payload, arcname='../escaped')

    try:
        conformance._safe_extract(archive, tmp_path / 'accepted')
    except ValueError as exc:
        assert 'unsafe evidence archive path' in str(exc)
    else:
        raise AssertionError('path traversal archive was accepted')
