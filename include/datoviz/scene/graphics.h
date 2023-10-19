/*************************************************************************************************/
/*  Collection of builtin graphics pipelines                                                     */
/*************************************************************************************************/

#ifndef DVZ_HEADER_GRAPHICS
#define DVZ_HEADER_GRAPHICS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../vklite.h" // TODO: remove this dependency
#include "mvp.h"
#include "viewport.h"



/*************************************************************************************************/
/*  Constants and macros                                                                         */
/*************************************************************************************************/

// Number of common descriptors
// NOTE: must correspond to the same constant in common.glsl
#define DVZ_USER_BINDING 2

#define DVZ_MAX_GLYPHS_PER_TEXT 256



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzVertex DvzVertex;
typedef struct DvzGraphicsPointVertex DvzGraphicsPointVertex;

// Forward declarations.
typedef struct DvzRenderpass DvzRenderpass;
typedef struct DvzGraphics DvzGraphics;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzVertex
{
    vec3 pos;    /* position */
    cvec4 color; /* color */
};



struct DvzGraphicsPointVertex
{
    vec3 pos;    /* position */
    cvec4 color; /* color */
    float size;  /* marker size, in pixels */
};



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a new graphics pipeline of a given builtin type.
 *
 * @param renderpass the renderpass
 * @param graphics the graphics to create
 * @param type the graphics type
 * @param flags the creation flags for the graphics
 */
DVZ_EXPORT void dvz_graphics_builtin(
    DvzRenderpass* renderpass, DvzGraphics* graphics, DvzGraphicsType type, int flags);



EXTERN_C_OFF

#endif
