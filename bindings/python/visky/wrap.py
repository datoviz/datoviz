import csv
import ctypes
from ctypes import pointer, POINTER
import logging
from pathlib import Path
from platform import system

from imageio import imread
import numpy as np
from numpy.ctypeslib import ndpointer

from . import _constants as const
from . import _types as tp
from ._types import (
    Bunch,
    T_VP, T_BOOL, T_INT, T_UINT32, T_FLOAT, T_VEC2, T_DOUBLE, T_NDARRAY, T_COLOR, T_DATA
)

logger = logging.getLogger(__name__)


def load_library():
    if system() == 'Linux':
        lib_path = next(Path(__file__).parent.parent.glob(
            'visky.*.so/libvisky.so')).resolve()
        return ctypes.cdll.LoadLibrary(lib_path)


# Load the shared library
viskylib = load_library()


# Wrap helpers

def wrap(fun, args, res=None):
    fun.argtypes = args
    fun.restype = res


def array_pointer(x):
    return x.ctypes.data_as(ctypes.c_void_p)


def make_vertices(pos, col):
    pos = np.asarray(pos, dtype=np.float32)
    col = np.asarray(col, dtype=np.float32)
    assert pos.ndim == 2
    assert col.ndim == 2
    n = pos.shape[0]
    assert pos.shape == (n, 3)
    assert col.shape == (n, 4)

    vertices = np.empty((n,), dtype=[('pos', 'f4', 3), ('color', 'u1', 4)])
    vertices['pos'] = pos
    vertices['color'] = col
    return vertices


def upload_data(visual, items=None, indices=None):
    assert items is not None
    assert isinstance(items, np.ndarray)
    assert items.size > 0
    if indices is None:
        data = T_DATA(
            items.size, array_pointer(items), 0, None, 0, None, False)
    else:
        data = T_DATA(
            0, None, len(items), array_pointer(items), len(indices), array_pointer(indices), False)
    viskylib.vky_visual_upload(visual, data)


# Constant helpers

def get_const(x, default=None):
    if isinstance(x, str):
        n = x
        x = getattr(const, x.upper(), None)
    elif x is None and default is not None:
        return get_const(default)
    if x is not None:
        return x
    else:
        logger.warning(f"Constant {n} could not be found, defaulting to 0")
    return x if x is not None else 0


def _stringify(n):
    return {
        getattr(const, s): s[len(n):].lower()
        for s in dir(const) if s.startswith(n)
    }


# TODO; Python 3 enum
_KEY_STRINGS = _stringify('KEY_')
_KEY_MODIFIER_STRINGS = _stringify('KEY_MODIFIER_')
_MOUSE_BUTTON_STRINGS = _stringify('MOUSE_BUTTON_')
_MOUSE_STATE_STRINGS = _stringify('MOUSE_STATE_')


def key_string(key):
    return _KEY_STRINGS.get(key, None)


def mouse_state(state):
    return _MOUSE_STATE_STRINGS.get(state, None)


def mouse_button(button):
    return _MOUSE_BUTTON_STRINGS.get(button, None) if button != 0 else None


def key_modifiers(keyboard):
    pass


def to_byte(x, vmin=0, vmax=1):
    if vmin >= vmax:
        logger.warning("vmin >= vmax")
        vmin = np.nanmin(x)
        vmax = np.nanmax(x)
    if vmin >= vmax:
        d = 1
    else:
        d = 1 / (vmax - vmin)
    assert d > 0
    x = np.clip(np.asarray(x, dtype=np.float64), vmin, vmax)
    x = np.round(255 * (x - vmin) * d).astype(np.uint8)
    return x


const.WHITE = T_COLOR(255, 255, 255, 255)
const.BLACK = T_COLOR(0, 0, 0, 255)


# Function wrappers

wrap(viskylib.log_set_level_env, [])
wrap(viskylib.vky_create_app, [T_INT, T_VP], T_VP)
wrap(viskylib.vky_run_app, [T_VP])
wrap(viskylib.vky_destroy_app, [T_VP])

