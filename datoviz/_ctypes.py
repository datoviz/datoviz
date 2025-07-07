"""
Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
Licensed under the MIT license. See LICENSE file in the project root for details.
SPDX-License-Identifier: MIT
"""

# WARNING: DO NOT EDIT: automatically-generated file

__version__ = "0.3.1"


# ===============================================================================
# Imports
# ===============================================================================

import ctypes
import faulthandler
import os
import pathlib
import platform
from collections.abc import Iterable
from ctypes import POINTER as P_  # noqa
from ctypes import byref  # noqa
from enum import IntEnum
from pathlib import Path
from typing import Optional

try:
    import numpy as np
    from numpy.ctypeslib import ndpointer as ndpointer_
except ImportError:
    float32 = object
    raise ImportError('NumPy is not available')


# ===============================================================================
# Fault handler
# ===============================================================================

faulthandler.enable()


# ===============================================================================
# Global variables
# ===============================================================================

PLATFORMS = {
    'Linux': 'linux',
    'Darwin': 'macos',
    'Windows': 'windows',
}
PLATFORM = PLATFORMS.get(platform.system(), None)

LIB_NAMES = {
    'linux': 'libdatoviz.so',
    'macos': 'libdatoviz.dylib',
    'windows': 'libdatoviz.dll',
}
LIB_NAME = LIB_NAMES.get(PLATFORM, '')

FILE_DIR = pathlib.Path(__file__).parent.resolve()

# Package paths: this Python file is stored alongside the dynamic libraries.
DATOVIZ_DIR = FILE_DIR
LIB_DIR = FILE_DIR
LIB_PATH = DATOVIZ_DIR / LIB_NAME

# Development paths: the libraries are in build/ and libs/
if not LIB_PATH.exists():
    DATOVIZ_DIR = (FILE_DIR / '../build/').resolve()
    LIB_DIR = (FILE_DIR / f'../libs/vulkan/{PLATFORM}/').resolve()
    LIB_PATH = DATOVIZ_DIR / LIB_NAME

if not LIB_PATH.exists():
    raise RuntimeError(f'Unable to find `{LIB_PATH}`.')


# ===============================================================================
# Loading the dynamic library
# ===============================================================================

assert LIB_PATH.exists()
try:
    dvz = ctypes.cdll.LoadLibrary(str(LIB_PATH))
except Exception as e:
    print(f'Error loading library at {LIB_PATH}: {e}')
    exit(1)

# on macOS, we need to set the VK_DRIVER_FILES environment variable to the path to the MoltenVK ICD
if PLATFORM == 'macos':
    os.environ['VK_DRIVER_FILES'] = str(LIB_DIR / 'MoltenVK_icd.json')


# ===============================================================================
# Util classes
# ===============================================================================


# see https://v4.chriskrycho.com/2015/ctypes-structures-and-dll-exports.html
class CtypesEnum(IntEnum):
    @classmethod
    def from_param(cls, obj):
        return int(obj)


class CStringArrayType:
    @classmethod
    def from_param(cls, value):
        if not isinstance(value, Iterable) or isinstance(value, (str, bytes)):
            raise TypeError('Expected a list of strings')

        encoded = [s.encode('utf-8') for s in value]
        bufs = [ctypes.create_string_buffer(s) for s in encoded]
        arr = (ctypes.c_char_p * len(bufs))(*[ctypes.cast(b, ctypes.c_char_p) for b in bufs])

        # keep references alive
        arr._buffers = bufs
        return arr


class CStringBuffer:
    """
    A tiny helper for C `char*` arguments.

    • You can still pass a plain Python `str` — a temporary buffer will be
      created exactly like before (read‑only use‑case).

    • Or allocate an *in/out* buffer yourself:

          buf = CStringBuffer("initial text", size=256)
          lib.dvz_gui_textbox(b"Label", buf, buf.size, 0)
          print("New text is:", buf.value)

      After the C call,   buf.value   holds the updated UTF‑8 string.
    """

    # -------- allocate an explicit, reusable buffer -----------------
    def __init__(self, initial: str = '', size: Optional[int] = 64):
        if isinstance(initial, Path):
            initial = str(initial)
        if not isinstance(initial, str):
            raise TypeError('Expected a string')

        raw = initial.encode('utf-8')
        size = (len(raw) + 1) if size is None else size
        if size < len(raw) + 1:
            raise ValueError('Buffer too small for initial contents')

        self._buf = ctypes.create_string_buffer(size)
        ctypes.memmove(self._buf, raw, len(raw))  # copy text, keep final NUL

    # -------- read the current value --------------------------------
    @property
    def value(self) -> str:
        """Return the UTF‑8 text currently stored in the buffer."""
        return self._buf.value.decode('utf‑8')

    # convenience: expose total capacity (bytes, inc. NUL)
    @property
    def size(self) -> int:
        return ctypes.sizeof(self._buf)

    # -------- interface for ctypes ----------------------------------
    @classmethod
    def from_param(cls, value):
        """
        Called automatically by ctypes when this object (or a str/Path)
        appears in `argtypes`.
        """
        # 1) already a CStringBuffer → pass its internal buffer
        if isinstance(value, cls):
            return value._buf

        # 2) plain Path or str (read‑only one‑shot)
        if isinstance(value, Path):
            value = str(value)
        if isinstance(value, str):
            buf = ctypes.create_string_buffer(value.encode('utf‑8'))
            buf._keepalive = buf  # prevent premature GC
            return buf

        raise TypeError('Expected CStringBuffer or str')


# ===============================================================================
# Out wrapper
# ===============================================================================


class Out:
    _ctype_map = {
        float: ctypes.c_float,
        int: ctypes.c_int,
        bool: ctypes.c_bool,
    }

    def __init__(self, initial, ctype=None):
        if ctype:
            self._ctype = getattr(ctypes, f'c_{ctype}') if isinstance(ctype, str) else ctype
        else:
            py_type = type(initial)
            if py_type not in self._ctype_map:
                raise TypeError(f'Unsupported type: {py_type}')
            self._ctype = self._ctype_map[py_type]
        self._buffer = self._ctype(initial)

    @property
    def value(self):
        return self._buffer.value

    @value.setter
    def value(self, new_value):
        self._buffer.value = new_value

    def __ctypes_from_outparam__(self):
        return ctypes.byref(self._buffer)

    @classmethod
    def from_param(cls, obj):
        if not isinstance(obj, cls):
            raise TypeError('Expected an Out instance')
        return ctypes.byref(obj._buffer)

    def __format__(self, format_spec):
        return format(self.value, format_spec)

    def __str__(self):
        return f'Out({self.value})'


# ===============================================================================
# Array wrappers
# ===============================================================================


# HACK: accept None ndarrays as arguments, see https://stackoverflow.com/a/37664693/1595060
def ndpointer(*args, **kwargs):
    ndim = kwargs.pop('ndim', None)
    ncol = kwargs.pop('ncol', None)
    base = ndpointer_(*args, **kwargs)

    @classmethod
    def from_param(cls, obj):
        if obj is None:
            return obj
        if isinstance(obj, np.ndarray):
            s = f'array <{obj.dtype}>{obj.shape}'
            if ndim and obj.ndim != ndim:
                raise ValueError(f'Wrong ndim {obj.ndim} (expected {ndim}) for {s}')
            if ncol and ncol > 1 and obj.shape[1] != ncol:
                raise ValueError(f'Wrong shape {obj.shape} (expected (*, {ncol})) for {s}')
            out = base.from_param(obj)
        else:
            # NOTE: allow passing ndpointers without change
            out = obj
        return out

    return type(base.__name__, (base,), {'from_param': from_param})


# ===============================================================================
# Vec types
# ===============================================================================


class CVectorBase(ctypes.Array):
    _type_ = ctypes.c_float
    _length_ = 0
    _name_ = ''

    def __new__(cls, *values):
        values = list(values) + [0] * (cls._length_ - len(values))
        return super().__new__(cls, *values[: cls._length_])

    def __repr__(self):
        vals = ', '.join(f'{v:.6g}' for v in self)
        return f'{self._name_}({vals})'


VEC_TYPES = []


def make_vector_type(name, ctype, length):
    t = type(
        name,
        (CVectorBase,),
        {
            '_type_': ctype,
            '_length_': length,
            '_name_': name,
        },
    )
    VEC_TYPES.append(t)
    return t


vec2 = make_vector_type('vec2', ctypes.c_float, 2)
vec3 = make_vector_type('vec3', ctypes.c_float, 3)
vec4 = make_vector_type('vec4', ctypes.c_float, 4)
dvec2 = make_vector_type('dvec2', ctypes.c_double, 2)
dvec3 = make_vector_type('dvec3', ctypes.c_double, 3)
dvec4 = make_vector_type('dvec4', ctypes.c_double, 4)
cvec4 = make_vector_type('cvec4', ctypes.c_uint8, 4)
uvec2 = make_vector_type('uvec2', ctypes.c_uint32, 2)
uvec3 = make_vector_type('uvec3', ctypes.c_uint32, 3)
uvec4 = make_vector_type('uvec4', ctypes.c_uint32, 4)
ivec2 = make_vector_type('ivec2', ctypes.c_int32, 2)
ivec3 = make_vector_type('ivec3', ctypes.c_int32, 3)
ivec4 = make_vector_type('ivec4', ctypes.c_int32, 4)
mat3 = make_vector_type('mat3', ctypes.c_float, 3 * 3)
mat4 = make_vector_type('mat4', ctypes.c_float, 4 * 4)


# ===============================================================================
# Aliases
# ===============================================================================

DvzId = ctypes.c_uint64
DvzSize = ctypes.c_uint64
DvzShaderStageFlags = ctypes.c_int32


# HACK: mock structs for Qt/Vulkan wrappers
class QApplication(ctypes.Structure):
    pass


class VkInstance(ctypes.Structure):
    pass


class VkDevice(ctypes.Structure):
    pass


class VkFramebuffer(ctypes.Structure):
    pass


class VkRenderPass(ctypes.Structure):
    pass


class VkCommandBuffer(ctypes.Structure):
    pass


DEFAULT_CLEAR_COLOR = cvec4()
DEFAULT_VIEWPORT = vec2()
ALPHA_MAX = 255
DVZ_COLOR_CVEC4 = 1
DvzColor = cvec4
DvzAlpha = ctypes.c_uint8


# ===============================================================================
# DEFINES
# ===============================================================================

DVZ_DEFAULT_FORMAT = 44
DVZ_BUFFER_TYPE_COUNT = 6
CMAP_NAT = 144
CMAP_USR_OFS = 144
CMAP_USR = 32
CMAP_TOT = 176
CPAL256_OFS = 176
CPAL256_NAT = 32
CPAL256_USR_OFS = 208
CPAL256_USR = 32
CPAL256_TOT = 64
CPAL032_OFS = 240
CPAL032_NAT = 8
CPAL032_USR_OFS = 248
CPAL032_USR = 8
CPAL032_PER_ROW = 8
CPAL032_SIZ = 32
CPAL032_TOT = 16
CMAP_COUNT = 256
CMAP_CUSTOM_COUNT = 16
CMAP_CUSTOM = 160
CPAL256_CUSTOM = 224
COLOR_UINT_MAX = 255
COLOR_FLOAT_MAX = 1.0
COLOR_MAX = 1.0
M_PI = 3.141592653589793
M_2PI = 6.283185307179586
M_INV_255 = 0.00392156862745098
EPSILON = 1e-10
GB = 1073741824
MB = 1048576
KB = 1024
DVZ_REQUEST_VERSION = 1
DVZ_BATCH_DEFAULT_CAPACITY = 4
DVZ_VERSION_MINOR = 3
DVZ_VERSION_PATCH = 1


# ===============================================================================
# ENUMERATIONS
# ===============================================================================

class DvzAppFlags(CtypesEnum):
    DVZ_APP_FLAGS_NONE = 0x000000
    DVZ_APP_FLAGS_OFFSCREEN = 0x008000
    DVZ_APP_FLAGS_WHITE_BACKGROUND = 0x100000


class DvzCanvasFlags(CtypesEnum):
    DVZ_CANVAS_FLAGS_NONE = 0x0000
    DVZ_CANVAS_FLAGS_IMGUI = 0x0001
    DVZ_CANVAS_FLAGS_FPS = 0x0003
    DVZ_CANVAS_FLAGS_MONITOR = 0x0005
    DVZ_CANVAS_FLAGS_FULLSCREEN = 0x0008
    DVZ_CANVAS_FLAGS_VSYNC = 0x0010
    DVZ_CANVAS_FLAGS_PICK = 0x0020
    DVZ_CANVAS_FLAGS_PUSH_SCALE = 0x0040


class DvzKeyboardModifiers(CtypesEnum):
    DVZ_KEY_MODIFIER_NONE = 0x00000000
    DVZ_KEY_MODIFIER_SHIFT = 0x00000001
    DVZ_KEY_MODIFIER_CONTROL = 0x00000002
    DVZ_KEY_MODIFIER_ALT = 0x00000004
    DVZ_KEY_MODIFIER_SUPER = 0x00000008


class DvzKeyboardEventType(CtypesEnum):
    DVZ_KEYBOARD_EVENT_NONE = 0
    DVZ_KEYBOARD_EVENT_PRESS = 1
    DVZ_KEYBOARD_EVENT_REPEAT = 2
    DVZ_KEYBOARD_EVENT_RELEASE = 3


class DvzMouseButton(CtypesEnum):
    DVZ_MOUSE_BUTTON_NONE = 0
    DVZ_MOUSE_BUTTON_LEFT = 1
    DVZ_MOUSE_BUTTON_MIDDLE = 2
    DVZ_MOUSE_BUTTON_RIGHT = 3


class DvzMouseState(CtypesEnum):
    DVZ_MOUSE_STATE_RELEASE = 0
    DVZ_MOUSE_STATE_PRESS = 1
    DVZ_MOUSE_STATE_CLICK = 3
    DVZ_MOUSE_STATE_CLICK_PRESS = 4
    DVZ_MOUSE_STATE_DOUBLE_CLICK = 5
    DVZ_MOUSE_STATE_DRAGGING = 11


class DvzMouseEventType(CtypesEnum):
    DVZ_MOUSE_EVENT_RELEASE = 0
    DVZ_MOUSE_EVENT_PRESS = 1
    DVZ_MOUSE_EVENT_MOVE = 2
    DVZ_MOUSE_EVENT_CLICK = 3
    DVZ_MOUSE_EVENT_DOUBLE_CLICK = 5
    DVZ_MOUSE_EVENT_DRAG_START = 10
    DVZ_MOUSE_EVENT_DRAG = 11
    DVZ_MOUSE_EVENT_DRAG_STOP = 12
    DVZ_MOUSE_EVENT_WHEEL = 20
    DVZ_MOUSE_EVENT_ALL = 255


class DvzGuiFlags(CtypesEnum):
    DVZ_GUI_FLAGS_NONE = 0x0000
    DVZ_GUI_FLAGS_OFFSCREEN = 0x0001
    DVZ_GUI_FLAGS_DOCKING = 0x0010


class DvzDialogFlags(CtypesEnum):
    DVZ_DIALOG_FLAGS_NONE = 0x0000
    DVZ_DIALOG_FLAGS_OVERLAY = 0x0001
    DVZ_DIALOG_FLAGS_BLANK = 0x0004
    DVZ_DIALOG_FLAGS_PANEL = 0x0008


class DvzCorner(CtypesEnum):
    DVZ_DIALOG_CORNER_TOP_LEFT = 0
    DVZ_DIALOG_CORNER_TOP_RIGHT = 1
    DVZ_DIALOG_CORNER_BOTTOM_LEFT = 2
    DVZ_DIALOG_CORNER_BOTTOM_RIGHT = 3


class DvzDim(CtypesEnum):
    DVZ_DIM_X = 0x0000
    DVZ_DIM_Y = 0x0001
    DVZ_DIM_Z = 0x0002
    DVZ_DIM_COUNT = 3


class DvzRefFlags(CtypesEnum):
    DVZ_REF_FLAGS_NONE = 0x00
    DVZ_REF_FLAGS_EQUAL = 0x01


class DvzAxisFlags(CtypesEnum):
    DVZ_AXIS_FLAGS_NONE = 0x00
    DVZ_AXIS_FLAGS_DARK = 0x01


class DvzArcballFlags(CtypesEnum):
    DVZ_ARCBALL_FLAGS_NONE = 0
    DVZ_ARCBALL_FLAGS_CONSTRAIN = 1


class DvzFlyFlags(CtypesEnum):
    DVZ_FLY_FLAGS_NONE = 0x0000
    DVZ_FLY_FLAGS_INVERT_MOUSE = 0x0001
    DVZ_FLY_FLAGS_FIXED_UP = 0x0002


class DvzPanzoomFlags(CtypesEnum):
    DVZ_PANZOOM_FLAGS_NONE = 0x00
    DVZ_PANZOOM_FLAGS_KEEP_ASPECT = 0x01
    DVZ_PANZOOM_FLAGS_FIXED_X = 0x10
    DVZ_PANZOOM_FLAGS_FIXED_Y = 0x20


class DvzCameraFlags(CtypesEnum):
    DVZ_CAMERA_FLAGS_PERSPECTIVE = 0x00
    DVZ_CAMERA_FLAGS_ORTHO = 0x01


class DvzVisualFlags(CtypesEnum):
    DVZ_VISUAL_FLAGS_DEFAULT = 0x000000
    DVZ_VISUAL_FLAGS_INDEXED = 0x010000
    DVZ_VISUAL_FLAGS_INDIRECT = 0x020000
    DVZ_VISUAL_FLAGS_FIXED_X = 0x001000
    DVZ_VISUAL_FLAGS_FIXED_Y = 0x002000
    DVZ_VISUAL_FLAGS_FIXED_Z = 0x004000
    DVZ_VISUAL_FLAGS_FIXED_ALL = 0x007000
    DVZ_VISUAL_FLAGS_VERTEX_MAPPABLE = 0x400000
    DVZ_VISUAL_FLAGS_INDEX_MAPPABLE = 0x800000


class DvzViewFlags(CtypesEnum):
    DVZ_VIEW_FLAGS_NONE = 0x0000
    DVZ_VIEW_FLAGS_STATIC = 0x0010
    DVZ_VIEW_FLAGS_NOCLIP = 0x0020


class DvzPanelLinkFlags(CtypesEnum):
    DVZ_PANEL_LINK_FLAGS_NONE = 0x00
    DVZ_PANEL_LINK_FLAGS_MODEL = 0x01
    DVZ_PANEL_LINK_FLAGS_VIEW = 0x02
    DVZ_PANEL_LINK_FLAGS_PROJECTION = 0x04


class DvzDatFlags(CtypesEnum):
    DVZ_DAT_FLAGS_NONE = 0x0000
    DVZ_DAT_FLAGS_STANDALONE = 0x0100
    DVZ_DAT_FLAGS_MAPPABLE = 0x0200
    DVZ_DAT_FLAGS_DUP = 0x0400
    DVZ_DAT_FLAGS_KEEP_ON_RESIZE = 0x1000
    DVZ_DAT_FLAGS_PERSISTENT_STAGING = 0x2000


class DvzUploadFlags(CtypesEnum):
    DVZ_UPLOAD_FLAGS_NOCOPY = 0x0800


class DvzTexFlags(CtypesEnum):
    DVZ_TEX_FLAGS_NONE = 0x0000
    DVZ_TEX_FLAGS_PERSISTENT_STAGING = 0x2000


class DvzFontFlags(CtypesEnum):
    DVZ_FONT_FLAGS_RGB = 0
    DVZ_FONT_FLAGS_RGBA = 1


class DvzMockFlags(CtypesEnum):
    DVZ_MOCK_FLAGS_NONE = 0x00
    DVZ_MOCK_FLAGS_CLOSED = 0x01


class DvzTexDims(CtypesEnum):
    DVZ_TEX_NONE = 0
    DVZ_TEX_1D = 1
    DVZ_TEX_2D = 2
    DVZ_TEX_3D = 3


class DvzGraphicsType(CtypesEnum):
    DVZ_GRAPHICS_NONE = 0
    DVZ_GRAPHICS_POINT = 1
    DVZ_GRAPHICS_TRIANGLE = 2
    DVZ_GRAPHICS_CUSTOM = 3


class DvzRecorderCommandType(CtypesEnum):
    DVZ_RECORDER_NONE = 0
    DVZ_RECORDER_BEGIN = 1
    DVZ_RECORDER_DRAW = 2
    DVZ_RECORDER_DRAW_INDEXED = 3
    DVZ_RECORDER_DRAW_INDIRECT = 4
    DVZ_RECORDER_DRAW_INDEXED_INDIRECT = 5
    DVZ_RECORDER_VIEWPORT = 6
    DVZ_RECORDER_PUSH = 7
    DVZ_RECORDER_END = 8
    DVZ_RECORDER_COUNT = 9


class DvzRequestAction(CtypesEnum):
    DVZ_REQUEST_ACTION_NONE = 0
    DVZ_REQUEST_ACTION_CREATE = 1
    DVZ_REQUEST_ACTION_DELETE = 2
    DVZ_REQUEST_ACTION_RESIZE = 3
    DVZ_REQUEST_ACTION_UPDATE = 4
    DVZ_REQUEST_ACTION_BIND = 5
    DVZ_REQUEST_ACTION_RECORD = 6
    DVZ_REQUEST_ACTION_UPLOAD = 7
    DVZ_REQUEST_ACTION_UPFILL = 8
    DVZ_REQUEST_ACTION_DOWNLOAD = 9
    DVZ_REQUEST_ACTION_SET = 10
    DVZ_REQUEST_ACTION_GET = 11


class DvzRequestObject(CtypesEnum):
    DVZ_REQUEST_OBJECT_NONE = 0
    DVZ_REQUEST_OBJECT_CANVAS = 101
    DVZ_REQUEST_OBJECT_DAT = 2
    DVZ_REQUEST_OBJECT_TEX = 3
    DVZ_REQUEST_OBJECT_SAMPLER = 4
    DVZ_REQUEST_OBJECT_COMPUTE = 5
    DVZ_REQUEST_OBJECT_PRIMITIVE = 6
    DVZ_REQUEST_OBJECT_DEPTH = 7
    DVZ_REQUEST_OBJECT_BLEND = 8
    DVZ_REQUEST_OBJECT_MASK = 9
    DVZ_REQUEST_OBJECT_POLYGON = 10
    DVZ_REQUEST_OBJECT_CULL = 11
    DVZ_REQUEST_OBJECT_FRONT = 12
    DVZ_REQUEST_OBJECT_SHADER = 13
    DVZ_REQUEST_OBJECT_VERTEX = 14
    DVZ_REQUEST_OBJECT_VERTEX_ATTR = 15
    DVZ_REQUEST_OBJECT_SLOT = 16
    DVZ_REQUEST_OBJECT_PUSH = 17
    DVZ_REQUEST_OBJECT_SPECIALIZATION = 18
    DVZ_REQUEST_OBJECT_GRAPHICS = 19
    DVZ_REQUEST_OBJECT_INDEX = 20
    DVZ_REQUEST_OBJECT_BACKGROUND = 21
    DVZ_REQUEST_OBJECT_RECORD = 22


class DvzPrimitiveTopology(CtypesEnum):
    DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST = 0
    DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST = 1
    DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP = 2
    DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST = 3
    DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP = 4
    DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN = 5


class DvzFormat(CtypesEnum):
    DVZ_FORMAT_NONE = 0
    DVZ_FORMAT_R8_UNORM = 9
    DVZ_FORMAT_R8_SNORM = 10
    DVZ_FORMAT_R8_UINT = 13
    DVZ_FORMAT_R8_SINT = 14
    DVZ_FORMAT_R8G8_UNORM = 16
    DVZ_FORMAT_R8G8_SNORM = 17
    DVZ_FORMAT_R8G8_UINT = 20
    DVZ_FORMAT_R8G8_SINT = 21
    DVZ_FORMAT_R8G8B8_UNORM = 23
    DVZ_FORMAT_R8G8B8_SNORM = 24
    DVZ_FORMAT_R8G8B8_UINT = 27
    DVZ_FORMAT_R8G8B8_SINT = 28
    DVZ_FORMAT_R8G8B8A8_UNORM = 37
    DVZ_FORMAT_R8G8B8A8_SNORM = 38
    DVZ_FORMAT_R8G8B8A8_UINT = 41
    DVZ_FORMAT_R8G8B8A8_SINT = 42
    DVZ_FORMAT_B8G8R8A8_UNORM = 44
    DVZ_FORMAT_R16_UNORM = 70
    DVZ_FORMAT_R16_SNORM = 71
    DVZ_FORMAT_R32_UINT = 98
    DVZ_FORMAT_R32_SINT = 99
    DVZ_FORMAT_R32_SFLOAT = 100
    DVZ_FORMAT_R32G32_UINT = 101
    DVZ_FORMAT_R32G32_SINT = 102
    DVZ_FORMAT_R32G32_SFLOAT = 103
    DVZ_FORMAT_R32G32B32_UINT = 104
    DVZ_FORMAT_R32G32B32_SINT = 105
    DVZ_FORMAT_R32G32B32_SFLOAT = 106
    DVZ_FORMAT_R32G32B32A32_UINT = 107
    DVZ_FORMAT_R32G32B32A32_SINT = 108
    DVZ_FORMAT_R32G32B32A32_SFLOAT = 109
    DVZ_FORMAT_R64_UINT = 110
    DVZ_FORMAT_R64_SINT = 111
    DVZ_FORMAT_R64_SFLOAT = 112
    DVZ_FORMAT_R64G64_UINT = 113
    DVZ_FORMAT_R64G64_SINT = 114
    DVZ_FORMAT_R64G64_SFLOAT = 115
    DVZ_FORMAT_R64G64B64_UINT = 116
    DVZ_FORMAT_R64G64B64_SINT = 117
    DVZ_FORMAT_R64G64B64_SFLOAT = 118
    DVZ_FORMAT_R64G64B64A64_UINT = 119
    DVZ_FORMAT_R64G64B64A64_SINT = 120
    DVZ_FORMAT_R64G64B64A64_SFLOAT = 121


class DvzColorMask(CtypesEnum):
    DVZ_MASK_COLOR_R = 0x00000001
    DVZ_MASK_COLOR_G = 0x00000002
    DVZ_MASK_COLOR_B = 0x00000004
    DVZ_MASK_COLOR_A = 0x00000008
    DVZ_MASK_COLOR_ALL = 0x0000000F


class DvzFilter(CtypesEnum):
    DVZ_FILTER_NEAREST = 0
    DVZ_FILTER_LINEAR = 1
    DVZ_FILTER_CUBIC_IMG = 1000015000


class DvzSamplerAddressMode(CtypesEnum):
    DVZ_SAMPLER_ADDRESS_MODE_REPEAT = 0
    DVZ_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT = 1
    DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE = 2
    DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER = 3
    DVZ_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE = 4


class DvzVertexInputRate(CtypesEnum):
    DVZ_VERTEX_INPUT_RATE_VERTEX = 0
    DVZ_VERTEX_INPUT_RATE_INSTANCE = 1


class DvzPolygonMode(CtypesEnum):
    DVZ_POLYGON_MODE_FILL = 0
    DVZ_POLYGON_MODE_LINE = 1
    DVZ_POLYGON_MODE_POINT = 2


class DvzFrontFace(CtypesEnum):
    DVZ_FRONT_FACE_COUNTER_CLOCKWISE = 0
    DVZ_FRONT_FACE_CLOCKWISE = 1


class DvzCullMode(CtypesEnum):
    DVZ_CULL_MODE_NONE = 0
    DVZ_CULL_MODE_FRONT = 0x00000001
    DVZ_CULL_MODE_BACK = 0x00000002


class DvzShaderType(CtypesEnum):
    DVZ_SHADER_VERTEX = 0x00000001
    DVZ_SHADER_TESSELLATION_CONTROL = 0x00000002
    DVZ_SHADER_TESSELLATION_EVALUATION = 0x00000004
    DVZ_SHADER_GEOMETRY = 0x00000008
    DVZ_SHADER_FRAGMENT = 0x00000010
    DVZ_SHADER_COMPUTE = 0x00000020


class DvzDescriptorType(CtypesEnum):
    DVZ_DESCRIPTOR_TYPE_SAMPLER = 0
    DVZ_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1
    DVZ_DESCRIPTOR_TYPE_SAMPLED_IMAGE = 2
    DVZ_DESCRIPTOR_TYPE_STORAGE_IMAGE = 3
    DVZ_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER = 4
    DVZ_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER = 5
    DVZ_DESCRIPTOR_TYPE_UNIFORM_BUFFER = 6
    DVZ_DESCRIPTOR_TYPE_STORAGE_BUFFER = 7
    DVZ_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC = 8
    DVZ_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC = 9


class DvzBufferType(CtypesEnum):
    DVZ_BUFFER_TYPE_UNDEFINED = 0
    DVZ_BUFFER_TYPE_STAGING = 1
    DVZ_BUFFER_TYPE_VERTEX = 2
    DVZ_BUFFER_TYPE_INDEX = 3
    DVZ_BUFFER_TYPE_STORAGE = 4
    DVZ_BUFFER_TYPE_UNIFORM = 5
    DVZ_BUFFER_TYPE_INDIRECT = 6


class DvzShaderFormat(CtypesEnum):
    DVZ_SHADER_NONE = 0
    DVZ_SHADER_SPIRV = 1
    DVZ_SHADER_GLSL = 2


class DvzSamplerAxis(CtypesEnum):
    DVZ_SAMPLER_AXIS_U = 0
    DVZ_SAMPLER_AXIS_V = 1
    DVZ_SAMPLER_AXIS_W = 2


class DvzBlendType(CtypesEnum):
    DVZ_BLEND_DISABLE = 0
    DVZ_BLEND_STANDARD = 1
    DVZ_BLEND_DESTINATION = 2
    DVZ_BLEND_OIT = 3


class DvzSlotType(CtypesEnum):
    DVZ_SLOT_DAT = 0
    DVZ_SLOT_TEX = 1
    DVZ_SLOT_COUNT = 2


class DvzMarkerShape(CtypesEnum):
    DVZ_MARKER_SHAPE_DISC = 0
    DVZ_MARKER_SHAPE_ASTERISK = 1
    DVZ_MARKER_SHAPE_CHEVRON = 2
    DVZ_MARKER_SHAPE_CLOVER = 3
    DVZ_MARKER_SHAPE_CLUB = 4
    DVZ_MARKER_SHAPE_CROSS = 5
    DVZ_MARKER_SHAPE_DIAMOND = 6
    DVZ_MARKER_SHAPE_ARROW = 7
    DVZ_MARKER_SHAPE_ELLIPSE = 8
    DVZ_MARKER_SHAPE_HBAR = 9
    DVZ_MARKER_SHAPE_HEART = 10
    DVZ_MARKER_SHAPE_INFINITY = 11
    DVZ_MARKER_SHAPE_PIN = 12
    DVZ_MARKER_SHAPE_RING = 13
    DVZ_MARKER_SHAPE_SPADE = 14
    DVZ_MARKER_SHAPE_SQUARE = 15
    DVZ_MARKER_SHAPE_TAG = 16
    DVZ_MARKER_SHAPE_TRIANGLE = 17
    DVZ_MARKER_SHAPE_VBAR = 18
    DVZ_MARKER_SHAPE_ROUNDED_RECT = 19
    DVZ_MARKER_SHAPE_COUNT = 20


class DvzMarkerMode(CtypesEnum):
    DVZ_MARKER_MODE_NONE = 0
    DVZ_MARKER_MODE_CODE = 1
    DVZ_MARKER_MODE_BITMAP = 2
    DVZ_MARKER_MODE_SDF = 3
    DVZ_MARKER_MODE_MSDF = 4
    DVZ_MARKER_MODE_MTSDF = 5


class DvzMarkerAspect(CtypesEnum):
    DVZ_MARKER_ASPECT_FILLED = 0
    DVZ_MARKER_ASPECT_STROKE = 1
    DVZ_MARKER_ASPECT_OUTLINE = 2


class DvzCapType(CtypesEnum):
    DVZ_CAP_NONE = 0
    DVZ_CAP_ROUND = 1
    DVZ_CAP_TRIANGLE_IN = 2
    DVZ_CAP_TRIANGLE_OUT = 3
    DVZ_CAP_SQUARE = 4
    DVZ_CAP_BUTT = 5
    DVZ_CAP_COUNT = 6


class DvzJoinType(CtypesEnum):
    DVZ_JOIN_SQUARE = 0
    DVZ_JOIN_ROUND = 1


class DvzPathFlags(CtypesEnum):
    DVZ_PATH_FLAGS_OPEN = 0
    DVZ_PATH_FLAGS_CLOSED = 1


class DvzImageFlags(CtypesEnum):
    DVZ_IMAGE_FLAGS_SIZE_PIXELS = 0x0000
    DVZ_IMAGE_FLAGS_SIZE_NDC = 0x0001
    DVZ_IMAGE_FLAGS_RESCALE_KEEP_RATIO = 0x0004
    DVZ_IMAGE_FLAGS_RESCALE = 0x0008
    DVZ_IMAGE_FLAGS_MODE_RGBA = 0x0000
    DVZ_IMAGE_FLAGS_MODE_COLORMAP = 0x0010
    DVZ_IMAGE_FLAGS_MODE_FILL = 0x0020
    DVZ_IMAGE_FLAGS_BORDER = 0x0080


class DvzShapeType(CtypesEnum):
    DVZ_SHAPE_NONE = 0
    DVZ_SHAPE_SQUARE = 1
    DVZ_SHAPE_DISC = 2
    DVZ_SHAPE_SECTOR = 3
    DVZ_SHAPE_POLYGON = 4
    DVZ_SHAPE_HISTOGRAM = 5
    DVZ_SHAPE_CUBE = 6
    DVZ_SHAPE_SPHERE = 7
    DVZ_SHAPE_CYLINDER = 8
    DVZ_SHAPE_CONE = 9
    DVZ_SHAPE_TORUS = 10
    DVZ_SHAPE_ARROW = 11
    DVZ_SHAPE_TETRAHEDRON = 12
    DVZ_SHAPE_HEXAHEDRON = 13
    DVZ_SHAPE_OCTAHEDRON = 14
    DVZ_SHAPE_DODECAHEDRON = 15
    DVZ_SHAPE_ICOSAHEDRON = 16
    DVZ_SHAPE_SURFACE = 17
    DVZ_SHAPE_OBJ = 18
    DVZ_SHAPE_OTHER = 19


class DvzContourFlags(CtypesEnum):
    DVZ_CONTOUR_NONE = 0x00
    DVZ_CONTOUR_EDGES = 0x01
    DVZ_CONTOUR_JOINTS = 0x02
    DVZ_CONTOUR_FULL = 0x04


class DvzShapeIndexingFlags(CtypesEnum):
    DVZ_INDEXING_NONE = 0x00
    DVZ_INDEXING_EARCUT = 0x10
    DVZ_INDEXING_SURFACE = 0x20


class DvzSphereFlags(CtypesEnum):
    DVZ_SPHERE_FLAGS_NONE = 0x0000
    DVZ_SPHERE_FLAGS_TEXTURED = 0x0001
    DVZ_SPHERE_FLAGS_LIGHTING = 0x0002
    DVZ_SPHERE_FLAGS_SIZE_PIXELS = 0x0004
    DVZ_SPHERE_FLAGS_EQUAL_RECTANGULAR = 0x0008


class DvzMeshFlags(CtypesEnum):
    DVZ_MESH_FLAGS_NONE = 0x0000
    DVZ_MESH_FLAGS_TEXTURED = 0x0001
    DVZ_MESH_FLAGS_LIGHTING = 0x0002
    DVZ_MESH_FLAGS_CONTOUR = 0x0004
    DVZ_MESH_FLAGS_ISOLINE = 0x0008


class DvzVolumeFlags(CtypesEnum):
    DVZ_VOLUME_FLAGS_NONE = 0x0000
    DVZ_VOLUME_FLAGS_RGBA = 0x0001
    DVZ_VOLUME_FLAGS_COLORMAP = 0x0002
    DVZ_VOLUME_FLAGS_BACK_FRONT = 0x0004


class DvzEasing(CtypesEnum):
    DVZ_EASING_NONE = 0
    DVZ_EASING_IN_SINE = 1
    DVZ_EASING_OUT_SINE = 2
    DVZ_EASING_IN_OUT_SINE = 3
    DVZ_EASING_IN_QUAD = 4
    DVZ_EASING_OUT_QUAD = 5
    DVZ_EASING_IN_OUT_QUAD = 6
    DVZ_EASING_IN_CUBIC = 7
    DVZ_EASING_OUT_CUBIC = 8
    DVZ_EASING_IN_OUT_CUBIC = 9
    DVZ_EASING_IN_QUART = 10
    DVZ_EASING_OUT_QUART = 11
    DVZ_EASING_IN_OUT_QUART = 12
    DVZ_EASING_IN_QUINT = 13
    DVZ_EASING_OUT_QUINT = 14
    DVZ_EASING_IN_OUT_QUINT = 15
    DVZ_EASING_IN_EXPO = 16
    DVZ_EASING_OUT_EXPO = 17
    DVZ_EASING_IN_OUT_EXPO = 18
    DVZ_EASING_IN_CIRC = 19
    DVZ_EASING_OUT_CIRC = 20
    DVZ_EASING_IN_OUT_CIRC = 21
    DVZ_EASING_IN_BACK = 22
    DVZ_EASING_OUT_BACK = 23
    DVZ_EASING_IN_OUT_BACK = 24
    DVZ_EASING_IN_ELASTIC = 25
    DVZ_EASING_OUT_ELASTIC = 26
    DVZ_EASING_IN_OUT_ELASTIC = 27
    DVZ_EASING_IN_BOUNCE = 28
    DVZ_EASING_OUT_BOUNCE = 29
    DVZ_EASING_IN_OUT_BOUNCE = 30
    DVZ_EASING_COUNT = 31


class DvzBoxExtentStrategy(CtypesEnum):
    DVZ_BOX_EXTENT_DEFAULT = 0
    DVZ_BOX_EXTENT_FIXED_ASPECT_EXPAND = 1
    DVZ_BOX_EXTENT_FIXED_ASPECT_CONTRACT = 2


class DvzBoxMergeStrategy(CtypesEnum):
    DVZ_BOX_MERGE_DEFAULT = 0
    DVZ_BOX_MERGE_CENTER = 1
    DVZ_BOX_MERGE_CORNER = 2


class DvzViewportClip(CtypesEnum):
    DVZ_VIEWPORT_CLIP_INNER = 0x0001
    DVZ_VIEWPORT_CLIP_OUTER = 0x0002
    DVZ_VIEWPORT_CLIP_BOTTOM = 0x0004
    DVZ_VIEWPORT_CLIP_LEFT = 0x0008


class DvzDepthTest(CtypesEnum):
    DVZ_DEPTH_TEST_DISABLE = 0
    DVZ_DEPTH_TEST_ENABLE = 1


class DvzAlign(CtypesEnum):
    DVZ_ALIGN_NONE = 0
    DVZ_ALIGN_LOW = 1
    DVZ_ALIGN_MIDDLE = 2
    DVZ_ALIGN_HIGH = 3


class DvzOrientation(CtypesEnum):
    DVZ_ORIENTATION_DEFAULT = 0
    DVZ_ORIENTATION_UP = 1
    DVZ_ORIENTATION_REVERSE = 2
    DVZ_ORIENTATION_DOWN = 3


class DvzSceneFont(CtypesEnum):
    DVZ_SCENE_FONT_MONO = 0
    DVZ_SCENE_FONT_LABEL = 1
    DVZ_SCENE_FONT_COUNT = 2


class DvzColormap(CtypesEnum):
    DVZ_CMAP_BINARY = 0
    DVZ_CMAP_HSV = 1
    DVZ_CMAP_CIVIDIS = 2
    DVZ_CMAP_INFERNO = 3
    DVZ_CMAP_MAGMA = 4
    DVZ_CMAP_PLASMA = 5
    DVZ_CMAP_VIRIDIS = 6
    DVZ_CMAP_BLUES = 7
    DVZ_CMAP_BUGN = 8
    DVZ_CMAP_BUPU = 9
    DVZ_CMAP_GNBU = 10
    DVZ_CMAP_GREENS = 11
    DVZ_CMAP_GREYS = 12
    DVZ_CMAP_ORANGES = 13
    DVZ_CMAP_ORRD = 14
    DVZ_CMAP_PUBU = 15
    DVZ_CMAP_PUBUGN = 16
    DVZ_CMAP_PURPLES = 17
    DVZ_CMAP_RDPU = 18
    DVZ_CMAP_REDS = 19
    DVZ_CMAP_YLGN = 20
    DVZ_CMAP_YLGNBU = 21
    DVZ_CMAP_YLORBR = 22
    DVZ_CMAP_YLORRD = 23
    DVZ_CMAP_AFMHOT = 24
    DVZ_CMAP_AUTUMN = 25
    DVZ_CMAP_BONE = 26
    DVZ_CMAP_COOL = 27
    DVZ_CMAP_COPPER = 28
    DVZ_CMAP_GIST_HEAT = 29
    DVZ_CMAP_GRAY = 30
    DVZ_CMAP_HOT = 31
    DVZ_CMAP_PINK = 32
    DVZ_CMAP_SPRING = 33
    DVZ_CMAP_SUMMER = 34
    DVZ_CMAP_WINTER = 35
    DVZ_CMAP_WISTIA = 36
    DVZ_CMAP_BRBG = 37
    DVZ_CMAP_BWR = 38
    DVZ_CMAP_COOLWARM = 39
    DVZ_CMAP_PIYG = 40
    DVZ_CMAP_PRGN = 41
    DVZ_CMAP_PUOR = 42
    DVZ_CMAP_RDBU = 43
    DVZ_CMAP_RDGY = 44
    DVZ_CMAP_RDYLBU = 45
    DVZ_CMAP_RDYLGN = 46
    DVZ_CMAP_SEISMIC = 47
    DVZ_CMAP_SPECTRAL = 48
    DVZ_CMAP_TWILIGHT_SHIFTED = 49
    DVZ_CMAP_TWILIGHT = 50
    DVZ_CMAP_BRG = 51
    DVZ_CMAP_CMRMAP = 52
    DVZ_CMAP_CUBEHELIX = 53
    DVZ_CMAP_FLAG = 54
    DVZ_CMAP_GIST_EARTH = 55
    DVZ_CMAP_GIST_NCAR = 56
    DVZ_CMAP_GIST_RAINBOW = 57
    DVZ_CMAP_GIST_STERN = 58
    DVZ_CMAP_GNUPLOT2 = 59
    DVZ_CMAP_GNUPLOT = 60
    DVZ_CMAP_JET = 61
    DVZ_CMAP_NIPY_SPECTRAL = 62
    DVZ_CMAP_OCEAN = 63
    DVZ_CMAP_PRISM = 64
    DVZ_CMAP_RAINBOW = 65
    DVZ_CMAP_TERRAIN = 66
    DVZ_CMAP_BKR = 67
    DVZ_CMAP_BKY = 68
    DVZ_CMAP_CET_D10 = 69
    DVZ_CMAP_CET_D11 = 70
    DVZ_CMAP_CET_D8 = 71
    DVZ_CMAP_CET_D13 = 72
    DVZ_CMAP_CET_D3 = 73
    DVZ_CMAP_CET_D1A = 74
    DVZ_CMAP_BJY = 75
    DVZ_CMAP_GWV = 76
    DVZ_CMAP_BWY = 77
    DVZ_CMAP_CET_D12 = 78
    DVZ_CMAP_CET_R3 = 79
    DVZ_CMAP_CET_D9 = 80
    DVZ_CMAP_CWR = 81
    DVZ_CMAP_CET_CBC1 = 82
    DVZ_CMAP_CET_CBC2 = 83
    DVZ_CMAP_CET_CBL1 = 84
    DVZ_CMAP_CET_CBL2 = 85
    DVZ_CMAP_CET_CBTC1 = 86
    DVZ_CMAP_CET_CBTC2 = 87
    DVZ_CMAP_CET_CBTL1 = 88
    DVZ_CMAP_BGY = 89
    DVZ_CMAP_BGYW = 90
    DVZ_CMAP_BMW = 91
    DVZ_CMAP_CET_C1 = 92
    DVZ_CMAP_CET_C1S = 93
    DVZ_CMAP_CET_C2 = 94
    DVZ_CMAP_CET_C4 = 95
    DVZ_CMAP_CET_C4S = 96
    DVZ_CMAP_CET_C5 = 97
    DVZ_CMAP_CET_I1 = 98
    DVZ_CMAP_CET_I3 = 99
    DVZ_CMAP_CET_L10 = 100
    DVZ_CMAP_CET_L11 = 101
    DVZ_CMAP_CET_L12 = 102
    DVZ_CMAP_CET_L16 = 103
    DVZ_CMAP_CET_L17 = 104
    DVZ_CMAP_CET_L18 = 105
    DVZ_CMAP_CET_L19 = 106
    DVZ_CMAP_CET_L4 = 107
    DVZ_CMAP_CET_L7 = 108
    DVZ_CMAP_CET_L8 = 109
    DVZ_CMAP_CET_L9 = 110
    DVZ_CMAP_CET_R1 = 111
    DVZ_CMAP_CET_R2 = 112
    DVZ_CMAP_COLORWHEEL = 113
    DVZ_CMAP_FIRE = 114
    DVZ_CMAP_ISOLUM = 115
    DVZ_CMAP_KB = 116
    DVZ_CMAP_KBC = 117
    DVZ_CMAP_KG = 118
    DVZ_CMAP_KGY = 119
    DVZ_CMAP_KR = 120
    DVZ_CMAP_BLACK_BODY = 121
    DVZ_CMAP_KINDLMANN = 122
    DVZ_CMAP_EXTENDED_KINDLMANN = 123
    DVZ_CPAL256_GLASBEY = CPAL256_OFS
    DVZ_CPAL256_GLASBEY_COOL = 125
    DVZ_CPAL256_GLASBEY_DARK = 126
    DVZ_CPAL256_GLASBEY_HV = 127
    DVZ_CPAL256_GLASBEY_LIGHT = 128
    DVZ_CPAL256_GLASBEY_WARM = 129
    DVZ_CPAL032_ACCENT = CPAL032_OFS
    DVZ_CPAL032_DARK2 = 131
    DVZ_CPAL032_PAIRED = 132
    DVZ_CPAL032_PASTEL1 = 133
    DVZ_CPAL032_PASTEL2 = 134
    DVZ_CPAL032_SET1 = 135
    DVZ_CPAL032_SET2 = 136
    DVZ_CPAL032_SET3 = 137
    DVZ_CPAL032_TAB10 = 138
    DVZ_CPAL032_TAB20 = 139
    DVZ_CPAL032_TAB20B = 140
    DVZ_CPAL032_TAB20C = 141
    DVZ_CPAL032_CATEGORY10_10 = 142
    DVZ_CPAL032_CATEGORY20_20 = 143
    DVZ_CPAL032_CATEGORY20B_20 = 144
    DVZ_CPAL032_CATEGORY20C_20 = 145
    DVZ_CPAL032_COLORBLIND8 = 146


class DvzKeyCode(CtypesEnum):
    DVZ_KEY_UNKNOWN = -1
    DVZ_KEY_NONE = +0
    DVZ_KEY_SPACE = 32
    DVZ_KEY_APOSTROPHE = 39
    DVZ_KEY_COMMA = 44
    DVZ_KEY_MINUS = 45
    DVZ_KEY_PERIOD = 46
    DVZ_KEY_SLASH = 47
    DVZ_KEY_0 = 48
    DVZ_KEY_1 = 49
    DVZ_KEY_2 = 50
    DVZ_KEY_3 = 51
    DVZ_KEY_4 = 52
    DVZ_KEY_5 = 53
    DVZ_KEY_6 = 54
    DVZ_KEY_7 = 55
    DVZ_KEY_8 = 56
    DVZ_KEY_9 = 57
    DVZ_KEY_SEMICOLON = 59
    DVZ_KEY_EQUAL = 61
    DVZ_KEY_A = 65
    DVZ_KEY_B = 66
    DVZ_KEY_C = 67
    DVZ_KEY_D = 68
    DVZ_KEY_E = 69
    DVZ_KEY_F = 70
    DVZ_KEY_G = 71
    DVZ_KEY_H = 72
    DVZ_KEY_I = 73
    DVZ_KEY_J = 74
    DVZ_KEY_K = 75
    DVZ_KEY_L = 76
    DVZ_KEY_M = 77
    DVZ_KEY_N = 78
    DVZ_KEY_O = 79
    DVZ_KEY_P = 80
    DVZ_KEY_Q = 81
    DVZ_KEY_R = 82
    DVZ_KEY_S = 83
    DVZ_KEY_T = 84
    DVZ_KEY_U = 85
    DVZ_KEY_V = 86
    DVZ_KEY_W = 87
    DVZ_KEY_X = 88
    DVZ_KEY_Y = 89
    DVZ_KEY_Z = 90
    DVZ_KEY_LEFT_BRACKET = 91
    DVZ_KEY_BACKSLASH = 92
    DVZ_KEY_RIGHT_BRACKET = 93
    DVZ_KEY_GRAVE_ACCENT = 96
    DVZ_KEY_WORLD_1 = 161
    DVZ_KEY_WORLD_2 = 162
    DVZ_KEY_ESCAPE = 256
    DVZ_KEY_ENTER = 257
    DVZ_KEY_TAB = 258
    DVZ_KEY_BACKSPACE = 259
    DVZ_KEY_INSERT = 260
    DVZ_KEY_DELETE = 261
    DVZ_KEY_RIGHT = 262
    DVZ_KEY_LEFT = 263
    DVZ_KEY_DOWN = 264
    DVZ_KEY_UP = 265
    DVZ_KEY_PAGE_UP = 266
    DVZ_KEY_PAGE_DOWN = 267
    DVZ_KEY_HOME = 268
    DVZ_KEY_END = 269
    DVZ_KEY_CAPS_LOCK = 280
    DVZ_KEY_SCROLL_LOCK = 281
    DVZ_KEY_NUM_LOCK = 282
    DVZ_KEY_PRINT_SCREEN = 283
    DVZ_KEY_PAUSE = 284
    DVZ_KEY_F1 = 290
    DVZ_KEY_F2 = 291
    DVZ_KEY_F3 = 292
    DVZ_KEY_F4 = 293
    DVZ_KEY_F5 = 294
    DVZ_KEY_F6 = 295
    DVZ_KEY_F7 = 296
    DVZ_KEY_F8 = 297
    DVZ_KEY_F9 = 298
    DVZ_KEY_F10 = 299
    DVZ_KEY_F11 = 300
    DVZ_KEY_F12 = 301
    DVZ_KEY_F13 = 302
    DVZ_KEY_F14 = 303
    DVZ_KEY_F15 = 304
    DVZ_KEY_F16 = 305
    DVZ_KEY_F17 = 306
    DVZ_KEY_F18 = 307
    DVZ_KEY_F19 = 308
    DVZ_KEY_F20 = 309
    DVZ_KEY_F21 = 310
    DVZ_KEY_F22 = 311
    DVZ_KEY_F23 = 312
    DVZ_KEY_F24 = 313
    DVZ_KEY_F25 = 314
    DVZ_KEY_KP_0 = 320
    DVZ_KEY_KP_1 = 321
    DVZ_KEY_KP_2 = 322
    DVZ_KEY_KP_3 = 323
    DVZ_KEY_KP_4 = 324
    DVZ_KEY_KP_5 = 325
    DVZ_KEY_KP_6 = 326
    DVZ_KEY_KP_7 = 327
    DVZ_KEY_KP_8 = 328
    DVZ_KEY_KP_9 = 329
    DVZ_KEY_KP_DECIMAL = 330
    DVZ_KEY_KP_DIVIDE = 331
    DVZ_KEY_KP_MULTIPLY = 332
    DVZ_KEY_KP_SUBTRACT = 333
    DVZ_KEY_KP_ADD = 334
    DVZ_KEY_KP_ENTER = 335
    DVZ_KEY_KP_EQUAL = 336
    DVZ_KEY_LEFT_SHIFT = 340
    DVZ_KEY_LEFT_CONTROL = 341
    DVZ_KEY_LEFT_ALT = 342
    DVZ_KEY_LEFT_SUPER = 343
    DVZ_KEY_RIGHT_SHIFT = 344
    DVZ_KEY_RIGHT_CONTROL = 345
    DVZ_KEY_RIGHT_ALT = 346
    DVZ_KEY_RIGHT_SUPER = 347
    DVZ_KEY_MENU = 348
    DVZ_KEY_LAST = 348


class DvzGraphicsRequestFlags(CtypesEnum):
    DVZ_GRAPHICS_REQUEST_FLAGS_NONE = 0x0000
    DVZ_GRAPHICS_REQUEST_FLAGS_OFFSCREEN = 0x1000


class DvzPrintFlagsFlags(CtypesEnum):
    DVZ_PRINT_FLAGS_NONE = 0x0000
    DVZ_PRINT_FLAGS_ALL = 0x0001
    DVZ_PRINT_FLAGS_SMALL = 0x0003


# Function aliases

Align = DvzAlign
AppFlags = DvzAppFlags
ArcballFlags = DvzArcballFlags
AxisFlags = DvzAxisFlags
BlendType = DvzBlendType
BoxExtentStrategy = DvzBoxExtentStrategy
BoxMergeStrategy = DvzBoxMergeStrategy
BufferType = DvzBufferType
CameraFlags = DvzCameraFlags
CanvasFlags = DvzCanvasFlags
CapType = DvzCapType
ColorMask = DvzColorMask
Colormap = DvzColormap
ContourFlags = DvzContourFlags
Corner = DvzCorner
CullMode = DvzCullMode
DatFlags = DvzDatFlags
DepthTest = DvzDepthTest
DescriptorType = DvzDescriptorType
DialogFlags = DvzDialogFlags
Dim = DvzDim
Easing = DvzEasing
Filter = DvzFilter
FlyFlags = DvzFlyFlags
FontFlags = DvzFontFlags
Format = DvzFormat
FrontFace = DvzFrontFace
GraphicsRequestFlags = DvzGraphicsRequestFlags
GraphicsType = DvzGraphicsType
GuiFlags = DvzGuiFlags
ImageFlags = DvzImageFlags
JoinType = DvzJoinType
KeyCode = DvzKeyCode
KeyboardEventType = DvzKeyboardEventType
KeyboardModifiers = DvzKeyboardModifiers
MarkerAspect = DvzMarkerAspect
MarkerMode = DvzMarkerMode
MarkerShape = DvzMarkerShape
MeshFlags = DvzMeshFlags
MockFlags = DvzMockFlags
MouseButton = DvzMouseButton
MouseEventType = DvzMouseEventType
MouseState = DvzMouseState
Orientation = DvzOrientation
PanelLinkFlags = DvzPanelLinkFlags
PanzoomFlags = DvzPanzoomFlags
PathFlags = DvzPathFlags
PolygonMode = DvzPolygonMode
PrimitiveTopology = DvzPrimitiveTopology
PrintFlagsFlags = DvzPrintFlagsFlags
RecorderCommandType = DvzRecorderCommandType
RefFlags = DvzRefFlags
RequestAction = DvzRequestAction
RequestObject = DvzRequestObject
SamplerAddressMode = DvzSamplerAddressMode
SamplerAxis = DvzSamplerAxis
SceneFont = DvzSceneFont
ShaderFormat = DvzShaderFormat
ShaderType = DvzShaderType
ShapeIndexingFlags = DvzShapeIndexingFlags
ShapeType = DvzShapeType
SlotType = DvzSlotType
SphereFlags = DvzSphereFlags
TexDims = DvzTexDims
TexFlags = DvzTexFlags
UploadFlags = DvzUploadFlags
VertexInputRate = DvzVertexInputRate
ViewFlags = DvzViewFlags
ViewportClip = DvzViewportClip
VisualFlags = DvzVisualFlags
VolumeFlags = DvzVolumeFlags

ALIGN_HIGH = 3
ALIGN_LOW = 1
ALIGN_MIDDLE = 2
ALIGN_NONE = 0
APP_FLAGS_NONE = 0x000000
APP_FLAGS_OFFSCREEN = 0x008000
APP_FLAGS_WHITE_BACKGROUND = 0x100000
ARCBALL_FLAGS_CONSTRAIN = 1
ARCBALL_FLAGS_NONE = 0
AXIS_FLAGS_DARK = 0x01
AXIS_FLAGS_NONE = 0x00
BLEND_DESTINATION = 2
BLEND_DISABLE = 0
BLEND_OIT = 3
BLEND_STANDARD = 1
BOX_EXTENT_DEFAULT = 0
BOX_EXTENT_FIXED_ASPECT_CONTRACT = 2
BOX_EXTENT_FIXED_ASPECT_EXPAND = 1
BOX_MERGE_CENTER = 1
BOX_MERGE_CORNER = 2
BOX_MERGE_DEFAULT = 0
BUFFER_TYPE_INDEX = 3
BUFFER_TYPE_INDIRECT = 6
BUFFER_TYPE_STAGING = 1
BUFFER_TYPE_STORAGE = 4
BUFFER_TYPE_UNDEFINED = 0
BUFFER_TYPE_UNIFORM = 5
BUFFER_TYPE_VERTEX = 2
CAMERA_FLAGS_ORTHO = 0x01
CAMERA_FLAGS_PERSPECTIVE = 0x00
CANVAS_FLAGS_FPS = 0x0003
CANVAS_FLAGS_FULLSCREEN = 0x0008
CANVAS_FLAGS_IMGUI = 0x0001
CANVAS_FLAGS_MONITOR = 0x0005
CANVAS_FLAGS_NONE = 0x0000
CANVAS_FLAGS_PICK = 0x0020
CANVAS_FLAGS_PUSH_SCALE = 0x0040
CANVAS_FLAGS_VSYNC = 0x0010
CAP_BUTT = 5
CAP_COUNT = 6
CAP_NONE = 0
CAP_ROUND = 1
CAP_SQUARE = 4
CAP_TRIANGLE_IN = 2
CAP_TRIANGLE_OUT = 3
CMAP_AFMHOT = 24
CMAP_AUTUMN = 25
CMAP_BGY = 89
CMAP_BGYW = 90
CMAP_BINARY = 0
CMAP_BJY = 75
CMAP_BKR = 67
CMAP_BKY = 68
CMAP_BLACK_BODY = 121
CMAP_BLUES = 7
CMAP_BMW = 91
CMAP_BONE = 26
CMAP_BRBG = 37
CMAP_BRG = 51
CMAP_BUGN = 8
CMAP_BUPU = 9
CMAP_BWR = 38
CMAP_BWY = 77
CMAP_CET_C1 = 92
CMAP_CET_C1S = 93
CMAP_CET_C2 = 94
CMAP_CET_C4 = 95
CMAP_CET_C4S = 96
CMAP_CET_C5 = 97
CMAP_CET_CBC1 = 82
CMAP_CET_CBC2 = 83
CMAP_CET_CBL1 = 84
CMAP_CET_CBL2 = 85
CMAP_CET_CBTC1 = 86
CMAP_CET_CBTC2 = 87
CMAP_CET_CBTL1 = 88
CMAP_CET_D10 = 69
CMAP_CET_D11 = 70
CMAP_CET_D12 = 78
CMAP_CET_D13 = 72
CMAP_CET_D1A = 74
CMAP_CET_D3 = 73
CMAP_CET_D8 = 71
CMAP_CET_D9 = 80
CMAP_CET_I1 = 98
CMAP_CET_I3 = 99
CMAP_CET_L10 = 100
CMAP_CET_L11 = 101
CMAP_CET_L12 = 102
CMAP_CET_L16 = 103
CMAP_CET_L17 = 104
CMAP_CET_L18 = 105
CMAP_CET_L19 = 106
CMAP_CET_L4 = 107
CMAP_CET_L7 = 108
CMAP_CET_L8 = 109
CMAP_CET_L9 = 110
CMAP_CET_R1 = 111
CMAP_CET_R2 = 112
CMAP_CET_R3 = 79
CMAP_CIVIDIS = 2
CMAP_CMRMAP = 52
CMAP_COLORWHEEL = 113
CMAP_COOL = 27
CMAP_COOLWARM = 39
CMAP_COPPER = 28
CMAP_CUBEHELIX = 53
CMAP_CWR = 81
CMAP_EXTENDED_KINDLMANN = 123
CMAP_FIRE = 114
CMAP_FLAG = 54
CMAP_GIST_EARTH = 55
CMAP_GIST_HEAT = 29
CMAP_GIST_NCAR = 56
CMAP_GIST_RAINBOW = 57
CMAP_GIST_STERN = 58
CMAP_GNBU = 10
CMAP_GNUPLOT = 60
CMAP_GNUPLOT2 = 59
CMAP_GRAY = 30
CMAP_GREENS = 11
CMAP_GREYS = 12
CMAP_GWV = 76
CMAP_HOT = 31
CMAP_HSV = 1
CMAP_INFERNO = 3
CMAP_ISOLUM = 115
CMAP_JET = 61
CMAP_KB = 116
CMAP_KBC = 117
CMAP_KG = 118
CMAP_KGY = 119
CMAP_KINDLMANN = 122
CMAP_KR = 120
CMAP_MAGMA = 4
CMAP_NIPY_SPECTRAL = 62
CMAP_OCEAN = 63
CMAP_ORANGES = 13
CMAP_ORRD = 14
CMAP_PINK = 32
CMAP_PIYG = 40
CMAP_PLASMA = 5
CMAP_PRGN = 41
CMAP_PRISM = 64
CMAP_PUBU = 15
CMAP_PUBUGN = 16
CMAP_PUOR = 42
CMAP_PURPLES = 17
CMAP_RAINBOW = 65
CMAP_RDBU = 43
CMAP_RDGY = 44
CMAP_RDPU = 18
CMAP_RDYLBU = 45
CMAP_RDYLGN = 46
CMAP_REDS = 19
CMAP_SEISMIC = 47
CMAP_SPECTRAL = 48
CMAP_SPRING = 33
CMAP_SUMMER = 34
CMAP_TERRAIN = 66
CMAP_TWILIGHT = 50
CMAP_TWILIGHT_SHIFTED = 49
CMAP_VIRIDIS = 6
CMAP_WINTER = 35
CMAP_WISTIA = 36
CMAP_YLGN = 20
CMAP_YLGNBU = 21
CMAP_YLORBR = 22
CMAP_YLORRD = 23
CONTOUR_EDGES = 0x01
CONTOUR_FULL = 0x04
CONTOUR_JOINTS = 0x02
CONTOUR_NONE = 0x00
CPAL032_ACCENT = CPAL032_OFS
CPAL032_CATEGORY10_10 = 142
CPAL032_CATEGORY20B_20 = 144
CPAL032_CATEGORY20C_20 = 145
CPAL032_CATEGORY20_20 = 143
CPAL032_COLORBLIND8 = 146
CPAL032_DARK2 = 131
CPAL032_PAIRED = 132
CPAL032_PASTEL1 = 133
CPAL032_PASTEL2 = 134
CPAL032_SET1 = 135
CPAL032_SET2 = 136
CPAL032_SET3 = 137
CPAL032_TAB10 = 138
CPAL032_TAB20 = 139
CPAL032_TAB20B = 140
CPAL032_TAB20C = 141
CPAL256_GLASBEY = CPAL256_OFS
CPAL256_GLASBEY_COOL = 125
CPAL256_GLASBEY_DARK = 126
CPAL256_GLASBEY_HV = 127
CPAL256_GLASBEY_LIGHT = 128
CPAL256_GLASBEY_WARM = 129
CULL_MODE_BACK = 0x00000002
CULL_MODE_FRONT = 0x00000001
CULL_MODE_NONE = 0
DAT_FLAGS_DUP = 0x0400
DAT_FLAGS_KEEP_ON_RESIZE = 0x1000
DAT_FLAGS_MAPPABLE = 0x0200
DAT_FLAGS_NONE = 0x0000
DAT_FLAGS_PERSISTENT_STAGING = 0x2000
DAT_FLAGS_STANDALONE = 0x0100
DEPTH_TEST_DISABLE = 0
DEPTH_TEST_ENABLE = 1
DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1
DESCRIPTOR_TYPE_SAMPLED_IMAGE = 2
DESCRIPTOR_TYPE_SAMPLER = 0
DESCRIPTOR_TYPE_STORAGE_BUFFER = 7
DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC = 9
DESCRIPTOR_TYPE_STORAGE_IMAGE = 3
DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER = 5
DESCRIPTOR_TYPE_UNIFORM_BUFFER = 6
DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC = 8
DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER = 4
DIALOG_CORNER_BOTTOM_LEFT = 2
DIALOG_CORNER_BOTTOM_RIGHT = 3
DIALOG_CORNER_TOP_LEFT = 0
DIALOG_CORNER_TOP_RIGHT = 1
DIALOG_FLAGS_BLANK = 0x0004
DIALOG_FLAGS_NONE = 0x0000
DIALOG_FLAGS_OVERLAY = 0x0001
DIALOG_FLAGS_PANEL = 0x0008
DIM_COUNT = 3
DIM_X = 0x0000
DIM_Y = 0x0001
DIM_Z = 0x0002
EASING_COUNT = 31
EASING_IN_BACK = 22
EASING_IN_BOUNCE = 28
EASING_IN_CIRC = 19
EASING_IN_CUBIC = 7
EASING_IN_ELASTIC = 25
EASING_IN_EXPO = 16
EASING_IN_OUT_BACK = 24
EASING_IN_OUT_BOUNCE = 30
EASING_IN_OUT_CIRC = 21
EASING_IN_OUT_CUBIC = 9
EASING_IN_OUT_ELASTIC = 27
EASING_IN_OUT_EXPO = 18
EASING_IN_OUT_QUAD = 6
EASING_IN_OUT_QUART = 12
EASING_IN_OUT_QUINT = 15
EASING_IN_OUT_SINE = 3
EASING_IN_QUAD = 4
EASING_IN_QUART = 10
EASING_IN_QUINT = 13
EASING_IN_SINE = 1
EASING_NONE = 0
EASING_OUT_BACK = 23
EASING_OUT_BOUNCE = 29
EASING_OUT_CIRC = 20
EASING_OUT_CUBIC = 8
EASING_OUT_ELASTIC = 26
EASING_OUT_EXPO = 17
EASING_OUT_QUAD = 5
EASING_OUT_QUART = 11
EASING_OUT_QUINT = 14
EASING_OUT_SINE = 2
FILTER_CUBIC_IMG = 1000015000
FILTER_LINEAR = 1
FILTER_NEAREST = 0
FLY_FLAGS_FIXED_UP = 0x0002
FLY_FLAGS_INVERT_MOUSE = 0x0001
FLY_FLAGS_NONE = 0x0000
FONT_FLAGS_RGB = 0
FONT_FLAGS_RGBA = 1
FORMAT_B8G8R8A8_UNORM = 44
FORMAT_NONE = 0
FORMAT_R16_SNORM = 71
FORMAT_R16_UNORM = 70
FORMAT_R32G32B32A32_SFLOAT = 109
FORMAT_R32G32B32A32_SINT = 108
FORMAT_R32G32B32A32_UINT = 107
FORMAT_R32G32B32_SFLOAT = 106
FORMAT_R32G32B32_SINT = 105
FORMAT_R32G32B32_UINT = 104
FORMAT_R32G32_SFLOAT = 103
FORMAT_R32G32_SINT = 102
FORMAT_R32G32_UINT = 101
FORMAT_R32_SFLOAT = 100
FORMAT_R32_SINT = 99
FORMAT_R32_UINT = 98
FORMAT_R64G64B64A64_SFLOAT = 121
FORMAT_R64G64B64A64_SINT = 120
FORMAT_R64G64B64A64_UINT = 119
FORMAT_R64G64B64_SFLOAT = 118
FORMAT_R64G64B64_SINT = 117
FORMAT_R64G64B64_UINT = 116
FORMAT_R64G64_SFLOAT = 115
FORMAT_R64G64_SINT = 114
FORMAT_R64G64_UINT = 113
FORMAT_R64_SFLOAT = 112
FORMAT_R64_SINT = 111
FORMAT_R64_UINT = 110
FORMAT_R8G8B8A8_SINT = 42
FORMAT_R8G8B8A8_SNORM = 38
FORMAT_R8G8B8A8_UINT = 41
FORMAT_R8G8B8A8_UNORM = 37
FORMAT_R8G8B8_SINT = 28
FORMAT_R8G8B8_SNORM = 24
FORMAT_R8G8B8_UINT = 27
FORMAT_R8G8B8_UNORM = 23
FORMAT_R8G8_SINT = 21
FORMAT_R8G8_SNORM = 17
FORMAT_R8G8_UINT = 20
FORMAT_R8G8_UNORM = 16
FORMAT_R8_SINT = 14
FORMAT_R8_SNORM = 10
FORMAT_R8_UINT = 13
FORMAT_R8_UNORM = 9
FRONT_FACE_CLOCKWISE = 1
FRONT_FACE_COUNTER_CLOCKWISE = 0
GRAPHICS_CUSTOM = 3
GRAPHICS_NONE = 0
GRAPHICS_POINT = 1
GRAPHICS_REQUEST_FLAGS_NONE = 0x0000
GRAPHICS_REQUEST_FLAGS_OFFSCREEN = 0x1000
GRAPHICS_TRIANGLE = 2
GUI_FLAGS_DOCKING = 0x0010
GUI_FLAGS_NONE = 0x0000
GUI_FLAGS_OFFSCREEN = 0x0001
IMAGE_FLAGS_BORDER = 0x0080
IMAGE_FLAGS_MODE_COLORMAP = 0x0010
IMAGE_FLAGS_MODE_FILL = 0x0020
IMAGE_FLAGS_MODE_RGBA = 0x0000
IMAGE_FLAGS_RESCALE = 0x0008
IMAGE_FLAGS_RESCALE_KEEP_RATIO = 0x0004
IMAGE_FLAGS_SIZE_NDC = 0x0001
IMAGE_FLAGS_SIZE_PIXELS = 0x0000
INDEXING_EARCUT = 0x10
INDEXING_NONE = 0x00
INDEXING_SURFACE = 0x20
JOIN_ROUND = 1
JOIN_SQUARE = 0
KEYBOARD_EVENT_NONE = 0
KEYBOARD_EVENT_PRESS = 1
KEYBOARD_EVENT_RELEASE = 3
KEYBOARD_EVENT_REPEAT = 2
KEY_0 = 48
KEY_1 = 49
KEY_2 = 50
KEY_3 = 51
KEY_4 = 52
KEY_5 = 53
KEY_6 = 54
KEY_7 = 55
KEY_8 = 56
KEY_9 = 57
KEY_A = 65
KEY_APOSTROPHE = 39
KEY_B = 66
KEY_BACKSLASH = 92
KEY_BACKSPACE = 259
KEY_C = 67
KEY_CAPS_LOCK = 280
KEY_COMMA = 44
KEY_D = 68
KEY_DELETE = 261
KEY_DOWN = 264
KEY_E = 69
KEY_END = 269
KEY_ENTER = 257
KEY_EQUAL = 61
KEY_ESCAPE = 256
KEY_F = 70
KEY_F1 = 290
KEY_F10 = 299
KEY_F11 = 300
KEY_F12 = 301
KEY_F13 = 302
KEY_F14 = 303
KEY_F15 = 304
KEY_F16 = 305
KEY_F17 = 306
KEY_F18 = 307
KEY_F19 = 308
KEY_F2 = 291
KEY_F20 = 309
KEY_F21 = 310
KEY_F22 = 311
KEY_F23 = 312
KEY_F24 = 313
KEY_F25 = 314
KEY_F3 = 292
KEY_F4 = 293
KEY_F5 = 294
KEY_F6 = 295
KEY_F7 = 296
KEY_F8 = 297
KEY_F9 = 298
KEY_G = 71
KEY_GRAVE_ACCENT = 96
KEY_H = 72
KEY_HOME = 268
KEY_I = 73
KEY_INSERT = 260
KEY_J = 74
KEY_K = 75
KEY_KP_0 = 320
KEY_KP_1 = 321
KEY_KP_2 = 322
KEY_KP_3 = 323
KEY_KP_4 = 324
KEY_KP_5 = 325
KEY_KP_6 = 326
KEY_KP_7 = 327
KEY_KP_8 = 328
KEY_KP_9 = 329
KEY_KP_ADD = 334
KEY_KP_DECIMAL = 330
KEY_KP_DIVIDE = 331
KEY_KP_ENTER = 335
KEY_KP_EQUAL = 336
KEY_KP_MULTIPLY = 332
KEY_KP_SUBTRACT = 333
KEY_L = 76
KEY_LAST = 348
KEY_LEFT = 263
KEY_LEFT_ALT = 342
KEY_LEFT_BRACKET = 91
KEY_LEFT_CONTROL = 341
KEY_LEFT_SHIFT = 340
KEY_LEFT_SUPER = 343
KEY_M = 77
KEY_MENU = 348
KEY_MINUS = 45
KEY_MODIFIER_ALT = 0x00000004
KEY_MODIFIER_CONTROL = 0x00000002
KEY_MODIFIER_NONE = 0x00000000
KEY_MODIFIER_SHIFT = 0x00000001
KEY_MODIFIER_SUPER = 0x00000008
KEY_N = 78
KEY_NONE = +0
KEY_NUM_LOCK = 282
KEY_O = 79
KEY_P = 80
KEY_PAGE_DOWN = 267
KEY_PAGE_UP = 266
KEY_PAUSE = 284
KEY_PERIOD = 46
KEY_PRINT_SCREEN = 283
KEY_Q = 81
KEY_R = 82
KEY_RIGHT = 262
KEY_RIGHT_ALT = 346
KEY_RIGHT_BRACKET = 93
KEY_RIGHT_CONTROL = 345
KEY_RIGHT_SHIFT = 344
KEY_RIGHT_SUPER = 347
KEY_S = 83
KEY_SCROLL_LOCK = 281
KEY_SEMICOLON = 59
KEY_SLASH = 47
KEY_SPACE = 32
KEY_T = 84
KEY_TAB = 258
KEY_U = 85
KEY_UNKNOWN = -1
KEY_UP = 265
KEY_V = 86
KEY_W = 87
KEY_WORLD_1 = 161
KEY_WORLD_2 = 162
KEY_X = 88
KEY_Y = 89
KEY_Z = 90
MARKER_ASPECT_FILLED = 0
MARKER_ASPECT_OUTLINE = 2
MARKER_ASPECT_STROKE = 1
MARKER_MODE_BITMAP = 2
MARKER_MODE_CODE = 1
MARKER_MODE_MSDF = 4
MARKER_MODE_MTSDF = 5
MARKER_MODE_NONE = 0
MARKER_MODE_SDF = 3
MARKER_SHAPE_ARROW = 7
MARKER_SHAPE_ASTERISK = 1
MARKER_SHAPE_CHEVRON = 2
MARKER_SHAPE_CLOVER = 3
MARKER_SHAPE_CLUB = 4
MARKER_SHAPE_COUNT = 20
MARKER_SHAPE_CROSS = 5
MARKER_SHAPE_DIAMOND = 6
MARKER_SHAPE_DISC = 0
MARKER_SHAPE_ELLIPSE = 8
MARKER_SHAPE_HBAR = 9
MARKER_SHAPE_HEART = 10
MARKER_SHAPE_INFINITY = 11
MARKER_SHAPE_PIN = 12
MARKER_SHAPE_RING = 13
MARKER_SHAPE_ROUNDED_RECT = 19
MARKER_SHAPE_SPADE = 14
MARKER_SHAPE_SQUARE = 15
MARKER_SHAPE_TAG = 16
MARKER_SHAPE_TRIANGLE = 17
MARKER_SHAPE_VBAR = 18
MASK_COLOR_A = 0x00000008
MASK_COLOR_ALL = 0x0000000F
MASK_COLOR_B = 0x00000004
MASK_COLOR_G = 0x00000002
MASK_COLOR_R = 0x00000001
MESH_FLAGS_CONTOUR = 0x0004
MESH_FLAGS_ISOLINE = 0x0008
MESH_FLAGS_LIGHTING = 0x0002
MESH_FLAGS_NONE = 0x0000
MESH_FLAGS_TEXTURED = 0x0001
MOCK_FLAGS_CLOSED = 0x01
MOCK_FLAGS_NONE = 0x00
MOUSE_BUTTON_LEFT = 1
MOUSE_BUTTON_MIDDLE = 2
MOUSE_BUTTON_NONE = 0
MOUSE_BUTTON_RIGHT = 3
MOUSE_EVENT_ALL = 255
MOUSE_EVENT_CLICK = 3
MOUSE_EVENT_DOUBLE_CLICK = 5
MOUSE_EVENT_DRAG = 11
MOUSE_EVENT_DRAG_START = 10
MOUSE_EVENT_DRAG_STOP = 12
MOUSE_EVENT_MOVE = 2
MOUSE_EVENT_PRESS = 1
MOUSE_EVENT_RELEASE = 0
MOUSE_EVENT_WHEEL = 20
MOUSE_STATE_CLICK = 3
MOUSE_STATE_CLICK_PRESS = 4
MOUSE_STATE_DOUBLE_CLICK = 5
MOUSE_STATE_DRAGGING = 11
MOUSE_STATE_PRESS = 1
MOUSE_STATE_RELEASE = 0
ORIENTATION_DEFAULT = 0
ORIENTATION_DOWN = 3
ORIENTATION_REVERSE = 2
ORIENTATION_UP = 1
PANEL_LINK_FLAGS_MODEL = 0x01
PANEL_LINK_FLAGS_NONE = 0x00
PANEL_LINK_FLAGS_PROJECTION = 0x04
PANEL_LINK_FLAGS_VIEW = 0x02
PANZOOM_FLAGS_FIXED_X = 0x10
PANZOOM_FLAGS_FIXED_Y = 0x20
PANZOOM_FLAGS_KEEP_ASPECT = 0x01
PANZOOM_FLAGS_NONE = 0x00
PATH_FLAGS_CLOSED = 1
PATH_FLAGS_OPEN = 0
POLYGON_MODE_FILL = 0
POLYGON_MODE_LINE = 1
POLYGON_MODE_POINT = 2
PRIMITIVE_TOPOLOGY_LINE_LIST = 1
PRIMITIVE_TOPOLOGY_LINE_STRIP = 2
PRIMITIVE_TOPOLOGY_POINT_LIST = 0
PRIMITIVE_TOPOLOGY_TRIANGLE_FAN = 5
PRIMITIVE_TOPOLOGY_TRIANGLE_LIST = 3
PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP = 4
PRINT_FLAGS_ALL = 0x0001
PRINT_FLAGS_NONE = 0x0000
PRINT_FLAGS_SMALL = 0x0003
RECORDER_BEGIN = 1
RECORDER_COUNT = 9
RECORDER_DRAW = 2
RECORDER_DRAW_INDEXED = 3
RECORDER_DRAW_INDEXED_INDIRECT = 5
RECORDER_DRAW_INDIRECT = 4
RECORDER_END = 8
RECORDER_NONE = 0
RECORDER_PUSH = 7
RECORDER_VIEWPORT = 6
REF_FLAGS_EQUAL = 0x01
REF_FLAGS_NONE = 0x00
REQUEST_ACTION_BIND = 5
REQUEST_ACTION_CREATE = 1
REQUEST_ACTION_DELETE = 2
REQUEST_ACTION_DOWNLOAD = 9
REQUEST_ACTION_GET = 11
REQUEST_ACTION_NONE = 0
REQUEST_ACTION_RECORD = 6
REQUEST_ACTION_RESIZE = 3
REQUEST_ACTION_SET = 10
REQUEST_ACTION_UPDATE = 4
REQUEST_ACTION_UPFILL = 8
REQUEST_ACTION_UPLOAD = 7
REQUEST_OBJECT_BACKGROUND = 21
REQUEST_OBJECT_BLEND = 8
REQUEST_OBJECT_CANVAS = 101
REQUEST_OBJECT_COMPUTE = 5
REQUEST_OBJECT_CULL = 11
REQUEST_OBJECT_DAT = 2
REQUEST_OBJECT_DEPTH = 7
REQUEST_OBJECT_FRONT = 12
REQUEST_OBJECT_GRAPHICS = 19
REQUEST_OBJECT_INDEX = 20
REQUEST_OBJECT_MASK = 9
REQUEST_OBJECT_NONE = 0
REQUEST_OBJECT_POLYGON = 10
REQUEST_OBJECT_PRIMITIVE = 6
REQUEST_OBJECT_PUSH = 17
REQUEST_OBJECT_RECORD = 22
REQUEST_OBJECT_SAMPLER = 4
REQUEST_OBJECT_SHADER = 13
REQUEST_OBJECT_SLOT = 16
REQUEST_OBJECT_SPECIALIZATION = 18
REQUEST_OBJECT_TEX = 3
REQUEST_OBJECT_VERTEX = 14
REQUEST_OBJECT_VERTEX_ATTR = 15
SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER = 3
SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE = 2
SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT = 1
SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE = 4
SAMPLER_ADDRESS_MODE_REPEAT = 0
SAMPLER_AXIS_U = 0
SAMPLER_AXIS_V = 1
SAMPLER_AXIS_W = 2
SCENE_FONT_COUNT = 2
SCENE_FONT_LABEL = 1
SCENE_FONT_MONO = 0
SHADER_COMPUTE = 0x00000020
SHADER_FRAGMENT = 0x00000010
SHADER_GEOMETRY = 0x00000008
SHADER_GLSL = 2
SHADER_NONE = 0
SHADER_SPIRV = 1
SHADER_TESSELLATION_CONTROL = 0x00000002
SHADER_TESSELLATION_EVALUATION = 0x00000004
SHADER_VERTEX = 0x00000001
SHAPE_ARROW = 11
SHAPE_CONE = 9
SHAPE_CUBE = 6
SHAPE_CYLINDER = 8
SHAPE_DISC = 2
SHAPE_DODECAHEDRON = 15
SHAPE_HEXAHEDRON = 13
SHAPE_HISTOGRAM = 5
SHAPE_ICOSAHEDRON = 16
SHAPE_NONE = 0
SHAPE_OBJ = 18
SHAPE_OCTAHEDRON = 14
SHAPE_OTHER = 19
SHAPE_POLYGON = 4
SHAPE_SECTOR = 3
SHAPE_SPHERE = 7
SHAPE_SQUARE = 1
SHAPE_SURFACE = 17
SHAPE_TETRAHEDRON = 12
SHAPE_TORUS = 10
SLOT_COUNT = 2
SLOT_DAT = 0
SLOT_TEX = 1
SPHERE_FLAGS_EQUAL_RECTANGULAR = 0x0008
SPHERE_FLAGS_LIGHTING = 0x0002
SPHERE_FLAGS_NONE = 0x0000
SPHERE_FLAGS_SIZE_PIXELS = 0x0004
SPHERE_FLAGS_TEXTURED = 0x0001
TEX_1D = 1
TEX_2D = 2
TEX_3D = 3
TEX_FLAGS_NONE = 0x0000
TEX_FLAGS_PERSISTENT_STAGING = 0x2000
TEX_NONE = 0
UPLOAD_FLAGS_NOCOPY = 0x0800
VERTEX_INPUT_RATE_INSTANCE = 1
VERTEX_INPUT_RATE_VERTEX = 0
VIEWPORT_CLIP_BOTTOM = 0x0004
VIEWPORT_CLIP_INNER = 0x0001
VIEWPORT_CLIP_LEFT = 0x0008
VIEWPORT_CLIP_OUTER = 0x0002
VIEW_FLAGS_NOCLIP = 0x0020
VIEW_FLAGS_NONE = 0x0000
VIEW_FLAGS_STATIC = 0x0010
VISUAL_FLAGS_DEFAULT = 0x000000
VISUAL_FLAGS_FIXED_ALL = 0x007000
VISUAL_FLAGS_FIXED_X = 0x001000
VISUAL_FLAGS_FIXED_Y = 0x002000
VISUAL_FLAGS_FIXED_Z = 0x004000
VISUAL_FLAGS_INDEXED = 0x010000
VISUAL_FLAGS_INDEX_MAPPABLE = 0x800000
VISUAL_FLAGS_INDIRECT = 0x020000
VISUAL_FLAGS_VERTEX_MAPPABLE = 0x400000
VOLUME_FLAGS_BACK_FRONT = 0x0004
VOLUME_FLAGS_COLORMAP = 0x0002
VOLUME_FLAGS_NONE = 0x0000
VOLUME_FLAGS_RGBA = 0x0001


# ===============================================================================
# FORWARD DECLARATIONS
# ===============================================================================

class DvzApp(ctypes.Structure):
    pass


class DvzArcball(ctypes.Structure):
    pass


class DvzAtlas(ctypes.Structure):
    pass


class DvzAxes(ctypes.Structure):
    pass


class DvzBox(ctypes.Structure):
    pass


class DvzCamera(ctypes.Structure):
    pass


class DvzColorbar(ctypes.Structure):
    pass


class DvzFifo(ctypes.Structure):
    pass


class DvzFigure(ctypes.Structure):
    pass


class DvzFly(ctypes.Structure):
    pass


class DvzFont(ctypes.Structure):
    pass


class DvzGuiWindow(ctypes.Structure):
    pass


class DvzIndex(ctypes.Structure):
    pass


class DvzKeyboard(ctypes.Structure):
    pass


class DvzList(ctypes.Structure):
    pass


class DvzMouse(ctypes.Structure):
    pass


class DvzOrtho(ctypes.Structure):
    pass


class DvzPanel(ctypes.Structure):
    pass


class DvzPanzoom(ctypes.Structure):
    pass


class DvzParams(ctypes.Structure):
    pass


class DvzQtApp(ctypes.Structure):
    pass


class DvzQtWindow(ctypes.Structure):
    pass


class DvzRef(ctypes.Structure):
    pass


class DvzRenderer(ctypes.Structure):
    pass


class DvzScene(ctypes.Structure):
    pass


class DvzServer(ctypes.Structure):
    pass


class DvzTex(ctypes.Structure):
    pass


class DvzTexture(ctypes.Structure):
    pass


class DvzTimerItem(ctypes.Structure):
    pass


class DvzTransform(ctypes.Structure):
    pass


class DvzVisual(ctypes.Structure):
    pass


# ===============================================================================
# STRUCTURES
# ===============================================================================

class DvzAtlasFont(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("ttf_size", ctypes.c_ulong),
        ("ttf_bytes", ctypes.POINTER(ctypes.c_char)),
        ("atlas", ctypes.POINTER(DvzAtlas)),
        ("font", ctypes.POINTER(DvzFont)),
        ("font_size", ctypes.c_float),
    ]


class DvzMVP(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("model", mat4),
        ("view", mat4),
        ("proj", mat4),
    ]


class _VkViewport(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("x", ctypes.c_float),
        ("y", ctypes.c_float),
        ("width", ctypes.c_float),
        ("height", ctypes.c_float),
        ("minDepth", ctypes.c_float),
        ("maxDepth", ctypes.c_float),
    ]


class DvzViewport(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("viewport", _VkViewport),
        ("margins", vec4),
        ("offset_screen", uvec2),
        ("size_screen", uvec2),
        ("offset_framebuffer", uvec2),
        ("size_framebuffer", uvec2),
        ("flags", ctypes.c_int),
    ]


class DvzShape(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("transform", mat4),
        ("first", ctypes.c_uint32),
        ("count", ctypes.c_uint32),
        ("type", ctypes.c_int32),
        ("vertex_count", ctypes.c_uint32),
        ("index_count", ctypes.c_uint32),
        ("pos", ctypes.POINTER(vec3)),
        ("normal", ctypes.POINTER(vec3)),
        ("color", ctypes.POINTER(DvzColor)),
        ("texcoords", ctypes.POINTER(vec4)),
        ("isoline", ctypes.POINTER(ctypes.c_float)),
        ("d_left", ctypes.POINTER(vec3)),
        ("d_right", ctypes.POINTER(vec3)),
        ("contour", ctypes.POINTER(cvec4)),
        ("index", ctypes.POINTER(ctypes.c_uint32)),
    ]


class DvzTime(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("seconds", ctypes.c_uint64),
        ("nanoseconds", ctypes.c_uint64),
    ]


class DvzKeyboardEvent(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("type", ctypes.c_int32),
        ("key", ctypes.c_int32),
        ("mods", ctypes.c_int),
        ("user_data", ctypes.c_void_p),
    ]


class DvzMouseWheelEvent(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("dir", vec2),
    ]


class DvzMouseDragEvent(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("press_pos", vec2),
        ("last_pos", vec2),
        ("shift", vec2),
        ("is_press_valid", ctypes.c_bool),
    ]


class DvzMouseEventUnion(ctypes.Union):
    _pack_ = 8
    _fields_ = [
        ("w", DvzMouseWheelEvent),
        ("d", DvzMouseDragEvent),
    ]


class DvzMouseEvent(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("type", ctypes.c_int32),
        ("content", DvzMouseEventUnion),
        ("pos", vec2),
        ("button", ctypes.c_int32),
        ("mods", ctypes.c_int),
        ("content_scale", ctypes.c_float),
        ("user_data", ctypes.c_void_p),
    ]


class DvzWindowEvent(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("framebuffer_width", ctypes.c_uint32),
        ("framebuffer_height", ctypes.c_uint32),
        ("screen_width", ctypes.c_uint32),
        ("screen_height", ctypes.c_uint32),
        ("flags", ctypes.c_int),
        ("user_data", ctypes.c_void_p),
    ]


class DvzFrameEvent(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("frame_idx", ctypes.c_uint64),
        ("time", ctypes.c_double),
        ("interval", ctypes.c_double),
        ("user_data", ctypes.c_void_p),
    ]


class DvzGuiEvent(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("gui_window", ctypes.POINTER(DvzGuiWindow)),
        ("user_data", ctypes.c_void_p),
    ]


class DvzTimerEvent(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("timer_idx", ctypes.c_uint32),
        ("timer_item", ctypes.POINTER(DvzTimerItem)),
        ("step_idx", ctypes.c_uint64),
        ("time", ctypes.c_double),
        ("user_data", ctypes.c_void_p),
    ]


class DvzRecorderViewport(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("offset", vec2),
        ("shape", vec2),
    ]


class DvzRecorderPush(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("pipe_id", DvzId),
        ("shader_stages", DvzShaderStageFlags),
        ("offset", DvzSize),
        ("size", DvzSize),
        ("data", ctypes.c_void_p),
    ]


class DvzRecorderDraw(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("pipe_id", DvzId),
        ("first_vertex", ctypes.c_uint32),
        ("vertex_count", ctypes.c_uint32),
        ("first_instance", ctypes.c_uint32),
        ("instance_count", ctypes.c_uint32),
    ]


class DvzRecorderDrawIndexed(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("pipe_id", DvzId),
        ("first_index", ctypes.c_uint32),
        ("vertex_offset", ctypes.c_uint32),
        ("index_count", ctypes.c_uint32),
        ("first_instance", ctypes.c_uint32),
        ("instance_count", ctypes.c_uint32),
    ]


class DvzRecorderDrawIndirect(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("pipe_id", DvzId),
        ("dat_indirect_id", DvzId),
        ("draw_count", ctypes.c_uint32),
    ]


class DvzRecorderDrawIndexedIndirect(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("pipe_id", DvzId),
        ("dat_indirect_id", DvzId),
        ("draw_count", ctypes.c_uint32),
    ]


class DvzRecorderUnion(ctypes.Union):
    _pack_ = 8
    _fields_ = [
        ("v", DvzRecorderViewport),
        ("p", DvzRecorderPush),
        ("draw", DvzRecorderDraw),
        ("draw_indexed", DvzRecorderDrawIndexed),
        ("draw_indirect", DvzRecorderDrawIndirect),
        ("draw_indexed_indirect", DvzRecorderDrawIndexedIndirect),
    ]


class DvzRecorderCommand(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("type", ctypes.c_int32),
        ("canvas_id", DvzId),
        ("object_type", ctypes.c_int32),
        ("contents", DvzRecorderUnion),
    ]


class DvzRequestBoard(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("width", ctypes.c_uint32),
        ("height", ctypes.c_uint32),
        ("background", cvec4),
    ]


class DvzRequestCanvas(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("framebuffer_width", ctypes.c_uint32),
        ("framebuffer_height", ctypes.c_uint32),
        ("screen_width", ctypes.c_uint32),
        ("screen_height", ctypes.c_uint32),
        ("is_offscreen", ctypes.c_bool),
        ("background", cvec4),
    ]


class DvzRequestDat(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("type", ctypes.c_int32),
        ("size", DvzSize),
    ]


class DvzRequestTex(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("dims", ctypes.c_int32),
        ("shape", uvec3),
        ("format", ctypes.c_int32),
    ]


class DvzRequestSampler(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("filter", ctypes.c_int32),
        ("mode", ctypes.c_int32),
    ]


class DvzRequestShader(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("format", ctypes.c_int32),
        ("type", ctypes.c_int32),
        ("size", DvzSize),
        ("code", ctypes.c_char_p),
        ("buffer", ctypes.POINTER(ctypes.c_uint32)),
    ]


class DvzRequestDatUpload(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("upload_type", ctypes.c_int),
        ("offset", DvzSize),
        ("size", DvzSize),
        ("data", ctypes.c_void_p),
    ]


class DvzRequestTexUpload(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("upload_type", ctypes.c_int),
        ("offset", uvec3),
        ("shape", uvec3),
        ("size", DvzSize),
        ("data", ctypes.c_void_p),
    ]


class DvzRequestGraphics(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("type", ctypes.c_int32),
    ]


class DvzRequestPrimitive(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("primitive", ctypes.c_int32),
    ]


class DvzRequestBlend(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("blend", ctypes.c_int32),
    ]


class DvzRequestMask(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("mask", ctypes.c_int32),
    ]


class DvzRequestDepth(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("depth", ctypes.c_int32),
    ]


class DvzRequestPolygon(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("polygon", ctypes.c_int32),
    ]


class DvzRequestCull(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("cull", ctypes.c_int32),
    ]


class DvzRequestFront(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("front", ctypes.c_int32),
    ]


class DvzRequestShaderSet(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("shader", DvzId),
    ]


class DvzRequestVertex(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("binding_idx", ctypes.c_uint32),
        ("stride", DvzSize),
        ("input_rate", ctypes.c_int32),
    ]


class DvzRequestAttr(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("binding_idx", ctypes.c_uint32),
        ("location", ctypes.c_uint32),
        ("format", ctypes.c_int32),
        ("offset", DvzSize),
    ]


class DvzRequestSlot(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("slot_idx", ctypes.c_uint32),
        ("type", ctypes.c_int32),
    ]


class DvzRequestPush(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("shader_stages", DvzShaderStageFlags),
        ("offset", DvzSize),
        ("size", DvzSize),
    ]


class DvzRequestSpecialization(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("shader", ctypes.c_int32),
        ("idx", ctypes.c_uint32),
        ("size", DvzSize),
        ("value", ctypes.c_void_p),
    ]


class DvzRequestBindVertex(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("binding_idx", ctypes.c_uint32),
        ("dat", DvzId),
        ("offset", DvzSize),
    ]


class DvzRequestBindIndex(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("dat", DvzId),
        ("offset", DvzSize),
    ]


class DvzRequestBindDat(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("slot_idx", ctypes.c_uint32),
        ("dat", DvzId),
        ("offset", DvzSize),
    ]


class DvzRequestBindTex(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("slot_idx", ctypes.c_uint32),
        ("tex", DvzId),
        ("sampler", DvzId),
        ("offset", uvec3),
    ]


class DvzRequestRecord(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("command", DvzRecorderCommand),
    ]


class DvzRequestContent(ctypes.Union):
    _pack_ = 8
    _fields_ = [
        ("canvas", DvzRequestCanvas),
        ("dat", DvzRequestDat),
        ("tex", DvzRequestTex),
        ("sampler", DvzRequestSampler),
        ("shader", DvzRequestShader),
        ("dat_upload", DvzRequestDatUpload),
        ("tex_upload", DvzRequestTexUpload),
        ("graphics", DvzRequestGraphics),
        ("set_primitive", DvzRequestPrimitive),
        ("set_blend", DvzRequestBlend),
        ("set_mask", DvzRequestMask),
        ("set_depth", DvzRequestDepth),
        ("set_polygon", DvzRequestPolygon),
        ("set_cull", DvzRequestCull),
        ("set_front", DvzRequestFront),
        ("set_shader", DvzRequestShaderSet),
        ("set_vertex", DvzRequestVertex),
        ("set_attr", DvzRequestAttr),
        ("set_slot", DvzRequestSlot),
        ("set_push", DvzRequestPush),
        ("set_specialization", DvzRequestSpecialization),
        ("bind_vertex", DvzRequestBindVertex),
        ("bind_index", DvzRequestBindIndex),
        ("bind_dat", DvzRequestBindDat),
        ("bind_tex", DvzRequestBindTex),
        ("record", DvzRequestRecord),
    ]


class DvzRequest(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("version", ctypes.c_uint32),
        ("action", ctypes.c_int32),
        ("type", ctypes.c_int32),
        ("id", DvzId),
        ("content", DvzRequestContent),
        ("tag", ctypes.c_int),
        ("flags", ctypes.c_int),
        ("desc", ctypes.c_char_p),
    ]


class DvzBatch(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("capacity", ctypes.c_uint32),
        ("count", ctypes.c_uint32),
        ("requests", ctypes.POINTER(DvzRequest)),
        ("pointers_to_free", ctypes.POINTER(DvzList)),
        ("flags", ctypes.c_int),
    ]


class DvzRequestsEvent(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("batch", ctypes.POINTER(DvzBatch)),
        ("user_data", ctypes.c_void_p),
    ]


class DvzRequester(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("fifo", ctypes.POINTER(DvzFifo)),
    ]


# Struct aliases

AtlasFont = DvzAtlasFont
MVP = DvzMVP
Viewport = DvzViewport
Shape = DvzShape
Time = DvzTime
KeyboardEvent = DvzKeyboardEvent
MouseWheelEvent = DvzMouseWheelEvent
MouseDragEvent = DvzMouseDragEvent
MouseEventUnion = DvzMouseEventUnion
MouseEvent = DvzMouseEvent
WindowEvent = DvzWindowEvent
FrameEvent = DvzFrameEvent
GuiEvent = DvzGuiEvent
TimerEvent = DvzTimerEvent
RecorderViewport = DvzRecorderViewport
RecorderPush = DvzRecorderPush
RecorderDraw = DvzRecorderDraw
RecorderDrawIndexed = DvzRecorderDrawIndexed
RecorderDrawIndirect = DvzRecorderDrawIndirect
RecorderDrawIndexedIndirect = DvzRecorderDrawIndexedIndirect
RecorderUnion = DvzRecorderUnion
RecorderCommand = DvzRecorderCommand
RequestBoard = DvzRequestBoard
RequestCanvas = DvzRequestCanvas
RequestDat = DvzRequestDat
RequestTex = DvzRequestTex
RequestSampler = DvzRequestSampler
RequestShader = DvzRequestShader
RequestDatUpload = DvzRequestDatUpload
RequestTexUpload = DvzRequestTexUpload
RequestGraphics = DvzRequestGraphics
RequestPrimitive = DvzRequestPrimitive
RequestBlend = DvzRequestBlend
RequestMask = DvzRequestMask
RequestDepth = DvzRequestDepth
RequestPolygon = DvzRequestPolygon
RequestCull = DvzRequestCull
RequestFront = DvzRequestFront
RequestShaderSet = DvzRequestShaderSet
RequestVertex = DvzRequestVertex
RequestAttr = DvzRequestAttr
RequestSlot = DvzRequestSlot
RequestPush = DvzRequestPush
RequestSpecialization = DvzRequestSpecialization
RequestBindVertex = DvzRequestBindVertex
RequestBindIndex = DvzRequestBindIndex
RequestBindDat = DvzRequestBindDat
RequestBindTex = DvzRequestBindTex
RequestRecord = DvzRequestRecord
RequestContent = DvzRequestContent
Request = DvzRequest
Batch = DvzBatch
RequestsEvent = DvzRequestsEvent
Requester = DvzRequester


# ===============================================================================
# FUNCTION CALLBACK TYPES
# ===============================================================================



on_gui = DvzAppGuiCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, P_(DvzGuiEvent))
on_mouse = DvzAppMouseCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, P_(DvzMouseEvent))
on_keyboard = DvzAppKeyboardCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, P_(DvzKeyboardEvent))
on_frame = DvzAppFrameCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, P_(DvzFrameEvent))
on_timer = DvzAppTimerCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, P_(DvzTimerEvent))
on_resize = DvzAppResizeCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, P_(DvzWindowEvent))
DvzErrorCallback = ctypes.CFUNCTYPE(None, ctypes.c_char_p)

# ===============================================================================
# FUNCTIONS
# ===============================================================================

# -------------------------------------------------------------------------------------------------
demo = dvz.dvz_demo
demo.__doc__ = """
Run a demo.
"""
demo.argtypes = [
]


# -------------------------------------------------------------------------------------------------
demo_panel_2D = dvz.dvz_demo_panel_2D
demo_panel_2D.__doc__ = """
Demo panel (random scatter plot).

Parameters
----------
panel : DvzPanel*
    the panel

Returns
-------
result : DvzVisual*
     the marker visual
"""
demo_panel_2D.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
]
demo_panel_2D.restype = ctypes.POINTER(DvzVisual)


# -------------------------------------------------------------------------------------------------
demo_panel_3D = dvz.dvz_demo_panel_3D
demo_panel_3D.__doc__ = """
Demo panel (random scatter plot).

Parameters
----------
panel : DvzPanel*
    the panel

Returns
-------
result : DvzVisual*
     the marker visual
"""
demo_panel_3D.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
]
demo_panel_3D.restype = ctypes.POINTER(DvzVisual)


# -------------------------------------------------------------------------------------------------
version = dvz.dvz_version
version.__doc__ = """
Return the current version string.


Returns
-------
result : char*
     the version string
"""
version.argtypes = [
]
version.restype = ctypes.c_char_p


# -------------------------------------------------------------------------------------------------
error_callback = dvz.dvz_error_callback
error_callback.__doc__ = """
Register an error callback, a C function taking as input a string.

Parameters
----------
cb : DvzErrorCallback
    the error callback
"""
error_callback.argtypes = [
    DvzErrorCallback,  # DvzErrorCallback cb
]


# -------------------------------------------------------------------------------------------------
qt_app = dvz.dvz_qt_app
qt_app.__doc__ = """
Placeholder.

Parameters
----------
qapp : np.ndarray[QApplication]
    placeholder
flags : int
    placeholder

Returns
-------
result : DvzQtApp*
"""
qt_app.argtypes = [
    ctypes.POINTER(QApplication),  # QApplication* qapp
    ctypes.c_int,  # int flags
]
qt_app.restype = ctypes.POINTER(DvzQtApp)


# -------------------------------------------------------------------------------------------------
qt_window = dvz.dvz_qt_window
qt_window.__doc__ = """
Placeholder.

Parameters
----------
app : DvzQtApp*
    placeholder

Returns
-------
result : DvzQtWindow*
"""
qt_window.argtypes = [
    ctypes.POINTER(DvzQtApp),  # DvzQtApp* app
]
qt_window.restype = ctypes.POINTER(DvzQtWindow)


# -------------------------------------------------------------------------------------------------
qt_submit = dvz.dvz_qt_submit
qt_submit.__doc__ = """
Placeholder.

Parameters
----------
app : DvzQtApp*
    placeholder
batch : DvzBatch*
    placeholder
"""
qt_submit.argtypes = [
    ctypes.POINTER(DvzQtApp),  # DvzQtApp* app
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
]


# -------------------------------------------------------------------------------------------------
qt_batch = dvz.dvz_qt_batch
qt_batch.__doc__ = """
Placeholder.

Parameters
----------
app : DvzQtApp*
    placeholder

Returns
-------
result : DvzBatch*
"""
qt_batch.argtypes = [
    ctypes.POINTER(DvzQtApp),  # DvzQtApp* app
]
qt_batch.restype = ctypes.POINTER(DvzBatch)


# -------------------------------------------------------------------------------------------------
qt_app_destroy = dvz.dvz_qt_app_destroy
qt_app_destroy.__doc__ = """
Placeholder.

Parameters
----------
app : DvzQtApp*
    placeholder
"""
qt_app_destroy.argtypes = [
    ctypes.POINTER(DvzQtApp),  # DvzQtApp* app
]


# -------------------------------------------------------------------------------------------------
server = dvz.dvz_server
server.__doc__ = """
Placeholder.

Parameters
----------
flags : int
    placeholder

Returns
-------
result : DvzServer*
"""
server.argtypes = [
    ctypes.c_int,  # int flags
]
server.restype = ctypes.POINTER(DvzServer)


# -------------------------------------------------------------------------------------------------
server_submit = dvz.dvz_server_submit
server_submit.__doc__ = """
Placeholder.

Parameters
----------
server : DvzServer*
    placeholder
batch : DvzBatch*
    placeholder
"""
server_submit.argtypes = [
    ctypes.POINTER(DvzServer),  # DvzServer* server
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
]


# -------------------------------------------------------------------------------------------------
server_mouse = dvz.dvz_server_mouse
server_mouse.__doc__ = """
Placeholder.

Parameters
----------
server : DvzServer*
    placeholder

Returns
-------
result : DvzMouse*
"""
server_mouse.argtypes = [
    ctypes.POINTER(DvzServer),  # DvzServer* server
]
server_mouse.restype = ctypes.POINTER(DvzMouse)


# -------------------------------------------------------------------------------------------------
server_keyboard = dvz.dvz_server_keyboard
server_keyboard.__doc__ = """
Placeholder.

Parameters
----------
server : DvzServer*
    placeholder

Returns
-------
result : DvzKeyboard*
"""
server_keyboard.argtypes = [
    ctypes.POINTER(DvzServer),  # DvzServer* server
]
server_keyboard.restype = ctypes.POINTER(DvzKeyboard)


# -------------------------------------------------------------------------------------------------
server_resize = dvz.dvz_server_resize
server_resize.__doc__ = """
Placeholder.

Parameters
----------
server : DvzServer*
    placeholder
canvas_id : DvzId
    placeholder
width : int
    placeholder
height : int
    placeholder
"""
server_resize.argtypes = [
    ctypes.POINTER(DvzServer),  # DvzServer* server
    DvzId,  # DvzId canvas_id
    ctypes.c_uint32,  # uint32_t width
    ctypes.c_uint32,  # uint32_t height
]


# -------------------------------------------------------------------------------------------------
server_grab = dvz.dvz_server_grab
server_grab.__doc__ = """
Placeholder.

Parameters
----------
server : DvzServer*
    placeholder
canvas_id : DvzId
    placeholder
flags : int
    placeholder

Returns
-------
result : uint8_t*
"""
server_grab.argtypes = [
    ctypes.POINTER(DvzServer),  # DvzServer* server
    DvzId,  # DvzId canvas_id
    ctypes.c_int,  # int flags
]
server_grab.restype = ndpointer(dtype=np.uint8, ndim=1, ncol=1, flags="C_CONTIGUOUS")


# -------------------------------------------------------------------------------------------------
scene_render = dvz.dvz_scene_render
scene_render.__doc__ = """
Placeholder.

Parameters
----------
scene : DvzScene*
    placeholder
server : DvzServer*
    placeholder
"""
scene_render.argtypes = [
    ctypes.POINTER(DvzScene),  # DvzScene* scene
    ctypes.POINTER(DvzServer),  # DvzServer* server
]


# -------------------------------------------------------------------------------------------------
server_destroy = dvz.dvz_server_destroy
server_destroy.__doc__ = """
Placeholder.

Parameters
----------
server : DvzServer*
    placeholder
"""
server_destroy.argtypes = [
    ctypes.POINTER(DvzServer),  # DvzServer* server
]


# -------------------------------------------------------------------------------------------------
scene = dvz.dvz_scene
scene.__doc__ = """
Create a scene.

Parameters
----------
batch : DvzBatch*
    the batch

Returns
-------
result : DvzScene*
     the scene
"""
scene.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
]
scene.restype = ctypes.POINTER(DvzScene)


# -------------------------------------------------------------------------------------------------
scene_batch = dvz.dvz_scene_batch
scene_batch.__doc__ = """
Return the batch from a scene.

Parameters
----------
scene : DvzScene*
    the scene

Returns
-------
result : DvzBatch*
     the batch
"""
scene_batch.argtypes = [
    ctypes.POINTER(DvzScene),  # DvzScene* scene
]
scene_batch.restype = ctypes.POINTER(DvzBatch)


# -------------------------------------------------------------------------------------------------
scene_run = dvz.dvz_scene_run
scene_run.__doc__ = """
Start the event loop and render the scene in a window.

Parameters
----------
scene : DvzScene*
    the scene
app : DvzApp*
    the app
frame_count : int
    the maximum number of frames, 0 for infinite loop
"""
scene_run.argtypes = [
    ctypes.POINTER(DvzScene),  # DvzScene* scene
    ctypes.POINTER(DvzApp),  # DvzApp* app
    ctypes.c_uint64,  # uint64_t frame_count
]


# -------------------------------------------------------------------------------------------------
scene_mouse = dvz.dvz_scene_mouse
scene_mouse.__doc__ = """
Manually pass a mouse event to the scene.

Parameters
----------
scene : DvzScene*
    the scene
fig : DvzFigure*
    the figure
ev : DvzMouseEvent*
    the mouse event
"""
scene_mouse.argtypes = [
    ctypes.POINTER(DvzScene),  # DvzScene* scene
    ctypes.POINTER(DvzFigure),  # DvzFigure* fig
    ctypes.POINTER(DvzMouseEvent),  # DvzMouseEvent* ev
]


# -------------------------------------------------------------------------------------------------
scene_destroy = dvz.dvz_scene_destroy
scene_destroy.__doc__ = """
Destroy a scene.

Parameters
----------
scene : DvzScene*
    the scene
"""
scene_destroy.argtypes = [
    ctypes.POINTER(DvzScene),  # DvzScene* scene
]


# -------------------------------------------------------------------------------------------------
mouse = dvz.dvz_mouse
mouse.__doc__ = """
Create a mouse object.


Returns
-------
result : DvzMouse*
     the mouse
"""
mouse.argtypes = [
]
mouse.restype = ctypes.POINTER(DvzMouse)


# -------------------------------------------------------------------------------------------------
mouse_move = dvz.dvz_mouse_move
mouse_move.__doc__ = """
Create a mouse move event.

Parameters
----------
mouse : DvzMouse*
    the mouse
pos : Tuple[float, float]
    the cursor position, in pixels
mods : int
    the keyboard modifier flags
"""
mouse_move.argtypes = [
    ctypes.POINTER(DvzMouse),  # DvzMouse* mouse
    vec2,  # vec2 pos
    ctypes.c_int,  # int mods
]


# -------------------------------------------------------------------------------------------------
mouse_press = dvz.dvz_mouse_press
mouse_press.__doc__ = """
Create a mouse press event.

Parameters
----------
mouse : DvzMouse*
    the mouse
button : DvzMouseButton
    the mouse button (enum int)
mods : int
    the keyboard modifier flags
"""
mouse_press.argtypes = [
    ctypes.POINTER(DvzMouse),  # DvzMouse* mouse
    DvzMouseButton,  # DvzMouseButton button
    ctypes.c_int,  # int mods
]


# -------------------------------------------------------------------------------------------------
mouse_release = dvz.dvz_mouse_release
mouse_release.__doc__ = """
Create a mouse release event.

Parameters
----------
mouse : DvzMouse*
    the mouse
button : DvzMouseButton
    the mouse button (enum int)
mods : int
    the keyboard modifier flags
"""
mouse_release.argtypes = [
    ctypes.POINTER(DvzMouse),  # DvzMouse* mouse
    DvzMouseButton,  # DvzMouseButton button
    ctypes.c_int,  # int mods
]


# -------------------------------------------------------------------------------------------------
mouse_wheel = dvz.dvz_mouse_wheel
mouse_wheel.__doc__ = """
Create a mouse wheel event.

Parameters
----------
mouse : DvzMouse*
    the mouse
dir : Tuple[float, float]
    the mouse wheel direction (x, y)
mods : int
    the keyboard modifier flags
"""
mouse_wheel.argtypes = [
    ctypes.POINTER(DvzMouse),  # DvzMouse* mouse
    vec2,  # vec2 dir
    ctypes.c_int,  # int mods
]


# -------------------------------------------------------------------------------------------------
mouse_event = dvz.dvz_mouse_event
mouse_event.__doc__ = """
Create a generic mouse event.

Parameters
----------
mouse : DvzMouse*
    the mouse
ev : DvzMouseEvent*
    the mouse event
"""
mouse_event.argtypes = [
    ctypes.POINTER(DvzMouse),  # DvzMouse* mouse
    ctypes.POINTER(DvzMouseEvent),  # DvzMouseEvent* ev
]


# -------------------------------------------------------------------------------------------------
mouse_destroy = dvz.dvz_mouse_destroy
mouse_destroy.__doc__ = """
Destroy a mouse.

Parameters
----------
mouse : DvzMouse*
    the mouse
"""
mouse_destroy.argtypes = [
    ctypes.POINTER(DvzMouse),  # DvzMouse* mouse
]


# -------------------------------------------------------------------------------------------------
figure = dvz.dvz_figure
figure.__doc__ = """
Create a figure, a desktop window with panels and visuals.

Parameters
----------
scene : DvzScene*
    the scene
width : int
    the window width
height : int
    the window height
flags : int
    the figure creation flags (not yet stabilized)

Returns
-------
result : DvzFigure*
     the figure
"""
figure.argtypes = [
    ctypes.POINTER(DvzScene),  # DvzScene* scene
    ctypes.c_uint32,  # uint32_t width
    ctypes.c_uint32,  # uint32_t height
    ctypes.c_int,  # int flags
]
figure.restype = ctypes.POINTER(DvzFigure)


# -------------------------------------------------------------------------------------------------
figure_id = dvz.dvz_figure_id
figure_id.__doc__ = """
Return a figure ID.

Parameters
----------
figure : DvzFigure*
    the figure

Returns
-------
result : DvzId
     the figure ID
"""
figure_id.argtypes = [
    ctypes.POINTER(DvzFigure),  # DvzFigure* figure
]
figure_id.restype = DvzId


# -------------------------------------------------------------------------------------------------
figure_resize = dvz.dvz_figure_resize
figure_resize.__doc__ = """
Resize a figure.

Parameters
----------
fig : DvzFigure*
    the figure
width : int
    the window width
height : int
    the window height
"""
figure_resize.argtypes = [
    ctypes.POINTER(DvzFigure),  # DvzFigure* fig
    ctypes.c_uint32,  # uint32_t width
    ctypes.c_uint32,  # uint32_t height
]


# -------------------------------------------------------------------------------------------------
figure_width = dvz.dvz_figure_width
figure_width.__doc__ = """
Return a figure width.

Parameters
----------
fig : DvzFigure*
    the figure

Returns
-------
result : uint32_t
     the figure width
"""
figure_width.argtypes = [
    ctypes.POINTER(DvzFigure),  # DvzFigure* fig
]
figure_width.restype = ctypes.c_uint32


# -------------------------------------------------------------------------------------------------
figure_height = dvz.dvz_figure_height
figure_height.__doc__ = """
Return a figure height.

Parameters
----------
fig : DvzFigure*
    the figure

Returns
-------
result : uint32_t
     the figure height
"""
figure_height.argtypes = [
    ctypes.POINTER(DvzFigure),  # DvzFigure* fig
]
figure_height.restype = ctypes.c_uint32


# -------------------------------------------------------------------------------------------------
app_fullscreen = dvz.dvz_app_fullscreen
app_fullscreen.__doc__ = """
Set display to fullscreen.

Parameters
----------
app : DvzApp*
    the app
canvas_id : DvzId
    the ID of the canvas
is_fullscreen : bool
    True for fullscreen, False for windowed.
"""
app_fullscreen.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
    DvzId,  # DvzId canvas_id
    ctypes.c_bool,  # bool is_fullscreen
]


# -------------------------------------------------------------------------------------------------
scene_figure = dvz.dvz_scene_figure
scene_figure.__doc__ = """
Get a figure from its id.

Parameters
----------
scene : DvzScene*
    the scene
id : DvzId
    the figure id

Returns
-------
result : DvzFigure*
     the figure
"""
scene_figure.argtypes = [
    ctypes.POINTER(DvzScene),  # DvzScene* scene
    DvzId,  # DvzId id
]
scene_figure.restype = ctypes.POINTER(DvzFigure)


# -------------------------------------------------------------------------------------------------
figure_update = dvz.dvz_figure_update
figure_update.__doc__ = """
Update a figure after the composition of the panels and visuals has changed.

Parameters
----------
figure : DvzFigure*
    the figure
"""
figure_update.argtypes = [
    ctypes.POINTER(DvzFigure),  # DvzFigure* figure
]


# -------------------------------------------------------------------------------------------------
figure_destroy = dvz.dvz_figure_destroy
figure_destroy.__doc__ = """
Destroy a figure.

Parameters
----------
figure : DvzFigure*
    the figure
"""
figure_destroy.argtypes = [
    ctypes.POINTER(DvzFigure),  # DvzFigure* figure
]


# -------------------------------------------------------------------------------------------------
panel = dvz.dvz_panel
panel.__doc__ = """
Create a panel in a figure (partial or complete rectangular portion of a figure).

Parameters
----------
fig : DvzFigure*
    the figure
x : float
    the x coordinate of the top left corner, in pixels
y : float
    the y coordinate of the top left corner, in pixels
width : float
    the panel width, in pixels
height : float
    the panel height, in pixels

Returns
-------
result : DvzPanel*
"""
panel.argtypes = [
    ctypes.POINTER(DvzFigure),  # DvzFigure* fig
    ctypes.c_float,  # float x
    ctypes.c_float,  # float y
    ctypes.c_float,  # float width
    ctypes.c_float,  # float height
]
panel.restype = ctypes.POINTER(DvzPanel)


# -------------------------------------------------------------------------------------------------
panel_flags = dvz.dvz_panel_flags
panel_flags.__doc__ = """
Set the panel flags

Parameters
----------
panel : DvzPanel*
    the panel
flags : int
    the panel flags
"""
panel_flags.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
panel_batch = dvz.dvz_panel_batch
panel_batch.__doc__ = """
Return the batch from a panel.

Parameters
----------
panel : DvzPanel*
    the panel

Returns
-------
result : DvzBatch*
     the batch
"""
panel_batch.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
]
panel_batch.restype = ctypes.POINTER(DvzBatch)


# -------------------------------------------------------------------------------------------------
panel_background = dvz.dvz_panel_background
panel_background.__doc__ = """
Set a colored background for a panel.

Parameters
----------
panel : DvzPanel*
    the panel
background : np.ndarray[cvec4]
    the colors of the four corners (top-left, top-right, bottom left,
"""
panel_background.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # cvec4* background
]


# -------------------------------------------------------------------------------------------------
panel_ref = dvz.dvz_panel_ref
panel_ref.__doc__ = """
Get the panel's reference.

Parameters
----------
panel : DvzPanel*
    the panel

Returns
-------
result : DvzRef*
     the reference
"""
panel_ref.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
]
panel_ref.restype = ctypes.POINTER(DvzRef)


# -------------------------------------------------------------------------------------------------
panel_axes = dvz.dvz_panel_axes
panel_axes.__doc__ = """
Get the axes.

Parameters
----------
panel : DvzPanel*
    the panel

Returns
-------
result : DvzAxes*
     the axes
"""
panel_axes.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
]
panel_axes.restype = ctypes.POINTER(DvzAxes)


# -------------------------------------------------------------------------------------------------
panel_axes_2D = dvz.dvz_panel_axes_2D
panel_axes_2D.__doc__ = """
Create 2D axes.

Parameters
----------
panel : DvzPanel*
    the panel
xmin : float
    xmin
xmax : float
    xmax
ymin : float
    ymin
ymax : float
    ymax

Returns
-------
result : DvzAxes*
     the axes
"""
panel_axes_2D.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
    ctypes.c_double,  # double xmin
    ctypes.c_double,  # double xmax
    ctypes.c_double,  # double ymin
    ctypes.c_double,  # double ymax
]
panel_axes_2D.restype = ctypes.POINTER(DvzAxes)


# -------------------------------------------------------------------------------------------------
panel_figure = dvz.dvz_panel_figure
panel_figure.__doc__ = """
Return the figure from a panel.

Parameters
----------
panel : DvzPanel*
    the panel

Returns
-------
result : DvzFigure*
     the figure
"""
panel_figure.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
]
panel_figure.restype = ctypes.POINTER(DvzFigure)


# -------------------------------------------------------------------------------------------------
panel_gui = dvz.dvz_panel_gui
panel_gui.__doc__ = """
Set a panel as a GUI panel.

Parameters
----------
panel : DvzPanel*
    the panel
title : str
    the GUI dialog title
flags : int
    the GUI dialog flags (unused at the moment)
"""
panel_gui.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
    CStringBuffer,  # char* title
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
panel_default = dvz.dvz_panel_default
panel_default.__doc__ = """
Return the default full panel spanning an entire figure.

Parameters
----------
fig : DvzFigure*
    the figure

Returns
-------
result : DvzPanel*
     the panel spanning the entire figure
"""
panel_default.argtypes = [
    ctypes.POINTER(DvzFigure),  # DvzFigure* fig
]
panel_default.restype = ctypes.POINTER(DvzPanel)


# -------------------------------------------------------------------------------------------------
panel_transform = dvz.dvz_panel_transform
panel_transform.__doc__ = """
Assign a transform to a panel.

Parameters
----------
panel : DvzPanel*
    the panel
tr : DvzTransform*
    the transform
"""
panel_transform.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
    ctypes.POINTER(DvzTransform),  # DvzTransform* tr
]


# -------------------------------------------------------------------------------------------------
panel_mvp = dvz.dvz_panel_mvp
panel_mvp.__doc__ = """
Assign a MVP structure to a panel.

Parameters
----------
panel : DvzPanel*
    the panel
mvp : DvzMVP*
    a pointer to the MVP structure
"""
panel_mvp.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
    ctypes.POINTER(DvzMVP),  # DvzMVP* mvp
]


# -------------------------------------------------------------------------------------------------
panel_mvpmat = dvz.dvz_panel_mvpmat
panel_mvpmat.__doc__ = """
Assign the model-view-proj matrices to a panel.

Parameters
----------
panel : DvzPanel*
    the panel
model : mat4
    the model matrix
view : mat4
    the view matrix
proj : mat4
    the projection matrix
"""
panel_mvpmat.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
    mat4,  # mat4 model
    mat4,  # mat4 view
    mat4,  # mat4 proj
]


# -------------------------------------------------------------------------------------------------
panel_resize = dvz.dvz_panel_resize
panel_resize.__doc__ = """
Resize a panel.

Parameters
----------
panel : DvzPanel*
    the panel
x : float
    the x coordinate of the top left corner, in pixels
y : float
    the y coordinate of the top left corner, in pixels
width : float
    the panel width, in pixels
height : float
    the panel height, in pixels
"""
panel_resize.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
    ctypes.c_float,  # float x
    ctypes.c_float,  # float y
    ctypes.c_float,  # float width
    ctypes.c_float,  # float height
]


# -------------------------------------------------------------------------------------------------
panel_margins = dvz.dvz_panel_margins
panel_margins.__doc__ = """
Set the margins of a panel.

Parameters
----------
panel : DvzPanel*
    the panel
top : float
    the top margin, in pixels
right : float
    the right margin, in pixels
bottom : float
    the bottom margin, in pixels
left : float
    the left margin, in pixels
"""
panel_margins.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
    ctypes.c_float,  # float top
    ctypes.c_float,  # float right
    ctypes.c_float,  # float bottom
    ctypes.c_float,  # float left
]


# -------------------------------------------------------------------------------------------------
panel_contains = dvz.dvz_panel_contains
panel_contains.__doc__ = """
Return whether a point is inside a panel.

Parameters
----------
panel : DvzPanel*
    the panel
pos : Tuple[float, float]
    the position

Returns
-------
result : bool
     true if the position lies within the panel
"""
panel_contains.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
    vec2,  # vec2 pos
]
panel_contains.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
panel_at = dvz.dvz_panel_at
panel_at.__doc__ = """
Return the panel containing a given point.

Parameters
----------
figure : DvzFigure*
    the figure
pos : Tuple[float, float]
    the position

Returns
-------
result : DvzPanel*
     the panel containing the point, or NULL if there is none
"""
panel_at.argtypes = [
    ctypes.POINTER(DvzFigure),  # DvzFigure* figure
    vec2,  # vec2 pos
]
panel_at.restype = ctypes.POINTER(DvzPanel)


# -------------------------------------------------------------------------------------------------
panel_camera = dvz.dvz_panel_camera
panel_camera.__doc__ = """
Set a camera for a panel.

Parameters
----------
panel : DvzPanel*
    the panel
flags : int
    the camera flags

Returns
-------
result : DvzCamera*
     the camera
"""
panel_camera.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
    ctypes.c_int,  # int flags
]
panel_camera.restype = ctypes.POINTER(DvzCamera)


# -------------------------------------------------------------------------------------------------
panel_panzoom = dvz.dvz_panel_panzoom
panel_panzoom.__doc__ = """
Set panzoom interactivity for a panel.

Parameters
----------
panel : DvzPanel*
    the panel
flags : int
    the flags

Returns
-------
result : DvzPanzoom*
     the panzoom
"""
panel_panzoom.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
    ctypes.c_int,  # int flags
]
panel_panzoom.restype = ctypes.POINTER(DvzPanzoom)


# -------------------------------------------------------------------------------------------------
panel_ortho = dvz.dvz_panel_ortho
panel_ortho.__doc__ = """
Set ortho interactivity for a panel.

Parameters
----------
panel : DvzPanel*
    the panel
flags : int
    the flags

Returns
-------
result : DvzOrtho*
     the ortho
"""
panel_ortho.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
    ctypes.c_int,  # int flags
]
panel_ortho.restype = ctypes.POINTER(DvzOrtho)


# -------------------------------------------------------------------------------------------------
panel_arcball = dvz.dvz_panel_arcball
panel_arcball.__doc__ = """
Set arcball interactivity for a panel.

Parameters
----------
panel : DvzPanel*
    the panel
flags : int
    the flags

Returns
-------
result : DvzArcball*
     the arcball
"""
panel_arcball.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
    ctypes.c_int,  # int flags
]
panel_arcball.restype = ctypes.POINTER(DvzArcball)


# -------------------------------------------------------------------------------------------------
panel_fly = dvz.dvz_panel_fly
panel_fly.__doc__ = """
Set fly interactivity for a panel.

Parameters
----------
panel : DvzPanel*
    the panel
flags : int
    the flags

Returns
-------
result : DvzFly*
     the fly
"""
panel_fly.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
    ctypes.c_int,  # int flags
]
panel_fly.restype = ctypes.POINTER(DvzFly)


# -------------------------------------------------------------------------------------------------
panel_grid = dvz.dvz_panel_grid
panel_grid.__doc__ = """
Add a 3D horizontal grid.

Parameters
----------
panel : DvzPanel*
    the panel
flags : int
    the grid creation flags

Returns
-------
result : DvzVisual*
     the grid
"""
panel_grid.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
    ctypes.c_int,  # int flags
]
panel_grid.restype = ctypes.POINTER(DvzVisual)


# -------------------------------------------------------------------------------------------------
panel_show = dvz.dvz_panel_show
panel_show.__doc__ = """
Show or hide a panel.

Parameters
----------
panel : DvzPanel*
    the panel
is_visible : bool
    whether to show or hide the panel
"""
panel_show.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
    ctypes.c_bool,  # bool is_visible
]


# -------------------------------------------------------------------------------------------------
panel_link = dvz.dvz_panel_link
panel_link.__doc__ = """
Add or remove a link between two panels.
At all times, the target panel's transform is copied from the source panel's transform.

Parameters
----------
panel : DvzPanel*
    the target panel
source : DvzPanel*
    the source panel
flags : int
    the panel link flags: 0 to remove, or a bit field with model, view, projection
"""
panel_link.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
    ctypes.POINTER(DvzPanel),  # DvzPanel* source
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
panel_update = dvz.dvz_panel_update
panel_update.__doc__ = """
Trigger a panel update.

Parameters
----------
panel : DvzPanel*
    the panel
"""
panel_update.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
]


# -------------------------------------------------------------------------------------------------
panel_visual = dvz.dvz_panel_visual
panel_visual.__doc__ = """
Add a visual to a panel.

Parameters
----------
panel : DvzPanel*
    the panel
visual : DvzVisual*
    the visual
flags : int
    the flags
"""
panel_visual.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
panel_remove = dvz.dvz_panel_remove
panel_remove.__doc__ = """
Remove a visual from a panel.

Parameters
----------
panel : DvzPanel*
    the panel
visual : DvzVisual*
    the visual
"""
panel_remove.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
]


# -------------------------------------------------------------------------------------------------
panel_destroy = dvz.dvz_panel_destroy
panel_destroy.__doc__ = """
Destroy a panel.

Parameters
----------
panel : DvzPanel*
    the panel
"""
panel_destroy.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
]


# -------------------------------------------------------------------------------------------------
visual_update = dvz.dvz_visual_update
visual_update.__doc__ = """
Update a visual after its data has changed.
Note: this function is automatically called in the event loop internally, so you should not need
to use it in most cases.

Parameters
----------
visual : DvzVisual*
    the visual
"""
visual_update.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
]


# -------------------------------------------------------------------------------------------------
visual_fixed = dvz.dvz_visual_fixed
visual_fixed.__doc__ = """
Fix some axes in a visual.

Parameters
----------
visual : DvzVisual*
    the visual
flags : int
    the fixed bitmask (combination of `DVZ_VISUAL_FLAGS_FIXED_X|Y|Z`)
"""
visual_fixed.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
visual_dynamic = dvz.dvz_visual_dynamic
visual_dynamic.__doc__ = """
Declare a dynamic attribute, meaning that it is stored in a separate dat rather than being
interleaved with the other attributes in the same vertex buffer.

Parameters
----------
visual : DvzVisual*
    the visual
attr_idx : int
    the attribute index
binding_idx : int
    the binding index (0 = common vertex buffer, use 1 or 2, 3... for each
"""
visual_dynamic.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t attr_idx
    ctypes.c_uint32,  # uint32_t binding_idx
]


# -------------------------------------------------------------------------------------------------
visual_clip = dvz.dvz_visual_clip
visual_clip.__doc__ = """
Set the visual clipping.

Parameters
----------
visual : DvzVisual*
    the visual
clip : DvzViewportClip
    the viewport clipping
"""
visual_clip.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzViewportClip,  # DvzViewportClip clip
]


# -------------------------------------------------------------------------------------------------
visual_depth = dvz.dvz_visual_depth
visual_depth.__doc__ = """
Set the visual depth.

Parameters
----------
visual : DvzVisual*
    the visual
depth_test : DvzDepthTest
    whether to activate the depth test
"""
visual_depth.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzDepthTest,  # DvzDepthTest depth_test
]


# -------------------------------------------------------------------------------------------------
visual_show = dvz.dvz_visual_show
visual_show.__doc__ = """
Set the visibility of a visual.

Parameters
----------
visual : DvzVisual*
    the visual
is_visible : bool
    the visual visibility
"""
visual_show.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_bool,  # bool is_visible
]


# -------------------------------------------------------------------------------------------------
visual_primitive = dvz.dvz_visual_primitive
visual_primitive.__doc__ = """
Set the primitive topology of a visual.

Parameters
----------
visual : DvzVisual*
    the visual
primitive : DvzPrimitiveTopology
    the primitive topology
"""
visual_primitive.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzPrimitiveTopology,  # DvzPrimitiveTopology primitive
]


# -------------------------------------------------------------------------------------------------
visual_blend = dvz.dvz_visual_blend
visual_blend.__doc__ = """
Set the blend type of a visual.

Parameters
----------
visual : DvzVisual*
    the visual
blend_type : DvzBlendType
    the blend type
"""
visual_blend.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzBlendType,  # DvzBlendType blend_type
]


# -------------------------------------------------------------------------------------------------
visual_polygon = dvz.dvz_visual_polygon
visual_polygon.__doc__ = """
Set the polygon mode of a visual.

Parameters
----------
visual : DvzVisual*
    the visual
polygon_mode : DvzPolygonMode
    the polygon mode
"""
visual_polygon.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzPolygonMode,  # DvzPolygonMode polygon_mode
]


# -------------------------------------------------------------------------------------------------
visual_cull = dvz.dvz_visual_cull
visual_cull.__doc__ = """
Set the cull mode of a visual.

Parameters
----------
visual : DvzVisual*
    the visual
cull_mode : DvzCullMode
    the cull mode
"""
visual_cull.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzCullMode,  # DvzCullMode cull_mode
]


# -------------------------------------------------------------------------------------------------
visual_front = dvz.dvz_visual_front
visual_front.__doc__ = """
Set the front face mode of a visual.

Parameters
----------
visual : DvzVisual*
    the visual
front_face : DvzFrontFace
    the front face mode
"""
visual_front.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzFrontFace,  # DvzFrontFace front_face
]


# -------------------------------------------------------------------------------------------------
visual_push = dvz.dvz_visual_push
visual_push.__doc__ = """
Set a push constant of a visual.

Parameters
----------
visual : DvzVisual*
    the visual
shader_stages : DvzShaderStageFlags
    the shader stage flags
offset : DvzSize
    the offset, in bytes
size : DvzSize
    the size, in bytes
"""
visual_push.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzShaderStageFlags,  # DvzShaderStageFlags shader_stages
    DvzSize,  # DvzSize offset
    DvzSize,  # DvzSize size
]


# -------------------------------------------------------------------------------------------------
visual_specialization = dvz.dvz_visual_specialization
visual_specialization.__doc__ = """
Set a specialization constant of a visual.

Parameters
----------
visual : DvzVisual*
    the visual
shader : DvzShaderType
    the shader type
idx : int
    the specialization constant index
size : DvzSize
    the size, in bytes, of the value passed to this function
value : np.ndarray
    a pointer to the value to use for that specialization constant
"""
visual_specialization.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzShaderType,  # DvzShaderType shader
    ctypes.c_uint32,  # uint32_t idx
    DvzSize,  # DvzSize size
    ctypes.c_void_p,  # void* value
]


# -------------------------------------------------------------------------------------------------
visual_spirv = dvz.dvz_visual_spirv
visual_spirv.__doc__ = """
Set the shader SPIR-V code of a visual.

Parameters
----------
visual : DvzVisual*
    the visual
type : DvzShaderType
    the shader type
size : DvzSize
    the size, in bytes, of the SPIR-V buffer
buffer : str
    a pointer to the SPIR-V buffer
"""
visual_spirv.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzShaderType,  # DvzShaderType type
    DvzSize,  # DvzSize size
    CStringBuffer,  # char* buffer
]


# -------------------------------------------------------------------------------------------------
visual_shader = dvz.dvz_visual_shader
visual_shader.__doc__ = """
Set the shader SPIR-V name of a visual.

Parameters
----------
visual : DvzVisual*
    the visual
name : str
    the built-in resource name of the shader (_vert and _frag are appended)
"""
visual_shader.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    CStringBuffer,  # char* name
]


# -------------------------------------------------------------------------------------------------
visual_resize = dvz.dvz_visual_resize
visual_resize.__doc__ = """
Resize a visual allocation.

Parameters
----------
visual : DvzVisual*
    the visual
item_count : int
    the number of items
vertex_count : int
    the number of vertices
index_count : int
    the number of indices (0 if there is no index buffer)
"""
visual_resize.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t item_count
    ctypes.c_uint32,  # uint32_t vertex_count
    ctypes.c_uint32,  # uint32_t index_count
]


# -------------------------------------------------------------------------------------------------
visual_groups = dvz.dvz_visual_groups
visual_groups.__doc__ = """
Set groups in a visual.

Parameters
----------
visual : DvzVisual*
    the visual
group_count : int
    the number of groups
group_sizes : np.ndarray[uint32_t]
    the size of each group
"""
visual_groups.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t group_count
    ndpointer(dtype=np.uint32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # uint32_t* group_sizes
]


# -------------------------------------------------------------------------------------------------
visual_attr = dvz.dvz_visual_attr
visual_attr.__doc__ = """
Declare a visual attribute.

Parameters
----------
visual : DvzVisual*
    the visual
attr_idx : int
    the attribute index
offset : DvzSize
    the attribute offset within the vertex buffer, in bytes
item_size : DvzSize
    the attribute size, in bytes
format : DvzFormat
    the attribute data format
flags : int
    the attribute flags
"""
visual_attr.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t attr_idx
    DvzSize,  # DvzSize offset
    DvzSize,  # DvzSize item_size
    DvzFormat,  # DvzFormat format
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
visual_stride = dvz.dvz_visual_stride
visual_stride.__doc__ = """
Declare a visual binding.

Parameters
----------
visual : DvzVisual*
    the visual
binding_idx : int
    the binding index
stride : DvzSize
    the binding stride, in bytes
"""
visual_stride.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t binding_idx
    DvzSize,  # DvzSize stride
]


# -------------------------------------------------------------------------------------------------
visual_slot = dvz.dvz_visual_slot
visual_slot.__doc__ = """
Declare a visual slot.

Parameters
----------
visual : DvzVisual*
    the visual
slot_idx : int
    the slot index
type : DvzSlotType
    the slot type
"""
visual_slot.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t slot_idx
    DvzSlotType,  # DvzSlotType type
]


# -------------------------------------------------------------------------------------------------
visual_params = dvz.dvz_visual_params
visual_params.__doc__ = """
Declare a set of visual parameters.

Parameters
----------
visual : DvzVisual*
    the visual
slot_idx : int
    the slot index of the uniform buffer storing the parameter values
size : DvzSize
    the size, in bytes, of that uniform buffer

Returns
-------
result : DvzParams*
"""
visual_params.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t slot_idx
    DvzSize,  # DvzSize size
]
visual_params.restype = ctypes.POINTER(DvzParams)


# -------------------------------------------------------------------------------------------------
visual_dat = dvz.dvz_visual_dat
visual_dat.__doc__ = """
Bind a dat to a visual slot.

Parameters
----------
visual : DvzVisual*
    the visual
slot_idx : int
    the slot index
dat : DvzId
    the dat ID
"""
visual_dat.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t slot_idx
    DvzId,  # DvzId dat
]


# -------------------------------------------------------------------------------------------------
visual_tex = dvz.dvz_visual_tex
visual_tex.__doc__ = """
Bind a tex to a visual slot.

Parameters
----------
visual : DvzVisual*
    the visual
slot_idx : int
    the slot index
tex : DvzId
    the tex ID
sampler : DvzId
    the sampler ID
offset : Tuple[int, int, int]
    the texture offset
"""
visual_tex.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t slot_idx
    DvzId,  # DvzId tex
    DvzId,  # DvzId sampler
    uvec3,  # uvec3 offset
]


# -------------------------------------------------------------------------------------------------
visual_alloc = dvz.dvz_visual_alloc
visual_alloc.__doc__ = """
Allocate a visual.

Parameters
----------
visual : DvzVisual*
    the visual
item_count : int
    the number of items
vertex_count : int
    the number of vertices
index_count : int
    the number of indices
"""
visual_alloc.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t item_count
    ctypes.c_uint32,  # uint32_t vertex_count
    ctypes.c_uint32,  # uint32_t index_count
]


# -------------------------------------------------------------------------------------------------
visual_transform = dvz.dvz_visual_transform
visual_transform.__doc__ = """
Set a visual transform.

Parameters
----------
visual : DvzVisual*
    the visual
tr : DvzTransform*
    the transform
vertex_attr : int
    the vertex attribute on which the transform applies to
"""
visual_transform.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.POINTER(DvzTransform),  # DvzTransform* tr
    ctypes.c_uint32,  # uint32_t vertex_attr
]


# -------------------------------------------------------------------------------------------------
visual_data = dvz.dvz_visual_data
visual_data.__doc__ = """
Set visual data.

Parameters
----------
visual : DvzVisual*
    the visual
attr_idx : int
    the attribute index
first : int
    the index of the first item to set
count : int
    the number of items to set
data : np.ndarray
    a pointer to the data buffer
"""
visual_data.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t attr_idx
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=None, ndim=None, flags="C_CONTIGUOUS"),  # void* data
]


# -------------------------------------------------------------------------------------------------
visual_quads = dvz.dvz_visual_quads
visual_quads.__doc__ = """
Set visual data as quads.

Parameters
----------
visual : DvzVisual*
    the visual
attr_idx : int
    the attribute index
first : int
    the index of the first item to set
count : int
    the number of items to set
tl_br : np.ndarray[vec4]
    a pointer to a buffer of vec4 with the 2D coordinates of the top-left and
"""
visual_quads.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t attr_idx
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # vec4* tl_br
]


# -------------------------------------------------------------------------------------------------
visual_index = dvz.dvz_visual_index
visual_index.__doc__ = """
Set the visual index data.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first index to set
count : int
    the number of indices
data : DvzIndex*
    a pointer to a buffer of DvzIndex (uint32_t) values with the indices
"""
visual_index.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # DvzIndex* data
]


# -------------------------------------------------------------------------------------------------
visual_param = dvz.dvz_visual_param
visual_param.__doc__ = """
Set a visual parameter value.

Parameters
----------
visual : DvzVisual*
    the visual
slot_idx : int
    the slot index
attr_idx : int
    the index of the parameter attribute within the params structure
item : np.ndarray
    a pointer to the value to use for that parameter
"""
visual_param.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t slot_idx
    ctypes.c_uint32,  # uint32_t attr_idx
    ctypes.c_void_p,  # void* item
]


# -------------------------------------------------------------------------------------------------
texture = dvz.dvz_texture
texture.__doc__ = """
Create a texture.

Parameters
----------
batch : DvzBatch*
    the batch
dims : DvzTexDims
    the number of dimensions in the texture
flags : int
    the texture creation flags

Returns
-------
result : DvzTexture*
     the texture
"""
texture.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzTexDims,  # DvzTexDims dims
    ctypes.c_int,  # int flags
]
texture.restype = ctypes.POINTER(DvzTexture)


# -------------------------------------------------------------------------------------------------
texture_shape = dvz.dvz_texture_shape
texture_shape.__doc__ = """
Set the texture shape.

Parameters
----------
texture : DvzTexture*
    the texture
width : int
    the width
height : int
    the height
depth : int
    the depth
"""
texture_shape.argtypes = [
    ctypes.POINTER(DvzTexture),  # DvzTexture* texture
    ctypes.c_uint32,  # uint32_t width
    ctypes.c_uint32,  # uint32_t height
    ctypes.c_uint32,  # uint32_t depth
]


# -------------------------------------------------------------------------------------------------
texture_format = dvz.dvz_texture_format
texture_format.__doc__ = """
Set the texture format.

Parameters
----------
texture : DvzTexture*
    the texture
format : DvzFormat
    the format
"""
texture_format.argtypes = [
    ctypes.POINTER(DvzTexture),  # DvzTexture* texture
    DvzFormat,  # DvzFormat format
]


# -------------------------------------------------------------------------------------------------
texture_filter = dvz.dvz_texture_filter
texture_filter.__doc__ = """
Set the texture's associated sampler's filter (nearest or linear).

Parameters
----------
texture : DvzTexture*
    the texture
filter : DvzFilter
    the filter
"""
texture_filter.argtypes = [
    ctypes.POINTER(DvzTexture),  # DvzTexture* texture
    DvzFilter,  # DvzFilter filter
]


# -------------------------------------------------------------------------------------------------
texture_address_mode = dvz.dvz_texture_address_mode
texture_address_mode.__doc__ = """
Set the texture's associated sampler's address mode.

Parameters
----------
texture : DvzTexture*
    the texture
address_mode : DvzSamplerAddressMode
    the address mode
"""
texture_address_mode.argtypes = [
    ctypes.POINTER(DvzTexture),  # DvzTexture* texture
    DvzSamplerAddressMode,  # DvzSamplerAddressMode address_mode
]


# -------------------------------------------------------------------------------------------------
texture_data = dvz.dvz_texture_data
texture_data.__doc__ = """
Upload all or part of the the texture data.

Parameters
----------
texture : DvzTexture*
    the texture
xoffset : int
    the x offset inside the texture
yoffset : int
    the y offset inside the texture
zoffset : int
    the z offset inside the texture
width : int
    the width of the uploaded image
height : int
    the height of the uploaded image
depth : int
    the depth of the uploaded image
size : DvzSize
    the size of the data buffer
data : np.ndarray
    the data buffer
"""
texture_data.argtypes = [
    ctypes.POINTER(DvzTexture),  # DvzTexture* texture
    ctypes.c_uint32,  # uint32_t xoffset
    ctypes.c_uint32,  # uint32_t yoffset
    ctypes.c_uint32,  # uint32_t zoffset
    ctypes.c_uint32,  # uint32_t width
    ctypes.c_uint32,  # uint32_t height
    ctypes.c_uint32,  # uint32_t depth
    DvzSize,  # DvzSize size
    ndpointer(dtype=None, ndim=None, flags="C_CONTIGUOUS"),  # void* data
]


# -------------------------------------------------------------------------------------------------
texture_create = dvz.dvz_texture_create
texture_create.__doc__ = """
Create the texture once set.

Parameters
----------
texture : DvzTexture*
    the texture
"""
texture_create.argtypes = [
    ctypes.POINTER(DvzTexture),  # DvzTexture* texture
]


# -------------------------------------------------------------------------------------------------
texture_destroy = dvz.dvz_texture_destroy
texture_destroy.__doc__ = """
Destroy a texture.

Parameters
----------
texture : DvzTexture*
    the texture
"""
texture_destroy.argtypes = [
    ctypes.POINTER(DvzTexture),  # DvzTexture* texture
]


# -------------------------------------------------------------------------------------------------
texture_1D = dvz.dvz_texture_1D
texture_1D.__doc__ = """
Create a 1D texture.

Parameters
----------
batch : DvzBatch*
    the batch
format : DvzFormat
    the texture format
filter : DvzFilter
    the filter
address_mode : DvzSamplerAddressMode
    the address mode
width : int
    the texture width
data : np.ndarray
    the texture data to upload
flags : int
    the texture creation flags

Returns
-------
result : DvzTexture*
     the texture
"""
texture_1D.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzFormat,  # DvzFormat format
    DvzFilter,  # DvzFilter filter
    DvzSamplerAddressMode,  # DvzSamplerAddressMode address_mode
    ctypes.c_uint32,  # uint32_t width
    ndpointer(dtype=None, ndim=None, flags="C_CONTIGUOUS"),  # void* data
    ctypes.c_int,  # int flags
]
texture_1D.restype = ctypes.POINTER(DvzTexture)


# -------------------------------------------------------------------------------------------------
texture_2D = dvz.dvz_texture_2D
texture_2D.__doc__ = """
Create a 2D texture to be used in an image visual.

Parameters
----------
batch : DvzBatch*
    the batch
format : DvzFormat
    the texture format
filter : DvzFilter
    the filter
address_mode : DvzSamplerAddressMode
    the address mode
width : int
    the texture width
height : int
    the texture height
data : np.ndarray
    the texture data to upload
flags : int
    the texture creation flags

Returns
-------
result : DvzTexture*
     the texture
"""
texture_2D.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzFormat,  # DvzFormat format
    DvzFilter,  # DvzFilter filter
    DvzSamplerAddressMode,  # DvzSamplerAddressMode address_mode
    ctypes.c_uint32,  # uint32_t width
    ctypes.c_uint32,  # uint32_t height
    ndpointer(dtype=None, ndim=None, flags="C_CONTIGUOUS"),  # void* data
    ctypes.c_int,  # int flags
]
texture_2D.restype = ctypes.POINTER(DvzTexture)


# -------------------------------------------------------------------------------------------------
texture_3D = dvz.dvz_texture_3D
texture_3D.__doc__ = """
Create a 3D texture to be used in a volume visual.

Parameters
----------
batch : DvzBatch*
    the batch
format : DvzFormat
    the texture format
filter : DvzFilter
    the filter
address_mode : DvzSamplerAddressMode
    the address mode
width : int
    the texture width
height : int
    the texture height
depth : int
    the texture depth
data : np.ndarray
    the texture data to upload
flags : int
    the texture creation flags

Returns
-------
result : DvzTexture*
     the texture
"""
texture_3D.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzFormat,  # DvzFormat format
    DvzFilter,  # DvzFilter filter
    DvzSamplerAddressMode,  # DvzSamplerAddressMode address_mode
    ctypes.c_uint32,  # uint32_t width
    ctypes.c_uint32,  # uint32_t height
    ctypes.c_uint32,  # uint32_t depth
    ndpointer(dtype=None, ndim=None, flags="C_CONTIGUOUS"),  # void* data
    ctypes.c_int,  # int flags
]
texture_3D.restype = ctypes.POINTER(DvzTexture)


# -------------------------------------------------------------------------------------------------
colormap = dvz.dvz_colormap
colormap.__doc__ = """
Fetch a color from a colormap and a value (either 8-bit or float, depending on DVZ_COLOR_CVEC4).

Parameters
----------
cmap : DvzColormap
    the colormap
value : uint8_t
    the value
color : Out[Tuple[int, int, int, int]] (out parameter)
    the fetched color
"""
colormap.argtypes = [
    DvzColormap,  # DvzColormap cmap
    ctypes.c_uint8,  # uint8_t value
    DvzColor,  # out DvzColor color
]


# -------------------------------------------------------------------------------------------------
colormap_8bit = dvz.dvz_colormap_8bit
colormap_8bit.__doc__ = """
Fetch a color from a colormap and a value (8-bit version).

Parameters
----------
cmap : DvzColormap
    the colormap
value : uint8_t
    the value
color : Out[cvec4] (out parameter)
    the fetched color
"""
colormap_8bit.argtypes = [
    DvzColormap,  # DvzColormap cmap
    ctypes.c_uint8,  # uint8_t value
    cvec4,  # out cvec4 color
]


# -------------------------------------------------------------------------------------------------
colormap_scale = dvz.dvz_colormap_scale
colormap_scale.__doc__ = """
Fetch a color from a colormap and an interpolated value.

Parameters
----------
cmap : DvzColormap
    the colormap
value : float
    the value
vmin : float
    the minimum value
vmax : float
    the maximum value
color : Out[Tuple[int, int, int, int]] (out parameter)
    the fetched color
"""
colormap_scale.argtypes = [
    DvzColormap,  # DvzColormap cmap
    ctypes.c_float,  # float value
    ctypes.c_float,  # float vmin
    ctypes.c_float,  # float vmax
    DvzColor,  # out DvzColor color
]


# -------------------------------------------------------------------------------------------------
colormap_array = dvz.dvz_colormap_array
colormap_array.__doc__ = """
Fetch colors from a colormap and an array of values.

Parameters
----------
cmap : DvzColormap
    the colormap
count : int
    the number of values
values : np.ndarray[float]
    pointer to the array of float numbers
vmin : float
    the minimum value
vmax : float
    the maximum value
out : Out[Tuple[int, int, int, int]] (out parameter)
    (array) the fetched colors
"""
colormap_array.argtypes = [
    DvzColormap,  # DvzColormap cmap
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # float* values
    ctypes.c_float,  # float vmin
    ctypes.c_float,  # float vmax
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # out DvzColor* out
]


# -------------------------------------------------------------------------------------------------
sdf_from_svg = dvz.dvz_sdf_from_svg
sdf_from_svg.__doc__ = """
Generate an SDF from an SVG path.

Parameters
----------
svg_path : str
    the SVG path
width : int
    the width of the generated SDF, in pixels
height : int
    the height of the generated SDF, in pixels

Returns
-------
result : float*
     the generated texture as RGB floats
"""
sdf_from_svg.argtypes = [
    CStringBuffer,  # char* svg_path
    ctypes.c_uint32,  # uint32_t width
    ctypes.c_uint32,  # uint32_t height
]
sdf_from_svg.restype = ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS")


# -------------------------------------------------------------------------------------------------
msdf_from_svg = dvz.dvz_msdf_from_svg
msdf_from_svg.__doc__ = """
Generate a multichannel SDF from an SVG path.

Parameters
----------
svg_path : str
    the SVG path
width : int
    the width of the generated SDF, in pixels
height : int
    the height of the generated SDF, in pixels

Returns
-------
result : float*
     the generated texture as RGB floats
"""
msdf_from_svg.argtypes = [
    CStringBuffer,  # char* svg_path
    ctypes.c_uint32,  # uint32_t width
    ctypes.c_uint32,  # uint32_t height
]
msdf_from_svg.restype = ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS")


# -------------------------------------------------------------------------------------------------
sdf_to_rgb = dvz.dvz_sdf_to_rgb
sdf_to_rgb.__doc__ = """
Convert an SDF float texture to a byte texture.

Parameters
----------
sdf : np.ndarray[float]
    the SDF float texture
width : int
    the width of the texture
height : int
    the height of the texture

Returns
-------
result : uint8_t*
     the byte texture
"""
sdf_to_rgb.argtypes = [
    ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # float* sdf
    ctypes.c_uint32,  # uint32_t width
    ctypes.c_uint32,  # uint32_t height
]
sdf_to_rgb.restype = ndpointer(dtype=np.uint8, ndim=1, ncol=1, flags="C_CONTIGUOUS")


# -------------------------------------------------------------------------------------------------
msdf_to_rgb = dvz.dvz_msdf_to_rgb
msdf_to_rgb.__doc__ = """
Convert a multichannel SDF float texture to a byte texture.

Parameters
----------
sdf : np.ndarray[float]
    the SDF float texture
width : int
    the width of the texture
height : int
    the height of the texture

Returns
-------
result : uint8_t*
     the byte texture
"""
msdf_to_rgb.argtypes = [
    ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # float* sdf
    ctypes.c_uint32,  # uint32_t width
    ctypes.c_uint32,  # uint32_t height
]
msdf_to_rgb.restype = ndpointer(dtype=np.uint8, ndim=1, ncol=1, flags="C_CONTIGUOUS")


# -------------------------------------------------------------------------------------------------
rgb_to_rgba_char = dvz.dvz_rgb_to_rgba_char
rgb_to_rgba_char.__doc__ = """
Convert an RGB byte texture to an RGBA one.

Parameters
----------
count : int
    the number of pixels (and NOT the number of bytes) in the byte texture
rgb : np.ndarray[uint8_t]
    the RGB texture
rgba : np.ndarray[uint8_t]
    the returned RGBA texture
"""
rgb_to_rgba_char.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint8, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # uint8_t* rgb
    ndpointer(dtype=np.uint8, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # uint8_t* rgba
]


# -------------------------------------------------------------------------------------------------
rgb_to_rgba_float = dvz.dvz_rgb_to_rgba_float
rgb_to_rgba_float.__doc__ = """
Convert an RGB float texture to an RGBA one.

Parameters
----------
count : int
    the number of pixels (and NOT the number of bytes) in the float texture
rgb : np.ndarray[float]
    the RGB texture
rgba : np.ndarray[float]
    the returned RGBA texture
"""
rgb_to_rgba_float.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # float* rgb
    ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # float* rgba
]


# -------------------------------------------------------------------------------------------------
compute_normals = dvz.dvz_compute_normals
compute_normals.__doc__ = """
Compute face normals.

Parameters
----------
vertex_count : int
    number of vertices
index_count : int
    number of indices (triple of the number of faces)
pos : np.ndarray[vec3]
    array of vec3 positions
index : DvzIndex*
    pos array of uint32_t indices
normal : Out[Tuple[float, float, float]] (out parameter)
    (array) the vec3 normals (to be overwritten by this function)
"""
compute_normals.argtypes = [
    ctypes.c_uint32,  # uint32_t vertex_count
    ctypes.c_uint32,  # uint32_t index_count
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* pos
    ndpointer(dtype=np.uint32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # DvzIndex* index
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # out vec3* normal
]


# -------------------------------------------------------------------------------------------------
shape_normals = dvz.dvz_shape_normals
shape_normals.__doc__ = """
Recompute the face normals.

Parameters
----------
shape : DvzShape*
    the shape
"""
shape_normals.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
]


# -------------------------------------------------------------------------------------------------
shape_merge = dvz.dvz_shape_merge
shape_merge.__doc__ = """
Merge several shapes.

Parameters
----------
shape : DvzShape*
    the merged shape
count : int
    the number of shapes to merge
shapes : DvzShape**
    the shapes to merge
"""
shape_merge.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    ctypes.c_uint32,  # uint32_t count
    ctypes.POINTER(ctypes.POINTER(Shape)),  # DvzShape** shapes
]


# -------------------------------------------------------------------------------------------------
shape_print = dvz.dvz_shape_print
shape_print.__doc__ = """
Show information about a shape.

Parameters
----------
shape : DvzShape*
    the shape
"""
shape_print.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
]


# -------------------------------------------------------------------------------------------------
shape_vertex_count = dvz.dvz_shape_vertex_count
shape_vertex_count.__doc__ = """
Return the number of vertices of a shape.

Parameters
----------
shape : DvzShape*
    the shape

Returns
-------
result : uint32_t
     the number of vertices
"""
shape_vertex_count.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
]
shape_vertex_count.restype = ctypes.c_uint32


# -------------------------------------------------------------------------------------------------
shape_index_count = dvz.dvz_shape_index_count
shape_index_count.__doc__ = """
Return the number of index of a shape.

Parameters
----------
shape : DvzShape*
    the shape

Returns
-------
result : uint32_t
     the number of index
"""
shape_index_count.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
]
shape_index_count.restype = ctypes.c_uint32


# -------------------------------------------------------------------------------------------------
shape_unindex = dvz.dvz_shape_unindex
shape_unindex.__doc__ = """
Convert an indexed shape to a non-indexed one by duplicating the vertex values according
to the indices.
This is used by the mesh wireframe option, as a given vertex may have distinct barycentric
coordinates depending on its index.

Parameters
----------
shape : DvzShape*
    the shape
flags : int
    the flags
"""
shape_unindex.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
shape_destroy = dvz.dvz_shape_destroy
shape_destroy.__doc__ = """
Destroy a shape.

Parameters
----------
shape : DvzShape*
    the shape
"""
shape_destroy.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
]


# -------------------------------------------------------------------------------------------------
shape_begin = dvz.dvz_shape_begin
shape_begin.__doc__ = """
Start a transformation sequence.

Parameters
----------
shape : DvzShape*
    the shape
first : int
    the first vertex to modify
count : int
    the number of vertices to modify
"""
shape_begin.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
]


# -------------------------------------------------------------------------------------------------
shape_scale = dvz.dvz_shape_scale
shape_scale.__doc__ = """
Append a scaling transform to a shape.

Parameters
----------
shape : DvzShape*
    the shape
scale : Tuple[float, float, float]
    the scaling factors
"""
shape_scale.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    vec3,  # vec3 scale
]


# -------------------------------------------------------------------------------------------------
shape_translate = dvz.dvz_shape_translate
shape_translate.__doc__ = """
Append a translation to a shape.

Parameters
----------
shape : DvzShape*
    the shape
translate : Tuple[float, float, float]
    the translation vector
"""
shape_translate.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    vec3,  # vec3 translate
]


# -------------------------------------------------------------------------------------------------
shape_rotate = dvz.dvz_shape_rotate
shape_rotate.__doc__ = """
Append a rotation to a shape.

Parameters
----------
shape : DvzShape*
    the shape
angle : float
    the rotation angle
axis : Tuple[float, float, float]
    the rotation axis
"""
shape_rotate.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    ctypes.c_float,  # float angle
    vec3,  # vec3 axis
]


# -------------------------------------------------------------------------------------------------
shape_transform = dvz.dvz_shape_transform
shape_transform.__doc__ = """
Append an arbitrary transformation.

Parameters
----------
shape : DvzShape*
    the shape
transform : mat4
    the transform mat4 matrix
"""
shape_transform.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    mat4,  # mat4 transform
]


# -------------------------------------------------------------------------------------------------
shape_rescaling = dvz.dvz_shape_rescaling
shape_rescaling.__doc__ = """
Compute the rescaling factor to renormalize a shape.

Parameters
----------
shape : DvzShape*
    the shape
flags : int
    the rescaling flags
out_scale : Out[Tuple[float, float, float]] (out parameter)
    the computed scaling factors

Returns
-------
result : float
"""
shape_rescaling.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    ctypes.c_int,  # int flags
    vec3,  # out vec3 out_scale
]
shape_rescaling.restype = ctypes.c_float


# -------------------------------------------------------------------------------------------------
shape_end = dvz.dvz_shape_end
shape_end.__doc__ = """
Apply the transformation sequence and reset it.

Parameters
----------
shape : DvzShape*
    the shape
"""
shape_end.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
]


# -------------------------------------------------------------------------------------------------
shape_square = dvz.dvz_shape_square
shape_square.__doc__ = """
Create a square shape.

Parameters
----------
shape : DvzShape*
    the shape
color : Tuple[int, int, int, int]
    the square color
"""
shape_square.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    DvzColor,  # DvzColor color
]


# -------------------------------------------------------------------------------------------------
shape_disc = dvz.dvz_shape_disc
shape_disc.__doc__ = """
Create a disc shape.

Parameters
----------
shape : DvzShape*
    the shape
count : int
    the number of points along the disc border
color : Tuple[int, int, int, int]
    the disc color
"""
shape_disc.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    ctypes.c_uint32,  # uint32_t count
    DvzColor,  # DvzColor color
]


# -------------------------------------------------------------------------------------------------
shape_sector = dvz.dvz_shape_sector
shape_sector.__doc__ = """
Create a sector shape.

Parameters
----------
shape : DvzShape*
    the shape
count : int
    the number of points along the sector border
angle_start : float
    the initial angle
angle_stop : float
    the final angle
color : Tuple[int, int, int, int]
    the sector color
"""
shape_sector.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    ctypes.c_uint32,  # uint32_t count
    ctypes.c_float,  # float angle_start
    ctypes.c_float,  # float angle_stop
    DvzColor,  # DvzColor color
]


# -------------------------------------------------------------------------------------------------
shape_histogram = dvz.dvz_shape_histogram
shape_histogram.__doc__ = """
Create a histogram shape.

Parameters
----------
shape : DvzShape*
    the shape
count : int
    the number of bars
heights : np.ndarray[float]
    the height of each bar
color : Tuple[int, int, int, int]
    the sector color
"""
shape_histogram.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # float* heights
    DvzColor,  # DvzColor color
]


# -------------------------------------------------------------------------------------------------
shape_polygon = dvz.dvz_shape_polygon
shape_polygon.__doc__ = """
Create a polygon shape using the simple earcut polygon triangulation algorithm.

Parameters
----------
shape : DvzShape*
    the shape
count : int
    the number of points along the polygon border
points : np.ndarray[dvec2]
    the points 2D coordinates
color : Tuple[int, int, int, int]
    the polygon color
"""
shape_polygon.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.double, ndim=2, ncol=2, flags="C_CONTIGUOUS"),  # dvec2* points
    DvzColor,  # DvzColor color
]


# -------------------------------------------------------------------------------------------------
shape = dvz.dvz_shape
shape.__doc__ = """
Create an empty shape.


Returns
-------
result : DvzShape*
     the shape
"""
shape.argtypes = [
]
shape.restype = ctypes.POINTER(DvzShape)


# -------------------------------------------------------------------------------------------------
shape_surface = dvz.dvz_shape_surface
shape_surface.__doc__ = """
Create a grid shape.

Parameters
----------
shape : DvzShape*
    the shape
row_count : int
    number of rows
col_count : int
    number of cols
heights : np.ndarray[float]
    a pointer to row_count*col_count height values (floats)
colors : DvzColor*
    a pointer to row_count*col_count color values (DvzColor: cvec4 or vec4)
o : Tuple[float, float, float]
    the origin
u : Tuple[float, float, float]
    the unit vector parallel to each column
v : Tuple[float, float, float]
    the unit vector parallel to each row
flags : int
    the grid creation flags
"""
shape_surface.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    ctypes.c_uint32,  # uint32_t row_count
    ctypes.c_uint32,  # uint32_t col_count
    ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # float* heights
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # DvzColor* colors
    vec3,  # vec3 o
    vec3,  # vec3 u
    vec3,  # vec3 v
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
shape_cube = dvz.dvz_shape_cube
shape_cube.__doc__ = """
Create a cube shape.

Parameters
----------
shape : DvzShape*
    the shape
colors : DvzColor*
    the colors of the six faces
"""
shape_cube.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # DvzColor* colors
]


# -------------------------------------------------------------------------------------------------
shape_sphere = dvz.dvz_shape_sphere
shape_sphere.__doc__ = """
Create a sphere shape.

Parameters
----------
shape : DvzShape*
    the shape
rows : int
    the number of rows
cols : int
    the number of columns
color : Tuple[int, int, int, int]
    the sphere color
"""
shape_sphere.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    ctypes.c_uint32,  # uint32_t rows
    ctypes.c_uint32,  # uint32_t cols
    DvzColor,  # DvzColor color
]


# -------------------------------------------------------------------------------------------------
shape_cylinder = dvz.dvz_shape_cylinder
shape_cylinder.__doc__ = """
Create a cylinder shape.

Parameters
----------
shape : DvzShape*
    the shape
count : int
    the number of points along the cylinder border
color : Tuple[int, int, int, int]
    the cylinder color
"""
shape_cylinder.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    ctypes.c_uint32,  # uint32_t count
    DvzColor,  # DvzColor color
]


# -------------------------------------------------------------------------------------------------
shape_cone = dvz.dvz_shape_cone
shape_cone.__doc__ = """
Create a cone shape.

Parameters
----------
shape : DvzShape*
    the shape
count : int
    the number of points along the disc border
color : Tuple[int, int, int, int]
    the cone color
"""
shape_cone.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    ctypes.c_uint32,  # uint32_t count
    DvzColor,  # DvzColor color
]


# -------------------------------------------------------------------------------------------------
shape_arrow = dvz.dvz_shape_arrow
shape_arrow.__doc__ = """
Create a 3D arrow using a cylinder and cone.
The total length is 1.

Parameters
----------
shape : DvzShape*
    the shape
count : int
    the number of sides to the shaft and head
head_length : float
    the length of the head
head_radius : float
    the radius of the head
shaft_radius : float
    the radius of the shaft
color : Tuple[int, int, int, int]
    the arrow color
"""
shape_arrow.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    ctypes.c_uint32,  # uint32_t count
    ctypes.c_float,  # float head_length
    ctypes.c_float,  # float head_radius
    ctypes.c_float,  # float shaft_radius
    DvzColor,  # DvzColor color
]


# -------------------------------------------------------------------------------------------------
shape_gizmo = dvz.dvz_shape_gizmo
shape_gizmo.__doc__ = """
Create a 3D gizmo with three arrows on the three axes.

Parameters
----------
shape : DvzShape*
    the shape
"""
shape_gizmo.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
]


# -------------------------------------------------------------------------------------------------
shape_torus = dvz.dvz_shape_torus
shape_torus.__doc__ = """
Create a torus shape.
The radius of the ring is 0.5.

Parameters
----------
shape : DvzShape*
    the shape
count_radial : int
    the number of points around the ring
count_tubular : int
    the number of points in each cross-section
tube_radius : float
    the radius of the tube.
color : Tuple[int, int, int, int]
    the torus color
"""
shape_torus.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    ctypes.c_uint32,  # uint32_t count_radial
    ctypes.c_uint32,  # uint32_t count_tubular
    ctypes.c_float,  # float tube_radius
    DvzColor,  # DvzColor color
]


# -------------------------------------------------------------------------------------------------
shape_tetrahedron = dvz.dvz_shape_tetrahedron
shape_tetrahedron.__doc__ = """
Create a tetrahedron.

Parameters
----------
shape : DvzShape*
    the shape
color : Tuple[int, int, int, int]
    the color
"""
shape_tetrahedron.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    DvzColor,  # DvzColor color
]


# -------------------------------------------------------------------------------------------------
shape_hexahedron = dvz.dvz_shape_hexahedron
shape_hexahedron.__doc__ = """
Create a tetrahedron.

Parameters
----------
shape : DvzShape*
    the shape
color : Tuple[int, int, int, int]
    the color
"""
shape_hexahedron.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    DvzColor,  # DvzColor color
]


# -------------------------------------------------------------------------------------------------
shape_octahedron = dvz.dvz_shape_octahedron
shape_octahedron.__doc__ = """
Create a octahedron.

Parameters
----------
shape : DvzShape*
    the shape
color : Tuple[int, int, int, int]
    the color
"""
shape_octahedron.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    DvzColor,  # DvzColor color
]


# -------------------------------------------------------------------------------------------------
shape_dodecahedron = dvz.dvz_shape_dodecahedron
shape_dodecahedron.__doc__ = """
Create a dodecahedron.

Parameters
----------
shape : DvzShape*
    the shape
color : Tuple[int, int, int, int]
    the color
"""
shape_dodecahedron.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    DvzColor,  # DvzColor color
]


# -------------------------------------------------------------------------------------------------
shape_icosahedron = dvz.dvz_shape_icosahedron
shape_icosahedron.__doc__ = """
Create a icosahedron.

Parameters
----------
shape : DvzShape*
    the shape
color : Tuple[int, int, int, int]
    the color
"""
shape_icosahedron.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    DvzColor,  # DvzColor color
]


# -------------------------------------------------------------------------------------------------
shape_normalize = dvz.dvz_shape_normalize
shape_normalize.__doc__ = """
Normalize a shape.

Parameters
----------
shape : DvzShape*
    the shape
"""
shape_normalize.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
]


# -------------------------------------------------------------------------------------------------
shape_obj = dvz.dvz_shape_obj
shape_obj.__doc__ = """
Load a .obj shape.

Parameters
----------
shape : DvzShape*
    the shape
file_path : str
    the path to the .obj file
"""
shape_obj.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    CStringBuffer,  # char* file_path
]


# -------------------------------------------------------------------------------------------------
shape_custom = dvz.dvz_shape_custom
shape_custom.__doc__ = """
Create a shape out of an array of vertices and faces.

Parameters
----------
shape : DvzShape*
    the shape
vertex_count : int
    number of vertices
positions : np.ndarray[vec3]
    3D positions of the vertices
normals : np.ndarray[vec3]
    normal vectors (optional, will be otherwise computed automatically)
colors : DvzColor*
    vertex vectors (optional)
texcoords : np.ndarray[vec4]
    texture uv*a coordinates (optional)
index_count : int
    number of indices (3x the number of triangular faces)
indices : DvzIndex*
    vertex indices, three per face
"""
shape_custom.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    ctypes.c_uint32,  # uint32_t vertex_count
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* positions
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* normals
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # DvzColor* colors
    ndpointer(dtype=np.float32, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # vec4* texcoords
    ctypes.c_uint32,  # uint32_t index_count
    ndpointer(dtype=np.uint32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # DvzIndex* indices
]


# -------------------------------------------------------------------------------------------------
atlas_font = dvz.dvz_atlas_font
atlas_font.__doc__ = """
Load the default atlas and font.

Parameters
----------
font_size : float
    the font size
af : Out[DvzAtlasFont] (out parameter)
    the returned DvzAtlasFont object with DvzAtlas and DvzFont objects.
"""
atlas_font.argtypes = [
    ctypes.c_float,  # float font_size
    ctypes.POINTER(DvzAtlasFont),  # out DvzAtlasFont* af
]


# -------------------------------------------------------------------------------------------------
atlas_destroy = dvz.dvz_atlas_destroy
atlas_destroy.__doc__ = """
Destroy an atlas.

Parameters
----------
atlas : DvzAtlas*
    the atlas
"""
atlas_destroy.argtypes = [
    ctypes.POINTER(DvzAtlas),  # DvzAtlas* atlas
]


# -------------------------------------------------------------------------------------------------
font = dvz.dvz_font
font.__doc__ = """
Create a font.

Parameters
----------
ttf_size : long
    size in bytes of a TTF font raw buffer
ttf_bytes : str
    TTF font raw buffer

Returns
-------
result : DvzFont*
     the font
"""
font.argtypes = [
    ctypes.c_long,  # long ttf_size
    CStringBuffer,  # char* ttf_bytes
]
font.restype = ctypes.POINTER(DvzFont)


# -------------------------------------------------------------------------------------------------
font_size = dvz.dvz_font_size
font_size.__doc__ = """
Set the font size.

Parameters
----------
font : DvzFont*
    the font
size : float
    the font size
"""
font_size.argtypes = [
    ctypes.POINTER(DvzFont),  # DvzFont* font
    ctypes.c_double,  # double size
]


# -------------------------------------------------------------------------------------------------
font_layout = dvz.dvz_font_layout
font_layout.__doc__ = """
Compute the shift of each glyph in a Unicode string, using the Freetype library.

Parameters
----------
font : DvzFont*
    the font
length : int
    the number of glyphs
codepoints : np.ndarray[uint32_t]
    the Unicode codepoints of the glyphs
xywh : np.ndarray[vec4]
    an array of (x,y,w,h) shifts
"""
font_layout.argtypes = [
    ctypes.POINTER(DvzFont),  # DvzFont* font
    ctypes.c_uint32,  # uint32_t length
    ndpointer(dtype=np.uint32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # uint32_t* codepoints
    ndpointer(dtype=np.float32, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # vec4* xywh
]


# -------------------------------------------------------------------------------------------------
font_ascii = dvz.dvz_font_ascii
font_ascii.__doc__ = """
Compute the shift of each glyph in an ASCII string, using the Freetype library.

Parameters
----------
font : DvzFont*
    the font
string : str
    the ASCII string
xywh : np.ndarray[vec4]
    the returned array of (x,y,w,h) shifts
"""
font_ascii.argtypes = [
    ctypes.POINTER(DvzFont),  # DvzFont* font
    CStringBuffer,  # char* string
    ndpointer(dtype=np.float32, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # vec4* xywh
]


# -------------------------------------------------------------------------------------------------
font_draw = dvz.dvz_font_draw
font_draw.__doc__ = """
Render a string using Freetype.
Note: the caller must free the output after use.

Parameters
----------
font : DvzFont*
    the font
length : int
    the number of glyphs
codepoints : np.ndarray[uint32_t]
    the Unicode codepoints of the glyphs
xywh : np.ndarray[vec4]
    an array of (x,y,w,h) shifts, returned by dvz_font_layout()
flags : int
    the font flags
out_size : Out[Tuple[int, int]] (out parameter)
    the number of bytes in the returned image

Returns
-------
result : uint8_t*
     an RGBA array allocated by this function and that MUST be freed by the caller
"""
font_draw.argtypes = [
    ctypes.POINTER(DvzFont),  # DvzFont* font
    ctypes.c_uint32,  # uint32_t length
    ndpointer(dtype=np.uint32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # uint32_t* codepoints
    ndpointer(dtype=np.float32, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # vec4* xywh
    ctypes.c_int,  # int flags
    uvec2,  # out uvec2 out_size
]
font_draw.restype = ndpointer(dtype=np.uint8, ndim=1, ncol=1, flags="C_CONTIGUOUS")


# -------------------------------------------------------------------------------------------------
font_texture = dvz.dvz_font_texture
font_texture.__doc__ = """
Generate a texture with a rendered text.

Parameters
----------
font : DvzFont*
    the font
batch : DvzBatch*
    the batch
length : int
    the number of Unicode codepoints
codepoints : np.ndarray[uint32_t]
    the Unicode codepoints
size : Out[Tuple[int, int, int]] (out parameter)
    the generated texture size

Returns
-------
result : DvzTexture*
     the texture
"""
font_texture.argtypes = [
    ctypes.POINTER(DvzFont),  # DvzFont* font
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_uint32,  # uint32_t length
    ndpointer(dtype=np.uint32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # uint32_t* codepoints
    uvec3,  # out uvec3 size
]
font_texture.restype = ctypes.POINTER(DvzTexture)


# -------------------------------------------------------------------------------------------------
font_destroy = dvz.dvz_font_destroy
font_destroy.__doc__ = """
Destroy a font.

Parameters
----------
font : DvzFont*
    the font
"""
font_destroy.argtypes = [
    ctypes.POINTER(DvzFont),  # DvzFont* font
]


# -------------------------------------------------------------------------------------------------
resample = dvz.dvz_resample
resample.__doc__ = """
Normalize a value in an interval.

Parameters
----------
t0 : float
    the interval start
t1 : float
    the interval end
t : float
    the value within the interval

Returns
-------
result : double
     the normalized value between 0 and 1
"""
resample.argtypes = [
    ctypes.c_double,  # double t0
    ctypes.c_double,  # double t1
    ctypes.c_double,  # double t
]
resample.restype = ctypes.c_double


# -------------------------------------------------------------------------------------------------
easing = dvz.dvz_easing
easing.__doc__ = """
Apply an easing function to a normalized value.

Parameters
----------
easing : DvzEasing
    the easing mode
t : float
    the normalized value

Returns
-------
result : double
     the eased value
"""
easing.argtypes = [
    DvzEasing,  # DvzEasing easing
    ctypes.c_double,  # double t
]
easing.restype = ctypes.c_double


# -------------------------------------------------------------------------------------------------
circular_2D = dvz.dvz_circular_2D
circular_2D.__doc__ = """
Generate a 2D circular motion.

Parameters
----------
center : Tuple[float, float]
    the circle center
radius : float
    the circle radius
angle : float
    the initial angle
t : float
    the normalized value
out : Out[Tuple[float, float]] (out parameter)
    the 2D position
"""
circular_2D.argtypes = [
    vec2,  # vec2 center
    ctypes.c_float,  # float radius
    ctypes.c_float,  # float angle
    ctypes.c_float,  # float t
    vec2,  # out vec2 out
]


# -------------------------------------------------------------------------------------------------
circular_3D = dvz.dvz_circular_3D
circular_3D.__doc__ = """
Generate a 3D circular motion.

Parameters
----------
pos_init : Tuple[float, float, float]
    the initial position
center : Tuple[float, float, float]
    the center position
axis : Tuple[float, float, float]
    the axis around which to rotate
t : float
    the normalized value (1 = full circle)
out : Out[Tuple[float, float, float]] (out parameter)
    the 3D position
"""
circular_3D.argtypes = [
    vec3,  # vec3 pos_init
    vec3,  # vec3 center
    vec3,  # vec3 axis
    ctypes.c_float,  # float t
    vec3,  # out vec3 out
]


# -------------------------------------------------------------------------------------------------
interpolate = dvz.dvz_interpolate
interpolate.__doc__ = """
Make a linear interpolation between two scalar value.

Parameters
----------
p0 : float
    the first value
p1 : float
    the second value
t : float
    the normalized value

Returns
-------
result : float
     the interpolated value
"""
interpolate.argtypes = [
    ctypes.c_float,  # float p0
    ctypes.c_float,  # float p1
    ctypes.c_float,  # float t
]
interpolate.restype = ctypes.c_float


# -------------------------------------------------------------------------------------------------
interpolate_2D = dvz.dvz_interpolate_2D
interpolate_2D.__doc__ = """
Make a linear interpolation between two 2D points.

Parameters
----------
p0 : Tuple[float, float]
    the first point
p1 : Tuple[float, float]
    the second point
t : float
    the normalized value
out : Out[Tuple[float, float]] (out parameter)
    the interpolated point
"""
interpolate_2D.argtypes = [
    vec2,  # vec2 p0
    vec2,  # vec2 p1
    ctypes.c_float,  # float t
    vec2,  # out vec2 out
]


# -------------------------------------------------------------------------------------------------
interpolate_3D = dvz.dvz_interpolate_3D
interpolate_3D.__doc__ = """
Make a linear interpolation between two 3D points.

Parameters
----------
p0 : Tuple[float, float, float]
    the first point
p1 : Tuple[float, float, float]
    the second point
t : float
    the normalized value
out : Out[Tuple[float, float, float]] (out parameter)
    the interpolated point
"""
interpolate_3D.argtypes = [
    vec3,  # vec3 p0
    vec3,  # vec3 p1
    ctypes.c_float,  # float t
    vec3,  # out vec3 out
]


# -------------------------------------------------------------------------------------------------
arcball_initial = dvz.dvz_arcball_initial
arcball_initial.__doc__ = """
Set the initial arcball angles.

Parameters
----------
arcball : DvzArcball*
    the arcball
angles : Tuple[float, float, float]
    the initial angles
"""
arcball_initial.argtypes = [
    ctypes.POINTER(DvzArcball),  # DvzArcball* arcball
    vec3,  # vec3 angles
]


# -------------------------------------------------------------------------------------------------
arcball_reset = dvz.dvz_arcball_reset
arcball_reset.__doc__ = """
Reset an arcball to its initial position.

Parameters
----------
arcball : DvzArcball*
    the arcball
"""
arcball_reset.argtypes = [
    ctypes.POINTER(DvzArcball),  # DvzArcball* arcball
]


# -------------------------------------------------------------------------------------------------
arcball_resize = dvz.dvz_arcball_resize
arcball_resize.__doc__ = """
Inform an arcball of a panel resize.

Parameters
----------
arcball : DvzArcball*
    the arcball
width : float
    the panel width
height : float
    the panel height
"""
arcball_resize.argtypes = [
    ctypes.POINTER(DvzArcball),  # DvzArcball* arcball
    ctypes.c_float,  # float width
    ctypes.c_float,  # float height
]


# -------------------------------------------------------------------------------------------------
arcball_flags = dvz.dvz_arcball_flags
arcball_flags.__doc__ = """
Set the arcball flags.

Parameters
----------
arcball : DvzArcball*
    the arcball
flags : int
    the flags
"""
arcball_flags.argtypes = [
    ctypes.POINTER(DvzArcball),  # DvzArcball* arcball
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
arcball_constrain = dvz.dvz_arcball_constrain
arcball_constrain.__doc__ = """
Add arcball constraints.

Parameters
----------
arcball : DvzArcball*
    the arcball
constrain : Tuple[float, float, float]
    the constrain values
"""
arcball_constrain.argtypes = [
    ctypes.POINTER(DvzArcball),  # DvzArcball* arcball
    vec3,  # vec3 constrain
]


# -------------------------------------------------------------------------------------------------
arcball_set = dvz.dvz_arcball_set
arcball_set.__doc__ = """
Set the arcball angles.

Parameters
----------
arcball : DvzArcball*
    the arcball
angles : Tuple[float, float, float]
    the angles
"""
arcball_set.argtypes = [
    ctypes.POINTER(DvzArcball),  # DvzArcball* arcball
    vec3,  # vec3 angles
]


# -------------------------------------------------------------------------------------------------
arcball_angles = dvz.dvz_arcball_angles
arcball_angles.__doc__ = """
Get the current arcball angles.

Parameters
----------
arcball : DvzArcball*
    the arcball
out_angles : Out[Tuple[float, float, float]] (out parameter)
    the arcball angles
"""
arcball_angles.argtypes = [
    ctypes.POINTER(DvzArcball),  # DvzArcball* arcball
    vec3,  # out vec3 out_angles
]


# -------------------------------------------------------------------------------------------------
arcball_rotate = dvz.dvz_arcball_rotate
arcball_rotate.__doc__ = """
Apply a rotation to an arcball.

Parameters
----------
arcball : DvzArcball*
    the arcball
cur_pos : Tuple[float, float]
    the initial position
last_pos : Tuple[float, float]
    the final position
"""
arcball_rotate.argtypes = [
    ctypes.POINTER(DvzArcball),  # DvzArcball* arcball
    vec2,  # vec2 cur_pos
    vec2,  # vec2 last_pos
]


# -------------------------------------------------------------------------------------------------
arcball_model = dvz.dvz_arcball_model
arcball_model.__doc__ = """
Return the model matrix of an arcball.

Parameters
----------
arcball : DvzArcball*
    the arcball
model : Out[mat4] (out parameter)
    the model
"""
arcball_model.argtypes = [
    ctypes.POINTER(DvzArcball),  # DvzArcball* arcball
    mat4,  # out mat4 model
]


# -------------------------------------------------------------------------------------------------
arcball_end = dvz.dvz_arcball_end
arcball_end.__doc__ = """
Finalize arcball position update.

Parameters
----------
arcball : DvzArcball*
    the arcball
"""
arcball_end.argtypes = [
    ctypes.POINTER(DvzArcball),  # DvzArcball* arcball
]


# -------------------------------------------------------------------------------------------------
arcball_mvp = dvz.dvz_arcball_mvp
arcball_mvp.__doc__ = """
Apply an MVP matrix to an arcball (only the model matrix).

Parameters
----------
arcball : DvzArcball*
    the arcball
mvp : DvzMVP*
    the MVP
"""
arcball_mvp.argtypes = [
    ctypes.POINTER(DvzArcball),  # DvzArcball* arcball
    ctypes.POINTER(DvzMVP),  # DvzMVP* mvp
]


# -------------------------------------------------------------------------------------------------
arcball_print = dvz.dvz_arcball_print
arcball_print.__doc__ = """
Display information about an arcball.

Parameters
----------
arcball : DvzArcball*
    the arcball
"""
arcball_print.argtypes = [
    ctypes.POINTER(DvzArcball),  # DvzArcball* arcball
]


# -------------------------------------------------------------------------------------------------
arcball_gui = dvz.dvz_arcball_gui
arcball_gui.__doc__ = """
Show a GUI with sliders controlling the three arcball angles.

Parameters
----------
arcball : DvzArcball*
    the arcball
app : DvzApp*
    the app
canvas_id : DvzId
    the canvas (or figure) ID
panel : DvzPanel*
    the panel
"""
arcball_gui.argtypes = [
    ctypes.POINTER(DvzArcball),  # DvzArcball* arcball
    ctypes.POINTER(DvzApp),  # DvzApp* app
    DvzId,  # DvzId canvas_id
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
]


# -------------------------------------------------------------------------------------------------
fly = dvz.dvz_fly
fly.__doc__ = """
Create a fly camera controller.

Parameters
----------
flags : int
    the fly camera controller flags

Returns
-------
result : DvzFly*
     the fly camera controller
"""
fly.argtypes = [
    ctypes.c_int,  # int flags
]
fly.restype = ctypes.POINTER(DvzFly)


# -------------------------------------------------------------------------------------------------
fly_reset = dvz.dvz_fly_reset
fly_reset.__doc__ = """
Reset a fly camera to its initial position and orientation.

Parameters
----------
fly : DvzFly*
    the fly camera controller
"""
fly_reset.argtypes = [
    ctypes.POINTER(DvzFly),  # DvzFly* fly
]


# -------------------------------------------------------------------------------------------------
fly_resize = dvz.dvz_fly_resize
fly_resize.__doc__ = """
Inform a fly camera of a panel resize.

Parameters
----------
fly : DvzFly*
    the fly
width : float
    the panel width
height : float
    the panel height
"""
fly_resize.argtypes = [
    ctypes.POINTER(DvzFly),  # DvzFly* fly
    ctypes.c_float,  # float width
    ctypes.c_float,  # float height
]


# -------------------------------------------------------------------------------------------------
fly_initial = dvz.dvz_fly_initial
fly_initial.__doc__ = """
Set the initial position and orientation of a fly camera.

Parameters
----------
fly : DvzFly*
    the fly camera controller
position : Tuple[float, float, float]
    the initial position
yaw : float
    the initial yaw angle (rotation around Y axis)
pitch : float
    the initial pitch angle (rotation around X axis)
roll : float
    the initial roll angle (rotation around Z/view axis)
"""
fly_initial.argtypes = [
    ctypes.POINTER(DvzFly),  # DvzFly* fly
    vec3,  # vec3 position
    ctypes.c_float,  # float yaw
    ctypes.c_float,  # float pitch
    ctypes.c_float,  # float roll
]


# -------------------------------------------------------------------------------------------------
fly_initial_lookat = dvz.dvz_fly_initial_lookat
fly_initial_lookat.__doc__ = """
Set the initial position and orientation of a fly camera.

Parameters
----------
fly : DvzFly*
    the fly camera controller
position : Tuple[float, float, float]
    the initial position
lookat : Tuple[float, float, float]
    the initial lookat position
"""
fly_initial_lookat.argtypes = [
    ctypes.POINTER(DvzFly),  # DvzFly* fly
    vec3,  # vec3 position
    vec3,  # vec3 lookat
]


# -------------------------------------------------------------------------------------------------
fly_move_forward = dvz.dvz_fly_move_forward
fly_move_forward.__doc__ = """
Move the fly camera forward or backward along its view direction.

Parameters
----------
fly : DvzFly*
    the fly camera controller
amount : float
    the movement amount (positive for forward, negative for backward)
"""
fly_move_forward.argtypes = [
    ctypes.POINTER(DvzFly),  # DvzFly* fly
    ctypes.c_float,  # float amount
]


# -------------------------------------------------------------------------------------------------
fly_move_right = dvz.dvz_fly_move_right
fly_move_right.__doc__ = """
Move the fly camera right or left perpendicular to its view direction.

Parameters
----------
fly : DvzFly*
    the fly camera controller
amount : float
    the movement amount (positive for right, negative for left)
"""
fly_move_right.argtypes = [
    ctypes.POINTER(DvzFly),  # DvzFly* fly
    ctypes.c_float,  # float amount
]


# -------------------------------------------------------------------------------------------------
fly_move_up = dvz.dvz_fly_move_up
fly_move_up.__doc__ = """
Move the fly camera up or down along its up vector.

Parameters
----------
fly : DvzFly*
    the fly camera controller
amount : float
    the movement amount (positive for up, negative for down)
"""
fly_move_up.argtypes = [
    ctypes.POINTER(DvzFly),  # DvzFly* fly
    ctypes.c_float,  # float amount
]


# -------------------------------------------------------------------------------------------------
fly_rotate = dvz.dvz_fly_rotate
fly_rotate.__doc__ = """
Rotate the fly camera's view direction (yaw and pitch).

Parameters
----------
fly : DvzFly*
    the fly camera controller
dx : float
    the horizontal rotation amount
dy : float
    the vertical rotation amount
"""
fly_rotate.argtypes = [
    ctypes.POINTER(DvzFly),  # DvzFly* fly
    ctypes.c_float,  # float dx
    ctypes.c_float,  # float dy
]


# -------------------------------------------------------------------------------------------------
fly_roll = dvz.dvz_fly_roll
fly_roll.__doc__ = """
Roll the fly camera around its view direction.

Parameters
----------
fly : DvzFly*
    the fly camera controller
dx : float
    the roll amount
"""
fly_roll.argtypes = [
    ctypes.POINTER(DvzFly),  # DvzFly* fly
    ctypes.c_float,  # float dx
]


# -------------------------------------------------------------------------------------------------
fly_get_position = dvz.dvz_fly_get_position
fly_get_position.__doc__ = """
Get the current position of the fly camera.

Parameters
----------
fly : DvzFly*
    the fly camera controller
out_pos : Out[Tuple[float, float, float]] (out parameter)
    the current position
"""
fly_get_position.argtypes = [
    ctypes.POINTER(DvzFly),  # DvzFly* fly
    vec3,  # out vec3 out_pos
]


# -------------------------------------------------------------------------------------------------
fly_get_lookat = dvz.dvz_fly_get_lookat
fly_get_lookat.__doc__ = """
Get the current lookat point of the fly camera.

Parameters
----------
fly : DvzFly*
    the fly camera controller
out_lookat : Out[Tuple[float, float, float]] (out parameter)
    the current lookat point
"""
fly_get_lookat.argtypes = [
    ctypes.POINTER(DvzFly),  # DvzFly* fly
    vec3,  # out vec3 out_lookat
]


# -------------------------------------------------------------------------------------------------
fly_set_lookat = dvz.dvz_fly_set_lookat
fly_set_lookat.__doc__ = """
Set the lookat point of the fly camera.

Parameters
----------
fly : DvzFly*
    the fly camera controller
lookat : Tuple[float, float, float]
    the lookat point
"""
fly_set_lookat.argtypes = [
    ctypes.POINTER(DvzFly),  # DvzFly* fly
    vec3,  # vec3 lookat
]


# -------------------------------------------------------------------------------------------------
fly_get_up = dvz.dvz_fly_get_up
fly_get_up.__doc__ = """
Get the current up vector of the fly camera.

Parameters
----------
fly : DvzFly*
    the fly camera controller
out_up : Out[Tuple[float, float, float]] (out parameter)
    the current up vector
"""
fly_get_up.argtypes = [
    ctypes.POINTER(DvzFly),  # DvzFly* fly
    vec3,  # out vec3 out_up
]


# -------------------------------------------------------------------------------------------------
fly_mouse = dvz.dvz_fly_mouse
fly_mouse.__doc__ = """
Process a mouse event for the fly camera controller.

Parameters
----------
fly : DvzFly*
    the fly camera controller
ev : DvzMouseEvent*
    the mouse event

Returns
-------
result : bool
     whether the event was handled by the fly camera
"""
fly_mouse.argtypes = [
    ctypes.POINTER(DvzFly),  # DvzFly* fly
    ctypes.POINTER(DvzMouseEvent),  # DvzMouseEvent* ev
]
fly_mouse.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
fly_keyboard = dvz.dvz_fly_keyboard
fly_keyboard.__doc__ = """
Process a keyboard event for the fly camera controller.

Parameters
----------
fly : DvzFly*
    the fly camera controller
ev : DvzKeyboardEvent*
    the keyboard event

Returns
-------
result : bool
     whether the event was handled by the fly camera
"""
fly_keyboard.argtypes = [
    ctypes.POINTER(DvzFly),  # DvzFly* fly
    ctypes.POINTER(DvzKeyboardEvent),  # DvzKeyboardEvent* ev
]
fly_keyboard.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
fly_destroy = dvz.dvz_fly_destroy
fly_destroy.__doc__ = """
Destroy a fly camera controller.

Parameters
----------
fly : DvzFly*
    the fly camera controller
"""
fly_destroy.argtypes = [
    ctypes.POINTER(DvzFly),  # DvzFly* fly
]


# -------------------------------------------------------------------------------------------------
camera_initial = dvz.dvz_camera_initial
camera_initial.__doc__ = """
Set the initial camera parameters.

Parameters
----------
camera : DvzCamera*
    the camera
pos : Tuple[float, float, float]
    the initial position
lookat : Tuple[float, float, float]
    the lookat position
up : Tuple[float, float, float]
    the up vector
"""
camera_initial.argtypes = [
    ctypes.POINTER(DvzCamera),  # DvzCamera* camera
    vec3,  # vec3 pos
    vec3,  # vec3 lookat
    vec3,  # vec3 up
]


# -------------------------------------------------------------------------------------------------
camera_reset = dvz.dvz_camera_reset
camera_reset.__doc__ = """
Reset a camera.

Parameters
----------
camera : DvzCamera*
    the camera
"""
camera_reset.argtypes = [
    ctypes.POINTER(DvzCamera),  # DvzCamera* camera
]


# -------------------------------------------------------------------------------------------------
camera_zrange = dvz.dvz_camera_zrange
camera_zrange.__doc__ = """
Set the camera zrange.

Parameters
----------
camera : DvzCamera*
    the camera
near : float
    the near value
far : float
    the far value
"""
camera_zrange.argtypes = [
    ctypes.POINTER(DvzCamera),  # DvzCamera* camera
    ctypes.c_float,  # float near
    ctypes.c_float,  # float far
]


# -------------------------------------------------------------------------------------------------
camera_ortho = dvz.dvz_camera_ortho
camera_ortho.__doc__ = """
Make an orthographic camera.

Parameters
----------
camera : DvzCamera*
    the camera
left : float
    the left value
right : float
    the right value
bottom : float
    the bottom value
top : float
    the top value
"""
camera_ortho.argtypes = [
    ctypes.POINTER(DvzCamera),  # DvzCamera* camera
    ctypes.c_float,  # float left
    ctypes.c_float,  # float right
    ctypes.c_float,  # float bottom
    ctypes.c_float,  # float top
]


# -------------------------------------------------------------------------------------------------
camera_resize = dvz.dvz_camera_resize
camera_resize.__doc__ = """
Inform a camera of a panel resize.

Parameters
----------
camera : DvzCamera*
    the camera
width : float
    the panel width
height : float
    the panel height
"""
camera_resize.argtypes = [
    ctypes.POINTER(DvzCamera),  # DvzCamera* camera
    ctypes.c_float,  # float width
    ctypes.c_float,  # float height
]


# -------------------------------------------------------------------------------------------------
camera_position = dvz.dvz_camera_position
camera_position.__doc__ = """
Set a camera position.

Parameters
----------
camera : DvzCamera*
    the camera
pos : Tuple[float, float, float]
    the pos
"""
camera_position.argtypes = [
    ctypes.POINTER(DvzCamera),  # DvzCamera* camera
    vec3,  # vec3 pos
]


# -------------------------------------------------------------------------------------------------
camera_get_position = dvz.dvz_camera_get_position
camera_get_position.__doc__ = """
Get the camera position.

Parameters
----------
camera : DvzCamera*
    the camera
pos : Out[Tuple[float, float, float]] (out parameter)
    the pos
"""
camera_get_position.argtypes = [
    ctypes.POINTER(DvzCamera),  # DvzCamera* camera
    vec3,  # out vec3 pos
]


# -------------------------------------------------------------------------------------------------
camera_lookat = dvz.dvz_camera_lookat
camera_lookat.__doc__ = """
Set a camera lookat position.

Parameters
----------
camera : DvzCamera*
    the camera
lookat : Tuple[float, float, float]
    the lookat position
"""
camera_lookat.argtypes = [
    ctypes.POINTER(DvzCamera),  # DvzCamera* camera
    vec3,  # vec3 lookat
]


# -------------------------------------------------------------------------------------------------
camera_get_lookat = dvz.dvz_camera_get_lookat
camera_get_lookat.__doc__ = """
Get the camera lookat position.

Parameters
----------
camera : DvzCamera*
    the camera
lookat : Out[Tuple[float, float, float]] (out parameter)
    the lookat position
"""
camera_get_lookat.argtypes = [
    ctypes.POINTER(DvzCamera),  # DvzCamera* camera
    vec3,  # out vec3 lookat
]


# -------------------------------------------------------------------------------------------------
camera_up = dvz.dvz_camera_up
camera_up.__doc__ = """
Set a camera up vector.

Parameters
----------
camera : DvzCamera*
    the camera
up : Tuple[float, float, float]
    the up vector
"""
camera_up.argtypes = [
    ctypes.POINTER(DvzCamera),  # DvzCamera* camera
    vec3,  # vec3 up
]


# -------------------------------------------------------------------------------------------------
camera_get_up = dvz.dvz_camera_get_up
camera_get_up.__doc__ = """
Get the camera up vector.

Parameters
----------
camera : DvzCamera*
    the camera
up : Out[Tuple[float, float, float]] (out parameter)
    the up vector
"""
camera_get_up.argtypes = [
    ctypes.POINTER(DvzCamera),  # DvzCamera* camera
    vec3,  # out vec3 up
]


# -------------------------------------------------------------------------------------------------
camera_perspective = dvz.dvz_camera_perspective
camera_perspective.__doc__ = """
Set a camera perspective.

Parameters
----------
camera : DvzCamera*
    the camera
fov : float
    the field of view angle (in radians)
"""
camera_perspective.argtypes = [
    ctypes.POINTER(DvzCamera),  # DvzCamera* camera
    ctypes.c_float,  # float fov
]


# -------------------------------------------------------------------------------------------------
camera_viewproj = dvz.dvz_camera_viewproj
camera_viewproj.__doc__ = """
Return the view and proj matrices of the camera.

Parameters
----------
camera : DvzCamera*
    the camera
view : Out[mat4] (out parameter)
    the view matrix
proj : Out[mat4] (out parameter)
    the proj matrix
"""
camera_viewproj.argtypes = [
    ctypes.POINTER(DvzCamera),  # DvzCamera* camera
    mat4,  # out mat4 view
    mat4,  # out mat4 proj
]


# -------------------------------------------------------------------------------------------------
camera_mvp = dvz.dvz_camera_mvp
camera_mvp.__doc__ = """
Apply an MVP to a camera.

Parameters
----------
camera : DvzCamera*
    the camera
mvp : DvzMVP*
    the MVP
"""
camera_mvp.argtypes = [
    ctypes.POINTER(DvzCamera),  # DvzCamera* camera
    ctypes.POINTER(DvzMVP),  # DvzMVP* mvp
]


# -------------------------------------------------------------------------------------------------
camera_print = dvz.dvz_camera_print
camera_print.__doc__ = """
Display information about a camera.

Parameters
----------
camera : DvzCamera*
    the camera
"""
camera_print.argtypes = [
    ctypes.POINTER(DvzCamera),  # DvzCamera* camera
]


# -------------------------------------------------------------------------------------------------
panzoom = dvz.dvz_panzoom
panzoom.__doc__ = """
Create a panzoom object (usually you'd rather use `dvz_panel_panzoom()`).

Parameters
----------
width : float
    the panel width
height : float
    the panel height
flags : int
    the panzoom creation flags

Returns
-------
result : DvzPanzoom*
     the Panzoom object
"""
panzoom.argtypes = [
    ctypes.c_float,  # float width
    ctypes.c_float,  # float height
    ctypes.c_int,  # int flags
]
panzoom.restype = ctypes.POINTER(DvzPanzoom)


# -------------------------------------------------------------------------------------------------
panzoom_reset = dvz.dvz_panzoom_reset
panzoom_reset.__doc__ = """
Reset a panzoom.

Parameters
----------
pz : DvzPanzoom*
    the panzoom
"""
panzoom_reset.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
]


# -------------------------------------------------------------------------------------------------
panzoom_resize = dvz.dvz_panzoom_resize
panzoom_resize.__doc__ = """
Inform a panzoom of a panel resize.

Parameters
----------
pz : DvzPanzoom*
    the panzoom
width : float
    the panel width
height : float
    the panel height
"""
panzoom_resize.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
    ctypes.c_float,  # float width
    ctypes.c_float,  # float height
]


# -------------------------------------------------------------------------------------------------
panzoom_flags = dvz.dvz_panzoom_flags
panzoom_flags.__doc__ = """
Set the panzoom flags.

Parameters
----------
pz : DvzPanzoom*
    the panzoom
flags : int
    the flags
"""
panzoom_flags.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
panzoom_pan = dvz.dvz_panzoom_pan
panzoom_pan.__doc__ = """
Apply a pan value to a panzoom.

Parameters
----------
pz : DvzPanzoom*
    the panzoom
pan : Tuple[float, float]
    the pan, in NDC
"""
panzoom_pan.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
    vec2,  # vec2 pan
]


# -------------------------------------------------------------------------------------------------
panzoom_zoom = dvz.dvz_panzoom_zoom
panzoom_zoom.__doc__ = """
Apply a zoom value to a panzoom.

Parameters
----------
pz : DvzPanzoom*
    the panzoom
zoom : Tuple[float, float]
    the zoom, in NDC
"""
panzoom_zoom.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
    vec2,  # vec2 zoom
]


# -------------------------------------------------------------------------------------------------
panzoom_pan_shift = dvz.dvz_panzoom_pan_shift
panzoom_pan_shift.__doc__ = """
Apply a pan shift to a panzoom.

Parameters
----------
pz : DvzPanzoom*
    the panzoom
shift_px : Tuple[float, float]
    the shift value, in pixels
center_px : Tuple[float, float]
    the center position, in pixels
"""
panzoom_pan_shift.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
    vec2,  # vec2 shift_px
    vec2,  # vec2 center_px
]


# -------------------------------------------------------------------------------------------------
panzoom_zoom_shift = dvz.dvz_panzoom_zoom_shift
panzoom_zoom_shift.__doc__ = """
Apply a zoom shift to a panzoom.

Parameters
----------
pz : DvzPanzoom*
    the panzoom
shift_px : Tuple[float, float]
    the shift value, in pixels
center_px : Tuple[float, float]
    the center position, in pixels
"""
panzoom_zoom_shift.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
    vec2,  # vec2 shift_px
    vec2,  # vec2 center_px
]


# -------------------------------------------------------------------------------------------------
panzoom_end = dvz.dvz_panzoom_end
panzoom_end.__doc__ = """
End a panzoom interaction.

Parameters
----------
pz : DvzPanzoom*
    the panzoom
"""
panzoom_end.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
]


# -------------------------------------------------------------------------------------------------
panzoom_zoom_wheel = dvz.dvz_panzoom_zoom_wheel
panzoom_zoom_wheel.__doc__ = """
Apply a wheel zoom to a panzoom.

Parameters
----------
pz : DvzPanzoom*
    the panzoom
dir : Tuple[float, float]
    the wheel direction
center_px : Tuple[float, float]
    the center position, in pixels
"""
panzoom_zoom_wheel.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
    vec2,  # vec2 dir
    vec2,  # vec2 center_px
]


# -------------------------------------------------------------------------------------------------
panzoom_level = dvz.dvz_panzoom_level
panzoom_level.__doc__ = """
Get the current zoom level.

Parameters
----------
pz : DvzPanzoom*
    the panzoom
dim : DvzDim
    the dimension

Returns
-------
result : float
"""
panzoom_level.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
    DvzDim,  # DvzDim dim
]
panzoom_level.restype = ctypes.c_float


# -------------------------------------------------------------------------------------------------
panzoom_extent = dvz.dvz_panzoom_extent
panzoom_extent.__doc__ = """
Get the extent box.

Parameters
----------
pz : DvzPanzoom*
    the panzoom
extent : Out[DvzBox] (out parameter)
    the extent box in normalized coordinates
"""
panzoom_extent.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
    Out,  # out DvzBox* extent
]


# -------------------------------------------------------------------------------------------------
panzoom_set = dvz.dvz_panzoom_set
panzoom_set.__doc__ = """
Set the extent box.

Parameters
----------
pz : DvzPanzoom*
    the panzoom
extent : DvzBox*
    the extent box
"""
panzoom_set.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
    ctypes.POINTER(DvzBox),  # DvzBox* extent
]


# -------------------------------------------------------------------------------------------------
panzoom_mvp = dvz.dvz_panzoom_mvp
panzoom_mvp.__doc__ = """
Apply an MVP matrix to a panzoom.

Parameters
----------
pz : DvzPanzoom*
    the panzoom
mvp : DvzMVP*
    the MVP
"""
panzoom_mvp.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
    ctypes.POINTER(DvzMVP),  # DvzMVP* mvp
]


# -------------------------------------------------------------------------------------------------
panzoom_bounds = dvz.dvz_panzoom_bounds
panzoom_bounds.__doc__ = """
Get x-y bounds.

Parameters
----------
pz : DvzPanzoom*
    the panzoom
ref : DvzRef*
    the ref
xmin : Out[float] (out parameter)
    xmin
xmax : Out[float] (out parameter)
    xmax
ymin : Out[float] (out parameter)
    ymin
ymax : Out[float] (out parameter)
    ymax
"""
panzoom_bounds.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
    ctypes.POINTER(DvzRef),  # DvzRef* ref
    Out,  # out double* xmin
    Out,  # out double* xmax
    Out,  # out double* ymin
    Out,  # out double* ymax
]


# -------------------------------------------------------------------------------------------------
panzoom_xlim = dvz.dvz_panzoom_xlim
panzoom_xlim.__doc__ = """
Set x bounds.

Parameters
----------
pz : DvzPanzoom*
    the panzoom
ref : DvzRef*
    the ref
xmin : float
    xmin
xmax : float
    xmax
"""
panzoom_xlim.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
    ctypes.POINTER(DvzRef),  # DvzRef* ref
    ctypes.c_double,  # double xmin
    ctypes.c_double,  # double xmax
]


# -------------------------------------------------------------------------------------------------
panzoom_ylim = dvz.dvz_panzoom_ylim
panzoom_ylim.__doc__ = """
Set y bounds.

Parameters
----------
pz : DvzPanzoom*
    the panzoom
ref : DvzRef*
    the ref
ymin : float
    ymin
ymax : float
    ymax
"""
panzoom_ylim.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
    ctypes.POINTER(DvzRef),  # DvzRef* ref
    ctypes.c_double,  # double ymin
    ctypes.c_double,  # double ymax
]


# -------------------------------------------------------------------------------------------------
panzoom_mouse = dvz.dvz_panzoom_mouse
panzoom_mouse.__doc__ = """
Register a mouse event to a panzoom.

Parameters
----------
pz : DvzPanzoom*
    the panzoom
ev : DvzMouseEvent*
    the mouse event

Returns
-------
result : bool
     whether the panzoom is affected by the mouse event
"""
panzoom_mouse.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
    ctypes.POINTER(DvzMouseEvent),  # DvzMouseEvent* ev
]
panzoom_mouse.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
panzoom_destroy = dvz.dvz_panzoom_destroy
panzoom_destroy.__doc__ = """
Destroy a panzoom.

Parameters
----------
pz : DvzPanzoom*
    the pz
"""
panzoom_destroy.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
]


# -------------------------------------------------------------------------------------------------
ortho_reset = dvz.dvz_ortho_reset
ortho_reset.__doc__ = """
Reset an ortho.

Parameters
----------
ortho : DvzOrtho*
    the ortho
"""
ortho_reset.argtypes = [
    ctypes.POINTER(DvzOrtho),  # DvzOrtho* ortho
]


# -------------------------------------------------------------------------------------------------
ortho_resize = dvz.dvz_ortho_resize
ortho_resize.__doc__ = """
Inform an ortho of a panel resize.

Parameters
----------
ortho : DvzOrtho*
    the ortho
width : float
    the panel width
height : float
    the panel height
"""
ortho_resize.argtypes = [
    ctypes.POINTER(DvzOrtho),  # DvzOrtho* ortho
    ctypes.c_float,  # float width
    ctypes.c_float,  # float height
]


# -------------------------------------------------------------------------------------------------
ortho_flags = dvz.dvz_ortho_flags
ortho_flags.__doc__ = """
Set the ortho flags.

Parameters
----------
ortho : DvzOrtho*
    the ortho
flags : int
    the flags
"""
ortho_flags.argtypes = [
    ctypes.POINTER(DvzOrtho),  # DvzOrtho* ortho
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
ortho_pan = dvz.dvz_ortho_pan
ortho_pan.__doc__ = """
Apply a pan value to an ortho.

Parameters
----------
ortho : DvzOrtho*
    the ortho
pan : Tuple[float, float]
    the pan, in NDC
"""
ortho_pan.argtypes = [
    ctypes.POINTER(DvzOrtho),  # DvzOrtho* ortho
    vec2,  # vec2 pan
]


# -------------------------------------------------------------------------------------------------
ortho_zoom = dvz.dvz_ortho_zoom
ortho_zoom.__doc__ = """
Apply a zoom value to an ortho.

Parameters
----------
ortho : DvzOrtho*
    the ortho
zoom : float
    the zoom level
"""
ortho_zoom.argtypes = [
    ctypes.POINTER(DvzOrtho),  # DvzOrtho* ortho
    ctypes.c_float,  # float zoom
]


# -------------------------------------------------------------------------------------------------
ortho_pan_shift = dvz.dvz_ortho_pan_shift
ortho_pan_shift.__doc__ = """
Apply a pan shift to an ortho.

Parameters
----------
ortho : DvzOrtho*
    the ortho
shift_px : Tuple[float, float]
    the shift value, in pixels
center_px : Tuple[float, float]
    the center position, in pixels
"""
ortho_pan_shift.argtypes = [
    ctypes.POINTER(DvzOrtho),  # DvzOrtho* ortho
    vec2,  # vec2 shift_px
    vec2,  # vec2 center_px
]


# -------------------------------------------------------------------------------------------------
ortho_zoom_shift = dvz.dvz_ortho_zoom_shift
ortho_zoom_shift.__doc__ = """
Apply a zoom shift to an ortho.

Parameters
----------
ortho : DvzOrtho*
    the ortho
shift_px : Tuple[float, float]
    the shift value, in pixels
center_px : Tuple[float, float]
    the center position, in pixels
"""
ortho_zoom_shift.argtypes = [
    ctypes.POINTER(DvzOrtho),  # DvzOrtho* ortho
    vec2,  # vec2 shift_px
    vec2,  # vec2 center_px
]


# -------------------------------------------------------------------------------------------------
ortho_end = dvz.dvz_ortho_end
ortho_end.__doc__ = """
End an ortho interaction.

Parameters
----------
ortho : DvzOrtho*
    the ortho
"""
ortho_end.argtypes = [
    ctypes.POINTER(DvzOrtho),  # DvzOrtho* ortho
]


# -------------------------------------------------------------------------------------------------
ortho_zoom_wheel = dvz.dvz_ortho_zoom_wheel
ortho_zoom_wheel.__doc__ = """
Apply a wheel zoom to an ortho.

Parameters
----------
ortho : DvzOrtho*
    the ortho
dir : Tuple[float, float]
    the wheel direction
center_px : Tuple[float, float]
    the center position, in pixels
"""
ortho_zoom_wheel.argtypes = [
    ctypes.POINTER(DvzOrtho),  # DvzOrtho* ortho
    vec2,  # vec2 dir
    vec2,  # vec2 center_px
]


# -------------------------------------------------------------------------------------------------
ortho_mvp = dvz.dvz_ortho_mvp
ortho_mvp.__doc__ = """
Apply an MVP matrix to an ortho.

Parameters
----------
ortho : DvzOrtho*
    the ortho
mvp : DvzMVP*
    the MVP
"""
ortho_mvp.argtypes = [
    ctypes.POINTER(DvzOrtho),  # DvzOrtho* ortho
    ctypes.POINTER(DvzMVP),  # DvzMVP* mvp
]


# -------------------------------------------------------------------------------------------------
ref = dvz.dvz_ref
ref.__doc__ = """
Create a reference frame (wrapping a 3D box representing the data in its original coordinates).

Parameters
----------
flags : int
    the flags

Returns
-------
result : DvzRef*
     the reference frame
"""
ref.argtypes = [
    ctypes.c_int,  # int flags
]
ref.restype = ctypes.POINTER(DvzRef)


# -------------------------------------------------------------------------------------------------
ref_is_set = dvz.dvz_ref_is_set
ref_is_set.__doc__ = """
Indicate whether the reference is set on a given axis.

Parameters
----------
ref : DvzRef*
    the reference frame
dim : DvzDim
    the dimension axis

Returns
-------
result : bool
     whether the ref is set on this axis.
"""
ref_is_set.argtypes = [
    ctypes.POINTER(DvzRef),  # DvzRef* ref
    DvzDim,  # DvzDim dim
]
ref_is_set.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
ref_set = dvz.dvz_ref_set
ref_set.__doc__ = """
Set the range on a given axis.

Parameters
----------
ref : DvzRef*
    the reference frame
dim : DvzDim
    the dimension axis
vmin : float
    the minimum value
vmax : float
    the maximum value
"""
ref_set.argtypes = [
    ctypes.POINTER(DvzRef),  # DvzRef* ref
    DvzDim,  # DvzDim dim
    ctypes.c_double,  # double vmin
    ctypes.c_double,  # double vmax
]


# -------------------------------------------------------------------------------------------------
ref_get = dvz.dvz_ref_get
ref_get.__doc__ = """
Get the range on a given axis.

Parameters
----------
ref : DvzRef*
    the reference frame
dim : DvzDim
    the dimension axis
vmin : Out[float] (out parameter)
    the minimum value
vmax : Out[float] (out parameter)
    the maximum value
"""
ref_get.argtypes = [
    ctypes.POINTER(DvzRef),  # DvzRef* ref
    DvzDim,  # DvzDim dim
    Out,  # out double* vmin
    Out,  # out double* vmax
]


# -------------------------------------------------------------------------------------------------
ref_expand = dvz.dvz_ref_expand
ref_expand.__doc__ = """
Expand the reference by ensuring it contains the specified range.

Parameters
----------
ref : DvzRef*
    the reference frame
dim : DvzDim
    the dimension axis
vmin : float
    the minimum value
vmax : float
    the maximum value
"""
ref_expand.argtypes = [
    ctypes.POINTER(DvzRef),  # DvzRef* ref
    DvzDim,  # DvzDim dim
    ctypes.c_double,  # double vmin
    ctypes.c_double,  # double vmax
]


# -------------------------------------------------------------------------------------------------
ref_expand_2D = dvz.dvz_ref_expand_2D
ref_expand_2D.__doc__ = """
Expand the reference by ensuring it contains the specified 2D data.

Parameters
----------
ref : DvzRef*
    the reference frame
count : int
    the number of positions
pos : np.ndarray[dvec2]
    the 2D positions
"""
ref_expand_2D.argtypes = [
    ctypes.POINTER(DvzRef),  # DvzRef* ref
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.double, ndim=2, ncol=2, flags="C_CONTIGUOUS"),  # dvec2* pos
]


# -------------------------------------------------------------------------------------------------
ref_expand_3D = dvz.dvz_ref_expand_3D
ref_expand_3D.__doc__ = """
Expand the reference by ensuring it contains the specified 3D data.

Parameters
----------
ref : DvzRef*
    the reference frame
count : int
    the number of positions
pos : np.ndarray[dvec3]
    the 3D positions
"""
ref_expand_3D.argtypes = [
    ctypes.POINTER(DvzRef),  # DvzRef* ref
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.double, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # dvec3* pos
]


# -------------------------------------------------------------------------------------------------
ref_normalize_1D = dvz.dvz_ref_normalize_1D
ref_normalize_1D.__doc__ = """
Transform 1D data from the reference frame to normalized device coordinates [-1..+1].

Parameters
----------
ref : DvzRef*
    the reference frame
dim : DvzDim
    which dimension
count : int
    the number of positions
pos : np.ndarray[double]
    the 1D positions
pos_tr : Out[Tuple[float, float, float]] (out parameter)
    (array) the transformed positions
"""
ref_normalize_1D.argtypes = [
    ctypes.POINTER(DvzRef),  # DvzRef* ref
    DvzDim,  # DvzDim dim
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.double, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # double* pos
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # out vec3* pos_tr
]


# -------------------------------------------------------------------------------------------------
ref_normalize_2D = dvz.dvz_ref_normalize_2D
ref_normalize_2D.__doc__ = """
Transform 2D data from the reference frame to normalized device coordinates [-1..+1].

Parameters
----------
ref : DvzRef*
    the reference frame
count : int
    the number of positions
pos : np.ndarray[dvec2]
    the 2D positions
pos_tr : Out[Tuple[float, float, float]] (out parameter)
    (array) the transformed 3D positions
"""
ref_normalize_2D.argtypes = [
    ctypes.POINTER(DvzRef),  # DvzRef* ref
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.double, ndim=2, ncol=2, flags="C_CONTIGUOUS"),  # dvec2* pos
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # out vec3* pos_tr
]


# -------------------------------------------------------------------------------------------------
ref_normalize_polygon = dvz.dvz_ref_normalize_polygon
ref_normalize_polygon.__doc__ = """
Transform 2D data from the reference frame to normalized device coordinates [-1..+1] in 2D.

Parameters
----------
ref : DvzRef*
    the reference frame
count : int
    the number of positions
pos : np.ndarray[dvec2]
    the 2D positions
pos_tr : Out[dvec2] (out parameter)
    (array) the transformed 2D positions
"""
ref_normalize_polygon.argtypes = [
    ctypes.POINTER(DvzRef),  # DvzRef* ref
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.double, ndim=2, ncol=2, flags="C_CONTIGUOUS"),  # dvec2* pos
    ndpointer(dtype=np.double, ndim=2, ncol=2, flags="C_CONTIGUOUS"),  # out dvec2* pos_tr
]


# -------------------------------------------------------------------------------------------------
ref_normalize_3D = dvz.dvz_ref_normalize_3D
ref_normalize_3D.__doc__ = """
Transform 3D data from the reference frame to normalized device coordinates [-1..+1].

Parameters
----------
ref : DvzRef*
    the reference frame
count : int
    the number of positions
pos : np.ndarray[dvec3]
    the 3D positions
pos_tr : Out[Tuple[float, float, float]] (out parameter)
    (array) the transformed positions
"""
ref_normalize_3D.argtypes = [
    ctypes.POINTER(DvzRef),  # DvzRef* ref
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.double, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # dvec3* pos
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # out vec3* pos_tr
]


# -------------------------------------------------------------------------------------------------
ref_inverse = dvz.dvz_ref_inverse
ref_inverse.__doc__ = """
Inverse transform from normalized device coordinates [-1..+1] to the reference frame.

Parameters
----------
ref : DvzRef*
    the reference frame
pos_tr : Tuple[float, float, float]
    the 3D position in normalized device coordinates
pos : Out[dvec3] (out parameter)
    the original position
"""
ref_inverse.argtypes = [
    ctypes.POINTER(DvzRef),  # DvzRef* ref
    vec3,  # vec3 pos_tr
    Out,  # out dvec3* pos
]


# -------------------------------------------------------------------------------------------------
ref_destroy = dvz.dvz_ref_destroy
ref_destroy.__doc__ = """
Destroy a reference frame.

Parameters
----------
ref : DvzRef*
    the reference frame
"""
ref_destroy.argtypes = [
    ctypes.POINTER(DvzRef),  # DvzRef* ref
]


# -------------------------------------------------------------------------------------------------
colorbar = dvz.dvz_colorbar
colorbar.__doc__ = """
Create a colorbar.

Parameters
----------
batch : DvzBatch*
    the batch
cmap : DvzColormap
    the colormap
dmin : float
    the minimal value
dmax : float
    the maximal value
flags : int
    the flags

Returns
-------
result : DvzColorbar*
     the colorbar
"""
colorbar.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzColormap,  # DvzColormap cmap
    ctypes.c_double,  # double dmin
    ctypes.c_double,  # double dmax
    ctypes.c_int,  # int flags
]
colorbar.restype = ctypes.POINTER(DvzColorbar)


# -------------------------------------------------------------------------------------------------
colorbar_range = dvz.dvz_colorbar_range
colorbar_range.__doc__ = """
Set the colorbar range.

Parameters
----------
colorbar : DvzColorbar*
    the colorbar
dmin : float
    the minimal value
dmax : float
    the maximal value
"""
colorbar_range.argtypes = [
    ctypes.POINTER(DvzColorbar),  # DvzColorbar* colorbar
    ctypes.c_double,  # double dmin
    ctypes.c_double,  # double dmax
]


# -------------------------------------------------------------------------------------------------
colorbar_cmap = dvz.dvz_colorbar_cmap
colorbar_cmap.__doc__ = """
Set the colormap of a colorbar.

Parameters
----------
colorbar : DvzColorbar*
    the colorbar
cmap : DvzColormap
    the colormap
"""
colorbar_cmap.argtypes = [
    ctypes.POINTER(DvzColorbar),  # DvzColorbar* colorbar
    DvzColormap,  # DvzColormap cmap
]


# -------------------------------------------------------------------------------------------------
colorbar_position = dvz.dvz_colorbar_position
colorbar_position.__doc__ = """
Set the position of a colorbar.

Parameters
----------
colorbar : DvzColorbar*
    the colorbar
position : Tuple[float, float]
    the 2D position in NDC
"""
colorbar_position.argtypes = [
    ctypes.POINTER(DvzColorbar),  # DvzColorbar* colorbar
    vec2,  # vec2 position
]


# -------------------------------------------------------------------------------------------------
colorbar_size = dvz.dvz_colorbar_size
colorbar_size.__doc__ = """
Set the size of a colorbar

Parameters
----------
colorbar : DvzColorbar*
    the colorbar
size : Tuple[float, float]
    the colorbar size in pixels
"""
colorbar_size.argtypes = [
    ctypes.POINTER(DvzColorbar),  # DvzColorbar* colorbar
    vec2,  # vec2 size
]


# -------------------------------------------------------------------------------------------------
colorbar_anchor = dvz.dvz_colorbar_anchor
colorbar_anchor.__doc__ = """
Set the anchor of a colorbar

Parameters
----------
colorbar : DvzColorbar*
    the colorbar
anchor : Tuple[float, float]
    the colorbar anchor
"""
colorbar_anchor.argtypes = [
    ctypes.POINTER(DvzColorbar),  # DvzColorbar* colorbar
    vec2,  # vec2 anchor
]


# -------------------------------------------------------------------------------------------------
colorbar_panel = dvz.dvz_colorbar_panel
colorbar_panel.__doc__ = """
Add a colorbar to a panel.

Parameters
----------
colorbar : DvzColorbar*
    the colorbar
panel : DvzPanel*
    the panel
"""
colorbar_panel.argtypes = [
    ctypes.POINTER(DvzColorbar),  # DvzColorbar* colorbar
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
]


# -------------------------------------------------------------------------------------------------
colorbar_update = dvz.dvz_colorbar_update
colorbar_update.__doc__ = """
Update a colorbar.

Parameters
----------
colorbar : DvzColorbar*
    the colorbar
"""
colorbar_update.argtypes = [
    ctypes.POINTER(DvzColorbar),  # DvzColorbar* colorbar
]


# -------------------------------------------------------------------------------------------------
colorbar_destroy = dvz.dvz_colorbar_destroy
colorbar_destroy.__doc__ = """
Destroy a colorbar.

Parameters
----------
colorbar : DvzColorbar*
    the colorbar
"""
colorbar_destroy.argtypes = [
    ctypes.POINTER(DvzColorbar),  # DvzColorbar* colorbar
]


# -------------------------------------------------------------------------------------------------
app = dvz.dvz_app
app.__doc__ = """
Create an app.

Parameters
----------
flags : int
    the app creation flags

Returns
-------
result : DvzApp*
     the app
"""
app.argtypes = [
    ctypes.c_int,  # int flags
]
app.restype = ctypes.POINTER(DvzApp)


# -------------------------------------------------------------------------------------------------
app_batch = dvz.dvz_app_batch
app_batch.__doc__ = """
Return the app batch.

Parameters
----------
app : DvzApp*
    the app

Returns
-------
result : DvzBatch*
     the batch
"""
app_batch.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
]
app_batch.restype = ctypes.POINTER(DvzBatch)


# -------------------------------------------------------------------------------------------------
app_frame = dvz.dvz_app_frame
app_frame.__doc__ = """
Run one frame.

Parameters
----------
app : DvzApp*
    the app
"""
app_frame.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
]


# -------------------------------------------------------------------------------------------------
app_on_frame = dvz.dvz_app_on_frame
app_on_frame.__doc__ = """
Register a frame callback.

Parameters
----------
app : DvzApp*
    the app
callback : DvzAppFrameCallback
    the callback
user_data : np.ndarray
    the user data
"""
app_on_frame.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
    DvzAppFrameCallback,  # DvzAppFrameCallback callback
    ctypes.c_void_p,  # void* user_data
]


# -------------------------------------------------------------------------------------------------
app_on_mouse = dvz.dvz_app_on_mouse
app_on_mouse.__doc__ = """
Register a mouse callback.

Parameters
----------
app : DvzApp*
    the app
callback : DvzAppMouseCallback
    the callback
user_data : np.ndarray
    the user data
"""
app_on_mouse.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
    DvzAppMouseCallback,  # DvzAppMouseCallback callback
    ctypes.c_void_p,  # void* user_data
]


# -------------------------------------------------------------------------------------------------
app_on_keyboard = dvz.dvz_app_on_keyboard
app_on_keyboard.__doc__ = """
Register a keyboard callback.

Parameters
----------
app : DvzApp*
    the app
callback : DvzAppKeyboardCallback
    the callback
user_data : np.ndarray
    the user data
"""
app_on_keyboard.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
    DvzAppKeyboardCallback,  # DvzAppKeyboardCallback callback
    ctypes.c_void_p,  # void* user_data
]


# -------------------------------------------------------------------------------------------------
app_on_resize = dvz.dvz_app_on_resize
app_on_resize.__doc__ = """
Register a resize callback.

Parameters
----------
app : DvzApp*
    the app
callback : DvzAppResizeCallback
    the callback
user_data : np.ndarray
    the user data
"""
app_on_resize.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
    DvzAppResizeCallback,  # DvzAppResizeCallback callback
    ctypes.c_void_p,  # void* user_data
]


# -------------------------------------------------------------------------------------------------
app_timer = dvz.dvz_app_timer
app_timer.__doc__ = """
Create a timer.

Parameters
----------
app : DvzApp*
    the app
delay : float
    the delay, in seconds, until the first event
period : float
    the period, in seconds, between two events
max_count : int
    the maximum number of events

Returns
-------
result : DvzTimerItem*
     the timer
"""
app_timer.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
    ctypes.c_double,  # double delay
    ctypes.c_double,  # double period
    ctypes.c_uint64,  # uint64_t max_count
]
app_timer.restype = ctypes.POINTER(DvzTimerItem)


# -------------------------------------------------------------------------------------------------
app_timer_clear = dvz.dvz_app_timer_clear
app_timer_clear.__doc__ = """
Stop and remove all timers.

Parameters
----------
app : DvzApp*
    the app
"""
app_timer_clear.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
]


# -------------------------------------------------------------------------------------------------
app_on_timer = dvz.dvz_app_on_timer
app_on_timer.__doc__ = """
Register a timer callback.

Parameters
----------
app : DvzApp*
    the app
callback : DvzAppTimerCallback
    the timer callback
user_data : np.ndarray
    the user data
"""
app_on_timer.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
    DvzAppTimerCallback,  # DvzAppTimerCallback callback
    ctypes.c_void_p,  # void* user_data
]


# -------------------------------------------------------------------------------------------------
app_gui = dvz.dvz_app_gui
app_gui.__doc__ = """
Register a GUI callback.

Parameters
----------
app : DvzApp*
    the app
canvas_id : DvzId
    the canvas ID
callback : DvzAppGuiCallback
    the GUI callback
user_data : np.ndarray
    the user data
"""
app_gui.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
    DvzId,  # DvzId canvas_id
    DvzAppGuiCallback,  # DvzAppGuiCallback callback
    ctypes.c_void_p,  # void* user_data
]


# -------------------------------------------------------------------------------------------------
app_run = dvz.dvz_app_run
app_run.__doc__ = """
Start the application event loop.

Parameters
----------
app : DvzApp*
    the app
frame_count : int
    the maximum number of frames, 0 for infinite loop
"""
app_run.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
    ctypes.c_uint64,  # uint64_t frame_count
]


# -------------------------------------------------------------------------------------------------
app_submit = dvz.dvz_app_submit
app_submit.__doc__ = """
Submit the current batch to the application.

Parameters
----------
app : DvzApp*
    the app
"""
app_submit.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
]


# -------------------------------------------------------------------------------------------------
app_screenshot = dvz.dvz_app_screenshot
app_screenshot.__doc__ = """
Make a screenshot of a canvas.

Parameters
----------
app : DvzApp*
    the app
canvas_id : DvzId
    the ID of the canvas
filename : str
    the path to the PNG file with the screenshot
"""
app_screenshot.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
    DvzId,  # DvzId canvas_id
    CStringBuffer,  # char* filename
]


# -------------------------------------------------------------------------------------------------
app_timestamps = dvz.dvz_app_timestamps
app_timestamps.__doc__ = """
Return the precise display timestamps of the last `count` frames.

Parameters
----------
app : DvzApp*
    the app
canvas_id : DvzId
    the ID of the canvas
count : int
    number of frames
seconds : Out[int] (out parameter)
    (array) a buffer holding at least `count` uint64_t values (seconds)
nanoseconds : Out[int] (out parameter)
    (array) a buffer holding at least `count` uint64_t values (nanoseconds)
"""
app_timestamps.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
    DvzId,  # DvzId canvas_id
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint64, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # out uint64_t* seconds
    ndpointer(dtype=np.uint64, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # out uint64_t* nanoseconds
]


# -------------------------------------------------------------------------------------------------
app_wait = dvz.dvz_app_wait
app_wait.__doc__ = """
Wait until the GPU has finished processing.

Parameters
----------
app : DvzApp*
    the app
"""
app_wait.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
]


# -------------------------------------------------------------------------------------------------
app_stop = dvz.dvz_app_stop
app_stop.__doc__ = """
Stop the app's client.

Parameters
----------
app : DvzApp*
    the app
"""
app_stop.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
]


# -------------------------------------------------------------------------------------------------
app_destroy = dvz.dvz_app_destroy
app_destroy.__doc__ = """
Destroy the app.

Parameters
----------
app : DvzApp*
    the app
"""
app_destroy.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
]


# -------------------------------------------------------------------------------------------------
time = dvz.dvz_time
time.__doc__ = """
Get the current time.

Parameters
----------
time : Out[DvzTime] (out parameter)
    fill a structure with seconds and nanoseconds integers
"""
time.argtypes = [
    Out,  # out DvzTime* time
]


# -------------------------------------------------------------------------------------------------
time_print = dvz.dvz_time_print
time_print.__doc__ = """
Display a time.

Parameters
----------
time : DvzTime*
    a time structure
"""
time_print.argtypes = [
    ctypes.POINTER(DvzTime),  # DvzTime* time
]


# -------------------------------------------------------------------------------------------------
app_mouse = dvz.dvz_app_mouse
app_mouse.__doc__ = """
Return the last mouse position and pressed button.

Parameters
----------
app : DvzApp*
    the app
canvas_id : DvzId
    the canvas id
x : Out[float] (out parameter)
    a pointer to the mouse x position
y : Out[float] (out parameter)
    a pointer to the mouse y position
button : Out[DvzMouseButton] (out parameter)
    a pointer to the pressed button
"""
app_mouse.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
    DvzId,  # DvzId canvas_id
    Out,  # out double* x
    Out,  # out double* y
    Out,  # out DvzMouseButton* button
]


# -------------------------------------------------------------------------------------------------
app_keyboard = dvz.dvz_app_keyboard
app_keyboard.__doc__ = """
Return the last keyboard key pressed.

Parameters
----------
app : DvzApp*
    the app
canvas_id : DvzId
    the canvas id
key : Out[DvzKeyCode] (out parameter)
    a pointer to the last pressed key
"""
app_keyboard.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
    DvzId,  # DvzId canvas_id
    Out,  # out DvzKeyCode* key
]


# -------------------------------------------------------------------------------------------------
free = dvz.dvz_free
free.__doc__ = """
Free a pointer.

Parameters
----------
pointer : np.ndarray
    a pointer
"""
free.argtypes = [
    ctypes.c_void_p,  # void* pointer
]


# -------------------------------------------------------------------------------------------------
external_vertex = dvz.dvz_external_vertex
external_vertex.__doc__ = """
Get an external memory handle of a vertex dat.

Parameters
----------
rd : DvzRenderer*
    the renderer
visual : DvzVisual*
    the visual
binding_idx : int
    the binding index of the dat that is being used as vertex buffer
offset : Out[DvzSize] (out parameter)
    the offset, in bytes, of the dat, within the buffer containing that dat

Returns
-------
result : int
     the external memory handle of that buffer
"""
external_vertex.argtypes = [
    ctypes.POINTER(DvzRenderer),  # DvzRenderer* rd
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t binding_idx
    Out,  # out DvzSize* offset
]
external_vertex.restype = ctypes.c_int


# -------------------------------------------------------------------------------------------------
external_index = dvz.dvz_external_index
external_index.__doc__ = """
Get an external memory handle of an index dat.

Parameters
----------
rd : DvzRenderer*
    the renderer
visual : DvzVisual*
    the visual
offset : Out[DvzSize] (out parameter)
    the offset, in bytes, of the dat, within the buffer containing that dat

Returns
-------
result : int
     the external memory handle of that buffer
"""
external_index.argtypes = [
    ctypes.POINTER(DvzRenderer),  # DvzRenderer* rd
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    Out,  # out DvzSize* offset
]
external_index.restype = ctypes.c_int


# -------------------------------------------------------------------------------------------------
external_dat = dvz.dvz_external_dat
external_dat.__doc__ = """
Get an external memory handle of a dat.

Parameters
----------
rd : DvzRenderer*
    the renderer
visual : DvzVisual*
    the visual
slot_idx : int
    the slot index of the dat
offset : Out[DvzSize] (out parameter)
    the offset, in bytes, of the dat, within the buffer containing that dat

Returns
-------
result : int
     the external memory handle of that buffer
"""
external_dat.argtypes = [
    ctypes.POINTER(DvzRenderer),  # DvzRenderer* rd
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t slot_idx
    Out,  # out DvzSize* offset
]
external_dat.restype = ctypes.c_int


# -------------------------------------------------------------------------------------------------
external_tex = dvz.dvz_external_tex
external_tex.__doc__ = """
Get an external memory handle of a tex's staging buffer.

Parameters
----------
rd : DvzRenderer*
    the renderer
visual : DvzVisual*
    the visual
slot_idx : int
    the slot index of the tex
offset : Out[DvzSize] (out parameter)
    the offset, in bytes, of the tex's staging dat, within the buffer containing

Returns
-------
result : int
     the external memory handle of that buffer
"""
external_tex.argtypes = [
    ctypes.POINTER(DvzRenderer),  # DvzRenderer* rd
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t slot_idx
    Out,  # out DvzSize* offset
]
external_tex.restype = ctypes.c_int


# -------------------------------------------------------------------------------------------------
gui_window_capture = dvz.dvz_gui_window_capture
gui_window_capture.__doc__ = """
Capture a GUI window.

Parameters
----------
gui_window : DvzGuiWindow*
    the GUI window
is_captured : bool
    whether the windows should be captured
"""
gui_window_capture.argtypes = [
    ctypes.POINTER(DvzGuiWindow),  # DvzGuiWindow* gui_window
    ctypes.c_bool,  # bool is_captured
]


# -------------------------------------------------------------------------------------------------
gui_moving = dvz.dvz_gui_moving
gui_moving.__doc__ = """
Return whether a dialog is being moved.


Returns
-------
result : bool
     whether the dialog is being moved
"""
gui_moving.argtypes = [
]
gui_moving.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
gui_resizing = dvz.dvz_gui_resizing
gui_resizing.__doc__ = """
Return whether a dialog is being resized


Returns
-------
result : bool
     whether the dialog is being resized
"""
gui_resizing.argtypes = [
]
gui_resizing.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
gui_moved = dvz.dvz_gui_moved
gui_moved.__doc__ = """
Return whether a dialog has just moved.


Returns
-------
result : bool
     whether the dialog has just moved
"""
gui_moved.argtypes = [
]
gui_moved.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
gui_resized = dvz.dvz_gui_resized
gui_resized.__doc__ = """
Return whether a dialog has just been resized.


Returns
-------
result : bool
     whether the dialog has just been resized
"""
gui_resized.argtypes = [
]
gui_resized.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
gui_collapsed = dvz.dvz_gui_collapsed
gui_collapsed.__doc__ = """
Return whether a dialog is collapsed.


Returns
-------
result : bool
     whether the dialog is collapsed
"""
gui_collapsed.argtypes = [
]
gui_collapsed.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
gui_collapse_changed = dvz.dvz_gui_collapse_changed
gui_collapse_changed.__doc__ = """
Return whether a dialog has just been collapsed or uncollapsed.


Returns
-------
result : bool
     whether the dialog has just been collapsed or uncollapsed.
"""
gui_collapse_changed.argtypes = [
]
gui_collapse_changed.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
gui_dragging = dvz.dvz_gui_dragging
gui_dragging.__doc__ = """
Return whether mouse is dragging.


Returns
-------
result : bool
     whether the mouse is dragging
"""
gui_dragging.argtypes = [
]
gui_dragging.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
gui_pos = dvz.dvz_gui_pos
gui_pos.__doc__ = """
Set the position of the next GUI dialog.

Parameters
----------
pos : Tuple[float, float]
    the dialog position
pivot : Tuple[float, float]
    the pivot
"""
gui_pos.argtypes = [
    vec2,  # vec2 pos
    vec2,  # vec2 pivot
]


# -------------------------------------------------------------------------------------------------
gui_fixed = dvz.dvz_gui_fixed
gui_fixed.__doc__ = """
Set a fixed position for a GUI dialog.

Parameters
----------
pos : Tuple[float, float]
    the dialog position
pivot : Tuple[float, float]
    the pivot
"""
gui_fixed.argtypes = [
    vec2,  # vec2 pos
    vec2,  # vec2 pivot
]


# -------------------------------------------------------------------------------------------------
gui_viewport = dvz.dvz_gui_viewport
gui_viewport.__doc__ = """
Get the position and size of the current dialog.

Parameters
----------
viewport : Tuple[float, float, float, float]
    the x, y, w, h values
"""
gui_viewport.argtypes = [
    vec4,  # vec4 viewport
]


# -------------------------------------------------------------------------------------------------
gui_corner = dvz.dvz_gui_corner
gui_corner.__doc__ = """
Set the corner position of the next GUI dialog.

Parameters
----------
corner : DvzCorner
    which corner
pad : Tuple[float, float]
    the pad
"""
gui_corner.argtypes = [
    DvzCorner,  # DvzCorner corner
    vec2,  # vec2 pad
]


# -------------------------------------------------------------------------------------------------
gui_size = dvz.dvz_gui_size
gui_size.__doc__ = """
Set the size of the next GUI dialog.

Parameters
----------
size : Tuple[float, float]
    the size
"""
gui_size.argtypes = [
    vec2,  # vec2 size
]


# -------------------------------------------------------------------------------------------------
gui_color = dvz.dvz_gui_color
gui_color.__doc__ = """
Set the color of an element.

Parameters
----------
type : int
    the element type for which to change the color
color : Tuple[int, int, int, int]
    the color
"""
gui_color.argtypes = [
    ctypes.c_int,  # int type
    DvzColor,  # DvzColor color
]


# -------------------------------------------------------------------------------------------------
gui_style = dvz.dvz_gui_style
gui_style.__doc__ = """
Set the style of an element.

Parameters
----------
type : int
    the element type for which to change the style
value : float
    the value
"""
gui_style.argtypes = [
    ctypes.c_int,  # int type
    ctypes.c_float,  # float value
]


# -------------------------------------------------------------------------------------------------
gui_flags = dvz.dvz_gui_flags
gui_flags.__doc__ = """
Set the flags of the next GUI dialog.

Parameters
----------
flags : int
    the flags

Returns
-------
result : int
"""
gui_flags.argtypes = [
    ctypes.c_int,  # int flags
]
gui_flags.restype = ctypes.c_int


# -------------------------------------------------------------------------------------------------
gui_alpha = dvz.dvz_gui_alpha
gui_alpha.__doc__ = """
Set the alpha transparency of the next GUI dialog.

Parameters
----------
alpha : float
    the alpha transparency value
"""
gui_alpha.argtypes = [
    ctypes.c_float,  # float alpha
]


# -------------------------------------------------------------------------------------------------
gui_begin = dvz.dvz_gui_begin
gui_begin.__doc__ = """
Start a new dialog.

Parameters
----------
title : str
    the dialog title
flags : int
    the flags
"""
gui_begin.argtypes = [
    CStringBuffer,  # char* title
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
gui_text = dvz.dvz_gui_text
gui_text.__doc__ = """
Add a text item in a dialog.

Parameters
----------
fmt : str
    the format string
"""
gui_text.argtypes = [
    CStringBuffer,  # char* fmt
]


# -------------------------------------------------------------------------------------------------
gui_textbox = dvz.dvz_gui_textbox
gui_textbox.__doc__ = """
Add a text box in a dialog.

Parameters
----------
label : str
    the label
str_len : int
    the size of the str buffer
str : str
    the modified string
flags : int
    the flags

Returns
-------
result : bool
     whether the text has changed
"""
gui_textbox.argtypes = [
    CStringBuffer,  # char* label
    ctypes.c_uint32,  # uint32_t str_len
    CStringBuffer,  # char* str
    ctypes.c_int,  # int flags
]
gui_textbox.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
gui_slider = dvz.dvz_gui_slider
gui_slider.__doc__ = """
Add a slider.

Parameters
----------
name : str
    the slider name
vmin : float
    the minimum value
vmax : float
    the maximum value
value : Out[float] (out parameter)
    the pointer to the value

Returns
-------
result : bool
     whether the value has changed
"""
gui_slider.argtypes = [
    CStringBuffer,  # char* name
    ctypes.c_float,  # float vmin
    ctypes.c_float,  # float vmax
    Out,  # out float* value
]
gui_slider.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
gui_slider_vec2 = dvz.dvz_gui_slider_vec2
gui_slider_vec2.__doc__ = """
Add a slider with 2 values.

Parameters
----------
name : str
    the slider name
vmin : float
    the minimum value
vmax : float
    the maximum value
value : Out[Tuple[float, float]] (out parameter)
    the pointer to the value

Returns
-------
result : bool
     whether the value has changed
"""
gui_slider_vec2.argtypes = [
    CStringBuffer,  # char* name
    ctypes.c_float,  # float vmin
    ctypes.c_float,  # float vmax
    vec2,  # out vec2 value
]
gui_slider_vec2.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
gui_slider_vec3 = dvz.dvz_gui_slider_vec3
gui_slider_vec3.__doc__ = """
Add a slider with 3 values.

Parameters
----------
name : str
    the slider name
vmin : float
    the minimum value
vmax : float
    the maximum value
value : Out[Tuple[float, float, float]] (out parameter)
    the pointer to the value

Returns
-------
result : bool
     whether the value has changed
"""
gui_slider_vec3.argtypes = [
    CStringBuffer,  # char* name
    ctypes.c_float,  # float vmin
    ctypes.c_float,  # float vmax
    vec3,  # out vec3 value
]
gui_slider_vec3.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
gui_slider_vec4 = dvz.dvz_gui_slider_vec4
gui_slider_vec4.__doc__ = """
Add a slider with 4 values.

Parameters
----------
name : str
    the slider name
vmin : float
    the minimum value
vmax : float
    the maximum value
value : Out[Tuple[float, float, float, float]] (out parameter)
    the pointer to the value

Returns
-------
result : bool
     whether the value has changed
"""
gui_slider_vec4.argtypes = [
    CStringBuffer,  # char* name
    ctypes.c_float,  # float vmin
    ctypes.c_float,  # float vmax
    vec4,  # out vec4 value
]
gui_slider_vec4.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
gui_slider_int = dvz.dvz_gui_slider_int
gui_slider_int.__doc__ = """
Add an integer slider.

Parameters
----------
name : str
    the slider name
vmin : int
    the minimum value
vmax : int
    the maximum value
value : Out[int] (out parameter)
    the pointer to the value

Returns
-------
result : bool
     whether the value has changed
"""
gui_slider_int.argtypes = [
    CStringBuffer,  # char* name
    ctypes.c_int,  # int vmin
    ctypes.c_int,  # int vmax
    Out,  # out int* value
]
gui_slider_int.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
gui_slider_ivec2 = dvz.dvz_gui_slider_ivec2
gui_slider_ivec2.__doc__ = """
Add an integer slider with 2 values.

Parameters
----------
name : str
    the slider name
vmin : int
    the minimum value
vmax : int
    the maximum value
value : Out[Tuple[int, int]] (out parameter)
    the pointer to the value

Returns
-------
result : bool
     whether the value has changed
"""
gui_slider_ivec2.argtypes = [
    CStringBuffer,  # char* name
    ctypes.c_int,  # int vmin
    ctypes.c_int,  # int vmax
    ivec2,  # out ivec2 value
]
gui_slider_ivec2.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
gui_slider_ivec3 = dvz.dvz_gui_slider_ivec3
gui_slider_ivec3.__doc__ = """
Add an integer slider with 3 values.

Parameters
----------
name : str
    the slider name
vmin : int
    the minimum value
vmax : int
    the maximum value
value : Out[Tuple[int, int, int]] (out parameter)
    the pointer to the value

Returns
-------
result : bool
     whether the value has changed
"""
gui_slider_ivec3.argtypes = [
    CStringBuffer,  # char* name
    ctypes.c_int,  # int vmin
    ctypes.c_int,  # int vmax
    ivec3,  # out ivec3 value
]
gui_slider_ivec3.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
gui_slider_ivec4 = dvz.dvz_gui_slider_ivec4
gui_slider_ivec4.__doc__ = """
Add an integer slider with 4 values.

Parameters
----------
name : str
    the slider name
vmin : int
    the minimum value
vmax : int
    the maximum value
value : Out[Tuple[int, int, int, int]] (out parameter)
    the pointer to the value

Returns
-------
result : bool
     whether the value has changed
"""
gui_slider_ivec4.argtypes = [
    CStringBuffer,  # char* name
    ctypes.c_int,  # int vmin
    ctypes.c_int,  # int vmax
    ivec4,  # out ivec4 value
]
gui_slider_ivec4.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
gui_button = dvz.dvz_gui_button
gui_button.__doc__ = """
Add a button.

Parameters
----------
name : str
    the button name
width : float
    the button width
height : float
    the button height

Returns
-------
result : bool
     whether the button was pressed
"""
gui_button.argtypes = [
    CStringBuffer,  # char* name
    ctypes.c_float,  # float width
    ctypes.c_float,  # float height
]
gui_button.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
gui_checkbox = dvz.dvz_gui_checkbox
gui_checkbox.__doc__ = """
Add a checkbox.

Parameters
----------
name : str
    the button name
checked : Out[bool] (out parameter)
    whether the checkbox is checked

Returns
-------
result : bool
     whether the checkbox's state has changed
"""
gui_checkbox.argtypes = [
    CStringBuffer,  # char* name
    Out,  # out bool* checked
]
gui_checkbox.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
gui_dropdown = dvz.dvz_gui_dropdown
gui_dropdown.__doc__ = """
Add a dropdown menu.

Parameters
----------
name : str
    the menu name
count : int
    the number of menu items
items : List[str]
    the item labels
selected : Out[int] (out parameter)
    a pointer to the selected index
flags : int
    the dropdown menu flags

Returns
-------
result : bool
"""
gui_dropdown.argtypes = [
    CStringBuffer,  # char* name
    ctypes.c_uint32,  # uint32_t count
    CStringArrayType,  # char** items
    Out,  # out uint32_t* selected
    ctypes.c_int,  # int flags
]
gui_dropdown.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
gui_progress = dvz.dvz_gui_progress
gui_progress.__doc__ = """
Add a progress widget.

Parameters
----------
fraction : float
    the fraction between 0 and 1
width : float
    the widget width
height : float
    the widget height
fmt : str
    the format string
"""
gui_progress.argtypes = [
    ctypes.c_float,  # float fraction
    ctypes.c_float,  # float width
    ctypes.c_float,  # float height
    CStringBuffer,  # char* fmt
]


# -------------------------------------------------------------------------------------------------
gui_image = dvz.dvz_gui_image
gui_image.__doc__ = """
Add an image in a GUI dialog.

Parameters
----------
tex : DvzTex*
    the texture
width : float
    the image width
height : float
    the image height
"""
gui_image.argtypes = [
    ctypes.POINTER(DvzTex),  # DvzTex* tex
    ctypes.c_float,  # float width
    ctypes.c_float,  # float height
]


# -------------------------------------------------------------------------------------------------
gui_colorpicker = dvz.dvz_gui_colorpicker
gui_colorpicker.__doc__ = """
Add a color picker

Parameters
----------
name : str
    the widget name
color : Tuple[float, float, float]
    the color
flags : int
    the widget flags

Returns
-------
result : bool
"""
gui_colorpicker.argtypes = [
    CStringBuffer,  # char* name
    vec3,  # vec3 color
    ctypes.c_int,  # int flags
]
gui_colorpicker.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
gui_node = dvz.dvz_gui_node
gui_node.__doc__ = """
Start a new tree node.

Parameters
----------
name : str
    the widget name

Returns
-------
result : bool
"""
gui_node.argtypes = [
    CStringBuffer,  # char* name
]
gui_node.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
gui_pop = dvz.dvz_gui_pop
gui_pop.__doc__ = """
Close the current tree node.
"""
gui_pop.argtypes = [
]


# -------------------------------------------------------------------------------------------------
gui_clicked = dvz.dvz_gui_clicked
gui_clicked.__doc__ = """
Close the current tree node.


Returns
-------
result : bool
"""
gui_clicked.argtypes = [
]
gui_clicked.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
gui_selectable = dvz.dvz_gui_selectable
gui_selectable.__doc__ = """
Close the current tree node.

Parameters
----------
name : str
    the widget name

Returns
-------
result : bool
"""
gui_selectable.argtypes = [
    CStringBuffer,  # char* name
]
gui_selectable.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
gui_table = dvz.dvz_gui_table
gui_table.__doc__ = """
Display a table with selectable rows.

Parameters
----------
name : str
    the widget name
row_count : int
    the number of rows
column_count : int
    the number of columns
labels : List[str]
    all cell labels
selected : np.ndarray[bool]
    a pointer to an array of boolean indicated which rows are selected
flags : int
    the Dear ImGui flags

Returns
-------
result : bool
     whether the row selection has changed (in the selected array)
"""
gui_table.argtypes = [
    CStringBuffer,  # char* name
    ctypes.c_uint32,  # uint32_t row_count
    ctypes.c_uint32,  # uint32_t column_count
    CStringArrayType,  # char** labels
    ndpointer(dtype=bool, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # bool* selected
    ctypes.c_int,  # int flags
]
gui_table.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
gui_tree = dvz.dvz_gui_tree
gui_tree.__doc__ = """
Display a collapsible tree. Assumes the data is in the right order, with level encoding the
depth of each row within the tree.
Filtering can be implemented with the "visible" parameter. Note that this function automatically
propagates the visibility of each node to all its descendents and ascendents, without modifying
in-place the "visible" array.

Parameters
----------
count : int
    the number of rows
ids : List[str]
    short id of each row
labels : List[str]
    full label of each row
levels : np.ndarray[uint32_t]
    a positive integer indicate
colors : DvzColor*
    the color of each square in each row
folded : np.ndarray[bool]
    whether each row is currently folded (modified by this function)
selected : np.ndarray[bool]
    whether each row is currently selected (modified by this function)
visible : np.ndarray[bool]
    whether each row is visible (used for filtering)

Returns
-------
result : bool
     whether the selection has changed
"""
gui_tree.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    CStringArrayType,  # char** ids
    CStringArrayType,  # char** labels
    ndpointer(dtype=np.uint32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # uint32_t* levels
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # DvzColor* colors
    ndpointer(dtype=bool, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # bool* folded
    ndpointer(dtype=bool, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # bool* selected
    ndpointer(dtype=bool, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # bool* visible
]
gui_tree.restype = ctypes.c_bool


# -------------------------------------------------------------------------------------------------
gui_demo = dvz.dvz_gui_demo
gui_demo.__doc__ = """
Show the demo GUI.
"""
gui_demo.argtypes = [
]


# -------------------------------------------------------------------------------------------------
gui_end = dvz.dvz_gui_end
gui_end.__doc__ = """
Stop the creation of the dialog.
"""
gui_end.argtypes = [
]


# -------------------------------------------------------------------------------------------------
num_procs = dvz.dvz_num_procs
num_procs.__doc__ = """
Return the number of processors on the current system.


Returns
-------
result : int
     the number of processors
"""
num_procs.argtypes = [
]
num_procs.restype = ctypes.c_int


# -------------------------------------------------------------------------------------------------
threads_set = dvz.dvz_threads_set
threads_set.__doc__ = """
Set the number of threads to use in OpenMP-aware functions.

Parameters
----------
num_threads : int
    the requested number of threads
"""
threads_set.argtypes = [
    ctypes.c_int,  # int num_threads
]


# -------------------------------------------------------------------------------------------------
threads_get = dvz.dvz_threads_get
threads_get.__doc__ = """
Get the number of threads to use in OpenMP-aware functions.


Returns
-------
result : int
     the current number of threads specified to OpenMP
"""
threads_get.argtypes = [
]
threads_get.restype = ctypes.c_int


# -------------------------------------------------------------------------------------------------
threads_default = dvz.dvz_threads_default
threads_default.__doc__ = """
Set the number of threads to use in OpenMP-aware functions based on DVZ_NUM_THREADS, or take
half of dvz_num_procs().
"""
threads_default.argtypes = [
]


# -------------------------------------------------------------------------------------------------
next_pow2 = dvz.dvz_next_pow2
next_pow2.__doc__ = """
Return the smallest power of 2 larger or equal than a positive integer.

Parameters
----------
x : int
    the value

Returns
-------
result : uint64_t
     the power of 2
"""
next_pow2.argtypes = [
    ctypes.c_uint64,  # uint64_t x
]
next_pow2.restype = ctypes.c_uint64


# -------------------------------------------------------------------------------------------------
mean = dvz.dvz_mean
mean.__doc__ = """
Compute the mean of an array of double values.

Parameters
----------
n : int
    the number of values
values : np.ndarray[double]
    an array of double numbers

Returns
-------
result : double
     the mean
"""
mean.argtypes = [
    ctypes.c_uint32,  # uint32_t n
    ndpointer(dtype=np.double, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # double* values
]
mean.restype = ctypes.c_double


# -------------------------------------------------------------------------------------------------
min_max = dvz.dvz_min_max
min_max.__doc__ = """
Compute the min and max of an array of float values.

Parameters
----------
n : int
    the number of values
values : np.ndarray[float]
    an array of float numbers
out_min_max : Tuple[float, float]
    the min and max
"""
min_max.argtypes = [
    ctypes.c_uint32,  # uint32_t n
    ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # float* values
    vec2,  # vec2 out_min_max
]


# -------------------------------------------------------------------------------------------------
normalize_bytes = dvz.dvz_normalize_bytes
normalize_bytes.__doc__ = """
Normalize the array.

Parameters
----------
min_max : Tuple[float, float]
    the minimum and maximum values, mapped to 0 and 255, the result will be clipped
count : int
    the number of values
values : np.ndarray[float]
    an array of float numbers
out : np.ndarray[uint8_t]
    the out uint8 array
"""
normalize_bytes.argtypes = [
    vec2,  # vec2 min_max
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # float* values
    ndpointer(dtype=np.uint8, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # uint8_t* out
]


# -------------------------------------------------------------------------------------------------
range = dvz.dvz_range
range.__doc__ = """
Compute the range of an array of double values.

Parameters
----------
n : int
    the number of values
values : np.ndarray[double]
    an array of double numbers
min_max : Out[dvec2] (out parameter)
    the min and max values
"""
range.argtypes = [
    ctypes.c_uint32,  # uint32_t n
    ndpointer(dtype=np.double, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # double* values
    dvec2,  # out dvec2 min_max
]


# -------------------------------------------------------------------------------------------------
earcut = dvz.dvz_earcut
earcut.__doc__ = """
Compute a polygon triangulation with only indexing on the polygon contour vertices.

Parameters
----------
point_count : int
    the number of points
polygon : np.ndarray[dvec2]
    the polygon 2D positions
out_index_count : Out[int] (out parameter)
    the computed index count

Returns
-------
result : DvzIndex*
     the computed indices (must be FREED by the caller)
"""
earcut.argtypes = [
    ctypes.c_uint32,  # uint32_t point_count
    ndpointer(dtype=np.double, ndim=2, ncol=2, flags="C_CONTIGUOUS"),  # dvec2* polygon
    Out,  # out uint32_t* out_index_count
]
earcut.restype = ndpointer(dtype=np.uint32, ndim=1, ncol=1, flags="C_CONTIGUOUS")


# -------------------------------------------------------------------------------------------------
rand_byte = dvz.dvz_rand_byte
rand_byte.__doc__ = """
Return a random integer number between 0 and 255.


Returns
-------
result : uint8_t
     random number
"""
rand_byte.argtypes = [
]
rand_byte.restype = ctypes.c_uint8


# -------------------------------------------------------------------------------------------------
rand_int = dvz.dvz_rand_int
rand_int.__doc__ = """
Return a random integer number.


Returns
-------
result : int
     random number
"""
rand_int.argtypes = [
]
rand_int.restype = ctypes.c_int


# -------------------------------------------------------------------------------------------------
rand_float = dvz.dvz_rand_float
rand_float.__doc__ = """
Return a random floating-point number between 0 and 1.


Returns
-------
result : float
     random number
"""
rand_float.argtypes = [
]
rand_float.restype = ctypes.c_float


# -------------------------------------------------------------------------------------------------
rand_double = dvz.dvz_rand_double
rand_double.__doc__ = """
Return a random floating-point number between 0 and 1.


Returns
-------
result : double
     random number
"""
rand_double.argtypes = [
]
rand_double.restype = ctypes.c_double


# -------------------------------------------------------------------------------------------------
rand_normal = dvz.dvz_rand_normal
rand_normal.__doc__ = """
Return a random normal floating-point number.


Returns
-------
result : double
     random number
"""
rand_normal.argtypes = [
]
rand_normal.restype = ctypes.c_double


# -------------------------------------------------------------------------------------------------
mock_pos_2D = dvz.dvz_mock_pos_2D
mock_pos_2D.__doc__ = """
Generate a set of random 2D positions.

Parameters
----------
count : int
    the number of positions to generate
std : float
    the standard deviation

Returns
-------
result : vec3*
     the positions
"""
mock_pos_2D.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    ctypes.c_float,  # float std
]
mock_pos_2D.restype = ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS")


# -------------------------------------------------------------------------------------------------
mock_circle = dvz.dvz_mock_circle
mock_circle.__doc__ = """
Generate points on a circle.

Parameters
----------
count : int
    the number of positions to generate
radius : float
    the radius of the circle

Returns
-------
result : vec3*
     the positions
"""
mock_circle.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    ctypes.c_float,  # float radius
]
mock_circle.restype = ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS")


# -------------------------------------------------------------------------------------------------
mock_band = dvz.dvz_mock_band
mock_band.__doc__ = """
Generate points on a band.

Parameters
----------
count : int
    the number of positions to generate
size : Tuple[float, float]
    the size of the band

Returns
-------
result : vec3*
     the positions
"""
mock_band.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    vec2,  # vec2 size
]
mock_band.restype = ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS")


# -------------------------------------------------------------------------------------------------
mock_pos_3D = dvz.dvz_mock_pos_3D
mock_pos_3D.__doc__ = """
Generate a set of random 3D positions.

Parameters
----------
count : int
    the number of positions to generate
std : float
    the standard deviation

Returns
-------
result : vec3*
     the positions
"""
mock_pos_3D.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    ctypes.c_float,  # float std
]
mock_pos_3D.restype = ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS")


# -------------------------------------------------------------------------------------------------
mock_fixed = dvz.dvz_mock_fixed
mock_fixed.__doc__ = """
Generate identical 3D positions.

Parameters
----------
count : int
    the number of positions to generate
fixed : Tuple[float, float, float]
    the position

Returns
-------
result : vec3*
     the repeated positions
"""
mock_fixed.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    vec3,  # vec3 fixed
]
mock_fixed.restype = ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS")


# -------------------------------------------------------------------------------------------------
mock_line = dvz.dvz_mock_line
mock_line.__doc__ = """
Generate 3D positions on a line.

Parameters
----------
count : int
    the number of positions to generate
p0 : Tuple[float, float, float]
    initial position
p1 : Tuple[float, float, float]
    terminal position

Returns
-------
result : vec3*
     the positions
"""
mock_line.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    vec3,  # vec3 p0
    vec3,  # vec3 p1
]
mock_line.restype = ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS")


# -------------------------------------------------------------------------------------------------
mock_uniform = dvz.dvz_mock_uniform
mock_uniform.__doc__ = """
Generate a set of uniformly random scalar values.

Parameters
----------
count : int
    the number of values to generate
vmin : float
    the minimum value of the interval
vmax : float
    the maximum value of the interval

Returns
-------
result : float*
     the values
"""
mock_uniform.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    ctypes.c_float,  # float vmin
    ctypes.c_float,  # float vmax
]
mock_uniform.restype = ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS")


# -------------------------------------------------------------------------------------------------
mock_full = dvz.dvz_mock_full
mock_full.__doc__ = """
Generate an array with the same value.

Parameters
----------
count : int
    the number of scalars to generate
value : float
    the value

Returns
-------
result : float*
     the values
"""
mock_full.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    ctypes.c_float,  # float value
]
mock_full.restype = ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS")


# -------------------------------------------------------------------------------------------------
mock_range = dvz.dvz_mock_range
mock_range.__doc__ = """
Generate an array of consecutive positive numbers.

Parameters
----------
count : int
    the number of consecutive integers to generate
initial : int
    the initial value

Returns
-------
result : uint32_t*
     the values
"""
mock_range.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    ctypes.c_uint32,  # uint32_t initial
]
mock_range.restype = ndpointer(dtype=np.uint32, ndim=1, ncol=1, flags="C_CONTIGUOUS")


# -------------------------------------------------------------------------------------------------
mock_linspace = dvz.dvz_mock_linspace
mock_linspace.__doc__ = """
Generate an array ranging from an initial value to a final value.

Parameters
----------
count : int
    the number of scalars to generate
initial : float
    the initial value
final : float
    the final value

Returns
-------
result : float*
     the values
"""
mock_linspace.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    ctypes.c_float,  # float initial
    ctypes.c_float,  # float final
]
mock_linspace.restype = ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS")


# -------------------------------------------------------------------------------------------------
mock_color = dvz.dvz_mock_color
mock_color.__doc__ = """
Generate a set of random colors.

Parameters
----------
count : int
    the number of colors to generate
alpha : DvzAlpha
    the alpha value

Returns
-------
result : DvzColor*
     random colors
"""
mock_color.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    DvzAlpha,  # DvzAlpha alpha
]
mock_color.restype = ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS")


# -------------------------------------------------------------------------------------------------
mock_monochrome = dvz.dvz_mock_monochrome
mock_monochrome.__doc__ = """
Repeat a color in an array.

Parameters
----------
count : int
    the number of colors to generate
mono : Tuple[int, int, int, int]
    the color to repeat

Returns
-------
result : DvzColor*
     colors
"""
mock_monochrome.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    DvzColor,  # DvzColor mono
]
mock_monochrome.restype = ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS")


# -------------------------------------------------------------------------------------------------
mock_cmap = dvz.dvz_mock_cmap
mock_cmap.__doc__ = """
Generate a set of colormap colors.

Parameters
----------
count : int
    the number of colors to generate
cmap : DvzColormap
    the colormap
alpha : DvzAlpha
    the alpha value

Returns
-------
result : DvzColor*
     colors
"""
mock_cmap.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    DvzColormap,  # DvzColormap cmap
    DvzAlpha,  # DvzAlpha alpha
]
mock_cmap.restype = ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS")


# -------------------------------------------------------------------------------------------------
requester = dvz.dvz_requester
requester.__doc__ = """
Create a requester, used to create requests.


Returns
-------
result : DvzRequester*
     the requester struct
"""
requester.argtypes = [
]
requester.restype = ctypes.POINTER(DvzRequester)


# -------------------------------------------------------------------------------------------------
requester_destroy = dvz.dvz_requester_destroy
requester_destroy.__doc__ = """
Destroy a requester.

Parameters
----------
rqr : DvzRequester*
    the requester
"""
requester_destroy.argtypes = [
    ctypes.POINTER(DvzRequester),  # DvzRequester* rqr
]


# -------------------------------------------------------------------------------------------------
mvp = dvz.dvz_mvp
mvp.__doc__ = """
Create a MVP structure.

Parameters
----------
model : mat4
    the model matrix
view : mat4
    the view matrix
proj : mat4
    the projection matrix
mvp : Out[DvzMVP] (out parameter)
    the MVP structure
"""
mvp.argtypes = [
    mat4,  # mat4 model
    mat4,  # mat4 view
    mat4,  # mat4 proj
    ctypes.POINTER(DvzMVP),  # out DvzMVP* mvp
]


# -------------------------------------------------------------------------------------------------
mvp_default = dvz.dvz_mvp_default
mvp_default.__doc__ = """
Return a default DvzMVP struct

Parameters
----------
mvp : Out[DvzMVP] (out parameter)
    the DvzMVP struct
"""
mvp_default.argtypes = [
    ctypes.POINTER(DvzMVP),  # out DvzMVP* mvp
]


# -------------------------------------------------------------------------------------------------
viewport_default = dvz.dvz_viewport_default
viewport_default.__doc__ = """
Return a default viewport

Parameters
----------
width : int
    the viewport width, in framebuffer pixels
height : int
    the viewport height, in framebuffer pixels
viewport : Out[DvzViewport] (out parameter)
    the viewport
"""
viewport_default.argtypes = [
    ctypes.c_uint32,  # uint32_t width
    ctypes.c_uint32,  # uint32_t height
    ctypes.POINTER(DvzViewport),  # out DvzViewport* viewport
]


# -------------------------------------------------------------------------------------------------
batch = dvz.dvz_batch
batch.__doc__ = """
Create a batch holding a number of requests.


Returns
-------
result : DvzBatch*
"""
batch.argtypes = [
]
batch.restype = ctypes.POINTER(DvzBatch)


# -------------------------------------------------------------------------------------------------
batch_clear = dvz.dvz_batch_clear
batch_clear.__doc__ = """
Remove all requests in a batch.

Parameters
----------
batch : DvzBatch*
    the batch
"""
batch_clear.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
]


# -------------------------------------------------------------------------------------------------
batch_add = dvz.dvz_batch_add
batch_add.__doc__ = """
Add a request to a batch.

Parameters
----------
batch : DvzBatch*
    the batch
req : DvzRequest
    the request
"""
batch_add.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzRequest,  # DvzRequest req
]


# -------------------------------------------------------------------------------------------------
batch_desc = dvz.dvz_batch_desc
batch_desc.__doc__ = """
Set the description of the last added request.

Parameters
----------
batch : DvzBatch*
    the batch
desc : str
    the description
"""
batch_desc.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    CStringBuffer,  # char* desc
]


# -------------------------------------------------------------------------------------------------
batch_requests = dvz.dvz_batch_requests
batch_requests.__doc__ = """
Return a pointer to the array of all requests in the batch.

Parameters
----------
batch : DvzBatch*
    the batch

Returns
-------
result : DvzRequest*
"""
batch_requests.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
]
batch_requests.restype = ctypes.POINTER(DvzRequest)


# -------------------------------------------------------------------------------------------------
batch_size = dvz.dvz_batch_size
batch_size.__doc__ = """
Return the number of requests in the batch.

Parameters
----------
batch : DvzBatch*
    the batch

Returns
-------
result : uint32_t
"""
batch_size.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
]
batch_size.restype = ctypes.c_uint32


# -------------------------------------------------------------------------------------------------
batch_print = dvz.dvz_batch_print
batch_print.__doc__ = """
Display information about all requests in the batch.

Parameters
----------
batch : DvzBatch*
    the batch
flags : int
    the flags
"""
batch_print.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
batch_yaml = dvz.dvz_batch_yaml
batch_yaml.__doc__ = """
Export requests in a YAML file.

Parameters
----------
batch : DvzBatch*
    the batch
filename : str
    the YAML filename
"""
batch_yaml.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    CStringBuffer,  # char* filename
]


# -------------------------------------------------------------------------------------------------
batch_dump = dvz.dvz_batch_dump
batch_dump.__doc__ = """
Dump all batch requests in raw binary file.

Parameters
----------
batch : DvzBatch*
    the batch
filename : str
    the dump filename

Returns
-------
result : int
"""
batch_dump.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    CStringBuffer,  # char* filename
]
batch_dump.restype = ctypes.c_int


# -------------------------------------------------------------------------------------------------
batch_load = dvz.dvz_batch_load
batch_load.__doc__ = """
Load a dump of batch requests into an existing batch object.

Parameters
----------
batch : DvzBatch*
    the batch
filename : str
    the dump filename
"""
batch_load.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    CStringBuffer,  # char* filename
]


# -------------------------------------------------------------------------------------------------
batch_copy = dvz.dvz_batch_copy
batch_copy.__doc__ = """
Create a copy of a batch.

Parameters
----------
batch : DvzBatch*
    the batch

Returns
-------
result : DvzBatch*
"""
batch_copy.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
]
batch_copy.restype = ctypes.POINTER(DvzBatch)


# -------------------------------------------------------------------------------------------------
batch_destroy = dvz.dvz_batch_destroy
batch_destroy.__doc__ = """
Destroy a batch.

Parameters
----------
batch : DvzBatch*
    the batch
"""
batch_destroy.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
]


# -------------------------------------------------------------------------------------------------
requester_commit = dvz.dvz_requester_commit
requester_commit.__doc__ = """
Add a batch's requests to a requester.

Parameters
----------
rqr : DvzRequester*
    the requester
batch : DvzBatch*
    the batch
"""
requester_commit.argtypes = [
    ctypes.POINTER(DvzRequester),  # DvzRequester* rqr
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
]


# -------------------------------------------------------------------------------------------------
requester_flush = dvz.dvz_requester_flush
requester_flush.__doc__ = """
Return the requests in the requester and clear it.
NOTE: the caller must free the output.

Parameters
----------
rqr : DvzRequester*
    the requester
count : Out[int] (out parameter)
    pointer to the number of requests, set by this function

Returns
-------
result : DvzBatch*
     an array with all requests in the requester
"""
requester_flush.argtypes = [
    ctypes.POINTER(DvzRequester),  # DvzRequester* rqr
    Out,  # out uint32_t* count
]
requester_flush.restype = ctypes.POINTER(DvzBatch)


# -------------------------------------------------------------------------------------------------
request_print = dvz.dvz_request_print
request_print.__doc__ = """
Display information about a request.

Parameters
----------
req : DvzRequest*
    the request
flags : int
    the flags
"""
request_print.argtypes = [
    ctypes.POINTER(DvzRequest),  # DvzRequest* req
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
create_canvas = dvz.dvz_create_canvas
create_canvas.__doc__ = """
Create a request for canvas creation.
A canvas is a live window on which to render.
NOTE: background color not implemented yet

Parameters
----------
batch : DvzBatch*
    the batch
width : int
    the canvas width (in screen pixels)
height : int
    the canvas height (in screen pixels)
background : cvec4
    the background color
flags : int
    the canvas creation flags

Returns
-------
result : DvzRequest
     the request, containing a newly-generated id for the canvas to be created
"""
create_canvas.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_uint32,  # uint32_t width
    ctypes.c_uint32,  # uint32_t height
    cvec4,  # cvec4 background
    ctypes.c_int,  # int flags
]
create_canvas.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
set_background = dvz.dvz_set_background
set_background.__doc__ = """
Change the background color of the canvas.

Parameters
----------
batch : DvzBatch*
    the batch
id : DvzId
    the canvas id
background : cvec4
    the background color

Returns
-------
result : DvzRequest
     the request
"""
set_background.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId id
    cvec4,  # cvec4 background
]
set_background.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
update_canvas = dvz.dvz_update_canvas
update_canvas.__doc__ = """
Create a request for a canvas redraw (command buffer submission).

Parameters
----------
batch : DvzBatch*
    the batch
id : DvzId
    the canvas id

Returns
-------
result : DvzRequest
     the request
"""
update_canvas.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId id
]
update_canvas.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
resize_canvas = dvz.dvz_resize_canvas
resize_canvas.__doc__ = """
Create a request to resize an offscreen canvas (regular canvases are resized by the client).

Parameters
----------
batch : DvzBatch*
    the batch
canvas : DvzId
    the canvas id
width : int
    the new canvas width
height : int
    the new canvas height

Returns
-------
result : DvzRequest
     the request
"""
resize_canvas.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId canvas
    ctypes.c_uint32,  # uint32_t width
    ctypes.c_uint32,  # uint32_t height
]
resize_canvas.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
delete_canvas = dvz.dvz_delete_canvas
delete_canvas.__doc__ = """
Create a request for a canvas deletion.

Parameters
----------
batch : DvzBatch*
    the batch
id : DvzId
    the canvas id

Returns
-------
result : DvzRequest
     the request
"""
delete_canvas.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId id
]
delete_canvas.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
create_dat = dvz.dvz_create_dat
create_dat.__doc__ = """
Create a request for a dat creation.

Parameters
----------
batch : DvzBatch*
    the batch
type : DvzBufferType
    the buffer type
size : DvzSize
    the dat size, in bytes
flags : int
    the dat creation flags

Returns
-------
result : DvzRequest
     the request, containing a newly-generated id for the dat to be created
"""
create_dat.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzBufferType,  # DvzBufferType type
    DvzSize,  # DvzSize size
    ctypes.c_int,  # int flags
]
create_dat.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
resize_dat = dvz.dvz_resize_dat
resize_dat.__doc__ = """
Create a request to resize a dat.

Parameters
----------
batch : DvzBatch*
    the batch
dat : DvzId
    the dat id
size : DvzSize
    the new dat size, in bytes

Returns
-------
result : DvzRequest
     the request
"""
resize_dat.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId dat
    DvzSize,  # DvzSize size
]
resize_dat.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
upload_dat = dvz.dvz_upload_dat
upload_dat.__doc__ = """
Create a request for dat upload.
NOTE: this function makes a COPY of the buffer to ensure it will live until the upload actually
occurs. The copy will be freed automatically as soon as it's safe.

Parameters
----------
batch : DvzBatch*
    the batch
dat : DvzId
    the id of the dat to upload to
offset : DvzSize
    the byte offset of the upload transfer
size : DvzSize
    the number of bytes in data to transfer
data : np.ndarray
    a pointer to the data to upload
flags : int
    the upload flags

Returns
-------
result : DvzRequest
     the request
"""
upload_dat.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId dat
    DvzSize,  # DvzSize offset
    DvzSize,  # DvzSize size
    ndpointer(dtype=None, ndim=None, flags="C_CONTIGUOUS"),  # void* data
    ctypes.c_int,  # int flags
]
upload_dat.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
delete_dat = dvz.dvz_delete_dat
delete_dat.__doc__ = """
Create a request for dat deletion.

Parameters
----------
batch : DvzBatch*
    the batch
id : DvzId
    the dat id

Returns
-------
result : DvzRequest
     the request
"""
delete_dat.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId id
]
delete_dat.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
create_tex = dvz.dvz_create_tex
create_tex.__doc__ = """
Create a request for a tex creation.

Parameters
----------
batch : DvzBatch*
    the batch
dims : DvzTexDims
    the number of dimensions, 1, 2, or 3
format : DvzFormat
    the image format
shape : Tuple[int, int, int]
    the texture shape
flags : int
    the dat creation flags

Returns
-------
result : DvzRequest
     the request, containing a newly-generated id for the tex to be created
"""
create_tex.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzTexDims,  # DvzTexDims dims
    DvzFormat,  # DvzFormat format
    uvec3,  # uvec3 shape
    ctypes.c_int,  # int flags
]
create_tex.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
resize_tex = dvz.dvz_resize_tex
resize_tex.__doc__ = """
Create a request to resize a tex.

Parameters
----------
batch : DvzBatch*
    the batch
tex : DvzId
    the tex id
shape : Tuple[int, int, int]
    the new tex shape

Returns
-------
result : DvzRequest
     the request
"""
resize_tex.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId tex
    uvec3,  # uvec3 shape
]
resize_tex.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
upload_tex = dvz.dvz_upload_tex
upload_tex.__doc__ = """
Create a request for tex upload.
NOTE: this function makes a COPY of the buffer to ensure it will live until the upload actually
occurs. The copy will be freed automatically as soon as it's safe.

Parameters
----------
batch : DvzBatch*
    the batch
tex : DvzId
    the id of the tex to upload to
offset : Tuple[int, int, int]
    the offset
shape : Tuple[int, int, int]
    the shape
size : DvzSize
    the number of bytes in data to transfer
data : np.ndarray
    a pointer to the data to upload
flags : int
    the upload flags

Returns
-------
result : DvzRequest
     the request
"""
upload_tex.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId tex
    uvec3,  # uvec3 offset
    uvec3,  # uvec3 shape
    DvzSize,  # DvzSize size
    ndpointer(dtype=None, ndim=None, flags="C_CONTIGUOUS"),  # void* data
    ctypes.c_int,  # int flags
]
upload_tex.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
delete_tex = dvz.dvz_delete_tex
delete_tex.__doc__ = """
Create a request for tex deletion.

Parameters
----------
batch : DvzBatch*
    the batch
id : DvzId
    the tex id

Returns
-------
result : DvzRequest
     the request
"""
delete_tex.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId id
]
delete_tex.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
create_sampler = dvz.dvz_create_sampler
create_sampler.__doc__ = """
Create a request for a sampler creation.

Parameters
----------
batch : DvzBatch*
    the batch
filter : DvzFilter
    the sampler filter
mode : DvzSamplerAddressMode
    the sampler address mode

Returns
-------
result : DvzRequest
     the request, containing a newly-generated id for the sampler to be created
"""
create_sampler.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzFilter,  # DvzFilter filter
    DvzSamplerAddressMode,  # DvzSamplerAddressMode mode
]
create_sampler.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
delete_sampler = dvz.dvz_delete_sampler
delete_sampler.__doc__ = """
Create a request for sampler deletion.

Parameters
----------
batch : DvzBatch*
    the batch
id : DvzId
    the sampler id

Returns
-------
result : DvzRequest
     the request
"""
delete_sampler.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId id
]
delete_sampler.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
create_glsl = dvz.dvz_create_glsl
create_glsl.__doc__ = """
Create a request for GLSL shader creation.

Parameters
----------
batch : DvzBatch*
    the batch
shader_type : DvzShaderType
    the shader type
code : str
    an ASCII string with the GLSL code

Returns
-------
result : DvzRequest
     the request
"""
create_glsl.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzShaderType,  # DvzShaderType shader_type
    CStringBuffer,  # char* code
]
create_glsl.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
create_spirv = dvz.dvz_create_spirv
create_spirv.__doc__ = """
Create a request for SPIR-V shader creation.

Parameters
----------
batch : DvzBatch*
    the batch
shader_type : DvzShaderType
    the shader type
size : DvzSize
    the size in bytes of the SPIR-V buffer
buffer : str
    pointer to a buffer with the SPIR-V bytecode

Returns
-------
result : DvzRequest
     the request
"""
create_spirv.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzShaderType,  # DvzShaderType shader_type
    DvzSize,  # DvzSize size
    CStringBuffer,  # char* buffer
]
create_spirv.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
create_graphics = dvz.dvz_create_graphics
create_graphics.__doc__ = """
Create a request for a builtin graphics pipe creation.

Parameters
----------
batch : DvzBatch*
    the batch
type : DvzGraphicsType
    the graphics type
flags : int
    the graphics creation flags

Returns
-------
result : DvzRequest
     the request, containing a newly-generated id for the graphics pipe to be created
"""
create_graphics.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzGraphicsType,  # DvzGraphicsType type
    ctypes.c_int,  # int flags
]
create_graphics.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
set_primitive = dvz.dvz_set_primitive
set_primitive.__doc__ = """
Create a request for setting the primitive topology of a graphics pipe.

Parameters
----------
batch : DvzBatch*
    the batch
graphics : DvzId
    the graphics pipe id
primitive : DvzPrimitiveTopology
    the graphics primitive topology

Returns
-------
result : DvzRequest
     the request
"""
set_primitive.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId graphics
    DvzPrimitiveTopology,  # DvzPrimitiveTopology primitive
]
set_primitive.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
set_blend = dvz.dvz_set_blend
set_blend.__doc__ = """
Create a request for setting the blend type of a graphics pipe.

Parameters
----------
batch : DvzBatch*
    the batch
graphics : DvzId
    the graphics pipe id
blend_type : DvzBlendType
    the graphics blend type

Returns
-------
result : DvzRequest
     the request
"""
set_blend.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId graphics
    DvzBlendType,  # DvzBlendType blend_type
]
set_blend.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
set_mask = dvz.dvz_set_mask
set_mask.__doc__ = """
Create a request for setting the color mask of a graphics pipe.

Parameters
----------
batch : DvzBatch*
    the batch
graphics : DvzId
    the graphics pipe id
mask : int
    the mask with RGBA boolean masks on the lower bits

Returns
-------
result : DvzRequest
     the request
"""
set_mask.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId graphics
    ctypes.c_int32,  # int32_t mask
]
set_mask.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
set_depth = dvz.dvz_set_depth
set_depth.__doc__ = """
Create a request for setting the depth test of a graphics pipe.

Parameters
----------
batch : DvzBatch*
    the batch
graphics : DvzId
    the graphics pipe id
depth_test : DvzDepthTest
    the graphics depth test

Returns
-------
result : DvzRequest
     the request
"""
set_depth.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId graphics
    DvzDepthTest,  # DvzDepthTest depth_test
]
set_depth.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
set_polygon = dvz.dvz_set_polygon
set_polygon.__doc__ = """
Create a request for setting the polygon mode of a graphics pipe.

Parameters
----------
batch : DvzBatch*
    the batch
graphics : DvzId
    the graphics pipe id
polygon_mode : DvzPolygonMode
    the polygon mode

Returns
-------
result : DvzRequest
     the request
"""
set_polygon.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId graphics
    DvzPolygonMode,  # DvzPolygonMode polygon_mode
]
set_polygon.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
set_cull = dvz.dvz_set_cull
set_cull.__doc__ = """
Create a request for setting the cull mode of a graphics pipe.

Parameters
----------
batch : DvzBatch*
    the batch
graphics : DvzId
    the graphics pipe id
cull_mode : DvzCullMode
    the cull mode

Returns
-------
result : DvzRequest
     the request
"""
set_cull.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId graphics
    DvzCullMode,  # DvzCullMode cull_mode
]
set_cull.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
set_front = dvz.dvz_set_front
set_front.__doc__ = """
Create a request for setting the front face of a graphics pipe.

Parameters
----------
batch : DvzBatch*
    the batch
graphics : DvzId
    the graphics pipe id
front_face : DvzFrontFace
    the front face

Returns
-------
result : DvzRequest
     the request
"""
set_front.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId graphics
    DvzFrontFace,  # DvzFrontFace front_face
]
set_front.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
set_shader = dvz.dvz_set_shader
set_shader.__doc__ = """
Create a request for setting a shader a graphics pipe.

Parameters
----------
batch : DvzBatch*
    the batch
graphics : DvzId
    the graphics pipe id
shader : DvzId
    the id of the shader object

Returns
-------
result : DvzRequest
     the request
"""
set_shader.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId graphics
    DvzId,  # DvzId shader
]
set_shader.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
set_vertex = dvz.dvz_set_vertex
set_vertex.__doc__ = """
Create a request for setting a vertex binding of a graphics pipe.

Parameters
----------
batch : DvzBatch*
    the batch
graphics : DvzId
    the graphics pipe id
binding_idx : int
    the index of the vertex binding
stride : DvzSize
    the binding stride
input_rate : DvzVertexInputRate
    the vertex input rate, per-vertex or per-instance

Returns
-------
result : DvzRequest
     the request
"""
set_vertex.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId graphics
    ctypes.c_uint32,  # uint32_t binding_idx
    DvzSize,  # DvzSize stride
    DvzVertexInputRate,  # DvzVertexInputRate input_rate
]
set_vertex.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
set_attr = dvz.dvz_set_attr
set_attr.__doc__ = """
Create a request for setting a vertex attribute of a graphics pipe.

Parameters
----------
batch : DvzBatch*
    the batch
graphics : DvzId
    the graphics pipe id
binding_idx : int
    the index of the vertex binding
location : int
    the GLSL attribute location
format : DvzFormat
    the attribute format
offset : DvzSize
    the byte offset of the attribute within the vertex binding

Returns
-------
result : DvzRequest
     the request
"""
set_attr.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId graphics
    ctypes.c_uint32,  # uint32_t binding_idx
    ctypes.c_uint32,  # uint32_t location
    DvzFormat,  # DvzFormat format
    DvzSize,  # DvzSize offset
]
set_attr.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
set_slot = dvz.dvz_set_slot
set_slot.__doc__ = """
Create a request for setting a binding slot (descriptor) of a graphics pipe.

Parameters
----------
batch : DvzBatch*
    the batch
graphics : DvzId
    the graphics pipe id
slot_idx : int
    the index of the GLSL binding slot
type : DvzDescriptorType
    the descriptor type

Returns
-------
result : DvzRequest
     the request
"""
set_slot.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId graphics
    ctypes.c_uint32,  # uint32_t slot_idx
    DvzDescriptorType,  # DvzDescriptorType type
]
set_slot.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
set_push = dvz.dvz_set_push
set_push.__doc__ = """
Create a request for setting a push constant layout for a graphics pipe.

Parameters
----------
batch : DvzBatch*
    the batch
graphics : DvzId
    the graphics pipe id
shader_stages : DvzShaderStageFlags
    the shader stages with the push constant
offset : DvzSize
    the byte offset for the push data visibility from the shader
size : DvzSize
    how much bytes the shader can see from the push constant

Returns
-------
result : DvzRequest
     the request
"""
set_push.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId graphics
    DvzShaderStageFlags,  # DvzShaderStageFlags shader_stages
    DvzSize,  # DvzSize offset
    DvzSize,  # DvzSize size
]
set_push.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
set_specialization = dvz.dvz_set_specialization
set_specialization.__doc__ = """
Create a request for setting a specialization constant of a graphics pipe.

Parameters
----------
batch : DvzBatch*
    the batch
graphics : DvzId
    the graphics pipe id
shader : DvzShaderType
    the shader with the specialization constant
idx : int
    the specialization constant index as specified in the GLSL code
size : DvzSize
    the byte size of the value
value : np.ndarray
    a pointer to the specialization constant value

Returns
-------
result : DvzRequest
     the request
"""
set_specialization.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId graphics
    DvzShaderType,  # DvzShaderType shader
    ctypes.c_uint32,  # uint32_t idx
    DvzSize,  # DvzSize size
    ctypes.c_void_p,  # void* value
]
set_specialization.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
delete_graphics = dvz.dvz_delete_graphics
delete_graphics.__doc__ = """
Create a request for graphics deletion.

Parameters
----------
batch : DvzBatch*
    the batch
id : DvzId
    the graphics id

Returns
-------
result : DvzRequest
     the request
"""
delete_graphics.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId id
]
delete_graphics.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
bind_vertex = dvz.dvz_bind_vertex
bind_vertex.__doc__ = """
Create a request for associating a vertex dat to a graphics pipe.

Parameters
----------
batch : DvzBatch*
    the batch
graphics : DvzId
    the id of the graphics pipe
binding_idx : int
    the vertex binding index
dat : DvzId
    the id of the dat with the vertex data
offset : DvzSize
    the offset within the dat

Returns
-------
result : DvzRequest
     the request
"""
bind_vertex.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId graphics
    ctypes.c_uint32,  # uint32_t binding_idx
    DvzId,  # DvzId dat
    DvzSize,  # DvzSize offset
]
bind_vertex.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
bind_index = dvz.dvz_bind_index
bind_index.__doc__ = """
Create a request for associating an index dat to a graphics pipe.

Parameters
----------
batch : DvzBatch*
    the batch
graphics : DvzId
    the id of the graphics pipe
dat : DvzId
    the id of the dat with the index data
offset : DvzSize
    the offset within the dat

Returns
-------
result : DvzRequest
     the request
"""
bind_index.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId graphics
    DvzId,  # DvzId dat
    DvzSize,  # DvzSize offset
]
bind_index.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
bind_dat = dvz.dvz_bind_dat
bind_dat.__doc__ = """
Create a request for associating a dat to a pipe's slot.

Parameters
----------
batch : DvzBatch*
    the batch
pipe : DvzId
    the id of the pipe
slot_idx : int
    the index of the descriptor slot
dat : DvzId
    the id of the dat to bind to the pipe
offset : DvzSize
    the offset

Returns
-------
result : DvzRequest
     the request
"""
bind_dat.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId pipe
    ctypes.c_uint32,  # uint32_t slot_idx
    DvzId,  # DvzId dat
    DvzSize,  # DvzSize offset
]
bind_dat.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
bind_tex = dvz.dvz_bind_tex
bind_tex.__doc__ = """
Create a request for associating a tex to a pipe's slot.

Parameters
----------
batch : DvzBatch*
    the batch
pipe : DvzId
    the id of the pipe
slot_idx : int
    the index of the descriptor slot
tex : DvzId
    the id of the tex to bind to the pipe
sampler : DvzId
    the id of the sampler
offset : Tuple[int, int, int]
    the offset

Returns
-------
result : DvzRequest
     the request
"""
bind_tex.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId pipe
    ctypes.c_uint32,  # uint32_t slot_idx
    DvzId,  # DvzId tex
    DvzId,  # DvzId sampler
    uvec3,  # uvec3 offset
]
bind_tex.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
record_begin = dvz.dvz_record_begin
record_begin.__doc__ = """
Create a request for starting recording of command buffer.

Parameters
----------
batch : DvzBatch*
    the batch
canvas_id : DvzId
    the id of the canvas

Returns
-------
result : DvzRequest
     the request
"""
record_begin.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId canvas_id
]
record_begin.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
record_viewport = dvz.dvz_record_viewport
record_viewport.__doc__ = """
Create a request for setting the viewport during command buffer recording.

Parameters
----------
batch : DvzBatch*
    the batch
canvas_id : DvzId
    the id of the canvas
offset : Tuple[float, float]
    the viewport offset, in framebuffer pixels
shape : Tuple[float, float]
    the viewport size, in framebuffer pixels

Returns
-------
result : DvzRequest
     the request
"""
record_viewport.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId canvas_id
    vec2,  # vec2 offset
    vec2,  # vec2 shape
]
record_viewport.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
record_draw = dvz.dvz_record_draw
record_draw.__doc__ = """
Create a request for a direct draw of a graphics during command buffer recording.

Parameters
----------
batch : DvzBatch*
    the batch
canvas_id : DvzId
    the id of the canvas
graphics : DvzId
    the id of the graphics pipe to draw
first_vertex : int
    the index of the first vertex to draw
vertex_count : int
    the number of vertices to draw
first_instance : int
    the index of the first instance to draw
instance_count : int
    the number of instances to draw

Returns
-------
result : DvzRequest
     the request
"""
record_draw.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId canvas_id
    DvzId,  # DvzId graphics
    ctypes.c_uint32,  # uint32_t first_vertex
    ctypes.c_uint32,  # uint32_t vertex_count
    ctypes.c_uint32,  # uint32_t first_instance
    ctypes.c_uint32,  # uint32_t instance_count
]
record_draw.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
record_draw_indexed = dvz.dvz_record_draw_indexed
record_draw_indexed.__doc__ = """
Create a request for an indexed draw of a graphics during command buffer recording.

Parameters
----------
batch : DvzBatch*
    the batch
canvas_id : DvzId
    the id of the canvas
graphics : DvzId
    the id of the graphics pipe to draw
first_index : int
    the index of the first index to draw
vertex_offset : int
    the vertex offset within the vertices indexed by the indexes
index_count : int
    the number of indexes to draw
first_instance : int
    the index of the first instance to draw
instance_count : int
    the number of instances to draw

Returns
-------
result : DvzRequest
     the request
"""
record_draw_indexed.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId canvas_id
    DvzId,  # DvzId graphics
    ctypes.c_uint32,  # uint32_t first_index
    ctypes.c_uint32,  # uint32_t vertex_offset
    ctypes.c_uint32,  # uint32_t index_count
    ctypes.c_uint32,  # uint32_t first_instance
    ctypes.c_uint32,  # uint32_t instance_count
]
record_draw_indexed.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
record_draw_indirect = dvz.dvz_record_draw_indirect
record_draw_indirect.__doc__ = """
Create a request for an indirect draw of a graphics during command buffer recording.

Parameters
----------
batch : DvzBatch*
    the batch
canvas_id : DvzId
    the id of the canvas
graphics : DvzId
    the id of the graphics pipe to draw
indirect : DvzId
    the id of the dat containing the indirect draw data
draw_count : int
    the number of draws to make

Returns
-------
result : DvzRequest
     the request
"""
record_draw_indirect.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId canvas_id
    DvzId,  # DvzId graphics
    DvzId,  # DvzId indirect
    ctypes.c_uint32,  # uint32_t draw_count
]
record_draw_indirect.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
record_draw_indexed_indirect = dvz.dvz_record_draw_indexed_indirect
record_draw_indexed_indirect.__doc__ = """
Create a request for an indexed indirect draw of a graphics during command buffer recording.

Parameters
----------
batch : DvzBatch*
    the batch
canvas_id : DvzId
    the id of the canvas
graphics : DvzId
    the id of the graphics pipe to draw
indirect : DvzId
    the id of the dat containing the indirect draw data
draw_count : int
    the number of draws to make

Returns
-------
result : DvzRequest
     the request
"""
record_draw_indexed_indirect.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId canvas_id
    DvzId,  # DvzId graphics
    DvzId,  # DvzId indirect
    ctypes.c_uint32,  # uint32_t draw_count
]
record_draw_indexed_indirect.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
record_push = dvz.dvz_record_push
record_push.__doc__ = """
Create a request for sending a push constant value while recording a command buffer.

Parameters
----------
batch : DvzBatch*
    the batch
canvas_id : DvzId
    the id of the canvas
graphics_id : DvzId
    the id of the graphics pipeline
shader_stages : DvzShaderStageFlags
    the shader stages
offset : DvzSize
    the byte offset
size : DvzSize
    the size of the data to upload
data : np.ndarray
    the push constant data to upload

Returns
-------
result : DvzRequest
     the request
"""
record_push.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId canvas_id
    DvzId,  # DvzId graphics_id
    DvzShaderStageFlags,  # DvzShaderStageFlags shader_stages
    DvzSize,  # DvzSize offset
    DvzSize,  # DvzSize size
    ndpointer(dtype=None, ndim=None, flags="C_CONTIGUOUS"),  # void* data
]
record_push.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
record_end = dvz.dvz_record_end
record_end.__doc__ = """
Create a request for ending recording of command buffer.

Parameters
----------
batch : DvzBatch*
    the batch
canvas_id : DvzId
    the id of the canvas

Returns
-------
result : DvzRequest
     the request
"""
record_end.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzId,  # DvzId canvas_id
]
record_end.restype = DvzRequest


# -------------------------------------------------------------------------------------------------
basic = dvz.dvz_basic
basic.__doc__ = """
Create a basic visual using the few GPU visual primitives (point, line, triangles).

Parameters
----------
batch : DvzBatch*
    the batch
topology : DvzPrimitiveTopology
    the primitive topology
flags : int
    the visual creation flags

Returns
-------
result : DvzVisual*
     the visual
"""
basic.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzPrimitiveTopology,  # DvzPrimitiveTopology topology
    ctypes.c_int,  # int flags
]
basic.restype = ctypes.POINTER(DvzVisual)


# -------------------------------------------------------------------------------------------------
basic_position = dvz.dvz_basic_position
basic_position.__doc__ = """
Set the vertex positions.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[vec3]
    the 3D positions of the items to update
flags : int
    the data update flags
"""
basic_position.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
basic_color = dvz.dvz_basic_color
basic_color.__doc__ = """
Set the vertex colors.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : DvzColor*
    the colors of the items to update
flags : int
    the data update flags
"""
basic_color.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # DvzColor* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
basic_group = dvz.dvz_basic_group
basic_group.__doc__ = """
Set the vertex group index.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[float]
    the group index of each vertex
flags : int
    the data update flags
"""
basic_group.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # float* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
basic_size = dvz.dvz_basic_size
basic_size.__doc__ = """
Set the point size (for POINT_LIST topology only).

Parameters
----------
visual : DvzVisual*
    the visual
size : float
    the point size in pixels
"""
basic_size.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_float,  # float size
]


# -------------------------------------------------------------------------------------------------
basic_alloc = dvz.dvz_basic_alloc
basic_alloc.__doc__ = """
Allocate memory for a visual.

Parameters
----------
visual : DvzVisual*
    the visual
item_count : int
    the total number of items to allocate for this visual
"""
basic_alloc.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t item_count
]


# -------------------------------------------------------------------------------------------------
basic_shape = dvz.dvz_basic_shape
basic_shape.__doc__ = """
Create a basic visual from a DvzShape instance.

Parameters
----------
batch : DvzBatch*
    the batch
shape : DvzShape*
    the shape
flags : int
    the visual creation flags

Returns
-------
result : DvzVisual*
     the visual
"""
basic_shape.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    ctypes.c_int,  # int flags
]
basic_shape.restype = ctypes.POINTER(DvzVisual)


# -------------------------------------------------------------------------------------------------
pixel = dvz.dvz_pixel
pixel.__doc__ = """
Create a pixel visual.

Parameters
----------
batch : DvzBatch*
    the batch
flags : int
    the visual creation flags

Returns
-------
result : DvzVisual*
     the visual
"""
pixel.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_int,  # int flags
]
pixel.restype = ctypes.POINTER(DvzVisual)


# -------------------------------------------------------------------------------------------------
pixel_position = dvz.dvz_pixel_position
pixel_position.__doc__ = """
Set the pixel positions.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[vec3]
    the 3D positions of the items to update
flags : int
    the data update flags
"""
pixel_position.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
pixel_color = dvz.dvz_pixel_color
pixel_color.__doc__ = """
Set the pixel colors.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : DvzColor*
    the colors of the items to update
flags : int
    the data update flags
"""
pixel_color.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # DvzColor* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
pixel_size = dvz.dvz_pixel_size
pixel_size.__doc__ = """
Set the pixel size.

Parameters
----------
visual : DvzVisual*
    the visual
size : float
    the point size in pixels
"""
pixel_size.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_float,  # float size
]


# -------------------------------------------------------------------------------------------------
pixel_alloc = dvz.dvz_pixel_alloc
pixel_alloc.__doc__ = """
Allocate memory for a visual.

Parameters
----------
visual : DvzVisual*
    the visual
item_count : int
    the total number of items to allocate for this visual
"""
pixel_alloc.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t item_count
]


# -------------------------------------------------------------------------------------------------
point = dvz.dvz_point
point.__doc__ = """
Create a point visual.

Parameters
----------
batch : DvzBatch*
    the batch
flags : int
    the visual creation flags

Returns
-------
result : DvzVisual*
     the visual
"""
point.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_int,  # int flags
]
point.restype = ctypes.POINTER(DvzVisual)


# -------------------------------------------------------------------------------------------------
point_position = dvz.dvz_point_position
point_position.__doc__ = """
Set the point positions.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[vec3]
    the 3D positions of the items to update
flags : int
    the data update flags
"""
point_position.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
point_size = dvz.dvz_point_size
point_size.__doc__ = """
Set the point sizes.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[float]
    the sizes of the items to update
flags : int
    the data update flags
"""
point_size.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # float* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
point_color = dvz.dvz_point_color
point_color.__doc__ = """
Set the point colors.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : DvzColor*
    the colors of the items to update
flags : int
    the data update flags
"""
point_color.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # DvzColor* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
point_alloc = dvz.dvz_point_alloc
point_alloc.__doc__ = """
Allocate memory for a visual.

Parameters
----------
visual : DvzVisual*
    the visual
item_count : int
    the total number of items to allocate for this visual
"""
point_alloc.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t item_count
]


# -------------------------------------------------------------------------------------------------
marker = dvz.dvz_marker
marker.__doc__ = """
Create a marker visual.

Parameters
----------
batch : DvzBatch*
    the batch
flags : int
    the visual creation flags

Returns
-------
result : DvzVisual*
     the visual
"""
marker.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_int,  # int flags
]
marker.restype = ctypes.POINTER(DvzVisual)


# -------------------------------------------------------------------------------------------------
marker_mode = dvz.dvz_marker_mode
marker_mode.__doc__ = """
Set the marker mode.

Parameters
----------
visual : DvzVisual*
    the visual
mode : DvzMarkerMode
    the marker mode, one of DVZ_MARKER_MODE_CODE, DVZ_MARKER_MODE_BITMAP,
"""
marker_mode.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzMarkerMode,  # DvzMarkerMode mode
]


# -------------------------------------------------------------------------------------------------
marker_aspect = dvz.dvz_marker_aspect
marker_aspect.__doc__ = """
Set the marker aspect.

Parameters
----------
visual : DvzVisual*
    the visual
aspect : DvzMarkerAspect
    the marker aspect, one of DVZ_MARKER_ASPECT_FILLED, DVZ_MARKER_ASPECT_STROKE,
"""
marker_aspect.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzMarkerAspect,  # DvzMarkerAspect aspect
]


# -------------------------------------------------------------------------------------------------
marker_shape = dvz.dvz_marker_shape
marker_shape.__doc__ = """
Set the marker shape.

Parameters
----------
visual : DvzVisual*
    the visual
shape : DvzMarkerShape
    the marker shape
"""
marker_shape.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzMarkerShape,  # DvzMarkerShape shape
]


# -------------------------------------------------------------------------------------------------
marker_position = dvz.dvz_marker_position
marker_position.__doc__ = """
Set the marker positions.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[vec3]
    the 3D positions of the items to update
flags : int
    the data update flags
"""
marker_position.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
marker_size = dvz.dvz_marker_size
marker_size.__doc__ = """
Set the marker sizes.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[float]
    the colors of the items to update
flags : int
    the data update flags
"""
marker_size.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # float* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
marker_angle = dvz.dvz_marker_angle
marker_angle.__doc__ = """
Set the marker angles.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[float]
    the angles of the items to update
flags : int
    the data update flags
"""
marker_angle.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # float* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
marker_color = dvz.dvz_marker_color
marker_color.__doc__ = """
Set the marker colors.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : DvzColor*
    the colors of the items to update
flags : int
    the data update flags
"""
marker_color.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # DvzColor* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
marker_edgecolor = dvz.dvz_marker_edgecolor
marker_edgecolor.__doc__ = """
Set the marker edge color.

Parameters
----------
visual : DvzVisual*
    the visual
color : Tuple[int, int, int, int]
    the edge color
"""
marker_edgecolor.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzColor,  # DvzColor color
]


# -------------------------------------------------------------------------------------------------
marker_linewidth = dvz.dvz_marker_linewidth
marker_linewidth.__doc__ = """
Set the marker edge width.

Parameters
----------
visual : DvzVisual*
    the visual
width : float
    the edge width
"""
marker_linewidth.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_float,  # float width
]


# -------------------------------------------------------------------------------------------------
marker_texture = dvz.dvz_marker_texture
marker_texture.__doc__ = """
Set the marker texture.

Parameters
----------
visual : DvzVisual*
    the visual
texture : DvzTexture*
    the texture
"""
marker_texture.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.POINTER(DvzTexture),  # DvzTexture* texture
]


# -------------------------------------------------------------------------------------------------
marker_tex_scale = dvz.dvz_marker_tex_scale
marker_tex_scale.__doc__ = """
Set the texture scale.

Parameters
----------
visual : DvzVisual*
    the visual
scale : float
    the texture scale
"""
marker_tex_scale.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_float,  # float scale
]


# -------------------------------------------------------------------------------------------------
marker_alloc = dvz.dvz_marker_alloc
marker_alloc.__doc__ = """
Allocate memory for a visual.

Parameters
----------
visual : DvzVisual*
    the visual
item_count : int
    the total number of items to allocate for this visual
"""
marker_alloc.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t item_count
]


# -------------------------------------------------------------------------------------------------
segment = dvz.dvz_segment
segment.__doc__ = """
Create a segment visual.

Parameters
----------
batch : DvzBatch*
    the batch
flags : int
    the visual creation flags

Returns
-------
result : DvzVisual*
     the visual
"""
segment.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_int,  # int flags
]
segment.restype = ctypes.POINTER(DvzVisual)


# -------------------------------------------------------------------------------------------------
segment_position = dvz.dvz_segment_position
segment_position.__doc__ = """
Set the segment positions.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
initial : np.ndarray[vec3]
    the initial 3D positions of the segments
terminal : np.ndarray[vec3]
    the terminal 3D positions of the segments
flags : int
    the data update flags
"""
segment_position.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* initial
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* terminal
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
segment_shift = dvz.dvz_segment_shift
segment_shift.__doc__ = """
Set the segment shift.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[vec4]
    the dx0,dy0,dx1,dy1 shift quadriplets of the segments to update
flags : int
    the data update flags
"""
segment_shift.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # vec4* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
segment_color = dvz.dvz_segment_color
segment_color.__doc__ = """
Set the segment colors.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : DvzColor*
    the colors of the items to update
flags : int
    the data update flags
"""
segment_color.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # DvzColor* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
segment_linewidth = dvz.dvz_segment_linewidth
segment_linewidth.__doc__ = """
Set the segment line widths.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[float]
    the segment line widths
flags : int
    the data update flags
"""
segment_linewidth.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # float* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
segment_cap = dvz.dvz_segment_cap
segment_cap.__doc__ = """
Set the segment cap types.

Parameters
----------
visual : DvzVisual*
    the visual
initial : DvzCapType
    the initial segment cap type
terminal : DvzCapType
    the terminal segment cap type
"""
segment_cap.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzCapType,  # DvzCapType initial
    DvzCapType,  # DvzCapType terminal
]


# -------------------------------------------------------------------------------------------------
segment_alloc = dvz.dvz_segment_alloc
segment_alloc.__doc__ = """
Allocate memory for a visual.

Parameters
----------
visual : DvzVisual*
    the visual
item_count : int
    the total number of items to allocate for this visual
"""
segment_alloc.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t item_count
]


# -------------------------------------------------------------------------------------------------
path = dvz.dvz_path
path.__doc__ = """
Create a path visual.

Parameters
----------
batch : DvzBatch*
    the batch
flags : int
    the visual creation flags

Returns
-------
result : DvzVisual*
     the visual
"""
path.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_int,  # int flags
]
path.restype = ctypes.POINTER(DvzVisual)


# -------------------------------------------------------------------------------------------------
path_position = dvz.dvz_path_position
path_position.__doc__ = """
Set the path positions. Note: all path point positions must be updated at once for now.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
point_count : int
    the total number of points across all paths
positions : np.ndarray[vec3]
    the path point positions
path_count : int
    the number of different paths
path_lengths : np.ndarray[uint32_t]
    the number of points in each path
flags : int
    the data update flags
"""
path_position.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t point_count
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* positions
    ctypes.c_uint32,  # uint32_t path_count
    ndpointer(dtype=np.uint32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # uint32_t* path_lengths
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
path_color = dvz.dvz_path_color
path_color.__doc__ = """
Set the path colors.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : DvzColor*
    the colors of the items to update
flags : int
    the data update flags
"""
path_color.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # DvzColor* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
path_linewidth = dvz.dvz_path_linewidth
path_linewidth.__doc__ = """
Set the path line width (may be variable along a path).

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[float]
    the line width of the vertex, in pixels
flags : int
    the data update flags
"""
path_linewidth.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # float* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
path_cap = dvz.dvz_path_cap
path_cap.__doc__ = """
Set the path cap.

Parameters
----------
visual : DvzVisual*
    the visual
cap : DvzCapType
    the cap
"""
path_cap.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzCapType,  # DvzCapType cap
]


# -------------------------------------------------------------------------------------------------
path_join = dvz.dvz_path_join
path_join.__doc__ = """
Set the path join.

Parameters
----------
visual : DvzVisual*
    the visual
join : DvzJoinType
    the join
"""
path_join.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzJoinType,  # DvzJoinType join
]


# -------------------------------------------------------------------------------------------------
path_alloc = dvz.dvz_path_alloc
path_alloc.__doc__ = """
Allocate memory for a visual.

Parameters
----------
visual : DvzVisual*
    the visual
total_point_count : int
    the total number of points to allocate for this visual
"""
path_alloc.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t total_point_count
]


# -------------------------------------------------------------------------------------------------
glyph = dvz.dvz_glyph
glyph.__doc__ = """
Create a glyph visual.

Parameters
----------
batch : DvzBatch*
    the batch
flags : int
    the visual creation flags

Returns
-------
result : DvzVisual*
     the visual
"""
glyph.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_int,  # int flags
]
glyph.restype = ctypes.POINTER(DvzVisual)


# -------------------------------------------------------------------------------------------------
glyph_alloc = dvz.dvz_glyph_alloc
glyph_alloc.__doc__ = """
Allocate memory for a visual.

Parameters
----------
visual : DvzVisual*
    the visual
item_count : int
    the total number of items to allocate for this visual
"""
glyph_alloc.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t item_count
]


# -------------------------------------------------------------------------------------------------
glyph_position = dvz.dvz_glyph_position
glyph_position.__doc__ = """
Set the glyph positions.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[vec3]
    the 3D positions of the items to update
flags : int
    the data update flags
"""
glyph_position.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
glyph_axis = dvz.dvz_glyph_axis
glyph_axis.__doc__ = """
Set the glyph axes.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[vec3]
    the 3D axis vectors of the items to update
flags : int
    the data update flags
"""
glyph_axis.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
glyph_size = dvz.dvz_glyph_size
glyph_size.__doc__ = """
Set the glyph sizes, in pixels.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[vec2]
    the sizes (width and height) of the items to update
flags : int
    the data update flags
"""
glyph_size.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=2, flags="C_CONTIGUOUS"),  # vec2* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
glyph_anchor = dvz.dvz_glyph_anchor
glyph_anchor.__doc__ = """
Set the glyph anchors.
The anchor should be the same for each glyph in a given string. In addition, it is important to
set dvz_glyph_group_size() (the size of each string in pixels) for the anchor computation to be
correct.
The anchor determines the relationship between the glyph 3D position, and the position of the
string bounding box. Each string comes with a local coordinate system extending from (-1, -1)
(bottom-left corner) to (+1, +1) (top-right corner), and (0, 0) refers to the center of the
string. The anchor is the point, in this local coordinate system, that matches the glyph 3D
position. For example, to center a string around the glyph 3D position, use (0, 0) as anchor.
To align the string to the right of the glyph 3D position, use (-1, -1) for example.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[vec2]
    the anchors (x and y) of the items to update
flags : int
    the data update flags
"""
glyph_anchor.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=2, flags="C_CONTIGUOUS"),  # vec2* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
glyph_shift = dvz.dvz_glyph_shift
glyph_shift.__doc__ = """
Set the glyph shifts.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[vec2]
    the shifts (x and y) of the items to update
flags : int
    the data update flags
"""
glyph_shift.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=2, flags="C_CONTIGUOUS"),  # vec2* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
glyph_texcoords = dvz.dvz_glyph_texcoords
glyph_texcoords.__doc__ = """
Set the glyph texture coordinates.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
coords : np.ndarray[vec4]
    the x,y,w,h texture coordinates
flags : int
    the data update flags
"""
glyph_texcoords.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # vec4* coords
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
glyph_group_size = dvz.dvz_glyph_group_size
glyph_group_size.__doc__ = """
Set the glyph group size, in pixels (size of each string).

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[vec2]
    the glyph group shapes (width and height, in pixels)
flags : int
    the data update flags
"""
glyph_group_size.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=2, flags="C_CONTIGUOUS"),  # vec2* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
glyph_scale = dvz.dvz_glyph_scale
glyph_scale.__doc__ = """
Set the glyph scaling applied to the size of all individual glyphs.
We assume that the scaling is the same within each string (group of glyphs).

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[float]
    the scaling of the items to update
flags : int
    the data update flags
"""
glyph_scale.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # float* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
glyph_angle = dvz.dvz_glyph_angle
glyph_angle.__doc__ = """
Set the glyph angles.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[float]
    the angles of the items to update
flags : int
    the data update flags
"""
glyph_angle.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # float* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
glyph_color = dvz.dvz_glyph_color
glyph_color.__doc__ = """
Set the glyph colors.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : DvzColor*
    the colors of the items to update
flags : int
    the data update flags
"""
glyph_color.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # DvzColor* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
glyph_bgcolor = dvz.dvz_glyph_bgcolor
glyph_bgcolor.__doc__ = """
Set the glyph background color.

Parameters
----------
visual : DvzVisual*
    the visual
bgcolor : Tuple[int, int, int, int]
    the background color
"""
glyph_bgcolor.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzColor,  # DvzColor bgcolor
]


# -------------------------------------------------------------------------------------------------
glyph_texture = dvz.dvz_glyph_texture
glyph_texture.__doc__ = """
Assign a texture to a glyph visual.

Parameters
----------
visual : DvzVisual*
    the visual
texture : DvzTexture*
    the texture
"""
glyph_texture.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.POINTER(DvzTexture),  # DvzTexture* texture
]


# -------------------------------------------------------------------------------------------------
glyph_atlas_font = dvz.dvz_glyph_atlas_font
glyph_atlas_font.__doc__ = """
Associate an atlas and font with a glyph visual.

Parameters
----------
visual : DvzVisual*
    the visual
af : DvzAtlasFont*
    the atlas font
"""
glyph_atlas_font.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.POINTER(DvzAtlasFont),  # DvzAtlasFont* af
]


# -------------------------------------------------------------------------------------------------
glyph_unicode = dvz.dvz_glyph_unicode
glyph_unicode.__doc__ = """
Set the glyph unicode code points.

Parameters
----------
visual : DvzVisual*
    the visual
count : int
    the number of glyphs
codepoints : np.ndarray[uint32_t]
    the unicode codepoints
"""
glyph_unicode.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # uint32_t* codepoints
]


# -------------------------------------------------------------------------------------------------
glyph_ascii = dvz.dvz_glyph_ascii
glyph_ascii.__doc__ = """
Set the glyph ascii characters.

Parameters
----------
visual : DvzVisual*
    the visual
string : str
    the characters
"""
glyph_ascii.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    CStringBuffer,  # char* string
]


# -------------------------------------------------------------------------------------------------
glyph_xywh = dvz.dvz_glyph_xywh
glyph_xywh.__doc__ = """
Set the xywh parameters of each glyph.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[vec4]
    the xywh values of each glyph
offset : Tuple[float, float]
    the xy offsets of each glyph
flags : int
    the data update flags
"""
glyph_xywh.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # vec4* values
    vec2,  # vec2 offset
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
glyph_strings = dvz.dvz_glyph_strings
glyph_strings.__doc__ = """
Helper function to easily set multiple strings of the same size and color on a glyph visual.

Parameters
----------
visual : DvzVisual*
    the visual
string_count : int
    the number of strings
strings : List[str]
    the strings
positions : np.ndarray[vec3]
    the positions of each string
scales : np.ndarray[float]
    the scaling of each string
color : Tuple[int, int, int, int]
    the same color for all strings
offset : Tuple[float, float]
    the same offset for all strings
anchor : Tuple[float, float]
    the same anchor for all strings
"""
glyph_strings.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t string_count
    CStringArrayType,  # char** strings
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* positions
    ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # float* scales
    DvzColor,  # DvzColor color
    vec2,  # vec2 offset
    vec2,  # vec2 anchor
]


# -------------------------------------------------------------------------------------------------
monoglyph = dvz.dvz_monoglyph
monoglyph.__doc__ = """
Create a monoglyph visual.

Parameters
----------
batch : DvzBatch*
    the batch
flags : int
    the visual creation flags

Returns
-------
result : DvzVisual*
     the visual
"""
monoglyph.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_int,  # int flags
]
monoglyph.restype = ctypes.POINTER(DvzVisual)


# -------------------------------------------------------------------------------------------------
monoglyph_position = dvz.dvz_monoglyph_position
monoglyph_position.__doc__ = """
Set the glyph positions.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[vec3]
    the 3D positions of the items to update
flags : int
    the data update flags
"""
monoglyph_position.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
monoglyph_offset = dvz.dvz_monoglyph_offset
monoglyph_offset.__doc__ = """
Set the glyph offsets.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[ivec2]
    the glyph offsets (ivec2 integers: row,column)
flags : int
    the data update flags
"""
monoglyph_offset.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ctypes.POINTER(ivec2),  # ivec2* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
monoglyph_color = dvz.dvz_monoglyph_color
monoglyph_color.__doc__ = """
Set the glyph colors.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : DvzColor*
    the colors of the items to update
flags : int
    the data update flags
"""
monoglyph_color.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # DvzColor* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
monoglyph_glyph = dvz.dvz_monoglyph_glyph
monoglyph_glyph.__doc__ = """
Set the text.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the first item
text : str
    the ASCII test (string length without the null terminal byte = number of glyphs)
flags : int
    the upload flags
"""
monoglyph_glyph.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    CStringBuffer,  # char* text
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
monoglyph_anchor = dvz.dvz_monoglyph_anchor
monoglyph_anchor.__doc__ = """
Set the glyph anchor (relative to the glyph size).

Parameters
----------
visual : DvzVisual*
    the visual
anchor : Tuple[float, float]
    the anchor
"""
monoglyph_anchor.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    vec2,  # vec2 anchor
]


# -------------------------------------------------------------------------------------------------
monoglyph_size = dvz.dvz_monoglyph_size
monoglyph_size.__doc__ = """
Set the glyph size (relative to the initial glyph size).

Parameters
----------
visual : DvzVisual*
    the visual
size : float
    the glyph size
"""
monoglyph_size.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_float,  # float size
]


# -------------------------------------------------------------------------------------------------
monoglyph_textarea = dvz.dvz_monoglyph_textarea
monoglyph_textarea.__doc__ = """
All-in-one function for multiline text.

Parameters
----------
visual : DvzVisual*
    the visual
pos : Tuple[float, float, float]
    the text position
color : Tuple[int, int, int, int]
    the text color
size : float
    the glyph size
text : str
    the text, can contain `\n` new lines
"""
monoglyph_textarea.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    vec3,  # vec3 pos
    DvzColor,  # DvzColor color
    ctypes.c_float,  # float size
    CStringBuffer,  # char* text
]


# -------------------------------------------------------------------------------------------------
monoglyph_alloc = dvz.dvz_monoglyph_alloc
monoglyph_alloc.__doc__ = """
Allocate memory for a visual.

Parameters
----------
visual : DvzVisual*
    the visual
item_count : int
    the total number of items to allocate for this visual
"""
monoglyph_alloc.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t item_count
]


# -------------------------------------------------------------------------------------------------
image = dvz.dvz_image
image.__doc__ = """
Create an image visual.

Parameters
----------
batch : DvzBatch*
    the batch
flags : int
    the visual creation flags

Returns
-------
result : DvzVisual*
     the visual
"""
image.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_int,  # int flags
]
image.restype = ctypes.POINTER(DvzVisual)


# -------------------------------------------------------------------------------------------------
image_position = dvz.dvz_image_position
image_position.__doc__ = """
Set the image positions.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[vec3]
    the 3D positions of the top left corner
flags : int
    the data update flags
"""
image_position.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
image_size = dvz.dvz_image_size
image_size.__doc__ = """
Set the image sizes.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[vec2]
    the sizes of each image, in pixels
flags : int
    the data update flags
"""
image_size.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=2, flags="C_CONTIGUOUS"),  # vec2* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
image_anchor = dvz.dvz_image_anchor
image_anchor.__doc__ = """
Set the image anchors.
The anchor determines the relationship between the image 3D position, and the position of the
image on the screen. Each images comes with a local coordinate system extending from (-1, -1)
(bottom-left corner) to (+1, +1) (top-right corner), and (0, 0) refers to the center of the
image. The anchor is the point, in this local coordinate system, that matches the image 3D
position. For example, to center an image around the image 3D position, use (0, 0) as anchor.
To align the image to the right and bottom of the image 3D position, so that this position
refers to the top-left corner, use (-1, +1) as anchor.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[vec2]
    the relative anchors of each image, (0,0 = position pertains to top left corner)
flags : int
    the data update flags
"""
image_anchor.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=2, flags="C_CONTIGUOUS"),  # vec2* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
image_texcoords = dvz.dvz_image_texcoords
image_texcoords.__doc__ = """
Set the image texture coordinates.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
tl_br : np.ndarray[vec4]
    the tex coordinates of the top left and bottom right corners (vec4 u0,v0,u1,v1)
flags : int
    the data update flags
"""
image_texcoords.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # vec4* tl_br
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
image_facecolor = dvz.dvz_image_facecolor
image_facecolor.__doc__ = """
Set the image colors (only when using DVZ_IMAGE_FLAGS_FILL).

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : DvzColor*
    the image colors
flags : int
    the data update flags
"""
image_facecolor.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # DvzColor* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
image_texture = dvz.dvz_image_texture
image_texture.__doc__ = """
Assign a texture to an image visual.

Parameters
----------
visual : DvzVisual*
    the visual
texture : DvzTexture*
    the texture
"""
image_texture.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.POINTER(DvzTexture),  # DvzTexture* texture
]


# -------------------------------------------------------------------------------------------------
image_edgecolor = dvz.dvz_image_edgecolor
image_edgecolor.__doc__ = """
Set the edge color.

Parameters
----------
visual : DvzVisual*
    the visual
color : Tuple[int, int, int, int]
    the edge color
"""
image_edgecolor.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzColor,  # DvzColor color
]


# -------------------------------------------------------------------------------------------------
image_permutation = dvz.dvz_image_permutation
image_permutation.__doc__ = """
Set the texture coordinates index permutation.

Parameters
----------
visual : DvzVisual*
    the visual
ij : Tuple[int, int]
    index permutation
"""
image_permutation.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ivec2,  # ivec2 ij
]


# -------------------------------------------------------------------------------------------------
image_linewidth = dvz.dvz_image_linewidth
image_linewidth.__doc__ = """
Set the edge width.

Parameters
----------
visual : DvzVisual*
    the visual
width : float
    the edge width
"""
image_linewidth.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_float,  # float width
]


# -------------------------------------------------------------------------------------------------
image_radius = dvz.dvz_image_radius
image_radius.__doc__ = """
Use a rounded rectangle for images, with a given radius in pixels.

Parameters
----------
visual : DvzVisual*
    the visual
radius : float
    the rounded corner radius, in pixel
"""
image_radius.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_float,  # float radius
]


# -------------------------------------------------------------------------------------------------
image_colormap = dvz.dvz_image_colormap
image_colormap.__doc__ = """
Specify the colormap when using DVZ_IMAGE_FLAGS_MODE_COLORMAP.
Only the following colormaps are available on the GPU at the moment:
`CMAP_BINARY`
`CMAP_HSV`
`CMAP_CIVIDIS`
`CMAP_INFERNO`
`CMAP_MAGMA`
`CMAP_PLASMA`
`CMAP_VIRIDIS`
`CMAP_AUTUMN`
`CMAP_BONE`
`CMAP_COOL`
`CMAP_COPPER`
`CMAP_HOT`
`CMAP_SPRING`
`CMAP_SUMMER`
`CMAP_WINTER`
`CMAP_JET`

Parameters
----------
visual : DvzVisual*
    the visual
cmap : DvzColormap
    the colormap
"""
image_colormap.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzColormap,  # DvzColormap cmap
]


# -------------------------------------------------------------------------------------------------
image_alloc = dvz.dvz_image_alloc
image_alloc.__doc__ = """
Allocate memory for a visual.

Parameters
----------
visual : DvzVisual*
    the visual
item_count : int
    the total number of images to allocate for this visual
"""
image_alloc.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t item_count
]


# -------------------------------------------------------------------------------------------------
wiggle = dvz.dvz_wiggle
wiggle.__doc__ = """
Create a wiggle visual.

Parameters
----------
batch : DvzBatch*
    the batch
flags : int
    the visual creation flags

Returns
-------
result : DvzVisual*
     the visual
"""
wiggle.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_int,  # int flags
]
wiggle.restype = ctypes.POINTER(DvzVisual)


# -------------------------------------------------------------------------------------------------
wiggle_bounds = dvz.dvz_wiggle_bounds
wiggle_bounds.__doc__ = """
Set the wiggle bounds.

Parameters
----------
visual : DvzVisual*
    the visual
xlim : Tuple[float, float]
    xmin and xmax
ylim : Tuple[float, float]
    ymin and ymax
"""
wiggle_bounds.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    vec2,  # vec2 xlim
    vec2,  # vec2 ylim
]


# -------------------------------------------------------------------------------------------------
wiggle_color = dvz.dvz_wiggle_color
wiggle_color.__doc__ = """
Set the color of the negative and positive sections.

Parameters
----------
visual : DvzVisual*
    the visual
negative_color : Tuple[int, int, int, int]
    the color of the negative section
positive_color : Tuple[int, int, int, int]
    the color of the positive section
"""
wiggle_color.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzColor,  # DvzColor negative_color
    DvzColor,  # DvzColor positive_color
]


# -------------------------------------------------------------------------------------------------
wiggle_edgecolor = dvz.dvz_wiggle_edgecolor
wiggle_edgecolor.__doc__ = """
Set the edge color.

Parameters
----------
visual : DvzVisual*
    the visual
color : Tuple[int, int, int, int]
    the edge color
"""
wiggle_edgecolor.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzColor,  # DvzColor color
]


# -------------------------------------------------------------------------------------------------
wiggle_xrange = dvz.dvz_wiggle_xrange
wiggle_xrange.__doc__ = """
Set the range of the wiggle on the x axis, in normalized coordinates ([0, 1]).

Parameters
----------
visual : DvzVisual*
    the visual
xrange : Tuple[float, float]
    the x0 and xl in the quad, the channels will be in the interval [x0, xl]
"""
wiggle_xrange.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    vec2,  # vec2 xrange
]


# -------------------------------------------------------------------------------------------------
wiggle_scale = dvz.dvz_wiggle_scale
wiggle_scale.__doc__ = """
Set the texture scaling factor.

Parameters
----------
visual : DvzVisual*
    the visual
scale : float
    the scaling factor
"""
wiggle_scale.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_float,  # float scale
]


# -------------------------------------------------------------------------------------------------
wiggle_texture = dvz.dvz_wiggle_texture
wiggle_texture.__doc__ = """
Assign a texture to an wiggle visual.

Parameters
----------
visual : DvzVisual*
    the visual
texture : DvzTexture*
    the texture
"""
wiggle_texture.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.POINTER(DvzTexture),  # DvzTexture* texture
]


# -------------------------------------------------------------------------------------------------
mesh = dvz.dvz_mesh
mesh.__doc__ = """
Create a mesh visual.

Parameters
----------
batch : DvzBatch*
    the batch
flags : int
    the visual creation flags

Returns
-------
result : DvzVisual*
     the visual
"""
mesh.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_int,  # int flags
]
mesh.restype = ctypes.POINTER(DvzVisual)


# -------------------------------------------------------------------------------------------------
mesh_position = dvz.dvz_mesh_position
mesh_position.__doc__ = """
Set the mesh vertex positions.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[vec3]
    the 3D vertex positions
flags : int
    the data update flags
"""
mesh_position.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
mesh_color = dvz.dvz_mesh_color
mesh_color.__doc__ = """
Set the mesh colors.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : DvzColor*
    the vertex colors
flags : int
    the data update flags
"""
mesh_color.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # DvzColor* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
mesh_texcoords = dvz.dvz_mesh_texcoords
mesh_texcoords.__doc__ = """
Set the mesh texture coordinates.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[vec4]
    the vertex texture coordinates (vec4 u,v,*,alpha)
flags : int
    the data update flags
"""
mesh_texcoords.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # vec4* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
mesh_normal = dvz.dvz_mesh_normal
mesh_normal.__doc__ = """
Set the mesh normals.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[vec3]
    the vertex normal vectors
flags : int
    the data update flags
"""
mesh_normal.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
mesh_isoline = dvz.dvz_mesh_isoline
mesh_isoline.__doc__ = """
Set the isolines values.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[float]
    the scalar field for which to draw isolines
flags : int
    the data update flags
"""
mesh_isoline.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # float* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
mesh_left = dvz.dvz_mesh_left
mesh_left.__doc__ = """
Set the distance between the current vertex to the left edge at corner A, B, or C in triangle
ABC.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[vec3]
    the distance to the left edge adjacent to each triangle vertex
flags : int
    the data update flags
"""
mesh_left.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
mesh_right = dvz.dvz_mesh_right
mesh_right.__doc__ = """
Set the distance between the current vertex to the right edge at corner A, B, or C in triangle
ABC.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[vec3]
    the distance to the right edge adjacent to each triangle vertex
flags : int
    the data update flags
"""
mesh_right.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
mesh_contour = dvz.dvz_mesh_contour
mesh_contour.__doc__ = """
Set the contour information for polygon contours.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : np.ndarray[cvec4]
    for vertex A, B, C, the least significant bit is 1 if the opposite edge is a
flags : int
    the data update flags
"""
mesh_contour.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # cvec4* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
mesh_texture = dvz.dvz_mesh_texture
mesh_texture.__doc__ = """
Assign a 2D texture to a mesh visual.

Parameters
----------
visual : DvzVisual*
    the visual
texture : DvzTexture*
    the texture
"""
mesh_texture.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.POINTER(DvzTexture),  # DvzTexture* texture
]


# -------------------------------------------------------------------------------------------------
mesh_index = dvz.dvz_mesh_index
mesh_index.__doc__ = """
Set the mesh indices.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
values : DvzIndex*
    the face indices (three vertex indices per triangle)
flags : int
    the data update flags
"""
mesh_index.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # DvzIndex* values
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
mesh_alloc = dvz.dvz_mesh_alloc
mesh_alloc.__doc__ = """
Allocate memory for a visual.

Parameters
----------
visual : DvzVisual*
    the visual
vertex_count : int
    the number of vertices
index_count : int
    the number of indices
"""
mesh_alloc.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t vertex_count
    ctypes.c_uint32,  # uint32_t index_count
]


# -------------------------------------------------------------------------------------------------
mesh_light_pos = dvz.dvz_mesh_light_pos
mesh_light_pos.__doc__ = """
Set the light direction.

Parameters
----------
visual : DvzVisual*
    the mesh
idx : int
    the light index (0, 1, 2, or 3)
pos : Tuple[float, float, float, float]
    the light position (w=0 indicates it is a direction.)
"""
mesh_light_pos.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t idx
    vec4,  # vec4 pos
]


# -------------------------------------------------------------------------------------------------
mesh_light_color = dvz.dvz_mesh_light_color
mesh_light_color.__doc__ = """
Set the light color.

Parameters
----------
visual : DvzVisual*
    the mesh
idx : int
    the light index (0, 1, 2, or 3)
rgba : Tuple[int, int, int, int]
    the light color (a>0 indicates light is on.)
"""
mesh_light_color.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t idx
    DvzColor,  # DvzColor rgba
]


# -------------------------------------------------------------------------------------------------
mesh_material_params = dvz.dvz_mesh_material_params
mesh_material_params.__doc__ = """
Set the mesh material parameters.

Parameters
----------
visual : DvzVisual*
    the mesh
idx : int
    the material index (0, 1, 2, or 3) for (ambient, diffuse, specular, exponent)
params : Tuple[float, float, float]
    the material parameters (vec3 r, g, b)
"""
mesh_material_params.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t idx
    vec3,  # vec3 params
]


# -------------------------------------------------------------------------------------------------
mesh_shine = dvz.dvz_mesh_shine
mesh_shine.__doc__ = """
Set the mesh surface shine level.

Parameters
----------
visual : DvzVisual*
    the mesh
shine : float
    the surface shininess
"""
mesh_shine.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_float,  # float shine
]


# -------------------------------------------------------------------------------------------------
mesh_emit = dvz.dvz_mesh_emit
mesh_emit.__doc__ = """
Set the mesh surface emission level.

Parameters
----------
visual : DvzVisual*
    the mesh
emit : float
    the emission level
"""
mesh_emit.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_float,  # float emit
]


# -------------------------------------------------------------------------------------------------
mesh_edgecolor = dvz.dvz_mesh_edgecolor
mesh_edgecolor.__doc__ = """
Set the marker edge color.
Note: the alpha component is currently unused.

Parameters
----------
visual : DvzVisual*
    the mesh
rgba : Tuple[int, int, int, int]
    the rgba components
"""
mesh_edgecolor.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzColor,  # DvzColor rgba
]


# -------------------------------------------------------------------------------------------------
mesh_linewidth = dvz.dvz_mesh_linewidth
mesh_linewidth.__doc__ = """
Set the mesh contour linewidth (wireframe or isoline).

Parameters
----------
visual : DvzVisual*
    the mesh
linewidth : float
    the line width
"""
mesh_linewidth.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_float,  # float linewidth
]


# -------------------------------------------------------------------------------------------------
mesh_density = dvz.dvz_mesh_density
mesh_density.__doc__ = """
Set the number of isolines

Parameters
----------
visual : DvzVisual*
    the mesh
count : int
    the number of isolines
"""
mesh_density.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t count
]


# -------------------------------------------------------------------------------------------------
mesh_shape = dvz.dvz_mesh_shape
mesh_shape.__doc__ = """
Create a mesh out of a shape.

Parameters
----------
batch : DvzBatch*
    the batch
shape : DvzShape*
    the shape
flags : int
    the visual creation flags

Returns
-------
result : DvzVisual*
     the mesh
"""
mesh_shape.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    ctypes.c_int,  # int flags
]
mesh_shape.restype = ctypes.POINTER(DvzVisual)


# -------------------------------------------------------------------------------------------------
mesh_reshape = dvz.dvz_mesh_reshape
mesh_reshape.__doc__ = """
Update a mesh once a shape has been updated.

Parameters
----------
visual : DvzVisual*
    the mesh
shape : DvzShape*
    the shape
"""
mesh_reshape.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.POINTER(DvzShape),  # DvzShape* shape
]


# -------------------------------------------------------------------------------------------------
sphere = dvz.dvz_sphere
sphere.__doc__ = """
Create a sphere visual.

Parameters
----------
batch : DvzBatch*
    the batch
flags : int
    the visual creation flags

Returns
-------
result : DvzVisual*
     the visual
"""
sphere.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_int,  # int flags
]
sphere.restype = ctypes.POINTER(DvzVisual)


# -------------------------------------------------------------------------------------------------
sphere_position = dvz.dvz_sphere_position
sphere_position.__doc__ = """
Set the sphere positions.

Parameters
----------
visual : DvzVisual*
    the sphere
first : int
    the index of the first item to update
count : int
    the number of items to update
pos : np.ndarray[vec3]
    the 3D positions of the sphere centers
flags : int
    the data update flags
"""
sphere_position.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* pos
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
sphere_color = dvz.dvz_sphere_color
sphere_color.__doc__ = """
Set the sphere colors.

Parameters
----------
visual : DvzVisual*
    the sphere
first : int
    the index of the first item to update
count : int
    the number of items to update
color : DvzColor*
    the sphere colors
flags : int
    the data update flags
"""
sphere_color.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # DvzColor* color
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
sphere_size = dvz.dvz_sphere_size
sphere_size.__doc__ = """
Set the sphere sizes.

Parameters
----------
visual : DvzVisual*
    the sphere
first : int
    the index of the first item to update
count : int
    the number of items to update
size : np.ndarray[float]
    the radius of the spheres
flags : int
    the data update flags
"""
sphere_size.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # float* size
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
sphere_alloc = dvz.dvz_sphere_alloc
sphere_alloc.__doc__ = """
Allocate memory for a visual.

Parameters
----------
visual : DvzVisual*
    the sphere
item_count : int
    the total number of spheres to allocate for this visual
"""
sphere_alloc.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t item_count
]


# -------------------------------------------------------------------------------------------------
sphere_texture = dvz.dvz_sphere_texture
sphere_texture.__doc__ = """
Assign a 2D texture to a sphere visual.

Parameters
----------
visual : DvzVisual*
    the visual
texture : DvzTexture*
    the texture
"""
sphere_texture.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.POINTER(DvzTexture),  # DvzTexture* texture
]


# -------------------------------------------------------------------------------------------------
sphere_light_pos = dvz.dvz_sphere_light_pos
sphere_light_pos.__doc__ = """
Set the sphere light position.

Parameters
----------
visual : DvzVisual*
    the sphere
idx : int
    the light index (0, 1, 2, or 3)
pos : Tuple[float, float, float, float]
    the light position (w=0 indicates it is a direction.)
"""
sphere_light_pos.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t idx
    vec4,  # vec4 pos
]


# -------------------------------------------------------------------------------------------------
sphere_light_color = dvz.dvz_sphere_light_color
sphere_light_color.__doc__ = """
Set the light color.

Parameters
----------
visual : DvzVisual*
    the sphere
idx : int
    the light index (0, 1, 2, or 3)
rgba : Tuple[int, int, int, int]
    the light color (a>0 indicates light is on.)
"""
sphere_light_color.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t idx
    DvzColor,  # DvzColor rgba
]


# -------------------------------------------------------------------------------------------------
sphere_material_params = dvz.dvz_sphere_material_params
sphere_material_params.__doc__ = """
Set the sphere material parameters.

Parameters
----------
visual : DvzVisual*
    the sphere
idx : int
    the material index (0, 1, 2, or 3) for (ambient, diffuse, specular, exponent)
params : Tuple[float, float, float]
    the material parameters (vec3 r, g, b)
"""
sphere_material_params.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t idx
    vec3,  # vec3 params
]


# -------------------------------------------------------------------------------------------------
sphere_shine = dvz.dvz_sphere_shine
sphere_shine.__doc__ = """
Set the sphere surface shininess.

Parameters
----------
visual : DvzVisual*
    the sphere
shine : float
    the surface shininess
"""
sphere_shine.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_float,  # float shine
]


# -------------------------------------------------------------------------------------------------
sphere_emit = dvz.dvz_sphere_emit
sphere_emit.__doc__ = """
Set the sphere surface emission level.

Parameters
----------
visual : DvzVisual*
    the sphere
emit : float
    the emission level
"""
sphere_emit.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_float,  # float emit
]


# -------------------------------------------------------------------------------------------------
volume = dvz.dvz_volume
volume.__doc__ = """
Create a volume visual.

Parameters
----------
batch : DvzBatch*
    the batch
flags : int
    the visual creation flags

Returns
-------
result : DvzVisual*
     the visual
"""
volume.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_int,  # int flags
]
volume.restype = ctypes.POINTER(DvzVisual)


# -------------------------------------------------------------------------------------------------
volume_texture = dvz.dvz_volume_texture
volume_texture.__doc__ = """
Assign a 3D texture to a volume visual.

Parameters
----------
visual : DvzVisual*
    the visual
texture : DvzTexture*
    the 3D texture
"""
volume_texture.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.POINTER(DvzTexture),  # DvzTexture* texture
]


# -------------------------------------------------------------------------------------------------
volume_bounds = dvz.dvz_volume_bounds
volume_bounds.__doc__ = """
Set the volume bounds.

Parameters
----------
visual : DvzVisual*
    the visual
xlim : Tuple[float, float]
    xmin and xmax
ylim : Tuple[float, float]
    ymin and ymax
zlim : Tuple[float, float]
    zmin and zmax
"""
volume_bounds.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    vec2,  # vec2 xlim
    vec2,  # vec2 ylim
    vec2,  # vec2 zlim
]


# -------------------------------------------------------------------------------------------------
volume_texcoords = dvz.dvz_volume_texcoords
volume_texcoords.__doc__ = """
Set the texture coordinates of two corner points.

Parameters
----------
visual : DvzVisual*
    the visual
uvw0 : Tuple[float, float, float]
    coordinates of one of the corner points
uvw1 : Tuple[float, float, float]
    coordinates of one of the corner points
"""
volume_texcoords.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    vec3,  # vec3 uvw0
    vec3,  # vec3 uvw1
]


# -------------------------------------------------------------------------------------------------
volume_permutation = dvz.dvz_volume_permutation
volume_permutation.__doc__ = """
Set the texture coordinates index permutation.

Parameters
----------
visual : DvzVisual*
    the visual
ijk : Tuple[int, int, int]
    index permutation
"""
volume_permutation.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ivec3,  # ivec3 ijk
]


# -------------------------------------------------------------------------------------------------
volume_slice = dvz.dvz_volume_slice
volume_slice.__doc__ = """
Set the bounding box face index on which to slice (showing the texture itself).

Parameters
----------
visual : DvzVisual*
    the visual
face_index : int
    -1 to disable, or the face index between 0 and 5 included
"""
volume_slice.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_int32,  # int32_t face_index
]


# -------------------------------------------------------------------------------------------------
volume_transfer = dvz.dvz_volume_transfer
volume_transfer.__doc__ = """
Set the volume size.

Parameters
----------
visual : DvzVisual*
    the visual
transfer : Tuple[float, float, float, float]
    transfer function, for now `vec4(x, 0, 0, 0)` where x is a scaling factor
"""
volume_transfer.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    vec4,  # vec4 transfer
]


# -------------------------------------------------------------------------------------------------
slice = dvz.dvz_slice
slice.__doc__ = """
Create a slice visual (multiple 2D images with slices of a 3D texture).

Parameters
----------
batch : DvzBatch*
    the batch
flags : int
    the visual creation flags

Returns
-------
result : DvzVisual*
     the visual
"""
slice.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_int,  # int flags
]
slice.restype = ctypes.POINTER(DvzVisual)


# -------------------------------------------------------------------------------------------------
slice_position = dvz.dvz_slice_position
slice_position.__doc__ = """
Set the slice positions.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
p0 : np.ndarray[vec3]
    the 3D positions of the top left corner
p1 : np.ndarray[vec3]
    the 3D positions of the top right corner
p2 : np.ndarray[vec3]
    the 3D positions of the bottom left corner
p3 : np.ndarray[vec3]
    the 3D positions of the bottom right corner
flags : int
    the data update flags
"""
slice_position.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* p0
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* p1
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* p2
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* p3
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
slice_texcoords = dvz.dvz_slice_texcoords
slice_texcoords.__doc__ = """
Set the slice texture coordinates.

Parameters
----------
visual : DvzVisual*
    the visual
first : int
    the index of the first item to update
count : int
    the number of items to update
uvw0 : np.ndarray[vec3]
    the 3D texture coordinates of the top left corner
uvw1 : np.ndarray[vec3]
    the 3D texture coordinates of the top right corner
uvw2 : np.ndarray[vec3]
    the 3D texture coordinates of the bottom left corner
uvw3 : np.ndarray[vec3]
    the 3D texture coordinates of the bottom right corner
flags : int
    the data update flags
"""
slice_texcoords.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* uvw0
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* uvw1
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* uvw2
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* uvw3
    ctypes.c_int,  # int flags
]


# -------------------------------------------------------------------------------------------------
slice_texture = dvz.dvz_slice_texture
slice_texture.__doc__ = """
Assign a texture to a slice visual.

Parameters
----------
visual : DvzVisual*
    the visual
texture : DvzTexture*
    the texture
"""
slice_texture.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.POINTER(DvzTexture),  # DvzTexture* texture
]


# -------------------------------------------------------------------------------------------------
slice_alloc = dvz.dvz_slice_alloc
slice_alloc.__doc__ = """
Allocate memory for a visual.

Parameters
----------
visual : DvzVisual*
    the visual
item_count : int
    the total number of slices to allocate for this visual
"""
slice_alloc.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t item_count
]


# -------------------------------------------------------------------------------------------------
tex_slice = dvz.dvz_tex_slice
tex_slice.__doc__ = """
Create a 3D texture to be used in a slice visual.

Parameters
----------
batch : DvzBatch*
    the batch
format : DvzFormat
    the texture format
width : int
    the texture width
height : int
    the texture height
depth : int
    the texture depth
data : np.ndarray
    the texture data to upload

Returns
-------
result : DvzId
     the texture ID
"""
tex_slice.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzFormat,  # DvzFormat format
    ctypes.c_uint32,  # uint32_t width
    ctypes.c_uint32,  # uint32_t height
    ctypes.c_uint32,  # uint32_t depth
    ndpointer(dtype=None, ndim=None, flags="C_CONTIGUOUS"),  # void* data
]
tex_slice.restype = DvzId


# -------------------------------------------------------------------------------------------------
slice_alpha = dvz.dvz_slice_alpha
slice_alpha.__doc__ = """
Set the slice transparency alpha value.

Parameters
----------
visual : DvzVisual*
    the visual
alpha : float
    the alpha value
"""
slice_alpha.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_float,  # float alpha
]


# -------------------------------------------------------------------------------------------------
grid_color = dvz.dvz_grid_color
grid_color.__doc__ = """
Set the grid line color.

Parameters
----------
grid : DvzVisual*
    the grid visual
value : Tuple[float, float, float, float]
    RGBA color of fine lines
"""
grid_color.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* grid
    vec4,  # vec4 value
]


# -------------------------------------------------------------------------------------------------
grid_linewidth = dvz.dvz_grid_linewidth
grid_linewidth.__doc__ = """
Set the line width.

Parameters
----------
grid : DvzVisual*
    the grid visual
value : float
    width of lines (in world units)
"""
grid_linewidth.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* grid
    ctypes.c_float,  # float value
]


# -------------------------------------------------------------------------------------------------
grid_scale = dvz.dvz_grid_scale
grid_scale.__doc__ = """
Set the grid spacing.

Parameters
----------
grid : DvzVisual*
    the grid visual
value : float
    spacing between grid lines (in world units)
"""
grid_scale.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* grid
    ctypes.c_float,  # float value
]


# -------------------------------------------------------------------------------------------------
grid_elevation = dvz.dvz_grid_elevation
grid_elevation.__doc__ = """
Set the grid elevation on the Y axis.

Parameters
----------
grid : DvzVisual*
    the grid visual
value : float
    grid elevation
"""
grid_elevation.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* grid
    ctypes.c_float,  # float value
]


DVZ_FORMAT_COLOR = FORMAT_R8G8B8A8_UNORM

# ===============================================================================
# Error handling
# ===============================================================================


class DatovizError(Exception):
    pass


@DvzErrorCallback
def error_handler(message):
    raise DatovizError(message.decode('utf-8'))


error_callback(error_handler)
