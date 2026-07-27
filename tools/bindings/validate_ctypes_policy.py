#!/usr/bin/env python3
"""Validate generated ctypes output against binding policy."""

from __future__ import annotations

import argparse
import ast
import json
from pathlib import Path

import generate_ctypes as ctypes_gen


ROOT_DIR = Path(__file__).resolve().parents[2]
DEFAULT_POLICY = ROOT_DIR / 'spec' / 'bindings' / 'ctypes.yml'
DEFAULT_CTYPES = ROOT_DIR / 'datoviz' / '_ctypes.py'
DEFAULT_API = ROOT_DIR / 'build' / 'bindings' / 'datoviz_api.json'


def _policy_list(path: Path, section: str, key: str) -> list[str]:
    out: list[str] = []
    in_section = False
    in_key = False
    for line in path.read_text().splitlines():
        stripped = line.strip()
        if stripped == f'{section}:':
            in_section = True
            in_key = False
            continue
        if in_section and not line.startswith(' ') and stripped:
            break
        if in_section and stripped == f'{key}:':
            in_key = True
            continue
        if in_key and stripped.startswith('- '):
            out.append(stripped[2:].strip())
        elif in_key and stripped and not stripped.startswith('#'):
            break
    return out


def _generated_literal(path: Path, name: str):
    module = ast.parse(path.read_text())
    found = None
    for node in module.body:
        if not isinstance(node, ast.Assign):
            continue
        if len(node.targets) != 1:
            continue
        target = node.targets[0]
        if isinstance(target, ast.Name) and target.id == name:
            found = ast.literal_eval(node.value)
    if found is None:
        raise RuntimeError(f'{name} not found in {path}')
    return found


def _check_same(label: str, expected: list[str], actual: list[str]) -> None:
    expected_set = set(expected)
    actual_set = set(actual)
    extra = sorted(actual_set - expected_set)
    missing = sorted(expected_set - actual_set)
    if extra or missing:
        lines = [f'{label} mismatch']
        if extra:
            lines.append('  extra generated: ' + ', '.join(extra))
        if missing:
            lines.append('  missing generated: ' + ', '.join(missing))
        raise SystemExit('\n'.join(lines))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--policy', type=Path, default=DEFAULT_POLICY)
    parser.add_argument('--ctypes', type=Path, default=DEFAULT_CTYPES)
    parser.add_argument('--api', type=Path, default=DEFAULT_API)
    args = parser.parse_args()

    expected_skipped = _policy_list(args.policy, 'skipped_functions', 'expected')
    actual_skipped = _generated_literal(args.ctypes, '_SKIPPED_FUNCTIONS')
    if not isinstance(actual_skipped, list) or not all(
        isinstance(item, str) for item in actual_skipped
    ):
        raise RuntimeError('_SKIPPED_FUNCTIONS is not a list of strings')
    _check_same('skipped functions', expected_skipped, actual_skipped)

    expected_layout = _policy_list(args.policy, 'layout_records', 'include')
    actual_layout = _generated_literal(args.ctypes, '_DATOVIZ_CTYPES_DECLARED_LAYOUT_RECORDS')
    if not isinstance(actual_layout, list) or not all(
        isinstance(item, str) for item in actual_layout
    ):
        raise RuntimeError('_DATOVIZ_CTYPES_DECLARED_LAYOUT_RECORDS is not a list of strings')
    missing_layout = sorted(set(expected_layout) - set(actual_layout))
    if missing_layout:
        raise SystemExit('missing generated layout records: ' + ', '.join(missing_layout))

    with args.api.open(encoding='utf8') as f:
        api = json.load(f)
    records = {record['name'] for record in api.get('records', []) if record.get('name')}
    enums = {enum['name'] for enum in api.get('enums', []) if enum.get('name')}
    callbacks = set(ctypes_gen._callback_typedefs(api))
    record_policy = ctypes_gen._concrete_record_policy_from_policy(args.policy)
    ordered_records = ctypes_gen._ordered_records(api, records)
    layoutable_records = ctypes_gen._layoutable_records(
        ordered_records,
        records,
        enums,
        callbacks,
        forced_records=ctypes_gen._layout_records_from_policy(args.policy),
        blocked_records=set(record_policy),
    )
    expected_dispositions = ctypes_gen._validate_concrete_record_policy(
        api, layoutable_records, record_policy
    )
    required_alignments = ctypes_gen._required_alignments_from_policy(args.policy)
    for name in required_alignments:
        if expected_dispositions.get(name) == 'layout':
            expected_dispositions[name] = 'conditional-layout'

    actual_policy = _generated_literal(args.ctypes, '_CONCRETE_RECORD_POLICY')
    if actual_policy != record_policy:
        raise SystemExit('generated concrete record policy does not match ctypes.yml')
    actual_dispositions = _generated_literal(args.ctypes, '_CONCRETE_RECORD_DISPOSITIONS')
    if actual_dispositions != expected_dispositions:
        raise SystemExit('generated concrete record disposition audit is stale')

    expected_unsupported = {}
    records_by_name = {
        record['name']: record for record in api.get('records', []) if record.get('name')
    }
    callback_typedefs = ctypes_gen._callback_typedefs(api)
    for function in api.get('functions', []):
        _, diagnostic = ctypes_gen._function_record_dependencies(
            function,
            records_by_name,
            layoutable_records,
            required_alignments,
            record_policy,
            callback_typedefs,
        )
        if diagnostic is not None:
            expected_unsupported[function['name']] = diagnostic
    actual_unsupported = _generated_literal(args.ctypes, '_POLICY_UNSUPPORTED_FUNCTIONS')
    if actual_unsupported != expected_unsupported:
        raise SystemExit('generated unsupported-function diagnostics are stale')

    print(
        'ctypes policy validation: OK '
        f'({len(actual_skipped)} skipped functions, {len(expected_layout)} forced layouts, '
        f'{len(expected_dispositions)} concrete record dispositions, '
        f'{len(actual_unsupported)} unsupported functions)'
    )
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
