#!/usr/bin/env python3
"""Validate generated array-facade coverage against binding policy."""

from __future__ import annotations

import argparse
import ast
import json
from pathlib import Path

from generate_array_facade import _load_policy


ROOT_DIR = Path(__file__).resolve().parents[2]
DEFAULT_API = ROOT_DIR / 'build' / 'bindings' / 'datoviz_api.json'
DEFAULT_FACADE = ROOT_DIR / 'datoviz' / '_array_facade.py'
DEFAULT_POLICY = ROOT_DIR / 'spec' / 'bindings' / 'ctypes.yml'


def _function_names(api_path: Path) -> set[str]:
    with api_path.open() as f:
        api = json.load(f)
    return {
        function['name']
        for function in api.get('functions', [])
        if isinstance(function, dict) and function.get('name', '').startswith('dvz_')
    }


def _generated_defs(path: Path) -> set[str]:
    if not path.exists():
        raise SystemExit(f'{path} is missing; run `just ctypes`')
    module = ast.parse(path.read_text())
    return {node.name for node in module.body if isinstance(node, ast.FunctionDef)}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--api', type=Path, default=DEFAULT_API)
    parser.add_argument('--facade', type=Path, default=DEFAULT_FACADE)
    parser.add_argument('--policy', type=Path, default=DEFAULT_POLICY)
    args = parser.parse_args()

    policy_names = set(_load_policy(args.policy))
    generated_names = _generated_defs(args.facade)
    api_names = _function_names(args.api)

    missing = sorted(policy_names - generated_names)
    unexpected = sorted((generated_names & api_names) - policy_names)
    if missing or unexpected:
        lines = ['array facade coverage mismatch']
        if missing:
            lines.append('  missing policy wrappers: ' + ', '.join(missing))
        if unexpected:
            lines.append('  generated wrappers without policy: ' + ', '.join(unexpected))
        raise SystemExit('\n'.join(lines))

    passthrough_count = len(api_names - policy_names)
    print(
        'array facade coverage: OK '
        f'({len(policy_names)} wrappers, {passthrough_count} dvz_* passthrough candidates)'
    )
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
