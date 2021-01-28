from .gendoc import (
    insert_functions_doc, insert_enums_doc, insert_graphics_doc,
    parse_headers,
)
from .export_colormap import generate_colormaps_doc


# ENABLE = 0


def config_hook(config):
    config['gendoc'] = parse_headers()
    return config


def page_hook(markdown, page, config, files):
    # if not ENABLE:
    #     return
    assert 'gendoc' in config
    path = page.file.abs_src_path
    if 'graphics' in path:
        return insert_graphics_doc(markdown, config)
    elif 'color' in path:
        return (
            markdown + '\n\n## List of colormaps and color palettes\n\n' +
            generate_colormaps_doc())
    elif 'enum' in path:
        return insert_enums_doc(markdown, config)
    elif 'api/' in path:
        return insert_functions_doc(markdown, config)
