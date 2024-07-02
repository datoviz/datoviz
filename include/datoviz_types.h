/*************************************************************************************************/
/*  Common enums                                                                                 */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PUBLIC_ENUMS
#define DVZ_HEADER_PUBLIC_ENUMS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "datoviz_keycodes.h"
#include "datoviz_math.h"



/*************************************************************************************************/
/*  Types                                                                                        */
/*************************************************************************************************/

// NOTE: we duplicate these common types here for simplicity, best would probably be to
// define them in a common file such as datoviz_types.h, used both by the public API header file
// include/datoviz.h, and by the common internal file _math.h
// typedef uint64_t DvzSize;
// typedef uint32_t DvzIndex;
// typedef uint64_t DvzId;

// typedef uint8_t cvec2[2];
// typedef uint8_t cvec3[3];
// typedef uint8_t cvec4[4];

// typedef double dvec2[2];
// typedef double dvec3[3];
// typedef double dvec4[4];

// typedef dvec2 dmat2[2];
// typedef dvec3 dmat3[3];
// typedef dvec4 dmat4[4];


/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzKeyboardEvent DvzKeyboardEvent;
typedef struct DvzMouseEvent DvzMouseEvent;
typedef union DvzMouseEventUnion DvzMouseEventUnion;
typedef struct DvzMouseButtonEvent DvzMouseButtonEvent;
typedef struct DvzMouseWheelEvent DvzMouseWheelEvent;
typedef struct DvzMouseDragEvent DvzMouseDragEvent;
typedef struct DvzMouseClickEvent DvzMouseClickEvent;

typedef struct DvzWindowEvent DvzWindowEvent;
typedef struct DvzFrameEvent DvzFrameEvent;
typedef struct DvzTimerEvent DvzTimerEvent;
typedef struct DvzRequestsEvent DvzRequestsEvent;

// Forward declarations.
typedef struct DvzTimerItem DvzTimerItem;
typedef struct DvzBatch DvzBatch;
typedef struct DvzApp DvzApp;

// Callback types.
typedef void (*DvzAppGui)(DvzApp* app, DvzId canvas_id, void* user_data);
typedef void (*DvzAppMouseCallback)(DvzApp* app, DvzId window_id, DvzMouseEvent ev);
typedef void (*DvzAppKeyboardCallback)(DvzApp* app, DvzId window_id, DvzKeyboardEvent ev);
typedef void (*DvzAppFrameCallback)(DvzApp* app, DvzId window_id, DvzFrameEvent ev);
typedef void (*DvzAppTimerCallback)(DvzApp* app, DvzId window_id, DvzTimerEvent ev);
typedef void (*DvzAppResizeCallback)(DvzApp* app, DvzId window_id, DvzWindowEvent ev);



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Canvas creation flags.
typedef enum
{
    DVZ_CANVAS_FLAGS_NONE = 0x0000,
    DVZ_CANVAS_FLAGS_IMGUI = 0x0001,
    DVZ_CANVAS_FLAGS_FPS = 0x0003,     // NOTE: 1 bit for ImGUI, 1 bit for FPS
    DVZ_CANVAS_FLAGS_MONITOR = 0x0005, // NOTE: 1 bit for ImGUI, 1 bit for Monitor
    DVZ_CANVAS_FLAGS_VSYNC = 0x0010,
    DVZ_CANVAS_FLAGS_PICK = 0x0020,
} DvzCanvasFlags;



// Keyboard mods
// NOTE: must match GLFW values! no mapping is done as of now
typedef enum
{
    DVZ_KEY_MODIFIER_NONE = 0x00000000,
    DVZ_KEY_MODIFIER_SHIFT = 0x00000001,
    DVZ_KEY_MODIFIER_CONTROL = 0x00000002,
    DVZ_KEY_MODIFIER_ALT = 0x00000004,
    DVZ_KEY_MODIFIER_SUPER = 0x00000008,
} DvzKeyboardModifiers;



// Keyboard event type (press or release)
typedef enum
{
    DVZ_KEYBOARD_EVENT_NONE,
    DVZ_KEYBOARD_EVENT_PRESS,
    DVZ_KEYBOARD_EVENT_RELEASE,
} DvzKeyboardEventType;



