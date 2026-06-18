#!/usr/bin/env python3
"""Extract Datoviz public C API metadata with libclang."""

from __future__ import annotations

import argparse
import json
import platform
import re
import subprocess
import tempfile
from pathlib import Path
from typing import Any

from clang import cindex


ROOT_DIR = Path(__file__).resolve().parents[2]
DEFAULT_OUTPUT = ROOT_DIR / 'build' / 'bindings' / 'datoviz_api.json'
DEFAULT_POLICY = ROOT_DIR / 'spec' / 'bindings' / 'ctypes.yml'

DEFAULT_HEADERS = [
    'datoviz/app.h',
    'datoviz/canvas.h',
    'datoviz/common.h',
    'datoviz/drp2.h',
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

IDENTIFIER_RE = re.compile(r'^[A-Za-z_][A-Za-z0-9_]*$')
SOURCE_CACHE: dict[str, list[str]] = {}


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


def _clang_runtime_version() -> str:
    try:
        return str(cindex.conf.lib.clang_getClangVersion())
    except Exception:
        return ''


def _optional_cmd_output(cmd: list[str]) -> str | None:
    try:
        return subprocess.check_output(cmd, text=True, stderr=subprocess.DEVNULL).strip()
    except (OSError, subprocess.CalledProcessError):
        return None


def _compile_args(clang: str) -> list[str]:
    args = ['-x', 'c', '-std=gnu11']
    resource_dir = _optional_cmd_output([clang, '-print-resource-dir'])
    if resource_dir:
        args.extend(['-resource-dir', resource_dir])
    if platform.system() == 'Darwin':
        sdk = _optional_cmd_output(['xcrun', '--show-sdk-path'])
        if sdk:
            args.extend(['-isysroot', sdk])
    args.extend(f'-I{path}' for path in INCLUDE_DIRS)
    args.extend(f'-D{name}' for name in DEFINES)
    return args


def _display_path(path: str | None) -> str | None:
    if not path:
        return None
    resolved = Path(path).resolve()
    try:
        return str(resolved.relative_to(ROOT_DIR))
    except ValueError:
        return str(resolved)


def _location(cursor: cindex.Cursor) -> dict[str, Any]:
    file = cursor.location.file
    return {
        'file': _display_path(file.name if file else None),
        'line': cursor.location.line or None,
        'column': cursor.location.column or None,
    }


def _is_public_location(cursor: cindex.Cursor) -> bool:
    file = _location(cursor).get('file') or ''
    return file.startswith('include/datoviz/')


def _type_info(type_: cindex.Type) -> dict[str, Any]:
    info = {'qualtype': type_.spelling}
    canonical = type_.get_canonical().spelling
    if canonical and canonical != type_.spelling:
        info['canonical'] = canonical
    if type_.kind == cindex.TypeKind.CONSTANTARRAY:
        info['array_size'] = type_.get_array_size()
        info['element_type'] = type_.get_array_element_type().spelling
    return info


def _is_identifier(name: str) -> bool:
    return bool(IDENTIFIER_RE.match(name))


def _result_type(cursor: cindex.Cursor) -> dict[str, Any]:
    return _type_info(cursor.result_type)


def _parameters(cursor: cindex.Cursor) -> list[dict[str, Any]]:
    out = []
    for arg in cursor.get_arguments():
        out.append(
            {
                'name': arg.spelling,
                'type': _type_info(arg.type),
                'location': _location(arg),
            }
        )
    return out


def _is_exported_function(cursor: cindex.Cursor) -> bool:
    if cursor.storage_class == cindex.StorageClass.STATIC:
        return False
    if cursor.linkage != cindex.LinkageKind.EXTERNAL:
        return False
    loc = _location(cursor)
    file = loc.get('file')
    line = loc.get('line')
    if not isinstance(file, str) or not isinstance(line, int):
        return False
    lines = SOURCE_CACHE.get(file)
    if lines is None:
        path = ROOT_DIR / file
        if not path.exists():
            return False
        lines = path.read_text(encoding='utf8').splitlines()
        SOURCE_CACHE[file] = lines
    start = max(0, line - 4)
    end = min(len(lines), line + 2)
    return any('DVZ_EXPORT' in lines[index] for index in range(start, end))


def _extract_translation_unit(tu: cindex.TranslationUnit) -> dict[str, list[dict[str, Any]]]:
    functions: dict[str, dict[str, Any]] = {}
    records: dict[str, dict[str, Any]] = {}
    typedefs: dict[str, dict[str, Any]] = {}
    enums: dict[str, dict[str, Any]] = {}

    for cursor in tu.cursor.walk_preorder():
        if not _is_public_location(cursor):
            continue

        kind = cursor.kind
        loc = _location(cursor)

        if kind == cindex.CursorKind.TYPEDEF_DECL:
            name = cursor.spelling
            if not name:
                continue
            typedefs[name] = {
                'name': name,
                'type': _type_info(cursor.underlying_typedef_type),
                'location': loc,
            }
            if cursor.underlying_typedef_type.kind == cindex.TypeKind.RECORD:
                records.setdefault(
                    name,
                    {
                        'name': name,
                        'kind': 'struct',
                        'opaque': True,
                        'location': loc,
                        'fields': [],
                    },
                )

        elif kind == cindex.CursorKind.ENUM_DECL:
            name = cursor.spelling
            if not name:
                continue
            values = []
            for child in cursor.get_children():
                if child.kind != cindex.CursorKind.ENUM_CONSTANT_DECL:
                    continue
                values.append(
                    {
                        'name': child.spelling,
                        'value': child.enum_value,
                        'location': _location(child),
                    }
                )
            if values:
                enums[name] = {
                    'name': name,
                    'location': loc,
                    'values': values,
                }

        elif kind in (cindex.CursorKind.STRUCT_DECL, cindex.CursorKind.UNION_DECL):
            name = cursor.spelling
            if not name or not _is_identifier(name):
                continue
            is_definition = cursor.is_definition()
            tag = 'union' if kind == cindex.CursorKind.UNION_DECL else 'struct'
            record: dict[str, Any] = {
                'name': name,
                'kind': tag,
                'opaque': not is_definition,
                'location': loc,
                'fields': [],
            }
            if is_definition:
                fields = []
                for child in cursor.get_children():
                    if child.kind != cindex.CursorKind.FIELD_DECL:
                        continue
                    fields.append(
                        {
                            'name': child.spelling,
                            'type': _type_info(child.type),
                            'location': _location(child),
                        }
                    )
                record['fields'] = fields
            if not records.get(name) or not record['opaque']:
                records[name] = record

        elif kind == cindex.CursorKind.FUNCTION_DECL:
            name = cursor.spelling
            if not name.startswith('dvz_'):
                continue
            if not _is_exported_function(cursor):
                continue
            functions[name] = {
                'name': name,
                'location': loc,
                'result': _result_type(cursor),
                'parameters': _parameters(cursor),
                'doc': (cursor.raw_comment or '').strip(),
            }

    return {
        'functions': sorted(functions.values(), key=lambda x: x['name']),
        'records': sorted(records.values(), key=lambda x: x['name']),
        'typedefs': sorted(typedefs.values(), key=lambda x: x['name']),
        'enums': sorted(enums.values(), key=lambda x: x['name']),
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
    tu = cindex.Index.create().parse(str(source), args=_compile_args(clang), options=0)
    errors = [
        str(diag)
        for diag in tu.diagnostics
        if diag.severity >= cindex.Diagnostic.Error
    ]
    if errors:
        raise RuntimeError(f'failed to parse {header}:\n' + '\n'.join(errors))
    return _extract_translation_unit(tu)


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
        'clang': {
            'executable': clang,
            'version': _clang_version(clang),
            'libclang': _clang_runtime_version(),
        },
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
