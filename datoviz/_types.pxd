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
        DVZ_REQUEST_OBJECT_PRIMITIVE = 7
        DVZ_REQUEST_OBJECT_DEPTH = 8
        DVZ_REQUEST_OBJECT_BLEND = 9
        DVZ_REQUEST_OBJECT_POLYGON = 10
        DVZ_REQUEST_OBJECT_CULL = 11
        DVZ_REQUEST_OBJECT_FRONT = 12
        DVZ_REQUEST_OBJECT_GLSL = 13
        DVZ_REQUEST_OBJECT_SPIRV = 14
        DVZ_REQUEST_OBJECT_VERTEX = 15
        DVZ_REQUEST_OBJECT_VERTEX_ATTR = 16
        DVZ_REQUEST_OBJECT_SLOT = 17
        DVZ_REQUEST_OBJECT_GRAPHICS = 18
        DVZ_REQUEST_OBJECT_INDEX = 19
        DVZ_REQUEST_OBJECT_BACKGROUND = 20
        DVZ_REQUEST_OBJECT_RECORD = 21

    ctypedef enum DvzBackend:
        DVZ_BACKEND_NONE = 0
        DVZ_BACKEND_GLFW = 1
        DVZ_BACKEND_OFFSCREEN = 2

    ctypedef enum DvzBufferType:
        DVZ_BUFFER_TYPE_UNDEFINED = 0
        DVZ_BUFFER_TYPE_STAGING = 1
        DVZ_BUFFER_TYPE_VERTEX = 2
        DVZ_BUFFER_TYPE_INDEX = 3
        DVZ_BUFFER_TYPE_STORAGE = 4
        DVZ_BUFFER_TYPE_UNIFORM = 5
        DVZ_BUFFER_TYPE_INDIRECT = 6

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
        DVZ_FORMAT_R32G32B32_SFLOAT = 106

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

    ctypedef enum DvzPrimitiveTopology:
        DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST = 0
        DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST = 1
        DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP = 2
        DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST = 3
        DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP = 4
        DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN = 5

    ctypedef enum DvzTexDims:
        DVZ_TEX_NONE = 0
        DVZ_TEX_1D = 1
        DVZ_TEX_2D = 2
        DVZ_TEX_3D = 3

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
        DVZ_GRAPHICS_RASTER = 7
        DVZ_GRAPHICS_MARKER = 8
        DVZ_GRAPHICS_SEGMENT = 9
        DVZ_GRAPHICS_ARROW = 10
        DVZ_GRAPHICS_PATH = 11
        DVZ_GRAPHICS_TEXT = 12
        DVZ_GRAPHICS_IMAGE = 13
        DVZ_GRAPHICS_IMAGE_CMAP = 14
        DVZ_GRAPHICS_VOLUME_SLICE = 15
        DVZ_GRAPHICS_MESH = 16
        DVZ_GRAPHICS_FAKE_SPHERE = 17
        DVZ_GRAPHICS_VOLUME = 18
        DVZ_GRAPHICS_COUNT = 19
        DVZ_GRAPHICS_CUSTOM = 20

    ctypedef enum DvzKeyboardModifiers:
        DVZ_KEY_MODIFIER_NONE = 0x00000000
        DVZ_KEY_MODIFIER_SHIFT = 0x00000001
        DVZ_KEY_MODIFIER_CONTROL = 0x00000002
        DVZ_KEY_MODIFIER_ALT = 0x00000004
        DVZ_KEY_MODIFIER_SUPER = 0x00000008

    ctypedef enum DvzKeyboardEventType:
        DVZ_KEYBOARD_EVENT_NONE = 0
        DVZ_KEYBOARD_EVENT_PRESS = 1
        DVZ_KEYBOARD_EVENT_RELEASE = 2

    ctypedef enum DvzMouseButton:
        DVZ_MOUSE_BUTTON_NONE = 0
        DVZ_MOUSE_BUTTON_LEFT = 1
        DVZ_MOUSE_BUTTON_MIDDLE = 2
        DVZ_MOUSE_BUTTON_RIGHT = 3

    ctypedef enum DvzMouseState:
        DVZ_MOUSE_STATE_RELEASE = 0
        DVZ_MOUSE_STATE_PRESS = 1
        DVZ_MOUSE_STATE_CLICK = 3
        DVZ_MOUSE_STATE_CLICK_PRESS = 4
        DVZ_MOUSE_STATE_DOUBLE_CLICK = 5
        DVZ_MOUSE_STATE_DRAGGING = 11

    ctypedef enum DvzMouseEventType:
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

    ctypedef enum DvzClientEventType:
        DVZ_CLIENT_EVENT_NONE = 0
        DVZ_CLIENT_EVENT_WINDOW_CREATE = 1
        DVZ_CLIENT_EVENT_WINDOW_RESIZE = 2
        DVZ_CLIENT_EVENT_WINDOW_DELETE = 3
        DVZ_CLIENT_EVENT_FRAME = 4
        DVZ_CLIENT_EVENT_MOUSE = 5
        DVZ_CLIENT_EVENT_KEYBOARD = 6
        DVZ_CLIENT_EVENT_TIMER = 7
        DVZ_CLIENT_EVENT_REQUESTS = 8
        DVZ_CLIENT_EVENT_DESTROY = 9

    ctypedef enum DvzClientCallbackMode:
        DVZ_CLIENT_CALLBACK_SYNC = 0
        DVZ_CLIENT_CALLBACK_ASYNC = 1

    ctypedef enum DvzRequestFlags:
        DVZ_REQUEST_FLAGS_NONE = 0x0000
        DVZ_REQUEST_FLAGS_OFFSCREEN = 0x1000

    ctypedef enum DvzViewportClip:
        DVZ_VIEWPORT_FULL = 0
        DVZ_VIEWPORT_INNER = 1
        DVZ_VIEWPORT_OUTER = 2
        DVZ_VIEWPORT_OUTER_BOTTOM = 3
        DVZ_VIEWPORT_OUTER_LEFT = 4

    ctypedef enum DvzVisualFlags:
        DVZ_VISUALS_FLAGS_DEFAULT = 0x0000
        DVZ_VISUALS_FLAGS_INDEXED = 0x0001
        DVZ_VISUALS_FLAGS_INDIRECT = 0x0002


    # ENUM END
