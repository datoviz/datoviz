"""Tests for package-index wheel verification."""

from pathlib import Path

from tools.release_wheels import index_verify


def _evidence(machine_id: str, *, status: str = 'pass') -> dict:
    return {
        'schema': index_verify.EVIDENCE_SCHEMA,
        'status': status,
        'index': 'testpypi',
        'version': '0.4.0rc1',
        'wheel_run_id': '123',
        'artifact_commit': 'artifact',
        'machine_id': machine_id,
        'artifact_name': f'wheel-{machine_id}',
        'canonical_wheel': {'name': f'datoviz-{machine_id}.whl', 'sha256': 'abc'},
        'index_wheel': {'name': f'datoviz-{machine_id}.whl', 'sha256': 'abc'},
        'identity': {'status': 'pass'},
        'installed_smoke': {'status': status},
        'failures': [] if status == 'pass' else ['smoke failed'],
    }


def test_identity_failures_accepts_byte_identical_wheel(tmp_path: Path) -> None:
    """Index and canonical wheels must have the same name and bytes."""
    canonical = tmp_path / 'canonical' / 'datoviz-0.4.0rc1-test.whl'
    indexed = tmp_path / 'indexed' / canonical.name
    canonical.parent.mkdir()
    indexed.parent.mkdir()
    canonical.write_bytes(b'same wheel')
    indexed.write_bytes(b'same wheel')

    assert index_verify.identity_failures(canonical, indexed) == []


def test_identity_failures_rejects_changed_wheel(tmp_path: Path) -> None:
    """An index-side wheel mutation invalidates transferred conformance evidence."""
    canonical = tmp_path / 'canonical.whl'
    indexed = tmp_path / 'indexed.whl'
    canonical.write_bytes(b'canonical')
    indexed.write_bytes(b'changed')

    failures = index_verify.identity_failures(canonical, indexed)

    assert any('filename differs' in failure for failure in failures)
    assert any('SHA-256 differs' in failure for failure in failures)


def test_report_requires_every_expected_machine() -> None:
    """A partial native-platform matrix cannot produce a green report."""
    report = index_verify.report_payload(
        [_evidence('index-linux-x86_64')],
        ['index-linux-x86_64', 'index-macos-arm64'],
    )

    assert report['status'] == 'fail'
    assert report['missing_machines'] == ['index-macos-arm64']


def test_report_passes_one_consistent_complete_campaign() -> None:
    """Consistent passing evidence produces a green consolidated report."""
    machines = ['index-linux-x86_64', 'index-macos-arm64']
    report = index_verify.report_payload([_evidence(machine) for machine in machines], machines)

    assert report['status'] == 'pass'
    assert report['index'] == 'testpypi'
    assert report['wheel_run_id'] == '123'
