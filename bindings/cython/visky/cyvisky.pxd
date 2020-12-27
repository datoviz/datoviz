# WARNING: parts of this file are auto-generated

cdef extern from "../include/visky/visky.h":

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

    ctypedef struct VklApp:
        pass

    ctypedef struct VklGpu:
        VklApp* app
        VklGpu* gpu

    ctypedef struct VklCanvas:
        VklApp* app

    ctypedef struct VklSubmit:
        pass

    ctypedef struct VklCommands:
        pass

    ctypedef struct VkViewport:
        float x
        float y
        float width
        float height
        float minDepth
        float maxDepth




    # ---------------------------------------------------------------------------------------------
    # AUTOMATICALLY-GENERATED PART:
    # ---------------------------------------------------------------------------------------------




    # ENUM START
    # from file: canvas.h

    ctypedef enum VklPrivateEventType:
        VKL_PRIVATE_EVENT_INIT = 0
        VKL_PRIVATE_EVENT_REFILL = 1
        VKL_PRIVATE_EVENT_INTERACT = 2
        VKL_PRIVATE_EVENT_FRAME = 3
        VKL_PRIVATE_EVENT_TIMER = 4
        VKL_PRIVATE_EVENT_RESIZE = 5
        VKL_PRIVATE_EVENT_PRE_SEND = 6
        VKL_PRIVATE_EVENT_POST_SEND = 7
        VKL_PRIVATE_EVENT_DESTROY = 8

    ctypedef enum VklCanvasSizeType:
        VKL_CANVAS_SIZE_SCREEN = 0
        VKL_CANVAS_SIZE_FRAMEBUFFER = 1

    ctypedef enum VklViewportClip:
        VKL_VIEWPORT_FULL = 0
        VKL_VIEWPORT_INNER = 1
        VKL_VIEWPORT_OUTER = 2
        VKL_VIEWPORT_OUTER_BOTTOM = 3
        VKL_VIEWPORT_OUTER_LEFT = 4

    ctypedef enum VklTransformAxis:
        VKL_TRANSFORM_AXIS_DEFAULT = 0
        VKL_TRANSFORM_AXIS_ALL = 1
        VKL_TRANSFORM_AXIS_X = 2
        VKL_TRANSFORM_AXIS_Y = 3
        VKL_TRANSFORM_AXIS_NONE = 4

    ctypedef enum VklMouseStateType:
        VKL_MOUSE_STATE_INACTIVE = 0
        VKL_MOUSE_STATE_DRAG = 1
        VKL_MOUSE_STATE_WHEEL = 2
        VKL_MOUSE_STATE_CLICK = 3
        VKL_MOUSE_STATE_DOUBLE_CLICK = 4
        VKL_MOUSE_STATE_CAPTURE = 5

    ctypedef enum VklKeyboardStateType:
        VKL_KEYBOARD_STATE_INACTIVE = 0
        VKL_KEYBOARD_STATE_ACTIVE = 1
        VKL_KEYBOARD_STATE_CAPTURE = 2

    ctypedef enum VklEventType:
        VKL_EVENT_NONE = 0
        VKL_EVENT_INIT = 1
        VKL_EVENT_MOUSE_BUTTON = 2
        VKL_EVENT_MOUSE_MOVE = 3
        VKL_EVENT_MOUSE_WHEEL = 4
        VKL_EVENT_MOUSE_DRAG_BEGIN = 5
        VKL_EVENT_MOUSE_DRAG_END = 6
        VKL_EVENT_MOUSE_CLICK = 7
        VKL_EVENT_MOUSE_DOUBLE_CLICK = 8
        VKL_EVENT_KEY = 9
        VKL_EVENT_FRAME = 10
        VKL_EVENT_SCREENCAST = 11

    ctypedef enum VklKeyType:
        VKL_KEY_RELEASE = 0
        VKL_KEY_PRESS = 1

    ctypedef enum VklMouseButtonType:
        VKL_MOUSE_RELEASE = 0
        VKL_MOUSE_PRESS = 1

    ctypedef enum VklKeyModifiers:
        VKL_KEY_MODIFIER_NONE = 0x00000000
        VKL_KEY_MODIFIER_SHIFT = 0x00000001
        VKL_KEY_MODIFIER_CONTROL = 0x00000002
        VKL_KEY_MODIFIER_ALT = 0x00000004
        VKL_KEY_MODIFIER_SUPER = 0x00000008

    ctypedef enum VklMouseButton:
        VKL_MOUSE_BUTTON_NONE = 0
        VKL_MOUSE_BUTTON_LEFT = 1
        VKL_MOUSE_BUTTON_MIDDLE = 2
        VKL_MOUSE_BUTTON_RIGHT = 3

    ctypedef enum VklScreencastStatus:
        VKL_SCREENCAST_NONE = 0
        VKL_SCREENCAST_IDLE = 1
        VKL_SCREENCAST_AWAIT_COPY = 2
        VKL_SCREENCAST_AWAIT_TRANSFER = 3

    # from file: context.h

    ctypedef enum VklDefaultBuffer:
        VKL_DEFAULT_BUFFER_STAGING = 0
        VKL_DEFAULT_BUFFER_VERTEX = 1
        VKL_DEFAULT_BUFFER_INDEX = 2
        VKL_DEFAULT_BUFFER_UNIFORM = 3
        VKL_DEFAULT_BUFFER_STORAGE = 4
        VKL_DEFAULT_BUFFER_UNIFORM_MAPPABLE = 5
        VKL_DEFAULT_BUFFER_COUNT = 6

    ctypedef enum VklDefaultQueue:
        VKL_DEFAULT_QUEUE_TRANSFER = 0
        VKL_DEFAULT_QUEUE_COMPUTE = 1
        VKL_DEFAULT_QUEUE_RENDER = 2
        VKL_DEFAULT_QUEUE_PRESENT = 3
        VKL_DEFAULT_QUEUE_COUNT = 4

    ctypedef enum VklFilterType:
        VKL_FILTER_MIN = 0
        VKL_FILTER_MAX = 1

    ctypedef enum VklTransferMode:
        VKL_TRANSFER_MODE_SYNC = 0
        VKL_TRANSFER_MODE_ASYNC = 1

    ctypedef enum VklDataTransferType:
        VKL_TRANSFER_NONE = 0
        VKL_TRANSFER_BUFFER_UPLOAD = 1
        VKL_TRANSFER_BUFFER_UPLOAD_IMMEDIATE = 2
        VKL_TRANSFER_BUFFER_DOWNLOAD = 3
        VKL_TRANSFER_BUFFER_COPY = 4
        VKL_TRANSFER_TEXTURE_UPLOAD = 5
        VKL_TRANSFER_TEXTURE_DOWNLOAD = 6
        VKL_TRANSFER_TEXTURE_COPY = 7

    # from file: keycode.h

    ctypedef enum VklKeyCode:
        VKL_KEY_UNKNOWN = -1
        VKL_KEY_NONE = +0
        VKL_KEY_SPACE = 32
        VKL_KEY_APOSTROPHE = 39
        VKL_KEY_COMMA = 44
        VKL_KEY_MINUS = 45
        VKL_KEY_PERIOD = 46
        VKL_KEY_SLASH = 47
        VKL_KEY_0 = 48
        VKL_KEY_1 = 49
        VKL_KEY_2 = 50
        VKL_KEY_3 = 51
        VKL_KEY_4 = 52
        VKL_KEY_5 = 53
        VKL_KEY_6 = 54
        VKL_KEY_7 = 55
        VKL_KEY_8 = 56
        VKL_KEY_9 = 57
        VKL_KEY_SEMICOLON = 59
        VKL_KEY_EQUAL = 61
        VKL_KEY_A = 65
        VKL_KEY_B = 66
        VKL_KEY_C = 67
        VKL_KEY_D = 68
        VKL_KEY_E = 69
        VKL_KEY_F = 70
        VKL_KEY_G = 71
        VKL_KEY_H = 72
        VKL_KEY_I = 73
        VKL_KEY_J = 74
        VKL_KEY_K = 75
        VKL_KEY_L = 76
        VKL_KEY_M = 77
        VKL_KEY_N = 78
        VKL_KEY_O = 79
        VKL_KEY_P = 80
        VKL_KEY_Q = 81
        VKL_KEY_R = 82
        VKL_KEY_S = 83
        VKL_KEY_T = 84
        VKL_KEY_U = 85
        VKL_KEY_V = 86
        VKL_KEY_W = 87
        VKL_KEY_X = 88
        VKL_KEY_Y = 89
        VKL_KEY_Z = 90
        VKL_KEY_LEFT_BRACKET = 91
        VKL_KEY_BACKSLASH = 92
        VKL_KEY_RIGHT_BRACKET = 93
        VKL_KEY_GRAVE_ACCENT = 96
        VKL_KEY_WORLD_1 = 161
        VKL_KEY_WORLD_2 = 162
        VKL_KEY_ESCAPE = 256
        VKL_KEY_ENTER = 257
        VKL_KEY_TAB = 258
        VKL_KEY_BACKSPACE = 259
        VKL_KEY_INSERT = 260
        VKL_KEY_DELETE = 261
        VKL_KEY_RIGHT = 262
        VKL_KEY_LEFT = 263
        VKL_KEY_DOWN = 264
        VKL_KEY_UP = 265
        VKL_KEY_PAGE_UP = 266
        VKL_KEY_PAGE_DOWN = 267
        VKL_KEY_HOME = 268
        VKL_KEY_END = 269
        VKL_KEY_CAPS_LOCK = 280
        VKL_KEY_SCROLL_LOCK = 281
        VKL_KEY_NUM_LOCK = 282
        VKL_KEY_PRINT_SCREEN = 283
        VKL_KEY_PAUSE = 284
        VKL_KEY_F1 = 290
        VKL_KEY_F2 = 291
        VKL_KEY_F3 = 292
        VKL_KEY_F4 = 293
        VKL_KEY_F5 = 294
        VKL_KEY_F6 = 295
        VKL_KEY_F7 = 296
        VKL_KEY_F8 = 297
        VKL_KEY_F9 = 298
        VKL_KEY_F10 = 299
        VKL_KEY_F11 = 300
        VKL_KEY_F12 = 301
        VKL_KEY_F13 = 302
        VKL_KEY_F14 = 303
        VKL_KEY_F15 = 304
        VKL_KEY_F16 = 305
        VKL_KEY_F17 = 306
        VKL_KEY_F18 = 307
        VKL_KEY_F19 = 308
        VKL_KEY_F20 = 309
        VKL_KEY_F21 = 310
        VKL_KEY_F22 = 311
        VKL_KEY_F23 = 312
        VKL_KEY_F24 = 313
        VKL_KEY_F25 = 314
        VKL_KEY_KP_0 = 320
        VKL_KEY_KP_1 = 321
        VKL_KEY_KP_2 = 322
        VKL_KEY_KP_3 = 323
        VKL_KEY_KP_4 = 324
        VKL_KEY_KP_5 = 325
        VKL_KEY_KP_6 = 326
        VKL_KEY_KP_7 = 327
        VKL_KEY_KP_8 = 328
        VKL_KEY_KP_9 = 329
        VKL_KEY_KP_DECIMAL = 330
        VKL_KEY_KP_DIVIDE = 331
        VKL_KEY_KP_MULTIPLY = 332
        VKL_KEY_KP_SUBTRACT = 333
        VKL_KEY_KP_ADD = 334
        VKL_KEY_KP_ENTER = 335
        VKL_KEY_KP_EQUAL = 336
        VKL_KEY_LEFT_SHIFT = 340
        VKL_KEY_LEFT_CONTROL = 341
        VKL_KEY_LEFT_ALT = 342
        VKL_KEY_LEFT_SUPER = 343
        VKL_KEY_RIGHT_SHIFT = 344
        VKL_KEY_RIGHT_CONTROL = 345
        VKL_KEY_RIGHT_ALT = 346
        VKL_KEY_RIGHT_SUPER = 347
        VKL_KEY_MENU = 348
        VKL_KEY_LAST = 348

    # from file: panel.h

    ctypedef enum VklPanelMode:
        VKL_PANEL_GRID = 0
        VKL_PANEL_INSET = 1

    ctypedef enum VklGridAxis:
        VKL_GRID_HORIZONTAL = 0
        VKL_GRID_VERTICAL = 1

    ctypedef enum VklPanelSizeUnit:
        VKL_PANEL_UNIT_NORMALIZED = 0
        VKL_PANEL_UNIT_FRAMEBUFFER = 1
        VKL_PANEL_UNIT_SCREEN = 2

    # from file: scene.h

    ctypedef enum VklControllerType:
        VKL_CONTROLLER_NONE = 0
        VKL_CONTROLLER_PANZOOM = 1
        VKL_CONTROLLER_AXES_2D = 2
        VKL_CONTROLLER_ARCBALL = 3
        VKL_CONTROLLER_CAMERA = 4
        VKL_CONTROLLER_AXES_3D = 5

    ctypedef enum VklCDS:
        VKL_CDS_DATA = 1
        VKL_CDS_GPU = 2
        VKL_CDS_PANZOOM = 3
        VKL_CDS_PANEL = 4
        VKL_CDS_CANVAS_NDC = 5
        VKL_CDS_CANVAS_PX = 6

    # from file: visuals.h

    ctypedef enum VklPipelineType:
        VKL_PIPELINE_GRAPHICS = 0
        VKL_PIPELINE_COMPUTE = 1

    ctypedef enum VklPropType:
        VKL_PROP_NONE = 0
        VKL_PROP_POS = 1
        VKL_PROP_COLOR = 2
        VKL_PROP_MARKER_SIZE = 3
        VKL_PROP_TEXT = 4
        VKL_PROP_TEXT_SIZE = 5
        VKL_PROP_LINE_WIDTH = 6
        VKL_PROP_TYPE = 7
        VKL_PROP_LENGTH = 8
        VKL_PROP_MARGIN = 9
        VKL_PROP_COLOR_TEXTURE = 10
        VKL_PROP_MODEL = 11
        VKL_PROP_VIEW = 12
        VKL_PROP_PROJ = 13

    ctypedef enum VklSourceKind:
        VKL_SOURCE_NONE = 0
        VKL_SOURCE_VERTEX = 1
        VKL_SOURCE_INDEX = 2
        VKL_SOURCE_UNIFORM = 3
        VKL_SOURCE_STORAGE = 4
        VKL_SOURCE_TEXTURE_1D = 5
        VKL_SOURCE_TEXTURE_2D = 6
        VKL_SOURCE_TEXTURE_3D = 7

    ctypedef enum VklSourceType:
        VKL_SOURCE_TYPE_NONE = 0
        VKL_SOURCE_TYPE_MVP = 1
        VKL_SOURCE_TYPE_VIEWPORT = 2
        VKL_SOURCE_TYPE_PARAM = 3
        VKL_SOURCE_TYPE_VERTEX = 4
        VKL_SOURCE_TYPE_INDEX = 5
        VKL_SOURCE_TYPE_IMAGE = 6
        VKL_SOURCE_TYPE_VOLUME = 7
        VKL_SOURCE_TYPE_COLOR_TEXTURE = 8
        VKL_SOURCE_TYPE_FONT_ATLAS = 9
        VKL_SOURCE_TYPE_OTHER = 10

    ctypedef enum VklSourceOrigin:
        VKL_SOURCE_ORIGIN_NONE = 0
        VKL_SOURCE_ORIGIN_LIB = 1
        VKL_SOURCE_ORIGIN_USER = 2
        VKL_SOURCE_ORIGIN_NOBAKE = 3

    ctypedef enum VklSourceFlags:
        VKL_SOURCE_FLAG_IMMEDIATE = 0x0001

    # from file: vklite.h

    ctypedef enum VklBackend:
        VKL_BACKEND_NONE = 0
        VKL_BACKEND_GLFW = 1
        VKL_BACKEND_OFFSCREEN = 2

    ctypedef enum VklQueueType:
        VKL_QUEUE_TRANSFER = 0x01
        VKL_QUEUE_GRAPHICS = 0x02
        VKL_QUEUE_COMPUTE = 0x04
        VKL_QUEUE_PRESENT = 0x08
        VKL_QUEUE_RENDER = 0x07
        VKL_QUEUE_ALL = 0x0F

    ctypedef enum VklCommandBufferType:
        VKL_COMMAND_TRANSFERS = 0
        VKL_COMMAND_GRAPHICS = 1
        VKL_COMMAND_COMPUTE = 2
        VKL_COMMAND_GUI = 3

    ctypedef enum VklTextureAxis:
        VKL_TEXTURE_AXIS_U = 0
        VKL_TEXTURE_AXIS_V = 1
        VKL_TEXTURE_AXIS_W = 2

    ctypedef enum VklBlendType:
        VKL_BLEND_DISABLE = 0
        VKL_BLEND_ENABLE = 1

    ctypedef enum VklDepthTest:
        VKL_DEPTH_TEST_DISABLE = 0
        VKL_DEPTH_TEST_ENABLE = 1

    ctypedef enum VklRenderpassAttachmentType:
        VKL_RENDERPASS_ATTACHMENT_COLOR = 0
        VKL_RENDERPASS_ATTACHMENT_DEPTH = 1


    # ENUM END


    # STRUCT START
    # from file: canvas.h

    ctypedef struct VklMouseButtonEvent:
        VklMouseButton button
        VklMouseButtonType type
        int modifiers

    ctypedef struct VklMouseMoveEvent:
        vec2 pos

    ctypedef struct VklMouseWheelEvent:
        vec2 dir

    ctypedef struct VklMouseDragEvent:
        vec2 pos
        VklMouseButton button

    ctypedef struct VklMouseClickEvent:
        vec2 pos
        VklMouseButton button
        bint double_click

    ctypedef struct VklKeyEvent:
        VklKeyType type
        VklKeyCode key_code
        int modifiers

    ctypedef struct VklFrameEvent:
        uint64_t idx
        double time
        double interval

    ctypedef struct VklTimerEvent:
        uint64_t idx
        double time
        double interval

    ctypedef struct VklScreencastEvent:
        uint64_t idx
        double time
        double interval
        uint32_t width
        uint32_t height
        uint8_t* rgba

    ctypedef struct VklViewport:
        VkViewport viewport
        vec4 margins
        uvec2 offset_screen
        uvec2 size_screen
        uvec2 offset_framebuffer
        uvec2 size_framebuffer
        VklViewportClip clip
        VklTransformAxis transform
        float dpi_scaling

    ctypedef union VklEventUnion:
        VklMouseButtonEvent b
        VklMouseMoveEvent m
        VklMouseWheelEvent w
        VklMouseDragEvent d
        VklMouseClickEvent c
        VklKeyEvent k
        VklFrameEvent f
        VklScreencastEvent s

    ctypedef struct VklEvent:
        VklEventType type
        void* user_data
        VklEventUnion u


    # STRUCT END


    ctypedef void (*VklEventCallback)(VklCanvas*, VklEvent)



    # FUNCTION START
    # from file: canvas.h
    VklCanvas* vkl_canvas(VklGpu* gpu, uint32_t width, uint32_t height)
    void vkl_event_callback(VklCanvas* canvas, VklEventType type, double param, VklEventCallback callback, void* user_data)
    void vkl_canvas_to_close(VklCanvas* canvas, bint value)
    void vkl_app_run(VklApp* app, uint64_t frame_count)

    # from file: vklite.h
    VklApp* vkl_app(VklBackend backend)
    int vkl_app_destroy(VklApp* app)
    VklGpu* vkl_gpu(VklApp* app, uint32_t idx)


    # FUNCTION END
