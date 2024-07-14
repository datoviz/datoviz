'''
WARNING: DO NOT EDIT: automatically-generated file
'''

# ===============================================================================
# Imports
# ===============================================================================

import ctypes
from enum import IntEnum
import faulthandler
import pathlib


# ===============================================================================
# Fault handler
# ===============================================================================

faulthandler.enable()


# ===============================================================================
# Global variables
# ===============================================================================

ROOT_DIR = pathlib.Path(__file__).parent.parent


# ===============================================================================
# Loading the dynamic library
# ===============================================================================

dvz = ctypes.cdll.LoadLibrary(ROOT_DIR / 'build/libdatoviz.so')


# ===============================================================================
# Util classes
# ===============================================================================

# see https://v4.chriskrycho.com/2015/ctypes-structures-and-dll-exports.html
class CtypesEnum(IntEnum):
    @classmethod
    def from_param(cls, obj):
        return int(obj)


# ===============================================================================
# DEFINES
# ===============================================================================

DATOVIZ_VERSION_MINOR = 2
M_PI = 3.141592653589793
M_2PI = 6.283185307179586
M_PI2 = 1.5707963267948966
M_INV_255 = 0.00392156862745098
GB = 1073741824
MB = 1048576
KB = 1024
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


# ===============================================================================
# ENUMERATIONS
# ===============================================================================

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
    DVZ_KEYBOARD_EVENT_RELEASE = 2

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
    DVZ_FORMAT_R8G8B8_UNORM = 23
    DVZ_FORMAT_R8G8B8A8_UNORM = 37
    DVZ_FORMAT_R8G8B8A8_UINT = 41
    DVZ_FORMAT_B8G8R8A8_UNORM = 44
    DVZ_FORMAT_R16_UNORM = 70
    DVZ_FORMAT_R16_SNORM = 71
    DVZ_FORMAT_R32_UINT = 98
    DVZ_FORMAT_R32_SINT = 99
    DVZ_FORMAT_R32_SFLOAT = 100
    DVZ_FORMAT_R32G32_SFLOAT = 103
    DVZ_FORMAT_R32G32B32_SFLOAT = 106
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
    DVZ_MARKER_SHAPE_COUNT = 19

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
    DVZ_CAP_TYPE_NONE = 0
    DVZ_CAP_ROUND = 1
    DVZ_CAP_TRIANGLE_IN = 2
    DVZ_CAP_TRIANGLE_OUT = 3
    DVZ_CAP_SQUARE = 4
    DVZ_CAP_BUTT = 5
    DVZ_CAP_COUNT = 6

class DvzJoinType(CtypesEnum):
    DVZ_JOIN_SQUARE = 0
    DVZ_JOIN_ROUND = 1

class DvzPathTopology(CtypesEnum):
    DVZ_PATH_OPEN = 0
    DVZ_PATH_CLOSED = 1

class DvzShapeType(CtypesEnum):
    DVZ_SHAPE_NONE = 0
    DVZ_SHAPE_SQUARE = 1
    DVZ_SHAPE_DISC = 2
    DVZ_SHAPE_CUBE = 3
    DVZ_SHAPE_SPHERE = 4
    DVZ_SHAPE_CYLINDER = 5
    DVZ_SHAPE_CONE = 6
    DVZ_SHAPE_OBJ = 7
    DVZ_SHAPE_OTHER = 8

class DvzMeshFlags(CtypesEnum):
    DVZ_MESH_FLAGS_NONE = 0x0000
    DVZ_MESH_FLAGS_TEXTURED = 0x0001
    DVZ_MESH_FLAGS_LIGHTING = 0x0002

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

class DvzMVP(ctypes.Structure):
    pass

class DvzPanel(ctypes.Structure):
    pass

class DvzPanzoom(ctypes.Structure):
    pass

class DvzScene(ctypes.Structure):
    pass

class DvzShape(ctypes.Structure):
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

class DvzShape(ctypes.Structure):
    _fields_ = [
        ("type", ctypes.c_int32),
        ("vertex_count", ctypes.c_uint32),
        ("index_count", ctypes.c_uint32),
        ("pos", ctypes.POINTER(ctypes.c_float)),
        ("normal", ctypes.POINTER(ctypes.c_float)),
        ("color", ctypes.POINTER(ctypes.c_uint8)),
        ("texcoords", ctypes.POINTER(ctypes.c_float)),
        ("index", ctypes.POINTER(ctypes.c_uint32)),
    ]

class DvzKeyboardEvent(ctypes.Structure):
    _fields_ = [
        ("type", ctypes.c_int32),
        ("key", ctypes.c_int32),
        ("mods", ctypes.c_int),
        ("user_data", ctypes.c_void_p),
    ]

class DvzMouseButtonEvent(ctypes.Structure):
    _fields_ = [
        ("button", ctypes.c_int32),
    ]

class DvzMouseWheelEvent(ctypes.Structure):
    _fields_ = [
        ("dir", ctypes.c_float * 2),
    ]

class DvzMouseDragEvent(ctypes.Structure):
    _fields_ = [
        ("button", ctypes.c_int32),
        ("cur_pos", ctypes.c_float * 2),
        ("shift", ctypes.c_float * 2),
    ]

class DvzMouseClickEvent(ctypes.Structure):
    _fields_ = [
        ("button", ctypes.c_int32),
    ]

class DvzMouseEventUnion(ctypes.Structure):
    _fields_ = [
        ("b", DvzMouseButtonEvent),
        ("w", DvzMouseWheelEvent),
        ("d", DvzMouseDragEvent),
        ("c", DvzMouseClickEvent),
    ]

class DvzMouseEvent(ctypes.Structure):
    _fields_ = [
        ("type", ctypes.c_int32),
        ("content", DvzMouseEventUnion),
        ("pos", ctypes.c_float * 2),
        ("mods", ctypes.c_int),
        ("content_scale", ctypes.c_float),
        ("user_data", ctypes.c_void_p),
    ]

class DvzWindowEvent(ctypes.Structure):
    _fields_ = [
        ("framebuffer_width", ctypes.c_uint32),
        ("framebuffer_height", ctypes.c_uint32),
        ("screen_width", ctypes.c_uint32),
        ("screen_height", ctypes.c_uint32),
        ("flags", ctypes.c_int),
        ("user_data", ctypes.c_void_p),
    ]

class DvzFrameEvent(ctypes.Structure):
    _fields_ = [
        ("frame_idx", ctypes.c_uint64),
        ("time", ctypes.c_double),
        ("interval", ctypes.c_double),
        ("user_data", ctypes.c_void_p),
    ]

class DvzTimerEvent(ctypes.Structure):
    _fields_ = [
        ("timer_idx", ctypes.c_uint32),
        ("timer_item", ctypes.POINTER(DvzTimerItem)),
        ("step_idx", ctypes.c_uint64),
        ("time", ctypes.c_double),
        ("user_data", ctypes.c_void_p),
    ]

