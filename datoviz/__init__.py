"""WARNING: DO NOT EDIT: automatically-generated file"""

__version__ = "0.2.1"


# ===============================================================================
# Imports
# ===============================================================================

import ctypes
from ctypes import POINTER as P_
import faulthandler
import os
import pathlib
import platform

from enum import IntEnum

try:
    import numpy as np
    from numpy import float32
    from numpy.ctypeslib import as_ctypes_type as _ctype
    from numpy.ctypeslib import ndpointer as ndpointer_
except ImportError:
    float32 = object
    raise ImportError("NumPy is not available")


# ===============================================================================
# Fault handler
# ===============================================================================

faulthandler.enable()


# ===============================================================================
# Global variables
# ===============================================================================

PLATFORMS = {
    "Linux": "linux",
    "Darwin": "macos",
    "Windows": "windows",
}
PLATFORM = PLATFORMS.get(platform.system(), None)

LIB_NAMES = {
    "linux": "libdatoviz.so",
    "macos": "libdatoviz.dylib",
    "windows": "libdatoviz.dll",
}
LIB_NAME = LIB_NAMES.get(PLATFORM, "")

FILE_DIR = pathlib.Path(__file__).parent.resolve()

# Package paths: this Python file is stored alongside the dynamic libraries.
DATOVIZ_DIR = FILE_DIR
LIB_DIR = FILE_DIR
LIB_PATH = DATOVIZ_DIR / LIB_NAME

# Development paths: the libraries are in build/ and libs/
if not LIB_PATH.exists():
    DATOVIZ_DIR = (FILE_DIR / "../build/").resolve()
    LIB_DIR = (FILE_DIR / f"../libs/vulkan/{PLATFORM}/").resolve()
    LIB_PATH = DATOVIZ_DIR / LIB_NAME

if not LIB_PATH.exists():
    raise RuntimeError(f"Unable to find `{LIB_PATH}`.")


# ===============================================================================
# Loading the dynamic library
# ===============================================================================

assert LIB_PATH.exists()
try:
    dvz = ctypes.cdll.LoadLibrary(LIB_PATH)
except Exception as e:
    print(f"Error loading {LIB_PATH}: {e}")
    class DVZ:
        def __getattr__(self, k):
            return DVZ()

        def __setattr__(self, k, v):
            pass
    dvz = DVZ()

# on macOS, we need to set the VK_DRIVER_FILES environment variable to the path to the MoltenVK ICD
if PLATFORM == "macos":
    os.environ['VK_DRIVER_FILES'] = str(LIB_DIR / "MoltenVK_icd.json")


# ===============================================================================
# Util classes
# ===============================================================================

# see https://v4.chriskrycho.com/2015/ctypes-structures-and-dll-exports.html
class CtypesEnum(IntEnum):
    @classmethod
    def from_param(cls, obj):
        return int(obj)


class WrappedValue:
    def __init__(self, initial_value, ctype_type=ctypes.c_float):
        self._value = ctype_type(initial_value)
        self.python_value = initial_value

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.python_value = self._value.value

    @property
    def P_(self):
        return ctypes.byref(self._value)

    @property
    def value(self):
        return self._value.value

    @value.setter
    def value(self, new_value):
        self._value.value = new_value

    def __repr__(self):
        return str(self.value)


def array_pointer(x, dtype=None):
    if not isinstance(x, np.ndarray):
        return x
    dtype = dtype or x.dtype
    x = x.astype(dtype)
    return x.ctypes.data_as(P_(_ctype(dtype)))


# HACK: accept None ndarrays as arguments, see https://stackoverflow.com/a/37664693/1595060
def ndpointer(*args, **kwargs):
    ndim = kwargs.pop('ndim', 1)
    ncol = kwargs.pop('ncol', 1)
    base = ndpointer_(*args, **kwargs)

    @classmethod
    def from_param(cls, obj):
        if obj is None:
            return obj
        if isinstance(obj, np.ndarray):
            s = f"array <{obj.dtype}>{obj.shape}"
            if obj.ndim != ndim:
                raise ValueError(
                    f"Wrong ndim {obj.ndim} (expected {ndim}) for {s}")
            if ncol > 1 and obj.shape[1] != ncol:
                raise ValueError(
                    f"Wrong shape {obj.shape} (expected (*, {ncol})) for {s}")
            out = base.from_param(obj)
        else:
            # NOTE: allow passing ndpointers without change
            out = obj
        return out
    return type(base.__name__, (base,), {'from_param': from_param})


def char_pointer(s):
    return str(s).encode('utf-8')


def vec2(x: float = 0, y: float = 0):
    return (ctypes.c_float * 2)(x, y)


def vec3(x: float = 0, y: float = 0, z: float = 0):
    return (ctypes.c_float * 3)(x, y, z)


def vec4(x: float = 0, y: float = 0, z: float = 0, w: float = 0):
    return (ctypes.c_float * 4)(x, y, z, w)


def cvec4(r: int = 0, g: int = 0, b: int = 0, a: int = 0):
    return (ctypes.c_uint8 * 4)(r, g, b, a)


# ===============================================================================
# Aliases
# ===============================================================================

DvzId = ctypes.c_uint64

A_ = array_pointer
S_ = char_pointer
V_ = WrappedValue


# ===============================================================================
# DEFINES
# ===============================================================================

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
M_PI = 3.141592653589793
M_2PI = 6.283185307179586
M_PI2 = 1.5707963267948966
M_INV_255 = 0.00392156862745098
EPSILON = 1e-10
GB = 1073741824
MB = 1048576
KB = 1024
DVZ_VERSION_MINOR = 2
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
    DVZ_CANVAS_FLAGS_VSYNC = 0x0010
    DVZ_CANVAS_FLAGS_PICK = 0x0020


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


class DvzDialogFlags(CtypesEnum):
    DVZ_DIALOG_FLAGS_NONE = 0x0000
    DVZ_DIALOG_FLAGS_OVERLAY = 0x0001


class DvzCorner(CtypesEnum):
    DVZ_DIALOG_CORNER_TOP_LEFT = 0
    DVZ_DIALOG_CORNER_TOP_RIGHT = 1
    DVZ_DIALOG_CORNER_BOTTOM_LEFT = 2
    DVZ_DIALOG_CORNER_BOTTOM_RIGHT = 3


class DvzArcballFlags(CtypesEnum):
    DVZ_ARCBALL_FLAGS_NONE = 0
    DVZ_ARCBALL_FLAGS_CONSTRAIN = 1


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
    DVZ_VISUAL_FLAGS_VERTEX_MAPPABLE = 0x400000
    DVZ_VISUAL_FLAGS_INDEX_MAPPABLE = 0x800000


class DvzViewFlags(CtypesEnum):
    DVZ_VIEW_FLAGS_NONE = 0x0000
    DVZ_VIEW_FLAGS_STATIC = 0x0001


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
    DVZ_IMAGE_FLAGS_FILL = 0x0010


class DvzShapeType(CtypesEnum):
    DVZ_SHAPE_NONE = 0
    DVZ_SHAPE_SQUARE = 1
    DVZ_SHAPE_DISC = 2
    DVZ_SHAPE_POLYGON = 3
    DVZ_SHAPE_CUBE = 4
    DVZ_SHAPE_SPHERE = 5
    DVZ_SHAPE_CYLINDER = 6
    DVZ_SHAPE_CONE = 7
    DVZ_SHAPE_SURFACE = 8
    DVZ_SHAPE_OBJ = 9
    DVZ_SHAPE_OTHER = 10


class DvzContourFlags(CtypesEnum):
    DVZ_CONTOUR_NONE = 0x00
    DVZ_CONTOUR_EDGES = 0x01
    DVZ_CONTOUR_JOINTS = 0x02
    DVZ_CONTOUR_FULL = 0xF0


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


class DvzViewportClip(CtypesEnum):
    DVZ_VIEWPORT_CLIP_INNER = 0x0001
    DVZ_VIEWPORT_CLIP_OUTER = 0x0002
    DVZ_VIEWPORT_CLIP_BOTTOM = 0x0004
    DVZ_VIEWPORT_CLIP_LEFT = 0x0008


class DvzDepthTest(CtypesEnum):
    DVZ_DEPTH_TEST_DISABLE = 0
    DVZ_DEPTH_TEST_ENABLE = 1


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


# Function aliases

