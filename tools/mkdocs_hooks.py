from pathlib import Path
import re


CURDIR = Path(__file__).parent
ROOT_DOCS = ('ARCHITECTURE', 'BUILD', 'CONTRIBUTING', 'MAINTAINERS')


def read(path):
    with open(CURDIR / path, "r") as f:
        return f.read()


def on_page_markdown(markdown, page, config, files):
    name = page.file.name
    if name == "index":
        markdown = read("../README.md")
        return re.sub(r'\]\(docs/(.*?)\.md\)', r'](\1.md)', markdown)
    elif name == 'LICENSE':
        return read(f"../LICENSE")
    elif name in ROOT_DOCS:
        return read(f"../{name}.md")
    return markdown
