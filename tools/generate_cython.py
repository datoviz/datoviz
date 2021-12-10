# Generate the Cython wrapping file.

# Imports
# -------------------------------------------------------------------------------------------------

from functools import lru_cache
import json
from pathlib import Path
import os
from pprint import pprint
import re
import sys
from textwrap import indent

from tqdm import tqdm
import pyparsing as pp
from pyparsing import (
    Suppress, Word, alphas, alphanums, nums, Optional, Group, ZeroOrMore, empty, restOfLine,
    Keyword, cStyleComment, Empty, Literal
)

from parse_headers import ROOT_DIR, iter_vars


# Constants
# -------------------------------------------------------------------------------------------------

ENUM_START = '# ENUM START'
ENUM_END = '# ENUM END'
STRUCT_START = '# STRUCT START'
STRUCT_END = '# STRUCT END'
UNION_START = '# UNION START'
UNION_END = '# UNION END'
FUNCTION_START = '# FUNCTION START'
FUNCTION_END = '# FUNCTION END'


# Cython generation utils
# -------------------------------------------------------------------------------------------------

def insert_into_file(filename, start, end, insert):
    text = filename.read_text()
    i0 = text.index(start)
    i1 = text.index(end)
    out = text[:i0 + len(start) + 1]
    out += indent(insert, '    ')
    out += text[i1 - 5:]
    filename.write_text(out)


@lru_cache(maxsize=64)
def _camel_to_snake(name):
    name = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', name).lower()


def _keep(n, t):
    if n in t:
        return True
    for _ in t:
        if n.startswith(_):
            return True
    return False


# Cython generation
# -------------------------------------------------------------------------------------------------

def _generate_enum(enum, defines=None):
    assert enum
    name = enum['name']
    values = enum['values']
    out = ''
    out += f'ctypedef enum {name}:\n'
    for identifier, value in values:
        # defines replacements
        if defines and value in defines:
            value = f'{defines[value]}  # {value}'
        out += f'    {identifier} = {value}\n'
    out += '\n'
    return out


def _generate_struct(struct, defines=None):
    assert struct
    name = struct['name']
    out = ''
    out += f'ctypedef {struct["type"]} {name}:\n'
    for field in struct['fields']:
        const = field.get('const', None)
        dtype = field.get('dtype', None)
        count = field.get('count', '')
        identifier = field.get('name', None)
        if dtype == 'bool':
            dtype = 'bint'
        if const:
            dtype = "const " + dtype
        if count:
            if defines and count in defines:
                count = defines[count]
            count = f'[{count}]'
        out += f'    {dtype} {identifier}{count}\n'
    out += '\n'
    return out


def _generate_function(func):
    assert func
    name = func['name']
    args_s = []
    out = func.get('returns', None) or 'void'
    if out == 'bool':
        out = 'bint'
    for arg in func['args']:
        const = arg.get('const', None)
        dtype = arg.get('dtype', None)
        argname = arg.get('name', None)
        if not argname:
            if 'int' in dtype:
                argname = 'n%d' % len(args_s)
            elif dtype == 'float':
                argname = 'value'
            elif 'vec' in dtype:
                argname = 'vec'
            elif dtype == 'void*':
                argname = 'buf'
            elif 'char' in dtype:
                argname = 's'
            elif dtype == 'void':
                dtype = ''
                argname = ''
            elif dtype == 'bool':
                argname = 'value'
            elif dtype == 'size_t':
                argname = 'size'
            elif 'Dvz' in dtype:
                argname = _camel_to_snake(
                    dtype.replace('Dvz', '')).replace('*', '')
            else:
                raise ValueError(dtype)
        if const:
            dtype = "const " + dtype
        if dtype == 'bool':
            dtype = 'bint'
        args_s.append(f'{dtype} {argname}'.strip())
    args = ', '.join(args_s)
    return f'{out} {name}({args})\n\n'


def generate_enums():
    out = ''
    defines = dict(iter_vars("defines"))
    for n, v in iter_vars('enums'):
        out += _generate_enum(v, defines=defines)
    return out


def generate_structs(structs):
    out = ''
    defines = dict(iter_vars("defines"))
    for n, v in iter_vars('structs'):
        if _keep(n, structs):
            out += _generate_struct(v, defines=defines)
    return out


def generate_functions(functions):
    out = ''
    for n, v in iter_vars('functions'):
        if _keep(n, functions):
            out += _generate_function(v)
    return out


def generate_cython():

    # _types.pxd
    path = ROOT_DIR / 'datoviz/_types.pxd'
    insert_into_file(path, ENUM_START, ENUM_END, generate_enums())

    # request.h
    functions = (
        'dvz_request',
        'dvz_create',
        'dvz_update',
        'dvz_set',
        'dvz_delete',
    )
    path = ROOT_DIR / 'datoviz/request.pxd'
    insert_into_file(path, FUNCTION_START, FUNCTION_END,
                     generate_functions(functions))
    # insert_into_file(path, STRUCT_START, STRUCT_END, generate_structs())

    # renderer.h
    functions = (
        'dvz_init',
        'dvz_host',
        'dvz_renderer',
    )
    path = ROOT_DIR / 'datoviz/renderer.pxd'
    insert_into_file(path, FUNCTION_START, FUNCTION_END,
                     generate_functions(functions))
    # insert_into_file(path, STRUCT_START, STRUCT_END, generate_structs())


if __name__ == '__main__':
    generate_cython()