APP_FLAGS_NONE = 0x000000
APP_FLAGS_OFFSCREEN = 0x008000
APP_FLAGS_WHITE_BACKGROUND = 0x100000
CANVAS_FLAGS_NONE = 0x0000
CANVAS_FLAGS_IMGUI = 0x0001
CANVAS_FLAGS_FPS = 0x0003
CANVAS_FLAGS_MONITOR = 0x0005
CANVAS_FLAGS_VSYNC = 0x0010
CANVAS_FLAGS_PICK = 0x0020
KEY_MODIFIER_NONE = 0x00000000
KEY_MODIFIER_SHIFT = 0x00000001
KEY_MODIFIER_CONTROL = 0x00000002
KEY_MODIFIER_ALT = 0x00000004
KEY_MODIFIER_SUPER = 0x00000008
KEYBOARD_EVENT_NONE = 0
KEYBOARD_EVENT_PRESS = 1
KEYBOARD_EVENT_REPEAT = 2
KEYBOARD_EVENT_RELEASE = 3
MOUSE_BUTTON_NONE = 0
MOUSE_BUTTON_LEFT = 1
MOUSE_BUTTON_MIDDLE = 2
MOUSE_BUTTON_RIGHT = 3
MOUSE_STATE_RELEASE = 0
MOUSE_STATE_PRESS = 1
MOUSE_STATE_CLICK = 3
MOUSE_STATE_CLICK_PRESS = 4
MOUSE_STATE_DOUBLE_CLICK = 5
MOUSE_STATE_DRAGGING = 11
MOUSE_EVENT_RELEASE = 0
MOUSE_EVENT_PRESS = 1
MOUSE_EVENT_MOVE = 2
MOUSE_EVENT_CLICK = 3
MOUSE_EVENT_DOUBLE_CLICK = 5
MOUSE_EVENT_DRAG_START = 10
MOUSE_EVENT_DRAG = 11
MOUSE_EVENT_DRAG_STOP = 12
MOUSE_EVENT_WHEEL = 20
MOUSE_EVENT_ALL = 255
DIALOG_FLAGS_NONE = 0x0000
DIALOG_FLAGS_OVERLAY = 0x0001
DIALOG_CORNER_TOP_LEFT = 0
DIALOG_CORNER_TOP_RIGHT = 1
DIALOG_CORNER_BOTTOM_LEFT = 2
DIALOG_CORNER_BOTTOM_RIGHT = 3
ARCBALL_FLAGS_NONE = 0
ARCBALL_FLAGS_CONSTRAIN = 1
PANZOOM_FLAGS_NONE = 0x00
PANZOOM_FLAGS_KEEP_ASPECT = 0x01
PANZOOM_FLAGS_FIXED_X = 0x10
PANZOOM_FLAGS_FIXED_Y = 0x20
CAMERA_FLAGS_PERSPECTIVE = 0x00
CAMERA_FLAGS_ORTHO = 0x01
VISUAL_FLAGS_DEFAULT = 0x000000
VISUAL_FLAGS_INDEXED = 0x010000
VISUAL_FLAGS_INDIRECT = 0x020000
VISUAL_FLAGS_VERTEX_MAPPABLE = 0x400000
VISUAL_FLAGS_INDEX_MAPPABLE = 0x800000
VIEW_FLAGS_NONE = 0x0000
VIEW_FLAGS_STATIC = 0x0001
DAT_FLAGS_NONE = 0x0000
DAT_FLAGS_STANDALONE = 0x0100
DAT_FLAGS_MAPPABLE = 0x0200
DAT_FLAGS_DUP = 0x0400
DAT_FLAGS_KEEP_ON_RESIZE = 0x1000
DAT_FLAGS_PERSISTENT_STAGING = 0x2000
UPLOAD_FLAGS_NOCOPY = 0x0800
TEX_FLAGS_NONE = 0x0000
TEX_FLAGS_PERSISTENT_STAGING = 0x2000
FONT_FLAGS_RGB = 0
FONT_FLAGS_RGBA = 1
MOCK_FLAGS_NONE = 0x00
MOCK_FLAGS_CLOSED = 0x01
PRIMITIVE_TOPOLOGY_POINT_LIST = 0
PRIMITIVE_TOPOLOGY_LINE_LIST = 1
PRIMITIVE_TOPOLOGY_LINE_STRIP = 2
PRIMITIVE_TOPOLOGY_TRIANGLE_LIST = 3
PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP = 4
PRIMITIVE_TOPOLOGY_TRIANGLE_FAN = 5
FORMAT_NONE = 0
FORMAT_R8_UNORM = 9
FORMAT_R8_SNORM = 10
FORMAT_R8_UINT = 13
FORMAT_R8_SINT = 14
FORMAT_R8G8_UNORM = 16
FORMAT_R8G8_SNORM = 17
FORMAT_R8G8_UINT = 20
FORMAT_R8G8_SINT = 21
FORMAT_R8G8B8_UNORM = 23
FORMAT_R8G8B8_SNORM = 24
FORMAT_R8G8B8_UINT = 27
FORMAT_R8G8B8_SINT = 28
FORMAT_R8G8B8A8_UNORM = 37
FORMAT_R8G8B8A8_SNORM = 38
FORMAT_R8G8B8A8_UINT = 41
FORMAT_R8G8B8A8_SINT = 42
FORMAT_B8G8R8A8_UNORM = 44
FORMAT_R16_UNORM = 70
FORMAT_R16_SNORM = 71
FORMAT_R32_UINT = 98
FORMAT_R32_SINT = 99
FORMAT_R32_SFLOAT = 100
FORMAT_R32G32_UINT = 101
FORMAT_R32G32_SINT = 102
FORMAT_R32G32_SFLOAT = 103
FORMAT_R32G32B32_UINT = 104
FORMAT_R32G32B32_SINT = 105
FORMAT_R32G32B32_SFLOAT = 106
FORMAT_R32G32B32A32_UINT = 107
FORMAT_R32G32B32A32_SINT = 108
FORMAT_R32G32B32A32_SFLOAT = 109
FILTER_NEAREST = 0
FILTER_LINEAR = 1
FILTER_CUBIC_IMG = 1000015000
SAMPLER_ADDRESS_MODE_REPEAT = 0
SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT = 1
SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE = 2
SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER = 3
SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE = 4
MARKER_SHAPE_DISC = 0
MARKER_SHAPE_ASTERISK = 1
MARKER_SHAPE_CHEVRON = 2
MARKER_SHAPE_CLOVER = 3
MARKER_SHAPE_CLUB = 4
MARKER_SHAPE_CROSS = 5
MARKER_SHAPE_DIAMOND = 6
MARKER_SHAPE_ARROW = 7
MARKER_SHAPE_ELLIPSE = 8
MARKER_SHAPE_HBAR = 9
MARKER_SHAPE_HEART = 10
MARKER_SHAPE_INFINITY = 11
MARKER_SHAPE_PIN = 12
MARKER_SHAPE_RING = 13
MARKER_SHAPE_SPADE = 14
MARKER_SHAPE_SQUARE = 15
MARKER_SHAPE_TAG = 16
MARKER_SHAPE_TRIANGLE = 17
MARKER_SHAPE_VBAR = 18
MARKER_SHAPE_ROUNDED_RECT = 19
MARKER_SHAPE_COUNT = 20
MARKER_MODE_NONE = 0
MARKER_MODE_CODE = 1
MARKER_MODE_BITMAP = 2
MARKER_MODE_SDF = 3
MARKER_MODE_MSDF = 4
MARKER_MODE_MTSDF = 5
MARKER_ASPECT_FILLED = 0
MARKER_ASPECT_STROKE = 1
MARKER_ASPECT_OUTLINE = 2
CAP_NONE = 0
CAP_ROUND = 1
CAP_TRIANGLE_IN = 2
CAP_TRIANGLE_OUT = 3
CAP_SQUARE = 4
CAP_BUTT = 5
CAP_COUNT = 6
JOIN_SQUARE = 0
JOIN_ROUND = 1
PATH_FLAGS_OPEN = 0
PATH_FLAGS_CLOSED = 1
IMAGE_FLAGS_SIZE_PIXELS = 0x0000
IMAGE_FLAGS_SIZE_NDC = 0x0001
IMAGE_FLAGS_RESCALE_KEEP_RATIO = 0x0004
IMAGE_FLAGS_RESCALE = 0x0008
IMAGE_FLAGS_FILL = 0x0010
SHAPE_NONE = 0
SHAPE_SQUARE = 1
SHAPE_DISC = 2
SHAPE_POLYGON = 3
SHAPE_CUBE = 4
SHAPE_SPHERE = 5
SHAPE_CYLINDER = 6
SHAPE_CONE = 7
SHAPE_SURFACE = 8
SHAPE_OBJ = 9
SHAPE_OTHER = 10
CONTOUR_NONE = 0x00
CONTOUR_EDGES = 0x01
CONTOUR_JOINTS = 0x02
CONTOUR_FULL = 0xF0
MESH_FLAGS_NONE = 0x0000
MESH_FLAGS_TEXTURED = 0x0001
MESH_FLAGS_LIGHTING = 0x0002
MESH_FLAGS_CONTOUR = 0x0004
MESH_FLAGS_ISOLINE = 0x0008
VOLUME_FLAGS_NONE = 0x0000
VOLUME_FLAGS_RGBA = 0x0001
VOLUME_FLAGS_COLORMAP = 0x0002
VOLUME_FLAGS_BACK_FRONT = 0x0004
EASING_NONE = 0
EASING_IN_SINE = 1
EASING_OUT_SINE = 2
EASING_IN_OUT_SINE = 3
EASING_IN_QUAD = 4
EASING_OUT_QUAD = 5
EASING_IN_OUT_QUAD = 6
EASING_IN_CUBIC = 7
EASING_OUT_CUBIC = 8
EASING_IN_OUT_CUBIC = 9
EASING_IN_QUART = 10
EASING_OUT_QUART = 11
EASING_IN_OUT_QUART = 12
EASING_IN_QUINT = 13
EASING_OUT_QUINT = 14
EASING_IN_OUT_QUINT = 15
EASING_IN_EXPO = 16
EASING_OUT_EXPO = 17
EASING_IN_OUT_EXPO = 18
EASING_IN_CIRC = 19
EASING_OUT_CIRC = 20
EASING_IN_OUT_CIRC = 21
EASING_IN_BACK = 22
EASING_OUT_BACK = 23
EASING_IN_OUT_BACK = 24
EASING_IN_ELASTIC = 25
EASING_OUT_ELASTIC = 26
EASING_IN_OUT_ELASTIC = 27
EASING_IN_BOUNCE = 28
EASING_OUT_BOUNCE = 29
EASING_IN_OUT_BOUNCE = 30
EASING_COUNT = 31
VIEWPORT_CLIP_INNER = 0x0001
VIEWPORT_CLIP_OUTER = 0x0002
VIEWPORT_CLIP_BOTTOM = 0x0004
VIEWPORT_CLIP_LEFT = 0x0008
DEPTH_TEST_DISABLE = 0
DEPTH_TEST_ENABLE = 1
CMAP_BINARY = 0
CMAP_HSV = 1
CMAP_CIVIDIS = 2
CMAP_INFERNO = 3
CMAP_MAGMA = 4
CMAP_PLASMA = 5
CMAP_VIRIDIS = 6
CMAP_BLUES = 7
CMAP_BUGN = 8
CMAP_BUPU = 9
CMAP_GNBU = 10
CMAP_GREENS = 11
CMAP_GREYS = 12
CMAP_ORANGES = 13
CMAP_ORRD = 14
CMAP_PUBU = 15
CMAP_PUBUGN = 16
CMAP_PURPLES = 17
CMAP_RDPU = 18
CMAP_REDS = 19
CMAP_YLGN = 20
CMAP_YLGNBU = 21
CMAP_YLORBR = 22
CMAP_YLORRD = 23
CMAP_AFMHOT = 24
CMAP_AUTUMN = 25
CMAP_BONE = 26
CMAP_COOL = 27
CMAP_COPPER = 28
CMAP_GIST_HEAT = 29
CMAP_GRAY = 30
CMAP_HOT = 31
CMAP_PINK = 32
CMAP_SPRING = 33
CMAP_SUMMER = 34
CMAP_WINTER = 35
CMAP_WISTIA = 36
CMAP_BRBG = 37
CMAP_BWR = 38
CMAP_COOLWARM = 39
CMAP_PIYG = 40
CMAP_PRGN = 41
CMAP_PUOR = 42
CMAP_RDBU = 43
CMAP_RDGY = 44
CMAP_RDYLBU = 45
CMAP_RDYLGN = 46
CMAP_SEISMIC = 47
CMAP_SPECTRAL = 48
CMAP_TWILIGHT_SHIFTED = 49
CMAP_TWILIGHT = 50
CMAP_BRG = 51
CMAP_CMRMAP = 52
CMAP_CUBEHELIX = 53
CMAP_FLAG = 54
CMAP_GIST_EARTH = 55
CMAP_GIST_NCAR = 56
CMAP_GIST_RAINBOW = 57
CMAP_GIST_STERN = 58
CMAP_GNUPLOT2 = 59
CMAP_GNUPLOT = 60
CMAP_JET = 61
CMAP_NIPY_SPECTRAL = 62
CMAP_OCEAN = 63
CMAP_PRISM = 64
CMAP_RAINBOW = 65
CMAP_TERRAIN = 66
CMAP_BKR = 67
CMAP_BKY = 68
CMAP_CET_D10 = 69
CMAP_CET_D11 = 70
CMAP_CET_D8 = 71
CMAP_CET_D13 = 72
CMAP_CET_D3 = 73
CMAP_CET_D1A = 74
CMAP_BJY = 75
CMAP_GWV = 76
CMAP_BWY = 77
CMAP_CET_D12 = 78
CMAP_CET_R3 = 79
CMAP_CET_D9 = 80
CMAP_CWR = 81
CMAP_CET_CBC1 = 82
CMAP_CET_CBC2 = 83
CMAP_CET_CBL1 = 84
CMAP_CET_CBL2 = 85
CMAP_CET_CBTC1 = 86
CMAP_CET_CBTC2 = 87
CMAP_CET_CBTL1 = 88
CMAP_BGY = 89
CMAP_BGYW = 90
CMAP_BMW = 91
CMAP_CET_C1 = 92
CMAP_CET_C1S = 93
CMAP_CET_C2 = 94
CMAP_CET_C4 = 95
CMAP_CET_C4S = 96
CMAP_CET_C5 = 97
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
CMAP_COLORWHEEL = 113
CMAP_FIRE = 114
CMAP_ISOLUM = 115
CMAP_KB = 116
CMAP_KBC = 117
CMAP_KG = 118
CMAP_KGY = 119
CMAP_KR = 120
CMAP_BLACK_BODY = 121
CMAP_KINDLMANN = 122
CMAP_EXTENDED_KINDLMANN = 123
CPAL256_GLASBEY = CPAL256_OFS
CPAL256_GLASBEY_COOL = 125
CPAL256_GLASBEY_DARK = 126
CPAL256_GLASBEY_HV = 127
CPAL256_GLASBEY_LIGHT = 128
CPAL256_GLASBEY_WARM = 129
CPAL032_ACCENT = CPAL032_OFS
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
CPAL032_CATEGORY10_10 = 142
CPAL032_CATEGORY20_20 = 143
CPAL032_CATEGORY20B_20 = 144
CPAL032_CATEGORY20C_20 = 145
CPAL032_COLORBLIND8 = 146
KEY_UNKNOWN = -1
KEY_NONE = +0
KEY_SPACE = 32
KEY_APOSTROPHE = 39
KEY_COMMA = 44
KEY_MINUS = 45
KEY_PERIOD = 46
KEY_SLASH = 47
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
KEY_SEMICOLON = 59
KEY_EQUAL = 61
KEY_A = 65
KEY_B = 66
KEY_C = 67
KEY_D = 68
KEY_E = 69
KEY_F = 70
KEY_G = 71
KEY_H = 72
KEY_I = 73
KEY_J = 74
KEY_K = 75
KEY_L = 76
KEY_M = 77
KEY_N = 78
KEY_O = 79
KEY_P = 80
KEY_Q = 81
KEY_R = 82
KEY_S = 83
KEY_T = 84
KEY_U = 85
KEY_V = 86
KEY_W = 87
KEY_X = 88
KEY_Y = 89
KEY_Z = 90
KEY_LEFT_BRACKET = 91
KEY_BACKSLASH = 92
KEY_RIGHT_BRACKET = 93
KEY_GRAVE_ACCENT = 96
KEY_WORLD_1 = 161
KEY_WORLD_2 = 162
KEY_ESCAPE = 256
KEY_ENTER = 257
KEY_TAB = 258
KEY_BACKSPACE = 259
KEY_INSERT = 260
KEY_DELETE = 261
KEY_RIGHT = 262
KEY_LEFT = 263
KEY_DOWN = 264
KEY_UP = 265
KEY_PAGE_UP = 266
KEY_PAGE_DOWN = 267
KEY_HOME = 268
KEY_END = 269
KEY_CAPS_LOCK = 280
KEY_SCROLL_LOCK = 281
KEY_NUM_LOCK = 282
KEY_PRINT_SCREEN = 283
KEY_PAUSE = 284
KEY_F1 = 290
KEY_F2 = 291
KEY_F3 = 292
KEY_F4 = 293
KEY_F5 = 294
KEY_F6 = 295
KEY_F7 = 296
KEY_F8 = 297
KEY_F9 = 298
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
KEY_F20 = 309
KEY_F21 = 310
KEY_F22 = 311
KEY_F23 = 312
KEY_F24 = 313
KEY_F25 = 314
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
KEY_KP_DECIMAL = 330
KEY_KP_DIVIDE = 331
KEY_KP_MULTIPLY = 332
KEY_KP_SUBTRACT = 333
KEY_KP_ADD = 334
KEY_KP_ENTER = 335
KEY_KP_EQUAL = 336
KEY_LEFT_SHIFT = 340
KEY_LEFT_CONTROL = 341
KEY_LEFT_ALT = 342
KEY_LEFT_SUPER = 343
KEY_RIGHT_SHIFT = 344
KEY_RIGHT_CONTROL = 345
KEY_RIGHT_ALT = 346
KEY_RIGHT_SUPER = 347
KEY_MENU = 348
KEY_LAST = 348


# ===============================================================================
# FORWARD DECLARATIONS
# ===============================================================================

class DvzApp(ctypes.Structure):
    pass


class DvzArcball(ctypes.Structure):
    pass


class DvzAtlas(ctypes.Structure):
    pass


class DvzBatch(ctypes.Structure):
    pass


class DvzCamera(ctypes.Structure):
    pass


class DvzCapType(ctypes.Structure):
    pass


class DvzFigure(ctypes.Structure):
    pass


class DvzFont(ctypes.Structure):
    pass


class DvzIndex(ctypes.Structure):
    pass


class DvzOrtho(ctypes.Structure):
    pass


class DvzPanel(ctypes.Structure):
    pass


class DvzPanzoom(ctypes.Structure):
    pass


class DvzScene(ctypes.Structure):
    pass


class DvzTex(ctypes.Structure):
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
    ]


class DvzMVP(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("model", ctypes.c_float * 16),
        ("view", ctypes.c_float * 16),
        ("proj", ctypes.c_float * 16),
    ]


class DvzShape(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("transform", ctypes.c_float * 16),
        ("first", ctypes.c_uint32),
        ("count", ctypes.c_uint32),
        ("type", ctypes.c_int32),
        ("vertex_count", ctypes.c_uint32),
        ("index_count", ctypes.c_uint32),
        ("pos", ctypes.POINTER(ctypes.c_float * 3)),
        ("normal", ctypes.POINTER(ctypes.c_float * 3)),
        ("color", ctypes.POINTER(ctypes.c_uint8 * 4)),
        ("texcoords", ctypes.POINTER(ctypes.c_float * 4)),
        ("isoline", ctypes.POINTER(ctypes.c_float)),
        ("d_left", ctypes.POINTER(ctypes.c_float * 3)),
        ("d_right", ctypes.POINTER(ctypes.c_float * 3)),
        ("contour", ctypes.POINTER(ctypes.c_uint8 * 4)),
        ("index", ctypes.POINTER(ctypes.c_uint32)),
    ]


class DvzKeyboardEvent(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("type", ctypes.c_int32),
        ("key", ctypes.c_int32),
        ("mods", ctypes.c_int),
        ("user_data", ctypes.c_void_p),
    ]


class DvzMouseButtonEvent(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("button", ctypes.c_int32),
    ]


class DvzMouseWheelEvent(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("dir", ctypes.c_float * 2),
    ]


class DvzMouseDragEvent(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("button", ctypes.c_int32),
        ("press_pos", ctypes.c_float * 2),
        ("shift", ctypes.c_float * 2),
    ]


class DvzMouseClickEvent(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("button", ctypes.c_int32),
    ]


class DvzMouseEventUnion(ctypes.Union):
    _pack_ = 8
    _fields_ = [
        ("b", DvzMouseButtonEvent),
        ("w", DvzMouseWheelEvent),
        ("d", DvzMouseDragEvent),
        ("c", DvzMouseClickEvent),
    ]


class DvzMouseEvent(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("type", ctypes.c_int32),
        ("content", DvzMouseEventUnion),
        ("pos", ctypes.c_float * 2),
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


class DvzRequestsEvent(ctypes.Structure):
    _pack_ = 8
    _fields_ = [
        ("batch", ctypes.POINTER(DvzBatch)),
        ("user_data", ctypes.c_void_p),
    ]


# Struct aliases

AtlasFont = DvzAtlasFont
MVP = DvzMVP
Shape = DvzShape
KeyboardEvent = DvzKeyboardEvent
MouseButtonEvent = DvzMouseButtonEvent
MouseWheelEvent = DvzMouseWheelEvent
MouseDragEvent = DvzMouseDragEvent
MouseClickEvent = DvzMouseClickEvent
MouseEventUnion = DvzMouseEventUnion
MouseEvent = DvzMouseEvent
WindowEvent = DvzWindowEvent
FrameEvent = DvzFrameEvent
GuiEvent = DvzGuiEvent
TimerEvent = DvzTimerEvent
RequestsEvent = DvzRequestsEvent


# ===============================================================================
# FUNCTION CALLBACK TYPES
# ===============================================================================

gui = DvzAppGuiCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, DvzGuiEvent)
mouse = DvzAppMouseCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, DvzMouseEvent)
keyboard = DvzAppKeyboardCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, DvzKeyboardEvent)
frame = DvzAppFrameCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, DvzFrameEvent)
timer = DvzAppTimerCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, DvzTimerEvent)
resize = DvzAppResizeCallback = ctypes.CFUNCTYPE(None, P_(DvzApp), DvzId, DvzWindowEvent)


