import ctypes
from pathlib import Path
from platform import system

import numpy as np
from numpy.ctypeslib import ndpointer

from . import _constants as vky


def load_library():
    if system() == 'Linux':
        lib_path = next(Path(__file__).parent.parent.glob(
            'visky.*.so/libvisky.so')).resolve()
        return ctypes.cdll.LoadLibrary(lib_path)


# Load the shared library
visky = load_library()


# Wrap helpers
def wrap(fun, args, res):
    fun.argtypes = args
    fun.restype = res


T_VP = ctypes.c_void_p
T_INT = ctypes.c_int
T_UINT32 = ctypes.c_uint32
T_RGBA = ctypes.c_float * 4


class ClearColor(ctypes.Union):
    _fields_ = [
        ("float32", ctypes.c_float * 4),
        ("int32", ctypes.c_int32 * 4),
        ("uint32", ctypes.c_int32 * 4),
    ]


# Function wrappers
wrap(visky.vky_create_app, [T_INT, T_VP], T_VP)
wrap(visky.vky_destroy_app, [T_VP], None)
wrap(visky.vky_create_canvas, [T_VP, T_UINT32, T_UINT32], T_VP)
wrap(visky.vky_create_scene, [T_VP, ClearColor, T_UINT32, T_UINT32], T_VP)
wrap(visky.vky_destroy_scene, [T_VP], None)


def demo_blank():
    visky.vky_demo_blank()


def figure():
    app = visky.vky_create_app(vky.BACKEND_GLFW, None)
    canvas = visky.vky_create_canvas(app, T_UINT32(100), T_UINT32(100))
    scene = visky.vky_create_scene(canvas, ClearColor(
        (0, 0, 0, 0)), T_UINT32(1), T_UINT32(1))
    # visky.vky_destroy_scene(scene)
    visky.vky_destroy_app(app)
