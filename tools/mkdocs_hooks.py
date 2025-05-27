# mkdocs hooks

# Imports
# -------------------------------------------------------------------------------------------------

import ast
import os
from pathlib import Path

# Constants
# -------------------------------------------------------------------------------------------------

CURDIR = Path(__file__).parent
ROOT_DOCS = ('ARCHITECTURE', 'BUILD', 'CONTRIBUTING', 'MAINTAINERS')


# Util functions
# -------------------------------------------------------------------------------------------------


def read(path):
    with open(CURDIR / path) as f:
        return f.read()


# Hooks
# -------------------------------------------------------------------------------------------------


def on_page_markdown(markdown, page, config, files):
    name = page.file.name
    # if name == "index":
    #     markdown = read("../README.md")
    #     return re.sub(r'\]\(docs/(.*?)\.md\)', r'](\1.md)', markdown)
    if name == 'LICENSE':
        return read('../LICENSE')
    elif name in ROOT_DOCS:
        return read(f'../{name}.md')
    return markdown


def on_pre_build(**kwargs):
    src_root = 'examples'
    dst_root = 'cleaned'

    for dirpath, _, filenames in os.walk(src_root):
        py_files = [f for f in filenames if f.endswith('.py')]
        if not py_files:
            continue

        rel_dir = os.path.relpath(dirpath, src_root)
        dst_dir = os.path.join(dst_root, rel_dir)
        os.makedirs(dst_dir, exist_ok=True)

        for filename in py_files:
            src_path = os.path.join(dirpath, filename)
            dst_path = os.path.join(dst_dir, filename)

            with open(src_path) as f:
                source = f.read()
            mod = ast.parse(source)
            lines = source.splitlines()

            if (
                mod.body
                and isinstance(mod.body[0], ast.Expr)
                and isinstance(mod.body[0].value, ast.Constant)
            ):
                docstring = mod.body[0].value.value
                doc_lines = docstring.splitlines()
                start = mod.body[0].lineno
                end = start + len(doc_lines)
                lines = lines[end:]

            with open(dst_path, 'w') as f:
                f.write('\n'.join(lines).lstrip() + '\n')