class DvzRequestsEvent(ctypes.Structure):
    _fields_ = [
        ("batch", ctypes.POINTER(DvzBatch)),
        ("user_data", ctypes.c_void_p),
    ]


# ===============================================================================
# FUNCTIONS
# ===============================================================================

# Function dvz_app()
app = dvz.dvz_app
app.argtypes = [
    ctypes.c_int, # int flags
]
app.restype = ctypes.POINTER(DvzApp)

# Function dvz_app_batch()
app_batch = dvz.dvz_app_batch
app_batch.argtypes = [
    ctypes.POINTER(DvzApp), # DvzApp* app
]
app_batch.restype = ctypes.POINTER(DvzBatch)

# Function dvz_app_frame()
app_frame = dvz.dvz_app_frame
app_frame.argtypes = [
    ctypes.POINTER(DvzApp), # DvzApp* app
]

# Function dvz_app_onframe()
app_onframe = dvz.dvz_app_onframe
app_onframe.argtypes = [
    ctypes.POINTER(DvzApp), # DvzApp* app
    ctypes.c_void_p, # DvzAppFrameCallback callback
    ctypes.c_void_p, # void* user_data
]

# Function dvz_app_onmouse()
app_onmouse = dvz.dvz_app_onmouse
app_onmouse.argtypes = [
    ctypes.POINTER(DvzApp), # DvzApp* app
    ctypes.c_void_p, # DvzAppMouseCallback callback
    ctypes.c_void_p, # void* user_data
]

# Function dvz_app_onkeyboard()
app_onkeyboard = dvz.dvz_app_onkeyboard
app_onkeyboard.argtypes = [
    ctypes.POINTER(DvzApp), # DvzApp* app
    ctypes.c_void_p, # DvzAppKeyboardCallback callback
    ctypes.c_void_p, # void* user_data
]

# Function dvz_app_onresize()
app_onresize = dvz.dvz_app_onresize
app_onresize.argtypes = [
    ctypes.POINTER(DvzApp), # DvzApp* app
    ctypes.c_void_p, # DvzAppResizeCallback callback
    ctypes.c_void_p, # void* user_data
]

# Function dvz_app_timer()
app_timer = dvz.dvz_app_timer
app_timer.argtypes = [
    ctypes.POINTER(DvzApp), # DvzApp* app
    ctypes.c_double, # double delay
    ctypes.c_double, # double period
    ctypes.c_uint64, # uint64_t max_count
]
app_timer.restype = ctypes.POINTER(DvzTimerItem)

# Function dvz_app_ontimer()
app_ontimer = dvz.dvz_app_ontimer
app_ontimer.argtypes = [
    ctypes.POINTER(DvzApp), # DvzApp* app
    ctypes.c_void_p, # DvzAppTimerCallback callback
    ctypes.c_void_p, # void* user_data
]

# Function dvz_app_gui()
app_gui = dvz.dvz_app_gui
app_gui.argtypes = [
    ctypes.POINTER(DvzApp), # DvzApp* app
    ctypes.c_uint64, # DvzId canvas_id
    ctypes.c_void_p, # DvzAppGui callback
    ctypes.c_void_p, # void* user_data
]

# Function dvz_app_run()
app_run = dvz.dvz_app_run
app_run.argtypes = [
    ctypes.POINTER(DvzApp), # DvzApp* app
    ctypes.c_uint64, # uint64_t n_frames
]

# Function dvz_app_submit()
app_submit = dvz.dvz_app_submit
app_submit.argtypes = [
    ctypes.POINTER(DvzApp), # DvzApp* app
]

# Function dvz_app_screenshot()
app_screenshot = dvz.dvz_app_screenshot
app_screenshot.argtypes = [
    ctypes.POINTER(DvzApp), # DvzApp* app
    ctypes.c_uint64, # DvzId canvas_id
    ctypes.c_char_p, # char* filename
]

# Function dvz_app_destroy()
app_destroy = dvz.dvz_app_destroy
app_destroy.argtypes = [
    ctypes.POINTER(DvzApp), # DvzApp* app
]

# Function dvz_free()
free = dvz.dvz_free
free.argtypes = [
    ctypes.c_void_p, # void* pointer
]

# Function dvz_scene()
scene = dvz.dvz_scene
scene.argtypes = [
    ctypes.POINTER(DvzBatch), # DvzBatch* batch
]
scene.restype = ctypes.POINTER(DvzScene)

# Function dvz_scene_run()
scene_run = dvz.dvz_scene_run
scene_run.argtypes = [
    ctypes.POINTER(DvzScene), # DvzScene* scene
    ctypes.POINTER(DvzApp), # DvzApp* app
    ctypes.c_uint64, # uint64_t n_frames
]

# Function dvz_scene_destroy()
scene_destroy = dvz.dvz_scene_destroy
scene_destroy.argtypes = [
    ctypes.POINTER(DvzScene), # DvzScene* scene
]

# Function dvz_figure()
figure = dvz.dvz_figure
figure.argtypes = [
    ctypes.POINTER(DvzScene), # DvzScene* scene
    ctypes.c_uint32, # uint32_t width
    ctypes.c_uint32, # uint32_t height
    ctypes.c_int, # int flags
]
figure.restype = ctypes.POINTER(DvzFigure)

# Function dvz_figure_resize()
figure_resize = dvz.dvz_figure_resize
figure_resize.argtypes = [
    ctypes.POINTER(DvzFigure), # DvzFigure* fig
    ctypes.c_uint32, # uint32_t width
    ctypes.c_uint32, # uint32_t height
]

# Function dvz_scene_figure()
scene_figure = dvz.dvz_scene_figure
scene_figure.argtypes = [
    ctypes.POINTER(DvzScene), # DvzScene* scene
    ctypes.c_uint64, # DvzId id
]
scene_figure.restype = ctypes.POINTER(DvzFigure)

# Function dvz_figure_destroy()
figure_destroy = dvz.dvz_figure_destroy
figure_destroy.argtypes = [
    ctypes.POINTER(DvzFigure), # DvzFigure* figure
]

# Function dvz_panel()
panel = dvz.dvz_panel
panel.argtypes = [
    ctypes.POINTER(DvzFigure), # DvzFigure* fig
    ctypes.c_float, # float x
    ctypes.c_float, # float y
    ctypes.c_float, # float width
    ctypes.c_float, # float height
]
panel.restype = ctypes.POINTER(DvzPanel)

# Function dvz_panel_default()
panel_default = dvz.dvz_panel_default
panel_default.argtypes = [
    ctypes.POINTER(DvzFigure), # DvzFigure* fig
]
panel_default.restype = ctypes.POINTER(DvzPanel)

# Function dvz_panel_transform()
panel_transform = dvz.dvz_panel_transform
panel_transform.argtypes = [
    ctypes.POINTER(DvzPanel), # DvzPanel* panel
    ctypes.POINTER(DvzTransform), # DvzTransform* tr
]

