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
/*  Enums                                                                                        */
/*************************************************************************************************/

// Viewport type.
// NOTE: must correspond to values in common.glsl
typedef enum
{
    DVZ_VIEWPORT_FLAGS_NONE = 0x00,

    // whether to integrate the margins in the transform or not
    DVZ_VIEWPORT_FLAGS_WITH_MARGINS = 0x00,
    DVZ_VIEWPORT_FLAGS_WITHOUT_MARGINS = 0x01,

    // clipping strategy
    // DVZ_VIEWPORT_FLAGS_CLIP_NONE = 0x00,
    DVZ_VIEWPORT_FLAGS_CLIP_INNER = 0x10,
    DVZ_VIEWPORT_FLAGS_CLIP_OUTER = 0x20,
    DVZ_VIEWPORT_FLAGS_CLIP_BOTTOM = 0x30,
    DVZ_VIEWPORT_FLAGS_CLIP_LEFT = 0x40,

} DvzViewportFlags;



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

    // Options
    // Viewport clipping.
    int flags; // used by the GPU for viewport clipping

    // Used to discard transform on one axis
    int32_t interact_axis;

    // TODO: aspect ratio
};



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
 * Return a default viewport
 *
 * @param width the viewport width, in framebuffer pixels
 * @param height the viewport height, in framebuffer pixels
 * @returns the viewport
 */
DVZ_EXPORT DvzViewport dvz_viewport_default(uint32_t width, uint32_t height);



EXTERN_C_OFF


#endif
