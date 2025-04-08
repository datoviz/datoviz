# Generate the documentation.

# Imports
# -------------------------------------------------------------------------------------------------

from operator import itemgetter
from textwrap import indent, dedent
import json
import itertools
from pathlib import Path
import re
import sys


# Constants
# -------------------------------------------------------------------------------------------------

ROOT_DIR = Path(__file__).parent.parent
HEADERS_FILE = ROOT_DIR / 'tools/headers.json'
API_OUTPUT = ROOT_DIR / 'docs/api.md'
EXAMPLES_PATH = ROOT_DIR / 'docs/examples.md'
PYTHON_EXAMPLES_DIR = ROOT_DIR / 'examples/'

ITEM_HEADER = re.compile(r'^#+\s+', flags=re.MULTILINE)
GRAPHICS_CODE = re.compile(r'```c\n([a-zA-Z0-9\_]+)\n```')
MAX_LINE_LENGTH = 76
INDEX_LINK = re.compile(r'\]\(docs/([^\)]+\.md)\)')
INSERT_CODE = re.compile(r'<!-- CODE_([^ ]+) ([^ ]+) -->')
INSERT_IMAGE = re.compile(r'<!-- IMAGE ([^ ]+) -->')

PYTHON_EXAMPLE_TEMPLATE = '''
{description}

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/{name}.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/{name}.py</code></summary>

```python
{code}
```
</details>

'''.strip()
PYTHON_DESC_REGEX = re.compile(r'"""([^"]+)"""')

ICONS = {
    'in': ':octicons-arrow-right-16:',
    'out': ':octicons-arrow-left-16:',
}


# Utils
# -------------------------------------------------------------------------------------------------

def insert_text(text, i, insert):
    return text[:i] + insert + text[i:]


def grouper(n, iterable):
    it = iter(iterable)
    while True:
        chunk_it = itertools.islice(it, n)
        try:
            first_el = next(chunk_it)
        except StopIteration:
            return
        yield itertools.chain((first_el,), chunk_it)


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


# Doc generation
# -------------------------------------------------------------------------------------------------

def _generate_enum(enum, defines=None):
    assert enum
    name = enum['name']
    values = enum['values']
    out = ''
    out += '| Name | Value |\n| ---- | ---- |\n'
    for identifier, value in values:
        out += f'| `{identifier}` | {value} |\n'
    return out + '\n'


def _generate_struct(struct, defines=None):
    assert struct
    name = struct['name']
    out = ''
    out += f'ctypedef {struct["type"]} {name}:\n'
    out = f'#### {name}\n\n| Field | Type | Description |\n| ---- | ---- | ---- |\n'
    for field in struct['fields']:
        const = field.get('const', None)
        dtype = field.get('dtype', None)
        count = field.get('count', '')
        desc = field.get('desc', '')
        identifier = field.get('name', None)
        if const:
            dtype = "const " + dtype
        if count:
            count = f'[{count}]'
        desc = desc.replace('/*', '').replace('*/', '').strip()
        out += f'| `{identifier}` | `{dtype}` | {desc} |\n'
    return out.strip() + '\n\n'


def _generate_function(func):
    # Generate the function documentation
    assert func
    name = func['name']
    out = func.get('returns', None) or 'void'
    args = func['args']
    docstring = func['docstring']
    docstring = docstring if docstring.startswith('/**\n') else ''
    docstring = '\n'.join(_[3:] for _ in docstring.splitlines())
    docstring = docstring.strip()

    def _arg_type(n):
        for arg in args:
            if arg['name'] == n:
                return arg['dtype']

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
        f"{'const ' if arg.get('const', None) else ''}{arg['dtype']} {arg['name']}"
        for arg in args)
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


# mkdocs plugins
# -------------------------------------------------------------------------------------------------

def insert_functions_doc(markdown, config):
    from .parse_headers import get_var
    funcs_to_output = _parse_markdown(markdown)
    out = markdown
    # Output text, copy of the original text
    for m in funcs_to_output:
        name = m.group(1)
        # Find the position of the function in the current text
        r = re.compile(r'^#+\s+`%s\(\)`' % name, flags=re.MULTILINE)
        m2 = r.search(out)
        i = m2.end(0)
        insert = _generate_function(get_var(name))
        out = insert_text(out, i, f'\n\n{insert}\n\n')
    out = out.strip() + '\n'
    return out


def insert_enums_doc(markdown, config):
    from .parse_headers import get_var
    enums_to_output = _parse_markdown_enums(markdown)
    out = markdown
    # Output text, copy of the original text
    for m in enums_to_output:
        name = m.group(1)
        # Find the position of the function in the current text
        r = re.compile(r'^#+\s+`%s`' % name, flags=re.MULTILINE)
        m2 = r.search(out)
        i = m2.end(0)
        insert = _generate_enum(get_var(name))
        out = insert_text(out, i, f'\n\n{insert}\n\n')
    out = out.strip() + '\n'
    return out


def insert_graphics_doc(markdown, config):
    from .parse_headers import get_var

    def _sub_graphics_code(m):
        n = m.group(1)
        out = ''
        for h in ('Item', 'Vertex', 'Params'):
            s = f'DvzGraphics{n}{h}'
            struct = get_var(s)
            if struct:
                struct[s] = f'{h} structure: `{s}`'
                out += _generate_struct(struct)
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
        print(f"Insert code from {path}")
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


