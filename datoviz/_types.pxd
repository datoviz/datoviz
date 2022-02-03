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
    ctypedef enum DvzObjectType:
        DVZ_OBJECT_TYPE_UNDEFINED = 0
        DVZ_OBJECT_TYPE_HOST = 1
        DVZ_OBJECT_TYPE_GPU = 2
        DVZ_OBJECT_TYPE_WINDOW = 3
        DVZ_OBJECT_TYPE_SWAPCHAIN = 4
        DVZ_OBJECT_TYPE_CANVAS = 5
        DVZ_OBJECT_TYPE_BOARD = 6
        DVZ_OBJECT_TYPE_COMMANDS = 7
        DVZ_OBJECT_TYPE_BUFFER = 8
        DVZ_OBJECT_TYPE_DAT = 9
        DVZ_OBJECT_TYPE_TEX = 10
        DVZ_OBJECT_TYPE_IMAGES = 11
        DVZ_OBJECT_TYPE_SAMPLER = 12
        DVZ_OBJECT_TYPE_BINDINGS = 13
        DVZ_OBJECT_TYPE_COMPUTE = 14
        DVZ_OBJECT_TYPE_GRAPHICS = 15
        DVZ_OBJECT_TYPE_PIPE = 16
        DVZ_OBJECT_TYPE_BARRIER = 17
        DVZ_OBJECT_TYPE_FENCES = 18
        DVZ_OBJECT_TYPE_SEMAPHORES = 19
        DVZ_OBJECT_TYPE_RENDERPASS = 20
        DVZ_OBJECT_TYPE_FRAMEBUFFER = 21
        DVZ_OBJECT_TYPE_WORKSPACE = 22
        DVZ_OBJECT_TYPE_PIPELIB = 23
        DVZ_OBJECT_TYPE_SUBMIT = 24
        DVZ_OBJECT_TYPE_SCREENCAST = 25
        DVZ_OBJECT_TYPE_TIMER = 26
        DVZ_OBJECT_TYPE_ARRAY = 27
        DVZ_OBJECT_TYPE_CUSTOM = 28

    ctypedef enum DvzObjectStatus:
        DVZ_OBJECT_STATUS_NONE = 0
        DVZ_OBJECT_STATUS_ALLOC = 1
        DVZ_OBJECT_STATUS_DESTROYED = 2
        DVZ_OBJECT_STATUS_INIT = 3
        DVZ_OBJECT_STATUS_CREATED = 4
        DVZ_OBJECT_STATUS_NEED_RECREATE = 5
        DVZ_OBJECT_STATUS_NEED_UPDATE = 6
        DVZ_OBJECT_STATUS_NEED_DESTROY = 7
        DVZ_OBJECT_STATUS_INACTIVE = 8
        DVZ_OBJECT_STATUS_INVALID = 9

    ctypedef enum DvzBufferType:
        DVZ_BUFFER_TYPE_UNDEFINED = 0
        DVZ_BUFFER_TYPE_STAGING = 1
        DVZ_BUFFER_TYPE_VERTEX = 2
        DVZ_BUFFER_TYPE_INDEX = 3
        DVZ_BUFFER_TYPE_STORAGE = 4
        DVZ_BUFFER_TYPE_UNIFORM = 5

    ctypedef enum DvzSamplerAxis:
        DVZ_SAMPLER_AXIS_U = 0
        DVZ_SAMPLER_AXIS_V = 1
        DVZ_SAMPLER_AXIS_W = 2

    ctypedef enum DvzSamplerFilter:
        DVZ_SAMPLER_FILTER_MIN = 0
        DVZ_SAMPLER_FILTER_MAG = 1

    ctypedef enum DvzFormat:
        DVZ_FORMAT_NONE = 0
        DVZ_FORMAT_R8_UNORM = 9
        DVZ_FORMAT_R8_SNORM = 10
        DVZ_FORMAT_R8G8B8_UNORM = 23
        DVZ_FORMAT_R8G8B8A8_UNORM = 37
        DVZ_FORMAT_R8G8B8A8_UINT = 41
        DVZ_FORMAT_B8G8R8A8_UNORM = 44
        DVZ_FORMAT_R16_UNORM = 70
        DVZ_FORMAT_R16_SNORM = 71
        DVZ_FORMAT_R32_UINT = 98
        DVZ_FORMAT_R32_SINT = 99
        DVZ_FORMAT_R32_SFLOAT = 100

    ctypedef enum DvzFilter:
        DVZ_FILTER_NEAREST = 0
        DVZ_FILTER_LINEAR = 1
        DVZ_FILTER_CUBIC_IMG = 1000015000

    ctypedef enum DvzSamplerAddressMode:
        DVZ_SAMPLER_ADDRESS_MODE_REPEAT = 0
        DVZ_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT = 1
        DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE = 2
        DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER = 3
        DVZ_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE = 4

    ctypedef enum DvzDefaultQueue:
        DVZ_DEFAULT_QUEUE_TRANSFER = 0
        DVZ_DEFAULT_QUEUE_COMPUTE = 1
        DVZ_DEFAULT_QUEUE_RENDER = 2
        DVZ_DEFAULT_QUEUE_PRESENT = 3
        DVZ_DEFAULT_QUEUE_COUNT = 4

    ctypedef enum DvzDatUsage:
        DVZ_DAT_USAGE_FREQUENT_NONE = 0
        DVZ_DAT_USAGE_FREQUENT_UPLOAD = 0x0001
        DVZ_DAT_USAGE_FREQUENT_DOWNLOAD = 0x0002
        DVZ_DAT_USAGE_FREQUENT_RESIZE = 0x0004

    ctypedef enum DvzDatFlags:
        DVZ_DAT_FLAGS_NONE = 0x0000
        DVZ_DAT_FLAGS_STANDALONE = 0x0100
        DVZ_DAT_FLAGS_MAPPABLE = 0x0200
        DVZ_DAT_FLAGS_DUP = 0x0400
        DVZ_DAT_FLAGS_KEEP_ON_RESIZE = 0x1000
        DVZ_DAT_FLAGS_PERSISTENT_STAGING = 0x2000

    ctypedef enum DvzTexDims:
        DVZ_TEX_NONE = 0
        DVZ_TEX_1D = 1
        DVZ_TEX_2D = 2
        DVZ_TEX_3D = 3

    ctypedef enum DvzTexFlags:
        DVZ_TEX_FLAGS_NONE = 0x0000
        DVZ_TEX_FLAGS_PERSISTENT_STAGING = 0x2000

    ctypedef enum DvzViewportClip:
        DVZ_VIEWPORT_FULL = 0
        DVZ_VIEWPORT_INNER = 1
        DVZ_VIEWPORT_OUTER = 2
        DVZ_VIEWPORT_OUTER_BOTTOM = 3
        DVZ_VIEWPORT_OUTER_LEFT = 4

    ctypedef enum DvzGraphicsFlags:
        DVZ_GRAPHICS_FLAGS_DEPTH_TEST = 0x0100
        DVZ_GRAPHICS_FLAGS_PICK = 0x0200

    ctypedef enum DvzGraphicsType:
        DVZ_GRAPHICS_NONE = 0
        DVZ_GRAPHICS_POINT = 1
        DVZ_GRAPHICS_LINE = 2
        DVZ_GRAPHICS_LINE_STRIP = 3
        DVZ_GRAPHICS_TRIANGLE = 4
        DVZ_GRAPHICS_TRIANGLE_STRIP = 5
        DVZ_GRAPHICS_TRIANGLE_FAN = 6
        DVZ_GRAPHICS_MARKER = 7
        DVZ_GRAPHICS_SEGMENT = 8
        DVZ_GRAPHICS_ARROW = 9
        DVZ_GRAPHICS_PATH = 10
        DVZ_GRAPHICS_TEXT = 11
        DVZ_GRAPHICS_IMAGE = 12
        DVZ_GRAPHICS_IMAGE_CMAP = 13
        DVZ_GRAPHICS_VOLUME_SLICE = 14
        DVZ_GRAPHICS_MESH = 15
        DVZ_GRAPHICS_FAKE_SPHERE = 16
        DVZ_GRAPHICS_VOLUME = 17
        DVZ_GRAPHICS_COUNT = 18
        DVZ_GRAPHICS_CUSTOM = 19

    ctypedef enum DvzMarkerType:
        DVZ_MARKER_DISC = 0
        DVZ_MARKER_ASTERISK = 1
        DVZ_MARKER_CHEVRON = 2
        DVZ_MARKER_CLOVER = 3
        DVZ_MARKER_CLUB = 4
        DVZ_MARKER_CROSS = 5
        DVZ_MARKER_DIAMOND = 6
        DVZ_MARKER_ARROW = 7
        DVZ_MARKER_ELLIPSE = 8
        DVZ_MARKER_HBAR = 9
        DVZ_MARKER_HEART = 10
        DVZ_MARKER_INFINITY = 11
        DVZ_MARKER_PIN = 12
        DVZ_MARKER_RING = 13
        DVZ_MARKER_SPADE = 14
        DVZ_MARKER_SQUARE = 15
        DVZ_MARKER_TAG = 16
        DVZ_MARKER_TRIANGLE = 17
        DVZ_MARKER_VBAR = 18
        DVZ_MARKER_COUNT = 19

    ctypedef enum DvzCapType:
        DVZ_CAP_TYPE_NONE = 0
        DVZ_CAP_ROUND = 1
        DVZ_CAP_TRIANGLE_IN = 2
        DVZ_CAP_TRIANGLE_OUT = 3
        DVZ_CAP_SQUARE = 4
        DVZ_CAP_BUTT = 5
        DVZ_CAP_COUNT = 6

    ctypedef enum DvzJoinType:
        DVZ_JOIN_SQUARE = 0
        DVZ_JOIN_ROUND = 1

    ctypedef enum DvzPathTopology:
        DVZ_PATH_OPEN = 0
        DVZ_PATH_CLOSED = 1

    ctypedef enum DvzRendererFlags:
        DVZ_RENDERER_FLAGS_NONE = 0

    ctypedef enum DvzDataType:
        DVZ_DTYPE_NONE = 0
        DVZ_DTYPE_CUSTOM = 1
        DVZ_DTYPE_STR = 2
        DVZ_DTYPE_CHAR = 3
        DVZ_DTYPE_CVEC2 = 4
        DVZ_DTYPE_CVEC3 = 5
        DVZ_DTYPE_CVEC4 = 6
        DVZ_DTYPE_USHORT = 7
        DVZ_DTYPE_USVEC2 = 8
        DVZ_DTYPE_USVEC3 = 9
        DVZ_DTYPE_USVEC4 = 10
        DVZ_DTYPE_SHORT = 11
        DVZ_DTYPE_SVEC2 = 12
        DVZ_DTYPE_SVEC3 = 13
        DVZ_DTYPE_SVEC4 = 14
        DVZ_DTYPE_UINT = 15
        DVZ_DTYPE_UVEC2 = 16
        DVZ_DTYPE_UVEC3 = 17
        DVZ_DTYPE_UVEC4 = 18
        DVZ_DTYPE_INT = 19
        DVZ_DTYPE_IVEC2 = 20
        DVZ_DTYPE_IVEC3 = 21
        DVZ_DTYPE_IVEC4 = 22
        DVZ_DTYPE_FLOAT = 23
        DVZ_DTYPE_VEC2 = 24
        DVZ_DTYPE_VEC3 = 25
        DVZ_DTYPE_VEC4 = 26
        DVZ_DTYPE_DOUBLE = 27
        DVZ_DTYPE_DVEC2 = 28
        DVZ_DTYPE_DVEC3 = 29
        DVZ_DTYPE_DVEC4 = 30
        DVZ_DTYPE_MAT2 = 31
        DVZ_DTYPE_MAT3 = 32
        DVZ_DTYPE_MAT4 = 33

    ctypedef enum DvzArrayCopyType:
        DVZ_ARRAY_COPY_NONE = 0
        DVZ_ARRAY_COPY_REPEAT = 1
        DVZ_ARRAY_COPY_SINGLE = 2

    ctypedef enum DvzCanvasFlags:
        DVZ_CANVAS_FLAGS_NONE = 0x0000
        DVZ_CANVAS_FLAGS_IMGUI = 0x0001
        DVZ_CANVAS_FLAGS_FPS = 0x0003
        DVZ_CANVAS_FLAGS_PICK = 0x0004

    ctypedef enum DvzCanvasSizeType:
        DVZ_CANVAS_SIZE_SCREEN = 0
        DVZ_CANVAS_SIZE_FRAMEBUFFER = 1

    ctypedef enum DvzColormap:
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
        DVZ_CPAL256_GLASBEY = 176  # CPAL256_OFS
        DVZ_CPAL256_GLASBEY_COOL = 125
        DVZ_CPAL256_GLASBEY_DARK = 126
        DVZ_CPAL256_GLASBEY_HV = 127
        DVZ_CPAL256_GLASBEY_LIGHT = 128
        DVZ_CPAL256_GLASBEY_WARM = 129
        DVZ_CPAL032_ACCENT = 240  # CPAL032_OFS
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

    ctypedef enum DvzBackend:
        DVZ_BACKEND_NONE = 0
        DVZ_BACKEND_GLFW = 1
        DVZ_BACKEND_OFFSCREEN = 2

    ctypedef enum DvzDeqProcCallbackPosition:
        DVZ_DEQ_PROC_CALLBACK_PRE = 0
        DVZ_DEQ_PROC_CALLBACK_POST = 1

    ctypedef enum DvzDeqProcBatchPosition:
        DVZ_DEQ_PROC_BATCH_BEGIN = 0
        DVZ_DEQ_PROC_BATCH_END = 1

    ctypedef enum DvzDeqStrategy:
        DVZ_DEQ_STRATEGY_BREADTH_FIRST = 0
        DVZ_DEQ_STRATEGY_DEPTH_FIRST = 1

    ctypedef enum DvzEventType:
        DVZ_EVENT_NONE = 0
        DVZ_EVENT_INIT = 1
        DVZ_EVENT_MOUSE_MOVE = 2
        DVZ_EVENT_MOUSE_PRESS = 3
        DVZ_EVENT_MOUSE_RELEASE = 4
        DVZ_EVENT_MOUSE_CLICK = 5
        DVZ_EVENT_MOUSE_DOUBLE_CLICK = 6
        DVZ_EVENT_MOUSE_WHEEL = 7
        DVZ_EVENT_MOUSE_DRAG_BEGIN = 8
        DVZ_EVENT_MOUSE_DRAG = 9
        DVZ_EVENT_MOUSE_DRAG_END = 10
        DVZ_EVENT_KEYBOARD_PRESS = 11
        DVZ_EVENT_KEYBOARD_RELEASE = 12
        DVZ_EVENT_KEYBOARD_STROKE = 13
        DVZ_EVENT_TIMER_TICK = 14

    ctypedef enum DvzKeyModifiers:
        DVZ_KEY_MODIFIER_NONE = 0x00000000
        DVZ_KEY_MODIFIER_SHIFT = 0x00000001
        DVZ_KEY_MODIFIER_CONTROL = 0x00000002
        DVZ_KEY_MODIFIER_ALT = 0x00000004
        DVZ_KEY_MODIFIER_SUPER = 0x00000008

    ctypedef enum DvzMouseButton:
        DVZ_MOUSE_BUTTON_NONE = 0
        DVZ_MOUSE_BUTTON_LEFT = 1
        DVZ_MOUSE_BUTTON_MIDDLE = 2
        DVZ_MOUSE_BUTTON_RIGHT = 3

    ctypedef enum DvzMouseStateType:
        DVZ_MOUSE_STATE_INACTIVE = 0
        DVZ_MOUSE_STATE_DRAG = 1
        DVZ_MOUSE_STATE_WHEEL = 2
        DVZ_MOUSE_STATE_CLICK = 3
        DVZ_MOUSE_STATE_DOUBLE_CLICK = 4
        DVZ_MOUSE_STATE_CAPTURE = 5

    ctypedef enum DvzKeyboardStateType:
        DVZ_KEYBOARD_STATE_INACTIVE = 0
        DVZ_KEYBOARD_STATE_ACTIVE = 1
        DVZ_KEYBOARD_STATE_CAPTURE = 2

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

    ctypedef enum DvzPipeType:
        DVZ_PIPE_NONE = 0
        DVZ_PIPE_GRAPHICS = 1
        DVZ_PIPE_COMPUTE = 2

    ctypedef enum DvzPipelibFlags:
        DVZ_PIPELIB_FLAGS_NONE = 0x0000
        DVZ_PIPELIB_FLAGS_CREATE_MVP = 0x0001
        DVZ_PIPELIB_FLAGS_CREATE_VIEWPORT = 0x0002

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
        DVZ_REQUEST_OBJECT_BEGIN = 8
        DVZ_REQUEST_OBJECT_BACKGROUND = 9
        DVZ_REQUEST_OBJECT_VIEWPORT = 10
        DVZ_REQUEST_OBJECT_VERTEX = 11
        DVZ_REQUEST_OBJECT_BARRIER = 12
        DVZ_REQUEST_OBJECT_DRAW = 13
        DVZ_REQUEST_OBJECT_END = 14

    ctypedef enum DvzCanvasEventType:
        DVZ_RUNNER_CANVAS_NONE = 0
        DVZ_RUNNER_CANVAS_FRAME = 1
        DVZ_RUNNER_CANVAS_NEW = 2
        DVZ_RUNNER_CANVAS_RECREATE = 3
        DVZ_RUNNER_CANVAS_RUNNING = 4
        DVZ_RUNNER_CANVAS_VISIBLE = 5
        DVZ_RUNNER_CANVAS_RESIZE = 6
        DVZ_RUNNER_CANVAS_CLEAR_COLOR = 7
        DVZ_RUNNER_CANVAS_DPI = 8
        DVZ_RUNNER_CANVAS_FPS = 9
        DVZ_RUNNER_CANVAS_UPFILL = 10
        DVZ_RUNNER_CANVAS_DELETE = 11
        DVZ_RUNNER_CANVAS_TO_REFILL = 12
        DVZ_RUNNER_CANVAS_REFILL_WRAP = 13
        DVZ_RUNNER_CANVAS_REFILL = 14
        DVZ_RUNNER_CANVAS_PRESENT = 15
        DVZ_RUNNER_REQUEST = 16

    ctypedef enum DvzTransferType:
        DVZ_TRANSFER_NONE = 0
        DVZ_TRANSFER_BUFFER_UPLOAD = 1
        DVZ_TRANSFER_BUFFER_DOWNLOAD = 2
        DVZ_TRANSFER_BUFFER_COPY = 3
        DVZ_TRANSFER_IMAGE_COPY = 4
        DVZ_TRANSFER_IMAGE_BUFFER = 5
        DVZ_TRANSFER_BUFFER_IMAGE = 6
        DVZ_TRANSFER_DOWNLOAD_DONE = 7
        DVZ_TRANSFER_UPLOAD_DONE = 8
        DVZ_TRANSFER_DUP_UPLOAD = 9
        DVZ_TRANSFER_DUP_COPY = 10

    ctypedef enum DvzQueueType:
        DVZ_QUEUE_TRANSFER = 0x01
        DVZ_QUEUE_GRAPHICS = 0x02
        DVZ_QUEUE_COMPUTE = 0x04
        DVZ_QUEUE_PRESENT = 0x08
        DVZ_QUEUE_RENDER = 0x07
        DVZ_QUEUE_ALL = 0x0F

    ctypedef enum DvzCommandBufferType:
        DVZ_COMMAND_TRANSFERS = 0
        DVZ_COMMAND_GRAPHICS = 1
        DVZ_COMMAND_COMPUTE = 2
        DVZ_COMMAND_GUI = 3

    ctypedef enum DvzDrawingFlags:
        DVZ_DRAWING_NONE = 0x00
        DVZ_DRAWING_FLAT = 0x01
        DVZ_DRAWING_INDEXED = 0x02
        DVZ_DRAWING_DIRECT = 0x04
        DVZ_DRAWING_INDIRECT = 0x08

    ctypedef enum DvzBlendType:
        DVZ_BLEND_DISABLE = 0
        DVZ_BLEND_ENABLE = 1

    ctypedef enum DvzDepthTest:
        DVZ_DEPTH_TEST_DISABLE = 0
        DVZ_DEPTH_TEST_ENABLE = 1

    ctypedef enum DvzRenderpassAttachmentType:
        DVZ_RENDERPASS_ATTACHMENT_COLOR = 0
        DVZ_RENDERPASS_ATTACHMENT_DEPTH = 1
        DVZ_RENDERPASS_ATTACHMENT_PICK = 2


    # ENUM END