# ===============================================================================
# FUNCTIONS
# ===============================================================================

# Function dvz_demo()
demo = dvz.dvz_demo
demo.__doc__ = """
Run a demo.
"""
demo.argtypes = [
]

# Function dvz_version()
version = dvz.dvz_version
version.__doc__ = """
Return the current version string.


Returns
-------
type
    the version string
"""
version.argtypes = [
]
version.restype = ctypes.c_char_p

# Function dvz_app()
app = dvz.dvz_app
app.__doc__ = """
Create an app.

Parameters
----------
flags : int
    the app creation flags

Returns
-------
type
    the app
"""
app.argtypes = [
    ctypes.c_int,  # int flags
]
app.restype = ctypes.POINTER(DvzApp)

# Function dvz_app_batch()
app_batch = dvz.dvz_app_batch
app_batch.__doc__ = """
Return the app batch.

Parameters
----------
app : DvzApp*
    the app

Returns
-------
type
    the batch
"""
app_batch.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
]
app_batch.restype = ctypes.POINTER(DvzBatch)

# Function dvz_app_frame()
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

# Function dvz_app_onframe()
app_onframe = dvz.dvz_app_onframe
app_onframe.__doc__ = """
Register a frame callback.

Parameters
----------
app : DvzApp*
    the app
callback : DvzAppFrameCallback
    the callback
user_data : void*
    the user data
"""
app_onframe.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
    DvzAppFrameCallback,  # DvzAppFrameCallback callback
    ctypes.c_void_p,  # void* user_data
]

# Function dvz_app_onmouse()
app_onmouse = dvz.dvz_app_onmouse
app_onmouse.__doc__ = """
Register a mouse callback.

Parameters
----------
app : DvzApp*
    the app
callback : DvzAppMouseCallback
    the callback
user_data : void*
    the user data
"""
app_onmouse.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
    DvzAppMouseCallback,  # DvzAppMouseCallback callback
    ctypes.c_void_p,  # void* user_data
]

# Function dvz_app_onkeyboard()
app_onkeyboard = dvz.dvz_app_onkeyboard
app_onkeyboard.__doc__ = """
Register a keyboard callback.

Parameters
----------
app : DvzApp*
    the app
callback : DvzAppKeyboardCallback
    the callback
user_data : void*
    the user data
"""
app_onkeyboard.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
    DvzAppKeyboardCallback,  # DvzAppKeyboardCallback callback
    ctypes.c_void_p,  # void* user_data
]

# Function dvz_app_onresize()
app_onresize = dvz.dvz_app_onresize
app_onresize.__doc__ = """
Register a resize callback.

Parameters
----------
app : DvzApp*
    the app
callback : DvzAppResizeCallback
    the callback
user_data : void*
    the user data
"""
app_onresize.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
    DvzAppResizeCallback,  # DvzAppResizeCallback callback
    ctypes.c_void_p,  # void* user_data
]

# Function dvz_app_timer()
app_timer = dvz.dvz_app_timer
app_timer.__doc__ = """
Create a timer.

Parameters
----------
app : DvzApp*
    the app
delay : double
    the delay, in seconds, until the first event
period : double
    the period, in seconds, between two events
max_count : uint64_t
    the maximum number of events

Returns
-------
type
    the timer
"""
app_timer.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
    ctypes.c_double,  # double delay
    ctypes.c_double,  # double period
    ctypes.c_uint64,  # uint64_t max_count
]
app_timer.restype = ctypes.POINTER(DvzTimerItem)

# Function dvz_app_ontimer()
app_ontimer = dvz.dvz_app_ontimer
app_ontimer.__doc__ = """
Register a timer callback.

Parameters
----------
app : DvzApp*
    the app
callback : DvzAppTimerCallback
    the timer callback
user_data : void*
    the user data
"""
app_ontimer.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
    DvzAppTimerCallback,  # DvzAppTimerCallback callback
    ctypes.c_void_p,  # void* user_data
]

# Function dvz_app_gui()
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
user_data : void*
    the user data
"""
app_gui.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
    DvzId,  # DvzId canvas_id
    DvzAppGuiCallback,  # DvzAppGuiCallback callback
    ctypes.c_void_p,  # void* user_data
]

# Function dvz_app_run()
app_run = dvz.dvz_app_run
app_run.__doc__ = """
Start the application event loop.

Parameters
----------
app : DvzApp*
    the app
n_frames : uint64_t
    the maximum number of frames, 0 for infinite loop
"""
app_run.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
    ctypes.c_uint64,  # uint64_t n_frames
]

# Function dvz_app_submit()
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

# Function dvz_app_screenshot()
app_screenshot = dvz.dvz_app_screenshot
app_screenshot.__doc__ = """
Make a screenshot of a canvas.

Parameters
----------
app : DvzApp*
    the app
canvas_id : DvzId
    the ID of the canvas
filename : char*
    the path to the PNG file with the screenshot
"""
app_screenshot.argtypes = [
    ctypes.POINTER(DvzApp),  # DvzApp* app
    DvzId,  # DvzId canvas_id
    ctypes.c_char_p,  # char* filename
]

# Function dvz_app_destroy()
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

# Function dvz_free()
free = dvz.dvz_free
free.__doc__ = """
Free a pointer.

Parameters
----------
pointer : void*
    a pointer
"""
free.argtypes = [
    ctypes.c_void_p,  # void* pointer
]

# Function dvz_scene()
scene = dvz.dvz_scene
scene.__doc__ = """
Create a scene.

Parameters
----------
batch : DvzBatch*
    the batch

Returns
-------
type
    the scene
"""
scene.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
]
scene.restype = ctypes.POINTER(DvzScene)

# Function dvz_scene_run()
scene_run = dvz.dvz_scene_run
scene_run.__doc__ = """
Start the event loop and render the scene in a window.

Parameters
----------
scene : DvzScene*
    the scene
app : DvzApp*
    the app
n_frames : uint64_t
    the maximum number of frames, 0 for infinite loop
"""
scene_run.argtypes = [
    ctypes.POINTER(DvzScene),  # DvzScene* scene
    ctypes.POINTER(DvzApp),  # DvzApp* app
    ctypes.c_uint64,  # uint64_t n_frames
]

# Function dvz_scene_destroy()
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

# Function dvz_figure()
figure = dvz.dvz_figure
figure.__doc__ = """
Create a figure, a desktop window with panels and visuals.

Parameters
----------
scene : DvzScene*
    the scene
width : uint32_t
    the window width
height : uint32_t
    the window height
flags : int
    the figure creation flags (not yet stabilized)

Returns
-------
type
    the figure
"""
figure.argtypes = [
    ctypes.POINTER(DvzScene),  # DvzScene* scene
    ctypes.c_uint32,  # uint32_t width
    ctypes.c_uint32,  # uint32_t height
    ctypes.c_int,  # int flags
]
figure.restype = ctypes.POINTER(DvzFigure)

# Function dvz_figure_id()
figure_id = dvz.dvz_figure_id
figure_id.__doc__ = """
Return a figure ID.

Parameters
----------
figure : DvzFigure*
    the figure

Returns
-------
type
    the figure ID
"""
figure_id.argtypes = [
    ctypes.POINTER(DvzFigure),  # DvzFigure* figure
]
figure_id.restype = DvzId

# Function dvz_figure_resize()
figure_resize = dvz.dvz_figure_resize
figure_resize.__doc__ = """
Resize a figure.

Parameters
----------
fig : DvzFigure*
    the figure
width : uint32_t
    the window width
height : uint32_t
    the window height
"""
figure_resize.argtypes = [
    ctypes.POINTER(DvzFigure),  # DvzFigure* fig
    ctypes.c_uint32,  # uint32_t width
    ctypes.c_uint32,  # uint32_t height
]

# Function dvz_scene_figure()
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
type
    the figure
"""
scene_figure.argtypes = [
    ctypes.POINTER(DvzScene),  # DvzScene* scene
    DvzId,  # DvzId id
]
scene_figure.restype = ctypes.POINTER(DvzFigure)

# Function dvz_figure_update()
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

# Function dvz_figure_destroy()
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

# Function dvz_mvp()
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

Returns
-------
type
    the MVP structure
"""
mvp.argtypes = [
    ctypes.c_float * 16,  # mat4 model
    ctypes.c_float * 16,  # mat4 view
    ctypes.c_float * 16,  # mat4 proj
]
mvp.restype = DvzMVP

# Function dvz_panel()
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
"""
panel.argtypes = [
    ctypes.POINTER(DvzFigure),  # DvzFigure* fig
    ctypes.c_float,  # float x
    ctypes.c_float,  # float y
    ctypes.c_float,  # float width
    ctypes.c_float,  # float height
]
panel.restype = ctypes.POINTER(DvzPanel)

# Function dvz_panel_default()
panel_default = dvz.dvz_panel_default
panel_default.__doc__ = """
Return the default full panel spanning an entire figure.

Parameters
----------
fig : DvzFigure*
    the figure

Returns
-------
type
    the panel spanning the entire figure
"""
panel_default.argtypes = [
    ctypes.POINTER(DvzFigure),  # DvzFigure* fig
]
panel_default.restype = ctypes.POINTER(DvzPanel)

# Function dvz_panel_transform()
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

# Function dvz_panel_mvp()
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

# Function dvz_panel_mvpmat()
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
    ctypes.c_float * 16,  # mat4 model
    ctypes.c_float * 16,  # mat4 view
    ctypes.c_float * 16,  # mat4 proj
]

# Function dvz_panel_resize()
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

# Function dvz_panel_margins()
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

# Function dvz_panel_contains()
panel_contains = dvz.dvz_panel_contains
panel_contains.__doc__ = """
Return whether a point is inside a panel.

Parameters
----------
panel : DvzPanel*
    the panel
pos : vec2
    the position

Returns
-------
type
    true if the position lies within the panel
"""
panel_contains.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
    ctypes.c_float * 2,  # vec2 pos
]
panel_contains.restype = ctypes.c_bool

# Function dvz_panel_at()
panel_at = dvz.dvz_panel_at
panel_at.__doc__ = """
Return the panel containing a given point.

Parameters
----------
figure : DvzFigure*
    the figure
pos : vec2
    the position

Returns
-------
type
    the panel containing the point, or NULL if there is none
"""
panel_at.argtypes = [
    ctypes.POINTER(DvzFigure),  # DvzFigure* figure
    ctypes.c_float * 2,  # vec2 pos
]
panel_at.restype = ctypes.POINTER(DvzPanel)

# Function dvz_panel_camera()
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
type
    the camera
"""
panel_camera.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
    ctypes.c_int,  # int flags
]
panel_camera.restype = ctypes.POINTER(DvzCamera)

# Function dvz_panel_panzoom()
panel_panzoom = dvz.dvz_panel_panzoom
panel_panzoom.__doc__ = """
Set panzoom interactivity for a panel.

Parameters
----------
panel : DvzPanel*
    the panel

Returns
-------
type
    the panzoom
"""
panel_panzoom.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
]
panel_panzoom.restype = ctypes.POINTER(DvzPanzoom)

# Function dvz_panel_ortho()
panel_ortho = dvz.dvz_panel_ortho
panel_ortho.__doc__ = """
Set ortho interactivity for a panel.

Parameters
----------
panel : DvzPanel*
    the panel

Returns
-------
type
    the ortho
"""
panel_ortho.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
]
panel_ortho.restype = ctypes.POINTER(DvzOrtho)

# Function dvz_panel_arcball()
panel_arcball = dvz.dvz_panel_arcball
panel_arcball.__doc__ = """
Set arcball interactivity for a panel.

Parameters
----------
panel : DvzPanel*
    the panel

Returns
-------
type
    the arcball
"""
panel_arcball.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
]
panel_arcball.restype = ctypes.POINTER(DvzArcball)

# Function dvz_panel_update()
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

# Function dvz_panel_visual()
panel_visual = dvz.dvz_panel_visual
panel_visual.__doc__ = """
Add a visual to a panel.

Parameters
----------
panel : DvzPanel*
    the panel
visual : DvzVisual*
    the visual
"""
panel_visual.argtypes = [
    ctypes.POINTER(DvzPanel),  # DvzPanel* panel
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_int,  # int flags
]

# Function dvz_panel_destroy()
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

# Function dvz_visual_update()
visual_update = dvz.dvz_visual_update
visual_update.__doc__ = """
Update a visual after its data has changed.

Parameters
----------
visual : DvzVisual*
    the visual
"""
visual_update.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
]

# Function dvz_visual_fixed()
visual_fixed = dvz.dvz_visual_fixed
visual_fixed.__doc__ = """
Fix some axes in a visual.

Parameters
----------
visual : DvzVisual*
    the visual
fixed_x : bool
    whether the x axis should be fixed
fixed_y : bool
    whether the y axis should be fixed
fixed_z : bool
    whether the z axis should be fixed
"""
visual_fixed.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_bool,  # bool fixed_x
    ctypes.c_bool,  # bool fixed_y
    ctypes.c_bool,  # bool fixed_z
]

# Function dvz_visual_clip()
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

# Function dvz_visual_depth()
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

# Function dvz_visual_show()
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

# Function dvz_colormap()
colormap = dvz.dvz_colormap
colormap.__doc__ = """
Fetch a color from a colormap and a value.

Parameters
----------
cmap : DvzColormap
    the colormap
value : uint8_t
    the value
color : cvec4 (out parameter)
    the fetched color
"""
colormap.argtypes = [
    DvzColormap,  # DvzColormap cmap
    ctypes.c_uint8,  # uint8_t value
    ctypes.c_uint8 * 4,  # cvec4 color
]

# Function dvz_colormap_scale()
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
color : cvec4 (out parameter)
    the fetched color
"""
colormap_scale.argtypes = [
    DvzColormap,  # DvzColormap cmap
    ctypes.c_float,  # float value
    ctypes.c_float,  # float vmin
    ctypes.c_float,  # float vmax
    ctypes.c_uint8 * 4,  # cvec4 color
]

# Function dvz_colormap_array()
colormap_array = dvz.dvz_colormap_array
colormap_array.__doc__ = """
Fetch colors from a colormap and an array of values.

Parameters
----------
cmap : DvzColormap
    the colormap
count : uint32_t
    the number of values
values : float*
    pointer to the array of float numbers
vmin : float
    the minimum value
vmax : float
    the maximum value
out : cvec4* (out parameter)
    the fetched colors
"""
colormap_array.argtypes = [
    DvzColormap,  # DvzColormap cmap
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # float* values
    ctypes.c_float,  # float vmin
    ctypes.c_float,  # float vmax
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # cvec4* out
]

