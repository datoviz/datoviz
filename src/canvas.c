#include "../include/visky/canvas.h"
#include <stdlib.h>



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Canvas                                                                                       */
/*************************************************************************************************/

VklCanvas* vkl_canvas(VklApp* app, uint32_t width, uint32_t height)
{
    ASSERT(app != NULL);
    if (app->canvases == NULL)
    {
        INSTANCES_INIT(VklCanvas, app, canvases, VKL_MAX_WINDOWS, VKL_OBJECT_TYPE_CANVAS)
    }

    INSTANCE_NEW(VklCanvas, canvas, app->canvases, app->canvas_count)
    canvas->app = app;
    canvas->width = width;
    canvas->height = height;

    return canvas;
}



void vkl_canvas_swapchain(VklCanvas* canvas, VklSwapchain* swapchain)
{
    // TODO
    // obtain swapchain images
    // update image_count
    // create depth image and image view
}



void vkl_canvas_offscreen(VklCanvas* canvas, VklGpu* gpu)
{
    // TODO
    // create 1 image, image view, depth image, depth image view
}



VklImage* vkl_canvas_acquire_image(VklCanvas* canvas, VklSyncGpu* sync_gpu, VklSyncCpu* sync_cpu)
{
    // TODO
    return NULL;
}



void vkl_canvas_create(VklCanvas* canvas)
{
    // must call offscreen or swapchain before
    // create renderpass, sync, predefined command buffers
    // if swapchain
    // create framebuffers
    // event system
}



void vkl_canvas_destroy(VklCanvas* canvas)
{
    if (canvas == NULL || canvas->obj.status == VKL_OBJECT_STATUS_DESTROYED)
    {
        log_trace("skip destruction of already-destroyed canvas");
        return;
    }
    // TODO

    obj_destroyed(&canvas->obj);
}



void vkl_canvases_destroy(uint32_t canvas_count, VklCanvas* canvases)
{
    for (uint32_t i = 0; i < canvas_count; i++)
    {
        vkl_canvas_destroy(&canvases[i]);
    }
}