# Function dvz_panel_resize()
panel_resize = dvz.dvz_panel_resize
panel_resize.argtypes = [
    ctypes.POINTER(DvzPanel), # DvzPanel* panel
    ctypes.c_float, # float x
    ctypes.c_float, # float y
    ctypes.c_float, # float width
    ctypes.c_float, # float height
]

# Function dvz_panel_margins()
panel_margins = dvz.dvz_panel_margins
panel_margins.argtypes = [
    ctypes.POINTER(DvzPanel), # DvzPanel* panel
    ctypes.c_float, # float top
    ctypes.c_float, # float right
    ctypes.c_float, # float bottom
    ctypes.c_float, # float left
]

# Function dvz_panel_contains()
panel_contains = dvz.dvz_panel_contains
panel_contains.argtypes = [
    ctypes.POINTER(DvzPanel), # DvzPanel* panel
    ctypes.c_float * 2, # vec2 pos
]
panel_contains.restype = ctypes.c_bool

# Function dvz_panel_at()
panel_at = dvz.dvz_panel_at
panel_at.argtypes = [
    ctypes.POINTER(DvzFigure), # DvzFigure* figure
    ctypes.c_float * 2, # vec2 pos
]
panel_at.restype = ctypes.POINTER(DvzPanel)

# Function dvz_panel_camera()
panel_camera = dvz.dvz_panel_camera
panel_camera.argtypes = [
    ctypes.POINTER(DvzPanel), # DvzPanel* panel
]
panel_camera.restype = ctypes.POINTER(DvzCamera)

# Function dvz_panel_panzoom()
panel_panzoom = dvz.dvz_panel_panzoom
panel_panzoom.argtypes = [
    ctypes.POINTER(DvzScene), # DvzScene* scene
    ctypes.POINTER(DvzPanel), # DvzPanel* panel
]
panel_panzoom.restype = ctypes.POINTER(DvzPanzoom)

# Function dvz_panel_arcball()
panel_arcball = dvz.dvz_panel_arcball
panel_arcball.argtypes = [
    ctypes.POINTER(DvzScene), # DvzScene* scene
    ctypes.POINTER(DvzPanel), # DvzPanel* panel
]
panel_arcball.restype = ctypes.POINTER(DvzArcball)

# Function dvz_panel_update()
panel_update = dvz.dvz_panel_update
panel_update.argtypes = [
    ctypes.POINTER(DvzPanel), # DvzPanel* panel
]

# Function dvz_panel_visual()
panel_visual = dvz.dvz_panel_visual
panel_visual.argtypes = [
    ctypes.POINTER(DvzPanel), # DvzPanel* panel
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
]

# Function dvz_panel_destroy()
panel_destroy = dvz.dvz_panel_destroy
panel_destroy.argtypes = [
    ctypes.POINTER(DvzPanel), # DvzPanel* panel
]

# Function dvz_visual_fixed()
visual_fixed = dvz.dvz_visual_fixed
visual_fixed.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_bool, # bool fixed_x
    ctypes.c_bool, # bool fixed_y
    ctypes.c_bool, # bool fixed_z
]

# Function dvz_visual_clip()
visual_clip = dvz.dvz_visual_clip
visual_clip.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    DvzViewportClip, # DvzViewportClip clip
]

# Function dvz_visual_show()
visual_show = dvz.dvz_visual_show
visual_show.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_bool, # bool is_visible
]

# Function dvz_colormap()
colormap = dvz.dvz_colormap
colormap.argtypes = [
    DvzColormap, # DvzColormap cmap
    ctypes.c_uint8, # uint8_t value
    ctypes.c_int8 * 4, # cvec4 color
]

# Function dvz_colormap_scale()
colormap_scale = dvz.dvz_colormap_scale
colormap_scale.argtypes = [
    DvzColormap, # DvzColormap cmap
    ctypes.c_double, # double value
    ctypes.c_double, # double vmin
    ctypes.c_double, # double vmax
    ctypes.c_int8 * 4, # cvec4 color
]

# Function dvz_shape_print()
shape_print = dvz.dvz_shape_print
shape_print.argtypes = [
    ctypes.POINTER(DvzShape), # DvzShape* shape
]

# Function dvz_shape_destroy()
shape_destroy = dvz.dvz_shape_destroy
shape_destroy.argtypes = [
    ctypes.POINTER(DvzShape), # DvzShape* shape
]

# Function dvz_shape_square()
shape_square = dvz.dvz_shape_square
shape_square.argtypes = [
    ctypes.c_int8 * 4, # cvec4 color
]
shape_square.restype = DvzShape

# Function dvz_shape_disc()
shape_disc = dvz.dvz_shape_disc
shape_disc.argtypes = [
    ctypes.c_uint32, # uint32_t count
    ctypes.c_int8 * 4, # cvec4 color
]
shape_disc.restype = DvzShape

# Function dvz_shape_cube()
shape_cube = dvz.dvz_shape_cube
shape_cube.argtypes = [
    ctypes.POINTER(ctypes.c_uint8), # cvec4* colors
]
shape_cube.restype = DvzShape

# Function dvz_shape_sphere()
shape_sphere = dvz.dvz_shape_sphere
shape_sphere.argtypes = [
    ctypes.c_uint32, # uint32_t rows
    ctypes.c_uint32, # uint32_t cols
    ctypes.c_int8 * 4, # cvec4 color
]
shape_sphere.restype = DvzShape

# Function dvz_shape_cone()
shape_cone = dvz.dvz_shape_cone
shape_cone.argtypes = [
    ctypes.c_uint32, # uint32_t count
    ctypes.c_int8 * 4, # cvec4 color
]
shape_cone.restype = DvzShape

# Function dvz_shape_cylinder()
shape_cylinder = dvz.dvz_shape_cylinder
shape_cylinder.argtypes = [
    ctypes.c_uint32, # uint32_t count
    ctypes.c_int8 * 4, # cvec4 color
]
shape_cylinder.restype = DvzShape

# Function dvz_shape_normalize()
shape_normalize = dvz.dvz_shape_normalize
shape_normalize.argtypes = [
    ctypes.POINTER(DvzShape), # DvzShape* shape
]

# Function dvz_shape_obj()
shape_obj = dvz.dvz_shape_obj
shape_obj.argtypes = [
    ctypes.c_char_p, # char* file_path
]
shape_obj.restype = DvzShape

# Function dvz_basic()
basic = dvz.dvz_basic
basic.argtypes = [
    ctypes.POINTER(DvzBatch), # DvzBatch* batch
    DvzPrimitiveTopology, # DvzPrimitiveTopology topology
    ctypes.c_int, # int flags
]
basic.restype = ctypes.POINTER(DvzVisual)

# Function dvz_basic_position()
basic_position = dvz.dvz_basic_position
basic_position.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float), # vec3* values
    ctypes.c_int, # int flags
]

