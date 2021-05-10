# Parse the C headers.

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

import pyparsing as pp
from pyparsing import (
    Suppress, Word, alphas, alphanums, nums, Optional, Group, ZeroOrMore, empty, restOfLine,
    Keyword, cStyleComment, Empty, Literal
)


# Constants
# -------------------------------------------------------------------------------------------------

ROOT_DIR = Path(__file__).parent.parent
HEADER_DIR = (ROOT_DIR / 'include/datoviz').resolve()
INTERNAL_HEADER_DIR = (ROOT_DIR / 'src').resolve()
EXTERNAL_HEADER_DIR = ROOT_DIR / 'external'
CACHE_PATH = ROOT_DIR / 'utils/headers.json'

LPAR, RPAR, LBRACE, RBRACE, LBRACKET, RBRACKET, COMMA, SEMICOLON, EQ, SPACE = \
    map(Suppress, "(){}[],;= ")


# Utils
# -------------------------------------------------------------------------------------------------

class Bunch(dict):
    def __init__(self, *args, **kwargs):
        super(Bunch, self).__init__(*args, **kwargs)
        self.__dict__ = self


def _parse_value(val, i=0):
    val = val.strip()
    if val.isdigit():
        val = int(val)
    elif not val:
        val = i
    elif val in ('false', 'true'):
        val = val.capitalize()
    return val


def _resolve_defines(defines):
    for _ in range(3):
        for k, v in defines.items():
            if isinstance(v, str):
                if v in defines:
                    defines[k] = defines[v]

    defines_cst = defines.copy()
    for k, v in defines.items():
        if isinstance(v, str):
            try:
                defines[k] = eval(v, defines_cst)
                defines_cst = defines.copy()
            except:
                defines[k] = None
            # print(k, v, defines[k])
    return {k: v for (k, v) in defines.items() if v}


# File explorer and manipulation
# -------------------------------------------------------------------------------------------------

def iter_header_files():
    for h in sorted(HEADER_DIR.glob('*.h')):
        yield h
    for h in sorted(INTERNAL_HEADER_DIR.glob('*.h')):
        yield h
    # for h in (INTERNAL_HEADER_DIR / 'log.h',):
    #     yield h


def remove_comments(text):
    return '\n'.join([l.split('//')[0] for l in text.splitlines()])


def read_file(filename):
    text = filename.read_text()
    return remove_comments(text)


# C header parsing
# -------------------------------------------------------------------------------------------------

def parse_defines(text):
    defines = {}
    # syntax we don't want to see in the final parse tree
    # LBRACE, RBRACE, EQ, COMMA, SEMICOLON, SPACE = map(Suppress, "{}=,; ")
    _define = Suppress("#define")
    identifier = Word(alphanums + "_")
    value = Word(alphanums + "_+-*/ ()")
    # defineName = identifier("name") + Optional(Suppress(cStyleComment))
    define = _define + identifier("name") + value("value")
    for item, start, stop in define.scanString(text):
        defines[item.name] = _parse_value(item.value)
    return _resolve_defines(defines)


def parse_enums(text):
    enums = {}
    # syntax we don't want to see in the final parse tree
    # LBRACE, RBRACE, EQ, COMMA, SEMICOLON = map(Suppress, "{}=,;")
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
            l.append((entry.name, _parse_value(entry.value, i=i)))
        enums[item.enum] = Bunch(name=item.enum, values=l)
    return enums


def parse_structs(text):
    structs = {}
    # LBRACE, RBRACE, LBRACKET, RBRACKET, COMMA, SEMICOLON = map(Suppress, "{}[],;")
    _struct = Literal("struct") ^ Literal("union")
    const = Keyword("const")
    dtype = Word(alphanums + "_*")
    identifier = Word(alphanums + "_")
    array = LBRACKET + identifier("array_name") + RBRACKET
    structDecl = Group(
        Optional(const("const")) +
        dtype("dtype") +
        identifier("name") +
        Optional(array("array")) +
        SEMICOLON)
    structList = Group(structDecl + ZeroOrMore(structDecl))
    struct = _struct('struct') + identifier("struct_name") + LBRACE + \
        structList("names") + RBRACE + SEMICOLON

    for item, start, stop in struct.scanString(text):
        l = []
        for i, entry in enumerate(item.names):
            b = Bunch(
                dtype=entry.dtype,
                name=entry.name,
            )
            if entry.const:
                b.const = entry.const
            if entry.array_name:
                b.count = entry.array_name
            l.append(b)
        structs[item.struct_name] = Bunch(name=item.struct_name, type=item.struct, args=l)
    return structs


def parse_functions(text):  # , is_output=False):
    # if is_output:
    #     text = text[text.index(FUNCTION_START):text.index(FUNCTION_END)]
    funcs = {}
    # LPAR, RPAR, LBRACE, RBRACE, COMMA, SEMICOLON = map(Suppress, "(){},;")
    const = Keyword("const")
    dtype = Word(alphanums + "_*")
    identifier = Word(alphanums + "_")
    argDecl = Group(
        Optional(const("const")) +
        dtype("dtype") +
        Optional(identifier("name")
                 ) + Optional(COMMA))
    args = Group(ZeroOrMore(argDecl))
    # if not is_output:
    func = Suppress("DVZ_EXPORT")
    # else:
    # func = Empty()
    func = func + \
        dtype("out") + \
        identifier("name") + \
        LPAR + args("args") + RPAR + \
        Optional(SEMICOLON)

    for item, start, stop in func.scanString(text):
        args = []
        for i, entry in enumerate(item.args):
            b = Bunch(
                dtype=entry.dtype,
                name=entry.name)
            if entry.const:
                b.const = entry.const
            args.append(b)
        funcs[item.name] = Bunch(
            name=item.name,
            args=args,
        )
        if item.out:
            funcs[item.name].returns = item.out
    return funcs


def parse_headers():
    headers = {}
    for filename in iter_header_files():
        print(f"Reading {filename}...")
        text = read_file(filename)

        headers[filename.name] = {
            'defines': parse_defines(text),
            'enums': parse_enums(text),
            'structs': parse_structs(text),
            'functions': parse_functions(text),
        }

    with open(CACHE_PATH, 'w') as f:
        json.dump(headers, f, indent=1, sort_keys=True)
    print(f"Saved {CACHE_PATH}.")


if __name__ == '__main__':
    parse_headers()
