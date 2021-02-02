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
HEADER_DIR = ROOT_DIR / 'include/datoviz'
INTERNAL_HEADER_DIR = ROOT_DIR / 'src'
EXTERNAL_HEADER_DIR = ROOT_DIR / 'external'
API_OUTPUT = ROOT_DIR / 'docs/api.md'
# HEADER_FILES = (
#     'app.h', 'array.h', 'vklite.h', 'context.h', 'canvas.h', 'colormaps.h',
#     'panel.h', 'visuals.h', 'scene.h')
ICONS = {
    'in': ':octicons-arrow-right-16:',
    'out': ':octicons-arrow-left-16:',
}
ITEM_HEADER = re.compile(r'^#+\s+', flags=re.MULTILINE)
GRAPHICS_CODE = re.compile(r'```c\n([a-zA-Z0-9\_]+)\n```')
MAX_LINE_LENGTH = 76
INDEX_LINK = re.compile(r'\]\(docs/([^\)]+\.md)\)')
INSERT_CODE = re.compile(r'<!-- CODE_([^ ]+) ([^ ]+) -->')
INSERT_IMAGE = re.compile(r'<!-- IMAGE ([^ ]+) -->')



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


def insert_text(text, i, insert):
    return text[:i] + insert + text[i:]


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
        r"#define ([A-Z\_0-9]+)\s+([a-zA-Z\_0-9\(\) \+\-]+)", text, re.MULTILINE)
    defines = dict(defines)
    defines = {k: v.replace('(', '').replace(')', '')
               for k, v in defines.items()}
    defines = {k: v.strip()
               for k, v in defines.items() if k.startswith('CMAP') or k.startswith('CPAL')}
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


def _parse_enum(text, defines=None):
    defines = defines or {}
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
            elif entry.value in defines:
                entry.value = defines[entry.value]
            l.append((entry.name, entry.value))
        enums[item.enum] = l
    return enums


def _gen_enum(name, values):
    out = ''
    out += '| Name | Value |\n| ---- | ---- |\n'
    for identifier, value in values:
        out += f'| `{identifier}` | {value} |\n'
    return out + '\n'


def _parse_struct(text):
    structs = {}
    # syntax we don't want to see in the final parse tree
    LBRACE, RBRACE, COMMA, SEMICOLON = map(Suppress, "{},;")
    _struct = Literal("struct") ^ Literal("union")
    dtype = Word(alphanums + "_*")
    identifier = Word(alphanums + "_[]")
    structDecl = Group(dtype("dtype") + identifier("name") +
                       SEMICOLON + Optional(cStyleComment("desc")))
    structList = Group(structDecl + ZeroOrMore(structDecl))
    struct = _struct('struct') + identifier("struct_name") + LBRACE + \
        structList("names") + RBRACE + SEMICOLON
    for item, start, stop in struct.scanString(text):
        l = []
        for i, entry in enumerate(item.names):
            l.append((entry.dtype, entry.name, entry.desc))
        structs[item.struct_name] = (item.struct, l)
    return structs


def _gen_struct(name, fields):
    out = f'#### {name}\n\n| Field | Type | Description |\n| ---- | ---- | ---- |\n'
    for dtype, field, desc in fields[1]:
        desc = desc.replace('/*', '').replace('*/', '').strip()
        out += f'| `{field}` | `{dtype}` | {desc} |\n'
    return out.strip() + '\n\n'


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
        func = Optional(Suppress("DVZ_EXPORT")) + \
            Optional(Suppress("DVZ_INLINE"))
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
    brief = description[0] if description else ''
    description = '\n'.join(description[1:]).strip()

    # Signature
    args_s = ', '.join(
        f"{'const ' if args.const else ''}{arg.dtype} {arg.name}" for arg in args)
    # Split long lines
    if len(out) + len(name) + len(args_s) >= MAX_LINE_LENGTH:
        if len(args_s) >= MAX_LINE_LENGTH - 4:
            args_s = '\n' + args_s
            args_s = ',\n'.join(indent(', '.join(_), '    ')
                                for _ in grouper(3, args_s.split(', ')))
        else:
            args_s = '\n' + indent(args_s, '    ')

    signature = f'```c\n{out} {name}({args_s});\n```'
    # signature = f'=== "C"\n{indent(signature, prefix="    ")}'

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

    return f"{brief}\n\n{signature}\n\n{params_s}\n{description}\n"


