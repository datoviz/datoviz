#!/usr/bin/env python3
"""Extract Datoviz public C API metadata with clang's JSON AST dump."""

from __future__ import annotations

import argparse
import json
import platform
import re
import subprocess
import tempfile
from pathlib import Path
from typing import Any


ROOT_DIR = Path(__file__).resolve().parents[2]
DEFAULT_OUTPUT = ROOT_DIR / 'build' / 'bindings' / 'datoviz_api.json'
DEFAULT_POLICY = ROOT_DIR / 'spec' / 'bindings' / 'ctypes.yml'

DEFAULT_HEADERS = [
    'datoviz/app.h',
    'datoviz/canvas.h',
    'datoviz/common.h',
    'datoviz/drp2.h',
    'datoviz/ds.h',
    'datoviz/dvzmath.h',
    'datoviz/fileio.h',
    'datoviz/font.h',
    'datoviz/geom.h',
    'datoviz/gui.h',
    'datoviz/imgui.h',
    'datoviz/input.h',
    'datoviz/runner.h',
    'datoviz/scene.h',
    'datoviz/stream.h',
    'datoviz/thread.h',
    'datoviz/video.h',
    'datoviz/vk.h',
    'datoviz/vklite.h',
    'datoviz/window.h',
]

INCLUDE_DIRS = [
    'include',
    'src/common',
    'external',
    'external/cglm/include',
    'external/cimgui',
    'external/volk',
]

DEFINES = [
    'VK_NO_PROTOTYPES',
]


def _run(cmd: list[str], *, cwd: Path, stdout: Any = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        cmd,
        cwd=cwd,
        check=True,
        text=True,
        stdout=stdout,
        stderr=subprocess.PIPE,
    )


def _clang_version(clang: str) -> str:
    try:
        result = _run([clang, '--version'], cwd=ROOT_DIR, stdout=subprocess.PIPE)
    except (OSError, subprocess.CalledProcessError):
        return ''
    return result.stdout.splitlines()[0] if result.stdout else ''


def _location(node: dict[str, Any]) -> dict[str, Any]:
    loc = node.get('loc') or {}
    rng = node.get('range') or {}
    begin = rng.get('begin') or {}
    end = rng.get('end') or {}
    expansion = begin.get('expansionLoc') or {}
    spelling = begin.get('spellingLoc') or {}
    loc_parent = loc.get('includedFrom') or {}
    begin_parent = begin.get('includedFrom') or {}
    end_parent = end.get('includedFrom') or {}
    file = (
        loc.get('file')
        or expansion.get('file')
        or spelling.get('file')
        or loc_parent.get('file')
        or begin_parent.get('file')
        or end_parent.get('file')
    )
    line = loc.get('line') or expansion.get('line') or spelling.get('line') or begin.get('line')
    col = loc.get('col') or expansion.get('col') or spelling.get('col') or begin.get('col')
    return {'file': file, 'line': line, 'column': col}


def _is_public_location(node: dict[str, Any]) -> bool:
    file = _location(node).get('file') or ''
    return file.startswith('include/datoviz/') or '/include/datoviz/' in file


def _walk(node: dict[str, Any]):
    yield node
    for child in node.get('inner') or []:
        yield from _walk(child)


def _comment_text(node: dict[str, Any]) -> str:
    parts: list[str] = []

    def visit(comment: dict[str, Any]) -> None:
        if comment.get('kind') == 'TextComment':
            text = comment.get('text', '').strip()
            if text:
                parts.append(text)
        for child in comment.get('inner') or []:
            visit(child)

    for child in node.get('inner') or []:
        if child.get('kind') == 'FullComment':
            visit(child)
            break
    return ' '.join(parts)


def _constant_value(node: dict[str, Any]) -> int | None:
    for child in _walk(node):
        value = child.get('value')
        if value is None:
            continue
        try:
            return int(value, 0)
        except ValueError:
            continue
    return None