// Mouse buttons
typedef enum
{
    DVZ_MOUSE_BUTTON_NONE = 0,
    DVZ_MOUSE_BUTTON_LEFT = 1,
    DVZ_MOUSE_BUTTON_MIDDLE = 2,
    DVZ_MOUSE_BUTTON_RIGHT = 3,
} DvzMouseButton;



// Mouse states.
typedef enum
{
    DVZ_MOUSE_STATE_RELEASE = 0,
    DVZ_MOUSE_STATE_PRESS = 1,
    DVZ_MOUSE_STATE_CLICK = 3,
    DVZ_MOUSE_STATE_CLICK_PRESS = 4,
    DVZ_MOUSE_STATE_DOUBLE_CLICK = 5,
    DVZ_MOUSE_STATE_DRAGGING = 11,
} DvzMouseState;



// Mouse events.
typedef enum
{
    DVZ_MOUSE_EVENT_RELEASE = 0,
    DVZ_MOUSE_EVENT_PRESS = 1,
    DVZ_MOUSE_EVENT_MOVE = 2,
    DVZ_MOUSE_EVENT_CLICK = 3,
    DVZ_MOUSE_EVENT_DOUBLE_CLICK = 5,
    DVZ_MOUSE_EVENT_DRAG_START = 10,
    DVZ_MOUSE_EVENT_DRAG = 11,
    DVZ_MOUSE_EVENT_DRAG_STOP = 12,
    DVZ_MOUSE_EVENT_WHEEL = 20,
    DVZ_MOUSE_EVENT_ALL = 255,
} DvzMouseEventType;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzKeyboardEvent
{
    DvzKeyboardEventType type;
    DvzKeyCode key;
    int mods;
    void* user_data;
};



struct DvzMouseButtonEvent
{
    DvzMouseButton button;
};

struct DvzMouseWheelEvent
{
    vec2 dir;
};

struct DvzMouseDragEvent
{
    DvzMouseButton button;
    vec2 cur_pos; // press_pos is in the mouse event itself
    vec2 shift;
};

struct DvzMouseClickEvent
{
    DvzMouseButton button;
};

union DvzMouseEventUnion
{
    DvzMouseButtonEvent b;
    DvzMouseWheelEvent w;
    DvzMouseDragEvent d;
    DvzMouseClickEvent c;
};

struct DvzMouseEvent
{
    DvzMouseEventType type;
    DvzMouseEventUnion content;
    vec2 pos;
    int mods;
    float content_scale;
    void* user_data;
};



struct DvzWindowEvent
{
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint32_t screen_width;
    uint32_t screen_height;
    int flags;
    void* user_data;
};

struct DvzFrameEvent
{
    uint64_t frame_idx;
    double time;
    double interval;
    void* user_data;
};

struct DvzTimerEvent
{
    uint32_t timer_idx;
    DvzTimerItem* timer_item;
    uint64_t step_idx;
    double time;
    void* user_data;
};

struct DvzRequestsEvent
{
    // uint32_t request_count;
    // void* requests;
    DvzBatch* batch;
    void* user_data;
};



/*************************************************************************************************/
/*  Vulkan wrapper enums, avoiding dependency to vulkan.h                                        */
/*  WARNING: they must match exactly the corresponding Vulkan enums.                             */
/*************************************************************************************************/

// VkPrimitiveTopology wrapper.
typedef enum
{
    DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST = 0,
    DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST = 1,
    DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP = 2,
    DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST = 3,
    DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP = 4,
    DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN = 5,

} DvzPrimitiveTopology;



