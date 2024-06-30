/*************************************************************************************************/
/*  Common enums                                                                                 */
/*************************************************************************************************/

#ifndef DVZ_HEADER_PUBLIC_ENUMS
#define DVZ_HEADER_PUBLIC_ENUMS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Vklite, also used by request.h                                                               */
/*************************************************************************************************/

// Buffer type.
// NOTE: the enum index should correspond to the buffer index in the context->buffers container
typedef enum
{
    DVZ_BUFFER_TYPE_UNDEFINED,
    DVZ_BUFFER_TYPE_STAGING,  // 1
    DVZ_BUFFER_TYPE_VERTEX,   // 2
    DVZ_BUFFER_TYPE_INDEX,    // 3
    DVZ_BUFFER_TYPE_STORAGE,  // 4
    DVZ_BUFFER_TYPE_UNIFORM,  // 5
    DVZ_BUFFER_TYPE_INDIRECT, // 6
} DvzBufferType;

#define DVZ_BUFFER_TYPE_COUNT 6



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



// Shader format.
typedef enum
{
    DVZ_SHADER_SPIRV,
    DVZ_SHADER_GLSL,
} DvzShaderFormat;



// Texture axis.
typedef enum
{
    DVZ_SAMPLER_AXIS_U,
    DVZ_SAMPLER_AXIS_V,
    DVZ_SAMPLER_AXIS_W,
} DvzSamplerAxis;



// Filter type.
typedef enum
{
    DVZ_SAMPLER_FILTER_MIN,
    DVZ_SAMPLER_FILTER_MAG,
} DvzSamplerFilter;



// Blend type.
typedef enum
{
    DVZ_BLEND_DISABLE,
    DVZ_BLEND_ENABLE,
} DvzBlendType;



// Depth test.
typedef enum
{
    DVZ_DEPTH_TEST_DISABLE,
    DVZ_DEPTH_TEST_ENABLE,
} DvzDepthTest;



/*************************************************************************************************/
/*  Visual-specific enums                                                                        */
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



/*************************************************************************************************/
/*  Scene                                                                                        */
/*************************************************************************************************/

// Build status
typedef enum
{
    DVZ_BUILD_CLEAR,
    DVZ_BUILD_BUSY,
    DVZ_BUILD_DIRTY,
} DvzBuildStatus;



// Panel resizing.
typedef enum
{
    DVZ_PANEL_RESIZE_STRETCH = 0x00,

    DVZ_PANEL_RESIZE_FIXED_WIDTH = 0x01,
    DVZ_PANEL_RESIZE_FIXED_HEIGHT = 0x02,

    // if these 2 flags are set, FIXED_WIDTH should not be set
    DVZ_PANEL_RESIZE_FIXED_TOP_LEFT = 0x10,
    DVZ_PANEL_RESIZE_FIXED_TOP_RIGHT = 0x20,

    // if these 2 flags are set, FIXED_HEIGHT should not be set
    DVZ_PANEL_RESIZE_FIXED_BOTTOM_LEFT = 0x40,
    DVZ_PANEL_RESIZE_FIXED_BOTTOM_RIGHT = 0x80,

    DVZ_PANEL_RESIZE_FIXED_SHAPE = 0x03,
    DVZ_PANEL_RESIZE_FIXED_OFFSET = 0xF0,

    DVZ_PANEL_RESIZE_FIXED = 0xF3,
} DvzPanelResizing;



#endif
