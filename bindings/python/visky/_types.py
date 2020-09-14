import ctypes

import numpy as np
from numpy.ctypeslib import ndpointer

from . import _constants as const


# Types
T_VP = ctypes.c_void_p

T_FLOAT = ctypes.c_float
T_DOUBLE = ctypes.c_double

T_VEC2 = ctypes.c_float * 2
T_VEC3 = ctypes.c_float * 3
T_VEC4 = ctypes.c_float * 4

T_INT = ctypes.c_int
T_UINT8 = ctypes.c_uint8
T_UINT32 = ctypes.c_uint32

T_IVEC2 = ctypes.c_int * 2
T_IVEC3 = ctypes.c_int * 3
T_IVEC4 = ctypes.c_int * 4


def T_NDARRAY(dtype, ndim=1):
    if isinstance(dtype, np.dtype):
        ctype = np.ctypeslib.as_ctypes_type(dtype)
    else:
        ctype = dtype
    return ndpointer(ctype, ndim=ndim, flags='C_CONTIGUOUS')


# Callbacks

canvas_callback = ctypes.CFUNCTYPE(None, T_VP)


# Structs

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


# Visual param structures

class T_PATH_PARAMS(ctypes.Structure):
    _fields_ = [
        ("linewidth", ctypes.c_float),
        ("miter_limit", ctypes.c_float),
        ("cap_type", ctypes.c_int32),
        ("round_join", ctypes.c_int32),
        ("enable_depth", ctypes.c_int32),
    ]


class T_PATH_DATA(ctypes.Structure):
    _fields_ = [
        ("point_count", ctypes.c_uint32),
        ("points", T_VP),
        ("colors", T_VP),
        ("topology", T_INT),
    ]


class T_MULTI_RAW_PATH_PARAMS(ctypes.Structure):
    raw_path_max_paths = int(const.RAW_PATH_MAX_PATHS)
    _fields_ = [
        ("info", ctypes.c_float * 4),
        ("y_offsets", ctypes.c_float * 4 * (raw_path_max_paths // 4)),
        ("colors", ctypes.c_float * 4 * raw_path_max_paths),
    ]


class T_TEXTURE_PARAMS(ctypes.Structure):
    _fields_ = [
        ("width", ctypes.c_uint32),
        ("height", ctypes.c_uint32),
        ("depth", ctypes.c_uint32),
        ("format_bytes", ctypes.c_uint8),
        ("format", ctypes.c_int),
        ("filter", ctypes.c_int),
        ("address_mode", ctypes.c_int),
        ("layout", ctypes.c_int),
        ("enable_compute", ctypes.c_bool),
    ]