// VkFormat wrapper.
typedef enum
{
    DVZ_FORMAT_NONE = 0,
    DVZ_FORMAT_R8_UNORM = 9,
    DVZ_FORMAT_R8_SNORM = 10,
    DVZ_FORMAT_R8_UINT = 13,
    DVZ_FORMAT_R8G8B8_UNORM = 23,
    DVZ_FORMAT_R8G8B8A8_UNORM = 37,
    DVZ_FORMAT_R8G8B8A8_UINT = 41,
    DVZ_FORMAT_B8G8R8A8_UNORM = 44,
    DVZ_FORMAT_R16_UNORM = 70,
    DVZ_FORMAT_R16_SNORM = 71,
    DVZ_FORMAT_R32_UINT = 98,
    DVZ_FORMAT_R32_SINT = 99,
    DVZ_FORMAT_R32_SFLOAT = 100,
    DVZ_FORMAT_R32G32_SFLOAT = 103,
    DVZ_FORMAT_R32G32B32_SFLOAT = 106,
    DVZ_FORMAT_R32G32B32A32_SFLOAT = 109,
} DvzFormat;



// VkFilter wrapper.
typedef enum
{
    DVZ_FILTER_NEAREST = 0,
    DVZ_FILTER_LINEAR = 1,
    DVZ_FILTER_CUBIC_IMG = 1000015000,
} DvzFilter;



// VkSamplerAddressMode wrapper.
typedef enum
{
    DVZ_SAMPLER_ADDRESS_MODE_REPEAT = 0,
    DVZ_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT = 1,
    DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE = 2,
    DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER = 3,
    DVZ_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE = 4,
} DvzSamplerAddressMode;



/*************************************************************************************************/
/*  Visual-specific enums                                                                        */
/*************************************************************************************************/

// Marker shape.
// NOTE: the numbers need to correspond to graphics_markers.frag.
typedef enum
{
    DVZ_MARKER_SHAPE_DISC = 0,
    DVZ_MARKER_SHAPE_ASTERISK = 1,
    DVZ_MARKER_SHAPE_CHEVRON = 2,
    DVZ_MARKER_SHAPE_CLOVER = 3,
    DVZ_MARKER_SHAPE_CLUB = 4,
    DVZ_MARKER_SHAPE_CROSS = 5,
    DVZ_MARKER_SHAPE_DIAMOND = 6,
    DVZ_MARKER_SHAPE_ARROW = 7,
    DVZ_MARKER_SHAPE_ELLIPSE = 8,
    DVZ_MARKER_SHAPE_HBAR = 9,
    DVZ_MARKER_SHAPE_HEART = 10,
    DVZ_MARKER_SHAPE_INFINITY = 11,
    DVZ_MARKER_SHAPE_PIN = 12,
    DVZ_MARKER_SHAPE_RING = 13,
    DVZ_MARKER_SHAPE_SPADE = 14,
    DVZ_MARKER_SHAPE_SQUARE = 15,
    DVZ_MARKER_SHAPE_TAG = 16,
    DVZ_MARKER_SHAPE_TRIANGLE = 17,
    DVZ_MARKER_SHAPE_VBAR = 18,
    DVZ_MARKER_SHAPE_COUNT,
} DvzMarkerShape;



// Marker mode.
typedef enum
{
    DVZ_MARKER_MODE_NONE = 0,
    DVZ_MARKER_MODE_CODE = 1,
    DVZ_MARKER_MODE_BITMAP = 2,
    DVZ_MARKER_MODE_SDF = 3,
    DVZ_MARKER_MODE_MSDF = 4,
    DVZ_MARKER_MODE_MTSDF = 5,
} DvzMarkerMode;



// Marker aspect.
typedef enum
{
    DVZ_MARKER_ASPECT_FILLED = 0,
    DVZ_MARKER_ASPECT_STROKE = 1,
    DVZ_MARKER_ASPECT_OUTLINE = 2,
} DvzMarkerAspect;



// Cap type.
typedef enum
{
    DVZ_CAP_TYPE_NONE = 0,
    DVZ_CAP_ROUND = 1,
    DVZ_CAP_TRIANGLE_IN = 2,
    DVZ_CAP_TRIANGLE_OUT = 3,
    DVZ_CAP_SQUARE = 4,
    DVZ_CAP_BUTT = 5,
    DVZ_CAP_COUNT,
} DvzCapType;



// Joint type.
typedef enum
{
    DVZ_JOIN_SQUARE = 0,
    DVZ_JOIN_ROUND = 1,
} DvzJoinType;



// Path topology.
typedef enum
{
    DVZ_PATH_OPEN,
    DVZ_PATH_CLOSED,
} DvzPathTopology;



