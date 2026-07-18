#!/usr/bin/env python3
"""Run installed-wheel conformance and build consolidated release evidence reports."""

from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import html
import json
import os
import platform
import re
import shutil
import socket
import struct
import subprocess
import sys
import tarfile
import zlib
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
EVIDENCE_SCHEMA = 'datoviz.release-conformance.v1'
REPORT_SCHEMA = 'datoviz.release-conformance-report.v1'
MANUAL_SCENARIOS = (
    'scatter-panzoom',
    'arcball-3d',
    'text-resize',
    'image-probe',
    'textured-mesh',
    'picking',
    'close-reopen',
)
WHEEL_ARTIFACTS = {
    ('Darwin', 'arm64'): 'wheel-macos-arm64',
    ('Darwin', 'x86_64'): 'wheel-macos-x86_64',
    ('Linux', 'x86_64'): 'wheel-linux-x86_64',
    ('Linux', 'aarch64'): 'wheel-linux-aarch64',
    ('Windows', 'AMD64'): 'wheel-windows-AMD64',
    ('Windows', 'ARM64'): 'wheel-windows-ARM64',
}


def utc_now() -> str:
    """Return the current UTC timestamp in the release evidence format."""
    return (
        dt.datetime.now(dt.timezone.utc).replace(microsecond=0).isoformat().replace('+00:00', 'Z')
    )


