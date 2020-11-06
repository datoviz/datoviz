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
    VklImage* images[VKL_MAX_SWAPCHAIN_IMAGES]; // swapchain images
    VklImage* depth_image;

    VklRenderpass* renderpass;
    // TODO: rename to SyncDevice/SyncHost
    VklSyncGpu* sync_image_acquired; // NOTE: wraps one VkSemaphore per image in flight
    VklSyncGpu* sync_image_rendered;
    VklSyncCpu* sync_render_finished;

    VklCommands* commands[4]; // transfer, graphics, compute, gui

    // TODO: event system
};



/*************************************************************************************************/
/*  Canvas                                                                                       */
/*************************************************************************************************/

VKY_EXPORT VklCanvas* vkl_canvas(VklApp* app, uint32_t width, uint32_t height);

VKY_EXPORT void vkl_canvas_swapchain(VklCanvas* canvas, VklSwapchain* swapchain);

VKY_EXPORT void vkl_canvas_offscreen(VklCanvas* canvas, VklGpu* gpu);

VKY_EXPORT void vkl_canvas_create(VklCanvas* canvas);

VKY_EXPORT VklImage*
vkl_canvas_acquire_image(VklCanvas* canvas, VklSyncGpu* sync_gpu, VklSyncCpu* sync_cpu);



#endif
