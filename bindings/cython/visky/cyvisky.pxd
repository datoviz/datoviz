# WARNING: parts of this file are auto-generated

cdef extern from "<visky/visky.h>":

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


    ctypedef struct VklApp:
        pass

    ctypedef struct VklContext:
        VklGpu* gpu

    ctypedef struct VklGpu:
        VklApp* app
        VklContext* context

    ctypedef struct VklTexture:
        VklGpu* gpu

    ctypedef struct VklCanvas:
        VklApp* app
        VklGpu* gpu

    ctypedef struct VklScene:
        VklCanvas* canvas

    ctypedef struct VklGrid:
        VklCanvas* canvas

    ctypedef struct VklPanel:
        VklGrid* grid

    ctypedef struct VklVisual:
        VklCanvas* canvas
        VklPanel* panel

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
        VK_FORMAT_R16_UNORM = 70

    ctypedef enum VkFilter:
        VK_FILTER_NEAREST = 0
        VK_FILTER_LINEAR = 1



    # ---------------------------------------------------------------------------------------------
    # AUTOMATICALLY-GENERATED PART:
    # ---------------------------------------------------------------------------------------------




    # ENUM START
    # from file: app.h

    ctypedef enum VklBackend:
        VKL_BACKEND_NONE = 0
        VKL_BACKEND_GLFW = 1
        VKL_BACKEND_OFFSCREEN = 2

    # from file: builtin_visuals.h

    ctypedef enum VklVisualVariant:
        VKL_VISUAL_VARIANT_NONE = 0
        VKL_VISUAL_VARIANT_RAW = 0x0001
        VKL_VISUAL_VARIANT_AGG = 0x0002
        VKL_VISUAL_VARIANT_SHADED = 0x0004
        VKL_VISUAL_VARIANT_TEXTURED = 0x0010
        VKL_VISUAL_VARIANT_TEXTURED_MULTI = 0x0020

    ctypedef enum VklVisualType:
        VKL_VISUAL_NONE = 0
        VKL_VISUAL_MARKER = 1
        VKL_VISUAL_SEGMENT = 2
        VKL_VISUAL_ARROW = 3
        VKL_VISUAL_PATH = 4
        VKL_VISUAL_TEXT = 5
        VKL_VISUAL_TRIANGLE = 6
        VKL_VISUAL_RECTANGLE = 7
        VKL_VISUAL_IMAGE = 8
        VKL_VISUAL_DISC = 9
        VKL_VISUAL_SECTOR = 10
        VKL_VISUAL_MESH = 11
        VKL_VISUAL_POLYGON = 12
        VKL_VISUAL_PSLG = 13
        VKL_VISUAL_HISTOGRAM = 14
        VKL_VISUAL_AREA = 15
        VKL_VISUAL_CANDLE = 16
        VKL_VISUAL_GRAPH = 17
        VKL_VISUAL_SURFACE = 18
        VKL_VISUAL_VOLUME_SLICE = 19
        VKL_VISUAL_VOLUME = 20
        VKL_VISUAL_FAKE_SPHERE = 21
        VKL_VISUAL_AXES_2D = 22
        VKL_VISUAL_AXES_3D = 23
        VKL_VISUAL_COLORMAP = 24
        VKL_VISUAL_COUNT = 25
        VKL_VISUAL_CUSTOM = 26

    ctypedef enum VklAxisLevel:
        VKL_AXES_LEVEL_MINOR = 0
        VKL_AXES_LEVEL_MAJOR = 1
        VKL_AXES_LEVEL_GRID = 2
        VKL_AXES_LEVEL_LIM = 3
        VKL_AXES_LEVEL_COUNT = 4

    # from file: canvas.h

    ctypedef enum VklCanvasFlags:
        VKL_CANVAS_FLAGS_NONE = 0x0000
        VKL_CANVAS_FLAGS_IMGUI = 0x0001
        VKL_CANVAS_FLAGS_FPS = 0x0003

    ctypedef enum VklCanvasSizeType:
        VKL_CANVAS_SIZE_SCREEN = 0
        VKL_CANVAS_SIZE_FRAMEBUFFER = 1

    ctypedef enum VklViewportClip:
        VKL_VIEWPORT_FULL = 0
        VKL_VIEWPORT_INNER = 1
        VKL_VIEWPORT_OUTER = 2
        VKL_VIEWPORT_OUTER_BOTTOM = 3
        VKL_VIEWPORT_OUTER_LEFT = 4

    ctypedef enum VklInteractAxis:
        VKL_INTERACT_AXIS_DEFAULT = 0
        VKL_INTERACT_AXIS_ALL = 1
        VKL_INTERACT_AXIS_X = 2
        VKL_INTERACT_AXIS_Y = 3
        VKL_INTERACT_AXIS_NONE = 4

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

    ctypedef enum VklTransferStatus:
        VKL_TRANSFER_STATUS_NONE = 0
        VKL_TRANSFER_STATUS_PROCESSING = 1
        VKL_TRANSFER_STATUS_DONE = 2

    ctypedef enum VklRefillStatus:
        VKL_REFILL_NONE = 0
        VKL_REFILL_REQUESTED = 1
        VKL_REFILL_PROCESSING = 2

    ctypedef enum VklScreencastStatus:
        VKL_SCREENCAST_NONE = 0
        VKL_SCREENCAST_IDLE = 1
        VKL_SCREENCAST_AWAIT_COPY = 2
        VKL_SCREENCAST_AWAIT_TRANSFER = 3

    ctypedef enum VklEventType:
        VKL_EVENT_NONE = 0
        VKL_EVENT_INIT = 1
        VKL_EVENT_REFILL = 2
        VKL_EVENT_INTERACT = 3
        VKL_EVENT_FRAME = 4
        VKL_EVENT_IMGUI = 5
        VKL_EVENT_SCREENCAST = 6
        VKL_EVENT_TIMER = 7
        VKL_EVENT_MOUSE_BUTTON = 8
        VKL_EVENT_MOUSE_MOVE = 9
        VKL_EVENT_MOUSE_WHEEL = 10
        VKL_EVENT_MOUSE_DRAG_BEGIN = 11
        VKL_EVENT_MOUSE_DRAG_END = 12
        VKL_EVENT_MOUSE_CLICK = 13
        VKL_EVENT_MOUSE_DOUBLE_CLICK = 14
        VKL_EVENT_KEY = 15
        VKL_EVENT_RESIZE = 16
        VKL_EVENT_PRE_SEND = 17
        VKL_EVENT_POST_SEND = 18
        VKL_EVENT_DESTROY = 19

    ctypedef enum VklEventMode:
        VKL_EVENT_MODE_SYNC = 0
        VKL_EVENT_MODE_ASYNC = 1

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

    # from file: context.h

    ctypedef enum VklDefaultQueue:
        VKL_DEFAULT_QUEUE_TRANSFER = 0
        VKL_DEFAULT_QUEUE_COMPUTE = 1
        VKL_DEFAULT_QUEUE_RENDER = 2
        VKL_DEFAULT_QUEUE_PRESENT = 3
        VKL_DEFAULT_QUEUE_COUNT = 4

    ctypedef enum VklFilterType:
        VKL_FILTER_MIN = 0
        VKL_FILTER_MAX = 1

    # from file: graphics.h

    ctypedef enum VklGraphicsFlags:
        VKL_GRAPHICS_FLAGS_NONE = 0x0000
        VKL_GRAPHICS_FLAGS_DEPTH_TEST = 0x0001

    ctypedef enum VkyMarkerType:
        VKL_MARKER_DISC = 0
        VKL_MARKER_ASTERISK = 1
        VKL_MARKER_CHEVRON = 2
        VKL_MARKER_CLOVER = 3
        VKL_MARKER_CLUB = 4
        VKL_MARKER_CROSS = 5
        VKL_MARKER_DIAMOND = 6
        VKL_MARKER_ARROW = 7
        VKL_MARKER_ELLIPSE = 8
        VKL_MARKER_HBAR = 9
        VKL_MARKER_HEART = 10
        VKL_MARKER_INFINITY = 11
        VKL_MARKER_PIN = 12
        VKL_MARKER_RING = 13
        VKL_MARKER_SPADE = 14
        VKL_MARKER_SQUARE = 15
        VKL_MARKER_TAG = 16
        VKL_MARKER_TRIANGLE = 17
        VKL_MARKER_VBAR = 18

    ctypedef enum VklCapType:
        VKL_CAP_TYPE_NONE = 0
        VKL_CAP_ROUND = 1
        VKL_CAP_TRIANGLE_IN = 2
        VKL_CAP_TRIANGLE_OUT = 3
        VKL_CAP_SQUARE = 4
        VKL_CAP_BUTT = 5
        VKL_CAP_COUNT = 6

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
        VKL_PANEL_FLOATING = 2

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

    ctypedef enum VklSceneVisualFlags:
        VKL_SCENE_VISUAL_FLAGS_NONE = 0x0000
        VKL_SCENE_VISUAL_FLAGS_TRANSFORM_NONE = 0x1000

    # from file: visuals.h

    ctypedef enum VklPipelineType:
        VKL_PIPELINE_GRAPHICS = 0
        VKL_PIPELINE_COMPUTE = 1

    ctypedef enum VklPropType:
        VKL_PROP_NONE = 0
        VKL_PROP_POS = 1
        VKL_PROP_COLOR = 2
        VKL_PROP_ALPHA = 3
        VKL_PROP_COLORMAP = 4
        VKL_PROP_MARKER_SIZE = 5
        VKL_PROP_TEXT = 6
        VKL_PROP_TEXT_SIZE = 7
        VKL_PROP_LINE_WIDTH = 8
        VKL_PROP_TYPE = 9
        VKL_PROP_LENGTH = 10
        VKL_PROP_MARGIN = 11
        VKL_PROP_NORMAL = 12
        VKL_PROP_TEXCOORDS = 13
        VKL_PROP_TEXCOEFS = 14
        VKL_PROP_IMAGE = 15
        VKL_PROP_VOLUME = 16
        VKL_PROP_COLOR_TEXTURE = 17
        VKL_PROP_TRANSFER_X = 18
        VKL_PROP_TRANSFER_Y = 19
        VKL_PROP_LIGHT_POS = 20
        VKL_PROP_LIGHT_PARAMS = 21
        VKL_PROP_CLIP = 22
        VKL_PROP_VIEW_POS = 23
        VKL_PROP_MODEL = 24
        VKL_PROP_VIEW = 25
        VKL_PROP_PROJ = 26
        VKL_PROP_TIME = 27
        VKL_PROP_INDEX = 28

    ctypedef enum VklSourceKind:
        VKL_SOURCE_KIND_NONE = 0
        VKL_SOURCE_KIND_VERTEX = 0x0010
        VKL_SOURCE_KIND_INDEX = 0x0020
        VKL_SOURCE_KIND_UNIFORM = 0x0030
        VKL_SOURCE_KIND_STORAGE = 0x0040
        VKL_SOURCE_KIND_TEXTURE_1D = 0x0050
        VKL_SOURCE_KIND_TEXTURE_2D = 0x0060
        VKL_SOURCE_KIND_TEXTURE_3D = 0x0070

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
        VKL_SOURCE_TYPE_COUNT = 11

    ctypedef enum VklSourceOrigin:
        VKL_SOURCE_ORIGIN_NONE = 0
        VKL_SOURCE_ORIGIN_LIB = 1
        VKL_SOURCE_ORIGIN_USER = 2
        VKL_SOURCE_ORIGIN_NOBAKE = 3

    ctypedef enum VklSourceFlags:
        VKL_SOURCE_FLAG_MAPPABLE = 0x0001

    ctypedef enum VklVisualRequest:
        VKL_VISUAL_REQUEST_NOT_SET = 0x0000
        VKL_VISUAL_REQUEST_SET = 0x0001
        VKL_VISUAL_REQUEST_REFILL = 0x0010
        VKL_VISUAL_REQUEST_UPLOAD = 0x0020
        VKL_VISUAL_REQUEST_NORMALIZATION = 0x0040
        VKL_VISUAL_REQUEST_VIEWPORT = 0x0080

    # from file: vklite.h

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

    ctypedef enum VklBufferType:
        VKL_BUFFER_TYPE_UNDEFINED = 0
        VKL_BUFFER_TYPE_STAGING = 1
        VKL_BUFFER_TYPE_VERTEX = 2
        VKL_BUFFER_TYPE_INDEX = 3
        VKL_BUFFER_TYPE_UNIFORM = 4
        VKL_BUFFER_TYPE_STORAGE = 5
        VKL_BUFFER_TYPE_UNIFORM_MAPPABLE = 6
        VKL_BUFFER_TYPE_COUNT = 7

    ctypedef enum VklGraphicsType:
        VKL_GRAPHICS_NONE = 0
        VKL_GRAPHICS_POINTS = 1
        VKL_GRAPHICS_LINES = 2
        VKL_GRAPHICS_LINE_STRIP = 3
        VKL_GRAPHICS_TRIANGLES = 4
        VKL_GRAPHICS_TRIANGLE_STRIP = 5
        VKL_GRAPHICS_TRIANGLE_FAN = 6
        VKL_GRAPHICS_MARKER_RAW = 7
        VKL_GRAPHICS_MARKER = 8
        VKL_GRAPHICS_SEGMENT = 9
        VKL_GRAPHICS_ARROW = 10
        VKL_GRAPHICS_PATH = 11
        VKL_GRAPHICS_TEXT = 12
        VKL_GRAPHICS_IMAGE = 13
        VKL_GRAPHICS_VOLUME_SLICE = 14
        VKL_GRAPHICS_MESH = 15
        VKL_GRAPHICS_FAKE_SPHERE = 16
        VKL_GRAPHICS_VOLUME = 17
        VKL_GRAPHICS_COUNT = 18

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

    ctypedef struct VklViewport:
        VkViewport viewport
        vec4 margins
        uvec2 offset_screen
        uvec2 size_screen
        uvec2 offset_framebuffer
        uvec2 size_framebuffer
        VklViewportClip clip
        VklInteractAxis interact_axis
        float dpi_scaling

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

    ctypedef struct VklRefillEvent:
        uint32_t img_idx
        uint32_t cmd_count
        VklCommands* cmds[32]
        VklViewport viewport
        VkClearColorValue clear_color

    ctypedef struct VklResizeEvent:
        uvec2 size_screen
        uvec2 size_framebuffer

    ctypedef struct VklSubmitEvent:
        VklSubmit* submit

    ctypedef union VklEventUnion:
        VklFrameEvent f
        VklFrameEvent t
        VklKeyEvent k
        VklMouseButtonEvent b
        VklMouseClickEvent c
        VklMouseDragEvent d
        VklMouseMoveEvent m
        VklMouseWheelEvent w
        VklRefillEvent rf
        VklResizeEvent r
        VklScreencastEvent sc
        VklSubmitEvent s

    ctypedef struct VklEvent:
        VklEventType type
        void* user_data
        VklEventUnion u


    # STRUCT END


    ctypedef void (*VklEventCallback)(VklCanvas*, VklEvent)



    # FUNCTION START
    # from file: canvas.h
    VklCanvas* vkl_canvas(VklGpu* gpu, uint32_t width, uint32_t height, int flags)
    void vkl_event_callback(VklCanvas* canvas, VklEventType type, double param, VklEventMode mode, VklEventCallback callback, void* user_data)
    void vkl_canvas_to_close(VklCanvas* canvas)
    void vkl_app_run(VklApp* app, uint64_t frame_count)

    # from file: context.h
    VklTexture* vkl_ctx_texture(VklContext* context, uint32_t dims, uvec3 size, VkFormat format)
    void vkl_texture_filter(VklTexture* texture, VklFilterType type, VkFilter filter)
    void vkl_texture_upload(VklTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size, const void* data)

    # from file: scene.h
    VklScene* vkl_scene(VklCanvas* canvas, uint32_t n_rows, uint32_t n_cols)
    void vkl_scene_destroy(VklScene* scene)
    VklVisual* vkl_scene_visual(VklPanel* panel, VklVisualType type, int flags)
    VklPanel* vkl_scene_panel(VklScene* scene, uint32_t row, uint32_t col, VklControllerType type, int flags)

    # from file: visuals.h
    void vkl_visual_data(VklVisual* visual, VklPropType type, uint32_t prop_idx, uint32_t count, const void* data)
    void vkl_visual_texture(VklVisual* visual, VklSourceType source_type, uint32_t source_idx, VklTexture* texture)

    # from file: vklite.h
    VklApp* vkl_app(VklBackend backend)
    int vkl_app_destroy(VklApp* app)
    VklGpu* vkl_gpu(VklApp* app, uint32_t idx)


    # FUNCTION END
