#!/usr/bin/env python3
"""Generate C ABI layout facts for the raw ctypes binding."""

from __future__ import annotations

import argparse
import json
import subprocess
import tempfile
from pathlib import Path

from extract_api import DEFAULT_OUTPUT as DEFAULT_API
from extract_api import DEFAULT_POLICY, ROOT_DIR, _compile_args, _headers_from_policy


DEFAULT_OUTPUT = ROOT_DIR / 'build' / 'bindings' / 'ctypes_abi.json'


def _header_for_record(record: dict) -> str | None:
    file = record.get('location', {}).get('file')
    if not isinstance(file, str) or not file.startswith('include/datoviz/'):
        return None
    return file[len('include/') :]


def _compile_and_run(source: Path, output: Path, clang: str) -> str:
    exe = output.with_suffix('.exe')
    cmd = [clang, *_compile_args(clang), str(source), '-o', str(exe)]
    subprocess.run(cmd, cwd=ROOT_DIR, check=True)
    result = subprocess.run([str(exe)], cwd=ROOT_DIR, check=True, text=True, stdout=subprocess.PIPE)
    return result.stdout


def _source_for_header(header: str, records: list[dict]) -> str:
    lines = [
        '#include <stddef.h>',
        '#include <stdio.h>',
        f'#include "{header}"',
        '#if defined(_MSC_VER)',
        '#define DVZ_ALIGNOF(T) __alignof(T)',
        '#else',
        '#define DVZ_ALIGNOF(T) _Alignof(T)',
        '#endif',
        'int main(void)',
        '{',
        '    int first_record = 1;',
        '    printf("{\\"records\\":{");',
    ]
    for index, record in enumerate(records):
        name = record['name']
        first_field = f'first_field_{index}'
        lines.extend(
            [
                f'    printf("%s\\"{name}\\":{{\\"size\\":%zu,\\"align\\":%zu,\\"fields\\":{{",',
                '        first_record ? "" : ",",',
                f'        sizeof({name}), DVZ_ALIGNOF({name}));',
                '    first_record = 0;',
                f'    int {first_field} = 1;',
            ]
        )
        for field in record.get('fields', []):
            field_name = field.get('name')
            if not field_name:
                continue
            lines.extend(
                [
                    f'    printf("%s\\"{field_name}\\":%zu",',
                    f'        {first_field} ? "" : ",",',
                    f'        offsetof({name}, {field_name}));',
                    f'    {first_field} = 0;',
                ]
            )
        lines.append('    printf("}}");')
    lines.extend(
        [
            '    printf("}}\\n");',
            '    return 0;',
            '}',
        ]
    )
    return '\n'.join(lines) + '\n'


def generate(api: dict, clang: str) -> dict:
    records_by_header: dict[str, list[dict]] = {}
    for record in api.get('records', []):
        if record.get('opaque') or not record.get('fields'):
            continue
        header = _header_for_record(record)
        if header:
            records_by_header.setdefault(header, []).append(record)

    merged: dict[str, dict] = {}
    with tempfile.TemporaryDirectory(prefix='datoviz-ctypes-abi-') as tmp:
        tmpdir = Path(tmp)
        for header, records in sorted(records_by_header.items()):
            source = tmpdir / (header.replace('/', '_').replace('.', '_') + '.c')
            source.write_text(_source_for_header(header, records))
            output = tmpdir / source.stem
            data = json.loads(_compile_and_run(source, output, clang))
            merged.update(data.get('records', {}))

    return {
        'schema_version': 1,
        'generator': 'tools/bindings/generate_ctypes_abi.py',
        'headers': _headers_from_policy(DEFAULT_POLICY) or [],
        'records': dict(sorted(merged.items())),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--api', type=Path, default=DEFAULT_API)
    parser.add_argument('--output', type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument('--clang', default='clang', help='clang executable')
    args = parser.parse_args()

    with args.api.open() as f:
        api = json.load(f)
    facts = generate(api, args.clang)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(facts, indent=2, sort_keys=True) + '\n')
    print(f"wrote {args.output} ({len(facts['records'])} records)")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
