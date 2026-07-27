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

REQUIRED_RECORD_FACTS = {
    'DvzBounds': (56, 8, {'min': 8, 'max': 32}),
    'DvzFieldGeometry': (104, 8, {'origin': 24, 'unit': 72}),
    'DvzGuideHit': (208, 8, {'box_px': 36, 'data_value': 64, 'label': 79}),
    'DvzGuideLayout': (208, 8, {'box_px': 36, 'data_value': 64, 'label': 80}),
    'DvzMVP': (208, 16, {'model': 0, 'view': 64, 'proj': 128, 'time': 192}),
    'DvzPanelFrameInfo': (
        4544,
        16,
        {
            'data_to_view': 304,
            'has_view2d': 368,
            'has_valid_source_x': 369,
            'has_valid_source_y': 370,
            'has_valid_visible_x': 371,
            'has_valid_visible_y': 372,
            'diagnostics': 376,
        },
    ),
    'DvzPanzoomEval': (24, 4, {'base_extent': 0, 'viewport_width': 16}),
    'DvzPanzoomResolved': (224, 16, {'mvp': 0, 'visible_extent': 208}),
    'DvzVisualTransformDesc': (112, 16, {'label': 32, 'matrix': 48}),
}

PORTABLE_REQUIRED_RECORDS = {
    'DvzBounds',
    'DvzFieldGeometry',
    'DvzGuideHit',
    'DvzGuideLayout',
    'DvzPanzoomEval',
}

CONDITIONAL_REQUIRED_RECORDS = {
    'DvzMVP',
    'DvzPanelFrameInfo',
    'DvzPanelView2DState',
    'DvzPanelView3DState',
    'DvzPanzoomResolved',
    'DvzVisualTransformDesc',
}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--abi', type=Path, default=DEFAULT_ABI)
    args = parser.parse_args()

    sys.path.insert(0, str(ROOT_DIR))
    import datoviz._ctypes as raw  # noqa: PLC0415

    with args.abi.open() as f:
        abi = json.load(f)
    facts = abi.get('records', {})
    active_records = set(raw._DATOVIZ_CTYPES_LAYOUT_RECORDS)

    missing_portable = sorted(PORTABLE_REQUIRED_RECORDS - active_records)
    if missing_portable:
        raise SystemExit(
            'required portable ctypes layouts are unavailable: ' + ', '.join(missing_portable)
        )

    alignment_supported = raw._ctypes_alignment_is_effective(16)
    active_conditional = CONDITIONAL_REQUIRED_RECORDS & active_records
    conditional_functions = {
        name
        for name, dependencies in raw._FUNCTION_LAYOUT_DEPENDENCIES.items()
        if CONDITIONAL_REQUIRED_RECORDS.intersection(dependencies)
    }
    if alignment_supported:
        missing_conditional = sorted(CONDITIONAL_REQUIRED_RECORDS - active_conditional)
        if missing_conditional:
            raise SystemExit(
                'required aligned ctypes layouts are unavailable on a supported runtime: '
                + ', '.join(missing_conditional)
            )
        missing_functions = sorted(
            name for name in conditional_functions if not hasattr(raw, name)
        )
        if missing_functions:
            raise SystemExit(
                'aligned-layout APIs are unavailable on a supported runtime: '
                + ', '.join(missing_functions)
            )
    else:
        if active_conditional:
            raise SystemExit(
                'aligned ctypes layouts are active on an unsupported runtime: '
                + ', '.join(sorted(active_conditional))
            )
        unexpected_functions = sorted(
            name for name in conditional_functions if hasattr(raw, name)
        )
        if unexpected_functions:
            raise SystemExit(
                'aligned-layout APIs are active on an unsupported runtime: '
                + ', '.join(unexpected_functions)
            )

    for name, (expected_size, expected_align, expected_fields) in REQUIRED_RECORD_FACTS.items():
        record = facts.get(name)
        if record is None:
            raise SystemExit(f'missing required ABI facts for {name}')
        if record.get('size') != expected_size:
            raise SystemExit(
                f'{name}: C size {record.get("size")} != required size {expected_size}'
            )
        if record.get('align') != expected_align:
            raise SystemExit(
                f'{name}: C alignment {record.get("align")} != required alignment {expected_align}'
            )
        for field_name, expected_offset in expected_fields.items():
            actual_offset = record.get('fields', {}).get(field_name)
            if actual_offset != expected_offset:
                raise SystemExit(
                    f'{name}.{field_name}: C offset {actual_offset} != required offset '
                    f'{expected_offset}'
                )

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
