#!/usr/bin/env python3
"""Verify package-index wheels against canonical GitHub Actions artifacts."""

from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import html
import json
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
EVIDENCE_SCHEMA = 'datoviz.package-index-verification.v1'
REPORT_SCHEMA = 'datoviz.package-index-verification-report.v1'


def utc_now() -> str:
    """Return a stable UTC timestamp."""
    return (
        dt.datetime.now(dt.timezone.utc).replace(microsecond=0).isoformat().replace('+00:00', 'Z')
    )


def sha256(path: Path) -> str:
    """Return a file SHA-256 digest."""
    digest = hashlib.sha256()
    with path.open('rb') as file:
        for chunk in iter(lambda: file.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def one_wheel(directory: Path) -> Path:
    """Require exactly one wheel below a platform artifact directory."""
    wheels = sorted(directory.glob('*.whl'))
    if len(wheels) != 1:
        raise ValueError(f'expected one wheel in {directory}, found {len(wheels)}')
    return wheels[0]


def wheel_record(path: Path) -> dict[str, Any]:
    """Return immutable wheel identity metadata."""
    return {'name': path.name, 'bytes': path.stat().st_size, 'sha256': sha256(path)}


def identity_failures(canonical: Path, indexed: Path) -> list[str]:
    """Compare an index download with its canonical artifact."""
    failures = []
    if canonical.name != indexed.name:
        failures.append(f'filename differs: {canonical.name!r} != {indexed.name!r}')
    if sha256(canonical) != sha256(indexed):
        failures.append('SHA-256 differs from canonical wheel artifact')
    return failures


def verify(args: argparse.Namespace) -> int:
    """Verify one platform wheel and run the lightweight installed-package smoke."""
    canonical = one_wheel(args.canonical_dir.resolve())
    indexed = one_wheel(args.indexed_dir.resolve())
    output = args.output_dir.resolve()
    output.mkdir(parents=True, exist_ok=True)
    failures = identity_failures(canonical, indexed)
    smoke: dict[str, Any] = {'status': 'blocked' if failures else 'pending'}
    log_path = output / 'installed-smoke.log'

    if not failures:
        command = [
            sys.executable,
            os.fspath(ROOT / 'tools' / 'release_wheels' / 'check_wheel.py'),
            '--wheel',
            os.fspath(indexed),
            '--release-build',
        ]
        completed = subprocess.run(  # noqa: S603
            command, cwd=ROOT, text=True, capture_output=True, check=False
        )
        log_path.write_text(
            completed.stdout
            + ('\n' if completed.stdout and completed.stderr else '')
            + completed.stderr,
            encoding='utf8',
        )
        smoke = {
            'status': 'pass' if completed.returncode == 0 else 'fail',
            'returncode': completed.returncode,
            'command': command,
            'log': log_path.name,
        }
        if completed.returncode != 0:
            failures.append(f'installed-package smoke returned {completed.returncode}')

    evidence = {
        'schema': EVIDENCE_SCHEMA,
        'created_at_utc': utc_now(),
        'status': 'fail' if failures else 'pass',
        'index': args.index,
        'index_url': args.index_url,
        'version': args.version,
        'wheel_run_id': str(args.wheel_run_id),
        'artifact_commit': args.artifact_commit,
        'machine_id': args.machine_id,
        'artifact_name': args.artifact_name,
        'platform': {
            'system': platform.system(),
            'machine': platform.machine(),
            'python': platform.python_version(),
        },
        'canonical_wheel': wheel_record(canonical),
        'index_wheel': wheel_record(indexed),
        'identity': {'status': 'fail' if identity_failures(canonical, indexed) else 'pass'},
        'installed_smoke': smoke,
        'failures': failures,
    }
    (output / 'evidence.json').write_text(
        json.dumps(evidence, indent=2, sort_keys=True) + '\n', encoding='utf8'
    )
    print(output / 'evidence.json')
    return 1 if failures else 0


def collect_evidence(inputs: list[Path]) -> list[dict[str, Any]]:
    """Load package-index evidence recursively."""
    evidence = []
    seen = set()
    for input_path in inputs:
        candidates = (
            [input_path] if input_path.is_file() else sorted(input_path.rglob('evidence.json'))
        )
        for path in candidates:
            resolved = path.resolve()
            if resolved in seen:
                continue
            seen.add(resolved)
            item = json.loads(path.read_text(encoding='utf8'))
            if item.get('schema') == EVIDENCE_SCHEMA:
                evidence.append(item)
    return sorted(evidence, key=lambda item: str(item.get('machine_id', '')))


def report_payload(evidence: list[dict[str, Any]], expected: list[str]) -> dict[str, Any]:
    """Build the consolidated package-index verification result."""
    machines = {str(item.get('machine_id', '')) for item in evidence}
    missing = sorted(set(expected) - machines)
    identities = {
        (
            str(item.get('index', '')),
            str(item.get('version', '')),
            str(item.get('wheel_run_id', '')),
            str(item.get('artifact_commit', '')),
        )
        for item in evidence
    }
    failures = [item for item in evidence if item.get('status') != 'pass']
    status = (
        'pass' if evidence and not missing and not failures and len(identities) == 1 else 'fail'
    )
    identity = next(iter(identities)) if len(identities) == 1 else ('', '', '', '')
    return {
        'schema': REPORT_SCHEMA,
        'generated_at_utc': utc_now(),
        'status': status,
        'index': identity[0],
        'version': identity[1],
        'wheel_run_id': identity[2],
        'artifact_commit': identity[3],
        'expected_machines': expected,
        'missing_machines': missing,
        'evidence': evidence,
    }


def render_html(report: dict[str, Any]) -> str:
    """Render a compact standalone verification report."""
    rows = []
    for item in report['evidence']:
        canonical = item.get('canonical_wheel') or {}
        indexed = item.get('index_wheel') or {}
        failures = '; '.join(item.get('failures') or []) or '-'
        rows.append(
            '<tr>'
            f'<td>{html.escape(str(item.get("machine_id", "")))}</td>'
            f'<td>{html.escape(str(item.get("artifact_name", "")))}</td>'
            f'<td>{html.escape(str(indexed.get("name", "")))}</td>'
            f'<td><code>{html.escape(str(canonical.get("sha256", ""))[:16])}…</code></td>'
            f'<td>{html.escape(str(item.get("identity", {}).get("status", "")))}</td>'
            f'<td>{html.escape(str(item.get("installed_smoke", {}).get("status", "")))}</td>'
            f'<td>{html.escape(failures)}</td>'
            '</tr>'
        )
    return f"""<!doctype html>
<html lang="en"><head><meta charset="utf-8"><title>Datoviz package-index verification</title>
<style>
body{{font:14px system-ui,sans-serif;margin:2rem;color:#172033}}h1{{margin-bottom:.25rem}}
.pass{{color:#087443}}.fail{{color:#b42318}}
table{{border-collapse:collapse;width:100%;margin-top:1.5rem}}
th,td{{border:1px solid #d5dae3;padding:.55rem;text-align:left;vertical-align:top}}
th{{background:#f3f5f8}}
code{{font-size:.9em}}
</style></head><body>
<h1>Package-index verification:
<span class="{report['status']}">{report['status'].upper()}</span></h1>
<p>Index: <strong>{html.escape(str(report.get('index', '')))}</strong> · Version:
<code>{html.escape(str(report.get('version', '')))}</code> · Wheels run:
<code>{html.escape(str(report.get('wheel_run_id', '')))}</code></p>
<p>Artifact commit: <code>{html.escape(str(report.get('artifact_commit', '')))}</code></p>
<p>Missing machines: {html.escape(', '.join(report.get('missing_machines', [])) or 'none')}</p>
<table><thead><tr><th>Machine</th><th>Artifact</th><th>Index wheel</th><th>SHA-256</th>
<th>Identity</th><th>Install smoke</th><th>Failures</th></tr></thead><tbody>
{''.join(rows)}
</tbody></table></body></html>"""


def aggregate(args: argparse.Namespace) -> int:
    """Aggregate platform evidence into JSON and HTML."""
    evidence = collect_evidence(args.input)
    report = report_payload(evidence, args.expected_machine)
    output = args.output_dir.resolve()
    output.mkdir(parents=True, exist_ok=True)
    (output / 'report.json').write_text(
        json.dumps(report, indent=2, sort_keys=True) + '\n', encoding='utf8'
    )
    (output / 'index.html').write_text(render_html(report), encoding='utf8')
    print(output / 'index.html')
    return 0 if report['status'] == 'pass' else 1


def fetch(args: argparse.Namespace) -> int:
    """Download and optionally open one completed package-index report artifact."""
    run_id = str(args.workflow_run_id)
    gh = shutil.which('gh')
    if gh is None:
        raise RuntimeError('gh is required to download package-index reports')
    metadata = subprocess.run(  # noqa: S603
        [
            gh,
            'run',
            'view',
            run_id,
            '--json',
            'workflowName,status,conclusion',
        ],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )
    if metadata.returncode != 0:
        raise RuntimeError(metadata.stderr.strip() or f'unable to inspect workflow run {run_id}')
    identity = json.loads(metadata.stdout)
    expected = ('Package index verification', 'completed', 'success')
    actual = (identity.get('workflowName'), identity.get('status'), identity.get('conclusion'))
    if actual != expected:
        raise ValueError(f'run {run_id} is not a successful package-index verification run')

    output = (
        args.output_dir.resolve()
        if args.output_dir
        else ROOT / 'build' / 'package-index-reports' / run_id
    )
    indexes = sorted(output.rglob('index.html')) if output.exists() else []
    if not indexes:
        output.mkdir(parents=True, exist_ok=True)
        completed = subprocess.run(  # noqa: S603
            [
                gh,
                'run',
                'download',
                run_id,
                '--pattern',
                'package-index-report-*',
                '--dir',
                os.fspath(output),
            ],
            cwd=ROOT,
            check=False,
        )
        if completed.returncode != 0:
            raise RuntimeError(f'unable to download package-index report from run {run_id}')
        indexes = sorted(output.rglob('index.html'))
    if len(indexes) != 1:
        raise ValueError(f'expected one package-index report in {output}, found {len(indexes)}')
    print(indexes[0])
    if args.open and sys.platform == 'darwin':
        subprocess.run(['open', os.fspath(indexes[0])], check=False)  # noqa: S603, S607
    return 0


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest='command', required=True)
    verify_parser = subparsers.add_parser('verify', help='verify one native index wheel')
    verify_parser.add_argument('--canonical-dir', type=Path, required=True)
    verify_parser.add_argument('--indexed-dir', type=Path, required=True)
    verify_parser.add_argument('--output-dir', type=Path, required=True)
    verify_parser.add_argument('--index', choices=('testpypi', 'pypi'), required=True)
    verify_parser.add_argument('--index-url', required=True)
    verify_parser.add_argument('--version', required=True)
    verify_parser.add_argument('--wheel-run-id', required=True)
    verify_parser.add_argument('--artifact-commit', required=True)
    verify_parser.add_argument('--machine-id', required=True)
    verify_parser.add_argument('--artifact-name', required=True)
    aggregate_parser = subparsers.add_parser('aggregate', help='build consolidated HTML report')
    aggregate_parser.add_argument('--input', type=Path, action='append', required=True)
    aggregate_parser.add_argument('--output-dir', type=Path, required=True)
    aggregate_parser.add_argument('--expected-machine', action='append', default=[])
    fetch_parser = subparsers.add_parser('fetch', help='download a successful workflow report')
    fetch_parser.add_argument('--workflow-run-id', required=True)
    fetch_parser.add_argument('--output-dir', type=Path)
    fetch_parser.add_argument('--open', action=argparse.BooleanOptionalAction, default=True)
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    """Run package-index verification."""
    args = parse_args(argv)
    if args.command == 'verify':
        return verify(args)
    if args.command == 'aggregate':
        return aggregate(args)
    if args.command == 'fetch':
        return fetch(args)
    raise AssertionError(args.command)


if __name__ == '__main__':
    raise SystemExit(main())
