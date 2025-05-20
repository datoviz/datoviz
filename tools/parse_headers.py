# Parse the C headers.

# Imports
# -------------------------------------------------------------------------------------------------

import json
import re
from functools import lru_cache
from pathlib import Path

from pyparsing import (
    Group,
    Keyword,
    Literal,
    Optional,
    Suppress,
    Word,
    ZeroOrMore,
    alphanums,
    cStyleComment,
)
from tqdm import tqdm

# Constants
# -------------------------------------------------------------------------------------------------

ROOT_DIR = Path(__file__).parent.parent
HEADER_DIR = (ROOT_DIR / 'include').resolve()
CACHE_PATH = ROOT_DIR / 'build/headers.json'
EXCLUDE_DEFINES = (
    'DVZ_COLOR_CVEC4',
    'DvzColor',
    'DvzAlpha',
    'DVZ_ALPHA_MAX',
    'DVZ_FORMAT_COLOR',
)

LPAR, RPAR, LBRACE, RBRACE, LBRACKET, RBRACKET, COMMA, SEMICOLON, EQ, SPACE = map(
    Suppress, '(){}[],;= '
)


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
            except Exception:
                # print(f"Error parsing {k}={v}: {e}")
                defines[k] = None
            # print(k, v, defines[k])
    return {k: v for (k, v) in defines.items() if v}


def parse_doxygen_docstring(docstring):
    # Regular expression patterns for different components of the docstring
    param_pattern = re.compile(r'@param\s*(?:\[\w+\])?\s*(\w+)\s+([^\n]+)', re.DOTALL)
    returns_pattern = re.compile(r'@returns\s+([^\n]+)', re.DOTALL)

    # Extract all lines inside the comment block
    lines = re.findall(r'^\s*\*\s?(.*)', docstring, re.MULTILINE)

    # Join lines and separate into description and tagged sections
    description_lines = []
    for line in lines:
        line = line.strip()
        if line.startswith('* '):
            line = line[2:].strip()
        if line.startswith(('@param', '@returns')):
            break
        description_lines.append(line)

    description = '\n'.join(description_lines).strip()

    # Extract parameters
    params = []
    for param_match in param_pattern.finditer(docstring):
        param_name = param_match.group(1).strip()
        param_description = param_match.group(2).strip()
        params.append((param_name, param_description))

    # Extract return value
    returns_match = returns_pattern.search(docstring)
    returns = returns_match.group(1).strip() if returns_match else ''

    return {'description': description, 'params': params, 'returns': returns}


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

    _define = Suppress('#define')
    _backslash = Optional(Suppress('\\'))

    identifier = Word(alphanums + '_')
    value = Word(alphanums + '._+- ()')

    # defineName = identifier("name") + Optional(Suppress(cStyleComment))
    define = _define + identifier('name') + _backslash + value('value')
    for item, start, stop in define.scanString(text):
        if item.name not in EXCLUDE_DEFINES:
            defines[item.name] = _parse_value(item.value)
    return defines


def parse_enums(text):
    enums = {}
    # syntax we don't want to see in the final parse tree
    # LBRACE, RBRACE, EQ, COMMA, SEMICOLON = map(Suppress, "{}=,;")
    _enum = Suppress('typedef enum')
    identifier = Word(alphanums + '_+-')

    enumValue = Group(
        identifier('name')
        + Optional(EQ + identifier('value'))
        + Optional(COMMA)
        + Optional(Suppress(cStyleComment))
    )

    enumList = Group(enumValue + ZeroOrMore(enumValue))
    enum = _enum + LBRACE + enumList('names') + RBRACE + identifier('enum') + SEMICOLON

    for item, start, stop in enum.scanString(text):
        l = []
        for i, entry in enumerate(item.names):
            l.append((entry.name, _parse_value(entry.value, i=i)))
        enums[item.enum] = Bunch(name=item.enum, values=l)
    return enums


def parse_structs(text):
    structs = {}
    # LBRACE, RBRACE, LBRACKET, RBRACKET, COMMA, SEMICOLON = map(Suppress, "{}[],;")
    _struct = Literal('struct') ^ Literal('union')
    const = Keyword('const')
    unsigned = Keyword('unsigned')
    dtype = Word(alphanums + '_*')
    identifier = Word(alphanums + '_')
    array = LBRACKET + identifier('array_name') + RBRACKET
    structDecl = Group(
        Optional(const('const'))
        + Optional(unsigned('unsigned'))
        + dtype('dtype')
        + identifier('name')
        + Optional(array('array'))
        + SEMICOLON
        + Optional(cStyleComment('desc'))
    )
    structList = Group(structDecl + ZeroOrMore(structDecl))
    struct = _struct('struct') + identifier('struct_name') + LBRACE + structList('names') + RBRACE

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
        structs[item.struct_name] = Bunch(name=item.struct_name, type=item.struct, fields=l)
    return structs


def parse_functions(text):
    funcs = {}
    const = Keyword('const')
    unsigned = Keyword('unsigned')
    static = Keyword('static')
    inline = Keyword('inline')
    dtype = Word(alphanums + '_*')
    identifier = Word(alphanums, alphanums + '_')
    ellipsis = Literal('...')('ellipsis')
    argType = (
        Optional(const('const'))
        + Optional(unsigned('unsigned'))
        + dtype('dtype')
        + Optional(identifier('name'))
    )
    argDecl = Group(argType | ellipsis) + Optional(COMMA)
    args = Group(ZeroOrMore(argDecl))
    # NOTE: make DVZ_EXPORT mandatory to avoid parsing non-functions such as DvzErrorCallback
    # in datoviz_macros.h
    func = (Suppress('DVZ_EXPORT')) + Optional(Suppress('DVZ_INLINE'))
    signature = (
        Optional(static('static'))
        + Optional(inline('inline'))
        + Optional(const('const'))
        + dtype('out')
        + identifier('name')
        + LPAR
        + args('args')
        + RPAR
        + Optional(SEMICOLON)
    )
    func = cStyleComment('docstring') + func + signature('signature')
    for item, start, stop in func.scanString(text):
        # Doxygen parsing
        dox = parse_doxygen_docstring(item.docstring)

        args = []
        # Detect if there is are out params.
        out_params = re.findall(r'@param\[out\]\s+(\w+)\s', item.docstring)
        for i, entry in enumerate(item.args):
            b = Bunch(dtype=entry.dtype, name=entry.name)
            if entry.ellipsis:
                b.varargs = True
            if entry.const:
                b.const = entry.const
            if entry.name in out_params:
                b.out = True
                if ' (array) ' in item.docstring:
                    b.out_type = 'array'
            if len(dox.get('params', [])) > i:
                if dox['params'][i][0] != entry.name:
                    print(
                        f'Argument name mismatch for {item.name}: '
                        f'{dox["params"][i][0]} != {entry.name}'
                    )
                b['docstring'] = dox['params'][i][1]
            args.append(b)
        funcs[item.name] = Bunch(
            name=item.name,
            args=args,
            docstring=dox['description'],
        )
        if item.out and item.out != 'void':
            funcs[item.name].returns = {
                'dtype': item.out,
                'docstring': dox['returns'],
            }
    return funcs


def parse_headers():
    headers = {}
    for filename in tqdm(
        iter_header_files(), total=count_header_files(), desc='Parsing C headers'
    ):
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
    print(f'Saved {CACHE_PATH}.')


# Object getter
# -------------------------------------------------------------------------------------------------


@lru_cache
def load_headers():
    if not Path(CACHE_PATH).exists():
        parse_headers()
    with open(CACHE_PATH) as f:
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
        print(f'Variable {name} not found in headers.json.')
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
