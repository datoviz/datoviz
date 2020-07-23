#ifndef VKY_AGG_HEADER
#define VKY_AGG_HEADER

#include "scene.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    VKY_CAP_TYPE_NONE,
    VKY_CAP_ROUND,
    VKY_CAP_TRIANGLE_IN,
    VKY_CAP_TRIANGLE_OUT,
    VKY_CAP_SQUARE,
    VKY_CAP_BUTT
} VkyCapType;

typedef enum
{
    VKY_JOIN_SQUARE = false,
    VKY_JOIN_ROUND = true
} VkyJoinType;

// NOTE: the numbers need to correspond to markers.glsl at the bottom.
typedef enum
{
    VKY_MARKER_ARROW = 0,
    VKY_MARKER_ASTERISK = 1,
    VKY_MARKER_CHEVRON = 2,
    VKY_MARKER_CLOVER = 3,
    VKY_MARKER_CLUB = 4,
    VKY_MARKER_CROSS = 5,
    VKY_MARKER_DIAMOND = 6,
    VKY_MARKER_DISC = 7,
    VKY_MARKER_ELLIPSE = 8,
    VKY_MARKER_HBAR = 9,
    VKY_MARKER_HEART = 10,
    VKY_MARKER_INFINITY = 11,
    VKY_MARKER_PIN = 12,
    VKY_MARKER_RING = 13,
    VKY_MARKER_SPADE = 14,
    VKY_MARKER_SQUARE = 15,
    VKY_MARKER_TAG = 16,
    VKY_MARKER_TRIANGLE = 17,
    VKY_MARKER_VBAR = 18,
} VkyMarkerType;

// NOTE: the numbers need to correspond to arrows.glsl at the bottom.
typedef enum
{
    VKY_ARROW_CURVED = 0,
    VKY_ARROW_STEALTH = 1,
    VKY_ARROW_ANGLE_30 = 2,
    VKY_ARROW_ANGLE_60 = 3,
    VKY_ARROW_ANGLE_90 = 4,
    VKY_ARROW_TRIANGLE_30 = 5,
    VKY_ARROW_TRIANGLE_60 = 6,
    VKY_ARROW_TRIANGLE_90 = 7,
} VkyArrowType;

typedef uint8_t VkyMarkerSize;



/*************************************************************************************************/
/*  Path visual                                                                                  */
/*************************************************************************************************/

typedef enum
{
    VKY_PATH_OPEN = 0,
    VKY_PATH_CLOSED = 1
} VkyPathTopology;

typedef enum
{
    VKY_DEPTH_DISABLE = 0,
    VKY_DEPTH_ENABLE = 1
} VkyDepthStatus;

typedef struct VkyPathParams VkyPathParams;
struct VkyPathParams
{
    float linewidth;
    float miter_limit;
    int32_t cap_type;
    int32_t round_join;
    int32_t enable_depth;
};

typedef struct VkyPathData VkyPathData;
struct VkyPathData
{
    uint32_t point_count;
    vec3* points;          // size: point_count
    VkyColorBytes* colors; // size: point_count
    VkyPathTopology topology;
};

typedef struct VkyPathVertex VkyPathVertex;
struct VkyPathVertex
{
    vec3 p0;
    vec3 p1;
    vec3 p2;
    vec3 p3;
    VkyColorBytes color;
};

VKY_EXPORT VkyVisual* vky_visual_path(VkyScene* scene, VkyPathParams params);



/*************************************************************************************************/
/*  Segment visual                                                                               */
/*************************************************************************************************/

typedef struct VkySegmentVertex VkySegmentVertex;
struct VkySegmentVertex
{
    vec3 P0;
    vec3 P1;
    vec4 shift;
    VkyColorBytes color;
    float linewidth;
    int32_t cap0;
    int32_t cap1;
    uint8_t is_static;
};

VKY_EXPORT VkyVisual* vky_visual_segment(VkyScene* scene);



/*************************************************************************************************/
/*  Markers visual                                                                               */
/*************************************************************************************************/

typedef struct VkyMarkersVertex VkyMarkersVertex;
struct VkyMarkersVertex
{
    vec3 pos;
    VkyColorBytes color;
    VkyMarkerSize size;
    uint8_t
        marker; // in fact a VkyMarkerType but we should control the exact data type for the GPU
    uint8_t angle;
};

typedef struct VkyMarkersParams VkyMarkersParams;
struct VkyMarkersParams
{
    vec4 edge_color;
    float edge_width;
};

VKY_EXPORT VkyVisual*
vky_visual_marker(VkyScene* scene, VkyMarkersParams params, bool enable_depth);



/*************************************************************************************************/
/*  Text visual                                                                                  */
/*************************************************************************************************/

static const char VKY_TEXT_CHARS[] =
    " !\"#$%&'()*+,-./"
    "0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7f";

typedef struct VkyTextData VkyTextData;
struct VkyTextData
{
    vec3 pos;
    vec2 shift;
    VkyColorBytes color;
    float glyph_size;
    vec2 anchor;
    float angle;
    uint32_t string_len;
    char* string;
    bool is_static;
};

typedef struct VkyTextVertex VkyTextVertex;
struct VkyTextVertex
{
    vec3 pos;
    vec2 shift;
    VkyColorBytes color;
    vec2 glyph_size;
    vec2 anchor;
    float angle;
    usvec4 glyph; // char, char_index, str_len, str_index
    uint8_t is_static;
};

typedef struct VkyTextParams VkyTextParams;
struct VkyTextParams
{
    ivec2 grid_size;
    ivec2 tex_size;
};

VKY_EXPORT VkyVisual* vky_visual_text(VkyScene* scene);



/*************************************************************************************************/
/*  Arrow visual                                                                                 */
/*************************************************************************************************/

typedef struct VkyArrowVertex VkyArrowVertex;
struct VkyArrowVertex
{
    vec3 P0;
    vec3 P1;
    VkyColorBytes color;
    float head;
    float linewidth;
    float arrow_type;
};

VKY_EXPORT VkyVisual* vky_visual_arrow(VkyScene* scene);



#endif
