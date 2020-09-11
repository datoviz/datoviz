import ctypes

from . import _constants as const


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


# struct VkyMultiRawPathParams
# {
#     vec4 info;                                  // path_count, vertex_count_per_path, scaling
#     vec4 y_offsets[VKY_RAW_PATH_MAX_PATHS / 4]; // NOTE: 16 bytes alignment enforced
#     vec4 colors[VKY_RAW_PATH_MAX_PATHS];        // 16 bytes per path

raw_path_max_paths = int(const.RAW_PATH_MAX_PATHS)


class T_MULTI_RAW_PATH_PARAMS(ctypes.Structure):
    _fields_ = [
        ("info", ctypes.c_float * 4),
        ("y_offsets", ctypes.c_float * 4 * (raw_path_max_paths // 4)),
        ("colors", ctypes.c_float * 4 * raw_path_max_paths),
    ]
