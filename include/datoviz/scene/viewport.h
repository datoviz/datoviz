/*************************************************************************************************/
/*  Viewport                                                                                     */
/*************************************************************************************************/

#ifndef DVZ_HEADER_VIEWPORT
#define DVZ_HEADER_VIEWPORT



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../_math.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzViewport DvzViewport;
typedef struct _VkViewport _VkViewport;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

// NOTE: this corresponds to VkViewport, but we want to avoid the inclusion of vklite.h
struct _VkViewport
{
    float x;
    float y;
    float width;
    float height;
    float minDepth;
    float maxDepth;
};

// NOTE: must correspond to the shader structure in common.glsl
struct DvzViewport
{
    _VkViewport viewport; // Vulkan viewport
    vec4 margins;

    // Position and size of the viewport in screen coordinates.
    uvec2 offset_screen;
    uvec2 size_screen;

    // Position and size of the viewport in framebuffer coordinates.
    uvec2 offset_framebuffer;
    uvec2 size_framebuffer;

    // NOTE: obsolete?
    int flags;
    // TODO: aspect ratio
};

static void dvz_viewport_print(DvzViewport* viewport)
{
    printf(
        "viewport: screen %dx%d, fb %dx%d, margins %.0f %.0f %.0f %.0f\n", //
        viewport->size_screen[0], viewport->size_screen[1],                //
        viewport->size_framebuffer[0], viewport->size_framebuffer[1],      //
        viewport->margins[0], viewport->margins[1], viewport->margins[2], viewport->margins[3]);
}



EXTERN_C_ON

/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Create a viewport.
 *
 * @param offset the viewport's offset
 * @param shape the viewport's shape
 * @returns the viewport
 */
DvzViewport dvz_viewport(vec2 offset, vec2 shape, int flags);



/**
 */
DVZ_EXPORT void dvz_viewport_margins(DvzViewport* viewport, vec4 margins);



/**
 * Return a default viewport
 *
 * @param width the viewport width, in framebuffer pixels
 * @param height the viewport height, in framebuffer pixels
 * @returns the viewport
 */
DVZ_EXPORT DvzViewport dvz_viewport_default(uint32_t width, uint32_t height);



EXTERN_C_OFF


#endif
