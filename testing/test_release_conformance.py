"""Tests for installed-wheel release conformance evidence."""

from __future__ import annotations

import json
import struct
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


def _evidence(root: Path, machine_id: str, status: str = 'pass') -> Path:
    evidence_dir = root / machine_id
    captures = evidence_dir / 'captures'
    captures.mkdir(parents=True)
    _png(captures / 'point.png', (20, 40, 60, 255))
    environment = {
        'platform': {'system': 'TestOS', 'machine': 'test64'},
        'execution_class': 'github-hosted-software-gpu',
    }
    (evidence_dir / 'environment.json').write_text(json.dumps(environment), encoding='utf8')
    evidence = {
        'schema': conformance.EVIDENCE_SCHEMA,
        'version': '0.4.0rc1',
        'campaign': {
            'wheel_run_id': '123',
            'artifact_commit': 'artifact',
            'validator_commit': 'validator',
        },
        'machine_id': machine_id,
        'execution_class': 'github-hosted-software-gpu',
        'mode': 'unattended',
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
        'manual': {'state': 'not-applicable', 'scenarios': []},
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


def test_aggregate_builds_self_contained_report(tmp_path: Path) -> None:
    """Aggregation preserves captures and writes HTML, JSON, and a manifest."""
    inputs = tmp_path / 'inputs'
    _evidence(inputs, 'linux-ci')
    _evidence(inputs, 'macbook-m3')
    output = tmp_path / 'report'
    args = Namespace(input=[inputs], output_dir=output, replace=False, strict=True)

    assert conformance.aggregate(args) == 0

    report = json.loads((output / 'report.json').read_text(encoding='utf8'))
    assert report['status'] == 'pass'
    assert len(report['evidence']) == 2
    assert (output / 'index.html').is_file()
    assert (output / 'manifest.json').is_file()
    assert (output / 'platforms/linux-ci/captures/point.png').is_file()


def test_aggregate_writes_report_before_strict_failure(tmp_path: Path) -> None:
    """A failing platform still produces the diagnostic report."""
    inputs = tmp_path / 'inputs'
    _evidence(inputs, 'windows-ci', status='fail')
    output = tmp_path / 'report'
    args = Namespace(input=[inputs], output_dir=output, replace=False, strict=True)

    assert conformance.aggregate(args) == 1
    assert (output / 'index.html').is_file()
    assert json.loads((output / 'report.json').read_text())['status'] == 'fail'