def generate_examples():
    readme = EXAMPLES_PATH
    examples = sorted(PYTHON_EXAMPLES_DIR.glob("*.py"), key=lambda x: (x.name == "drp.py", x.name))
    out = ""
    toc = ""

    for f in examples:
        print(f"Generating Python example {f.name}")

        code = f.read_text()
        m = PYTHON_DESC_REGEX.match(code)
        assert m
        desc = m.group(1)
        desc = "#" + desc

        # NOTE: only remove the first docstring.
        code = PYTHON_DESC_REGEX.sub('', code, count=1).strip()
        doc = PYTHON_EXAMPLE_TEMPLATE.format(
            description=desc, code=code, name=f.stem, path=f)

        title = desc.splitlines()[0][2:].strip()
        anchor = title.lower().replace(' ', '-')
        toc += f"* [{title}](#{anchor})\n"
        out += f"{doc}\n\n"

    with open(readme, 'w', encoding='utf-8') as fr:
        out = f"# Examples\n\n{toc}\n\n{out}"
        fr.write(out)


def parse_doxygen_docstring(docstring):
    # Regular expression patterns for different components of the docstring
    description_pattern = re.compile(r'/\*\*\s*\n\s*([^\n]+)', re.DOTALL)
    param_pattern = re.compile(r'@param\s+(\w+)\s+([^\n]+)', re.DOTALL)
    returns_pattern = re.compile(r'@returns\s+([^\n]+)', re.DOTALL)

    # Extract the description
    description_match = description_pattern.search(docstring)
    description = description_match.group(
        1).strip() if description_match else ''

    # Extract parameters
    params = []
    for param_match in param_pattern.finditer(docstring):
        param_name = param_match.group(1).strip()
        param_description = param_match.group(2).strip()
        params.append((param_name, param_description))

    # Extract return value
    returns_match = returns_pattern.search(docstring)
    returns = returns_match.group(1).strip() if returns_match else ''

    return {
        'description': description,
        'params': params,
        'returns': returns
    }


def generate_api():
    import datoviz as dvz

    with open(HEADERS_FILE, 'r') as f:
        objects = json.load(f)
    md = dedent("""
    # C API Reference

    """).lstrip()

    # Functions
    md += f"## Main functions\n\n"

    # HACK: put DRP functions at the end.
    files = sorted(objects.keys())
    files.remove('datoviz_protocol.h')
    files += ['datoviz_protocol.h']

    for filename in files:
        if filename == 'datoviz_protocol.h':
            md += f"## Datoviz Rendering Protocol functions\n\n"

        items = objects[filename]
        for func_name, func_info in sorted(items["functions"].items(), key=itemgetter(0)):

            md += f"### `{func_name}()`\n\n"

            docstring = func_info["docstring"]
            parsed_docstring = parse_doxygen_docstring(docstring)

            return_type = func_info["returns"]
            return_type_ = return_type + " " if return_type else ""

            return_desc = parsed_docstring["returns"]
            dtypes = {_["name"]: _["dtype"] for _ in func_info["args"]}

            desc = parsed_docstring['description'].replace('* ', '')

            md += desc + "\n\n"

            # Python-style
            py_func_name = func_name.replace("dvz_", "")

            # ctypes argument type mapping
            mtypes = []
            func = getattr(dvz, py_func_name, None)
            if func:
                mtypes = [_.__name__ for _ in func.argtypes]

            py_args = "\n".join(
                f"    {arg_name},  # {arg_desc} ({arg_type})"
                for (arg_name, arg_desc), arg_type in zip(parsed_docstring["params"], mtypes))
            py_return_type = func.restype.__name__
            py_return_desc = f"  # returns: {return_desc} ({py_return_type})" if return_desc else ""
            py_func_desc = f"``` python\ndvz.{py_func_name}("
            if py_args:
                py_func_desc += f"{py_return_desc}\n{py_args}\n)"
            else:
                py_func_desc += f"){py_return_desc}"
            py_func_desc += "\n```\n\n"
            md += f"""=== "Python"\n\n{indent(py_func_desc, '    ')}"""

            # C-style
            args = "\n".join(
                f"    {dtypes.get(arg_name, '')} {arg_name},  // {arg_desc}"
                for (arg_name, arg_desc) in parsed_docstring["params"])
            return_desc = f"  // returns: {return_desc}" if return_desc else ""
            func_desc = f"``` c\n{return_type_}{func_name}("
            if args:
                func_desc += f"{return_desc}\n{args}\n);"
            else:
                func_desc += f");{return_desc}"
            func_desc += "\n```\n\n"
            md += f"""=== "C"\n\n{indent(func_desc, '    ')}"""

    # Enumerations
    md += f"## Enumerations"

    for filename, items in sorted(objects.items(), key=itemgetter(0)):
        for enum_name, enum_info in sorted(items["enums"].items(), key=itemgetter(0)):

            md += f"\n\n### `{enum_name}`\n\n"
            md += '```\n'
            md += '\n'.join(f"{value[0]}" for value in enum_info["values"])
            md += '\n```'

    # Structures
    md += f"\n\n## Structures\n\n"
    for filename, items in sorted(objects.items(), key=itemgetter(0)):
        for struct_name, struct_info in sorted(items["structs"].items(), key=itemgetter(0)):

            md += f"### `{struct_name}`\n\n"

            md += f'```\n{struct_info["type"]} {struct_name}\n'
            for field in struct_info['fields']:
                unsigned = 'unsigned ' if field.get("unsigned", None) else ''
                md += f'    {unsigned}{field["dtype"]} {field["name"]}\n'
            md += '```\n\n'

    # Write the output.
    with open(API_OUTPUT, 'w') as f:
        f.write(md)


if __name__ == '__main__':
    if 'api' in sys.argv:
        generate_api()
    if 'examples' in sys.argv:
        generate_examples()
