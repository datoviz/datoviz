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

from tqdm import tqdm
import pyparsing as pp
from pyparsing import (
    Suppress, Word, alphas, alphanums, nums, Optional, Group, ZeroOrMore, empty, restOfLine,
    Keyword, cStyleComment, Empty, Literal
)


# Constants
# -------------------------------------------------------------------------------------------------

ROOT_DIR = Path(__file__).parent.parent
HEADER_DIR = (ROOT_DIR / 'include').resolve()
CACHE_PATH = ROOT_DIR / 'tools/headers.json'
EXCLUDE_DEFINES = (
    'DVZ_COLOR_CVEC4',
    'DvzColor',
    'DvzAlpha',
    'DVZ_ALPHA_MAX',
    'DVZ_FORMAT_COLOR',
)

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


def _resolve_defines(defines, ctx=None):
    for _ in range(3):
        for k, v in defines.items():
            if isinstance(v, str):
                if v in defines:
                    defines[k] = defines[v]

    ctx = (ctx or {}).copy()
    ctx.update(defines.copy())
    for k, v in defines.items():
        if isinstance(v, str):
            try:
                defines[k] = eval(v, ctx)
                ctx.update(defines.copy())
            except Exception as e:
                # print(f"Error parsing {k}={v}: {e}")
                defines[k] = None
            # print(k, v, defines[k])
    return {k: v for (k, v) in defines.items() if v}


# File explorer and manipulation
# -------------------------------------------------------------------------------------------------

def iter_header_files():
    for h in sorted(HEADER_DIR.glob('*.h')):
        yield h
    # for h in sorted(HEADER_DIR.glob('*/*.h')):
    #     yield h


def count_header_files():
    return len(list(iter_header_files()))


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
    _backslash = Optional(Suppress("\\"))

    identifier = Word(alphanums + "_")
    value = Word(alphanums + "._+- ()")

    # defineName = identifier("name") + Optional(Suppress(cStyleComment))
    define = _define + identifier("name") + _backslash + value("value")
    for item, start, stop in define.scanString(text):
        if item.name not in EXCLUDE_DEFINES:
            defines[item.name] = _parse_value(item.value)
    return defines


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
    unsigned = Keyword("unsigned")
    dtype = Word(alphanums + "_*")
    identifier = Word(alphanums + "_")
    array = LBRACKET + identifier("array_name") + RBRACKET
    structDecl = Group(
        Optional(const("const")) +
        Optional(unsigned("unsigned")) +
        dtype("dtype") +
        identifier("name") +
        Optional(array("array")) +
        SEMICOLON +
        Optional(cStyleComment("desc")))
    structList = Group(structDecl + ZeroOrMore(structDecl))
    struct = _struct('struct') + identifier("struct_name") + LBRACE + \
        structList("names") + RBRACE

    for item, start, stop in struct.scanString(text):
        l = []
        for i, entry in enumerate(item.names):
            b = Bunch(
                dtype=entry.dtype,
                name=entry.name,
            )
            if entry.unsigned:
                b.unsigned = True
            if entry.const:
                b.const = entry.const
            if entry.array_name:
                b.count = entry.array_name
            if entry.desc:
                b.desc = entry.desc[2:-2].strip()
            l.append(b)
        structs[item.struct_name] = Bunch(
            name=item.struct_name, type=item.struct, fields=l)
    return structs


def parse_functions(text):
    funcs = {}
    const = Keyword("const")
    unsigned = Keyword("unsigned")
    static = Keyword("static")
    inline = Keyword("inline")
    dtype = Word(alphanums + "_*")
    identifier = Word(alphanums, alphanums + "_")
    argDecl = Group(
        Optional(const("const")) +
        Optional(unsigned("unsigned")) +
        dtype("dtype") +
        Optional(identifier("name")
                 ) + Optional(COMMA))
    args = Group(ZeroOrMore(argDecl))
    # NOTE: make DVZ_EXPORT mandatory to avoid parsing non-functions such as DvzErrorCallback
    # in datoviz_macros.h
    func = (Suppress("DVZ_EXPORT")) + \
        Optional(Suppress("DVZ_INLINE"))
    signature = Optional(static("static")) + \
        Optional(inline("inline")) + \
        Optional(const("const")) + \
        dtype("out") + \
        identifier("name") + \
        LPAR + args("args") + RPAR + \
        Optional(SEMICOLON)
    func = cStyleComment("docstring") + func + \
        signature("signature")
    for item, start, stop in func.scanString(text):
        args = []
        # Detect if there is are out params.
        out_params = re.findall(r'@param\[out\]\s+(\w+)\s', item.docstring)
        for i, entry in enumerate(item.args):
            b = Bunch(
                dtype=entry.dtype,
                name=entry.name)
            if entry.const:
                b.const = entry.const
            if entry.name in out_params:
                b.out = True
                if " (array) " in item.docstring:
                    b.out_type = "array"
            args.append(b)
        funcs[item.name] = Bunch(
            name=item.name,
            args=args,
            docstring=item.docstring,
        )
        if item.out:
            funcs[item.name].returns = item.out
    return funcs


def parse_headers():
    headers = {}
    for filename in tqdm(
            iter_header_files(), total=count_header_files(), desc="Parsing C headers"):
        # print(f"Reading {filename}...")
        text = read_file(filename)

        headers[filename.name] = {
            'defines': parse_defines(text),
            'enums': parse_enums(text),
            'structs': parse_structs(text),
            'functions': parse_functions(text),
        }

    ctx = {}
    for filename in iter_header_files():
        d = headers[filename.name]
        for v in d['enums'].values():
            ctx.update(dict(v['values']))

    # HACK: avoid reference to Python type object "float"
    ctx['float'] = 'float'

    for filename in iter_header_files():
        d = headers[filename.name]
        d['defines'] = _resolve_defines(d['defines'], ctx)

    with open(CACHE_PATH, 'w') as f:
        json.dump(headers, f, indent=1)
    print(f"Saved {CACHE_PATH}.")


# Object getter
# -------------------------------------------------------------------------------------------------

@lru_cache
def load_headers():
    if not Path(CACHE_PATH).exists():
        parse_headers()
    with open(CACHE_PATH, 'r') as f:
        return json.load(f)


def _get(name):
    for fn, d in load_headers().items():
        for t in d:
            for f, v in d[t].items():
                if f == name:
                    return v


@lru_cache
def get_var(name):
    v = _get(name)
    if v is None:
        print(f"Variable {name} not found in headers.json.")
    #     parse_headers()
    #     load_headers.cache_clear()
    #     return _get(name)
    # else:
    return v


def iter_vars(vtype):
    headers = load_headers()
    for fn, d in load_headers().items():
        yield from d[vtype].items()


if __name__ == '__main__':
    parse_headers()
    # print(get_var("DvzAutorun"))