# Function dvz_shape_normals()
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

# Function dvz_shape_merge()
shape_merge = dvz.dvz_shape_merge
shape_merge.__doc__ = """
Merge several shapes.

Parameters
----------
count : uint32_t
    the number of shapes to merge
shapes : DvzShape*
    the shapes to merge

Returns
-------
type
    the merged shape
"""
shape_merge.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    ctypes.POINTER(DvzShape),  # DvzShape* shapes
]
shape_merge.restype = DvzShape

# Function dvz_shape_print()
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

# Function dvz_shape_unindex()
shape_unindex = dvz.dvz_shape_unindex
shape_unindex.__doc__ = """
Convert an indexed shape to a non-indexed one by duplicating the vertex values according to the indices.  This is used by the mesh wireframe option, as a given vertex may have distinct barycentric coordinates depending on its index.

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

# Function dvz_shape_destroy()
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

# Function dvz_shape_begin()
shape_begin = dvz.dvz_shape_begin
shape_begin.__doc__ = """
Start a transformation sequence.

Parameters
----------
shape : DvzShape*
    the shape
first : uint32_t
    the first vertex to modify
count : uint32_t
    the number of vertices to modify
"""
shape_begin.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
]

# Function dvz_shape_scale()
shape_scale = dvz.dvz_shape_scale
shape_scale.__doc__ = """
Append a scaling transform to a shape.

Parameters
----------
shape : DvzShape*
    the shape
scale : vec3
    the scaling factors
"""
shape_scale.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    ctypes.c_float * 3,  # vec3 scale
]

# Function dvz_shape_translate()
shape_translate = dvz.dvz_shape_translate
shape_translate.__doc__ = """
Append a translation to a shape.

Parameters
----------
shape : DvzShape*
    the shape
translate : vec3
    the translation vector
"""
shape_translate.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    ctypes.c_float * 3,  # vec3 translate
]

# Function dvz_shape_rotate()
shape_rotate = dvz.dvz_shape_rotate
shape_rotate.__doc__ = """
Append a rotation to a shape.

Parameters
----------
shape : DvzShape*
    the shape
angle : float
    the rotation angle
axis : vec3
    the rotation axis
"""
shape_rotate.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    ctypes.c_float,  # float angle
    ctypes.c_float * 3,  # vec3 axis
]

# Function dvz_shape_transform()
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
    ctypes.c_float * 16,  # mat4 transform
]

# Function dvz_shape_rescaling()
shape_rescaling = dvz.dvz_shape_rescaling
shape_rescaling.__doc__ = """
Compute the rescaling factor to renormalize a shape.

Parameters
----------
shape : DvzShape*
    the shape
flags : int
    the rescaling flags
out_scale : vec3 (out parameter)
    the computed scaling factors
"""
shape_rescaling.argtypes = [
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    ctypes.c_int,  # int flags
    ctypes.c_float * 3,  # vec3 out_scale
]
shape_rescaling.restype = ctypes.c_float

# Function dvz_shape_end()
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

# Function dvz_shape_square()
shape_square = dvz.dvz_shape_square
shape_square.__doc__ = """
Create a square shape.

Parameters
----------
color : cvec4
    the square color

Returns
-------
type
    the shape
"""
shape_square.argtypes = [
    ctypes.c_uint8 * 4,  # cvec4 color
]
shape_square.restype = DvzShape

# Function dvz_shape_disc()
shape_disc = dvz.dvz_shape_disc
shape_disc.__doc__ = """
Create a disc shape.

Parameters
----------
count : uint32_t
    the number of points along the disc border
color : cvec4
    the disc color

Returns
-------
type
    the shape
"""
shape_disc.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    ctypes.c_uint8 * 4,  # cvec4 color
]
shape_disc.restype = DvzShape

# Function dvz_shape_polygon()
shape_polygon = dvz.dvz_shape_polygon
shape_polygon.__doc__ = """
Create a polygon shape using the simple earcut polygon triangulation algorithm.

Parameters
----------
count : uint32_t
    the number of points along the polygon border
points : dvec2*
    the points 2D coordinates
color : cvec4
    the polygon color

Returns
-------
type
    the shape
"""
shape_polygon.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.double, ndim=2, ncol=2, flags="C_CONTIGUOUS"),  # dvec2* points
    ctypes.c_uint8 * 4,  # cvec4 color
]
shape_polygon.restype = DvzShape

# Function dvz_shape_surface()
shape_surface = dvz.dvz_shape_surface
shape_surface.__doc__ = """
Create a grid shape.

Parameters
----------
row_count : uint32_t
    number of rows
col_count : uint32_t
    number of cols
heights : float*
    a pointer to row_count*col_count height values (floats)
colors : cvec4*
    a pointer to row_count*col_count color values (cvec4)
o : vec3
    the origin
u : vec3
    the unit vector parallel to each column
v : vec3
    the unit vector parallel to each row
flags : int
    the grid creation flags

Returns
-------
type
    the shape
"""
shape_surface.argtypes = [
    ctypes.c_uint32,  # uint32_t row_count
    ctypes.c_uint32,  # uint32_t col_count
    ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # float* heights
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # cvec4* colors
    ctypes.c_float * 3,  # vec3 o
    ctypes.c_float * 3,  # vec3 u
    ctypes.c_float * 3,  # vec3 v
    ctypes.c_int,  # int flags
]
shape_surface.restype = DvzShape

# Function dvz_shape_cube()
shape_cube = dvz.dvz_shape_cube
shape_cube.__doc__ = """
Create a cube shape.

Parameters
----------
colors : cvec4*
    the colors of the six faces

Returns
-------
type
    the shape
"""
shape_cube.argtypes = [
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # cvec4* colors
]
shape_cube.restype = DvzShape

# Function dvz_shape_sphere()
shape_sphere = dvz.dvz_shape_sphere
shape_sphere.__doc__ = """
Create a sphere shape.

Parameters
----------
rows : uint32_t
    the number of rows
cols : uint32_t
    the number of columns
color : cvec4
    the sphere color

Returns
-------
type
    the shape
"""
shape_sphere.argtypes = [
    ctypes.c_uint32,  # uint32_t rows
    ctypes.c_uint32,  # uint32_t cols
    ctypes.c_uint8 * 4,  # cvec4 color
]
shape_sphere.restype = DvzShape

# Function dvz_shape_cone()
shape_cone = dvz.dvz_shape_cone
shape_cone.__doc__ = """
Create a cone shape.

Parameters
----------
count : uint32_t
    the number of points along the disc border
color : cvec4
    the cone color

Returns
-------
type
    the shape
"""
shape_cone.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    ctypes.c_uint8 * 4,  # cvec4 color
]
shape_cone.restype = DvzShape

# Function dvz_shape_cylinder()
shape_cylinder = dvz.dvz_shape_cylinder
shape_cylinder.__doc__ = """
Create a cylinder shape.

Parameters
----------
count : uint32_t
    the number of points along the cylinder border
color : cvec4
    the cylinder color

Returns
-------
type
    the shape
"""
shape_cylinder.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    ctypes.c_uint8 * 4,  # cvec4 color
]
shape_cylinder.restype = DvzShape

# Function dvz_shape_normalize()
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

# Function dvz_shape_obj()
shape_obj = dvz.dvz_shape_obj
shape_obj.__doc__ = """
Load a .obj shape.

Parameters
----------
file_path : char*
    the path to the .obj file

Returns
-------
type
    the shape
"""
shape_obj.argtypes = [
    ctypes.c_char_p,  # char* file_path
]
shape_obj.restype = DvzShape

# Function dvz_basic()
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
type
    the visual
"""
basic.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzPrimitiveTopology,  # DvzPrimitiveTopology topology
    ctypes.c_int,  # int flags
]
basic.restype = ctypes.POINTER(DvzVisual)

# Function dvz_basic_position()
basic_position = dvz.dvz_basic_position
basic_position.__doc__ = """
Set the vertex positions.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : vec3*
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

# Function dvz_basic_color()
basic_color = dvz.dvz_basic_color
basic_color.__doc__ = """
Set the vertex colors.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : cvec4*
    the colors of the items to update
flags : int
    the data update flags
"""
basic_color.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # cvec4* values
    ctypes.c_int,  # int flags
]

# Function dvz_basic_group()
basic_group = dvz.dvz_basic_group
basic_group.__doc__ = """
Set the vertex group index.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : float*
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

# Function dvz_basic_size()
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

# Function dvz_basic_alloc()
basic_alloc = dvz.dvz_basic_alloc
basic_alloc.__doc__ = """
Allocate memory for a visual.

Parameters
----------
visual : DvzVisual*
    the visual
item_count : uint32_t
    the total number of items to allocate for this visual
"""
basic_alloc.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t item_count
]

# Function dvz_pixel()
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
type
    the visual
"""
pixel.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_int,  # int flags
]
pixel.restype = ctypes.POINTER(DvzVisual)

# Function dvz_pixel_position()
pixel_position = dvz.dvz_pixel_position
pixel_position.__doc__ = """
Set the pixel positions.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : vec3*
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

# Function dvz_pixel_color()
pixel_color = dvz.dvz_pixel_color
pixel_color.__doc__ = """
Set the pixel colors.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : cvec4*
    the colors of the items to update
flags : int
    the data update flags
"""
pixel_color.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # cvec4* values
    ctypes.c_int,  # int flags
]

# Function dvz_pixel_alloc()
pixel_alloc = dvz.dvz_pixel_alloc
pixel_alloc.__doc__ = """
Allocate memory for a visual.

Parameters
----------
visual : DvzVisual*
    the visual
item_count : uint32_t
    the total number of items to allocate for this visual
"""
pixel_alloc.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t item_count
]

# Function dvz_point()
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
type
    the visual
"""
point.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_int,  # int flags
]
point.restype = ctypes.POINTER(DvzVisual)

# Function dvz_point_position()
point_position = dvz.dvz_point_position
point_position.__doc__ = """
Set the point positions.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : vec3*
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

# Function dvz_point_color()
point_color = dvz.dvz_point_color
point_color.__doc__ = """
Set the point colors.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : cvec4*
    the colors of the items to update
flags : int
    the data update flags
"""
point_color.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # cvec4* values
    ctypes.c_int,  # int flags
]

# Function dvz_point_size()
point_size = dvz.dvz_point_size
point_size.__doc__ = """
Set the point sizes.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : float*
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

# Function dvz_point_alloc()
point_alloc = dvz.dvz_point_alloc
point_alloc.__doc__ = """
Allocate memory for a visual.

Parameters
----------
visual : DvzVisual*
    the visual
item_count : uint32_t
    the total number of items to allocate for this visual
"""
point_alloc.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t item_count
]

# Function dvz_marker()
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
type
    the visual
"""
marker.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_int,  # int flags
]
marker.restype = ctypes.POINTER(DvzVisual)

# Function dvz_marker_mode()
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

# Function dvz_marker_aspect()
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

# Function dvz_marker_shape()
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

# Function dvz_marker_position()
marker_position = dvz.dvz_marker_position
marker_position.__doc__ = """
Set the marker positions.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : vec3*
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

# Function dvz_marker_size()
marker_size = dvz.dvz_marker_size
marker_size.__doc__ = """
Set the marker sizes.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : float*
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

# Function dvz_marker_angle()
marker_angle = dvz.dvz_marker_angle
marker_angle.__doc__ = """
Set the marker angles.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : float*
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

# Function dvz_marker_color()
marker_color = dvz.dvz_marker_color
marker_color.__doc__ = """
Set the marker colors.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : cvec4*
    the colors of the items to update
flags : int
    the data update flags
"""
marker_color.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # cvec4* values
    ctypes.c_int,  # int flags
]

# Function dvz_marker_edge_color()
marker_edge_color = dvz.dvz_marker_edge_color
marker_edge_color.__doc__ = """
Set the marker edge color.

Parameters
----------
visual : DvzVisual*
    the visual
color : cvec4
    the edge color
"""
marker_edge_color.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint8 * 4,  # cvec4 color
]

# Function dvz_marker_edge_width()
marker_edge_width = dvz.dvz_marker_edge_width
marker_edge_width.__doc__ = """
Set the marker edge width.

Parameters
----------
visual : DvzVisual*
    the visual
width : float
    the edge width
"""
marker_edge_width.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_float,  # float width
]

# Function dvz_marker_tex()
marker_tex = dvz.dvz_marker_tex
marker_tex.__doc__ = """
Set the marker texture.

Parameters
----------
visual : DvzVisual*
    the visual
tex : DvzId
    the texture ID
sampler : DvzId
    the sampler ID
"""
marker_tex.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzId,  # DvzId tex
    DvzId,  # DvzId sampler
]

# Function dvz_marker_tex_scale()
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

# Function dvz_marker_alloc()
marker_alloc = dvz.dvz_marker_alloc
marker_alloc.__doc__ = """
Allocate memory for a visual.

Parameters
----------
visual : DvzVisual*
    the visual
item_count : uint32_t
    the total number of items to allocate for this visual
"""
marker_alloc.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t item_count
]

# Function dvz_segment()
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
type
    the visual
"""
segment.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_int,  # int flags
]
segment.restype = ctypes.POINTER(DvzVisual)

# Function dvz_segment_position()
segment_position = dvz.dvz_segment_position
segment_position.__doc__ = """
Set the segment positions.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
initial : vec3*
    the initial 3D positions of the segments
terminal : vec3*
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

# Function dvz_segment_shift()
segment_shift = dvz.dvz_segment_shift
segment_shift.__doc__ = """
Set the segment shift.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : vec4*
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

# Function dvz_segment_color()
segment_color = dvz.dvz_segment_color
segment_color.__doc__ = """
Set the segment colors.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : cvec4*
    the colors of the items to update
flags : int
    the data update flags
"""
segment_color.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # cvec4* values
    ctypes.c_int,  # int flags
]

# Function dvz_segment_linewidth()
segment_linewidth = dvz.dvz_segment_linewidth
segment_linewidth.__doc__ = """
Set the segment line widths.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : float*
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

# Function dvz_segment_cap()
segment_cap = dvz.dvz_segment_cap
segment_cap.__doc__ = """
Set the segment cap types.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
initial : DvzCapType*
    the initial segment cap types
terminal : DvzCapType*
    the terminal segment cap types
flags : int
    the data update flags
"""
segment_cap.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ctypes.POINTER(DvzCapType),  # DvzCapType* initial
    ctypes.POINTER(DvzCapType),  # DvzCapType* terminal
    ctypes.c_int,  # int flags
]

# Function dvz_segment_alloc()
segment_alloc = dvz.dvz_segment_alloc
segment_alloc.__doc__ = """
Allocate memory for a visual.

Parameters
----------
visual : DvzVisual*
    the visual
item_count : uint32_t
    the total number of items to allocate for this visual
"""
segment_alloc.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t item_count
]

# Function dvz_path()
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
type
    the visual
"""
path.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_int,  # int flags
]
path.restype = ctypes.POINTER(DvzVisual)

