#include "../include/visky/canvas.h"
#include <stdlib.h>



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Canvas creation                                                                              */
/*************************************************************************************************/

VklCanvas* vkl_canvas(VklGpu* gpu, uint32_t width, uint32_t height)
{
    ASSERT(gpu != NULL);
    VklApp* app = gpu->app;

    ASSERT(app != NULL);
    if (app->canvases == NULL)
    {
        INSTANCES_INIT(
            VklCanvas, app, canvases, max_canvases, VKL_MAX_WINDOWS, VKL_OBJECT_TYPE_CANVAS)
    }

    INSTANCE_NEW(VklCanvas, canvas, app->canvases, app->max_canvases)
    canvas->app = app;
    canvas->width = width;
    canvas->height = height;

    INSTANCES_INIT(
        VklCommands, canvas, commands, max_commands, VKL_MAX_COMMANDS, VKL_OBJECT_TYPE_COMMANDS)
    INSTANCES_INIT(
        VklRenderpass, canvas, renderpasses, max_renderpasses, VKL_MAX_RENDERPASSES,
        VKL_OBJECT_TYPE_RENDERPASS)
    INSTANCES_INIT(
        VklSemaphores, canvas, semaphores, max_semaphores, VKL_MAX_SEMAPHORES,
        VKL_OBJECT_TYPE_SEMAPHORES)
    INSTANCES_INIT(VklFences, canvas, fences, max_fences, VKL_MAX_FENCES, VKL_OBJECT_TYPE_FENCES)
    INSTANCES_INIT(
        VklSwapchain, canvas, swapchains, max_swapchains, VKL_MAX_WINDOWS,
        VKL_OBJECT_TYPE_SWAPCHAIN)
    INSTANCES_INIT(
        VklFramebuffers, canvas, framebuffers, max_framebuffers, VKL_MAX_FRAMEBUFFERS,
        VKL_OBJECT_TYPE_FRAMEBUFFER)


    // TODO: create semaphores, fences, swap chain, renderpass, etc.

    return canvas;
}



/*************************************************************************************************/
/*  Canvas destruction                                                                           */
/*************************************************************************************************/

void vkl_canvas_destroy(VklCanvas* canvas)
{
    if (canvas == NULL || canvas->obj.status == VKL_OBJECT_STATUS_DESTROYED)
    {
        log_trace("skip destruction of already-destroyed canvas");
        return;
    }

    // TODO
    // join the background thread

    log_trace("canvas destroy commands");
    for (uint32_t i = 0; i < canvas->max_commands; i++)
    {
        if (canvas->commands[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_commands_destroy(&canvas->commands[i]);
    }
    INSTANCES_DESTROY(canvas->commands)


    log_trace("canvas destroy renderpass(es)");
    for (uint32_t i = 0; i < canvas->max_renderpasses; i++)
    {
        if (canvas->renderpasses[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_renderpass_destroy(&canvas->renderpasses[i]);
    }
    INSTANCES_DESTROY(canvas->renderpasses)


    log_trace("canvas destroy semaphores");
    for (uint32_t i = 0; i < canvas->max_semaphores; i++)
    {
        if (canvas->semaphores[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_semaphores_destroy(&canvas->semaphores[i]);
    }
    INSTANCES_DESTROY(canvas->semaphores)


    log_trace("canvas destroy fences");
    for (uint32_t i = 0; i < canvas->max_fences; i++)
    {
        if (canvas->fences[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_fences_destroy(&canvas->fences[i]);
    }
    INSTANCES_DESTROY(canvas->fences)


    log_trace("canvas destroy swapchains");
    for (uint32_t i = 0; i < canvas->max_swapchains; i++)
    {
        if (canvas->swapchains[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_swapchain_destroy(&canvas->swapchains[i]);
    }
    INSTANCES_DESTROY(canvas->swapchains)


    log_trace("canvas destroy framebuffers");
    for (uint32_t i = 0; i < canvas->max_framebuffers; i++)
    {
        if (canvas->framebuffers[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_framebuffers_destroy(&canvas->framebuffers[i]);
    }
    INSTANCES_DESTROY(canvas->framebuffers)


    obj_destroyed(&canvas->obj);
}



void vkl_canvases_destroy(uint32_t canvas_count, VklCanvas* canvases)
{
    for (uint32_t i = 0; i < canvas_count; i++)
    {
        if (canvases[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_canvas_destroy(&canvases[i]);
    }
}
