#ifndef VKL_CANVAS_HEADER
#define VKL_CANVAS_HEADER

#include "vklite2.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Type definitions                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct VklCanvas
{
    VklObject obj;
    VklApp* app;

    VklWindow* window;
    uint32_t width, height;

    VklSwapchain* swapchain;
    VklImages* images[VKL_MAX_SWAPCHAIN_IMAGES]; // swapchain images
    VklImages* depth_image;

    VklRenderpass* renderpass;
    VklRenderpass* renderpass_gui;

    VklSemaphores* sync_image_acquired; // NOTE: wraps one VkSemaphore per image in flight
    VklSemaphores* sync_image_rendered;
    VklFences* sync_render_finished;

    // TODO: event system
};



/*************************************************************************************************/
/*  Canvas                                                                                       */
/*************************************************************************************************/

VKY_EXPORT VklCanvas* vkl_canvas(VklApp* app, uint32_t width, uint32_t height);

VKY_EXPORT void vkl_canvas_swapchain(VklCanvas* canvas, VklSwapchain* swapchain);

VKY_EXPORT void vkl_canvas_offscreen(VklCanvas* canvas, VklGpu* gpu);

VKY_EXPORT void vkl_canvas_create(VklCanvas* canvas);

VKY_EXPORT VklImages*
vkl_canvas_acquire_image(VklCanvas* canvas, VklSemaphores* semaphores, VklFences* fences);



#endif