# Function dvz_basic_color()
basic_color = dvz.dvz_basic_color
basic_color.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_uint8), # cvec4* values
    ctypes.c_int, # int flags
]

# Function dvz_basic_alloc()
basic_alloc = dvz.dvz_basic_alloc
basic_alloc.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t item_count
]

# Function dvz_pixel()
pixel = dvz.dvz_pixel
pixel.argtypes = [
    ctypes.POINTER(DvzBatch), # DvzBatch* batch
    ctypes.c_int, # int flags
]
pixel.restype = ctypes.POINTER(DvzVisual)

# Function dvz_pixel_position()
pixel_position = dvz.dvz_pixel_position
pixel_position.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float), # vec3* values
    ctypes.c_int, # int flags
]

# Function dvz_pixel_color()
pixel_color = dvz.dvz_pixel_color
pixel_color.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_uint8), # cvec4* values
    ctypes.c_int, # int flags
]

# Function dvz_pixel_alloc()
pixel_alloc = dvz.dvz_pixel_alloc
pixel_alloc.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t item_count
]

# Function dvz_point()
point = dvz.dvz_point
point.argtypes = [
    ctypes.POINTER(DvzBatch), # DvzBatch* batch
    ctypes.c_int, # int flags
]
point.restype = ctypes.POINTER(DvzVisual)

# Function dvz_point_position()
point_position = dvz.dvz_point_position
point_position.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float), # vec3* values
    ctypes.c_int, # int flags
]

# Function dvz_point_color()
point_color = dvz.dvz_point_color
point_color.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_uint8), # cvec4* values
    ctypes.c_int, # int flags
]

# Function dvz_point_size()
point_size = dvz.dvz_point_size
point_size.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float), # float* values
    ctypes.c_int, # int flags
]

# Function dvz_point_alloc()
point_alloc = dvz.dvz_point_alloc
point_alloc.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t item_count
]

# Function dvz_marker()
marker = dvz.dvz_marker
marker.argtypes = [
    ctypes.POINTER(DvzBatch), # DvzBatch* batch
    ctypes.c_int, # int flags
]
marker.restype = ctypes.POINTER(DvzVisual)

# Function dvz_marker_mode()
marker_mode = dvz.dvz_marker_mode
marker_mode.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    DvzMarkerMode, # DvzMarkerMode mode
]

# Function dvz_marker_aspect()
marker_aspect = dvz.dvz_marker_aspect
marker_aspect.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    DvzMarkerAspect, # DvzMarkerAspect aspect
]

# Function dvz_marker_shape()
marker_shape = dvz.dvz_marker_shape
marker_shape.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    DvzMarkerShape, # DvzMarkerShape shape
]

# Function dvz_marker_position()
marker_position = dvz.dvz_marker_position
marker_position.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float), # vec3* values
    ctypes.c_int, # int flags
]

# Function dvz_marker_size()
marker_size = dvz.dvz_marker_size
marker_size.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float), # float* values
    ctypes.c_int, # int flags
]

# Function dvz_marker_angle()
marker_angle = dvz.dvz_marker_angle
marker_angle.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float), # float* values
    ctypes.c_int, # int flags
]

# Function dvz_marker_color()
marker_color = dvz.dvz_marker_color
marker_color.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_uint8), # cvec4* values
    ctypes.c_int, # int flags
]

# Function dvz_marker_edge_color()
marker_edge_color = dvz.dvz_marker_edge_color
marker_edge_color.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_int8 * 4, # cvec4 value
]

# Function dvz_marker_edge_width()
marker_edge_width = dvz.dvz_marker_edge_width
marker_edge_width.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_float, # float value
]

# Function dvz_marker_tex()
marker_tex = dvz.dvz_marker_tex
marker_tex.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint64, # DvzId tex
    ctypes.c_uint64, # DvzId sampler
]

# Function dvz_marker_tex_scale()
marker_tex_scale = dvz.dvz_marker_tex_scale
marker_tex_scale.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_float, # float value
]

# Function dvz_marker_alloc()
marker_alloc = dvz.dvz_marker_alloc
marker_alloc.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t item_count
]

# Function dvz_segment()
segment = dvz.dvz_segment
segment.argtypes = [
    ctypes.POINTER(DvzBatch), # DvzBatch* batch
    ctypes.c_int, # int flags
]
segment.restype = ctypes.POINTER(DvzVisual)

# Function dvz_segment_position()
segment_position = dvz.dvz_segment_position
segment_position.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* segment
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float), # vec3* initial
    ctypes.POINTER(ctypes.c_float), # vec3* terminal
    ctypes.c_int, # int flags
]

# Function dvz_segment_shift()
segment_shift = dvz.dvz_segment_shift
segment_shift.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float), # vec4* values
    ctypes.c_int, # int flags
]

# Function dvz_segment_color()
segment_color = dvz.dvz_segment_color
segment_color.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_uint8), # cvec4* values
    ctypes.c_int, # int flags
]

# Function dvz_segment_linewidth()
segment_linewidth = dvz.dvz_segment_linewidth
segment_linewidth.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* segment
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float), # float* values
    ctypes.c_int, # int flags
]

# Function dvz_segment_cap()
segment_cap = dvz.dvz_segment_cap
segment_cap.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* segment
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(DvzCapType), # DvzCapType* initial
    ctypes.POINTER(DvzCapType), # DvzCapType* terminal
    ctypes.c_int, # int flags
]

# Function dvz_segment_alloc()
segment_alloc = dvz.dvz_segment_alloc
segment_alloc.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t item_count
]

# Function dvz_path()
path = dvz.dvz_path
path.argtypes = [
    ctypes.POINTER(DvzBatch), # DvzBatch* batch
    ctypes.c_int, # int flags
]
path.restype = ctypes.POINTER(DvzVisual)

# Function dvz_path_position()
path_position = dvz.dvz_path_position
path_position.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t vertex_count
    ctypes.POINTER(ctypes.c_float), # vec3* positions
    ctypes.c_uint32, # uint32_t path_count
    ctypes.POINTER(ctypes.c_uint32), # uint32_t* path_lengths
    ctypes.c_int, # int flags
]

# Function dvz_path_color()
path_color = dvz.dvz_path_color
path_color.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_uint8), # cvec4* values
    ctypes.c_int, # int flags
]

# Function dvz_path_linewidth()
path_linewidth = dvz.dvz_path_linewidth
path_linewidth.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_float, # float value
]

# Function dvz_path_alloc()
path_alloc = dvz.dvz_path_alloc
path_alloc.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t total_point_count
]

# Function dvz_glyph()
glyph = dvz.dvz_glyph
glyph.argtypes = [
    ctypes.POINTER(DvzBatch), # DvzBatch* batch
    ctypes.c_int, # int flags
]
glyph.restype = ctypes.POINTER(DvzVisual)

# Function dvz_glyph_alloc()
glyph_alloc = dvz.dvz_glyph_alloc
glyph_alloc.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t item_count
]