def _camel_to_snake(name):
    name = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', name).lower()


def _parse_markdown(api_text):
    # Parse the api.md file and extracts all function definitions
    r = re.compile(r'#+\s+`?(\w+)\(\)`?')  # ex: "### `dvz_canvas()`"
    functions = r.finditer(api_text)
    return functions


def _parse_markdown_enums(api_text):
    # Parse the enums.md file and extracts all enums definitions
    r = re.compile(r'#{2,}\s+`(\w+)`')  # ex: "### `DvzMyEnum`"
    enums = r.finditer(api_text)
    return enums


def parse_headers():
    all_funcs = {}
    all_enums = {}
    all_structs = {}
    for filename in iter_header_files():
        text = read_file(filename)

        funcs = _parse_funcs(text)
        defines = parse_defines(text)
        enums = _parse_enum(text, defines)
        structs = _parse_struct(text)

        all_funcs.update(funcs)
        all_enums.update(enums)
        all_structs.update(structs)
    return {
        'functions': all_funcs,
        'enums': all_enums,
        'structs': all_structs,
    }


def insert_functions_doc(markdown, config):
    funcs_to_output = _parse_markdown(markdown)
    out = markdown
    # Output text, copy of the original text
    for m in funcs_to_output:
        name = m.group(1)
        # Find the position of the function in the current text
        r = re.compile(r'^#+\s+`%s\(\)`' % name, flags=re.MULTILINE)
        m2 = r.search(out)
        i = m2.end(0)
        insert = _gen_func_doc(name, config['gendoc']['functions'][name])
        out = insert_text(out, i, f'\n\n{insert}\n\n')
    out = out.strip() + '\n'
    return out


def insert_enums_doc(markdown, config):
    enums_to_output = _parse_markdown_enums(markdown)
    out = markdown
    # Output text, copy of the original text
    for m in enums_to_output:
        name = m.group(1)
        # Find the position of the function in the current text
        r = re.compile(r'^#+\s+`%s`' % name, flags=re.MULTILINE)
        m2 = r.search(out)
        i = m2.end(0)
        insert = _gen_enum(name, config['gendoc']['enums'][name])
        out = insert_text(out, i, f'\n\n{insert}\n\n')
    out = out.strip() + '\n'
    return out


def insert_graphics_doc(markdown, config):
    def _sub_graphics_code(m):
        n = m.group(1)
        out = ''
        for h in ('Item', 'Vertex', 'Params'):
            s = f'DvzGraphics{n}{h}'
            struct = config['gendoc']['structs'].get(s, None)
            if struct:
                out += _gen_struct(f'{h} structure: `{s}`', struct)
        return out

    return GRAPHICS_CODE.sub(_sub_graphics_code, markdown)


def process_index_page(markdown, config):
    index = (ROOT_DIR / 'README.md').read_text()
    index = INDEX_LINK.sub(r'](\1)', index)
    return index


def process_code_image(markdown, config):
    def _repl_code(m):
        path = ROOT_DIR / m.group(2)
        lang = m.group(1).lower()
        assert path.exists(), path
        code = path.read_text()
        if lang == 'python':
            comment = '#'
        elif lang == 'c':
            comment = '//'
        elif lang == 'glsl':
            comment = '//'
        else:
            raise NotImplementedError("unknown language")
        return f'```{lang}\n{comment} code from `{m.group(2)}`:\n\n{code}\n```\n'

    def _repl_image(m):
        rel_path = m.group(1)
        return f'![]({rel_path})'

    markdown = INSERT_CODE.sub(_repl_code, markdown)
    markdown = INSERT_IMAGE.sub(_repl_image, markdown)
    return markdown
