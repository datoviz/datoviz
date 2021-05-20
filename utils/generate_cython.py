# Generate the Cython wrapping file.

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

from parse_headers import ROOT_DIR, load_headers, iter_vars


# Objects to wrap
# -------------------------------------------------------------------------------------------------

STRUCTS = (
    'DvzAutorun',
    'DvzEvent',
    'DvzEventUnion',
    'DvzFrameEvent',
    'DvzGui',  # all structs that start with this
    'DvzKeyEvent',
    'DvzMouseButtonEvent',
    'DvzMouseClickEvent',
    'DvzMouseDragEvent',
    'DvzMouseMoveEvent',
    'DvzMouseWheelEvent',
    'DvzRefillEvent',
    'DvzResizeEvent',
    'DvzScreencastEvent',
    'DvzSubmitEvent',
    'DvzTimerEvent',
    'DvzViewport',
)

FUNCTIONS = (
    'dvz_app_destroy',
    'dvz_app_run',
    'dvz_app',
    'dvz_autorun_setup',
    'dvz_canvas_clear_color',
    'dvz_canvas_frame',
    'dvz_canvas_pause',
    'dvz_canvas_pick',
    'dvz_canvas_stop',
    'dvz_canvas_to_close',
    'dvz_canvas_video',
    'dvz_canvas',
    'dvz_colormap_array',
    'dvz_colormap_custom',
    'dvz_colormap_packuv',
    'dvz_context_colormap',
    'dvz_context',
    'dvz_copy_buffer',
    'dvz_copy_texture',
    'dvz_ctx_texture',
    'dvz_download_buffer',
    'dvz_download_texture',
    'dvz_event',  # all dvz_event_*() functions
    'dvz_gpu_best',
    'dvz_gpu_default',
    'dvz_gpu',
    'dvz_gui',  # all dvz_gui*() functions
    'dvz_imgui_demo',
    'dvz_mesh',
    'dvz_panel_at',
    'dvz_panel_transpose',
    'dvz_process_transfers',
    'dvz_prop_get',
    'dvz_scene_destroy',
    'dvz_scene_panel',
    'dvz_scene_visual',
    'dvz_scene',
    'dvz_screenshot_file',
    'dvz_texture_filter',
    'dvz_texture_upload',
    'dvz_transform',
    'dvz_upload_buffer',
    'dvz_upload_texture',
    'dvz_visual_data_source',
    'dvz_visual_data',
    'dvz_visual_texture',
)


# Constants
# -------------------------------------------------------------------------------------------------

CYTHON_OUTPUT = (ROOT_DIR / 'bindings/cython/datoviz/cydatoviz.pxd').resolve()

ENUM_START = '# ENUM START'
ENUM_END = '# ENUM END'
STRUCT_START = '# STRUCT START'
STRUCT_END = '# STRUCT END'
UNION_START = '# UNION START'
UNION_END = '# UNION END'
FUNCTION_START = '# FUNCTION START'
FUNCTION_END = '# FUNCTION END'



# Cython generation utils
# -------------------------------------------------------------------------------------------------

def insert_into_file(filename, start, end, insert):
    text = filename.read_text()
    i0 = text.index(start)
    i1 = text.index(end)
    out = text[:i0 + len(start) + 1]
    out += indent(insert, '    ')
    out += text[i1 - 5:]
    filename.write_text(out)


@lru_cache(maxsize=64)
def _camel_to_snake(name):
    name = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', name).lower()


def _keep(n, t):
    if n in t:
        return True
    for _ in t:
        if n.startswith(_):
            return True
    return False


# Cython generation
# -------------------------------------------------------------------------------------------------

def _generate_enum(enum, defines=None):
    assert enum
    name = enum['name']
    values = enum['values']
    out = ''
    out += f'ctypedef enum {name}:\n'
    for identifier, value in values:
        # defines replacements
        if defines and value in defines:
            value = f'{defines[value]}  # {value}'
        out += f'    {identifier} = {value}\n'
    out += '\n'
    return out


def _generate_struct(struct, defines=None):
    assert struct
    name = struct['name']
    out = ''
    out += f'ctypedef {struct["type"]} {name}:\n'
    for field in struct['fields']:
        const = field.get('const', None)
        dtype = field.get('dtype', None)
        count = field.get('count', '')
        identifier = field.get('name', None)
        if dtype == 'bool':
            dtype = 'bint'
        if const:
            dtype = "const " + dtype
        if count:
            if defines and count in defines:
                count = defines[count]
            count = f'[{count}]'
        out += f'    {dtype} {identifier}{count}\n'
    out += '\n'
    return out


def _generate_function(func):
    assert func
    name = func['name']
    args_s = []
    out = func.get('returns', None) or 'void'
    for arg in func['args']:
        const = arg.get('const', None)
        dtype = arg.get('dtype', None)
        argname = arg.get('name', None)
        if not argname:
            if 'int' in dtype:
                argname = 'n%d' % len(args_s)
            elif dtype == 'float':
                argname = 'value'
            elif 'vec' in dtype:
                argname = 'vec'
            elif dtype == 'void*':
                argname = 'buf'
            elif 'char' in dtype:
                argname = 's'
            elif dtype == 'void':
                dtype = ''
                argname = ''
            elif dtype == 'bool':
                argname = 'value'
            elif dtype == 'size_t':
                argname = 'size'
            elif 'Dvz' in dtype:
                argname = _camel_to_snake(
                    dtype.replace('Dvz', '')).replace('*', '')
            else:
                raise ValueError(dtype)
        if const:
            dtype = "const " + dtype
        if dtype == 'bool':
            dtype = 'bint'
        args_s.append(f'{dtype} {argname}'.strip())
    args = ', '.join(args_s)
    return f'{out} {name}({args})\n\n'


def generate_enums():
    out = ''
    defines = dict(iter_vars("defines"))
    for n, v in iter_vars('enums'):
        out += _generate_enum(v, defines=defines)
    return out


def generate_structs():
    out = ''
    defines = dict(iter_vars("defines"))
    for n, v in iter_vars('structs'):
        if _keep(n, STRUCTS):
            out += _generate_struct(v, defines=defines)
    return out


def generate_functions():
    out = ''
    for n, v in iter_vars('functions'):
        if _keep(n, FUNCTIONS):
            out += _generate_function(v)
    return out


def generate_cython():
    insert_into_file(CYTHON_OUTPUT, ENUM_START, ENUM_END, generate_enums())
    insert_into_file(CYTHON_OUTPUT, STRUCT_START, STRUCT_END, generate_structs())
    insert_into_file(CYTHON_OUTPUT, FUNCTION_START, FUNCTION_END, generate_functions())


if __name__ == '__main__':
    generate_cython()