# Function dvz_glyph_position()
glyph_position = dvz.dvz_glyph_position
glyph_position.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float), # vec3* values
    ctypes.c_int, # int flags
]

# Function dvz_glyph_axis()
glyph_axis = dvz.dvz_glyph_axis
glyph_axis.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float), # vec3* values
    ctypes.c_int, # int flags
]

# Function dvz_glyph_size()
glyph_size = dvz.dvz_glyph_size
glyph_size.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float * 2), # vec2* values
    ctypes.c_int, # int flags
]

# Function dvz_glyph_anchor()
glyph_anchor = dvz.dvz_glyph_anchor
glyph_anchor.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float * 2), # vec2* values
    ctypes.c_int, # int flags
]

# Function dvz_glyph_shift()
glyph_shift = dvz.dvz_glyph_shift
glyph_shift.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float * 2), # vec2* values
    ctypes.c_int, # int flags
]

# Function dvz_glyph_texcoords()
glyph_texcoords = dvz.dvz_glyph_texcoords
glyph_texcoords.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float), # vec4* coords
    ctypes.c_int, # int flags
]

# Function dvz_glyph_angle()
glyph_angle = dvz.dvz_glyph_angle
glyph_angle.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float), # float* values
    ctypes.c_int, # int flags
]

# Function dvz_glyph_color()
glyph_color = dvz.dvz_glyph_color
glyph_color.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_uint8), # cvec4* values
    ctypes.c_int, # int flags
]

# Function dvz_glyph_groupsize()
glyph_groupsize = dvz.dvz_glyph_groupsize
glyph_groupsize.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float), # float* values
    ctypes.c_int, # int flags
]

# Function dvz_glyph_bgcolor()
glyph_bgcolor = dvz.dvz_glyph_bgcolor
glyph_bgcolor.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_float * 4, # vec4 bgcolor
]

# Function dvz_glyph_texture()
glyph_texture = dvz.dvz_glyph_texture
glyph_texture.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint64, # DvzId tex
]

# Function dvz_glyph_atlas()
glyph_atlas = dvz.dvz_glyph_atlas
glyph_atlas.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.POINTER(DvzAtlas), # DvzAtlas* atlas
]

# Function dvz_glyph_unicode()
glyph_unicode = dvz.dvz_glyph_unicode
glyph_unicode.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_uint32), # uint32_t* codepoints
]

# Function dvz_glyph_ascii()
glyph_ascii = dvz.dvz_glyph_ascii
glyph_ascii.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_char_p, # char* string
]

# Function dvz_glyph_xywh()
glyph_xywh = dvz.dvz_glyph_xywh
glyph_xywh.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float), # vec4* values
    ctypes.c_float * 2, # vec2 offset
    ctypes.c_int, # int flags
]

# Function dvz_image()
image = dvz.dvz_image
image.argtypes = [
    ctypes.POINTER(DvzBatch), # DvzBatch* batch
    ctypes.c_int, # int flags
]
image.restype = ctypes.POINTER(DvzVisual)

# Function dvz_image_position()
image_position = dvz.dvz_image_position
image_position.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float), # vec4* ul_lr
    ctypes.c_int, # int flags
]

# Function dvz_image_texcoords()
image_texcoords = dvz.dvz_image_texcoords
image_texcoords.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float), # vec4* ul_lr
    ctypes.c_int, # int flags
]

# Function dvz_image_texture()
image_texture = dvz.dvz_image_texture
image_texture.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint64, # DvzId tex
    DvzFilter, # DvzFilter filter
    DvzSamplerAddressMode, # DvzSamplerAddressMode address_mode
]

# Function dvz_image_alloc()
image_alloc = dvz.dvz_image_alloc
image_alloc.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t item_count
]

# Function dvz_tex_image()
tex_image = dvz.dvz_tex_image
tex_image.argtypes = [
    ctypes.POINTER(DvzBatch), # DvzBatch* batch
    DvzFormat, # DvzFormat format
    ctypes.c_uint32, # uint32_t width
    ctypes.c_uint32, # uint32_t height
    ctypes.c_void_p, # void* data
]
tex_image.restype = ctypes.c_uint64

# Function dvz_mesh()
mesh = dvz.dvz_mesh
mesh.argtypes = [
    ctypes.POINTER(DvzBatch), # DvzBatch* batch
    ctypes.c_int, # int flags
]
mesh.restype = ctypes.POINTER(DvzVisual)

# Function dvz_mesh_position()
mesh_position = dvz.dvz_mesh_position
mesh_position.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float), # vec3* values
    ctypes.c_int, # int flags
]

# Function dvz_mesh_color()
mesh_color = dvz.dvz_mesh_color
mesh_color.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_uint8), # cvec4* values
    ctypes.c_int, # int flags
]

# Function dvz_mesh_texcoords()
mesh_texcoords = dvz.dvz_mesh_texcoords
mesh_texcoords.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float), # vec4* values
    ctypes.c_int, # int flags
]

# Function dvz_mesh_normal()
mesh_normal = dvz.dvz_mesh_normal
mesh_normal.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float), # vec3* values
    ctypes.c_int, # int flags
]

# Function dvz_mesh_texture()
mesh_texture = dvz.dvz_mesh_texture
mesh_texture.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint64, # DvzId tex
    DvzFilter, # DvzFilter filter
    DvzSamplerAddressMode, # DvzSamplerAddressMode address_mode
]

# Function dvz_mesh_index()
mesh_index = dvz.dvz_mesh_index
mesh_index.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_uint32), # DvzIndex* values
]

# Function dvz_mesh_alloc()
mesh_alloc = dvz.dvz_mesh_alloc
mesh_alloc.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t vertex_count
    ctypes.c_uint32, # uint32_t index_count
]

# Function dvz_mesh_light_pos()
mesh_light_pos = dvz.dvz_mesh_light_pos
mesh_light_pos.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_float * 4, # vec4 pos
]

# Function dvz_mesh_light_params()
mesh_light_params = dvz.dvz_mesh_light_params
mesh_light_params.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_float * 4, # vec4 params
]

# Function dvz_mesh_shape()
mesh_shape = dvz.dvz_mesh_shape
mesh_shape.argtypes = [
    ctypes.POINTER(DvzBatch), # DvzBatch* batch
    ctypes.POINTER(DvzShape), # DvzShape* shape
    ctypes.c_int, # int flags
]
mesh_shape.restype = ctypes.POINTER(DvzVisual)

# Function dvz_fake_sphere()
fake_sphere = dvz.dvz_fake_sphere
fake_sphere.argtypes = [
    ctypes.POINTER(DvzBatch), # DvzBatch* batch
    ctypes.c_int, # int flags
]
fake_sphere.restype = ctypes.POINTER(DvzVisual)

# Function dvz_fake_sphere_position()
fake_sphere_position = dvz.dvz_fake_sphere_position
fake_sphere_position.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float), # vec3* pos
    ctypes.c_int, # int flags
]

