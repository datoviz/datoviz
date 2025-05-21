# C API generator

# Imports
# -------------------------------------------------------------------------------------------------

#!/usr/bin/env python3
import json
from collections import defaultdict
from pathlib import Path
from textwrap import indent

from build_ctypes import map_python_type

# Constants
# -------------------------------------------------------------------------------------------------

HEADERS_PATH = Path('build/headers.json')
OUTPUT_PATH = Path('docs/reference/api_c.md')


SECTION_NAMES = {
    'functions': 'Functions',
    'visuals': 'Visual functions',
    'gui': 'GUI functions',
    'protocol': 'Datoviz Rendering Protocol functions',
}


# Classification
# -------------------------------------------------------------------------------------------------


def classify_header(header_name):
    if header_name == 'datoviz_protocol.h':
        return 'protocol'
    elif header_name == 'datoviz_gui.h':
        return 'gui'
    elif header_name == 'datoviz_visuals.h':
        return 'visuals'
    else:
        return 'functions'


def group_by_section_and_object(headers):
    sections = {
        'functions': defaultdict(list),
        'visuals': defaultdict(list),
        'gui': defaultdict(list),
        'protocol': defaultdict(list),
    }

    for header, contents in headers.items():
        section = classify_header(header)
        for fn in contents.get('functions', {}).values():
            name = fn['name']
            if not name.startswith('dvz_'):
                continue
            parts = name[4:].split('_', 1)
            obj = parts[0] if len(parts) > 1 else 'misc'
            obj = obj.title()
            sections[section][obj].append(fn)

    return sections


# Formatting
# -------------------------------------------------------------------------------------------------


def format_signature_py(fn):
    ret = fn.get('returns', {})
    ret_type = ret.get('dtype', 'void')
    ret_desc = ret.get('docstring', '')
    if ret_desc:
        ret_desc = f'  # returns {ret_desc} : {ret_type}'
    lines = [f'dvz.{fn["name"][4:]}({ret_desc}']
    for arg in fn.get('args', []):
        name = arg.get('name', '')
        doc = arg.get('docstring', '')
        pytype = map_python_type(arg)
        if name:
            lines.append(f'    {name},  # {doc} : {pytype}')
    lines.append(')')
    return '\n'.join(lines)


def format_signature_c(fn):
    lines = []

    ret = fn.get('returns', {})
    ret_type = ret.get('dtype', 'void')
    ret_desc = ret.get('docstring', '')
    if ret_desc:
        ret_desc = '  // returns ' + ret_desc
    lines.append(f'{ret_type} {fn["name"]}({ret_desc}')

    args = fn.get('args', [])
    for arg in args:
        dtype = arg.get('dtype', 'void')
        name = arg.get('name', '')
        doc = arg.get('docstring', '')
        lines.append(f'    {dtype} {name},  // {doc}')
    if args:
        lines[-1] = lines[-1].rstrip(',')  # remove trailing comma
    lines.append(');')
    return '\n'.join(lines)


def format_function(fn):
    docstring = fn.get('docstring', '').strip()
    out = [f'#### `{fn["name"]}()`\n']
    if docstring:
        out.append(docstring + '\n')
    out.append('=== "C"\n')
    out.append('    ```c\n' + indent(format_signature_c(fn), '    ') + '\n    ```')
    out.append('=== "Python"\n')
    out.append('    ```python\n' + indent(format_signature_py(fn), '    ') + '\n    ```')
    return '\n'.join(out) + '\n---\n'


def format_enum(name, enum_data):
    values = '\n'.join(f'{k} = {v}' for k, v in enum_data['values'])
    return f'### `{name}`\n```c\n{values}\n```' + '\n\n---\n'


def format_struct(name, struct_data):
    fields = '\n'.join(f'    {f["dtype"]} {f["name"]};' for f in struct_data['fields'])
    return f'### `{name}`\n```c\nstruct {struct_data["name"]} {{\n{fields}\n}};\n```' + '\n\n---\n'


def format_define(name, value):
    return f'### `{name}`\n```c\n#define {name} {value}\n```' + '\n\n---\n'


# Main function
# -------------------------------------------------------------------------------------------------


def build_api_c():
    data = json.loads(HEADERS_PATH.read_text())
    out = ['# C API Reference']

    sections = group_by_section_and_object(data)

    for section_key in ['functions', 'visuals', 'gui', 'protocol']:
        header = SECTION_NAMES[section_key]
        out.append(f'\n## {header}')
        for obj in sorted(sections[section_key]):
            out.append(f'\n### {obj}')
            for fn in sorted(sections[section_key][obj], key=lambda f: f['name']):
                out.append(format_function(fn))

    # Enums
    enums = {}
    for contents in data.values():
        enums.update(contents.get('enums', {}))
    if enums:
        out.append('\n## Enumerations')
        for name in sorted(enums):
            out.append(format_enum(name, enums[name]))

    # Structs
    structs = {}
    for contents in data.values():
        structs.update(contents.get('structs', {}))
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
