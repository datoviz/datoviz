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
} VklGraphicsBuiltin;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct VklVertex VklVertex;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VklVertex
{
    vec3 pos;
    cvec4 color;
};



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

VKY_EXPORT VklGraphics* vkl_graphics_builtin(VklCanvas* canvas, VklGraphicsBuiltin type);



#endif
