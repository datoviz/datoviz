from functools import lru_cache
from pathlib import Path
import os
from pprint import pprint
import re
import sys
from textwrap import indent

import pyparsing as pp
from pyparsing import (
    Suppress, Word, alphas, alphanums, nums, Optional, Group, ZeroOrMore, empty, restOfLine,
    Keyword, cStyleComment, Empty, Literal
)


HEADER_DIR = (Path(__file__).parent / '../../../include/visky').resolve()
INTERNAL_HEADER_DIR = (Path(__file__).parent / '../../../src').resolve()
EXTERNAL_HEADER_DIR = HEADER_DIR / '../../external'
CYTHON_OUTPUT = (Path(__file__).parent / '../visky/cyvisky.pxd').resolve()
HEADER_FILES = (
    'vklite.h', 'context.h', 'canvas.h', 'keycode.h',
    'panel.h', 'visuals.h', 'scene.h')
STRUCTS = (
    'VklViewport',
    'VklMouseButtonEvent',
    'VklMouseMoveEvent',
    'VklMouseWheelEvent',
    'VklMouseDragEvent',
    'VklMouseClickEvent',
    'VklKeyEvent',
    'VklFrameEvent',
    'VklTimerEvent',
    'VklScreencastEvent',
    'VklEvent',
    'VklEventUnion',
)

ENUM_START = '# ENUM START'
ENUM_END = '# ENUM END'
STRUCT_START = '# STRUCT START'
STRUCT_END = '# STRUCT END'
UNION_START = '# UNION START'
UNION_END = '# UNION END'
FUNCTION_START = '# FUNCTION START'
FUNCTION_END = '# FUNCTION END'


# File explorer and manipulation
# -------------------------------------------------------------------------------------------------

def iter_header_files():
    for h in sorted(HEADER_DIR.glob('*.h')):
        yield h
    for h in sorted(INTERNAL_HEADER_DIR.glob('*.h')):
        yield h
    # for h in (INTERNAL_HEADER_DIR / 'log.h',):
    #     yield h


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


# _STRUCT_NAMES = ('VklPrivateEvent', 'VklEvent')


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
        l = []
        for i, entry in enumerate(item.names):
            if entry.value.isdigit():
                entry.value = int(entry.value)
            elif not entry.value:
                entry.value = i
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
    _struct = Literal("struct") ^ Literal("union")
    dtype = Word(alphanums + "_*")
    identifier = Word(alphanums + "_[]")
    structDecl = Group(dtype("dtype") + identifier("name") + SEMICOLON)
    structList = Group(structDecl + ZeroOrMore(structDecl))
    struct = _struct('struct') + identifier("struct_name") + LBRACE + \
        structList("names") + RBRACE + SEMICOLON

    for item, start, stop in struct.scanString(text):
        l = []
        for i, entry in enumerate(item.names):
            l.append((entry.dtype, entry.name))
        structs[item.struct_name] = (item.struct, l)
    return structs


def _gen_struct(structs):
    out = ''
    for name, (struct, l) in structs.items():
        if name in STRUCTS:
            out += f'ctypedef {struct} {name}:\n'
            for dtype, identifier in l:
                if dtype == 'bool':
                    dtype = 'bint'
                out += f'    {dtype} {identifier}\n'
            out += '\n'
    return out


# def _parse_union(text):
#     unions = {}
#     # syntax we don't want to see in the final parse tree
#     LBRACE, RBRACE, COMMA, SEMICOLON = map(Suppress, "{},;")
#     _union = Suppress("union")
#     dtype = Word(alphanums + "_*")
#     identifier = Word(alphanums + "_")
#     unionDecl = Group(dtype("dtype") + identifier("name") + SEMICOLON)
#     unionList = Group(unionDecl + ZeroOrMore(unionDecl))
#     union = _union + identifier("union_name") + LBRACE + \
#         unionList("names") + RBRACE + SEMICOLON

#     for item, start, stop in union.scanString(text):
#         l = []
#         for i, entry in enumerate(item.names):
#             l.append((entry.dtype, entry.name))
#         unions[item.union_name] = l
#     return unions