# Function dvz_path_position()
path_position = dvz.dvz_path_position
path_position.__doc__ = """
Set the path positions. Note: all path point positions must be updated at once for now.

Parameters
----------
visual : DvzVisual*
    the visual
vertex_count : unknown
    the total number of points across all paths
positions : vec3*
    the path point positions
path_count : uint32_t
    the number of different paths
path_lengths : uint32_t*
    the number of points in each path
flags : int
    the data update flags
"""
path_position.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t point_count
    ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS"),  # vec3* positions
    ctypes.c_uint32,  # uint32_t path_count
    ndpointer(dtype=np.uint32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # uint32_t* path_lengths
    ctypes.c_int,  # int flags
]

# Function dvz_path_color()
path_color = dvz.dvz_path_color
path_color.__doc__ = """
Set the path colors.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : cvec4*
    the colors of the items to update
flags : int
    the data update flags
"""
path_color.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # cvec4* values
    ctypes.c_int,  # int flags
]

# Function dvz_path_linewidth()
path_linewidth = dvz.dvz_path_linewidth
path_linewidth.__doc__ = """
Set the path line width.

Parameters
----------
visual : DvzVisual*
    the visual
width : float
    the line width
"""
path_linewidth.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_float,  # float width
]

# Function dvz_path_cap()
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

# Function dvz_path_join()
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

# Function dvz_path_alloc()
path_alloc = dvz.dvz_path_alloc
path_alloc.__doc__ = """
Allocate memory for a visual.

Parameters
----------
visual : DvzVisual*
    the visual
total_point_count : uint32_t
    the total number of points to allocate for this visual
"""
path_alloc.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t total_point_count
]

# Function dvz_atlas_font()
atlas_font = dvz.dvz_atlas_font
atlas_font.__doc__ = """
Load the default atlas and font.

Parameters
----------
font_size : double
    the font size

Returns
-------
type
    a DvzAtlasFont struct with DvzAtlas and DvzFont objects.
"""
atlas_font.argtypes = [
    ctypes.c_double,  # double font_size
]
atlas_font.restype = DvzAtlasFont

# Function dvz_atlas_destroy()
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

# Function dvz_font()
font = dvz.dvz_font
font.__doc__ = """
Create a font.

Parameters
----------
ttf_size : long
    size in bytes of a TTF font raw buffer
ttf_bytes : char*
    TTF font raw buffer

Returns
-------
type
    the font
"""
font.argtypes = [
    ctypes.c_long,  # long ttf_size
    ctypes.c_char_p,  # char* ttf_bytes
]
font.restype = ctypes.POINTER(DvzFont)

# Function dvz_font_size()
font_size = dvz.dvz_font_size
font_size.__doc__ = """
Set the font size.

Parameters
----------
font : DvzFont*
    the font
size : double
    the font size
"""
font_size.argtypes = [
    ctypes.POINTER(DvzFont),  # DvzFont* font
    ctypes.c_double,  # double size
]

# Function dvz_font_layout()
font_layout = dvz.dvz_font_layout
font_layout.__doc__ = """
Compute the shift of each glyph in a Unicode string, using the Freetype library.

Parameters
----------
font : DvzFont*
    the font
length : uint32_t
    the number of glyphs
codepoints : uint32_t*
    the Unicode codepoints of the glyphs

Returns
-------
type
    an array of (x,y,w,h) shifts
"""
font_layout.argtypes = [
    ctypes.POINTER(DvzFont),  # DvzFont* font
    ctypes.c_uint32,  # uint32_t length
    ndpointer(dtype=np.uint32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # uint32_t* codepoints
]
font_layout.restype = ndpointer(dtype=np.float32, ndim=2, ncol=4, flags="C_CONTIGUOUS")

# Function dvz_font_ascii()
font_ascii = dvz.dvz_font_ascii
font_ascii.__doc__ = """
Compute the shift of each glyph in an ASCII string, using the Freetype library.  Note: the caller must free the output after use.

Parameters
----------
font : DvzFont*
    the font
string : char*
    the ASCII string

Returns
-------
type
    an array of (x,y,w,h) shifts
"""
font_ascii.argtypes = [
    ctypes.POINTER(DvzFont),  # DvzFont* font
    ctypes.c_char_p,  # char* string
]
font_ascii.restype = ndpointer(dtype=np.float32, ndim=2, ncol=4, flags="C_CONTIGUOUS")

# Function dvz_font_draw()
font_draw = dvz.dvz_font_draw
font_draw.__doc__ = """
Render a string using Freetype.  Note: the caller must free the output after use.

Parameters
----------
font : DvzFont*
    the font
length : uint32_t
    the number of glyphs
codepoints : uint32_t*
    the Unicode codepoints of the glyphs
xywh : vec4*
    an array of (x,y,w,h) shifts, returned by dvz_font_layout()
flags : int
    the font flags
out_size : uvec2 (out parameter)
    the number of bytes in the returned image

Returns
-------
type
    an RGBA array allocated by this function and that MUST be freed by the caller
"""
font_draw.argtypes = [
    ctypes.POINTER(DvzFont),  # DvzFont* font
    ctypes.c_uint32,  # uint32_t length
    ndpointer(dtype=np.uint32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # uint32_t* codepoints
    ndpointer(dtype=np.float32, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # vec4* xywh
    ctypes.c_int,  # int flags
    ctypes.c_uint32 * 2,  # uvec2 out_size
]
font_draw.restype = ndpointer(dtype=np.uint8, ndim=1, ncol=1, flags="C_CONTIGUOUS")

# Function dvz_font_texture()
font_texture = dvz.dvz_font_texture
font_texture.__doc__ = """
Generate a texture with a rendered text.

Parameters
----------
font : DvzFont*
    the font
batch : DvzBatch*
    the batch
length : uint32_t
    the number of Unicode codepoints
codepoints : uint32_t*
    the Unicode codepoints
size : uvec3 (out parameter)
    the generated texture size

Returns
-------
type
    a tex ID
"""
font_texture.argtypes = [
    ctypes.POINTER(DvzFont),  # DvzFont* font
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_uint32,  # uint32_t length
    ndpointer(dtype=np.uint32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # uint32_t* codepoints
    ctypes.c_uint32 * 3,  # uvec3 size
]
font_texture.restype = DvzId

# Function dvz_font_destroy()
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

# Function dvz_glyph()
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
type
    the visual
"""
glyph.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_int,  # int flags
]
glyph.restype = ctypes.POINTER(DvzVisual)

# Function dvz_glyph_alloc()
glyph_alloc = dvz.dvz_glyph_alloc
glyph_alloc.__doc__ = """
Allocate memory for a visual.

Parameters
----------
visual : DvzVisual*
    the visual
item_count : uint32_t
    the total number of items to allocate for this visual
"""
glyph_alloc.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t item_count
]

# Function dvz_glyph_position()
glyph_position = dvz.dvz_glyph_position
glyph_position.__doc__ = """
Set the glyph positions.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : vec3*
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

# Function dvz_glyph_axis()
glyph_axis = dvz.dvz_glyph_axis
glyph_axis.__doc__ = """
Set the glyph axes.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : vec3*
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

# Function dvz_glyph_size()
glyph_size = dvz.dvz_glyph_size
glyph_size.__doc__ = """
Set the glyph sizes.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : vec2*
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

# Function dvz_glyph_anchor()
glyph_anchor = dvz.dvz_glyph_anchor
glyph_anchor.__doc__ = """
Set the glyph anchors.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : vec2*
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

# Function dvz_glyph_shift()
glyph_shift = dvz.dvz_glyph_shift
glyph_shift.__doc__ = """
Set the glyph shifts.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : vec2*
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

# Function dvz_glyph_texcoords()
glyph_texcoords = dvz.dvz_glyph_texcoords
glyph_texcoords.__doc__ = """
Set the glyph texture coordinates.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
coords : vec4*
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

# Function dvz_glyph_angle()
glyph_angle = dvz.dvz_glyph_angle
glyph_angle.__doc__ = """
Set the glyph angles.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : float*
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

# Function dvz_glyph_color()
glyph_color = dvz.dvz_glyph_color
glyph_color.__doc__ = """
Set the glyph colors.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : cvec4*
    the colors of the items to update
flags : int
    the data update flags
"""
glyph_color.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # cvec4* values
    ctypes.c_int,  # int flags
]

# Function dvz_glyph_groupsize()
glyph_groupsize = dvz.dvz_glyph_groupsize
glyph_groupsize.__doc__ = """
Set the glyph group size.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : float*
    the glyph group sizes
flags : int
    the data update flags
"""
glyph_groupsize.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # float* values
    ctypes.c_int,  # int flags
]

# Function dvz_glyph_bgcolor()
glyph_bgcolor = dvz.dvz_glyph_bgcolor
glyph_bgcolor.__doc__ = """
Set the glyph background color.

Parameters
----------
visual : DvzVisual*
    the visual
bgcolor : vec4
    the background color
"""
glyph_bgcolor.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_float * 4,  # vec4 bgcolor
]

# Function dvz_glyph_texture()
glyph_texture = dvz.dvz_glyph_texture
glyph_texture.__doc__ = """
Assign a texture to a glyph visual.

Parameters
----------
visual : DvzVisual*
    the visual
tex : DvzId
    the texture ID
"""
glyph_texture.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzId,  # DvzId tex
]

# Function dvz_glyph_atlas()
glyph_atlas = dvz.dvz_glyph_atlas
glyph_atlas.__doc__ = """
Associate an atlas with a glyph visual.

Parameters
----------
visual : DvzVisual*
    the visual
atlas : DvzAtlas*
    the atlas
"""
glyph_atlas.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.POINTER(DvzAtlas),  # DvzAtlas* atlas
]

# Function dvz_glyph_unicode()
glyph_unicode = dvz.dvz_glyph_unicode
glyph_unicode.__doc__ = """
Set the glyph unicode code points.

Parameters
----------
visual : DvzVisual*
    the visual
count : uint32_t
    the number of glyphs
codepoints : uint32_t*
    the unicode codepoints
"""
glyph_unicode.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # uint32_t* codepoints
]

# Function dvz_glyph_ascii()
glyph_ascii = dvz.dvz_glyph_ascii
glyph_ascii.__doc__ = """
Set the glyph ascii characters.

Parameters
----------
visual : DvzVisual*
    the visual
string : char*
    the characters
"""
glyph_ascii.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_char_p,  # char* string
]

# Function dvz_glyph_xywh()
glyph_xywh = dvz.dvz_glyph_xywh
glyph_xywh.__doc__ = """
Set the xywh parameters of each glyph.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : vec4*
    the xywh values of each glyph
offset : vec2
    the xy offsets of each glyph
flags : int
    the data update flags
"""
glyph_xywh.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # vec4* values
    ctypes.c_float * 2,  # vec2 offset
    ctypes.c_int,  # int flags
]

# Function dvz_monoglyph()
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
type
    the visual
"""
monoglyph.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_int,  # int flags
]
monoglyph.restype = ctypes.POINTER(DvzVisual)

# Function dvz_monoglyph_position()
monoglyph_position = dvz.dvz_monoglyph_position
monoglyph_position.__doc__ = """
Set the glyph positions.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : vec3*
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

# Function dvz_monoglyph_offset()
monoglyph_offset = dvz.dvz_monoglyph_offset
monoglyph_offset.__doc__ = """
Set the glyph offsets.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : ivec2*
    the glyph offsets (ivec2 integers: row,column)
flags : int
    the data update flags
"""
monoglyph_offset.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ctypes.POINTER(ctypes.c_int32 * 2),  # ivec2* values
    ctypes.c_int,  # int flags
]

# Function dvz_monoglyph_color()
monoglyph_color = dvz.dvz_monoglyph_color
monoglyph_color.__doc__ = """
Set the glyph colors.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : cvec4*
    the colors of the items to update
flags : int
    the data update flags
"""
monoglyph_color.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # cvec4* values
    ctypes.c_int,  # int flags
]

# Function dvz_monoglyph_glyph()
monoglyph_glyph = dvz.dvz_monoglyph_glyph
monoglyph_glyph.__doc__ = """
Set the text.

Parameters
----------
visual : DvzVisual*
    the visual
text : char*
    the ASCII test (string length without the null terminal byte = number of glyphs)
"""
monoglyph_glyph.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_char_p,  # char* text
    ctypes.c_int,  # int flags
]

# Function dvz_monoglyph_anchor()
monoglyph_anchor = dvz.dvz_monoglyph_anchor
monoglyph_anchor.__doc__ = """
Set the glyph anchor (relative to the glyph size).

Parameters
----------
visual : DvzVisual*
    the visual
anchor : vec2
    the anchor
"""
monoglyph_anchor.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_float * 2,  # vec2 anchor
]

# Function dvz_monoglyph_size()
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

# Function dvz_monoglyph_textarea()
monoglyph_textarea = dvz.dvz_monoglyph_textarea
monoglyph_textarea.__doc__ = """
All-in-one function for multiline text.

Parameters
----------
visual : DvzVisual*
    the visual
pos : vec3
    the text position
color : cvec4
    the text color
size : float
    the glyph size
text : char*
    the text, can contain `\n` new lines
"""
monoglyph_textarea.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_float * 3,  # vec3 pos
    ctypes.c_uint8 * 4,  # cvec4 color
    ctypes.c_float,  # float size
    ctypes.c_char_p,  # char* text
]

# Function dvz_monoglyph_alloc()
monoglyph_alloc = dvz.dvz_monoglyph_alloc
monoglyph_alloc.__doc__ = """
Allocate memory for a visual.

Parameters
----------
visual : DvzVisual*
    the visual
item_count : uint32_t
    the total number of items to allocate for this visual
"""
monoglyph_alloc.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t item_count
]

# Function dvz_image()
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
type
    the visual
"""
image.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_int,  # int flags
]
image.restype = ctypes.POINTER(DvzVisual)

# Function dvz_image_position()
image_position = dvz.dvz_image_position
image_position.__doc__ = """
Set the image positions.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : vec3*
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

# Function dvz_image_size()
image_size = dvz.dvz_image_size
image_size.__doc__ = """
Set the image sizes.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : vec2*
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

# Function dvz_image_anchor()
image_anchor = dvz.dvz_image_anchor
image_anchor.__doc__ = """
Set the image anchors.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : vec2*
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

# Function dvz_image_texcoords()
image_texcoords = dvz.dvz_image_texcoords
image_texcoords.__doc__ = """
Set the image texture coordinates.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
tl_br : vec4*
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

# Function dvz_image_color()
image_color = dvz.dvz_image_color
image_color.__doc__ = """
Set the image colors (only when using DVZ_IMAGE_FLAGS_FILL).

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : cvec4*
    the image colors
flags : int
    the data update flags
"""
image_color.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # cvec4* values
    ctypes.c_int,  # int flags
]

# Function dvz_image_texture()
image_texture = dvz.dvz_image_texture
image_texture.__doc__ = """
Assign a texture to an image visual.

Parameters
----------
visual : DvzVisual*
    the visual
tex : DvzId
    the texture ID
filter : DvzFilter
    the texture filtering mode
address_mode : DvzSamplerAddressMode
    the texture address mode
