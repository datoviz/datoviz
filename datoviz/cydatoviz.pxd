# WARNING: parts of this file are auto-generated

cdef extern from "<datoviz/request.h>":
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

    # Semi-opaque structs:

    ctypedef struct DvzRequester:
        pass

    ctypedef struct DvzRequest:
        pass

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
        DVZ_OBJECT_TYPE_SUBMIT = 22
        DVZ_OBJECT_TYPE_SCREENCAST = 23
        DVZ_OBJECT_TYPE_TIMER = 24
        DVZ_OBJECT_TYPE_ARRAY = 25
        DVZ_OBJECT_TYPE_CUSTOM = 26

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

    ctypedef enum DvzDatOptions:
        DVZ_DAT_OPTIONS_NONE = 0x0000
        DVZ_DAT_OPTIONS_STANDALONE = 0x0100
        DVZ_DAT_OPTIONS_MAPPABLE = 0x0200
        DVZ_DAT_OPTIONS_DUP = 0x0400
        DVZ_DAT_OPTIONS_KEEP_ON_RESIZE = 0x1000
        DVZ_DAT_OPTIONS_PERSISTENT_STAGING = 0x2000

    ctypedef enum DvzTexDims:
        DVZ_TEX_NONE = 0
        DVZ_TEX_1D = 1
        DVZ_TEX_2D = 2
        DVZ_TEX_3D = 3

    ctypedef enum DvzTexOptions:
        DVZ_TEX_OPTIONS_NONE = 0x0000
        DVZ_TEX_OPTIONS_PERSISTENT_STAGING = 0x2000

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

    ctypedef enum DvzPipeType:
        DVZ_PIPE_NONE = 0
        DVZ_PIPE_GRAPHICS = 1
        DVZ_PIPE_COMPUTE = 2

    ctypedef enum DvzRequestAction:
        DVZ_REQUEST_ACTION_NONE = 0
        DVZ_REQUEST_ACTION_CREATE = 1
        DVZ_REQUEST_ACTION_DELETE = 2
        DVZ_REQUEST_ACTION_RESIZE = 3
        DVZ_REQUEST_ACTION_UPDATE = 4
        DVZ_REQUEST_ACTION_UPLOAD = 5
        DVZ_REQUEST_ACTION_UPFILL = 6
        DVZ_REQUEST_ACTION_DOWNLOAD = 7
        DVZ_REQUEST_ACTION_SET = 8
        DVZ_REQUEST_ACTION_GET = 9

    ctypedef enum DvzRequestObject:
        DVZ_REQUEST_OBJECT_NONE = 0
        DVZ_REQUEST_OBJECT_BOARD = 1
        DVZ_REQUEST_OBJECT_CANVAS = 2
        DVZ_REQUEST_OBJECT_DAT = 3
        DVZ_REQUEST_OBJECT_TEX = 4
        DVZ_REQUEST_OBJECT_SAMPLER = 5
        DVZ_REQUEST_OBJECT_COMPUTE = 6
        DVZ_REQUEST_OBJECT_GRAPHICS = 7
        DVZ_REQUEST_OBJECT_BEGIN = 8
        DVZ_REQUEST_OBJECT_VIEWPORT = 9
        DVZ_REQUEST_OBJECT_VERTEX = 10
        DVZ_REQUEST_OBJECT_BARRIER = 11
        DVZ_REQUEST_OBJECT_DRAW = 12
        DVZ_REQUEST_OBJECT_END = 13

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

    # Structures
    # ---------------------------------------------------------------------------------------------

    # STRUCT START

    # STRUCT END

    # Functions
    # ---------------------------------------------------------------------------------------------

    # FUNCTION START
    DvzRequester dvz_requester()

    void dvz_requester_destroy(DvzRequester* rqr)

    void dvz_requester_begin(DvzRequester* rqr)

    void dvz_requester_add(DvzRequester* rqr, DvzRequest req)

    DvzRequest* dvz_requester_end(DvzRequester* rqr, uint32_t* count)

    void dvz_request_print(DvzRequest* req)

    DvzRequest dvz_create_board(DvzRequester* rqr, uint32_t width, uint32_t height, int flags)

    DvzRequest dvz_update_board(DvzRequester* rqr, DvzId id)

    DvzRequest dvz_delete_board(DvzRequester* rqr, DvzId id)

    DvzRequest dvz_create_dat(DvzRequester* rqr, DvzBufferType type, DvzSize size, int flags)

    DvzRequest dvz_create_tex(DvzRequester* rqr, DvzTexDims dims, uvec3 shape, DvzFormat format, int flags)

    DvzRequest dvz_create_graphics(DvzRequester* rqr, DvzId board, DvzGraphicsType type, int flags)

    DvzRequest dvz_set_vertex(DvzRequester* rqr, DvzId graphics, DvzId dat)

    DvzRequest dvz_set_begin(DvzRequester* rqr, DvzId board)

    DvzRequest dvz_set_viewport(DvzRequester* rqr, DvzId board, vec2 offset, vec2 shape)

    DvzRequest dvz_set_draw(DvzRequester* rqr, DvzId board, DvzId graphics, uint32_t first_vertex, uint32_t vertex_count)

    DvzRequest dvz_set_end(DvzRequester* rqr, DvzId board)


    # FUNCTION END