# def _gen_union(unions):
#     out = ''
#     for name, l in unions.items():
#         out += f'ctypedef union {name}:\n'
#         for dtype, identifier in l:
#             out += f'    {dtype} {identifier}\n'
#         out += '\n'
#     return out


def _parse_func(text, is_output=False):
    if is_output:
        text = text[text.index(FUNCTION_START):text.index(FUNCTION_END)]
    funcs = {}
    # syntax we don't want to see in the final parse tree
    LPAR, RPAR, LBRACE, RBRACE, COMMA, SEMICOLON = map(Suppress, "(){},;")
    const = Keyword("const")
    dtype = Word(alphanums + "_*")
    identifier = Word(alphanums + "_")
    argDecl = Group(
        Optional(const("const")) +
        dtype("dtype") +
        Optional(identifier("name")
                 ) + Optional(COMMA))
    args = Group(ZeroOrMore(argDecl))
    if not is_output:
        func = Suppress("VKY_EXPORT")
    else:
        func = Empty()
    func = func + \
        dtype("out") + \
        identifier("name") + \
        LPAR + args("args") + RPAR + \
        Optional(SEMICOLON)

    for item, start, stop in func.scanString(text):
        args = []
        for i, entry in enumerate(item.args):
            args.append((entry.const, entry.dtype, entry.name))
        funcs[item.name] = (item.out, tuple(args))
    return funcs


@lru_cache(maxsize=64)
def _camel_to_snake(name):
    name = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', name).lower()


def _gen_cython_func(name, func):
    out, args = func
    args_s = []
    for const, dtype, argname in args:
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
            elif 'Vkl' in dtype:
                argname = _camel_to_snake(
                    dtype.replace('Vkl', '')).replace('*', '')
            else:
                raise ValueError(dtype)
        if const:
            dtype = "const " + dtype
        if dtype == 'bool':
            dtype = 'bint'
        args_s.append(f'{dtype} {argname}')
    args = ', '.join(args_s)
    return f'{out} {name}({args})'


if __name__ == '__main__':
    enums_to_insert = ''
    structs_to_insert = ''
    # unions_to_insert = ''
    funcs_to_insert = ''

    # Parse already-defined functions in the pxd
    already_defined_funcs = _parse_func(
        read_file(CYTHON_OUTPUT), is_output=True)

    for filename in iter_header_files():
        if filename.name not in HEADER_FILES:
            continue
        text = read_file(filename)

        # Parse the enums
        enums = _parse_enum(text)
        # Generate the Cython enum definitions
        generated = _gen_enum(enums)
        if generated:
            enums_to_insert += f'# from file: {filename.name}\n\n{generated}'

        # Parse the structs
        structs = _parse_struct(text)
        # Generate the Cython enum definitions
        generated = _gen_struct(structs)
        if generated:
            structs_to_insert += f'# from file: {filename.name}\n\n{generated}'

        # # Parse the unions
        # unions = _parse_union(text)
        # # Generate the Cython enum definitions
        # generated = _gen_union(unions)
        # if generated:
        #     unions_to_insert += f'# from file: {filename.name}\n\n{generated}'

        # Parse the functions
        funcs = _parse_func(text)
        h = f'# from file: {filename.name}\n'
        funcs_to_insert += h
        generated = ''
        for name, func in funcs.items():
            existing = already_defined_funcs.pop(name, None)
            if not existing:
                continue
            generated = _gen_cython_func(name, func)
            funcs_to_insert += generated + '\n'
        if not generated:
            funcs_to_insert = funcs_to_insert[:-len(h)]
        else:
            funcs_to_insert += '\n'

    if already_defined_funcs.keys():
        print(already_defined_funcs)
        raise RuntimeError(
            "Some Cython function bindings are missing, check gencython.py")

    # Insert into the Cython file
    insert_into_file(
        CYTHON_OUTPUT, ENUM_START, ENUM_END, enums_to_insert)
    insert_into_file(
        CYTHON_OUTPUT, STRUCT_START, STRUCT_END, structs_to_insert)
    # insert_into_file(
    #     CYTHON_OUTPUT, UNION_START, UNION_END, unions_to_insert)
    insert_into_file(
        CYTHON_OUTPUT, FUNCTION_START, FUNCTION_END, funcs_to_insert)