"""
image_texture.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzId,  # DvzId tex
    DvzFilter,  # DvzFilter filter
    DvzSamplerAddressMode,  # DvzSamplerAddressMode address_mode
]

# Function dvz_image_radius()
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

# Function dvz_image_edge_width()
image_edge_width = dvz.dvz_image_edge_width
image_edge_width.__doc__ = """
Set the edge width.

Parameters
----------
visual : DvzVisual*
    the visual
width : float
    the edge width
"""
image_edge_width.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_float,  # float width
]

# Function dvz_image_edge_color()
image_edge_color = dvz.dvz_image_edge_color
image_edge_color.__doc__ = """
Set the edge color.

Parameters
----------
visual : DvzVisual*
    the visual
color : cvec4
    the edge color
"""
image_edge_color.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint8 * 4,  # cvec4 color
]

# Function dvz_image_alloc()
image_alloc = dvz.dvz_image_alloc
image_alloc.__doc__ = """
Allocate memory for a visual.

Parameters
----------
visual : DvzVisual*
    the visual
item_count : uint32_t
    the total number of images to allocate for this visual
"""
image_alloc.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t item_count
]

# Function dvz_tex_image()
tex_image = dvz.dvz_tex_image
tex_image.__doc__ = """
Create a 2D texture to be used in an image visual.

Parameters
----------
batch : DvzBatch*
    the batch
format : DvzFormat
    the texture format
width : uint32_t
    the texture width
height : uint32_t
    the texture height
data : void*
    the texture data to upload

Returns
-------
type
    the texture ID
"""
tex_image.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzFormat,  # DvzFormat format
    ctypes.c_uint32,  # uint32_t width
    ctypes.c_uint32,  # uint32_t height
    ctypes.c_void_p,  # void* data
]
tex_image.restype = DvzId

# Function dvz_mesh()
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
type
    the visual
"""
mesh.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_int,  # int flags
]
mesh.restype = ctypes.POINTER(DvzVisual)

# Function dvz_mesh_position()
mesh_position = dvz.dvz_mesh_position
mesh_position.__doc__ = """
Set the mesh vertex positions.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : vec3*
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

# Function dvz_mesh_color()
mesh_color = dvz.dvz_mesh_color
mesh_color.__doc__ = """
Set the mesh colors.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : cvec4*
    the vertex colors
flags : int
    the data update flags
"""
mesh_color.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # cvec4* values
    ctypes.c_int,  # int flags
]

# Function dvz_mesh_texcoords()
mesh_texcoords = dvz.dvz_mesh_texcoords
mesh_texcoords.__doc__ = """
Set the mesh texture coordinates.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : vec4*
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

# Function dvz_mesh_normal()
mesh_normal = dvz.dvz_mesh_normal
mesh_normal.__doc__ = """
Set the mesh normals.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : vec3*
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

# Function dvz_mesh_isoline()
mesh_isoline = dvz.dvz_mesh_isoline
mesh_isoline.__doc__ = """
Set the isolines values.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : float*
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

# Function dvz_mesh_left()
mesh_left = dvz.dvz_mesh_left
mesh_left.__doc__ = """
Set the distance between the current vertex to the left edge at corner A, B, or C in triangle ABC.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : vec3*
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

# Function dvz_mesh_right()
mesh_right = dvz.dvz_mesh_right
mesh_right.__doc__ = """
Set the distance between the current vertex to the right edge at corner A, B, or C in triangle ABC.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : vec3*
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

# Function dvz_mesh_contour()
mesh_contour = dvz.dvz_mesh_contour
mesh_contour.__doc__ = """
Set the contour information for polygon contours.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
values : cvec4*
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

# Function dvz_mesh_texture()
mesh_texture = dvz.dvz_mesh_texture
mesh_texture.__doc__ = """
Assign a 2D texture to a mesh visual.

Parameters
----------
visual : DvzVisual*
    the visual
tex : DvzId
    the texture ID
filter : DvzFilter
    the texture filtering mode
address_mode : DvzSamplerAddressMode
    the texture address mode
"""
mesh_texture.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzId,  # DvzId tex
    DvzFilter,  # DvzFilter filter
    DvzSamplerAddressMode,  # DvzSamplerAddressMode address_mode
]

# Function dvz_mesh_index()
mesh_index = dvz.dvz_mesh_index
mesh_index.__doc__ = """
Set the mesh indices.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
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

# Function dvz_mesh_alloc()
mesh_alloc = dvz.dvz_mesh_alloc
mesh_alloc.__doc__ = """
Allocate memory for a visual.

Parameters
----------
visual : DvzVisual*
    the visual
vertex_count : uint32_t
    the number of vertices
index_count : uint32_t
    the number of indices
"""
mesh_alloc.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t vertex_count
    ctypes.c_uint32,  # uint32_t index_count
]

# Function dvz_mesh_light_pos()
mesh_light_pos = dvz.dvz_mesh_light_pos
mesh_light_pos.__doc__ = """
Set the mesh light position.

Parameters
----------
visual : DvzVisual*
    the mesh
pos : vec3
    the light position
"""
mesh_light_pos.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_float * 3,  # vec3 pos
]

# Function dvz_mesh_light_params()
mesh_light_params = dvz.dvz_mesh_light_params
mesh_light_params.__doc__ = """
Set the mesh light parameters.

Parameters
----------
visual : DvzVisual*
    the mesh
params : vec4
    the light parameters (vec4 ambient, diffuse, specular, exponent)
"""
mesh_light_params.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_float * 4,  # vec4 params
]

# Function dvz_mesh_stroke()
mesh_stroke = dvz.dvz_mesh_stroke
mesh_stroke.__doc__ = """
Set the stroke color.  Note: the alpha component is currently unused.

Parameters
----------
visual : DvzVisual*
    the mesh
stroke : unknown
    the rgba components
"""
mesh_stroke.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint8 * 4,  # cvec4 rgba
]

# Function dvz_mesh_linewidth()
mesh_linewidth = dvz.dvz_mesh_linewidth
mesh_linewidth.__doc__ = """
Set the stroke linewidth (wireframe or isoline).

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

# Function dvz_mesh_density()
mesh_density = dvz.dvz_mesh_density
mesh_density.__doc__ = """
Set the number of isolines

Parameters
----------
visual : DvzVisual*
    the mesh
count : uint32_t
    the number of isolines
"""
mesh_density.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t count
]

# Function dvz_mesh_shape()
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
type
    the mesh
"""
mesh_shape.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.POINTER(DvzShape),  # DvzShape* shape
    ctypes.c_int,  # int flags
]
mesh_shape.restype = ctypes.POINTER(DvzVisual)

# Function dvz_mesh_reshape()
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

# Function dvz_sphere()
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
type
    the visual
"""
sphere.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_int,  # int flags
]
sphere.restype = ctypes.POINTER(DvzVisual)

# Function dvz_sphere_position()
sphere_position = dvz.dvz_sphere_position
sphere_position.__doc__ = """
Set the sphere positions.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
pos : vec3*
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

# Function dvz_sphere_color()
sphere_color = dvz.dvz_sphere_color
sphere_color.__doc__ = """
Set the sphere colors.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
color : cvec4*
    the sphere colors
flags : int
    the data update flags
"""
sphere_color.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t first
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS"),  # cvec4* color
    ctypes.c_int,  # int flags
]

# Function dvz_sphere_size()
sphere_size = dvz.dvz_sphere_size
sphere_size.__doc__ = """
Set the sphere sizes.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
size : float*
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

# Function dvz_sphere_alloc()
sphere_alloc = dvz.dvz_sphere_alloc
sphere_alloc.__doc__ = """
Allocate memory for a visual.

Parameters
----------
visual : DvzVisual*
    the visual
item_count : uint32_t
    the total number of spheres to allocate for this visual
"""
sphere_alloc.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t item_count
]

# Function dvz_sphere_light_pos()
sphere_light_pos = dvz.dvz_sphere_light_pos
sphere_light_pos.__doc__ = """
Set the sphere light position.

Parameters
----------
visual : DvzVisual*
    the visual
pos : vec3
    the light position
"""
sphere_light_pos.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_float * 3,  # vec3 pos
]

# Function dvz_sphere_light_params()
sphere_light_params = dvz.dvz_sphere_light_params
sphere_light_params.__doc__ = """
Set the sphere light parameters.

Parameters
----------
visual : DvzVisual*
    the visual
params : vec4
    the light parameters (vec4 ambient, diffuse, specular, exponent)
"""
sphere_light_params.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_float * 4,  # vec4 params
]

# Function dvz_volume()
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
type
    the visual
"""
volume.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_int,  # int flags
]
volume.restype = ctypes.POINTER(DvzVisual)

# Function dvz_volume_alloc()
volume_alloc = dvz.dvz_volume_alloc
volume_alloc.__doc__ = """
Allocate memory for a visual.

Parameters
----------
visual : DvzVisual*
    the visual
item_count : uint32_t
    the total number of volumes to allocate for this visual
"""
volume_alloc.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t item_count
]

# Function dvz_volume_texture()
volume_texture = dvz.dvz_volume_texture
volume_texture.__doc__ = """
Assign a 3D texture to a volume visual.

Parameters
----------
visual : DvzVisual*
    the visual
tex : DvzId
    the texture ID
filter : DvzFilter
    the texture filtering mode
address_mode : DvzSamplerAddressMode
    the texture address mode
"""
volume_texture.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzId,  # DvzId tex
    DvzFilter,  # DvzFilter filter
    DvzSamplerAddressMode,  # DvzSamplerAddressMode address_mode
]

# Function dvz_volume_size()
volume_size = dvz.dvz_volume_size
volume_size.__doc__ = """
Set the volume size.

Parameters
----------
visual : DvzVisual*
    the visual
w : float
    the texture width
h : float
    the texture height
d : float
    the texture depth
"""
volume_size.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_float,  # float w
    ctypes.c_float,  # float h
    ctypes.c_float,  # float d
]

# Function dvz_volume_texcoords()
volume_texcoords = dvz.dvz_volume_texcoords
volume_texcoords.__doc__ = """
Set the texture coordinates of two corner points.

Parameters
----------
visual : DvzVisual*
    the visual
uvw0 : vec3
    coordinates of one of the corner points
uvw1 : vec3
    coordinates of one of the corner points
"""
volume_texcoords.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_float * 3,  # vec3 uvw0
    ctypes.c_float * 3,  # vec3 uvw1
]

# Function dvz_volume_transfer()
volume_transfer = dvz.dvz_volume_transfer
volume_transfer.__doc__ = """
Set the volume size.

Parameters
----------
visual : DvzVisual*
    the visual
transfer : vec4
    transfer function, for now `vec4(x, 0, 0, 0)` where x is a scaling factor
"""
volume_transfer.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_float * 4,  # vec4 transfer
]

# Function dvz_tex_volume()
tex_volume = dvz.dvz_tex_volume
tex_volume.__doc__ = """
Create a 3D texture to be used in a volume visual.

Parameters
----------
batch : DvzBatch*
    the batch
format : DvzFormat
    the texture format
width : uint32_t
    the texture width
height : uint32_t
    the texture height
depth : uint32_t
    the texture depth
data : void*
    the texture data to upload

Returns
-------
type
    the texture ID
"""
tex_volume.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzFormat,  # DvzFormat format
    ctypes.c_uint32,  # uint32_t width
    ctypes.c_uint32,  # uint32_t height
    ctypes.c_uint32,  # uint32_t depth
    ctypes.c_void_p,  # void* data
]
tex_volume.restype = DvzId

# Function dvz_slice()
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
type
    the visual
"""
slice.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    ctypes.c_int,  # int flags
]
slice.restype = ctypes.POINTER(DvzVisual)

# Function dvz_slice_position()
slice_position = dvz.dvz_slice_position
slice_position.__doc__ = """
Set the slice positions.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
p0 : vec3*
    the 3D positions of the top left corner
p1 : vec3*
    the 3D positions of the top right corner
p2 : vec3*
    the 3D positions of the bottom left corner
p3 : vec3*
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

# Function dvz_slice_texcoords()
slice_texcoords = dvz.dvz_slice_texcoords
slice_texcoords.__doc__ = """
Set the slice texture coordinates.

Parameters
----------
visual : DvzVisual*
    the visual
first : uint32_t
    the index of the first item to update
count : uint32_t
    the number of items to update
uvw0 : vec3*
    the 3D texture coordinates of the top left corner
uvw1 : vec3*
    the 3D texture coordinates of the top right corner
uvw2 : vec3*
    the 3D texture coordinates of the bottom left corner
uvw3 : vec3*
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

# Function dvz_slice_texture()
slice_texture = dvz.dvz_slice_texture
slice_texture.__doc__ = """
Assign a texture to a slice visual.

Parameters
----------
visual : DvzVisual*
    the visual
tex : DvzId
    the texture ID
filter : DvzFilter
    the texture filtering mode
address_mode : DvzSamplerAddressMode
    the texture address mode
"""
slice_texture.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    DvzId,  # DvzId tex
    DvzFilter,  # DvzFilter filter
    DvzSamplerAddressMode,  # DvzSamplerAddressMode address_mode
]

# Function dvz_slice_alloc()
slice_alloc = dvz.dvz_slice_alloc
slice_alloc.__doc__ = """
Allocate memory for a visual.

Parameters
----------
visual : DvzVisual*
    the visual
item_count : uint32_t
    the total number of slices to allocate for this visual
"""
slice_alloc.argtypes = [
    ctypes.POINTER(DvzVisual),  # DvzVisual* visual
    ctypes.c_uint32,  # uint32_t item_count
]

# Function dvz_tex_slice()
tex_slice = dvz.dvz_tex_slice
tex_slice.__doc__ = """
Create a 3D texture to be used in a slice visual.

Parameters
----------
batch : DvzBatch*
    the batch
format : DvzFormat
    the texture format
width : uint32_t
    the texture width
height : uint32_t
    the texture height
depth : uint32_t
    the texture depth
data : void*
    the texture data to upload

Returns
-------
type
    the texture ID
"""
tex_slice.argtypes = [
    ctypes.POINTER(DvzBatch),  # DvzBatch* batch
    DvzFormat,  # DvzFormat format
    ctypes.c_uint32,  # uint32_t width
    ctypes.c_uint32,  # uint32_t height
    ctypes.c_uint32,  # uint32_t depth
    ctypes.c_void_p,  # void* data
]
tex_slice.restype = DvzId

# Function dvz_slice_alpha()
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

# Function dvz_resample()
resample = dvz.dvz_resample
resample.__doc__ = """
Normalize a value in an interval.

Parameters
----------
t0 : double
    the interval start
t1 : double
    the interval end
t : double
    the value within the interval

Returns
-------
type
    the normalized value between 0 and 1
"""
resample.argtypes = [
    ctypes.c_double,  # double t0
    ctypes.c_double,  # double t1
    ctypes.c_double,  # double t
]
resample.restype = ctypes.c_double

# Function dvz_easing()
easing = dvz.dvz_easing
easing.__doc__ = """
Apply an easing function to a normalized value.

Parameters
----------
easing : DvzEasing
    the easing mode
t : double
    the normalized value

Returns
-------
type
    the eased value
"""
easing.argtypes = [
    DvzEasing,  # DvzEasing easing
    ctypes.c_double,  # double t
]
easing.restype = ctypes.c_double

# Function dvz_circular_2D()
circular_2D = dvz.dvz_circular_2D
circular_2D.__doc__ = """
Generate a 2D circular motion.