# Function dvz_fake_sphere_color()
fake_sphere_color = dvz.dvz_fake_sphere_color
fake_sphere_color.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_uint8), # cvec4* color
    ctypes.c_int, # int flags
]

# Function dvz_fake_sphere_size()
fake_sphere_size = dvz.dvz_fake_sphere_size
fake_sphere_size.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float), # float* size
    ctypes.c_int, # int flags
]

# Function dvz_fake_sphere_alloc()
fake_sphere_alloc = dvz.dvz_fake_sphere_alloc
fake_sphere_alloc.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t item_count
]

# Function dvz_fake_sphere_light_pos()
fake_sphere_light_pos = dvz.dvz_fake_sphere_light_pos
fake_sphere_light_pos.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_float * 3, # vec3 pos
]

# Function dvz_volume()
volume = dvz.dvz_volume
volume.argtypes = [
    ctypes.POINTER(DvzBatch), # DvzBatch* batch
    ctypes.c_int, # int flags
]
volume.restype = ctypes.POINTER(DvzVisual)

# Function dvz_volume_alloc()
volume_alloc = dvz.dvz_volume_alloc
volume_alloc.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t item_count
]

# Function dvz_volume_texture()
volume_texture = dvz.dvz_volume_texture
volume_texture.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint64, # DvzId tex
    DvzFilter, # DvzFilter filter
    DvzSamplerAddressMode, # DvzSamplerAddressMode address_mode
]

# Function dvz_volume_size()
volume_size = dvz.dvz_volume_size
volume_size.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_float, # float w
    ctypes.c_float, # float h
    ctypes.c_float, # float d
]

# Function dvz_tex_volume()
tex_volume = dvz.dvz_tex_volume
tex_volume.argtypes = [
    ctypes.POINTER(DvzBatch), # DvzBatch* batch
    DvzFormat, # DvzFormat format
    ctypes.c_uint32, # uint32_t width
    ctypes.c_uint32, # uint32_t height
    ctypes.c_uint32, # uint32_t depth
    ctypes.c_void_p, # void* data
]
tex_volume.restype = ctypes.c_uint64

# Function dvz_slice()
slice = dvz.dvz_slice
slice.argtypes = [
    ctypes.POINTER(DvzBatch), # DvzBatch* batch
    ctypes.c_int, # int flags
]
slice.restype = ctypes.POINTER(DvzVisual)

# Function dvz_slice_position()
slice_position = dvz.dvz_slice_position
slice_position.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* slice
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float), # vec3* p0
    ctypes.POINTER(ctypes.c_float), # vec3* p1
    ctypes.POINTER(ctypes.c_float), # vec3* p2
    ctypes.POINTER(ctypes.c_float), # vec3* p3
    ctypes.c_int, # int flags
]

# Function dvz_slice_texcoords()
slice_texcoords = dvz.dvz_slice_texcoords
slice_texcoords.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* slice
    ctypes.c_uint32, # uint32_t first
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float), # vec3* uvw0
    ctypes.POINTER(ctypes.c_float), # vec3* uvw1
    ctypes.POINTER(ctypes.c_float), # vec3* uvw2
    ctypes.POINTER(ctypes.c_float), # vec3* uvw3
    ctypes.c_int, # int flags
]

# Function dvz_slice_texture()
slice_texture = dvz.dvz_slice_texture
slice_texture.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint64, # DvzId tex
    DvzFilter, # DvzFilter filter
    DvzSamplerAddressMode, # DvzSamplerAddressMode address_mode
]

# Function dvz_slice_alloc()
slice_alloc = dvz.dvz_slice_alloc
slice_alloc.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_uint32, # uint32_t item_count
]

# Function dvz_tex_slice()
tex_slice = dvz.dvz_tex_slice
tex_slice.argtypes = [
    ctypes.POINTER(DvzBatch), # DvzBatch* batch
    DvzFormat, # DvzFormat format
    ctypes.c_uint32, # uint32_t width
    ctypes.c_uint32, # uint32_t height
    ctypes.c_uint32, # uint32_t depth
    ctypes.c_void_p, # void* data
]
tex_slice.restype = ctypes.c_uint64

# Function dvz_slice_alpha()
slice_alpha = dvz.dvz_slice_alpha
slice_alpha.argtypes = [
    ctypes.POINTER(DvzVisual), # DvzVisual* visual
    ctypes.c_float, # float alpha
]

# Function dvz_resample()
resample = dvz.dvz_resample
resample.argtypes = [
    ctypes.c_double, # double t0
    ctypes.c_double, # double t1
    ctypes.c_double, # double t
]
resample.restype = ctypes.c_double

# Function dvz_easing()
easing = dvz.dvz_easing
easing.argtypes = [
    DvzEasing, # DvzEasing easing
    ctypes.c_double, # double t
]
easing.restype = ctypes.c_double

# Function dvz_circular_2D()
circular_2D = dvz.dvz_circular_2D
circular_2D.argtypes = [
    ctypes.c_float * 2, # vec2 center
    ctypes.c_float, # float radius
    ctypes.c_float, # float angle
    ctypes.c_float, # float t
    ctypes.c_float * 2, # vec2 out
]

# Function dvz_circular_3D()
circular_3D = dvz.dvz_circular_3D
circular_3D.argtypes = [
    ctypes.c_float * 3, # vec3 center
    ctypes.c_float * 3, # vec3 u
    ctypes.c_float * 3, # vec3 v
    ctypes.c_float, # float radius
    ctypes.c_float, # float angle
    ctypes.c_float, # float t
    ctypes.c_float * 3, # vec3 out
]

# Function dvz_interpolate()
interpolate = dvz.dvz_interpolate
interpolate.argtypes = [
    ctypes.c_float, # float p0
    ctypes.c_float, # float p1
    ctypes.c_float, # float t
]
interpolate.restype = ctypes.c_float

# Function dvz_interpolate_2D()
interpolate_2D = dvz.dvz_interpolate_2D
interpolate_2D.argtypes = [
    ctypes.c_float * 2, # vec2 p0
    ctypes.c_float * 2, # vec2 p1
    ctypes.c_float, # float t
    ctypes.c_float * 2, # vec2 out
]

# Function dvz_interpolate_3D()
interpolate_3D = dvz.dvz_interpolate_3D
interpolate_3D.argtypes = [
    ctypes.c_float * 3, # vec3 p0
    ctypes.c_float * 3, # vec3 p1
    ctypes.c_float, # float t
    ctypes.c_float * 3, # vec3 out
]

# Function dvz_arcball_initial()
arcball_initial = dvz.dvz_arcball_initial
arcball_initial.argtypes = [
    ctypes.POINTER(DvzArcball), # DvzArcball* arcball
    ctypes.c_float * 3, # vec3 angles
]

