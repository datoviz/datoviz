from pathlib import Path
import os
from pprint import pprint
import re
import sys
from textwrap import indent

import pyparsing as pp
from pyparsing import (
    Suppress, Word, alphas, alphanums, nums, Optional, Group, ZeroOrMore, empty, restOfLine,
)


HEADER_DIR = (Path(__file__).parent / '../../include/visky').resolve()
CYTHON_OUTPUT = (Path(__file__).parent / 'visky/cyvisky.pxd').resolve()

ENUM_START = '# ENUM START'
ENUM_END = '# ENUM END'


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


def _parse_enum(text):
    enums = {}
    # syntax we don't want to see in the final parse tree
    LBRACE, RBRACE, EQ, COMMA, SEMICOLON = map(Suppress, "{}=,;")
    _enum = Suppress("typedef enum")
    identifier = Word(alphanums + "_")
    enumValue = Group(identifier("name") + Optional(EQ + identifier("value")))
    enumList = Group(enumValue + ZeroOrMore(COMMA + enumValue))
    enum = _enum + LBRACE + \
        enumList("names") + Optional(COMMA) + RBRACE + \
        identifier("enum") + SEMICOLON

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


if __name__ == '__main__':
    to_insert = ''
    for filename in iter_header_files():
        # Parse the enums
        enums = _parse_enum(read_file(filename))
        # Generate the Cython enum definitions
        generated = _gen_enum(enums)
        if not generated:
            continue
        to_insert += f'# from file: {filename.name}\n\n{generated}'
    # Insert it into the Cython file
    insert_into_file(
        CYTHON_OUTPUT, ENUM_START, ENUM_END, to_insert)
