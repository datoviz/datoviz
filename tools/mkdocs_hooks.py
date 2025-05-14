from pathlib import Path

CURDIR = Path(__file__).parent
ROOT_DOCS = ('ARCHITECTURE', 'BUILD', 'CONTRIBUTING', 'MAINTAINERS')


def read(path):
    with open(CURDIR / path) as f:
        return f.read()


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