Parameters
----------
center : vec2
    the circle center
radius : float
    the circle radius
angle : float
    the initial angle
t : float
    the normalized value
out : vec2 (out parameter)
    the 2D position
"""
circular_2D.argtypes = [
    ctypes.c_float * 2,  # vec2 center
    ctypes.c_float,  # float radius
    ctypes.c_float,  # float angle
    ctypes.c_float,  # float t
    ctypes.c_float * 2,  # vec2 out
]

# Function dvz_circular_3D()
circular_3D = dvz.dvz_circular_3D
circular_3D.__doc__ = """
Generate a 3D circular motion.

Parameters
----------
center : vec3
    the circle center
u : vec3
    the first 3D vector defining the plane containing the circle
v : vec3
    the second 3D vector defining the plane containing the circle
radius : float
    the circle radius
angle : float
    the initial angle
t : float
    the normalized value
out : vec3 (out parameter)
    the 3D position
"""
circular_3D.argtypes = [
    ctypes.c_float * 3,  # vec3 center
    ctypes.c_float * 3,  # vec3 u
    ctypes.c_float * 3,  # vec3 v
    ctypes.c_float,  # float radius
    ctypes.c_float,  # float angle
    ctypes.c_float,  # float t
    ctypes.c_float * 3,  # vec3 out
]

# Function dvz_interpolate()
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
type
    the interpolated value
"""
interpolate.argtypes = [
    ctypes.c_float,  # float p0
    ctypes.c_float,  # float p1
    ctypes.c_float,  # float t
]
interpolate.restype = ctypes.c_float

# Function dvz_interpolate_2D()
interpolate_2D = dvz.dvz_interpolate_2D
interpolate_2D.__doc__ = """
Make a linear interpolation between two 2D points.

Parameters
----------
p0 : vec2
    the first point
p1 : vec2
    the second point
t : float
    the normalized value

Returns
-------
type
    the interpolated point
"""
interpolate_2D.argtypes = [
    ctypes.c_float * 2,  # vec2 p0
    ctypes.c_float * 2,  # vec2 p1
    ctypes.c_float,  # float t
    ctypes.c_float * 2,  # vec2 out
]

# Function dvz_interpolate_3D()
interpolate_3D = dvz.dvz_interpolate_3D
interpolate_3D.__doc__ = """
Make a linear interpolation between two 3D points.

Parameters
----------
p0 : vec3
    the first point
p1 : vec3
    the second point
t : float
    the normalized value

Returns
-------
type
    the interpolated point
"""
interpolate_3D.argtypes = [
    ctypes.c_float * 3,  # vec3 p0
    ctypes.c_float * 3,  # vec3 p1
    ctypes.c_float,  # float t
    ctypes.c_float * 3,  # vec3 out
]

# Function dvz_arcball_initial()
arcball_initial = dvz.dvz_arcball_initial
arcball_initial.__doc__ = """
Set the initial arcball angles.

Parameters
----------
arcball : DvzArcball*
    the arcball
angles : vec3
    the initial angles
"""
arcball_initial.argtypes = [
    ctypes.POINTER(DvzArcball),  # DvzArcball* arcball
    ctypes.c_float * 3,  # vec3 angles
]

# Function dvz_arcball_reset()
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

# Function dvz_arcball_resize()
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

# Function dvz_arcball_flags()
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

# Function dvz_arcball_constrain()
arcball_constrain = dvz.dvz_arcball_constrain
arcball_constrain.__doc__ = """
Add arcball constraints.

Parameters
----------
arcball : DvzArcball*
    the arcball
constrain : vec3
    the constrain values
"""
arcball_constrain.argtypes = [
    ctypes.POINTER(DvzArcball),  # DvzArcball* arcball
    ctypes.c_float * 3,  # vec3 constrain
]

# Function dvz_arcball_set()
arcball_set = dvz.dvz_arcball_set
arcball_set.__doc__ = """
Set the arcball angles.

Parameters
----------
arcball : DvzArcball*
    the arcball
angles : vec3
    the angles
"""
arcball_set.argtypes = [
    ctypes.POINTER(DvzArcball),  # DvzArcball* arcball
    ctypes.c_float * 3,  # vec3 angles
]

# Function dvz_arcball_angles()
arcball_angles = dvz.dvz_arcball_angles
arcball_angles.__doc__ = """
Get the current arcball angles.

Parameters
----------
arcball : DvzArcball*
    the arcball
out_angles : vec3 (out parameter)
    the arcball angles
"""
arcball_angles.argtypes = [
    ctypes.POINTER(DvzArcball),  # DvzArcball* arcball
    ctypes.c_float * 3,  # vec3 out_angles
]

# Function dvz_arcball_rotate()
arcball_rotate = dvz.dvz_arcball_rotate
arcball_rotate.__doc__ = """
Apply a rotation to an arcball.

Parameters
----------
arcball : DvzArcball*
    the arcball
cur_pos : vec2
    the initial position
last_pos : vec2
    the final position
"""
arcball_rotate.argtypes = [
    ctypes.POINTER(DvzArcball),  # DvzArcball* arcball
    ctypes.c_float * 2,  # vec2 cur_pos
    ctypes.c_float * 2,  # vec2 last_pos
]

# Function dvz_arcball_model()
arcball_model = dvz.dvz_arcball_model
arcball_model.__doc__ = """
Return the model matrix of an arcball.

Parameters
----------
arcball : DvzArcball*
    the arcball
model : mat4 (out parameter)
    the model
"""
arcball_model.argtypes = [
    ctypes.POINTER(DvzArcball),  # DvzArcball* arcball
    ctypes.c_float * 16,  # mat4 model
]

# Function dvz_arcball_end()
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

# Function dvz_arcball_mvp()
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

# Function dvz_arcball_print()
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

# Function dvz_arcball_gui()
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

# Function dvz_camera_initial()
camera_initial = dvz.dvz_camera_initial
camera_initial.__doc__ = """
Set the initial camera parameters.

Parameters
----------
camera : DvzCamera*
    the camera
pos : vec3
    the initial position
lookat : vec3
    the lookat position
up : vec3
    the up vector
"""
camera_initial.argtypes = [
    ctypes.POINTER(DvzCamera),  # DvzCamera* camera
    ctypes.c_float * 3,  # vec3 pos
    ctypes.c_float * 3,  # vec3 lookat
    ctypes.c_float * 3,  # vec3 up
]

# Function dvz_camera_reset()
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

# Function dvz_camera_zrange()
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

# Function dvz_camera_ortho()
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

# Function dvz_camera_resize()
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

# Function dvz_camera_position()
camera_position = dvz.dvz_camera_position
camera_position.__doc__ = """
Set a camera position.

Parameters
----------
camera : DvzCamera*
    the camera
pos : vec3
    the pos
"""
camera_position.argtypes = [
    ctypes.POINTER(DvzCamera),  # DvzCamera* camera
    ctypes.c_float * 3,  # vec3 pos
]

# Function dvz_camera_lookat()
camera_lookat = dvz.dvz_camera_lookat
camera_lookat.__doc__ = """
Set a camera lookat position.

Parameters
----------
camera : DvzCamera*
    the camera
lookat : vec3
    the lookat position
"""
camera_lookat.argtypes = [
    ctypes.POINTER(DvzCamera),  # DvzCamera* camera
    ctypes.c_float * 3,  # vec3 lookat
]

# Function dvz_camera_up()
camera_up = dvz.dvz_camera_up
camera_up.__doc__ = """
Set a camera up vector.

Parameters
----------
camera : DvzCamera*
    the camera
up : vec3
    the up vector
"""
camera_up.argtypes = [
    ctypes.POINTER(DvzCamera),  # DvzCamera* camera
    ctypes.c_float * 3,  # vec3 up
]

# Function dvz_camera_perspective()
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

# Function dvz_camera_viewproj()
camera_viewproj = dvz.dvz_camera_viewproj
camera_viewproj.__doc__ = """
Return the view and proj matrices of the camera.

Parameters
----------
camera : DvzCamera*
    the camera
view : mat4 (out parameter)
    the view matrix
proj : mat4 (out parameter)
    the proj matrix
"""
camera_viewproj.argtypes = [
    ctypes.POINTER(DvzCamera),  # DvzCamera* camera
    ctypes.c_float * 16,  # mat4 view
    ctypes.c_float * 16,  # mat4 proj
]

# Function dvz_camera_mvp()
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

# Function dvz_camera_print()
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

# Function dvz_panzoom_reset()
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

# Function dvz_panzoom_resize()
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

# Function dvz_panzoom_flags()
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

# Function dvz_panzoom_xlim()
panzoom_xlim = dvz.dvz_panzoom_xlim
panzoom_xlim.__doc__ = """
Set a panzoom x limits.

Parameters
----------
pz : DvzPanzoom*
    the panzoom
xlim : vec2
    the xlim (FLOAT_MIN/MAX=no lim)
"""
panzoom_xlim.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
    ctypes.c_float * 2,  # vec2 xlim
]

# Function dvz_panzoom_ylim()
panzoom_ylim = dvz.dvz_panzoom_ylim
panzoom_ylim.__doc__ = """
Set a panzoom y limits.

Parameters
----------
pz : DvzPanzoom*
    the panzoom
ylim : vec2
    the ylim (FLOAT_MIN/MAX=no lim)
"""
panzoom_ylim.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
    ctypes.c_float * 2,  # vec2 ylim
]

# Function dvz_panzoom_zlim()
panzoom_zlim = dvz.dvz_panzoom_zlim
panzoom_zlim.__doc__ = """
Set a panzoom z limits.

Parameters
----------
pz : DvzPanzoom*
    the panzoom
zlim : vec2
    the zlim (FLOAT_MIN/MAX=no lim)
"""
panzoom_zlim.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
    ctypes.c_float * 2,  # vec2 zlim
]

# Function dvz_panzoom_pan()
panzoom_pan = dvz.dvz_panzoom_pan
panzoom_pan.__doc__ = """
Apply a pan value to a panzoom.

Parameters
----------
pz : DvzPanzoom*
    the panzoom
pan : vec2
    the pan, in NDC
"""
panzoom_pan.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
    ctypes.c_float * 2,  # vec2 pan
]

# Function dvz_panzoom_zoom()
panzoom_zoom = dvz.dvz_panzoom_zoom
panzoom_zoom.__doc__ = """
Apply a zoom value to a panzoom.

Parameters
----------
pz : DvzPanzoom*
    the panzoom
zoom : vec2
    the zoom, in NDC
"""
panzoom_zoom.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
    ctypes.c_float * 2,  # vec2 zoom
]

# Function dvz_panzoom_pan_shift()
panzoom_pan_shift = dvz.dvz_panzoom_pan_shift
panzoom_pan_shift.__doc__ = """
Apply a pan shift to a panzoom.

Parameters
----------
pz : DvzPanzoom*
    the panzoom
shift_px : vec2
    the shift value, in pixels
center_px : vec2
    the center position, in pixels
"""
panzoom_pan_shift.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
    ctypes.c_float * 2,  # vec2 shift_px
    ctypes.c_float * 2,  # vec2 center_px
]

# Function dvz_panzoom_zoom_shift()
panzoom_zoom_shift = dvz.dvz_panzoom_zoom_shift
panzoom_zoom_shift.__doc__ = """
Apply a zoom shift to a panzoom.

Parameters
----------
pz : DvzPanzoom*
    the panzoom
shift_px : vec2
    the shift value, in pixels
center_px : vec2
    the center position, in pixels
"""
panzoom_zoom_shift.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
    ctypes.c_float * 2,  # vec2 shift_px
    ctypes.c_float * 2,  # vec2 center_px
]

# Function dvz_panzoom_end()
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

# Function dvz_panzoom_zoom_wheel()
panzoom_zoom_wheel = dvz.dvz_panzoom_zoom_wheel
panzoom_zoom_wheel.__doc__ = """
Apply a wheel zoom to a panzoom.

Parameters
----------
pz : DvzPanzoom*
    the panzoom
dir : vec2
    the wheel direction
center_px : vec2
    the center position, in pixels
"""
panzoom_zoom_wheel.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
    ctypes.c_float * 2,  # vec2 dir
    ctypes.c_float * 2,  # vec2 center_px
]

# Function dvz_panzoom_xrange()
panzoom_xrange = dvz.dvz_panzoom_xrange
panzoom_xrange.__doc__ = """
Get or set the xrange.

Parameters
----------
pz : DvzPanzoom*
    the panzoom
xrange : vec2
    the xrange (get if (0,0), set otherwise)
"""
panzoom_xrange.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
    ctypes.c_float * 2,  # vec2 xrange
]

# Function dvz_panzoom_yrange()
panzoom_yrange = dvz.dvz_panzoom_yrange
panzoom_yrange.__doc__ = """
Get or set the yrange.

Parameters
----------
pz : DvzPanzoom*
    the panzoom
yrange : vec2
    the yrange (get if (0,0), set otherwise)
"""
panzoom_yrange.argtypes = [
    ctypes.POINTER(DvzPanzoom),  # DvzPanzoom* pz
    ctypes.c_float * 2,  # vec2 yrange
]

# Function dvz_panzoom_mvp()
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

# Function dvz_ortho_reset()
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

# Function dvz_ortho_resize()
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

# Function dvz_ortho_flags()
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

# Function dvz_ortho_pan()
ortho_pan = dvz.dvz_ortho_pan
ortho_pan.__doc__ = """
Apply a pan value to an ortho.

Parameters
----------
ortho : DvzOrtho*
    the ortho
pan : vec2
    the pan, in NDC
"""
ortho_pan.argtypes = [
    ctypes.POINTER(DvzOrtho),  # DvzOrtho* ortho
    ctypes.c_float * 2,  # vec2 pan
]

# Function dvz_ortho_zoom()
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

# Function dvz_ortho_pan_shift()
ortho_pan_shift = dvz.dvz_ortho_pan_shift
ortho_pan_shift.__doc__ = """
Apply a pan shift to an ortho.

Parameters
----------
ortho : DvzOrtho*
    the ortho
shift_px : vec2
    the shift value, in pixels
center_px : vec2
    the center position, in pixels
"""
ortho_pan_shift.argtypes = [
    ctypes.POINTER(DvzOrtho),  # DvzOrtho* ortho
    ctypes.c_float * 2,  # vec2 shift_px
    ctypes.c_float * 2,  # vec2 center_px
]

# Function dvz_ortho_zoom_shift()
ortho_zoom_shift = dvz.dvz_ortho_zoom_shift
ortho_zoom_shift.__doc__ = """
Apply a zoom shift to an ortho.

Parameters
----------
ortho : DvzOrtho*
    the ortho
shift_px : vec2
    the shift value, in pixels
center_px : vec2
    the center position, in pixels
"""
ortho_zoom_shift.argtypes = [
    ctypes.POINTER(DvzOrtho),  # DvzOrtho* ortho
    ctypes.c_float * 2,  # vec2 shift_px
    ctypes.c_float * 2,  # vec2 center_px
]

# Function dvz_ortho_end()
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