# Function dvz_arcball_reset()
arcball_reset = dvz.dvz_arcball_reset
arcball_reset.argtypes = [
    ctypes.POINTER(DvzArcball), # DvzArcball* arcball
]

# Function dvz_arcball_resize()
arcball_resize = dvz.dvz_arcball_resize
arcball_resize.argtypes = [
    ctypes.POINTER(DvzArcball), # DvzArcball* arcball
    ctypes.c_float, # float width
    ctypes.c_float, # float height
]

# Function dvz_arcball_flags()
arcball_flags = dvz.dvz_arcball_flags
arcball_flags.argtypes = [
    ctypes.POINTER(DvzArcball), # DvzArcball* arcball
    ctypes.c_int, # int flags
]

# Function dvz_arcball_constrain()
arcball_constrain = dvz.dvz_arcball_constrain
arcball_constrain.argtypes = [
    ctypes.POINTER(DvzArcball), # DvzArcball* arcball
    ctypes.c_float * 3, # vec3 constrain
]

# Function dvz_arcball_set()
arcball_set = dvz.dvz_arcball_set
arcball_set.argtypes = [
    ctypes.POINTER(DvzArcball), # DvzArcball* arcball
    ctypes.c_float * 3, # vec3 angles
]

# Function dvz_arcball_angles()
arcball_angles = dvz.dvz_arcball_angles
arcball_angles.argtypes = [
    ctypes.POINTER(DvzArcball), # DvzArcball* arcball
    ctypes.c_float * 3, # vec3 out_angles
]

# Function dvz_arcball_rotate()
arcball_rotate = dvz.dvz_arcball_rotate
arcball_rotate.argtypes = [
    ctypes.POINTER(DvzArcball), # DvzArcball* arcball
    ctypes.c_float * 2, # vec2 cur_pos
    ctypes.c_float * 2, # vec2 last_pos
]

# Function dvz_arcball_model()
arcball_model = dvz.dvz_arcball_model
arcball_model.argtypes = [
    ctypes.POINTER(DvzArcball), # DvzArcball* arcball
    ctypes.c_float * 16, # mat4 model
]

# Function dvz_arcball_end()
arcball_end = dvz.dvz_arcball_end
arcball_end.argtypes = [
    ctypes.POINTER(DvzArcball), # DvzArcball* arcball
]

# Function dvz_arcball_mvp()
arcball_mvp = dvz.dvz_arcball_mvp
arcball_mvp.argtypes = [
    ctypes.POINTER(DvzArcball), # DvzArcball* arcball
    ctypes.POINTER(DvzMVP), # DvzMVP* mvp
]

# Function dvz_arcball_print()
arcball_print = dvz.dvz_arcball_print
arcball_print.argtypes = [
    ctypes.POINTER(DvzArcball), # DvzArcball* arcball
]

# Function dvz_camera_initial()
camera_initial = dvz.dvz_camera_initial
camera_initial.argtypes = [
    ctypes.POINTER(DvzCamera), # DvzCamera* camera
    ctypes.c_float * 3, # vec3 pos
    ctypes.c_float * 3, # vec3 lookat
    ctypes.c_float * 3, # vec3 up
]

# Function dvz_camera_reset()
camera_reset = dvz.dvz_camera_reset
camera_reset.argtypes = [
    ctypes.POINTER(DvzCamera), # DvzCamera* camera
]

# Function dvz_camera_zrange()
camera_zrange = dvz.dvz_camera_zrange
camera_zrange.argtypes = [
    ctypes.POINTER(DvzCamera), # DvzCamera* camera
    ctypes.c_float, # float near
    ctypes.c_float, # float far
]

# Function dvz_camera_ortho()
camera_ortho = dvz.dvz_camera_ortho
camera_ortho.argtypes = [
    ctypes.POINTER(DvzCamera), # DvzCamera* camera
    ctypes.c_float, # float left
    ctypes.c_float, # float right
    ctypes.c_float, # float bottom
    ctypes.c_float, # float top
]

# Function dvz_camera_resize()
camera_resize = dvz.dvz_camera_resize
camera_resize.argtypes = [
    ctypes.POINTER(DvzCamera), # DvzCamera* camera
    ctypes.c_float, # float width
    ctypes.c_float, # float height
]

# Function dvz_camera_position()
camera_position = dvz.dvz_camera_position
camera_position.argtypes = [
    ctypes.POINTER(DvzCamera), # DvzCamera* camera
    ctypes.c_float * 3, # vec3 pos
]

# Function dvz_camera_lookat()
camera_lookat = dvz.dvz_camera_lookat
camera_lookat.argtypes = [
    ctypes.POINTER(DvzCamera), # DvzCamera* camera
    ctypes.c_float * 3, # vec3 lookat
]

# Function dvz_camera_up()
camera_up = dvz.dvz_camera_up
camera_up.argtypes = [
    ctypes.POINTER(DvzCamera), # DvzCamera* camera
    ctypes.c_float * 3, # vec3 up
]

# Function dvz_camera_perspective()
camera_perspective = dvz.dvz_camera_perspective
camera_perspective.argtypes = [
    ctypes.POINTER(DvzCamera), # DvzCamera* camera
    ctypes.c_float, # float fov
]

# Function dvz_camera_viewproj()
camera_viewproj = dvz.dvz_camera_viewproj
camera_viewproj.argtypes = [
    ctypes.POINTER(DvzCamera), # DvzCamera* camera
    ctypes.c_float * 16, # mat4 view
    ctypes.c_float * 16, # mat4 proj
]

# Function dvz_camera_mvp()
camera_mvp = dvz.dvz_camera_mvp
camera_mvp.argtypes = [
    ctypes.POINTER(DvzCamera), # DvzCamera* camera
    ctypes.POINTER(DvzMVP), # DvzMVP* mvp
]

# Function dvz_camera_print()
camera_print = dvz.dvz_camera_print
camera_print.argtypes = [
    ctypes.POINTER(DvzCamera), # DvzCamera* camera
]

# Function dvz_panzoom_reset()
panzoom_reset = dvz.dvz_panzoom_reset
panzoom_reset.argtypes = [
    ctypes.POINTER(DvzPanzoom), # DvzPanzoom* pz
]

# Function dvz_panzoom_resize()
panzoom_resize = dvz.dvz_panzoom_resize
panzoom_resize.argtypes = [
    ctypes.POINTER(DvzPanzoom), # DvzPanzoom* pz
    ctypes.c_float, # float width
    ctypes.c_float, # float height
]

# Function dvz_panzoom_flags()
panzoom_flags = dvz.dvz_panzoom_flags
panzoom_flags.argtypes = [
    ctypes.POINTER(DvzPanzoom), # DvzPanzoom* pz
    ctypes.c_int, # int flags
]

# Function dvz_panzoom_xlim()
panzoom_xlim = dvz.dvz_panzoom_xlim
panzoom_xlim.argtypes = [
    ctypes.POINTER(DvzPanzoom), # DvzPanzoom* pz
    ctypes.c_float * 2, # vec2 xlim
]

