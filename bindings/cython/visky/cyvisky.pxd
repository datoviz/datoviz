# WARNING: parts of this file are auto-generated

cdef extern from "../include/visky/visky2.h":

    # Numerical types
    ctypedef long int32_t
    ctypedef long long int64_t

    ctypedef unsigned long uint32_t
    ctypedef unsigned long long uint64_t

    ctypedef char uint8_t

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


    # Callback function types
    # ctypedef void (*VkyFrameCallback)(VkyCanvas*, void*)
    # ctypedef void (*VkyCloseCallback)(VkyCanvas*, void*)


    # FUNCTION START
    # from file: canvas.h
    VklCanvas* vkl_canvas(VklGpu* gpu, uint32_t width, uint32_t height)
    void vkl_canvas_to_close(VklCanvas* canvas, bint value)
    void vkl_app_run(VklApp* app, uint64_t frame_count)

    # from file: vklite2.h
    VklApp* vkl_app(VklBackend backend)
    int vkl_app_destroy(VklApp* app)
    VklGpu* vkl_gpu(VklApp* app, uint32_t idx)


    # FUNCTION END




    # ---------------------------------------------------------------------------------------------
    # AUTOMATICALLY-GENERATED PART:
    # ---------------------------------------------------------------------------------------------

    # STRUCT START

    # STRUCT END






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

    ctypedef enum VklEventType:
        VKL_EVENT_NONE = 0
        VKL_EVENT_INIT = 1
        VKL_EVENT_MOUSE_BUTTON = 2
        VKL_EVENT_MOUSE_MOVE = 3
        VKL_EVENT_MOUSE_WHEEL = 4
        VKL_EVENT_KEY = 5
        VKL_EVENT_FRAME = 6
        VKL_EVENT_SCREENCAST = 7

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

    ctypedef enum VklMouseStateType:
        VKL_MOUSE_STATE_INACTIVE = 0
        VKL_MOUSE_STATE_DRAG = 1
        VKL_MOUSE_STATE_WHEEL = 2
        VKL_MOUSE_STATE_CLICK = 3
        VKL_MOUSE_STATE_DOUBLE_CLICK = 4
        VKL_MOUSE_STATE_CAPTURE = 5

    ctypedef enum VklKeyStateType:
        VKL_KEYBOARD_STATE_INACTIVE = 0
        VKL_KEYBOARD_STATE_ACTIVE = 1
        VKL_KEYBOARD_STATE_CAPTURE = 2

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
        VKL_DEFAULT_BUFFER_STORAGE = 3
        VKL_DEFAULT_BUFFER_UNIFORM = 4
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
        VKL_TRANSFER_BUFFER_UPLOAD_FAST = 2
        VKL_TRANSFER_BUFFER_DOWNLOAD = 3
        VKL_TRANSFER_BUFFER_COPY = 4
        VKL_TRANSFER_TEXTURE_UPLOAD = 5
        VKL_TRANSFER_TEXTURE_DOWNLOAD = 6
        VKL_TRANSFER_TEXTURE_COPY = 7

    # from file: vklite2.h

    ctypedef enum VklObjectType:
        VKL_OBJECT_TYPE_UNDEFINED = 0
        VKL_OBJECT_TYPE_APP = 1
        VKL_OBJECT_TYPE_GPU = 2
        VKL_OBJECT_TYPE_WINDOW = 3
        VKL_OBJECT_TYPE_SWAPCHAIN = 4
        VKL_OBJECT_TYPE_CANVAS = 5
        VKL_OBJECT_TYPE_COMMANDS = 6
        VKL_OBJECT_TYPE_BUFFER = 7
        VKL_OBJECT_TYPE_TEXTURE = 8
        VKL_OBJECT_TYPE_IMAGES = 9
        VKL_OBJECT_TYPE_SAMPLER = 10
        VKL_OBJECT_TYPE_BINDINGS = 11
        VKL_OBJECT_TYPE_COMPUTE = 12
        VKL_OBJECT_TYPE_GRAPHICS = 13
        VKL_OBJECT_TYPE_BARRIER = 14
        VKL_OBJECT_TYPE_FENCES = 15
        VKL_OBJECT_TYPE_SEMAPHORES = 16
        VKL_OBJECT_TYPE_RENDERPASS = 17
        VKL_OBJECT_TYPE_FRAMEBUFFER = 18
        VKL_OBJECT_TYPE_SUBMIT = 19
        VKL_OBJECT_TYPE_SCREENCAST = 20
        VKL_OBJECT_TYPE_CUSTOM = 21

    ctypedef enum VklObjectStatus:
        VKL_OBJECT_STATUS_NONE = 0
        VKL_OBJECT_STATUS_DESTROYED = 1
        VKL_OBJECT_STATUS_INIT = 2
        VKL_OBJECT_STATUS_CREATED = 3
        VKL_OBJECT_STATUS_NEED_RECREATE = 4
        VKL_OBJECT_STATUS_NEED_UPDATE = 5
        VKL_OBJECT_STATUS_NEED_DESTROY = 6
        VKL_OBJECT_STATUS_INACTIVE = 7
        VKL_OBJECT_STATUS_INVALID = 8

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
