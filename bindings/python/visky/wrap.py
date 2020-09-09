import ctypes
from pathlib import Path
from platform import system

import numpy as np
from numpy.ctypeslib import ndpointer


def load_library():
    if system() == 'Linux':
        lib_path = next(Path(__file__).parent.parent.glob(
            'visky.*.so/libvisky.so')).resolve()
        return ctypes.cdll.LoadLibrary(lib_path)


# Load the shared library
viskylib = load_library()


# Wrap helpers
def wrap(fun, args, res):
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


# Types
T_VP = ctypes.c_void_p
T_INT = ctypes.c_int
T_UINT8 = ctypes.c_uint8
T_UINT32 = ctypes.c_uint32


class T_COLOR(ctypes.Structure):
    _fields_ = [
        ("r", ctypes.c_uint8),
        ("g", ctypes.c_uint8),
        ("b", ctypes.c_uint8),
        ("a", ctypes.c_uint8),
    ]


class T_DATA(ctypes.Structure):
    _fields_ = [
        ("item_count", ctypes.c_uint32),
        ("items", ctypes.c_void_p),

        ("vertex_count", ctypes.c_uint32),
        ("vertices", ctypes.c_void_p),

        ("index_count", ctypes.c_uint32),
        ("indices", ctypes.c_void_p),

        ("no_vertices_alloc", ctypes.c_bool),
    ]


# Function wrappers
wrap(viskylib.vky_create_app, [T_INT, T_VP], T_VP)
wrap(viskylib.vky_run_app, [T_VP], None)
wrap(viskylib.vky_destroy_app, [T_VP], None)

wrap(viskylib.vky_create_canvas, [T_VP, T_UINT32, T_UINT32], T_VP)

wrap(viskylib.vky_create_scene, [T_VP, T_COLOR, T_UINT32, T_UINT32], T_VP)
wrap(viskylib.vky_destroy_scene, [T_VP], None)

wrap(viskylib.vky_get_panel, [T_VP, T_UINT32, T_UINT32], T_VP)
wrap(viskylib.vky_set_controller, [T_INT, T_VP], None)
wrap(viskylib.vky_add_visual_to_panel, [T_VP, T_INT, T_INT], None)
wrap(viskylib.vky_visual, [T_VP, T_INT, T_VP], T_VP)
wrap(viskylib.vky_visual_upload, [T_VP, T_DATA], None)