wrap(viskylib.vky_glfw_run_app_begin, [T_VP])
wrap(viskylib.vky_glfw_run_app_process, [T_VP])
wrap(viskylib.vky_glfw_run_app_end, [T_VP])
wrap(viskylib.vky_all_windows_closed, [T_VP], T_BOOL)

wrap(viskylib.vky_create_canvas, [T_VP, T_UINT32, T_UINT32], T_VP)

wrap(viskylib.vky_create_scene, [T_VP, T_COLOR, T_UINT32, T_UINT32], T_VP)
wrap(viskylib.vky_set_grid_widths, [T_VP, T_NDARRAY(np.float32)], None)
wrap(viskylib.vky_set_grid_heights, [T_VP, T_NDARRAY(np.float32)], None)
wrap(viskylib.vky_destroy_scene, [T_VP])

wrap(viskylib.vky_get_panel, [T_VP, T_UINT32, T_UINT32], T_VP)
wrap(viskylib.vky_get_panel_index, [T_VP], tp.T_PANEL_INDEX)
wrap(viskylib.vky_set_controller, [T_INT, T_VP])
wrap(viskylib.vky_set_panel_aspect_ratio, [T_VP, T_FLOAT])

wrap(viskylib.vky_add_visual_to_panel, [T_VP, T_INT, T_INT])
wrap(viskylib.vky_visual, [T_VP, T_INT, T_VP, T_VP], T_VP)
wrap(viskylib.vky_visual_upload, [T_VP, T_DATA])

wrap(viskylib.vky_get_axes, [T_VP], T_VP)
wrap(viskylib.vky_axes_set_initial_range, [T_VP, tp.T_BOX2D])
wrap(viskylib.vky_axes_set_range, [T_VP, tp.T_BOX2D, T_BOOL])
wrap(viskylib.vky_pick, [T_VP, T_VEC2], tp.T_PICK)

wrap(viskylib.vky_visual_image_upload, [T_VP, T_VP])
wrap(viskylib.vky_default_texture_params, [tp.T_IVEC3], tp.T_TEXTURE_PARAMS)

wrap(viskylib.vky_add_frame_callback, [T_VP, T_VP])
wrap(viskylib.vky_event_keyboard, [T_VP], POINTER(tp.T_KEYBOARD))
wrap(viskylib.vky_event_mouse, [T_VP], POINTER(tp.T_MOUSE))

wrap(viskylib.vky_colormap_apply, [
     T_INT, T_DOUBLE, T_DOUBLE, T_UINT32, POINTER(tp.T_DOUBLE), POINTER(T_COLOR)])
wrap(viskylib.vky_demo_raytracing, [])


viskylib.log_set_level_env()


def read_csv(path):
    out = {}
    with open(path, 'r') as f:
        csv_reader = csv.DictReader(f)
        for row in csv_reader:
            out[row['name'].lower()] = (
                int(row['row']), int(row['col']), int(row['size']))
    return out


# Read the colormap texture.
COLORMAP = imread(
    Path(__file__).parent / '../../../data/textures/color_texture.png')
COLORMAP_INFO = read_csv(
    Path(__file__).parent / '../../../data/textures/color_texture.csv')


def get_color(cmap, x, vmin=0, vmax=1, alpha=1):
    out = np.empty(x.shape + (4,), dtype=np.uint8)
    assert cmap in COLORMAP_INFO, cmap
    row, col, size = COLORMAP_INFO.get(cmap)
    if size == 256:
        # continuous colormap with interpolation
        i = to_byte(x, vmin=vmin, vmax=vmax)
        out[..., :] = COLORMAP[row, i, :]
    else:
        # colormap with no interpolation
        out[..., :] = COLORMAP[row, x, :]
    out[..., 3] = to_byte(alpha)
    return out