def sha256(path: Path) -> str:
    """Return the SHA-256 digest of a file."""
    digest = hashlib.sha256()
    with path.open('rb') as file:
        for chunk in iter(lambda: file.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def safe_name(value: str) -> str:
    """Convert an external identifier to a safe path component."""
    return re.sub(r'[^A-Za-z0-9_.-]+', '-', value).strip('-.') or 'unknown'


def validate_wheels_run(run: dict[str, Any], run_id: str) -> None:
    """Require metadata for the successful canonical wheel workflow."""
    identity = (run.get('path'), run.get('status'), run.get('conclusion'))
    expected = ('.github/workflows/wheels.yml', 'completed', 'success')
    if identity != expected:
        raise ValueError(f'run {run_id} is not a successful completed Wheels run')


def command_output(argv: list[str], *, timeout: int = 20) -> dict[str, Any]:
    """Run a metadata probe and return a structured result."""
    try:
        result = subprocess.run(  # noqa: S603
            argv, text=True, capture_output=True, check=False, timeout=timeout
        )
    except FileNotFoundError:
        return {'argv': argv, 'status': 'missing'}
    except subprocess.TimeoutExpired as exc:
        return {
            'argv': argv,
            'status': 'timeout',
            'stdout': exc.stdout or '',
            'stderr': exc.stderr or '',
        }
    return {
        'argv': argv,
        'status': 'pass' if result.returncode == 0 else 'fail',
        'returncode': result.returncode,
        'stdout': result.stdout.strip(),
        'stderr': result.stderr.strip(),
    }


def environment_metadata(args: argparse.Namespace, wheel: Path) -> dict[str, Any]:
    """Collect environment and immutable wheel identity metadata."""
    vulkaninfo = shutil.which('vulkaninfo')
    return {
        'schema': 'datoviz.release-conformance-environment.v1',
        'created_at_utc': utc_now(),
        'machine_id': args.machine_id,
        'execution_class': args.execution_class,
        'human_observation': args.mode == 'physical',
        'hostname': socket.gethostname(),
        'platform': {
            'system': platform.system(),
            'release': platform.release(),
            'version': platform.version(),
            'machine': platform.machine(),
            'processor': platform.processor(),
        },
        'python': {
            'version': platform.python_version(),
            'implementation': platform.python_implementation(),
            'executable': sys.executable,
        },
        'wheel': {'name': wheel.name, 'bytes': wheel.stat().st_size, 'sha256': sha256(wheel)},
        'vulkaninfo': command_output([vulkaninfo, '--summary'], timeout=30)
        if vulkaninfo
        else {'status': 'missing'},
        'tool_versions': {
            'cmake': command_output(['cmake', '--version']),
            'ninja': command_output(['ninja', '--version']),
        },
        'discovery_environment': {
            key: os.environ.get(key, '')
            for key in (
                'VULKAN_SDK',
                'VK_DRIVER_FILES',
                'VK_ICD_FILENAMES',
                'VK_LAYER_PATH',
                'DYLD_LIBRARY_PATH',
                'DYLD_FALLBACK_LIBRARY_PATH',
                'LD_LIBRARY_PATH',
            )
        },
    }


def _paeth(left: int, above: int, upper_left: int) -> int:
    estimate = left + above - upper_left
    left_distance = abs(estimate - left)
    above_distance = abs(estimate - above)
    upper_left_distance = abs(estimate - upper_left)
    if left_distance <= above_distance and left_distance <= upper_left_distance:
        return left
    if above_distance <= upper_left_distance:
        return above
    return upper_left


def png_stats(path: Path) -> dict[str, Any]:
    """Return deterministic structural and channel statistics for an 8-bit PNG."""
    data = path.read_bytes()
    if not data.startswith(b'\x89PNG\r\n\x1a\n'):
        raise ValueError(f'not a PNG: {path}')
    offset = 8
    width = height = color_type = bit_depth = interlace = 0
    compressed = bytearray()
    while offset < len(data):
        length = struct.unpack('>I', data[offset : offset + 4])[0]
        kind = data[offset + 4 : offset + 8]
        payload = data[offset + 8 : offset + 8 + length]
        offset += 12 + length
        if kind == b'IHDR':
            width, height, bit_depth, color_type, _, _, interlace = struct.unpack(
                '>IIBBBBB', payload
            )
        elif kind == b'IDAT':
            compressed.extend(payload)
        elif kind == b'IEND':
            break
    channels = {0: 1, 2: 3, 4: 2, 6: 4}.get(color_type)
    if bit_depth != 8 or channels is None or interlace != 0:
        return {
            'width': width,
            'height': height,
            'bit_depth': bit_depth,
            'color_type': color_type,
            'analysis': 'unsupported-pixel-layout',
        }
    raw = zlib.decompress(compressed)
    stride = width * channels
    previous = bytearray(stride)
    pixels = bytearray()
    cursor = 0
    for _ in range(height):
        filter_type = raw[cursor]
        cursor += 1
        scanline = bytearray(raw[cursor : cursor + stride])
        cursor += stride
        for index in range(stride):
            left = scanline[index - channels] if index >= channels else 0
            above = previous[index]
            upper_left = previous[index - channels] if index >= channels else 0
            if filter_type == 1:
                scanline[index] = (scanline[index] + left) & 0xFF
            elif filter_type == 2:
                scanline[index] = (scanline[index] + above) & 0xFF
            elif filter_type == 3:
                scanline[index] = (scanline[index] + ((left + above) // 2)) & 0xFF
            elif filter_type == 4:
                scanline[index] = (scanline[index] + _paeth(left, above, upper_left)) & 0xFF
            elif filter_type != 0:
                raise ValueError(f'unsupported PNG filter {filter_type}: {path}')
        pixels.extend(scanline)
        previous = scanline
    count = width * height
    channel_values = [pixels[index::channels] for index in range(channels)]
    alpha = channel_values[-1] if color_type in {4, 6} else bytes([255]) * count
    return {
        'width': width,
        'height': height,
        'bit_depth': bit_depth,
        'color_type': color_type,
        'channels': channels,
        'channel_min': [min(values) for values in channel_values],
        'channel_max': [max(values) for values in channel_values],
        'channel_mean': [round(sum(values) / count, 3) for values in channel_values],
        'nontransparent_fraction': round(sum(value > 0 for value in alpha) / count, 6),
    }


def capture_records(work_dirs: list[Path], capture_dir: Path) -> list[dict[str, Any]]:
    """Copy captures from repeated runs and compare their fingerprints."""
    by_repeat: list[dict[str, Path]] = []
    for work in work_dirs:
        by_repeat.append(
            {
                path.relative_to(work).as_posix(): path
                for path in sorted(work.rglob('*.png'))
                if 'site-packages' not in path.parts
            }
        )
    names = sorted(set().union(*(set(paths) for paths in by_repeat)))
    records = []
    capture_dir.mkdir(parents=True, exist_ok=True)
    for name in names:
        paths = [repeat.get(name) for repeat in by_repeat]
        digests = [sha256(path) if path else None for path in paths]
        source = next((path for path in paths if path is not None), None)
        if source is None:
            continue
        destination = capture_dir / f'{safe_name(name)}.png'
        shutil.copy2(source, destination)
        records.append(
            {
                'scenario': name,
                'path': f'captures/{destination.name}',
                'bytes': destination.stat().st_size,
                'sha256': sha256(destination),
                'repeat_sha256': digests,
                'deterministic': len(paths) > 1 and None not in digests and len(set(digests)) == 1,
                'stats': png_stats(destination),
            }
        )
    return records


def run_conformance(args: argparse.Namespace) -> int:
    """Run the installed-wheel profile and write a conformance evidence bundle."""
    wheel = args.wheel.resolve()
    output = args.output_dir.resolve()
    if not wheel.is_file():
        raise FileNotFoundError(wheel)
    if output.exists():
        if not args.replace:
            raise FileExistsError(f'output already exists: {output}; pass --replace')
        shutil.rmtree(output)
    output.mkdir(parents=True)
    log_dir = output / 'logs'
    log_dir.mkdir()
    work_dirs = []
    results = []
    failures = []
    started = utc_now()
    environment = environment_metadata(args, wheel)
    (output / 'environment.json').write_text(
        json.dumps(environment, indent=2, sort_keys=True) + '\n', encoding='utf8'
    )

    repeat_count = max(1, args.repeat if args.render else 1)
    for repeat in range(1, repeat_count + 1):
        work = output / f'work-{repeat}'
        work_dirs.append(work)
        argv = [
            sys.executable,
            os.fspath(ROOT / 'tools/release_wheels/check_wheel.py'),
            '--wheel',
            os.fspath(wheel),
            '--work-dir',
            os.fspath(work),
            '--release-build',
            '--precompiled-shaders',
            '--examples',
            'render' if args.render else 'basic',
            '--qt-probe',
            'optional',
        ]
        if args.shaderc:
            argv.append('--shaderc')
        if args.cmake_consumer:
            argv.append('--cmake-consumer')
        if args.render:
            argv.append('--render')
        result = subprocess.run(  # noqa: S603
            argv, cwd=ROOT, text=True, capture_output=True, check=False
        )
        log_path = log_dir / f'unattended-{repeat}.log'
        log_path.write_text(
            'COMMAND: '
            + ' '.join(argv)
            + '\n\nSTDOUT:\n'
            + result.stdout
            + '\nSTDERR:\n'
            + result.stderr,
            encoding='utf8',
        )
        record = {
            'id': f'installed-wheel-{repeat}',
            'argv': argv,
            'status': 'pass' if result.returncode == 0 else 'fail',
            'returncode': result.returncode,
            'log': f'logs/{log_path.name}',
        }
        results.append(record)
        if result.returncode != 0:
            failures.append(record)
            if not args.keep_going:
                break

    captures = capture_records(work_dirs, output / 'captures') if args.render else []
    nondeterministic = [capture for capture in captures if not capture['deterministic']]
    if args.render and not captures:
        failures.append({'id': 'captures', 'status': 'fail', 'message': 'no PNG captures found'})
    for capture in nondeterministic:
        failures.append(
            {
                'id': 'determinism',
                'status': 'fail',
                'message': f'repeat capture differs: {capture["scenario"]}',
            }
        )

    manual_state = 'pending' if args.mode == 'physical' else 'not-applicable'
    manual = {
        'state': manual_state,
        'approved_by': '',
        'approved_at_utc': '',
        'scenarios': [
            {'id': scenario, 'status': 'pending', 'observation': ''}
            for scenario in MANUAL_SCENARIOS
        ],
    }
    (output / 'manual-observations.json').write_text(
        json.dumps(manual, indent=2, sort_keys=True) + '\n', encoding='utf8'
    )
    skips = []
    if not args.render:
        skips.append({'id': 'render', 'reason': args.render_skip_reason})
    if not args.cmake_consumer:
        skips.append({'id': 'cmake-consumer', 'reason': args.cmake_skip_reason})
    if not args.shaderc:
        skips.append({'id': 'shaderc', 'reason': args.shaderc_skip_reason})
    evidence = {
        'schema': EVIDENCE_SCHEMA,
        'version': args.version,
        'campaign': {
            'wheel_run_id': str(args.wheel_run_id),
            'artifact_commit': args.artifact_commit,
            'validator_commit': args.validator_commit,
        },
        'machine_id': args.machine_id,
        'execution_class': args.execution_class,
        'mode': args.mode,
        'started_at_utc': started,
        'finished_at_utc': utc_now(),
        'artifact_checksums': {'wheel': environment['wheel']},
        'environment': 'environment.json',
        'results': results,
        'captures': captures,
        'manual': manual,
        'skips': skips,
        'failures': failures,
        'status': 'fail' if failures else 'pass',
    }
    (output / 'evidence.json').write_text(
        json.dumps(evidence, indent=2, sort_keys=True) + '\n', encoding='utf8'
    )
    (output / 'failures.md').write_text(
        '# Conformance failures\n\n'
        + (
            '\n'.join(f'- {item.get("message", item.get("id"))}' for item in failures)
            or 'No failures recorded.'
        )
        + '\n',
        encoding='utf8',
    )
    for work in work_dirs:
        shutil.rmtree(work, ignore_errors=True)
    print(output / 'evidence.json')
    return 1 if failures else 0


def approve_physical(args: argparse.Namespace) -> int:
    """Record the maintainer's explicit physical interaction decision."""
    evidence_dir = args.evidence_dir.resolve()
    evidence_path = evidence_dir / 'evidence.json'
    manual_path = evidence_dir / 'manual-observations.json'
    evidence = json.loads(evidence_path.read_text(encoding='utf8'))
    manual = json.loads(manual_path.read_text(encoding='utf8'))
    supplied: dict[str, tuple[str, str]] = {}
    for value in args.result:
        scenario, separator, remainder = value.partition('=')
        if not separator:
            raise ValueError(f'manual result must be ID=STATUS[:NOTE]: {value}')
        status, _, note = remainder.partition(':')
        if status not in {'pass', 'fail', 'skip'}:
            raise ValueError(f'invalid manual status {status!r}: {value}')
        supplied[scenario] = (status, note)
    known = {item['id'] for item in manual['scenarios']}
    unknown = set(supplied) - known
    if unknown:
        raise ValueError(f'unknown manual scenarios: {sorted(unknown)}')
    for item in manual['scenarios']:
        if item['id'] in supplied:
            item['status'], item['observation'] = supplied[item['id']]
    pending = [item['id'] for item in manual['scenarios'] if item['status'] == 'pending']
    if pending:
        raise RuntimeError(f'manual scenarios still pending: {pending}')
    rejected = [item for item in manual['scenarios'] if item['status'] == 'fail']
    manual['state'] = 'rejected' if rejected else 'approved'
    manual['approved_by'] = args.approved_by
    manual['approved_at_utc'] = utc_now()
    manual_path.write_text(json.dumps(manual, indent=2, sort_keys=True) + '\n', encoding='utf8')
    evidence['manual'] = manual
    if rejected:
        evidence['status'] = 'fail'
        evidence['failures'].append({'id': 'manual', 'message': 'manual validation rejected'})
    evidence_path.write_text(
        json.dumps(evidence, indent=2, sort_keys=True) + '\n', encoding='utf8'
    )
    print(f'manual validation {manual["state"]}: {evidence_path}')
    return 1 if rejected else 0


def discover_evidence(inputs: list[Path]) -> list[tuple[Path, dict[str, Any]]]:
    """Find and validate conformance evidence records beneath input roots."""
    records = []
    for input_path in inputs:
        root = input_path.resolve()
        paths = [root] if root.name == 'evidence.json' else sorted(root.rglob('evidence.json'))
        for path in paths:
            record = json.loads(path.read_text(encoding='utf8'))
            if record.get('schema') != EVIDENCE_SCHEMA:
                raise ValueError(f'unsupported evidence schema in {path}: {record.get("schema")}')
            records.append((path, record))
    if not records:
        raise FileNotFoundError('no conformance evidence found')
    return records


def report_gates(evidence: list[dict[str, Any]], missing: list[str]) -> dict[str, str]:
    """Calculate separate hosted, rendering, physical, coverage, and integrity gates."""
    hosted = [
        item for item in evidence if str(item.get('execution_class', '')).startswith('github-')
    ]
    hosted_render = [
        item for item in hosted if item.get('execution_class') != 'github-hosted-no-gpu'
    ]
    physical = [item for item in evidence if item.get('mode') == 'physical']

    def automated(items: list[dict[str, Any]]) -> str:
        if not items:
            return 'not-applicable'
        return 'pass' if all(item.get('status') == 'pass' for item in items) else 'fail'

    if not hosted_render:
        rendering = 'not-applicable'
    else:
        rendering = (
            'pass'
            if all(item.get('status') == 'pass' and item.get('captures') for item in hosted_render)
            else 'fail'
        )
    if not physical:
        human = 'not-applicable'
    elif any(item.get('manual', {}).get('state') == 'rejected' for item in physical):
        human = 'fail'
    elif all(item.get('manual', {}).get('state') == 'approved' for item in physical):
        human = 'pass'
    else:
        human = 'pending'
    return {
        'hosted_artifact_conformance': automated(hosted),
        'hosted_rendering': rendering,
        'physical_unattended': automated(physical),
        'physical_human_interaction': human,
        'required_machine_coverage': 'fail' if missing else 'pass',
        'evidence_integrity': 'pass',
    }


def _report_html(report: dict[str, Any]) -> str:
    rows = []
    sections = []
    for item in report['evidence']:
        manual = item.get('manual', {})
        rows.append(
            '<tr>'
            f'<td>{html.escape(item["machine_id"])}</td>'
            f'<td>{html.escape(item["execution_class"])}</td>'
            f"<td class='{item['status']}'>{html.escape(item['status'].upper())}</td>"
            f'<td>{html.escape(str(manual.get("state", "not-applicable")))}</td>'
            f'<td>{len(item.get("captures", []))}</td>'
            '</tr>'
        )
        capture_html = []
        for capture in item.get('captures', []):
            image_path = f'platforms/{safe_name(item["machine_id"])}/{capture["path"]}'
            stats = capture.get('stats', {})
            capture_html.append(
                '<figure>'
                f"<img src='{html.escape(image_path)}' alt='{html.escape(capture['scenario'])}'>"
                f'<figcaption><strong>{html.escape(capture["scenario"])}</strong><br>'
                f'{stats.get("width", "?")}×{stats.get("height", "?")} · '
                f'sha256 {capture.get("sha256", "")[:16]}… · '
                f'repeat deterministic={capture.get("deterministic")}</figcaption></figure>'
            )
        environment = item.get('environment_data', {})
        platform_data = environment.get('platform', {})
        wheel = (item.get('artifact_checksums') or {}).get('wheel') or {}
        sections.append(
            f'<section><h2>{html.escape(item["machine_id"])}</h2>'
            f'<p><b>Environment:</b> {html.escape(item["execution_class"])}; '
            f'{html.escape(str(platform_data.get("system", "")))} '
            f'{html.escape(str(platform_data.get("machine", "")))}</p>'
            f'<p><b>Wheel:</b> {html.escape(str(wheel.get("name", "")))} · '
            f'sha256 {html.escape(str(wheel.get("sha256", "")))}</p>'
            f'<p><b>Automated:</b> {html.escape(item["status"])}; '
            f'<b>human:</b> {html.escape(str(manual.get("state", "not-applicable")))}</p>'
            "<div class='captures'>"
            f'{"".join(capture_html) or "<p>No captures.</p>"}</div></section>'
        )
    campaign = report['campaign']
    missing = report.get('missing_machines', [])
    missing_html = (
        '<p class="fail"><b>Missing evidence:</b> ' + html.escape(', '.join(missing)) + '</p>'
        if missing
        else ''
    )
    gate_rows = ''.join(
        '<tr>'
        f'<td>{html.escape(name.replace("_", " "))}</td>'
        f"<td class='{status}'>{html.escape(status.upper())}</td>"
        '</tr>'
        for name, status in report['gates'].items()
    )
    return f"""<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width">
<title>Datoviz wheel conformance {html.escape(str(campaign.get('wheel_run_id', '')))}</title>
<style>
body{{font:15px system-ui,sans-serif;max-width:1400px;margin:2rem auto;padding:0 1rem;
color:#20242b}}
table{{border-collapse:collapse;width:100%}}
th,td{{border:1px solid #ccd2da;padding:.5rem;text-align:left}}
.pass{{color:#087830;font-weight:700}}.fail{{color:#b00020;font-weight:700}}
.pending{{color:#9a6500;font-weight:700}}
.captures{{display:grid;grid-template-columns:repeat(auto-fit,minmax(260px,1fr));gap:1rem}}
figure{{margin:0;border:1px solid #ccd2da;padding:.75rem;background:#f7f8fa}}
img{{width:100%;image-rendering:auto}}
code{{word-break:break-all}}section{{margin-top:2.5rem}}.meta{{background:#f0f3f6;padding:1rem}}
</style></head><body>
<h1>Datoviz wheel conformance</h1>
<div class="meta"><b>Wheel run:</b> {html.escape(str(campaign.get('wheel_run_id', '')))}<br>
<b>Artifact commit:</b> <code>{html.escape(str(campaign.get('artifact_commit', '')))}</code><br>
<b>Validator commit:</b> <code>{html.escape(str(campaign.get('validator_commit', '')))}</code><br>
<b>Generated:</b> {html.escape(report['generated_at_utc'])}</div>
{missing_html}
<h2>Release gates</h2><table><thead><tr><th>Gate</th><th>Status</th></tr></thead>
<tbody>{gate_rows}</tbody></table>
<h2>Machines</h2><table><thead><tr><th>Machine</th><th>Environment</th><th>Automated</th>
<th>Human</th><th>Captures</th></tr></thead><tbody>{''.join(rows)}</tbody></table>
{''.join(sections)}
</body></html>"""


def aggregate(args: argparse.Namespace) -> int:
    """Consolidate machine evidence into one self-contained HTML report."""
    output = args.output_dir.resolve()
    if output.exists():
        if not args.replace:
            raise FileExistsError(f'output already exists: {output}; pass --replace')
        shutil.rmtree(output)
    output.mkdir(parents=True)
    records = discover_evidence(args.input)
    evidence = []
    campaigns = set()
    for path, record in records:
        _verify_evidence_files(path, record)
        machine_id = safe_name(str(record.get('machine_id', 'unknown')))
        destination = output / 'platforms' / machine_id
        shutil.copytree(path.parent, destination)
        environment_path = path.parent / str(record.get('environment', 'environment.json'))
        record['environment_data'] = json.loads(environment_path.read_text(encoding='utf8'))
        evidence.append(record)
        campaign = record.get('campaign', {})
        campaigns.add(
            (
                str(campaign.get('wheel_run_id', '')),
                str(campaign.get('artifact_commit', '')),
                str(campaign.get('validator_commit', '')),
            )
        )
    if len(campaigns) != 1:
        raise ValueError(f'evidence belongs to multiple campaigns: {sorted(campaigns)}')
    wheel_run_id, artifact_commit, validator_commit = campaigns.pop()
    actual_machines = {str(item.get('machine_id', '')) for item in evidence}
    missing_machines = sorted(set(args.expected_machine) - actual_machines)
    gates = report_gates(evidence, missing_machines)
    gate_states = set(gates.values())
    status = 'fail' if 'fail' in gate_states else 'pending' if 'pending' in gate_states else 'pass'
    report = {
        'schema': REPORT_SCHEMA,
        'generated_at_utc': utc_now(),
        'campaign': {
            'wheel_run_id': wheel_run_id,
            'artifact_commit': artifact_commit,
            'validator_commit': validator_commit,
        },
        'status': status,
        'gates': gates,
        'missing_machines': missing_machines,
        'evidence': evidence,
    }
    (output / 'report.json').write_text(
        json.dumps(report, indent=2, sort_keys=True) + '\n', encoding='utf8'
    )
    (output / 'index.html').write_text(_report_html(report), encoding='utf8')
    manifest = {
        'schema': 'datoviz.release-conformance-manifest.v1',
        'generated_at_utc': utc_now(),
        'campaign': report['campaign'],
        'files': [
            {
                'path': path.relative_to(output).as_posix(),
                'bytes': path.stat().st_size,
                'sha256': sha256(path),
            }
            for path in sorted(output.rglob('*'))
            if path.is_file() and path.name != 'manifest.json'
        ],
    }
    (output / 'manifest.json').write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + '\n', encoding='utf8'
    )
    print(output / 'index.html')
    return 1 if args.strict and report['status'] != 'pass' else 0


def validate_manifest(report_dir: Path) -> dict[str, Any]:
    """Validate every file recorded by a consolidated report manifest."""
    manifest_path = report_dir / 'manifest.json'
    manifest = json.loads(manifest_path.read_text(encoding='utf8'))
    for item in manifest.get('files', []):
        path = (report_dir / item['path']).resolve()
        if not path.is_relative_to(report_dir.resolve()) or not path.is_file():
            raise ValueError(f'missing or unsafe report file: {item["path"]}')
        if path.stat().st_size != item['bytes'] or sha256(path) != item['sha256']:
            raise ValueError(f'report file checksum mismatch: {item["path"]}')
    return manifest


def fetch_report(args: argparse.Namespace) -> int:
    """Download, verify, and optionally open a consolidated report artifact."""
    list_command = [
        'gh',
        'run',
        'list',
        '--workflow',
        'wheel-conformance.yml',
        '--status',
        'success',
        '--limit',
        '30',
        '--json',
        'databaseId,displayTitle,headSha,createdAt',
    ]
    result = subprocess.run(  # noqa: S603
        list_command, cwd=ROOT, text=True, capture_output=True, check=False
    )
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or 'unable to query conformance runs')
    runs = json.loads(result.stdout)
    if args.wheel_run_id:
        marker = f'Wheels {args.wheel_run_id}'
        runs = [run for run in runs if marker in str(run.get('displayTitle', ''))]
    if not runs:
        requested = args.wheel_run_id or 'latest'
        raise RuntimeError(f'no successful conformance report found for {requested}')
    selected = runs[0]
    match = re.search(r'Wheels\s+(\d+)', str(selected.get('displayTitle', '')))
    wheel_run_id = args.wheel_run_id or (match.group(1) if match else 'unknown')
    output = (args.output_dir / str(wheel_run_id)).resolve()
    if output.exists() and args.replace:
        shutil.rmtree(output)
    if not output.exists():
        output.mkdir(parents=True)
        download = [
            'gh',
            'run',
            'download',
            str(selected['databaseId']),
            '--name',
            f'wheel-conformance-report-{wheel_run_id}',
            '--dir',
            os.fspath(output),
        ]
        completed = subprocess.run(download, cwd=ROOT, check=False)  # noqa: S603
        if completed.returncode != 0:
            raise RuntimeError(f'unable to download report from run {selected["databaseId"]}')
    manifest = validate_manifest(output)
    campaign = manifest.get('campaign', {})
    if args.wheel_run_id and str(campaign.get('wheel_run_id')) != str(args.wheel_run_id):
        raise ValueError('downloaded report belongs to a different wheel run')
    index = output / 'index.html'
    print(index)
    if args.open:
        if sys.platform == 'darwin':
            subprocess.run(['open', os.fspath(index)], check=False)  # noqa: S603, S607
        elif os.name == 'nt':
            os.startfile(index)  # type: ignore[attr-defined]  # noqa: S606
        elif shutil.which('xdg-open'):
            subprocess.run(['xdg-open', os.fspath(index)], check=False)  # noqa: S603, S607
    return 0


def _safe_extract(archive: Path, output: Path) -> None:
    """Extract a data-only evidence archive without path traversal."""
    root = output.resolve()
    with tarfile.open(archive) as tar:
        members = tar.getmembers()
        if len(members) > 10_000 or sum(member.size for member in members) > 512 * 1024 * 1024:
            raise ValueError('evidence archive exceeds intake limits')
        for member in members:
            if not (member.isfile() or member.isdir()):
                raise ValueError(f'unsupported evidence archive member: {member.name}')
            target = (output / member.name).resolve()
            if not target.is_relative_to(root):
                raise ValueError(f'unsafe evidence archive path: {member.name}')
            if member.isdir():
                target.mkdir(parents=True, exist_ok=True)
                continue
            target.parent.mkdir(parents=True, exist_ok=True)
            source = tar.extractfile(member)
            if source is None:
                raise ValueError(f'unreadable evidence archive member: {member.name}')
            with source, target.open('wb') as destination:
                shutil.copyfileobj(source, destination)


def _verify_evidence_files(evidence_path: Path, evidence: dict[str, Any]) -> None:
    """Verify that evidence references stay inside the bundle and match recorded digests."""
    root = evidence_path.parent.resolve()

    def resolve_file(relative: str) -> Path:
        path = (root / relative).resolve()
        if not path.is_relative_to(root) or not path.is_file():
            raise ValueError(f'invalid evidence file reference: {relative}')
        return path

    environment = resolve_file(str(evidence.get('environment', '')))
    json.loads(environment.read_text(encoding='utf8'))
    manual_path = resolve_file('manual-observations.json')
    manual = json.loads(manual_path.read_text(encoding='utf8'))
    if manual != evidence.get('manual'):
        raise ValueError('manual observation file does not match evidence record')
    for capture in evidence.get('captures', []):
        path = resolve_file(str(capture.get('path', '')))
        if sha256(path) != capture.get('sha256'):
            raise ValueError(f'capture checksum mismatch: {capture.get("path", "")}')


def verify_bundle(args: argparse.Namespace) -> int:
    """Safely extract and validate a submitted physical evidence archive."""
    archive = args.archive.resolve()
    output = args.output_dir.resolve()
    if output.exists():
        if not args.replace:
            raise FileExistsError(f'output already exists: {output}; pass --replace')
        shutil.rmtree(output)
    output.mkdir(parents=True)
    _safe_extract(archive, output)
    records = discover_evidence([output])
    if len(records) != 1:
        raise ValueError(f'evidence archive must contain one record, found {len(records)}')
    evidence_path, evidence = records[0]
    _verify_evidence_files(evidence_path, evidence)
    campaign = evidence.get('campaign', {})
    if str(campaign.get('wheel_run_id')) != str(args.wheel_run_id):
        raise ValueError('evidence wheel run does not match intake request')
    if str(evidence.get('machine_id')) != args.machine_id:
        raise ValueError('evidence machine ID does not match intake request')
    if str(evidence.get('version')) != args.version:
        raise ValueError('evidence version does not match intake request')
    if evidence.get('status') != 'pass':
        raise ValueError('submitted physical evidence is not passing')
    if evidence.get('mode') != 'physical':
        raise ValueError('submitted evidence is not a physical validation record')
    if evidence.get('execution_class') != 'physical-interactive':
        raise ValueError('submitted evidence is not from an interactive physical machine')
    if evidence.get('manual', {}).get('state') != 'approved':
        raise ValueError('physical evidence lacks explicit manual approval')
    print(records[0][0].parent)
    return 0


def submit_physical(args: argparse.Namespace) -> int:
    """Upload approved physical evidence to GHCR and dispatch its intake workflow."""
    if args.confirm != 'yes':
        raise RuntimeError('refusing physical evidence upload without --confirm yes')
    evidence_dir = args.evidence_dir.resolve()
    evidence = json.loads((evidence_dir / 'evidence.json').read_text(encoding='utf8'))
    if evidence.get('status') != 'pass':
        raise ValueError('only passing evidence may be submitted')
    if evidence.get('mode') != 'physical':
        raise ValueError('only physical evidence may be submitted')
    if evidence.get('execution_class') != 'physical-interactive':
        raise ValueError('physical evidence must come from an interactive machine')
    if evidence.get('manual', {}).get('state') != 'approved':
        raise ValueError('physical evidence must have explicit manual approval before submission')
    campaign = evidence.get('campaign', {})
    wheel_run_id = str(campaign.get('wheel_run_id', ''))
    machine_id = safe_name(str(evidence.get('machine_id', 'unknown')))
    archive = evidence_dir.parent / f'evidence-{wheel_run_id}-{machine_id}.tar.gz'
    with tarfile.open(archive, 'w:gz') as tar:
        tar.add(evidence_dir, arcname=machine_id, recursive=True)
    digest = sha256(archive)
    package = args.package.rstrip('/')
    package_ref = f'{package}:wheels-{wheel_run_id}-{machine_id}-{digest[:16]}'
    media_type = 'application/vnd.datoviz.release-evidence.v1+gzip'
    push = [
        'oras',
        'push',
        '--artifact-type',
        'application/vnd.datoviz.release-evidence.v1',
        '--format',
        'json',
        package_ref,
        f'{archive}:{media_type}',
    ]
    result = subprocess.run(  # noqa: S603
        push, cwd=ROOT, text=True, capture_output=True, check=False
    )
    if result.returncode != 0:
        print(result.stderr, file=sys.stderr)
        return result.returncode
    immutable_ref = str(json.loads(result.stdout).get('reference', ''))
    if '@sha256:' not in immutable_ref:
        raise ValueError('ORAS did not return an immutable manifest reference')
    dispatch = [
        'gh',
        'workflow',
        'run',
        'physical-evidence-intake.yml',
        '--ref',
        args.ref,
        '-f',
        f'package_ref={immutable_ref}',
        '-f',
        f'wheel_run_id={wheel_run_id}',
        '-f',
        f'machine_id={machine_id}',
        '-f',
        f'version={evidence.get("version", "")}',
    ]
    result = subprocess.run(dispatch, cwd=ROOT, check=False)  # noqa: S603
    if result.returncode == 0:
        print(f'submitted {machine_id}: {immutable_ref}')
    return result.returncode


def prepare_physical(args: argparse.Namespace) -> int:
    """Download an exact native wheel and run the shared physical validation profile."""
    repository_result = command_output(
        ['gh', 'repo', 'view', '--json', 'nameWithOwner', '--jq', '.nameWithOwner']
    )
    if repository_result.get('status') != 'pass':
        raise RuntimeError('unable to identify the GitHub repository')
    repository = repository_result['stdout']
    run_result = command_output(
        ['gh', 'api', f'repos/{repository}/actions/runs/{args.wheel_run_id}']
    )
    if run_result.get('status') != 'pass':
        raise RuntimeError(f'unable to query Wheels run {args.wheel_run_id}')
    run = json.loads(run_result['stdout'])
    validate_wheels_run(run, str(args.wheel_run_id))
    platform_key = (platform.system(), platform.machine())
    artifact = WHEEL_ARTIFACTS.get(platform_key)
    if not artifact:
        raise ValueError(f'no native wheel artifact mapping for {platform_key!r}')
    machine_id = safe_name(args.machine_id or socket.gethostname())
    output = (
        args.output_dir or ROOT / 'build' / 'release' / args.version / 'conformance' / machine_id
    ).resolve()
    wheelhouse = output.parent / f'.wheel-{args.wheel_run_id}-{artifact}'
    if wheelhouse.exists():
        shutil.rmtree(wheelhouse)
    wheelhouse.mkdir(parents=True)
    download = [
        'gh',
        'run',
        'download',
        str(args.wheel_run_id),
        '--name',
        artifact,
        '--dir',
        os.fspath(wheelhouse),
    ]
    completed = subprocess.run(download, cwd=ROOT, check=False)  # noqa: S603
    if completed.returncode != 0:
        return completed.returncode
    wheels = sorted(wheelhouse.glob('*.whl'))
    if len(wheels) != 1:
        raise ValueError(f'expected one wheel in {artifact}, found {len(wheels)}')
    for key in (
        'VULKAN_SDK',
        'VK_DRIVER_FILES',
        'VK_ICD_FILENAMES',
        'VK_LAYER_PATH',
        'DYLD_LIBRARY_PATH',
        'DYLD_FALLBACK_LIBRARY_PATH',
        'LD_LIBRARY_PATH',
    ):
        os.environ.pop(key, None)
    conformance_args = argparse.Namespace(
        wheel=wheels[0],
        output_dir=output,
        version=args.version,
        wheel_run_id=str(args.wheel_run_id),
        artifact_commit=str(run['head_sha']),
        validator_commit=command_output(['git', 'rev-parse', 'HEAD'])['stdout'],
        machine_id=machine_id,
        execution_class='physical-interactive',
        mode='physical',
        repeat=args.repeat,
        render=True,
        render_skip_reason='',
        shaderc=True,
        shaderc_skip_reason='',
        cmake_consumer=True,
        cmake_skip_reason='',
        keep_going=args.keep_going,
        replace=args.replace,
    )
    result = run_conformance(conformance_args)
    print(f'physical evidence: {output}')
    return result


def sync_physical(args: argparse.Namespace) -> int:
    """Download accepted physical evidence and regenerate the local campaign report."""
    list_command = [
        'gh',
        'run',
        'list',
        '--workflow',
        'physical-evidence-intake.yml',
        '--status',
        'success',
        '--limit',
        '100',
        '--json',
        'databaseId,displayTitle,createdAt',
    ]
    result = subprocess.run(  # noqa: S603
        list_command, cwd=ROOT, text=True, capture_output=True, check=False
    )
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or 'unable to query physical evidence intake')
    marker = f'Wheels {args.wheel_run_id} · '
    selected: dict[str, dict[str, Any]] = {}
    for run in json.loads(result.stdout):
        title = str(run.get('displayTitle', ''))
        if marker not in title:
            continue
        machine_id = title.split(marker, 1)[1].strip()
        selected.setdefault(machine_id, run)
    output = args.output_dir.resolve()
    inputs = output / 'inputs'
    inputs.mkdir(parents=True, exist_ok=True)
    for machine_id, run in selected.items():
        destination = inputs / safe_name(machine_id)
        if destination.exists():
            continue
        destination.mkdir()
        download = [
            'gh',
            'run',
            'download',
            str(run['databaseId']),
            '--name',
            f'physical-evidence-{args.wheel_run_id}-{safe_name(machine_id)}',
            '--dir',
            os.fspath(destination),
        ]
        completed = subprocess.run(download, cwd=ROOT, check=False)  # noqa: S603
        if completed.returncode != 0:
            raise RuntimeError(f'unable to download physical evidence run {run["databaseId"]}')
    aggregate_inputs = [inputs, *args.input]
    report_args = argparse.Namespace(
        input=aggregate_inputs,
        output_dir=output / 'report',
        replace=True,
        strict=False,
        expected_machine=args.expected_machine,
    )
    aggregate(report_args)
    index = output / 'report' / 'index.html'
    print(f'accepted physical machines: {", ".join(sorted(selected)) or "none"}')
    if args.open and sys.platform == 'darwin':
        subprocess.run(['open', os.fspath(index)], check=False)  # noqa: S603, S607
    return 0


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """Parse conformance command-line arguments."""
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest='command', required=True)
    run_parser = subparsers.add_parser('run', help='run installed-wheel conformance')
    run_parser.add_argument('--wheel', type=Path, required=True)
    run_parser.add_argument('--output-dir', type=Path, required=True)
    run_parser.add_argument('--version', required=True)
    run_parser.add_argument('--wheel-run-id', required=True)
    run_parser.add_argument('--artifact-commit', required=True)
    run_parser.add_argument('--validator-commit', default='')
    run_parser.add_argument('--machine-id', required=True)
    run_parser.add_argument(
        '--execution-class',
        choices=(
            'physical-interactive',
            'physical-unattended',
            'self-hosted-remote',
            'github-hosted-hardware-gpu',
            'github-hosted-software-gpu',
            'github-hosted-no-gpu',
        ),
        required=True,
    )
    run_parser.add_argument('--mode', choices=('unattended', 'physical'), default='unattended')
    run_parser.add_argument('--repeat', type=int, default=2)
    run_parser.add_argument('--render', action=argparse.BooleanOptionalAction, default=True)
    run_parser.add_argument('--render-skip-reason', default='graphics capability unavailable')
    run_parser.add_argument('--shaderc', action=argparse.BooleanOptionalAction, default=True)
    run_parser.add_argument('--shaderc-skip-reason', default='shaderc capability unavailable')
    run_parser.add_argument(
        '--cmake-consumer', action=argparse.BooleanOptionalAction, default=True
    )
    run_parser.add_argument('--cmake-skip-reason', default='native CMake toolchain unavailable')
    run_parser.add_argument('--keep-going', action='store_true')
    run_parser.add_argument('--replace', action='store_true')

    approve_parser = subparsers.add_parser('approve', help='record explicit physical observations')
    approve_parser.add_argument('--evidence-dir', type=Path, required=True)
    approve_parser.add_argument('--approved-by', required=True)
    approve_parser.add_argument('--result', action='append', default=[])

    aggregate_parser = subparsers.add_parser('aggregate', help='create one HTML evidence report')
    aggregate_parser.add_argument('--input', type=Path, action='append', required=True)
    aggregate_parser.add_argument('--output-dir', type=Path, required=True)
    aggregate_parser.add_argument('--replace', action='store_true')
    aggregate_parser.add_argument('--strict', action='store_true')
    aggregate_parser.add_argument('--expected-machine', action='append', default=[])

    fetch_parser = subparsers.add_parser('fetch', help='download and open a consolidated report')
    fetch_parser.add_argument('--wheel-run-id', default='')
    fetch_parser.add_argument('--output-dir', type=Path, default=ROOT / 'build' / 'wheel-reports')
    fetch_parser.add_argument('--open', action=argparse.BooleanOptionalAction, default=True)
    fetch_parser.add_argument('--replace', action='store_true')

    verify_parser = subparsers.add_parser(
        'verify-bundle', help='verify a physical evidence upload'
    )
    verify_parser.add_argument('--archive', type=Path, required=True)
    verify_parser.add_argument('--output-dir', type=Path, required=True)
    verify_parser.add_argument('--wheel-run-id', required=True)
    verify_parser.add_argument('--machine-id', required=True)
    verify_parser.add_argument('--version', required=True)
    verify_parser.add_argument('--replace', action='store_true')

    submit_parser = subparsers.add_parser('submit', help='submit approved physical evidence')
    submit_parser.add_argument('--evidence-dir', type=Path, required=True)
    submit_parser.add_argument('--package', default='ghcr.io/datoviz/datoviz-release-evidence')
    submit_parser.add_argument('--ref', default='v0.4-dev')
    submit_parser.add_argument('--confirm', default='no')

    physical_parser = subparsers.add_parser(
        'physical', help='download an exact wheel and run physical validation'
    )
    physical_parser.add_argument('--wheel-run-id', required=True)
    physical_parser.add_argument('--version', required=True)
    physical_parser.add_argument('--machine-id', default='')
    physical_parser.add_argument('--output-dir', type=Path)
    physical_parser.add_argument('--repeat', type=int, default=2)
    physical_parser.add_argument('--keep-going', action='store_true')
    physical_parser.add_argument('--replace', action='store_true')

    sync_parser = subparsers.add_parser('sync', help='sync physical evidence and build a report')
    sync_parser.add_argument('--wheel-run-id', required=True)
    sync_parser.add_argument(
        '--output-dir', type=Path, default=ROOT / 'build' / 'physical-evidence'
    )
    sync_parser.add_argument('--input', type=Path, action='append', default=[])
    sync_parser.add_argument('--expected-machine', action='append', default=[])
    sync_parser.add_argument('--open', action=argparse.BooleanOptionalAction, default=True)
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    """Run the selected conformance command."""
    args = parse_args(argv)
    if args.command == 'run':
        if not args.validator_commit:
            args.validator_commit = command_output(['git', 'rev-parse', 'HEAD'])['stdout']
        return run_conformance(args)
    if args.command == 'approve':
        return approve_physical(args)
    if args.command == 'aggregate':
        return aggregate(args)
    if args.command == 'fetch':
        return fetch_report(args)
    if args.command == 'verify-bundle':
        return verify_bundle(args)
    if args.command == 'submit':
        return submit_physical(args)
    if args.command == 'physical':
        return prepare_physical(args)
    if args.command == 'sync':
        return sync_physical(args)
    raise AssertionError(args.command)


if __name__ == '__main__':
    raise SystemExit(main())
