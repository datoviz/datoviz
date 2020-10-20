# WARNING: parts of this file are auto-generated

cdef extern from "../../include/visky/visky.h":

    # Numerical types
    ctypedef unsigned long uint32_t
    ctypedef char uint8_t
    ctypedef char[3] cvec3
    ctypedef struct VkyColor:
        cvec3 rgb
        uint8_t alpha


    # Opaque types
    ctypedef struct VkyApp:
        pass

    ctypedef struct VkyCanvas:
        pass

    ctypedef struct VkyScene:
        pass

    ctypedef struct VkyPanel:
        pass


    # Functions
    void log_set_level_env()
    VkyApp* vky_create_app(VkyBackendType backend, void* params)
    VkyCanvas* vky_create_canvas(VkyApp* app, uint32_t width, uint32_t height)
    VkyScene* vky_create_scene(
        VkyCanvas* canvas, VkyColor clear_color, uint32_t row_count, uint32_t col_count)
    VkyPanel* vky_get_panel(VkyScene* scene, uint32_t row, uint32_t col)
    void vky_set_controller(VkyPanel* panel, VkyControllerType controller_type, const void*)

    # void vky_add_frame_callback(canvas, callback1)

    void vky_run_app(VkyApp* app)
    void vky_close_canvas(VkyCanvas* canvas)
    void vky_destroy_app(VkyApp* app)



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
        VKY_MARKER_ARROW = 0
        VKY_MARKER_ASTERISK = 1
        VKY_MARKER_CHEVRON = 2
        VKY_MARKER_CLOVER = 3
        VKY_MARKER_CLUB = 4
        VKY_MARKER_CROSS = 5
        VKY_MARKER_DIAMOND = 6
        VKY_MARKER_DISC = 7
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
