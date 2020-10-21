# WARNING: parts of this file are auto-generated

cdef extern from "../../include/visky/visky.h":

    # Numerical types
    ctypedef long int32_t
    ctypedef unsigned long uint32_t
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

    ctypedef struct VkyColor:
        cvec3 rgb
        uint8_t alpha

    ctypedef struct VkyPanelIndex:
        uint32_t row
        uint32_t col


    # Opaque types
    ctypedef struct VkyVideo:
        pass
    ctypedef struct VkyApp:
        pass

    ctypedef struct VkyCanvas:
        VkyApp* app

    ctypedef struct VkyScene:
        VkyCanvas* canvas

    ctypedef struct VkyPanel:
        VkyScene* scene

    ctypedef struct VkyVisual:
        VkyScene* scene


    # Callback function types
    ctypedef void (*VkyFrameCallback)(VkyCanvas*, void*)


    # NOTE: to remove later
    ctypedef enum  VkFormat:
        VK_FORMAT_R8_UNORM = 9
    ctypedef struct VkyTextureParams:
        uint32_t width
        uint32_t height
        uint32_t depth
        uint8_t format_bytes
        VkFormat format



    # Functions
    void log_set_level_env()
    VkyApp* vky_create_app(VkyBackendType backend, void* params)
    VkyCanvas* vky_create_canvas(VkyApp* app, uint32_t width, uint32_t height)
    VkyScene* vky_create_scene(
        VkyCanvas* canvas, VkyColor clear_color, uint32_t row_count, uint32_t col_count)
    VkyPanel* vky_get_panel(VkyScene* scene, uint32_t row, uint32_t col)
    VkyPanelIndex vky_get_panel_index(VkyPanel* panel)
    void vky_set_controller(VkyPanel* panel, VkyControllerType controller_type, const void*)

    VkyVisual* vky_visual(VkyScene* scene, VkyVisualType visual_type, const void* params, const void* obj)

    void vky_add_visual_to_panel(VkyVisual* visual, VkyPanel* panel, VkyViewportType viewport_type, VkyVisualPriority priority)

    void vky_visual_data_set_size(
        VkyVisual* visual, uint32_t item_count,
        uint32_t group_count, const uint32_t* group_lengths, const void* group_params)

    void vky_visual_data(
        VkyVisual* visual, VkyVisualPropType prop_type, uint32_t prop_index,
        uint32_t value_count, const void* values)

    VkyTextureParams vky_default_texture_params(uint32_t width, uint32_t height, uint32_t depth);
    void vky_visual_image_upload(VkyVisual* visual, const void* image)

    void vky_add_frame_callback(VkyCanvas* canvas, VkyFrameCallback callback, void* data)
    VkyMouse* vky_event_mouse(VkyCanvas* canvas)
    VkyKeyboard* vky_event_keyboard(VkyCanvas* canvas)

    void vky_prompt(VkyCanvas* canvas)
    char* vky_prompt_get(VkyCanvas* canvas)

    void vky_run_app(VkyApp* app)
    void vky_close_canvas(VkyCanvas* canvas)
    void vky_destroy_app(VkyApp* app)




    # ---------------------------------------------------------------------------------------------
    # AUTOMATICALLY-GENERATED PART:
    # ---------------------------------------------------------------------------------------------

    # STRUCT START
    # from file: app.h

    ctypedef struct VkyBackendVideoParams:
        char* filename
        int fps
        int bitrate
        double duration
        VkyVideo* video

    ctypedef struct VkyBackendScreenshotParams:
        char* filename
        uint32_t frame_index

    ctypedef struct VkyMouse:
        VkyMouseButton button
        vec2 press_pos
        vec2 last_pos
        vec2 cur_pos
        vec2 wheel_delta
        VkyMouseState prev_state
        VkyMouseState cur_state
        double press_time
        double click_time

    ctypedef struct VkyKeyboard:
        VkyKey key
        uint32_t modifiers
        double press_time

    # from file: visuals.h

    ctypedef struct VkyRectangleParams:
        vec3 origin
        vec3 u
        vec3 v

    ctypedef struct VkyAreaParams:
        vec3 origin
        vec3 u
        vec3 v

    ctypedef struct VkyMeshParams:
        vec4 light_pos
        vec4 light_coefs
        ivec2 tex_size
        int32_t mode_color
        int32_t mode_shading
        float wire_linewidth

    ctypedef struct VkyMarkersParams:
        vec4 edge_color
        float edge_width
        int32_t enable_depth

    ctypedef struct VkyMarkersRawParams:
        vec2 marker_size
        int32_t scaling_mode
        int32_t alpha_scaling_mode

    ctypedef struct VkyMarkersTransientParams:
        float local_time

    ctypedef struct VkyPathParams:
        float linewidth
        float miter_limit
        int32_t cap_type
        int32_t round_join
        int32_t enable_depth

    ctypedef struct VkyFakeSphereParams:
        vec4 light_pos

    ctypedef struct VkyImageCmapParams:
        uint32_t cmap
        float scaling
        float alpha
        VkyTextureParams* tex_params

    ctypedef struct VkyVolumeParams:
        mat4 inv_proj_view
        mat4 normal_mat

    ctypedef struct VkyGraphParams:
        float marker_edge_width
        vec4 marker_edge_color

    ctypedef struct VkyTextParams:
        ivec2 grid_size
        ivec2 tex_size

    ctypedef struct VkyPolygonParams:
        float linewidth
        VkyColor edge_color

    ctypedef struct VkyPSLGParams:
        float linewidth
        VkyColor edge_color

    ctypedef struct VkyTriangulationParams:
        float linewidth
        VkyColor edge_color
        vec2 marker_size
        VkyColor marker_color


    # STRUCT END






    # ENUM START
    # from file: app.h

    ctypedef enum VkyKeyModifiers:
        VKY_KEY_MODIFIER_NONE = 0x00000000
        VKY_KEY_MODIFIER_SHIFT = 0x00000001
        VKY_KEY_MODIFIER_CONTROL = 0x00000002
        VKY_KEY_MODIFIER_ALT = 0x00000004
        VKY_KEY_MODIFIER_SUPER = 0x00000008

    ctypedef enum VkyMouseButton:
        VKY_MOUSE_BUTTON_NONE = 0
        VKY_MOUSE_BUTTON_LEFT = 1
        VKY_MOUSE_BUTTON_MIDDLE = 2
        VKY_MOUSE_BUTTON_RIGHT = 3

    ctypedef enum VkyMouseState:
        VKY_MOUSE_STATE_STATIC = 0
        VKY_MOUSE_STATE_DRAG = 1
        VKY_MOUSE_STATE_WHEEL = 2
        VKY_MOUSE_STATE_CLICK = 3
        VKY_MOUSE_STATE_DOUBLE_CLICK = 4

    ctypedef enum VkyKey:
        VKY_KEY_UNKNOWN = -1
        VKY_KEY_NONE = +0
        VKY_KEY_SPACE = 32
        VKY_KEY_APOSTROPHE = 39
        VKY_KEY_COMMA = 44
        VKY_KEY_MINUS = 45
        VKY_KEY_PERIOD = 46
        VKY_KEY_SLASH = 47
        VKY_KEY_0 = 48
        VKY_KEY_1 = 49
        VKY_KEY_2 = 50
        VKY_KEY_3 = 51
        VKY_KEY_4 = 52
        VKY_KEY_5 = 53
        VKY_KEY_6 = 54
        VKY_KEY_7 = 55
        VKY_KEY_8 = 56
        VKY_KEY_9 = 57
        VKY_KEY_SEMICOLON = 59
        VKY_KEY_EQUAL = 61
        VKY_KEY_A = 65
        VKY_KEY_B = 66
        VKY_KEY_C = 67
        VKY_KEY_D = 68
        VKY_KEY_E = 69
        VKY_KEY_F = 70
        VKY_KEY_G = 71
        VKY_KEY_H = 72
        VKY_KEY_I = 73
        VKY_KEY_J = 74
        VKY_KEY_K = 75
        VKY_KEY_L = 76
        VKY_KEY_M = 77
        VKY_KEY_N = 78
        VKY_KEY_O = 79
        VKY_KEY_P = 80
        VKY_KEY_Q = 81
        VKY_KEY_R = 82
        VKY_KEY_S = 83
        VKY_KEY_T = 84
        VKY_KEY_U = 85
        VKY_KEY_V = 86
        VKY_KEY_W = 87
        VKY_KEY_X = 88
        VKY_KEY_Y = 89
        VKY_KEY_Z = 90
        VKY_KEY_LEFT_BRACKET = 91
        VKY_KEY_BACKSLASH = 92
        VKY_KEY_RIGHT_BRACKET = 93
        VKY_KEY_GRAVE_ACCENT = 96
        VKY_KEY_WORLD_1 = 161
        VKY_KEY_WORLD_2 = 162
        VKY_KEY_ESCAPE = 256
        VKY_KEY_ENTER = 257
        VKY_KEY_TAB = 258
        VKY_KEY_BACKSPACE = 259
        VKY_KEY_INSERT = 260
        VKY_KEY_DELETE = 261
        VKY_KEY_RIGHT = 262
        VKY_KEY_LEFT = 263
        VKY_KEY_DOWN = 264
        VKY_KEY_UP = 265
        VKY_KEY_PAGE_UP = 266
        VKY_KEY_PAGE_DOWN = 267
        VKY_KEY_HOME = 268
        VKY_KEY_END = 269
        VKY_KEY_CAPS_LOCK = 280
        VKY_KEY_SCROLL_LOCK = 281
        VKY_KEY_NUM_LOCK = 282
        VKY_KEY_PRINT_SCREEN = 283
        VKY_KEY_PAUSE = 284
        VKY_KEY_F1 = 290
        VKY_KEY_F2 = 291
        VKY_KEY_F3 = 292
        VKY_KEY_F4 = 293
        VKY_KEY_F5 = 294
        VKY_KEY_F6 = 295
        VKY_KEY_F7 = 296
        VKY_KEY_F8 = 297
        VKY_KEY_F9 = 298
        VKY_KEY_F10 = 299
        VKY_KEY_F11 = 300
        VKY_KEY_F12 = 301
        VKY_KEY_F13 = 302
        VKY_KEY_F14 = 303
        VKY_KEY_F15 = 304
        VKY_KEY_F16 = 305
        VKY_KEY_F17 = 306
        VKY_KEY_F18 = 307
        VKY_KEY_F19 = 308
        VKY_KEY_F20 = 309
        VKY_KEY_F21 = 310
        VKY_KEY_F22 = 311
        VKY_KEY_F23 = 312
        VKY_KEY_F24 = 313
        VKY_KEY_F25 = 314
        VKY_KEY_KP_0 = 320
        VKY_KEY_KP_1 = 321
        VKY_KEY_KP_2 = 322
        VKY_KEY_KP_3 = 323
        VKY_KEY_KP_4 = 324
        VKY_KEY_KP_5 = 325
        VKY_KEY_KP_6 = 326
        VKY_KEY_KP_7 = 327
        VKY_KEY_KP_8 = 328
        VKY_KEY_KP_9 = 329
        VKY_KEY_KP_DECIMAL = 330
        VKY_KEY_KP_DIVIDE = 331
        VKY_KEY_KP_MULTIPLY = 332
        VKY_KEY_KP_SUBTRACT = 333
        VKY_KEY_KP_ADD = 334
        VKY_KEY_KP_ENTER = 335
        VKY_KEY_KP_EQUAL = 336
        VKY_KEY_LEFT_SHIFT = 340
        VKY_KEY_LEFT_CONTROL = 341
        VKY_KEY_LEFT_ALT = 342
        VKY_KEY_LEFT_SUPER = 343
        VKY_KEY_RIGHT_SHIFT = 344
        VKY_KEY_RIGHT_CONTROL = 345
        VKY_KEY_RIGHT_ALT = 346
        VKY_KEY_RIGHT_SUPER = 347
        VKY_KEY_MENU = 348
        VKY_KEY_LAST = 348

    ctypedef enum VkyBackendType:
        VKY_BACKEND_NONE = 0
        VKY_BACKEND_GLFW = 1
        VKY_BACKEND_OFFSCREEN = 10
        VKY_BACKEND_SCREENSHOT = 11
        VKY_BACKEND_VIDEO = 12

    # from file: colormaps.h

    ctypedef enum VkyColormap:
        VKY_CMAP_BINARY = 0
        VKY_CMAP_HSV = 1
        VKY_CMAP_CIVIDIS = 2
        VKY_CMAP_INFERNO = 3
        VKY_CMAP_MAGMA = 4
        VKY_CMAP_PLASMA = 5
        VKY_CMAP_VIRIDIS = 6
        VKY_CMAP_BLUES = 7
        VKY_CMAP_BUGN = 8
        VKY_CMAP_BUPU = 9
        VKY_CMAP_GNBU = 10
        VKY_CMAP_GREENS = 11
        VKY_CMAP_GREYS = 12
        VKY_CMAP_ORANGES = 13
        VKY_CMAP_ORRD = 14
        VKY_CMAP_PUBU = 15
        VKY_CMAP_PUBUGN = 16
        VKY_CMAP_PURPLES = 17
        VKY_CMAP_RDPU = 18
        VKY_CMAP_REDS = 19
        VKY_CMAP_YLGN = 20
        VKY_CMAP_YLGNBU = 21
        VKY_CMAP_YLORBR = 22
        VKY_CMAP_YLORRD = 23
        VKY_CMAP_AFMHOT = 24
        VKY_CMAP_AUTUMN = 25
        VKY_CMAP_BONE = 26
        VKY_CMAP_COOL = 27
        VKY_CMAP_COPPER = 28
        VKY_CMAP_GIST_HEAT = 29
        VKY_CMAP_GRAY = 30
        VKY_CMAP_HOT = 31
        VKY_CMAP_PINK = 32
        VKY_CMAP_SPRING = 33
        VKY_CMAP_SUMMER = 34
        VKY_CMAP_WINTER = 35
        VKY_CMAP_WISTIA = 36
        VKY_CMAP_BRBG = 37
        VKY_CMAP_BWR = 38
        VKY_CMAP_COOLWARM = 39
        VKY_CMAP_PIYG = 40
        VKY_CMAP_PRGN = 41
        VKY_CMAP_PUOR = 42
        VKY_CMAP_RDBU = 43
        VKY_CMAP_RDGY = 44
        VKY_CMAP_RDYLBU = 45
        VKY_CMAP_RDYLGN = 46
        VKY_CMAP_SEISMIC = 47
        VKY_CMAP_SPECTRAL = 48
        VKY_CMAP_TWILIGHT_SHIFTED = 49
        VKY_CMAP_TWILIGHT = 50
        VKY_CMAP_BRG = 51
        VKY_CMAP_CMRMAP = 52
        VKY_CMAP_CUBEHELIX = 53
        VKY_CMAP_FLAG = 54
        VKY_CMAP_GIST_EARTH = 55
        VKY_CMAP_GIST_NCAR = 56
        VKY_CMAP_GIST_RAINBOW = 57
        VKY_CMAP_GIST_STERN = 58
        VKY_CMAP_GNUPLOT2 = 59
        VKY_CMAP_GNUPLOT = 60
        VKY_CMAP_JET = 61
        VKY_CMAP_NIPY_SPECTRAL = 62
        VKY_CMAP_OCEAN = 63
        VKY_CMAP_PRISM = 64
        VKY_CMAP_RAINBOW = 65
        VKY_CMAP_TERRAIN = 66
        VKY_CMAP_BKR = 67
        VKY_CMAP_BKY = 68
        VKY_CMAP_CET_D10 = 69
        VKY_CMAP_CET_D11 = 70
        VKY_CMAP_CET_D8 = 71
        VKY_CMAP_CET_D13 = 72
        VKY_CMAP_CET_D3 = 73
        VKY_CMAP_CET_D1A = 74
        VKY_CMAP_BJY = 75
        VKY_CMAP_GWV = 76
        VKY_CMAP_BWY = 77
        VKY_CMAP_CET_D12 = 78
        VKY_CMAP_CET_R3 = 79
        VKY_CMAP_CET_D9 = 80
        VKY_CMAP_CWR = 81
        VKY_CMAP_CET_CBC1 = 82
        VKY_CMAP_CET_CBC2 = 83
        VKY_CMAP_CET_CBL1 = 84
        VKY_CMAP_CET_CBL2 = 85
        VKY_CMAP_CET_CBTC1 = 86
        VKY_CMAP_CET_CBTC2 = 87
        VKY_CMAP_CET_CBTL1 = 88
        VKY_CMAP_BGY = 89
        VKY_CMAP_BGYW = 90
        VKY_CMAP_BMW = 91
        VKY_CMAP_CET_C1 = 92
        VKY_CMAP_CET_C1S = 93
        VKY_CMAP_CET_C2 = 94
        VKY_CMAP_CET_C4 = 95
        VKY_CMAP_CET_C4S = 96
        VKY_CMAP_CET_C5 = 97
        VKY_CMAP_CET_I1 = 98
        VKY_CMAP_CET_I3 = 99
        VKY_CMAP_CET_L10 = 100
        VKY_CMAP_CET_L11 = 101
        VKY_CMAP_CET_L12 = 102
        VKY_CMAP_CET_L16 = 103
        VKY_CMAP_CET_L17 = 104
        VKY_CMAP_CET_L18 = 105
        VKY_CMAP_CET_L19 = 106
        VKY_CMAP_CET_L4 = 107
        VKY_CMAP_CET_L7 = 108
        VKY_CMAP_CET_L8 = 109
        VKY_CMAP_CET_L9 = 110
        VKY_CMAP_CET_R1 = 111
        VKY_CMAP_CET_R2 = 112
        VKY_CMAP_COLORWHEEL = 113
        VKY_CMAP_FIRE = 114
        VKY_CMAP_ISOLUM = 115
        VKY_CMAP_KB = 116
        VKY_CMAP_KBC = 117
        VKY_CMAP_KG = 118
        VKY_CMAP_KGY = 119
        VKY_CMAP_KR = 120
        VKY_CMAP_BLACK_BODY = 121
        VKY_CMAP_KINDLMANN = 122
        VKY_CMAP_EXTENDED_KINDLMANN = 123
        VKY_CPAL256_GLASBEY = 176
        VKY_CPAL256_GLASBEY_COOL = 125
        VKY_CPAL256_GLASBEY_DARK = 126
        VKY_CPAL256_GLASBEY_HV = 127
        VKY_CPAL256_GLASBEY_LIGHT = 128
        VKY_CPAL256_GLASBEY_WARM = 129
        VKY_CPAL032_ACCENT = 240
        VKY_CPAL032_DARK2 = 131
        VKY_CPAL032_PAIRED = 132
        VKY_CPAL032_PASTEL1 = 133
        VKY_CPAL032_PASTEL2 = 134
        VKY_CPAL032_SET1 = 135
        VKY_CPAL032_SET2 = 136
        VKY_CPAL032_SET3 = 137
        VKY_CPAL032_TAB10 = 138
        VKY_CPAL032_TAB20 = 139
        VKY_CPAL032_TAB20B = 140
        VKY_CPAL032_TAB20C = 141
        VKY_CPAL032_CATEGORY10_10 = 142
        VKY_CPAL032_CATEGORY20_20 = 143
        VKY_CPAL032_CATEGORY20B_20 = 144
        VKY_CPAL032_CATEGORY20C_20 = 145
        VKY_CPAL032_COLORBLIND8 = 146

    # from file: gui.h

    ctypedef enum VkyGuiStyle:
        VKY_GUI_STANDARD = 0
        VKY_GUI_PROMPT = 1
        VKY_GUI_FIXED_TL = 10
        VKY_GUI_FIXED_TR = 11
        VKY_GUI_FIXED_LL = 12
        VKY_GUI_FIXED_LR = 13

    ctypedef enum VkyGuiControlType:
        VKY_GUI_BUTTON = 1
        VKY_GUI_CHECKBOX = 2
        VKY_GUI_RADIO = 3
        VKY_GUI_COMBO = 4
        VKY_GUI_LISTBOX = 5
        VKY_GUI_TEXTBOX = 6
        VKY_GUI_TEXTBOX_PROMPT = 7
        VKY_GUI_TEXT = 8
        GKY_GUI_INT_STEPPER = 10
        VKY_GUI_INT_SLIDER = 11
        VKY_GUI_FLOAT_SLIDER = 12
        VKY_GUI_COLOR = 20
        VKY_GUI_FPS = 99

    ctypedef enum VkyPromptState:
        VKY_PROMPT_HIDDEN = 0
        VKY_PROMPT_SHOWN = 1
        VKY_PROMPT_ACTIVE = 2

    # from file: scene.h

    ctypedef enum VkyVisualType:
        VKY_VISUAL_EMPTY = 0
        VKY_VISUAL_RECTANGLE = 10
        VKY_VISUAL_RECTANGLE_AXIS = 11
        VKY_VISUAL_AREA = 12
        VKY_VISUAL_MESH = 20
        VKY_VISUAL_MESH_RAW = 21
        VKY_VISUAL_MARKER = 30
        VKY_VISUAL_MARKER_RAW = 31
        VKY_VISUAL_MARKER_TRANSIENT = 32
        VKY_VISUAL_SEGMENT = 35
        VKY_VISUAL_ARROW = 36
        VKY_VISUAL_GRAPH = 37
        VKY_VISUAL_PATH = 40
        VKY_VISUAL_PATH_RAW = 41
        VKY_VISUAL_PATH_RAW_MULTI = 42
        VKY_VISUAL_POLYGON = 50
        VKY_VISUAL_PSLG = 51
        VKY_VISUAL_TRIANGULATION = 52
        VKY_VISUAL_FAKE_SPHERE = 60
        VKY_VISUAL_AXES_TEXT = 70
        VKY_VISUAL_AXES_TICK = 71
        VKY_VISUAL_AXES_3D = 72
        VKY_VISUAL_AXES_3D_SEGMENTS = 73
        VKY_VISUAL_AXES_3D_TEXT = 74
        VKY_VISUAL_IMAGE = 80
        VKY_VISUAL_IMAGE_CMAP = 81
        VKY_VISUAL_VOLUME = 85
        VKY_VISUAL_VOLUME_SLICER = 86
        VKY_VISUAL_COLORBAR = 90
        VKY_VISUAL_TEXT = 100
        VKY_VISUAL_CUSTOM = 255

    ctypedef enum VkyVisualPropType:
        VKY_VISUAL_PROP_NONE = 0
        VKY_VISUAL_PROP_POS = 1
        VKY_VISUAL_PROP_POS_GPU = 2
        VKY_VISUAL_PROP_TEXTURE_COORDS = 3
        VKY_VISUAL_PROP_NORMAL = 4
        VKY_VISUAL_PROP_COLOR = 5
        VKY_VISUAL_PROP_SIZE = 10
        VKY_VISUAL_PROP_SHAPE = 11
        VKY_VISUAL_PROP_SHIFT = 12
        VKY_VISUAL_PROP_LENGTH = 13
        VKY_VISUAL_PROP_AXIS = 14
        VKY_VISUAL_PROP_TEXT = 15
        VKY_VISUAL_PROP_ANGLE = 16
        VKY_VISUAL_PROP_TIME = 17
        VKY_VISUAL_PROP_LINEWIDTH = 20
        VKY_VISUAL_PROP_EDGE_COLOR = 21
        VKY_VISUAL_PROP_EDGE_ALPHA = 22
        VKY_VISUAL_PROP_IMAGE = 24
        VKY_VISUAL_PROP_VOLUME = 25
        VKY_VISUAL_PROP_BUFFER = 26
        VKY_VISUAL_PROP_TRANSFORM = 31

    ctypedef enum VkyControllerType:
        VKY_CONTROLLER_NONE = 0
        VKY_CONTROLLER_PANZOOM = 10
        VKY_CONTROLLER_AXES_2D = 11
        VKY_CONTROLLER_ARCBALL = 20
        VKY_CONTROLLER_TURNTABLE = 21
        VKY_CONTROLLER_AUTOROTATE = 22
        VKY_CONTROLLER_AXES_3D = 23
        VKY_CONTROLLER_FLY = 30
        VKY_CONTROLLER_FPS = 31
        VKY_CONTROLLER_IMAGE = 40
        VKY_CONTROLLER_VOLUME = 41

    ctypedef enum VkyMVPMatrix:
        VKY_MVP_MODEL = 1
        VKY_MVP_VIEW = 2
        VKY_MVP_PROJ = 3
        VKY_MVP_ORTHO = 4

    ctypedef enum VkyViewportType:
        VKY_VIEWPORT_INNER = 0
        VKY_VIEWPORT_OUTER = 1

    ctypedef enum VkyAspect:
        VKY_ASPECT_UNCONSTRAINED = 0
        VKY_ASPECT_SQUARE = 1

    ctypedef enum VkyVisualPriority:
        VKY_VISUAL_PRIORITY_NONE = 0
        VKY_VISUAL_PRIORITY_FIRST = 1
        VKY_VISUAL_PRIORITY_LAST = 2

    ctypedef enum VkyPanelLinkMode:
        VKY_PANEL_LINK_NONE = 0x0
        VKY_PANEL_LINK_X = 0x1
        VKY_PANEL_LINK_Y = 0x2
        VKY_PANEL_LINK_Z = 0x4
        VKY_PANEL_LINK_ALL = 0x7

    ctypedef enum VkyTransformMode:
        VKY_TRANSFORM_MODE_NORMAL = 0x0
        VKY_TRANSFORM_MODE_X_ONLY = 0x1
        VKY_TRANSFORM_MODE_Y_ONLY = 0x2
        VKY_TRANSFORM_MODE_STATIC = 0x7

    ctypedef enum VkyControllerSource:
        VKY_CONTROLLER_SOURCE_NONE = 0
        VKY_CONTROLLER_SOURCE_HUMAN = 1
        VKY_CONTROLLER_SOURCE_MOCK = 2
        VKY_CONTROLLER_SOURCE_LINK = 3

    ctypedef enum VkyPanelStatus:
        VKY_PANEL_STATUS_NONE = 0
        VKY_PANEL_STATUS_ACTIVE = 1
        VKY_PANEL_STATUS_LINKED = 2
        VKY_PANEL_STATUS_RESET = 7

    ctypedef enum VkyAxis:
        VKY_AXIS_X = 0
        VKY_AXIS_Y = 1

    ctypedef enum VkyColorbarPosition:
        VKY_COLORBAR_NONE = 0
        VKY_COLORBAR_TOP = 1
        VKY_COLORBAR_RIGHT = 2
        VKY_COLORBAR_BOTTOM = 3
        VKY_COLORBAR_LEFT = 4

    # from file: visuals.h

    ctypedef enum VkyAlphaScalingMode:
        VKY_ALPHA_SCALING_OFF = 0
        VKY_ALPHA_SCALING_ON = 1

    ctypedef enum VkyArrowType:
        VKY_ARROW_CURVED = 0
        VKY_ARROW_STEALTH = 1
        VKY_ARROW_ANGLE_30 = 2
        VKY_ARROW_ANGLE_60 = 3
        VKY_ARROW_ANGLE_90 = 4
        VKY_ARROW_TRIANGLE_30 = 5
        VKY_ARROW_TRIANGLE_60 = 6
        VKY_ARROW_TRIANGLE_90 = 7

    ctypedef enum VkyCapType:
        VKY_CAP_TYPE_NONE = 0
        VKY_CAP_ROUND = 1
        VKY_CAP_TRIANGLE_IN = 2
        VKY_CAP_TRIANGLE_OUT = 3
        VKY_CAP_SQUARE = 4
        VKY_CAP_BUTT = 5

    ctypedef enum VkyDepthStatus:
        VKY_DEPTH_DISABLE = 0
        VKY_DEPTH_ENABLE = 1

    ctypedef enum VkyJoinType:
        VKY_JOIN_SQUARE = False
        VKY_JOIN_ROUND = True

    ctypedef enum VkyMarkerType:
        VKY_MARKER_DISC = 0
        VKY_MARKER_ASTERISK = 1
        VKY_MARKER_CHEVRON = 2
        VKY_MARKER_CLOVER = 3
        VKY_MARKER_CLUB = 4
        VKY_MARKER_CROSS = 5
        VKY_MARKER_DIAMOND = 6
        VKY_MARKER_ARROW = 7
        VKY_MARKER_ELLIPSE = 8
        VKY_MARKER_HBAR = 9
        VKY_MARKER_HEART = 10
        VKY_MARKER_INFINITY = 11
        VKY_MARKER_PIN = 12
        VKY_MARKER_RING = 13
        VKY_MARKER_SPADE = 14
        VKY_MARKER_SQUARE = 15
        VKY_MARKER_TAG = 16
        VKY_MARKER_TRIANGLE = 17
        VKY_MARKER_VBAR = 18

    ctypedef enum VkyMeshColorType:
        VKY_MESH_COLOR_RGBA = 1
        VKY_MESH_COLOR_UV = 2

    ctypedef enum VkyMeshShading:
        VKY_MESH_SHADING_NONE = 0
        VKY_MESH_SHADING_BLINN_PHONG = 1

    ctypedef enum VkyPathTopology:
        VKY_PATH_OPEN = 0
        VKY_PATH_CLOSED = 1

    ctypedef enum VkyScalingMode:
        VKY_SCALING_OFF = 0
        VKY_SCALING_ON = 1


    # ENUM END