// Shape type.
typedef enum
{
    DVZ_SHAPE_NONE,
    DVZ_SHAPE_SQUARE,
    DVZ_SHAPE_DISC,
    DVZ_SHAPE_CUBE,
    DVZ_SHAPE_SPHERE,
    DVZ_SHAPE_CYLINDER,
    DVZ_SHAPE_CONE,
    DVZ_SHAPE_OBJ,
    DVZ_SHAPE_OTHER,
} DvzShapeType;



// Easing.
typedef enum
{
    DVZ_EASING_NONE,
    DVZ_EASING_IN_SINE,
    DVZ_EASING_OUT_SINE,
    DVZ_EASING_IN_OUT_SINE,
    DVZ_EASING_IN_QUAD,
    DVZ_EASING_OUT_QUAD,
    DVZ_EASING_IN_OUT_QUAD,
    DVZ_EASING_IN_CUBIC,
    DVZ_EASING_OUT_CUBIC,
    DVZ_EASING_IN_OUT_CUBIC,
    DVZ_EASING_IN_QUART,
    DVZ_EASING_OUT_QUART,
    DVZ_EASING_IN_OUT_QUART,
    DVZ_EASING_IN_QUINT,
    DVZ_EASING_OUT_QUINT,
    DVZ_EASING_IN_OUT_QUINT,
    DVZ_EASING_IN_EXPO,
    DVZ_EASING_OUT_EXPO,
    DVZ_EASING_IN_OUT_EXPO,
    DVZ_EASING_IN_CIRC,
    DVZ_EASING_OUT_CIRC,
    DVZ_EASING_IN_OUT_CIRC,
    DVZ_EASING_IN_BACK,
    DVZ_EASING_OUT_BACK,
    DVZ_EASING_IN_OUT_BACK,
    DVZ_EASING_IN_ELASTIC,
    DVZ_EASING_OUT_ELASTIC,
    DVZ_EASING_IN_OUT_ELASTIC,
    DVZ_EASING_IN_BOUNCE,
    DVZ_EASING_OUT_BOUNCE,
    DVZ_EASING_IN_OUT_BOUNCE,
    DVZ_EASING_COUNT,
} DvzEasing;



// NOTE: must correspond to values in common.glsl
typedef enum
{
    DVZ_VIEWPORT_CLIP_INNER = 0x0001,
    DVZ_VIEWPORT_CLIP_OUTER = 0x0002,
    DVZ_VIEWPORT_CLIP_BOTTOM = 0x0004,
    DVZ_VIEWPORT_CLIP_LEFT = 0x0008,
} DvzViewportClip;



/*************************************************************************************************/
/*  Colormaps enums                                                                              */
/*************************************************************************************************/

// Colormaps: native, user-defined, total.
#define CMAP_OFS     0                     //   0
#define CMAP_NAT     144                   // 144
#define CMAP_USR_OFS CMAP_NAT              // 144
#define CMAP_USR     32                    //  32
#define CMAP_TOT     (CMAP_NAT + CMAP_USR) // 176

// Color palettes with 256 colors each: native, user-defined, total.
#define CPAL256_OFS     CMAP_TOT                    // 176
#define CPAL256_NAT     32                          //  32
#define CPAL256_USR_OFS (CPAL256_OFS + CPAL256_NAT) // 208
#define CPAL256_USR     32                          //  32
#define CPAL256_TOT     (CPAL256_NAT + CPAL256_USR) //  64

// Color palettes with 32 colors each: native, user-defined, total.
// There are 8 palettes per row in the texture (8*32=256)
#define CPAL032_OFS     (CPAL256_OFS + CPAL256_TOT) // 240
#define CPAL032_NAT     8                           //   8
#define CPAL032_USR_OFS (CPAL032_OFS + CPAL032_NAT) // 248
#define CPAL032_USR     8                           //   8
#define CPAL032_PER_ROW 8                           //   8
#define CPAL032_SIZ     32                          //  32
#define CPAL032_TOT     (CPAL032_NAT + CPAL032_USR) //  16

#define CMAP_COUNT 256

