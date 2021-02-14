from pathlib import Path

from joblib import Memory
from mkdocs.structure.files import File
from mkdocs.structure.pages import Page

from .gendoc import (
    insert_functions_doc, insert_enums_doc, insert_graphics_doc,
    parse_headers, process_index_page, process_code_image,
    # example_list, nav_example_list,
)
from .export_colormap import generate_colormaps_doc


cachedir = Path(__file__).parent / '.joblib'
MEM = Memory(cachedir)
parse_headers = MEM.cache(parse_headers)


def config_hook(config):
    config['gendoc'] = parse_headers()
    return config


def page_hook(markdown, page, config, files):
    assert 'gendoc' in config
    path = page.file.abs_src_path
    if 'docs/index.md' in path:
        return process_index_page(markdown, config)
    elif 'graphics/' in path:
        return insert_graphics_doc(markdown, config)
    elif 'color' in path:
        return (markdown + '\n\n' + generate_colormaps_doc())
    elif 'enum' in path:
        return insert_enums_doc(markdown, config)
    elif 'api/' in path:
        return insert_functions_doc(markdown, config)
    # elif 'examples/index.md' in path:
    #     return example_list(markdown, config)
    elif 'developer' not in path:
        return process_code_image(markdown, config)


# def nav_hook(nav, config, files):
#     # NOTE: this hook doesn't seem to work
#     src_dir = 'docs/examples/'
#     dest_dir = 'examples/'

#     s = [s for s in nav.items if s.title == 'Examples'][0]

#     for title, path in nav_example_list():
#         file = File(path, src_dir, dest_dir, True)
#         print(f"- {title}: 'examples/{path}'")
#         page = Page(title, file, config)
#         s.children.append(page)
#     return nav