def _type_info(type_node: dict[str, Any] | None) -> dict[str, Any]:
    type_node = type_node or {}
    qual = type_node.get('qualType', '')
    info = {'qualtype': qual}
    if type_node.get('desugaredQualType'):
        info['desugared'] = type_node['desugaredQualType']
    if type_node.get('typeAliasDeclId'):
        info['type_alias_id'] = type_node['typeAliasDeclId']
    return info


def _result_type(function_node: dict[str, Any]) -> dict[str, Any]:
    qual = function_node.get('type', {}).get('qualType', '')
    match = re.match(r'^(?P<result>.+?)\s*\(', qual)
    if match:
        return {'qualtype': match.group('result').strip()}
    return {'qualtype': ''}


def _parameters(function_node: dict[str, Any]) -> list[dict[str, Any]]:
    out = []
    for child in function_node.get('inner') or []:
        if child.get('kind') != 'ParmVarDecl':
            continue
        loc = _location(child)
        out.append(
            {
                'name': child.get('name', ''),
                'type': _type_info(child.get('type')),
                'location': loc,
            }
        )
    return out


def _is_exported_function(function_node: dict[str, Any]) -> bool:
    if function_node.get('storageClass') == 'static':
        return False
    for child in function_node.get('inner') or []:
        if child.get('kind') == 'VisibilityAttr' and child.get('visibility') == 'default':
            return True
    return False


def _extract_ast(ast: dict[str, Any]) -> dict[str, list[dict[str, Any]]]:
    functions: dict[str, dict[str, Any]] = {}
    records: dict[str, dict[str, Any]] = {}
    typedefs: dict[str, dict[str, Any]] = {}
    enums_by_id: dict[str, dict[str, Any]] = {}
    enum_names_by_id: dict[str, str] = {}

    for node in _walk(ast):
        if not _is_public_location(node):
            continue
        kind = node.get('kind')
        loc = _location(node)

        if kind == 'TypedefDecl':
            name = node.get('name', '')
            if not name:
                continue
            typedefs[name] = {
                'name': name,
                'type': _type_info(node.get('type')),
                'location': loc,
            }
            for child in node.get('inner') or []:
                owned = child.get('ownedTagDecl') or {}
                if owned.get('kind') == 'EnumDecl':
                    enum_names_by_id[owned.get('id', '')] = name
                elif owned.get('kind') == 'RecordDecl':
                    record_name = owned.get('name') or name
                    records.setdefault(
                        record_name,
                        {
                            'name': record_name,
                            'kind': 'struct',
                            'opaque': True,
                            'location': loc,
                            'fields': [],
                        },
                    )

        elif kind == 'EnumDecl':
            values = []
            next_value = 0
            for child in node.get('inner') or []:
                if child.get('kind') != 'EnumConstantDecl':
                    continue
                value = _constant_value(child)
                if value is None:
                    value = next_value
                values.append(
                    {
                        'name': child.get('name', ''),
                        'value': value,
                        'location': _location(child),
                    }
                )
                next_value = value + 1
            if values:
                enums_by_id[node.get('id', '')] = {
                    'name': node.get('name', ''),
                    'location': loc,
                    'values': values,
                }

        elif kind == 'RecordDecl':
            name = node.get('name', '')
            if not name:
                continue
            record = {
                'name': name,
                'kind': node.get('tagUsed', 'struct'),
                'opaque': not bool(node.get('completeDefinition')),
                'location': loc,
                'fields': [],
            }
            if node.get('completeDefinition'):
                fields = []
                for child in node.get('inner') or []:
                    if child.get('kind') != 'FieldDecl':
                        continue
                    fields.append(
                        {
                            'name': child.get('name', ''),
                            'type': _type_info(child.get('type')),
                            'location': _location(child),
                        }
                    )
                record['fields'] = fields
            if not records.get(name) or not record['opaque']:
                records[name] = record

        elif kind == 'FunctionDecl':
            name = node.get('name', '')
            if not name.startswith('dvz_'):
                continue
            if not _is_exported_function(node):
                continue
            functions[name] = {
                'name': name,
                'location': loc,
                'result': _result_type(node),
                'parameters': _parameters(node),
                'doc': _comment_text(node),
            }

    enums = []
    for enum_id, enum in enums_by_id.items():
        if not enum['name'] and enum_id in enum_names_by_id:
            enum['name'] = enum_names_by_id[enum_id]
        if enum['name']:
            enums.append(enum)

    return {
        'functions': sorted(functions.values(), key=lambda x: x['name']),
        'records': sorted(records.values(), key=lambda x: x['name']),
        'typedefs': sorted(typedefs.values(), key=lambda x: x['name']),
        'enums': sorted(enums, key=lambda x: x['name']),
    }


