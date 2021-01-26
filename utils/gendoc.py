from functools import lru_cache
import itertools
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


ROOT_DIR = Path(__file__).parent.parent.resolve()
HEADER_DIR = ROOT_DIR / 'include/visky'
INTERNAL_HEADER_DIR =  ROOT_DIR / 'src'
EXTERNAL_HEADER_DIR = ROOT_DIR / 'external'
API_OUTPUT = ROOT_DIR / 'docs/api.md'
HEADER_FILES = (
    'app.h', 'array.h', 'vklite.h', 'context.h', 'canvas.h', 'colormaps.h',
    'panel.h', 'visuals.h', 'scene.h')
ICONS = {
    'in': ':octicons-arrow-right-16:',
    'out': ':octicons-arrow-left-16:',
}
MAX_LINE_LENGTH = 76


# File explorer and manipulation
# -------------------------------------------------------------------------------------------------

def iter_header_files():
    for h in sorted(HEADER_DIR.glob('*.h')):
        yield h
    for h in sorted(INTERNAL_HEADER_DIR.glob('*.h')):
        yield h


def read_file(filename):
    text = filename.read_text()
    return _remove_comments(text)


def insert_text(text, i, n, insert):
    out = text[:i]
    out += insert
    out += text[i + n:]
    return out


def _remove_comments(text):
    return '\n'.join([l.split('//')[0] if not l.startswith(' *') else l for l in text.splitlines()])


def grouper(n, iterable):
    it = iter(iterable)
    while True:
        chunk_it = itertools.islice(it, n)
        try:
            first_el = next(chunk_it)
        except StopIteration:
            return
        yield itertools.chain((first_el,), chunk_it)


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


def _parse_funcs(text, is_output=False):
    if is_output:
        text = text[text.index(FUNCTION_START):text.index(FUNCTION_END)]
    funcs = {}
    # syntax we don't want to see in the final parse tree
    LPAR, RPAR, LBRACE, RBRACE, COMMA, SEMICOLON = map(Suppress, "(){},;")
    const = Keyword("const")
    static = Keyword("static")
    inline = Keyword("inline")
    dtype = Word(alphanums + "_*")
    identifier = Word(alphanums + "_")
    argDecl = Group(
        Optional(const("const")) +
        dtype("dtype") +
        Optional(identifier("name")
                 ) + Optional(COMMA))
    args = Group(ZeroOrMore(argDecl))
    if not is_output:
        func = Optional(Suppress("VKY_EXPORT"))
    else:
        func = Empty()
    signature = Optional(static("static")) + \
        Optional(inline("inline")) + \
        dtype("out") + \
        identifier("name") + \
        LPAR + args("args") + RPAR + \
        Optional(SEMICOLON)
    func = cStyleComment("docstring") + func + \
        signature("signature")
    for item, start, stop in func.scanString(text):
        args = []
        # for i, entry in enumerate(item.args):
        #     args.append((entry.const, entry.dtype, entry.name))
        funcs[item.name] = item
    return funcs



def _gen_func_doc(name, func):
    # Generate the function documentation
    out = func.out
    args = func.args
    docstring = func.docstring
    docstring = docstring if docstring.startswith('/**\n') else ''
    docstring = '\n'.join(_[3:] for _ in docstring.splitlines())
    docstring = docstring.strip()

    def _arg_type(n):
        for arg in args:
            if arg.name == n:
                return arg.dtype

    # Extract the argument docstring from the whole docstring, @param keywords etc.
    lines = docstring.splitlines()
    params = []
    returns = []
    description = []
    for l in lines:
        # WARNING: @param should be quite short
        # WARNING: 1 line per param
        if l.startswith('@param'):
            params.append(l[6:].strip())
        elif l.startswith('@returns'):
            returns.append(l[8:].strip())
        else:
            # if l == '':
            #     l = '\n\n'
            description.append(l)
    description = '\n'.join(description).strip()

    # Signature
    args_s = ', '.join(
        f"{'const ' if args.const else ''}{arg.dtype} {arg.name}" for arg in args)
    # Split long lines
    if len(out) + len(name) + len(args_s) >= MAX_LINE_LENGTH:
        if len(args_s) >= MAX_LINE_LENGTH - 4:
            args_s = '\n' + args_s
            args_s = ',\n'.join(indent(', '.join(_), '    ') for _ in grouper(3, args_s.split(', ')))
        else:
            args_s = '\n' + indent(args_s, '    ')

    signature = f'```c\n{out} {name}({args_s});\n```'
    signature = f'=== "C"\n{indent(signature, prefix="    ")}'

    # Parameters table
    params_s = ""
    if params:
        params_s += "| Parameter | Type | Description |\n"
        params_s += "| ---- | ---- | ---- |\n"
        for param in params:
            if param[0] != '[':
                param = '[in]' + param
            i = param.index('[')
            j = param.index(']')
            inout = param[i + 1:j]
            assert inout in ('in', 'out')
            desc = param[j+1:].strip()
            argname = desc[:desc.index(' ')].strip()
            desc = desc[len(argname):].strip()
            icon = ICONS[inout]
            dtype = _arg_type(argname)
            params_s += f"| {icon} `{argname}` | `{dtype}` | {desc} |\n"
    for ret in returns:
        desc = ret[ret.index(' ') + 1:]
        params_s += f"| {ICONS['out']} `returns` | `{out}` | {desc} |\n"

    return f"### `{name}()`\n\n{signature}\n\n{params_s}\n{description}\n"


def _camel_to_snake(name):
    name = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', name).lower()


def _parse_markdown(api_text):
    # Parse the api.md file and extracts all function definitions
    r = re.compile(r'#+\s+`?(\w+)\(\)`?')  # ex: "### `vkl_canvas()`"
    functions = r.finditer(api_text)
    return functions


ITEM_HEADER = re.compile(r'^#+\s+', flags=re.MULTILINE)


if __name__ == '__main__':
    # enums_to_insert = ''
    # structs_to_insert = ''
    # funcs_to_insert = ''

    # Parse all functions.
    all_funcs = {}
    for filename in iter_header_files():
        if filename.name not in HEADER_FILES:
            continue
        text = read_file(filename)
        # Parse the functions
        f = _parse_funcs(text)
        print(f"{len(f):02d} functions found in {filename}")
        for name in f.keys():
            print(f'### `{name}()`\n')
        all_funcs.update(f)

    # TODO: enums and structs

    # Find where to insert the documentation strings in the output file.
    api_text = API_OUTPUT.read_text()
    funcs_to_output = _parse_markdown(api_text)

    # Output text, copy of the original text
    for m in funcs_to_output:
        name = m.group(1)
        # Find the position of the function in the current text
        header = f'### `{name}()`'
        assert header in api_text, header
        i = api_text.index(header)
        # All text after the current function header
        rem = api_text[i + len(header):]
        assert rem
        # We need to go up to where?
        r = ITEM_HEADER.search(rem)
        if r:
            n = r.start(0)
        else:
            n = len(rem)
        n += len(header)
        assert n > 0
        # Docstring to insert.
        insert = _gen_func_doc(name, all_funcs[name])
        api_text = insert_text(api_text, i, n - 1, insert)

    api_text = api_text.strip() + '\n'

    # print(api_text)
    API_OUTPUT.write_text(api_text)