// Custom colormaps.
#define CMAP_CUSTOM_COUNT 16
#define CMAP_CUSTOM       (CMAP_TOT - CMAP_CUSTOM_COUNT)    // 160
#define CPAL256_CUSTOM    (CPAL032_OFS - CMAP_CUSTOM_COUNT) // 224
// TODO: CPAL032 custom

// Colormaps.
typedef enum
{
    // Standard colormaps
    DVZ_CMAP_BINARY, // grey level
    DVZ_CMAP_HSV,    // all HSV hues

    // matplotlib perceptually uniform

    DVZ_CMAP_CIVIDIS,
    DVZ_CMAP_INFERNO,
    DVZ_CMAP_MAGMA,
    DVZ_CMAP_PLASMA,
    DVZ_CMAP_VIRIDIS,

    // matplotlib sequential

    DVZ_CMAP_BLUES,
    DVZ_CMAP_BUGN,
    DVZ_CMAP_BUPU,
    DVZ_CMAP_GNBU,
    DVZ_CMAP_GREENS,
    DVZ_CMAP_GREYS,
    DVZ_CMAP_ORANGES,
    DVZ_CMAP_ORRD,
    DVZ_CMAP_PUBU,
    DVZ_CMAP_PUBUGN,
    DVZ_CMAP_PURPLES,
    DVZ_CMAP_RDPU,
    DVZ_CMAP_REDS,
    DVZ_CMAP_YLGN,
    DVZ_CMAP_YLGNBU,
    DVZ_CMAP_YLORBR,
    DVZ_CMAP_YLORRD,

    // matplotlib sequential 2

    DVZ_CMAP_AFMHOT,
    DVZ_CMAP_AUTUMN,
    DVZ_CMAP_BONE,
    DVZ_CMAP_COOL,
    DVZ_CMAP_COPPER,
    DVZ_CMAP_GIST_HEAT,
    DVZ_CMAP_GRAY,
    DVZ_CMAP_HOT,
    DVZ_CMAP_PINK,
    DVZ_CMAP_SPRING,
    DVZ_CMAP_SUMMER,
    DVZ_CMAP_WINTER,
    DVZ_CMAP_WISTIA,

    // matplotlib diverging

    DVZ_CMAP_BRBG,
    DVZ_CMAP_BWR,
    DVZ_CMAP_COOLWARM,
    DVZ_CMAP_PIYG,
    DVZ_CMAP_PRGN,
    DVZ_CMAP_PUOR,
    DVZ_CMAP_RDBU,
    DVZ_CMAP_RDGY,
    DVZ_CMAP_RDYLBU,
    DVZ_CMAP_RDYLGN,
    DVZ_CMAP_SEISMIC,
    DVZ_CMAP_SPECTRAL,

    // matplotlib cyclic

    DVZ_CMAP_TWILIGHT_SHIFTED,
    DVZ_CMAP_TWILIGHT,

    // matplotlib misc

    DVZ_CMAP_BRG,
    DVZ_CMAP_CMRMAP,
    DVZ_CMAP_CUBEHELIX,
    DVZ_CMAP_FLAG,
    DVZ_CMAP_GIST_EARTH,
    DVZ_CMAP_GIST_NCAR,
    DVZ_CMAP_GIST_RAINBOW,
    DVZ_CMAP_GIST_STERN,
    DVZ_CMAP_GNUPLOT2,
    DVZ_CMAP_GNUPLOT,
    DVZ_CMAP_JET,
    DVZ_CMAP_NIPY_SPECTRAL,
    DVZ_CMAP_OCEAN,
    DVZ_CMAP_PRISM,
    DVZ_CMAP_RAINBOW,
    DVZ_CMAP_TERRAIN,

    // colorcet diverging

    DVZ_CMAP_BKR,
    DVZ_CMAP_BKY,
    DVZ_CMAP_CET_D10,
    DVZ_CMAP_CET_D11,
    DVZ_CMAP_CET_D8,
    DVZ_CMAP_CET_D13,
    DVZ_CMAP_CET_D3,
    DVZ_CMAP_CET_D1A,
    DVZ_CMAP_BJY,
    DVZ_CMAP_GWV,
    DVZ_CMAP_BWY,
    DVZ_CMAP_CET_D12,
    DVZ_CMAP_CET_R3,
    DVZ_CMAP_CET_D9,
    DVZ_CMAP_CWR,

    // colorcet colorblind

    DVZ_CMAP_CET_CBC1,
    DVZ_CMAP_CET_CBC2,
    DVZ_CMAP_CET_CBL1,
    DVZ_CMAP_CET_CBL2,
    DVZ_CMAP_CET_CBTC1,
    DVZ_CMAP_CET_CBTC2,
    DVZ_CMAP_CET_CBTL1,

    // colorcet others

    DVZ_CMAP_BGY,
    DVZ_CMAP_BGYW,
    DVZ_CMAP_BMW,
    DVZ_CMAP_CET_C1,
    DVZ_CMAP_CET_C1S,
    DVZ_CMAP_CET_C2,
    DVZ_CMAP_CET_C4,
    DVZ_CMAP_CET_C4S,
    DVZ_CMAP_CET_C5,
    DVZ_CMAP_CET_I1,
    DVZ_CMAP_CET_I3,
    DVZ_CMAP_CET_L10,
    DVZ_CMAP_CET_L11,
    DVZ_CMAP_CET_L12,
    DVZ_CMAP_CET_L16,
    DVZ_CMAP_CET_L17,
    DVZ_CMAP_CET_L18,
    DVZ_CMAP_CET_L19,
    DVZ_CMAP_CET_L4,
    DVZ_CMAP_CET_L7,
    DVZ_CMAP_CET_L8,
    DVZ_CMAP_CET_L9,
    DVZ_CMAP_CET_R1,
    DVZ_CMAP_CET_R2,
    DVZ_CMAP_COLORWHEEL,
    DVZ_CMAP_FIRE,
    DVZ_CMAP_ISOLUM,
    DVZ_CMAP_KB,
    DVZ_CMAP_KBC,
    DVZ_CMAP_KG,
    DVZ_CMAP_KGY,
    DVZ_CMAP_KR,

    // Moreland colormaps

    DVZ_CMAP_BLACK_BODY,
    DVZ_CMAP_KINDLMANN,
    DVZ_CMAP_EXTENDED_KINDLMANN,

    // colorcet palettes with 256 colors

    DVZ_CPAL256_GLASBEY = CPAL256_OFS,
    DVZ_CPAL256_GLASBEY_COOL,
    DVZ_CPAL256_GLASBEY_DARK,
    DVZ_CPAL256_GLASBEY_HV,
    DVZ_CPAL256_GLASBEY_LIGHT,
    DVZ_CPAL256_GLASBEY_WARM,

    // matplotlib palettes with <= 32 colors

    DVZ_CPAL032_ACCENT = CPAL032_OFS,
    DVZ_CPAL032_DARK2,
    DVZ_CPAL032_PAIRED,
    DVZ_CPAL032_PASTEL1,
    DVZ_CPAL032_PASTEL2,
    DVZ_CPAL032_SET1,
    DVZ_CPAL032_SET2,
    DVZ_CPAL032_SET3,

    // (new row in the texture after 8 palettes)

    DVZ_CPAL032_TAB10,
    DVZ_CPAL032_TAB20,
    DVZ_CPAL032_TAB20B,
    DVZ_CPAL032_TAB20C,

    // bokeh palettes with <= 32 colors

    DVZ_CPAL032_CATEGORY10_10,
    DVZ_CPAL032_CATEGORY20_20,
    DVZ_CPAL032_CATEGORY20B_20,
    DVZ_CPAL032_CATEGORY20C_20,

    // (new row in the texture after 8 palettes)

    // BUG: this is 256, =0 with uint8, so this colormap is not working atm
    DVZ_CPAL032_COLORBLIND8,

    // OS palettes

    // DVZ_CPAL032_WINDOWS_16,
    // DVZ_CPAL032_WINDOWS_20,
    // DVZ_CPAL032_APPLE_16,
    // DVZ_CPAL032_RISC_16,
    // DVZ_CPAL032_WEB_16,

} DvzColormap;



#endif
