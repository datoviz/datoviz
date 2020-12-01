#ifndef VKL_GRAPHICS_HEADER
#define VKL_GRAPHICS_HEADER

#include "vklite2.h"



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

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
    VKL_GRAPHICS_MARKER_AGG,

    VKL_GRAPHICS_SEGMENT_AGG,
    VKL_GRAPHICS_ARROW_AGG,
    VKL_GRAPHICS_PATH_AGG,
    VKL_GRAPHICS_TEXT_AGG,

    VKL_GRAPHICS_MESH_RAW,
    VKL_GRAPHICS_MESH_TEXTURED,
    VKL_GRAPHICS_MESH_MULTI_TEXTURED,
    VKL_GRAPHICS_MESH_SHADED,

    VKL_GRAPHICS_FAKE_SPHERE,
    VKL_GRAPHICS_VOLUME,

    VKL_GRAPHICS_COUNT,
} VklGraphicsBuiltin;



typedef enum {
    VKL_TRANSFORM_AXIS_NONE = 0x0000,
    VKL_TRANSFORM_AXIS_X = 0x0010,
    VKL_TRANSFORM_AXIS_Y = 0x0020,
    VKL_TRANSFORM_AXIS_Z = 0x0040,
    VKL_TRANSFORM_AXIS_ALL = 0x0070,
} VklTransformAxis;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VklVertex VklVertex;
typedef struct VklViewport VklViewport;
typedef struct VklDataCoords VklDataCoords;
typedef struct VklMVP VklMVP;
typedef struct VklGraphicsPointsParams VklGraphicsPointsParams;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VklVertex
{
    vec3 pos;
    cvec4 color;
};



struct VklViewport
{
    VkViewport viewport; // Vulkan viewport
    vec4 margins;
    uvec4 screen; // (tlx, tly, w, h)
    uvec4 framebuffer; // (tlx, tly, w, h)
    float dpi_scaling; // DPI  scaling
};



// TODO
struct VklDataCoords
{
    dvec4 data; // (tlx, tly, brx, bry)
    vec4 gpu; // (tlx, tly, brx, bry)
};



struct VklMVP
{
    mat4 model;
    mat4 view;
    mat4 proj;
};



struct VklGraphicsPointsParams 
{
    float point_size;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

VKY_EXPORT VklGraphics* vkl_graphics_builtin(VklCanvas* canvas, VklGraphicsBuiltin type);



#endif
