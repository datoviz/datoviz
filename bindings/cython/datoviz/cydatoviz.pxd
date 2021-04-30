# WARNING: parts of this file are auto-generated

cdef extern from "<datoviz/datoviz.h>":

    # Numerical types
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

    ctypedef uint64_t VkDeviceSize


    ctypedef struct DvzObject:
        pass

    ctypedef struct DvzApp:
        pass

    ctypedef struct DvzContext:
        DvzGpu* gpu

    ctypedef struct DvzGpu:
        DvzApp* app
        DvzContext* context

    ctypedef struct DvzTexture:
        DvzGpu* gpu

    ctypedef struct DvzCanvas:
        DvzApp* app
        DvzGpu* gpu

    ctypedef struct DvzGrid:
        DvzCanvas* canvas

    ctypedef struct DvzScene:
        DvzCanvas* canvas
        DvzGrid grid

    ctypedef struct DvzDataCoords:
        DvzTransformType transform;

    ctypedef struct DvzPanel:
        DvzGrid* grid
        DvzDataCoords data_coords
        uint32_t row
        uint32_t col

    ctypedef struct DvzProp:
        DvzPropType prop_type
        uint32_t prop_idx
        DvzDataType dtype

    ctypedef struct DvzVisual:
        DvzCanvas* canvas
        DvzPanel* panel

    ctypedef struct DvzArray:
        void* data
        uint32_t item_count

    ctypedef struct DvzMesh:
        DvzArray vertices
        DvzArray indices

    ctypedef struct DvzSubmit:
        pass

    ctypedef struct DvzBufferRegions:
        pass

    ctypedef struct DvzCommands:
        pass

    ctypedef struct VkViewport:
        float x
        float y
        float width
        float height
        float minDepth
        float maxDepth

    ctypedef union VkClearColorValue:
        float       float32[4]
        int32_t     int32[4]
        uint32_t    uint32[4]


    # ---------------------------------------------------------------------------------------------
    # Vulkan enums:
    # ---------------------------------------------------------------------------------------------

    # HACK: manual copy for now
    ctypedef enum VkFormat:
        VK_FORMAT_R8_UNORM = 9
        VK_FORMAT_R8_SNORM = 10
        VK_FORMAT_R8G8B8_UNORM = 23
        VK_FORMAT_R8G8B8A8_UNORM = 37
        VK_FORMAT_R16_UNORM = 70
        VK_FORMAT_R16_SNORM = 71
        VK_FORMAT_R32_UINT = 98,
        VK_FORMAT_R32_SINT = 99,
        VK_FORMAT_R32_SFLOAT = 100,

    ctypedef enum VkFilter:
        VK_FILTER_NEAREST = 0
        VK_FILTER_LINEAR = 1



    # ---------------------------------------------------------------------------------------------
    # AUTOMATICALLY-GENERATED PART:
    # ---------------------------------------------------------------------------------------------




    # ENUM START
    # from file: app.h

    ctypedef enum DvzBackend:
        DVZ_BACKEND_NONE = 0
        DVZ_BACKEND_GLFW = 1
        DVZ_BACKEND_OFFSCREEN = 2

    # from file: array.h

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

    # from file: canvas.h

    ctypedef enum DvzCanvasFlags:
        DVZ_CANVAS_FLAGS_NONE = 0x0000
        DVZ_CANVAS_FLAGS_IMGUI = 0x0001
        DVZ_CANVAS_FLAGS_FPS = 0x0003
        DVZ_CANVAS_FLAGS_PICK = 0x0004
        DVZ_CANVAS_FLAGS_DPI_SCALE_050 = 0x1000
        DVZ_CANVAS_FLAGS_DPI_SCALE_100 = 0x2000
        DVZ_CANVAS_FLAGS_DPI_SCALE_150 = 0x3000
        DVZ_CANVAS_FLAGS_DPI_SCALE_200 = 0x4000

    ctypedef enum DvzCanvasSizeType:
        DVZ_CANVAS_SIZE_SCREEN = 0
        DVZ_CANVAS_SIZE_FRAMEBUFFER = 1

    ctypedef enum DvzViewportClip:
        DVZ_VIEWPORT_FULL = 0
        DVZ_VIEWPORT_INNER = 1
        DVZ_VIEWPORT_OUTER = 2
        DVZ_VIEWPORT_OUTER_BOTTOM = 3
        DVZ_VIEWPORT_OUTER_LEFT = 4

    ctypedef enum DvzInteractAxis:
        DVZ_INTERACT_FIXED_AXIS_DEFAULT = 0x0000
        DVZ_INTERACT_FIXED_AXIS_X = 0x1000
        DVZ_INTERACT_FIXED_AXIS_Y = 0x2000
        DVZ_INTERACT_FIXED_AXIS_Z = 0x4000
        DVZ_INTERACT_FIXED_AXIS_XY = 0x3000
        DVZ_INTERACT_FIXED_AXIS_XZ = 0x5000
        DVZ_INTERACT_FIXED_AXIS_YZ = 0x6000
        DVZ_INTERACT_FIXED_AXIS_ALL = 0x7000
        DVZ_INTERACT_FIXED_AXIS_NONE = 0x8000

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

    ctypedef enum DvzTransferStatus:
        DVZ_TRANSFER_STATUS_NONE = 0
        DVZ_TRANSFER_STATUS_PROCESSING = 1
        DVZ_TRANSFER_STATUS_DONE = 2

    ctypedef enum DvzRefillStatus:
        DVZ_REFILL_NONE = 0
        DVZ_REFILL_REQUESTED = 1
        DVZ_REFILL_PROCESSING = 2

    ctypedef enum DvzScreencastStatus:
        DVZ_SCREENCAST_NONE = 0
        DVZ_SCREENCAST_IDLE = 1
        DVZ_SCREENCAST_AWAIT_COPY = 2
        DVZ_SCREENCAST_AWAIT_TRANSFER = 3

    ctypedef enum DvzEventType:
        DVZ_EVENT_NONE = 0
        DVZ_EVENT_INIT = 1
        DVZ_EVENT_REFILL = 2
        DVZ_EVENT_INTERACT = 3
        DVZ_EVENT_FRAME = 4
        DVZ_EVENT_IMGUI = 5
        DVZ_EVENT_GUI = 6
        DVZ_EVENT_SCREENCAST = 7
        DVZ_EVENT_TIMER = 8
        DVZ_EVENT_MOUSE_PRESS = 9
        DVZ_EVENT_MOUSE_RELEASE = 10
        DVZ_EVENT_MOUSE_MOVE = 11
        DVZ_EVENT_MOUSE_WHEEL = 12
        DVZ_EVENT_MOUSE_DRAG_BEGIN = 13
        DVZ_EVENT_MOUSE_DRAG_END = 14
        DVZ_EVENT_MOUSE_CLICK = 15
        DVZ_EVENT_MOUSE_DOUBLE_CLICK = 16
        DVZ_EVENT_KEY_PRESS = 17
        DVZ_EVENT_KEY_RELEASE = 18
        DVZ_EVENT_RESIZE = 19
        DVZ_EVENT_PRE_SEND = 20
        DVZ_EVENT_POST_SEND = 21
        DVZ_EVENT_DESTROY = 22

    ctypedef enum DvzEventMode:
        DVZ_EVENT_MODE_SYNC = 0
        DVZ_EVENT_MODE_ASYNC = 1

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

    # from file: colormaps.h

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
        DVZ_CPAL256_GLASBEY = 176
        DVZ_CPAL256_GLASBEY_COOL = 125
        DVZ_CPAL256_GLASBEY_DARK = 126
        DVZ_CPAL256_GLASBEY_HV = 127
        DVZ_CPAL256_GLASBEY_LIGHT = 128
        DVZ_CPAL256_GLASBEY_WARM = 129
        DVZ_CPAL032_ACCENT = 240
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

    # from file: context.h

    ctypedef enum DvzDefaultQueue:
        DVZ_DEFAULT_QUEUE_TRANSFER = 0
        DVZ_DEFAULT_QUEUE_COMPUTE = 1
        DVZ_DEFAULT_QUEUE_RENDER = 2
        DVZ_DEFAULT_QUEUE_PRESENT = 3
        DVZ_DEFAULT_QUEUE_COUNT = 4

    ctypedef enum DvzFilterType:
        DVZ_FILTER_MIN = 0
        DVZ_FILTER_MAG = 1

    # from file: controls.h

    ctypedef enum DvzGuiFlags:
        DVZ_GUI_FLAGS_NONE = 0x0000
        DVZ_GUI_FLAGS_FIXED = 0x0001
        DVZ_GUI_FLAGS_CORNER_UL = 0x0010
        DVZ_GUI_FLAGS_CORNER_UR = 0x0020
        DVZ_GUI_FLAGS_CORNER_LR = 0x0040
        DVZ_GUI_FLAGS_CORNER_LL = 0x0030

    ctypedef enum DvzGuiControlType:
        DVZ_GUI_CONTROL_NONE = 0
        DVZ_GUI_CONTROL_CHECKBOX = 1
        DVZ_GUI_CONTROL_SLIDER_FLOAT = 2
        DVZ_GUI_CONTROL_SLIDER_FLOAT2 = 3
        DVZ_GUI_CONTROL_SLIDER_INT = 4
        DVZ_GUI_CONTROL_INPUT_FLOAT = 5
        DVZ_GUI_CONTROL_LABEL = 6
        DVZ_GUI_CONTROL_TEXTBOX = 7
        DVZ_GUI_CONTROL_BUTTON = 8
        DVZ_GUI_CONTROL_COLORMAP = 9

    # from file: graphics.h

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
        DVZ_JOIN_SQUARE = False
        DVZ_JOIN_ROUND = True

    ctypedef enum DvzPathTopology:
        DVZ_PATH_OPEN = 0
        DVZ_PATH_CLOSED = 1

    # from file: keycode.h

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

    # from file: mesh.h

    ctypedef enum DvzMeshType:
        DVZ_MESH_CUSTOM = 0
        DVZ_MESH_CUBE = 1
        DVZ_MESH_SPHERE = 2
        DVZ_MESH_SURFACE = 3
        DVZ_MESH_CYLINDER = 4
        DVZ_MESH_CONE = 5
        DVZ_MESH_SQUARE = 6
        DVZ_MESH_DISC = 7
        DVZ_MESH_OBJ = 8
        DVZ_MESH_COUNT = 9

    # from file: panel.h

    ctypedef enum DvzPanelMode:
        DVZ_PANEL_GRID = 0
        DVZ_PANEL_INSET = 1
        DVZ_PANEL_FLOATING = 2

    ctypedef enum DvzGridAxis:
        DVZ_GRID_HORIZONTAL = 0
        DVZ_GRID_VERTICAL = 1

    ctypedef enum DvzPanelSizeUnit:
        DVZ_PANEL_UNIT_NORMALIZED = 0
        DVZ_PANEL_UNIT_FRAMEBUFFER = 1
        DVZ_PANEL_UNIT_SCREEN = 2

    # from file: scene.h

    ctypedef enum DvzControllerType:
        DVZ_CONTROLLER_NONE = 0
        DVZ_CONTROLLER_PANZOOM = 1
        DVZ_CONTROLLER_AXES_2D = 2
        DVZ_CONTROLLER_ARCBALL = 3
        DVZ_CONTROLLER_CAMERA = 4
        DVZ_CONTROLLER_AXES_3D = 5

    ctypedef enum DvzVisualFlags:
        DVZ_VISUAL_FLAGS_TRANSFORM_AUTO = 0x0000
        DVZ_VISUAL_FLAGS_TRANSFORM_NONE = 0x0010
        DVZ_VISUAL_FLAGS_TRANSFORM_BOX_INIT = 0x0020

    ctypedef enum DvzSceneUpdateType:
        DVZ_SCENE_UPDATE_NONE = 0
        DVZ_SCENE_UPDATE_VISUAL_ADDED = 1
        DVZ_SCENE_UPDATE_VISUAL_CHANGED = 2
        DVZ_SCENE_UPDATE_PROP_CHANGED = 3
        DVZ_SCENE_UPDATE_VISIBILITY_CHANGED = 4
        DVZ_SCENE_UPDATE_ITEM_COUNT_CHANGED = 5
        DVZ_SCENE_UPDATE_PANEL_CHANGED = 6
        DVZ_SCENE_UPDATE_INTERACT_CHANGED = 7
        DVZ_SCENE_UPDATE_COORDS_CHANGED = 8

    # from file: transfers.h

    ctypedef enum DvzDataTransferType:
        DVZ_TRANSFER_NONE = 0
        DVZ_TRANSFER_BUFFER_UPLOAD = 1
        DVZ_TRANSFER_BUFFER_DOWNLOAD = 2
        DVZ_TRANSFER_BUFFER_COPY = 3
        DVZ_TRANSFER_TEXTURE_UPLOAD = 4
        DVZ_TRANSFER_TEXTURE_DOWNLOAD = 5
        DVZ_TRANSFER_TEXTURE_COPY = 6

    # from file: transforms.h

    ctypedef enum DvzTransformType:
        DVZ_TRANSFORM_NONE = 0
        DVZ_TRANSFORM_CARTESIAN = 1
        DVZ_TRANSFORM_POLAR = 2
        DVZ_TRANSFORM_CYLINDRICAL = 3
        DVZ_TRANSFORM_SPHERICAL = 4
        DVZ_TRANSFORM_EARTH_MERCATOR_WEB = 5

    ctypedef enum DvzTransformFlags:
        DVZ_TRANSFORM_FLAGS_NONE = 0x0000
        DVZ_TRANSFORM_FLAGS_LOGX = 0x0001
        DVZ_TRANSFORM_FLAGS_LOGY = 0x0002
        DVZ_TRANSFORM_FLAGS_LOGLOG = 0x0003
        DVZ_TRANSFORM_FLAGS_FIXED_ASPECT = 0x0008

    ctypedef enum DvzCDS:
        DVZ_CDS_NONE = 0
        DVZ_CDS_DATA = 1
        DVZ_CDS_SCENE = 2
        DVZ_CDS_VULKAN = 3
        DVZ_CDS_FRAMEBUFFER = 4
        DVZ_CDS_WINDOW = 5

    ctypedef enum DvzCDSTranspose:
        DVZ_CDS_TRANSPOSE_NONE = 0
        DVZ_CDS_TRANSPOSE_XFYRZU = 1
        DVZ_CDS_TRANSPOSE_XBYDZL = 2
        DVZ_CDS_TRANSPOSE_XLYBZD = 3

    # from file: vislib.h

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

    ctypedef enum DvzAxisLevel:
        DVZ_AXES_LEVEL_MINOR = 0
        DVZ_AXES_LEVEL_MAJOR = 1
        DVZ_AXES_LEVEL_GRID = 2
        DVZ_AXES_LEVEL_LIM = 3
        DVZ_AXES_LEVEL_COUNT = 4

    ctypedef enum DvzAxesFlags:
        DVZ_AXES_FLAGS_DEFAULT = 0x0000
        DVZ_AXES_FLAGS_HIDE_MINOR = 0x0400
        DVZ_AXES_FLAGS_HIDE_GRID = 0x0800

    # from file: visuals.h

    ctypedef enum DvzPipelineType:
        DVZ_PIPELINE_GRAPHICS = 0
        DVZ_PIPELINE_COMPUTE = 1

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
        DVZ_PROP_LINE_WIDTH = 10
        DVZ_PROP_MITER_LIMIT = 11
        DVZ_PROP_CAP_TYPE = 12
        DVZ_PROP_JOIN_TYPE = 13
        DVZ_PROP_TOPOLOGY = 14
        DVZ_PROP_LENGTH = 15
        DVZ_PROP_RANGE = 16
        DVZ_PROP_MARGIN = 17
        DVZ_PROP_NORMAL = 18
        DVZ_PROP_TEXCOORDS = 19
        DVZ_PROP_TEXCOEFS = 20
        DVZ_PROP_TRANSFER_X = 21
        DVZ_PROP_TRANSFER_Y = 22
        DVZ_PROP_LIGHT_POS = 23
        DVZ_PROP_LIGHT_PARAMS = 24
        DVZ_PROP_CLIP = 25
        DVZ_PROP_MODEL = 26
        DVZ_PROP_VIEW = 27
        DVZ_PROP_PROJ = 28
        DVZ_PROP_VIEWPORT = 29
        DVZ_PROP_TIME = 30
        DVZ_PROP_INDEX = 31
        DVZ_PROP_SCALE = 32
        DVZ_PROP_TRANSFORM = 33

    ctypedef enum DvzSourceKind:
        DVZ_SOURCE_KIND_NONE = 0
        DVZ_SOURCE_KIND_VERTEX = 0x0010
        DVZ_SOURCE_KIND_INDEX = 0x0020
        DVZ_SOURCE_KIND_UNIFORM = 0x0030
        DVZ_SOURCE_KIND_STORAGE = 0x0040
        DVZ_SOURCE_KIND_TEXTURE_1D = 0x0050
        DVZ_SOURCE_KIND_TEXTURE_2D = 0x0060
        DVZ_SOURCE_KIND_TEXTURE_3D = 0x0070

    ctypedef enum DvzSourceType:
        DVZ_SOURCE_TYPE_NONE = 0
        DVZ_SOURCE_TYPE_MVP = 1
        DVZ_SOURCE_TYPE_VIEWPORT = 2
        DVZ_SOURCE_TYPE_PARAM = 3
        DVZ_SOURCE_TYPE_VERTEX = 4
        DVZ_SOURCE_TYPE_INDEX = 5
        DVZ_SOURCE_TYPE_IMAGE = 6
        DVZ_SOURCE_TYPE_VOLUME = 7
        DVZ_SOURCE_TYPE_TRANSFER = 8
        DVZ_SOURCE_TYPE_COLOR_TEXTURE = 9
        DVZ_SOURCE_TYPE_FONT_ATLAS = 10
        DVZ_SOURCE_TYPE_OTHER = 11
        DVZ_SOURCE_TYPE_COUNT = 12

    ctypedef enum DvzSourceOrigin:
        DVZ_SOURCE_ORIGIN_NONE = 0
        DVZ_SOURCE_ORIGIN_LIB = 1
        DVZ_SOURCE_ORIGIN_USER = 2
        DVZ_SOURCE_ORIGIN_NOBAKE = 3

    ctypedef enum DvzSourceFlags:
        DVZ_SOURCE_FLAG_MAPPABLE = 0x0001

    ctypedef enum DvzVisualRequest:
        DVZ_VISUAL_REQUEST_NOT_SET = 0x0000
        DVZ_VISUAL_REQUEST_SET = 0x0001
        DVZ_VISUAL_REQUEST_UPLOAD = 0x0002

    # from file: vklite.h

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

    ctypedef enum DvzBufferType:
        DVZ_BUFFER_TYPE_UNDEFINED = 0
        DVZ_BUFFER_TYPE_STAGING = 1
        DVZ_BUFFER_TYPE_VERTEX = 2
        DVZ_BUFFER_TYPE_INDEX = 3
        DVZ_BUFFER_TYPE_UNIFORM = 4
        DVZ_BUFFER_TYPE_STORAGE = 5
        DVZ_BUFFER_TYPE_UNIFORM_MAPPABLE = 6
        DVZ_BUFFER_TYPE_COUNT = 7

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

    ctypedef enum DvzTextureAxis:
        DVZ_TEXTURE_AXIS_U = 0
        DVZ_TEXTURE_AXIS_V = 1
        DVZ_TEXTURE_AXIS_W = 2

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


    # STRUCT START
    # from file: canvas.h

    ctypedef struct DvzViewport:
        VkViewport viewport
        vec4 margins
        uvec2 offset_screen
        uvec2 size_screen
        uvec2 offset_framebuffer
        uvec2 size_framebuffer
        DvzViewportClip clip
        int32_t interact_axis

    ctypedef struct DvzMouseButtonEvent:
        DvzMouseButton button
        int modifiers

    ctypedef struct DvzMouseMoveEvent:
        vec2 pos
        int modifiers

    ctypedef struct DvzMouseWheelEvent:
        vec2 pos
        vec2 dir
        int modifiers

    ctypedef struct DvzMouseDragEvent:
        vec2 pos
        DvzMouseButton button
        int modifiers

    ctypedef struct DvzMouseClickEvent:
        vec2 pos
        DvzMouseButton button
        int modifiers
        bint double_click

    ctypedef struct DvzKeyEvent:
        DvzKeyCode key_code
        int modifiers

    ctypedef struct DvzFrameEvent:
        uint64_t idx
        double time
        double interval

    ctypedef struct DvzTimerEvent:
        uint64_t idx
        double time
        double interval

    ctypedef struct DvzScreencastEvent:
        uint64_t idx
        double time
        double interval
        uint32_t width
        uint32_t height
        uint8_t* rgba

    ctypedef struct DvzRefillEvent:
        uint32_t img_idx
        uint32_t cmd_count
        DvzCommands* cmds[32]
        DvzViewport viewport
        VkClearColorValue clear_color

    ctypedef struct DvzResizeEvent:
        uvec2 size_screen
        uvec2 size_framebuffer

    ctypedef struct DvzSubmitEvent:
        DvzSubmit* submit

    ctypedef struct DvzGuiEvent:
        DvzGui* gui
        DvzGuiControl* control

    ctypedef union DvzEventUnion:
        DvzFrameEvent f
        DvzFrameEvent t
        DvzKeyEvent k
        DvzMouseButtonEvent b
        DvzMouseClickEvent c
        DvzMouseDragEvent d
        DvzMouseMoveEvent m
        DvzMouseWheelEvent w
        DvzRefillEvent rf
        DvzResizeEvent r
        DvzScreencastEvent sc
        DvzSubmitEvent s
        DvzGuiEvent g

    ctypedef struct DvzEvent:
        DvzEventType type
        void* user_data
        DvzEventUnion u

    # from file: controls.h

    ctypedef struct DvzGuiControlSliderFloat:
        float vmin
        float vmax

    ctypedef struct DvzGuiControlSliderFloat2:
        float vmin
        float vmax
        bint force_increasing

    ctypedef struct DvzGuiControlSliderInt:
        int vmin
        int vmax

    ctypedef struct DvzGuiControlInputFloat:
        float step
        float step_fast

    ctypedef union DvzGuiControlUnion:
        DvzGuiControlSliderFloat sf
        DvzGuiControlSliderFloat2 sf2
        DvzGuiControlSliderInt si
        DvzGuiControlInputFloat f

    ctypedef struct DvzGuiControl:
        DvzGui* gui
        const char* name
        int flags
        void* value
        DvzGuiControlType type
        DvzGuiControlUnion u

    ctypedef struct DvzGui:
        DvzObject obj
        DvzCanvas* canvas
        const char* title
        int flags
        uint32_t control_count
        DvzGuiControl controls[32]


    # STRUCT END


    ctypedef void (*DvzEventCallback)(DvzCanvas*, DvzEvent)
    void dvz_colormap_array(DvzColormap cmap, uint32_t count, double* values, double vmin, double vmax, cvec4* out);
    void dvz_colormap_packuv(cvec3 color, vec2 uv)
    void dvz_colormap_custom(uint8_t cmap, uint32_t color_count, cvec4* colors)
    void dvz_context_colormap(DvzContext* context)
    void dvz_imgui_demo(DvzCanvas* canvas)




    # FUNCTION START
    # from file: canvas.h
    DvzCanvas* dvz_canvas(DvzGpu* gpu, uint32_t width, uint32_t height, int flags)
    void dvz_canvas_clear_color(DvzCanvas* canvas, float red, float green, float blue)
    void dvz_event_callback(DvzCanvas* canvas, DvzEventType type, double param, DvzEventMode mode, DvzEventCallback callback, void* user_data)
    void dvz_canvas_to_close(DvzCanvas* canvas)
    void dvz_screenshot_file(DvzCanvas* canvas, const char* png_path)
    void dvz_canvas_pick(DvzCanvas* canvas, uvec2 pos_screen, ivec4 picked)
    void dvz_canvas_video(DvzCanvas* canvas, int framerate, int bitrate, const char* path, bint record)
    void dvz_canvas_pause(DvzCanvas* canvas, bint record)
    void dvz_canvas_stop(DvzCanvas* canvas)
    int dvz_canvas_frame(DvzCanvas* canvas)
    int dvz_app_run(DvzApp* app, uint64_t frame_count)

    # from file: context.h
    DvzTexture* dvz_ctx_texture(DvzContext* context, uint32_t dims, uvec3 size, VkFormat format)
    void dvz_texture_filter(DvzTexture* texture, DvzFilterType type, VkFilter filter)
    void dvz_texture_upload(DvzTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size, const void* data)

    # from file: controls.h
    DvzGui* dvz_gui(DvzCanvas* canvas, const char* title, int flags)
    void* dvz_gui_value(DvzGuiControl* control)
    DvzGuiControl* dvz_gui_checkbox(DvzGui* gui, const char* name, bint value)
    DvzGuiControl* dvz_gui_slider_float(DvzGui* gui, const char* name, float vmin, float vmax, float value)
    DvzGuiControl* dvz_gui_slider_float2(DvzGui* gui, const char* name, float vmin, float vmax, vec2 value, bint force_increasing)
    DvzGuiControl* dvz_gui_slider_int(DvzGui* gui, const char* name, int vmin, int vmax, int value)
    DvzGuiControl* dvz_gui_input_float(DvzGui* gui, const char* name, float step, float step_fast, float value)
    DvzGuiControl* dvz_gui_label(DvzGui* gui, const char* name, char* text)
    DvzGuiControl* dvz_gui_button(DvzGui* gui, const char* name, int flags)

    # from file: mesh.h
    void dvz_mesh_normals(DvzMesh* mesh)
    DvzMesh dvz_mesh_grid(uint32_t row_count, uint32_t col_count, const vec3* positions, const vec2* texcoords)
    DvzMesh dvz_mesh_obj(const char* file_path)

    # from file: panel.h
    void dvz_panel_transpose(DvzPanel* panel, DvzCDSTranspose transpose)
    DvzPanel* dvz_panel_at(DvzGrid* grid, vec2 screen_pos)

    # from file: scene.h
    DvzScene* dvz_scene(DvzCanvas* canvas, uint32_t n_rows, uint32_t n_cols)
    void dvz_scene_destroy(DvzScene* scene)
    DvzPanel* dvz_scene_panel(DvzScene* scene, uint32_t row, uint32_t col, DvzControllerType type, int flags)
    DvzVisual* dvz_scene_visual(DvzPanel* panel, DvzVisualType type, int flags)

    # from file: transfers.h
    void dvz_upload_buffer(DvzContext* context, DvzBufferRegions br, VkDeviceSize offset, VkDeviceSize size, void* data)
    void dvz_download_buffer(DvzContext* context, DvzBufferRegions br, VkDeviceSize offset, VkDeviceSize size, void* data)
    void dvz_copy_buffer(DvzContext* context, DvzBufferRegions src, VkDeviceSize src_offset, DvzBufferRegions dst, VkDeviceSize dst_offset, VkDeviceSize size)
    void dvz_upload_texture(DvzContext* context, DvzTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size, void* data)
    void dvz_download_texture(DvzContext* context, DvzTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size, void* data)
    void dvz_copy_texture(DvzContext* context, DvzTexture* src, uvec3 src_offset, DvzTexture* dst, uvec3 dst_offset, uvec3 shape, VkDeviceSize size)

    # from file: transforms.h
    void dvz_transform(DvzPanel* panel, DvzCDS source, dvec3 pos_in, DvzCDS target, dvec3 pos_out)

    # from file: visuals.h
    void dvz_visual_data(DvzVisual* visual, DvzPropType prop_type, uint32_t prop_idx, uint32_t count, const void* data)
    void dvz_visual_data_source(DvzVisual* visual, DvzSourceType source_type, uint32_t source_idx, uint32_t first_item, uint32_t item_count, uint32_t data_item_count, const void* data)
    void dvz_visual_texture(DvzVisual* visual, DvzSourceType source_type, uint32_t source_idx, DvzTexture* texture)
    DvzProp* dvz_prop_get(DvzVisual* visual, DvzPropType prop_type, uint32_t prop_idx)

    # from file: vklite.h
    DvzApp* dvz_app(DvzBackend backend)
    int dvz_app_destroy(DvzApp* app)
    DvzGpu* dvz_gpu(DvzApp* app, uint32_t idx)
    DvzGpu* dvz_gpu_best(DvzApp* app)


    # FUNCTION END
