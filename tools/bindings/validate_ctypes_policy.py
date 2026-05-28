#!/usr/bin/env python3
"""Validate generated ctypes output against binding policy."""

from __future__ import annotations

import argparse
import ast
from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parents[2]
DEFAULT_POLICY = ROOT_DIR / 'spec' / 'bindings' / 'ctypes.yml'
DEFAULT_CTYPES = ROOT_DIR / 'datoviz' / '_ctypes.py'


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


def _generated_list(path: Path, name: str) -> list[str]:
    module = ast.parse(path.read_text())
    found: list[str] | None = None
    for node in module.body:
        if not isinstance(node, ast.Assign):
            continue
        if len(node.targets) != 1:
            continue
        target = node.targets[0]
        if isinstance(target, ast.Name) and target.id == name:
            value = ast.literal_eval(node.value)
            if isinstance(value, list) and all(isinstance(item, str) for item in value):
                found = value
                continue
            raise RuntimeError(f'{name} is not a list of strings')
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
    args = parser.parse_args()

    expected_skipped = _policy_list(args.policy, 'skipped_functions', 'expected')
    actual_skipped = _generated_list(args.ctypes, '_SKIPPED_FUNCTIONS')
    _check_same('skipped functions', expected_skipped, actual_skipped)

    expected_layout = _policy_list(args.policy, 'layout_records', 'include')
    actual_layout = _generated_list(args.ctypes, '_DATOVIZ_CTYPES_LAYOUT_RECORDS')
    missing_layout = sorted(set(expected_layout) - set(actual_layout))
    if missing_layout:
        raise SystemExit('missing generated layout records: ' + ', '.join(missing_layout))

    print(
        'ctypes policy validation: OK '
        f'({len(actual_skipped)} skipped functions, {len(expected_layout)} forced layouts)'
    )
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
