from pathlib import Path
import os
from pprint import pprint
import re
import sys
from textwrap import indent

import pyparsing as pp
from pyparsing import (
    Suppress, Word, alphas, alphanums, nums, Optional, Group, ZeroOrMore, empty, restOfLine,
    cStyleComment,
)


HEADER_DIR = (Path(__file__).parent / '../../include/visky').resolve()
CYTHON_OUTPUT = (Path(__file__).parent / 'visky/cyvisky.pxd').resolve()

ENUM_START = '# ENUM START'
ENUM_END = '# ENUM END'
STRUCT_START = '# STRUCT START'
STRUCT_END = '# STRUCT END'


# File explorer and manipulation
# -------------------------------------------------------------------------------------------------

def iter_header_files():
    for h in sorted(HEADER_DIR.glob('*.h')):
        yield h


def read_file(filename):
    text = filename.read_text()
    return _remove_comments(text)


def insert_into_file(filename, start, end, insert):
    text = filename.read_text()
    i0 = text.index(start)
    i1 = text.index(end)
    out = text[:i0 + len(start) + 1]
    out += indent(insert, '    ')
    out += text[i1 - 5:]
    filename.write_text(out)


def _remove_comments(text):
    return '\n'.join([l.split('//')[0] for l in text.splitlines()])


# C header parsing
# -------------------------------------------------------------------------------------------------

# def _parse_define(text):
#     macro = Suppress("#define") + Word(alphas + "_", alphanums + "_") \
#         .setResultsName("macro") + empty + restOfLine.setResultsName("value")
#     for item, start, stop in macro.scanString(text):
#         print(item)


def parse_defines(text):
    defines = re.findall(
        r"#define (C[A-Z\_0-9]+)\s+([^\n]+)", text, re.MULTILINE)
    defines = dict(defines)
    defines = {k: v.replace('(', '').replace(')', '')
               for k, v in defines.items()}
    for k, v in defines.items():
        if v.isdigit():
            defines[k] = int(v)
    for k, v in defines.items():
        if isinstance(v, str) and '+' not in v:
            defines[k] = defines[v]
    for k, v in defines.items():
        if isinstance(v, str) and '+' in v:
            defines[k] = defines[v.split(' + ')[0]] + \
                defines[v.split(' + ')[1]]
    return defines


COLOR_CONSTANTS = parse_defines(read_file(HEADER_DIR / 'colormaps.h'))

_STRUCT_NAMES = ('VkyMouse', 'VkyKeyboard', 'VkyPick')


def _parse_enum(text):
    enums = {}
    # syntax we don't want to see in the final parse tree
    LBRACE, RBRACE, EQ, COMMA, SEMICOLON = map(Suppress, "{}=,;")
    _enum = Suppress("typedef enum")
    identifier = Word(alphanums + "_+-")

    enumValue = Group(
        identifier("name") +
        Optional(EQ + identifier("value")) +
        Optional(COMMA) +
        Optional(Suppress(cStyleComment)))

    enumList = Group(enumValue + ZeroOrMore(enumValue))
    enum = _enum + LBRACE + \
        enumList("names") + RBRACE + identifier("enum") + SEMICOLON

    for item, start, stop in enum.scanString(text):
        if item.enum == 'VkyConstantName':
            continue
        l = []
        for i, entry in enumerate(item.names):
            if entry.value.isdigit():
                entry.value = int(entry.value)
            elif not entry.value:
                entry.value = i
            elif entry.value in COLOR_CONSTANTS:
                entry.value = COLOR_CONSTANTS[entry.value]
            elif entry.value in ('false', 'true'):
                entry.value = entry.value.capitalize()
            l.append((entry.name, entry.value))
        enums[item.enum] = l
    return enums


def _gen_enum(enums):
    out = ''
    for name, l in enums.items():
        out += f'ctypedef enum {name}:\n'
        for identifier, value in l:
            out += f'    {identifier} = {value}\n'
        out += '\n'
    return out


def _parse_struct(text):
    structs = {}
    # syntax we don't want to see in the final parse tree
    LBRACE, RBRACE, COMMA, SEMICOLON = map(Suppress, "{},;")
    _struct = Suppress("struct")
    dtype = Word(alphanums + "_*")
    identifier = Word(alphanums + "_")
    structDecl = Group(dtype("dtype") + identifier("name") + SEMICOLON)
    structList = Group(structDecl + ZeroOrMore(structDecl))
    struct = _struct + identifier("struct_name") + LBRACE + \
        structList("names") + RBRACE + SEMICOLON

    for item, start, stop in struct.scanString(text):
        l = []
        for i, entry in enumerate(item.names):
            l.append((entry.dtype, entry.name))
        structs[item.struct_name] = l
    return structs


def _gen_struct(structs):
    out = ''
    for name, l in structs.items():
        if 'Axes' in name or 'Colorbar' in name:
            continue
        if name.endswith('Params') or name in _STRUCT_NAMES:
            out += f'ctypedef struct {name}:\n'
            for dtype, identifier in l:
                out += f'    {dtype} {identifier}\n'
            out += '\n'
    return out


if __name__ == '__main__':
    enums_to_insert = ''
    structs_to_insert = ''

    for filename in iter_header_files():
        text = read_file(filename)

        # Parse the enums
        enums = _parse_enum(text)
        # Generate the Cython enum definitions
        generated = _gen_enum(enums)
        if generated:
            enums_to_insert += f'# from file: {filename.name}\n\n{generated}'

        # Parse the structs
        if filename.name in ('visuals.h', 'app.h', 'gui.h', 'scene.h'):
            structs = _parse_struct(text)
            # Generate the Cython enum definitions
            generated = _gen_struct(structs)
            if generated:
                structs_to_insert += f'# from file: {filename.name}\n\n{generated}'

    # Insert into the Cython file
    insert_into_file(
        CYTHON_OUTPUT, ENUM_START, ENUM_END, enums_to_insert)
    insert_into_file(
        CYTHON_OUTPUT, STRUCT_START, STRUCT_END, structs_to_insert)
