#ifndef VKL_GRAPHICS_HEADER
#define VKL_GRAPHICS_HEADER

#include "canvas.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Graphics builtins
typedef enum
{
    VKL_GRAPHICS_NONE,
    VKL_GRAPHICS_POINTS,

    VKL_GRAPHICS_LINES,
    VKL_GRAPHICS_LINE_STRIP,
    VKL_GRAPHICS_TRIANGLES,
    VKL_GRAPHICS_TRIANGLE_STRIP,
    VKL_GRAPHICS_TRIANGLE_FAN,

    VKL_GRAPHICS_MARKER_RAW,
    VKL_GRAPHICS_MARKER,

    VKL_GRAPHICS_SEGMENT,
    VKL_GRAPHICS_ARROW,
    VKL_GRAPHICS_PATH,
    VKL_GRAPHICS_TEXT,

    VKL_GRAPHICS_MESH_RAW,
    VKL_GRAPHICS_MESH_TEXTURED,
    VKL_GRAPHICS_MESH_MULTI_TEXTURED,
    VKL_GRAPHICS_MESH_SHADED,

    VKL_GRAPHICS_FAKE_SPHERE,
    VKL_GRAPHICS_VOLUME,

    VKL_GRAPHICS_COUNT,
} VklGraphicsBuiltin;



// Transform axis
typedef enum
{
    VKL_TRANSFORM_AXIS_NONE = 0x0000,
    VKL_TRANSFORM_AXIS_X = 0x0010,
    VKL_TRANSFORM_AXIS_Y = 0x0020,
    VKL_TRANSFORM_AXIS_Z = 0x0040,
    VKL_TRANSFORM_AXIS_ALL = 0x0070,
} VklTransformAxis;



// NOTE: the numbers need to correspond to markers.glsl at the bottom.
typedef enum
{
    VKL_MARKER_DISC = 0,
    VKL_MARKER_ASTERISK = 1,
    VKL_MARKER_CHEVRON = 2,
    VKL_MARKER_CLOVER = 3,
    VKL_MARKER_CLUB = 4,
    VKL_MARKER_CROSS = 5,
    VKL_MARKER_DIAMOND = 6,
    VKL_MARKER_ARROW = 7,
    VKL_MARKER_ELLIPSE = 8,
    VKL_MARKER_HBAR = 9,
    VKL_MARKER_HEART = 10,
    VKL_MARKER_INFINITY = 11,
    VKL_MARKER_PIN = 12,
    VKL_MARKER_RING = 13,
    VKL_MARKER_SPADE = 14,
    VKL_MARKER_SQUARE = 15,
    VKL_MARKER_TAG = 16,
    VKL_MARKER_TRIANGLE = 17,
    VKL_MARKER_VBAR = 18,
} VkyMarkerType;



typedef enum
{
    VKL_CAP_TYPE_NONE = 0,
    VKL_CAP_ROUND = 1,
    VKL_CAP_TRIANGLE_IN = 2,
    VKL_CAP_TRIANGLE_OUT = 3,
    VKL_CAP_SQUARE = 4,
    VKL_CAP_BUTT = 5,
    VKL_CAP_COUNT,
} VklCapType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VklVertex VklVertex;
typedef struct VklMVP VklMVP;

typedef struct VklGraphicsPointParams VklGraphicsPointParams;
typedef struct VklGraphicsMarkerVertex VklGraphicsMarkerVertex;
typedef struct VklGraphicsMarkerParams VklGraphicsMarkerParams;
typedef struct VklGraphicsSegmentVertex VklGraphicsSegmentVertex;
typedef struct VklGraphicsTextParams VklGraphicsTextParams;
typedef struct VklGraphicsTextVertex VklGraphicsTextVertex;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VklVertex
{
    vec3 pos;
    cvec4 color;
};



struct VklMVP
{
    mat4 model;
    mat4 view;
    mat4 proj;
};



/*************************************************************************************************/
/*  Graphics points                                                                              */
/*************************************************************************************************/

struct VklGraphicsPointParams
{
    float point_size;
};



/*************************************************************************************************/
/*  Graphics marker                                                                              */
/*************************************************************************************************/

struct VklGraphicsMarkerVertex
{
    vec3 pos;
    cvec4 color;
    float size;
    // in fact a VklMarkerType but we should control the exact data type for the GPU
    uint8_t marker;
    uint8_t angle;
};

struct VklGraphicsMarkerParams
{
    vec4 edge_color;
    float edge_width;
    int32_t enable_depth;
};



/*************************************************************************************************/
/*  Graphics segment                                                                             */
/*************************************************************************************************/

struct VklGraphicsSegmentVertex
{
    vec3 P0;
    vec3 P1;
    vec4 shift;
    cvec4 color;
    float linewidth;
    VklCapType cap0;
    VklCapType cap1;
    // uint8_t transform_mode;
};



/*************************************************************************************************/
/*  Graphics text                                                                                */
/*************************************************************************************************/

struct VklGraphicsTextParams
{
    ivec2 grid_size;
    ivec2 tex_size;
};

struct VklGraphicsTextVertex
{
    vec3 pos;
    vec2 shift;
    cvec4 color;
    vec2 glyph_size;
    vec2 anchor;
    float angle;
    usvec4 glyph; // char, char_index, str_len, str_index
    // uint8_t transform_mode;
};



/*************************************************************************************************/
/*  Util functions                                                                               */
/*************************************************************************************************/

static void _graphics_text_string(VklGraphicsTextVertex* vertices, const char* str)
{
    // vertices must point to an array of at least 4*strlen Vertex structs.
    ASSERT(vertices != NULL);
    ASSERT(str != NULL);
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

VKY_EXPORT VklGraphics*
vkl_graphics_builtin(VklCanvas* canvas, VklGraphicsBuiltin type, int flags);

VKY_EXPORT VklViewport vkl_viewport_full(VklCanvas* canvas);



#endif
