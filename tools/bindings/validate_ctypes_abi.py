#!/usr/bin/env python3
"""Validate generated ctypes record layouts against C ABI facts."""

from __future__ import annotations

import argparse
import ctypes
import json
import sys
from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parents[2]
DEFAULT_ABI = ROOT_DIR / 'build' / 'bindings' / 'ctypes_abi.json'


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--abi', type=Path, default=DEFAULT_ABI)
    args = parser.parse_args()

    sys.path.insert(0, str(ROOT_DIR))
    import datoviz._ctypes as raw  # noqa: PLC0415

    with args.abi.open() as f:
        abi = json.load(f)
    facts = abi.get('records', {})

    checked = 0
    for name in raw._DATOVIZ_CTYPES_LAYOUT_RECORDS:
        if name not in facts:
            raise SystemExit(f'missing ABI facts for {name}')
        cls = getattr(raw, name)
        record = facts[name]
        size = ctypes.sizeof(cls)
        align = ctypes.alignment(cls)
        if size != record['size']:
            raise SystemExit(f'{name}: ctypes size {size} != C size {record["size"]}')
        if align != record['align']:
            raise SystemExit(f'{name}: ctypes alignment {align} != C alignment {record["align"]}')
        for field_name, offset in record.get('fields', {}).items():
            if not hasattr(cls, field_name):
                raise SystemExit(f'{name}: missing ctypes field {field_name}')
            actual = getattr(cls, field_name).offset
            if actual != offset:
                raise SystemExit(
                    f'{name}.{field_name}: ctypes offset {actual} != C offset {offset}'
                )
        checked += 1

    print(f'ctypes ABI validation: OK ({checked} records)')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