def _merge(target: dict[str, dict[str, Any]], key: str, values: list[dict[str, Any]]) -> None:
    for value in values:
        name = value.get('name', '')
        if not name:
            continue
        if key == 'records' and name in target and target[name].get('opaque') and not value.get('opaque'):
            target[name] = value
        else:
            target.setdefault(name, value)


def _extract_header(clang: str, header: str, tmpdir: Path) -> dict[str, list[dict[str, Any]]]:
    source = tmpdir / (header.replace('/', '_').replace('.', '_') + '.c')
    source.write_text(f'#include "{header}"\n')
    ast_path = tmpdir / (source.stem + '.json')
    cmd = [clang, '-x', 'c', '-std=c11']
    cmd.extend(f'-I{path}' for path in INCLUDE_DIRS)
    cmd.extend(f'-D{name}' for name in DEFINES)
    cmd.extend(['-fsyntax-only', '-Xclang', '-ast-dump=json', str(source)])
    with ast_path.open('w') as f:
        _run(cmd, cwd=ROOT_DIR, stdout=f)
    with ast_path.open() as f:
        return _extract_ast(json.load(f))


def extract_api(headers: list[str], clang: str) -> dict[str, Any]:
    merged = {'functions': {}, 'records': {}, 'typedefs': {}, 'enums': {}}
    with tempfile.TemporaryDirectory(prefix='datoviz-api-') as tmp:
        tmpdir = Path(tmp)
        for header in headers:
            extracted = _extract_header(clang, header, tmpdir)
            for key in merged:
                _merge(merged[key], key, extracted[key])

    return {
        'schema_version': 1,
        'generator': 'tools/bindings/extract_api.py',
        'platform': platform.platform(),
        'clang': {'executable': clang, 'version': _clang_version(clang)},
        'headers': headers,
        'include_dirs': INCLUDE_DIRS,
        'defines': DEFINES,
        'functions': sorted(merged['functions'].values(), key=lambda x: x['name']),
        'records': sorted(merged['records'].values(), key=lambda x: x['name']),
        'typedefs': sorted(merged['typedefs'].values(), key=lambda x: x['name']),
        'enums': sorted(merged['enums'].values(), key=lambda x: x['name']),
    }


def _headers_from_policy(path: Path) -> list[str] | None:
    if not path.exists():
        return None
    try:
        import yaml  # type: ignore

        with path.open() as f:
            policy = yaml.safe_load(f) or {}
        headers = policy.get('headers', {}).get('include')
        if isinstance(headers, list) and all(isinstance(item, str) for item in headers):
            return headers
    except ModuleNotFoundError:
        pass

    headers: list[str] = []
    in_include_list = False
    for line in path.read_text().splitlines():
        stripped = line.strip()
        if stripped == 'include:':
            in_include_list = True
            continue
        if in_include_list and stripped.startswith('- '):
            headers.append(stripped[2:].strip())
        elif in_include_list and stripped and not stripped.startswith('#'):
            break
    return headers or None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--clang', default='clang', help='clang executable')
    parser.add_argument('--output', type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument('--policy', type=Path, default=DEFAULT_POLICY)
    parser.add_argument('--header', action='append', dest='headers', help='public header to parse')
    args = parser.parse_args()

    headers = args.headers or _headers_from_policy(args.policy) or DEFAULT_HEADERS
    api = extract_api(headers, args.clang)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(api, indent=2, sort_keys=True) + '\n')
    print(
        f"wrote {args.output} "
        f"({len(api['functions'])} functions, {len(api['records'])} records, "
        f"{len(api['enums'])} enums)"
    )
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