# Function dvz_ortho_zoom_wheel()
ortho_zoom_wheel = dvz.dvz_ortho_zoom_wheel
ortho_zoom_wheel.__doc__ = """
Apply a wheel zoom to an ortho.

Parameters
----------
ortho : DvzOrtho*
    the ortho
dir : vec2
    the wheel direction
center_px : vec2
    the center position, in pixels
"""
ortho_zoom_wheel.argtypes = [
    ctypes.POINTER(DvzOrtho),  # DvzOrtho* ortho
    ctypes.c_float * 2,  # vec2 dir
    ctypes.c_float * 2,  # vec2 center_px
]

# Function dvz_ortho_mvp()
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

# Function dvz_gui_pos()
gui_pos = dvz.dvz_gui_pos
gui_pos.__doc__ = """
Set the position of the next GUI dialog.

Parameters
----------
pos : vec2
    the dialog position
pivot : vec2
    the pivot
"""
gui_pos.argtypes = [
    ctypes.c_float * 2,  # vec2 pos
    ctypes.c_float * 2,  # vec2 pivot
]

# Function dvz_gui_corner()
gui_corner = dvz.dvz_gui_corner
gui_corner.__doc__ = """
Set the corner position of the next GUI dialog.

Parameters
----------
corner : DvzCorner
    which corner
pad : vec2
    the pad
"""
gui_corner.argtypes = [
    DvzCorner,  # DvzCorner corner
    ctypes.c_float * 2,  # vec2 pad
]

# Function dvz_gui_size()
gui_size = dvz.dvz_gui_size
gui_size.__doc__ = """
Set the size of the next GUI dialog.

Parameters
----------
size : vec2
    the size
"""
gui_size.argtypes = [
    ctypes.c_float * 2,  # vec2 size
]

# Function dvz_gui_flags()
gui_flags = dvz.dvz_gui_flags
gui_flags.__doc__ = """
Set the flags of the next GUI dialog.

Parameters
----------
flags : int
    the flags
"""
gui_flags.argtypes = [
    ctypes.c_int,  # int flags
]
gui_flags.restype = ctypes.c_int

# Function dvz_gui_alpha()
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

# Function dvz_gui_begin()
gui_begin = dvz.dvz_gui_begin
gui_begin.__doc__ = """
Start a new dialog.

Parameters
----------
title : char*
    the dialog title
flags : int
    the flags
"""
gui_begin.argtypes = [
    ctypes.c_char_p,  # char* title
    ctypes.c_int,  # int flags
]

# Function dvz_gui_slider()
gui_slider = dvz.dvz_gui_slider
gui_slider.__doc__ = """
Add a slider.

Parameters
----------
name : char*
    the slider name
vmin : float
    the minimum value
vmax : float
    the maximum value
value : float*
    the pointer to the value

Returns
-------
type
    whether the value has changed
"""
gui_slider.argtypes = [
    ctypes.c_char_p,  # char* name
    ctypes.c_float,  # float vmin
    ctypes.c_float,  # float vmax
    ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # float* value
]
gui_slider.restype = ctypes.c_bool

# Function dvz_gui_button()
gui_button = dvz.dvz_gui_button
gui_button.__doc__ = """
Add a button.

Parameters
----------
name : char*
    the button name
width : float
    the button width
height : float
    the button height

Returns
-------
type
    whether the button was pressed
"""
gui_button.argtypes = [
    ctypes.c_char_p,  # char* name
    ctypes.c_float,  # float width
    ctypes.c_float,  # float height
]
gui_button.restype = ctypes.c_bool

# Function dvz_gui_checkbox()
gui_checkbox = dvz.dvz_gui_checkbox
gui_checkbox.__doc__ = """
Add a checkbox.

Parameters
----------
name : char*
    the button name
checked : bool* (out parameter)
    whether the checkbox is checked

Returns
-------
type
    whether the checkbox's state has changed
"""
gui_checkbox.argtypes = [
    ctypes.c_char_p,  # char* name
    ndpointer(dtype=bool, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # bool* checked
]
gui_checkbox.restype = ctypes.c_bool

# Function dvz_gui_image()
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

# Function dvz_gui_colorpicker()
gui_colorpicker = dvz.dvz_gui_colorpicker
gui_colorpicker.__doc__ = """
Add a color picker

Parameters
----------
name : char*
    the widget name
color : vec3
    the color
flags : int
    the widget flags
"""
gui_colorpicker.argtypes = [
    ctypes.c_char_p,  # char* name
    ctypes.c_float * 3,  # vec3 color
    ctypes.c_int,  # int flags
]
gui_colorpicker.restype = ctypes.c_bool

# Function dvz_gui_demo()
gui_demo = dvz.dvz_gui_demo
gui_demo.__doc__ = """
Show the demo GUI.
"""
gui_demo.argtypes = [
]

# Function dvz_gui_end()
gui_end = dvz.dvz_gui_end
gui_end.__doc__ = """
Stop the creation of the dialog.
"""
gui_end.argtypes = [
]

# Function dvz_next_pow2()
next_pow2 = dvz.dvz_next_pow2
next_pow2.__doc__ = """
Return the smallest power of 2 larger or equal than a positive integer.

Parameters
----------
x : uint64_t
    the value

Returns
-------
type
    the power of 2
"""
next_pow2.argtypes = [
    ctypes.c_uint64,  # uint64_t x
]
next_pow2.restype = ctypes.c_uint64

# Function dvz_mean()
mean = dvz.dvz_mean
mean.__doc__ = """
Compute the mean of an array of double values.

Parameters
----------
n : uint32_t
    the number of values
values : double*
    an array of double numbers

Returns
-------
type
    the mean
"""
mean.argtypes = [
    ctypes.c_uint32,  # uint32_t n
    ndpointer(dtype=np.double, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # double* values
]
mean.restype = ctypes.c_double

# Function dvz_min_max()
min_max = dvz.dvz_min_max
min_max.__doc__ = """
Compute the min and max of an array of float values.

Parameters
----------
n : uint32_t
    the number of values
values : float*
    an array of float numbers
out_min_max : vec2
    the min and max

Returns
-------
type
    the mean
"""
min_max.argtypes = [
    ctypes.c_uint32,  # uint32_t n
    ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # float* values
    ctypes.c_float * 2,  # vec2 out_min_max
]

# Function dvz_normalize_bytes()
normalize_bytes = dvz.dvz_normalize_bytes
normalize_bytes.__doc__ = """
Normalize the array.

Parameters
----------
count : uint32_t
    the number of values
values : float*
    an array of float numbers

Returns
-------
type
    the normalized array
"""
normalize_bytes.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # float* values
]
normalize_bytes.restype = ndpointer(dtype=np.uint8, ndim=1, ncol=1, flags="C_CONTIGUOUS")

# Function dvz_range()
range = dvz.dvz_range
range.__doc__ = """
Compute the range of an array of double values.

Parameters
----------
n : uint32_t
    the number of values
values : double*
    an array of double numbers
the : unknown (out parameter)
    min and max values
"""
range.argtypes = [
    ctypes.c_uint32,  # uint32_t n
    ndpointer(dtype=np.double, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # double* values
    ctypes.c_double * 2,  # dvec2 min_max
]

# Function dvz_earcut()
earcut = dvz.dvz_earcut
earcut.__doc__ = """
Compute a polygon triangulation with only indexing on the polygon contour vertices.

Parameters
----------
point_count : uint32_t
    the number of points
polygon : dvec2*
    the polygon 2D positions
out_index_count : uint32_t* (out parameter)
    the computed index count

Returns
-------
type
    the computed indices (must be FREED by the caller)
"""
earcut.argtypes = [
    ctypes.c_uint32,  # uint32_t point_count
    ndpointer(dtype=np.double, ndim=2, ncol=2, flags="C_CONTIGUOUS"),  # dvec2* polygon
    ndpointer(dtype=np.uint32, ndim=1, ncol=1, flags="C_CONTIGUOUS"),  # uint32_t* out_index_count
]
earcut.restype = ndpointer(dtype=np.uint32, ndim=1, ncol=1, flags="C_CONTIGUOUS")

# Function dvz_rand_byte()
rand_byte = dvz.dvz_rand_byte
rand_byte.__doc__ = """
Return a random integer number between 0 and 255.


Returns
-------
type
    random number
"""
rand_byte.argtypes = [
]
rand_byte.restype = ctypes.c_uint8

# Function dvz_rand_int()
rand_int = dvz.dvz_rand_int
rand_int.__doc__ = """
Return a random integer number.


Returns
-------
type
    random number
"""
rand_int.argtypes = [
]
rand_int.restype = ctypes.c_int

# Function dvz_rand_float()
rand_float = dvz.dvz_rand_float
rand_float.__doc__ = """
Return a random floating-point number between 0 and 1.


Returns
-------
type
    random number
"""
rand_float.argtypes = [
]
rand_float.restype = ctypes.c_float

# Function dvz_rand_double()
rand_double = dvz.dvz_rand_double
rand_double.__doc__ = """
Return a random floating-point number between 0 and 1.


Returns
-------
type
    random number
"""
rand_double.argtypes = [
]
rand_double.restype = ctypes.c_double

# Function dvz_rand_normal()
rand_normal = dvz.dvz_rand_normal
rand_normal.__doc__ = """
Return a random normal floating-point number.


Returns
-------
type
    random number
"""
rand_normal.argtypes = [
]
rand_normal.restype = ctypes.c_double

# Function dvz_mock_pos2D()
mock_pos2D = dvz.dvz_mock_pos2D
mock_pos2D.__doc__ = """
Generate a set of random 2D positions.

Parameters
----------
count : uint32_t
    the number of positions to generate
std : float
    the standard deviation

Returns
-------
type
    the positions
"""
mock_pos2D.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    ctypes.c_float,  # float std
]
mock_pos2D.restype = ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS")

# Function dvz_mock_circle()
mock_circle = dvz.dvz_mock_circle
mock_circle.__doc__ = """
Generate points on a circle.

Parameters
----------
count : uint32_t
    the number of positions to generate
radius : float
    the radius of the circle

Returns
-------
type
    the positions
"""
mock_circle.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    ctypes.c_float,  # float radius
]
mock_circle.restype = ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS")

# Function dvz_mock_band()
mock_band = dvz.dvz_mock_band
mock_band.__doc__ = """
Generate points on a band.

Parameters
----------
count : uint32_t
    the number of positions to generate
size : vec2
    the size of the band

Returns
-------
type
    the positions
"""
mock_band.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    ctypes.c_float * 2,  # vec2 size
]
mock_band.restype = ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS")

# Function dvz_mock_pos3D()
mock_pos3D = dvz.dvz_mock_pos3D
mock_pos3D.__doc__ = """
Generate a set of random 3D positions.

Parameters
----------
count : uint32_t
    the number of positions to generate
std : float
    the standard deviation

Returns
-------
type
    the positions
"""
mock_pos3D.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    ctypes.c_float,  # float std
]
mock_pos3D.restype = ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS")

# Function dvz_mock_fixed()
mock_fixed = dvz.dvz_mock_fixed
mock_fixed.__doc__ = """
Generate identical 3D positions.

Parameters
----------
count : uint32_t
    the number of positions to generate
fixed : vec3
    the position

Returns
-------
type
    the repeated positions
"""
mock_fixed.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    ctypes.c_float * 3,  # vec3 fixed
]
mock_fixed.restype = ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS")

# Function dvz_mock_line()
mock_line = dvz.dvz_mock_line
mock_line.__doc__ = """
Generate 3D positions on a line.

Parameters
----------
count : uint32_t
    the number of positions to generate
p0 : vec3
    initial position
p1 : vec3
    terminal position

Returns
-------
type
    the positions
"""
mock_line.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    ctypes.c_float * 3,  # vec3 p0
    ctypes.c_float * 3,  # vec3 p1
]
mock_line.restype = ndpointer(dtype=np.float32, ndim=2, ncol=3, flags="C_CONTIGUOUS")

# Function dvz_mock_uniform()
mock_uniform = dvz.dvz_mock_uniform
mock_uniform.__doc__ = """
Generate a set of uniformly random scalar values.

Parameters
----------
count : uint32_t
    the number of values to generate
vmin : float
    the minimum value of the interval
vmax : float
    the maximum value of the interval

Returns
-------
type
    the values
"""
mock_uniform.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    ctypes.c_float,  # float vmin
    ctypes.c_float,  # float vmax
]
mock_uniform.restype = ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS")

# Function dvz_mock_full()
mock_full = dvz.dvz_mock_full
mock_full.__doc__ = """
Generate an array with the same value.

Parameters
----------
count : uint32_t
    the number of scalars to generate
value : float
    the value

Returns
-------
type
    the values
"""
mock_full.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    ctypes.c_float,  # float value
]
mock_full.restype = ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS")

# Function dvz_mock_range()
mock_range = dvz.dvz_mock_range
mock_range.__doc__ = """
Generate an array of consecutive positive numbers.

Parameters
----------
count : uint32_t
    the number of consecutive integers to generate
initial : uint32_t
    the initial value

Returns
-------
type
    the values
"""
mock_range.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    ctypes.c_uint32,  # uint32_t initial
]
mock_range.restype = ndpointer(dtype=np.uint32, ndim=1, ncol=1, flags="C_CONTIGUOUS")

# Function dvz_mock_linspace()
mock_linspace = dvz.dvz_mock_linspace
mock_linspace.__doc__ = """
Generate an array ranging from an initial value to a final value.

Parameters
----------
count : uint32_t
    the number of scalars to generate
initial : float
    the initial value
final : float
    the final value

Returns
-------
type
    the values
"""
mock_linspace.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    ctypes.c_float,  # float initial
    ctypes.c_float,  # float final
]
mock_linspace.restype = ndpointer(dtype=np.float32, ndim=1, ncol=1, flags="C_CONTIGUOUS")

# Function dvz_mock_color()
mock_color = dvz.dvz_mock_color
mock_color.__doc__ = """
Generate a set of random colors.

Parameters
----------
count : uint32_t
    the number of colors to generate
alpha : uint8_t
    the alpha value

Returns
-------
type
    random colors
"""
mock_color.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    ctypes.c_uint8,  # uint8_t alpha
]
mock_color.restype = ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS")

# Function dvz_mock_monochrome()
mock_monochrome = dvz.dvz_mock_monochrome
mock_monochrome.__doc__ = """
Repeat a color in an array.

Parameters
----------
count : uint32_t
    the number of colors to generate
mono : cvec4
    the color to repeat

Returns
-------
type
    colors
"""
mock_monochrome.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    ctypes.c_uint8 * 4,  # cvec4 mono
]
mock_monochrome.restype = ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS")

# Function dvz_mock_cmap()
mock_cmap = dvz.dvz_mock_cmap
mock_cmap.__doc__ = """
Generate a set of HSV colors.

Parameters
----------
count : uint32_t
    the number of colors to generate
alpha : uint8_t
    the alpha value

Returns
-------
type
    colors
"""
mock_cmap.argtypes = [
    ctypes.c_uint32,  # uint32_t count
    DvzColormap,  # DvzColormap cmap
    ctypes.c_uint8,  # uint8_t alpha
]
mock_cmap.restype = ndpointer(dtype=np.uint8, ndim=2, ncol=4, flags="C_CONTIGUOUS")