# Function dvz_panzoom_ylim()
panzoom_ylim = dvz.dvz_panzoom_ylim
panzoom_ylim.argtypes = [
    ctypes.POINTER(DvzPanzoom), # DvzPanzoom* pz
    ctypes.c_float * 2, # vec2 ylim
]

# Function dvz_panzoom_zlim()
panzoom_zlim = dvz.dvz_panzoom_zlim
panzoom_zlim.argtypes = [
    ctypes.POINTER(DvzPanzoom), # DvzPanzoom* pz
    ctypes.c_float * 2, # vec2 zlim
]

# Function dvz_panzoom_pan()
panzoom_pan = dvz.dvz_panzoom_pan
panzoom_pan.argtypes = [
    ctypes.POINTER(DvzPanzoom), # DvzPanzoom* pz
    ctypes.c_float * 2, # vec2 pan
]

# Function dvz_panzoom_zoom()
panzoom_zoom = dvz.dvz_panzoom_zoom
panzoom_zoom.argtypes = [
    ctypes.POINTER(DvzPanzoom), # DvzPanzoom* pz
    ctypes.c_float * 2, # vec2 zoom
]

# Function dvz_panzoom_pan_shift()
panzoom_pan_shift = dvz.dvz_panzoom_pan_shift
panzoom_pan_shift.argtypes = [
    ctypes.POINTER(DvzPanzoom), # DvzPanzoom* pz
    ctypes.c_float * 2, # vec2 shift_px
    ctypes.c_float * 2, # vec2 center_px
]

# Function dvz_panzoom_zoom_shift()
panzoom_zoom_shift = dvz.dvz_panzoom_zoom_shift
panzoom_zoom_shift.argtypes = [
    ctypes.POINTER(DvzPanzoom), # DvzPanzoom* pz
    ctypes.c_float * 2, # vec2 shift_px
    ctypes.c_float * 2, # vec2 center_px
]

# Function dvz_panzoom_end()
panzoom_end = dvz.dvz_panzoom_end
panzoom_end.argtypes = [
    ctypes.POINTER(DvzPanzoom), # DvzPanzoom* pz
]

# Function dvz_panzoom_zoom_wheel()
panzoom_zoom_wheel = dvz.dvz_panzoom_zoom_wheel
panzoom_zoom_wheel.argtypes = [
    ctypes.POINTER(DvzPanzoom), # DvzPanzoom* pz
    ctypes.c_float * 2, # vec2 dir
    ctypes.c_float * 2, # vec2 center_px
]

# Function dvz_panzoom_xrange()
panzoom_xrange = dvz.dvz_panzoom_xrange
panzoom_xrange.argtypes = [
    ctypes.POINTER(DvzPanzoom), # DvzPanzoom* pz
    ctypes.c_float * 2, # vec2 xrange
]

# Function dvz_panzoom_yrange()
panzoom_yrange = dvz.dvz_panzoom_yrange
panzoom_yrange.argtypes = [
    ctypes.POINTER(DvzPanzoom), # DvzPanzoom* pz
    ctypes.c_float * 2, # vec2 yrange
]

# Function dvz_panzoom_mvp()
panzoom_mvp = dvz.dvz_panzoom_mvp
panzoom_mvp.argtypes = [
    ctypes.POINTER(DvzPanzoom), # DvzPanzoom* pz
    ctypes.POINTER(DvzMVP), # DvzMVP* mvp
]

# Function dvz_demo()
demo = dvz.dvz_demo
demo.argtypes = [
]

# Function dvz_next_pow2()
next_pow2 = dvz.dvz_next_pow2
next_pow2.argtypes = [
    ctypes.c_uint64, # uint64_t x
]
next_pow2.restype = ctypes.c_uint64

# Function dvz_mean()
mean = dvz.dvz_mean
mean.argtypes = [
    ctypes.c_uint32, # uint32_t n
    ctypes.POINTER(ctypes.c_double), # double* values
]
mean.restype = ctypes.c_double

# Function dvz_min_max()
min_max = dvz.dvz_min_max
min_max.argtypes = [
    ctypes.c_uint32, # uint32_t n
    ctypes.POINTER(ctypes.c_float), # float* values
    ctypes.c_float * 2, # vec2 out_min_max
]

# Function dvz_normalize_bytes()
normalize_bytes = dvz.dvz_normalize_bytes
normalize_bytes.argtypes = [
    ctypes.c_uint32, # uint32_t count
    ctypes.POINTER(ctypes.c_float), # float* values
]
normalize_bytes.restype = ctypes.POINTER(ctypes.c_uint8)

# Function dvz_range()
range = dvz.dvz_range
range.argtypes = [
    ctypes.c_uint32, # uint32_t n
    ctypes.POINTER(ctypes.c_double), # double* values
    ctypes.c_double * 2, # dvec2 min_max
]

# Function dvz_rand_byte()
rand_byte = dvz.dvz_rand_byte
rand_byte.argtypes = [
]
rand_byte.restype = ctypes.c_uint8

# Function dvz_rand_int()
rand_int = dvz.dvz_rand_int
rand_int.argtypes = [
]
rand_int.restype = ctypes.c_int

# Function dvz_rand_float()
rand_float = dvz.dvz_rand_float
rand_float.argtypes = [
]
rand_float.restype = ctypes.c_float

# Function dvz_rand_double()
rand_double = dvz.dvz_rand_double
rand_double.argtypes = [
]
rand_double.restype = ctypes.c_double

# Function dvz_rand_normal()
rand_normal = dvz.dvz_rand_normal
rand_normal.argtypes = [
]
rand_normal.restype = ctypes.c_double

# Function dvz_mock_pos2D()
mock_pos2D = dvz.dvz_mock_pos2D
mock_pos2D.argtypes = [
    ctypes.c_uint32, # uint32_t count
    ctypes.c_float, # float std
]
mock_pos2D.restype = ctypes.POINTER(ctypes.c_float)

# Function dvz_mock_pos3D()
mock_pos3D = dvz.dvz_mock_pos3D
mock_pos3D.argtypes = [
    ctypes.c_uint32, # uint32_t count
    ctypes.c_float, # float std
]
mock_pos3D.restype = ctypes.POINTER(ctypes.c_float)

# Function dvz_mock_uniform()
mock_uniform = dvz.dvz_mock_uniform
mock_uniform.argtypes = [
    ctypes.c_uint32, # uint32_t count
    ctypes.c_float, # float vmin
    ctypes.c_float, # float vmax
]
mock_uniform.restype = ctypes.POINTER(ctypes.c_float)

# Function dvz_mock_color()
mock_color = dvz.dvz_mock_color
mock_color.argtypes = [
    ctypes.c_uint32, # uint32_t count
    ctypes.c_uint8, # uint8_t alpha
]
mock_color.restype = ctypes.POINTER(ctypes.c_uint8)

