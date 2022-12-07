cdef extern from "<datoviz/common.h>":

    # Numerical types:
    ctypedef long int32_t
    ctypedef long long int64_t

    ctypedef unsigned long uint32_t
    ctypedef unsigned long long uint64_t

    ctypedef char uint8_t

    ctypedef uint32_t[2] uvec2
    ctypedef uint32_t[3] uvec3
    ctypedef uint32_t[4] uvec4

    ctypedef int32_t[2] ivec2
    ctypedef int32_t[3] ivec3
    ctypedef int32_t[4] ivec4

    ctypedef char[2] cvec2
    ctypedef char[3] cvec3
    ctypedef char[4] cvec4

    ctypedef float[2] vec2
    ctypedef float[3] vec3
    ctypedef float[4] vec4

    ctypedef double[2] dvec2
    ctypedef double[3] dvec3
    ctypedef double[4] dvec4

    ctypedef float[4][4] mat4

    ctypedef uint64_t DvzSize
    ctypedef uint64_t DvzId

    # ---------------------------------------------------------------------------------------------
    # ---------------------------------------------------------------------------------------------
    # ---------------------------------------------------------------------------------------------
    # !!!!!!!!!!!!!!!!!!!! AUTOMATICALLY-GENERATED PART: DO NOT EDIT MANUALLY !!!!!!!!!!!!!!!!!!!!
    # ---------------------------------------------------------------------------------------------
    # ---------------------------------------------------------------------------------------------
    # ---------------------------------------------------------------------------------------------

    # Enumerations
    # ---------------------------------------------------------------------------------------------

    # ENUM START
    ctypedef enum DvzBackend:
        DVZ_BACKEND_NONE = 0
        DVZ_BACKEND_GLFW = 1
        DVZ_BACKEND_OFFSCREEN = 2

    ctypedef enum DvzKeyCode:
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

    ctypedef enum DvzRequestAction:
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

    ctypedef enum DvzRequestObject:
        DVZ_REQUEST_OBJECT_NONE = 0
        DVZ_REQUEST_OBJECT_BOARD = 100
        DVZ_REQUEST_OBJECT_CANVAS = 2
        DVZ_REQUEST_OBJECT_DAT = 3
        DVZ_REQUEST_OBJECT_TEX = 4
        DVZ_REQUEST_OBJECT_SAMPLER = 5
        DVZ_REQUEST_OBJECT_COMPUTE = 6
        DVZ_REQUEST_OBJECT_GRAPHICS = 7
        DVZ_REQUEST_OBJECT_BACKGROUND = 8
        DVZ_REQUEST_OBJECT_VERTEX = 9
        DVZ_REQUEST_OBJECT_RECORD = 10

    ctypedef enum DvzPanelType:
        DVZ_PANEL_TYPE_NONE = 0
        DVZ_PANEL_TYPE_PANZOOM = 1
        DVZ_PANEL_TYPE_AXES_2D = 2

    ctypedef enum DvzPropType:
        DVZ_PROP_NONE = 0
        DVZ_PROP_POS = 1
        DVZ_PROP_COLOR = 2
        DVZ_PROP_ALPHA = 3
        DVZ_PROP_COLORMAP = 4
        DVZ_PROP_MARKER_SIZE = 5
        DVZ_PROP_MARKER_TYPE = 6
        DVZ_PROP_ANGLE = 7
        DVZ_PROP_TEXT = 8
        DVZ_PROP_TEXT_SIZE = 9
        DVZ_PROP_GLYPH = 10
        DVZ_PROP_ANCHOR = 11
        DVZ_PROP_LINE_WIDTH = 12
        DVZ_PROP_MITER_LIMIT = 13
        DVZ_PROP_CAP_TYPE = 14
        DVZ_PROP_JOIN_TYPE = 15
        DVZ_PROP_TOPOLOGY = 16
        DVZ_PROP_LENGTH = 17
        DVZ_PROP_RANGE = 18
        DVZ_PROP_MARGIN = 19
        DVZ_PROP_NORMAL = 20
        DVZ_PROP_TEXCOORDS = 21
        DVZ_PROP_TEXCOEFS = 22
        DVZ_PROP_IMAGE = 23
        DVZ_PROP_VOLUME = 24
        DVZ_PROP_TRANSFER_X = 25
        DVZ_PROP_TRANSFER_Y = 26
        DVZ_PROP_LIGHT_POS = 27
        DVZ_PROP_LIGHT_PARAMS = 28
        DVZ_PROP_CLIP = 29
        DVZ_PROP_MODEL = 30
        DVZ_PROP_VIEW = 31
        DVZ_PROP_PROJ = 32
        DVZ_PROP_VIEWPORT = 33
        DVZ_PROP_TIME = 34
        DVZ_PROP_INDEX = 35
        DVZ_PROP_SCALE = 36
        DVZ_PROP_TRANSFORM = 37

    ctypedef enum DvzVisualType:
        DVZ_VISUAL_NONE = 0
        DVZ_VISUAL_POINT = 1
        DVZ_VISUAL_LINE = 2
        DVZ_VISUAL_LINE_STRIP = 3
        DVZ_VISUAL_TRIANGLE = 4
        DVZ_VISUAL_TRIANGLE_STRIP = 5
        DVZ_VISUAL_TRIANGLE_FAN = 6
        DVZ_VISUAL_RECTANGLE = 7
        DVZ_VISUAL_MARKER = 8
        DVZ_VISUAL_SEGMENT = 9
        DVZ_VISUAL_ARROW = 10
        DVZ_VISUAL_PATH = 11
        DVZ_VISUAL_TEXT = 12
        DVZ_VISUAL_IMAGE = 13
        DVZ_VISUAL_IMAGE_CMAP = 14
        DVZ_VISUAL_DISC = 15
        DVZ_VISUAL_SECTOR = 16
        DVZ_VISUAL_MESH = 17
        DVZ_VISUAL_POLYGON = 18
        DVZ_VISUAL_PSLG = 19
        DVZ_VISUAL_HISTOGRAM = 20
        DVZ_VISUAL_AREA = 21
        DVZ_VISUAL_CANDLE = 22
        DVZ_VISUAL_GRAPH = 23
        DVZ_VISUAL_SURFACE = 24
        DVZ_VISUAL_VOLUME_SLICE = 25
        DVZ_VISUAL_VOLUME = 26
        DVZ_VISUAL_FAKE_SPHERE = 27
        DVZ_VISUAL_AXES_2D = 28
        DVZ_VISUAL_AXES_3D = 29
        DVZ_VISUAL_COLORMAP = 30
        DVZ_VISUAL_COUNT = 31
        DVZ_VISUAL_CUSTOM = 32


    # ENUM END
