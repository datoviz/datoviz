# C API generator

# Imports
# -------------------------------------------------------------------------------------------------

import json
import re
from collections import defaultdict
from pathlib import Path
from textwrap import indent

from bindings.generate_ctypes import _callback_typedefs, _ctype_for_type

# Constants
# -------------------------------------------------------------------------------------------------

API_PATH = Path('build/bindings/datoviz_api.json')
OUTPUT_PATH = Path('docs/reference/api_c.md')


SECTION_NAMES = {
    'functions': 'Functions',
    'visuals': 'Visual functions',
    'gui': 'GUI functions',
    'protocol': 'Datoviz Rendering Protocol functions',
}


# Classification
# -------------------------------------------------------------------------------------------------


PARAM_RE = re.compile(r'^@param\s+(?P<name>\w+)\s*(?P<doc>.*)$')
RETURN_RE = re.compile(r'^@returns?\s+(?P<doc>.*)$')


def classify_header(header_name):
    path = Path(header_name)
    name = path.name
    if name == 'drp2.h' or '/drp2/' in header_name:
        return 'protocol'
    elif name in {'gui.h', 'imgui.h'}:
        return 'gui'
    elif name == 'datoviz_visuals.h':
        return 'visuals'
    else:
        return 'functions'


def group_by_section_and_object(api):
    sections = {
        'functions': defaultdict(list),
        'visuals': defaultdict(list),
        'gui': defaultdict(list),
        'protocol': defaultdict(list),
    }

    for fn in api.get('functions', []):
        name = fn['name']
        if not name.startswith('dvz_'):
            continue
        header = fn.get('location', {}).get('file') or ''
        section = classify_header(header)
        parts = name[4:].split('_', 1)
        obj = parts[0] if len(parts) > 1 else 'misc'
        obj = obj.title()
        sections[section][obj].append(fn)

    return sections


# Formatting
# -------------------------------------------------------------------------------------------------


def _type_name(type_info):
    return type_info.get('qualtype') or type_info.get('canonical') or 'void'


def _clean_doc(raw):
    raw = (raw or '').strip()
    if raw.startswith('/**'):
        raw = raw[3:]
    if raw.endswith('*/'):
        raw = raw[:-2]
    lines = []
    for line in raw.splitlines():
        line = line.strip()
        if line.startswith('*'):
            line = line[1:].strip()
        lines.append(line)
    return lines


def _doc_parts(raw):
    text = []
    params = {}
    ret = ''
    for line in _clean_doc(raw):
        if not line:
            if text and text[-1]:
                text.append('')
            continue
        param_match = PARAM_RE.match(line)
        if param_match:
            params[param_match.group('name')] = param_match.group('doc').strip()
            continue
        return_match = RETURN_RE.match(line)
        if return_match:
            ret = return_match.group('doc').strip()
            continue
        if line.startswith('@'):
            continue
        text.append(line)
    while text and not text[-1]:
        text.pop()
    return '\n'.join(text), params, ret


def _doc_context(api):
    records = {record['name'] for record in api.get('records', []) if record.get('name')}
    enums = {enum['name'] for enum in api.get('enums', []) if enum.get('name')}
    callbacks = set(_callback_typedefs(api))
    return records, enums, callbacks


def _python_type(type_info, ctx):
    records, enums, callbacks = ctx
    return _ctype_for_type(type_info, records, enums, callbacks=callbacks) or 'ctypes.c_void_p'


def format_signature_py(fn, ctx):
    _, _, ret_desc = _doc_parts(fn.get('doc', ''))
    ret_type = _python_type(fn.get('result', {'qualtype': 'void'}), ctx)
    if ret_desc:
        ret_desc = f'  # returns {ret_desc} : {ret_type}'
    lines = [f'dvz.{fn["name"][4:]}({ret_desc}']
    _, param_docs, _ = _doc_parts(fn.get('doc', ''))
    for arg in fn.get('parameters', []):
        name = arg.get('name', '')
        doc = param_docs.get(name, '')
        pytype = _python_type(arg.get('type', {}), ctx)
        if name:
            lines.append(f'    {name},  # {doc} : {pytype}')
    lines.append(')')
    return '\n'.join(lines)


