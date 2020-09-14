import ctypes
from ctypes import pointer
import logging
from pathlib import Path
from platform import system

import numpy as np
from numpy.ctypeslib import ndpointer

from . import _constants as const
from . import _types as tp
from ._types import T_VP, T_INT, T_UINT32, T_FLOAT, T_DOUBLE, T_COLOR, T_DATA

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


def get_const(x, default=None):
    if isinstance(x, str):
        val = getattr(const, x.upper(), None)
        if val is None:
            logger.warning("Constant %s not found", x)
    else:
        val = x
    val = val if val is not None else get_const(default)
    return val


const.WHITE = T_COLOR(255, 255, 255, 255)
const.BLACK = T_COLOR(0, 0, 0, 255)


# Function wrappers
wrap(viskylib.log_set_level_env, [])
wrap(viskylib.vky_create_app, [T_INT, T_VP], T_VP)
wrap(viskylib.vky_run_app, [T_VP])
wrap(viskylib.vky_destroy_app, [T_VP])

wrap(viskylib.vky_create_canvas, [T_VP, T_UINT32, T_UINT32], T_VP)

wrap(viskylib.vky_create_scene, [T_VP, T_COLOR, T_UINT32, T_UINT32], T_VP)
wrap(viskylib.vky_destroy_scene, [T_VP])

wrap(viskylib.vky_get_panel, [T_VP, T_UINT32, T_UINT32], T_VP)
wrap(viskylib.vky_set_controller, [T_INT, T_VP])
wrap(viskylib.vky_add_visual_to_panel, [T_VP, T_INT, T_INT])
wrap(viskylib.vky_visual, [T_VP, T_INT, T_VP, T_VP], T_VP)
wrap(viskylib.vky_visual_upload, [T_VP, T_DATA])

wrap(viskylib.vky_get_axes, [T_VP], T_VP)
wrap(viskylib.vky_axes_set_range, [
     T_VP, T_DOUBLE, T_DOUBLE, T_DOUBLE, T_DOUBLE])

wrap(viskylib.vky_visual_image_upload, [T_VP, T_VP])
wrap(viskylib.vky_default_texture_params, [tp.T_IVEC3], tp.T_TEXTURE_PARAMS)

wrap(viskylib.vky_add_frame_callback, [T_VP, T_VP])
wrap(viskylib.vky_event_key, [T_VP], T_INT)