def format_signature_c(fn):
    lines = []

    _, _, ret_desc = _doc_parts(fn.get('doc', ''))
    ret_type = _type_name(fn.get('result', {'qualtype': 'void'}))
    if ret_desc:
        ret_desc = '  // returns ' + ret_desc
    lines.append(f'{ret_type} {fn["name"]}({ret_desc}')

    _, param_docs, _ = _doc_parts(fn.get('doc', ''))
    args = fn.get('parameters', [])
    for arg in args:
        dtype = _type_name(arg.get('type', {}))
        name = arg.get('name', '')
        doc = param_docs.get(name, '')
        lines.append(f'    {dtype} {name},  // {doc}')
    if args:
        lines[-1] = lines[-1].rstrip(',')  # remove trailing comma
    lines.append(');')
    return '\n'.join(lines)


def format_function(fn, ctx):
    docstring, _, _ = _doc_parts(fn.get('doc', ''))
    out = [f'#### `{fn["name"]}()`\n']
    if docstring:
        out.append(docstring + '\n')
    out.append('=== "C"\n')
    out.append('    ```c\n' + indent(format_signature_c(fn), '    ') + '\n    ```')
    out.append('=== "Python"\n')
    out.append('    ```python\n' + indent(format_signature_py(fn, ctx), '    ') + '\n    ```')
    return '\n'.join(out) + '\n---\n'


def format_enum(name, enum_data):
    values = '\n'.join(f'{value["name"]} = {value["value"]}' for value in enum_data['values'])
    return f'### `{name}`\n```c\n{values}\n```' + '\n\n---\n'


def format_struct(name, struct_data):
    if struct_data.get('opaque'):
        body = f'typedef struct {name} {name};'
    else:
        fields = '\n'.join(
            f'    {_type_name(field.get("type", {}))} {field["name"]};'
            for field in struct_data.get('fields', [])
        )
        body = f'{struct_data.get("kind", "struct")} {name} {{\n{fields}\n}};'
    return f'### `{name}`\n```c\n{body}\n```' + '\n\n---\n'


def format_define(name, value):
    return f'### `{name}`\n```c\n#define {name} {value}\n```' + '\n\n---\n'


# Main function
# -------------------------------------------------------------------------------------------------


def build_api_c():
    data = json.loads(API_PATH.read_text())
    out = ['# C API Reference']

    sections = group_by_section_and_object(data)
    ctx = _doc_context(data)

    for section_key in ['functions', 'visuals', 'gui', 'protocol']:
        header = SECTION_NAMES[section_key]
        out.append(f'\n## {header}')
        for obj in sorted(sections[section_key]):
            out.append(f'\n### {obj}')
            for fn in sorted(sections[section_key][obj], key=lambda f: f['name']):
                out.append(format_function(fn, ctx))

    # Enums
    enums = {enum['name']: enum for enum in data.get('enums', [])}
    if enums:
        out.append('\n## Enumerations')
        for name in sorted(enums):
            out.append(format_enum(name, enums[name]))

    # Structs
    structs = {record['name']: record for record in data.get('records', [])}
    if structs:
        out.append(
            '\n## Structures\n\n> **Note**: The information about these structures is provided for reference only, do not use them in production as the structures may change with each release.\n\n'
        )
        for name in sorted(structs):
            out.append(format_struct(name, structs[name]))

    # # Defines
    # defines = {}
    # for contents in data.values():
    #     defines.update(contents.get('defines', {}))
    # if defines:
    #     out.append('\n## Defines')
    #     for name in sorted(defines):
    #         out.append(format_define(name, defines[name]))

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_PATH.write_text('\n\n'.join(out), encoding='utf-8')


if __name__ == '__main__':
    build_api_c()
