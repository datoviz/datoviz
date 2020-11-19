#include "../include/visky/canvas.h"
#include "../include/visky/context.h"
#include "../src/vklite2_utils.h"
#include <stdlib.h>


/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/


/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

static const VkClearColorValue bgcolor = {{.4f, .6f, .8f, 1.0f}};
#define VKL_DEFAULT_IMAGE_FORMAT      VK_FORMAT_B8G8R8A8_UNORM
#define VKL_DEFAULT_PRESENT_MODE      VK_PRESENT_MODE_FIFO_KHR
#define VKL_MIN_SWAPCHAIN_IMAGE_COUNT 3
#define VKL_SEMAPHORE_IMG_AVAILABLE   0
#define VKL_SEMAPHORE_RENDER_FINISHED 1
#define VKL_FENCE_RENDER_FINISHED     0
#define VKL_FENCES_FLIGHT             1
#define VKL_DEFAULT_COMMANDS_TRANSFER 0
#define VKL_DEFAULT_COMMANDS_RENDER   1
#define VKL_MAX_FRAMES_IN_FLIGHT      2



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static VklRenderpass default_renderpass(
    VklGpu* gpu, VkClearColorValue clear_color_value, VkFormat format, VkImageLayout layout)
{
    VklRenderpass renderpass = vkl_renderpass(gpu);

    VkClearValue clear_color = {0};
    clear_color.color = clear_color_value;

    VkClearValue clear_depth = {0};
    clear_depth.depthStencil.depth = 1.0f;

    vkl_renderpass_clear(&renderpass, clear_color);
    vkl_renderpass_clear(&renderpass, clear_depth);

    // Color attachment.
    vkl_renderpass_attachment(
        &renderpass, 0, //
        VKL_RENDERPASS_ATTACHMENT_COLOR, format, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    vkl_renderpass_attachment_layout(&renderpass, 0, VK_IMAGE_LAYOUT_UNDEFINED, layout);
    vkl_renderpass_attachment_ops(
        &renderpass, 0, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);

    // Depth attachment.
    vkl_renderpass_attachment(
        &renderpass, 1, //
        VKL_RENDERPASS_ATTACHMENT_DEPTH, VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    vkl_renderpass_attachment_layout(
        &renderpass, 1, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    vkl_renderpass_attachment_ops(
        &renderpass, 1, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE);

    // Subpass.
    vkl_renderpass_subpass_attachment(&renderpass, 0, 0);
    vkl_renderpass_subpass_attachment(&renderpass, 0, 1);
    vkl_renderpass_subpass_dependency(&renderpass, 0, VK_SUBPASS_EXTERNAL, 0);
    vkl_renderpass_subpass_dependency_stage(
        &renderpass, 0, //
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    vkl_renderpass_subpass_dependency_access(
        &renderpass, 0, 0,
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

    return renderpass;
}



static void
depth_image(VklImages* depth_images, VklRenderpass* renderpass, uint32_t width, uint32_t height)
{
    // Depth attachment
    vkl_images_format(depth_images, renderpass->attachments[1].format);
    vkl_images_size(depth_images, width, height, 1);
    vkl_images_tiling(depth_images, VK_IMAGE_TILING_OPTIMAL);
    vkl_images_usage(depth_images, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    vkl_images_memory(depth_images, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkl_images_layout(depth_images, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vkl_images_aspect(depth_images, VK_IMAGE_ASPECT_DEPTH_BIT);
    vkl_images_queue_access(depth_images, 0);
    vkl_images_create(depth_images);
}



static void blank_commands(VklCanvas* canvas, VklCommands* cmds)
{
    vkl_cmd_reset(cmds);
    for (uint32_t i = 0; i < cmds->count; i++)
    {
        vkl_cmd_begin(cmds, i);
        vkl_cmd_begin_renderpass(cmds, i, &canvas->renderpasses[0], &canvas->framebuffers);
        vkl_cmd_end_renderpass(cmds, i);
        vkl_cmd_end(cmds, i);
    }
}



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
    canvas->gpu = gpu;
    canvas->width = width;
    canvas->height = height;

    // Allocate memory for canvas objects.
    INSTANCES_INIT(
        VklCommands, canvas, commands, max_commands, VKL_MAX_COMMANDS, VKL_OBJECT_TYPE_COMMANDS)
    INSTANCES_INIT(
        VklRenderpass, canvas, renderpasses, max_renderpasses, VKL_MAX_RENDERPASSES,
        VKL_OBJECT_TYPE_RENDERPASS)
    INSTANCES_INIT(
        VklSemaphores, canvas, semaphores, max_semaphores, VKL_MAX_SEMAPHORES,
        VKL_OBJECT_TYPE_SEMAPHORES)
    INSTANCES_INIT(VklFences, canvas, fences, max_fences, VKL_MAX_FENCES, VKL_OBJECT_TYPE_FENCES)

    // Create the window.
    VklWindow* window = vkl_window(app, width, height);
    canvas->window = window;
    uint32_t framebuffer_width, framebuffer_height;
    vkl_window_get_size(window, &framebuffer_width, &framebuffer_height);
    ASSERT(framebuffer_width > 0);
    ASSERT(framebuffer_height > 0);

    if (gpu->context == NULL || gpu->context->obj.status < VKL_OBJECT_STATUS_CREATED)
    {
        log_trace("canvas automatically create the GPU context");
        gpu->context = vkl_context(gpu, window);
    }

    // Create default renderpass.
    INSTANCE_NEW(VklRenderpass, renderpass, canvas->renderpasses, canvas->max_renderpasses)
    *renderpass = default_renderpass(
        gpu, bgcolor, VKL_DEFAULT_IMAGE_FORMAT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // Create swapchain
    {
        canvas->swapchain = vkl_swapchain(gpu, window, VKL_MIN_SWAPCHAIN_IMAGE_COUNT);
        vkl_swapchain_format(&canvas->swapchain, VKL_DEFAULT_IMAGE_FORMAT);
        vkl_swapchain_present_mode(&canvas->swapchain, VKL_DEFAULT_PRESENT_MODE);
        vkl_swapchain_create(&canvas->swapchain);

        // Depth attachment.
        canvas->depth_image = vkl_images(gpu, VK_IMAGE_TYPE_2D, 1);
        depth_image(
            &canvas->depth_image, renderpass, //
            canvas->swapchain.images->width, canvas->swapchain.images->height);
    }

    // Create renderpass.
    vkl_renderpass_create(renderpass);

    // Create framebuffers.
    {
        canvas->framebuffers = vkl_framebuffers(gpu);
        vkl_framebuffers_attachment(&canvas->framebuffers, 0, canvas->swapchain.images);
        vkl_framebuffers_attachment(&canvas->framebuffers, 1, &canvas->depth_image);
        vkl_framebuffers_create(&canvas->framebuffers, renderpass);
    }

    // Create synchronization objects.
    {
        canvas->semaphores[VKL_SEMAPHORE_IMG_AVAILABLE] =
            vkl_semaphores(gpu, VKY_MAX_FRAMES_IN_FLIGHT);
        canvas->semaphores[VKL_SEMAPHORE_RENDER_FINISHED] =
            vkl_semaphores(gpu, VKY_MAX_FRAMES_IN_FLIGHT);
        canvas->fences[VKL_FENCE_RENDER_FINISHED] = vkl_fences(gpu, VKY_MAX_FRAMES_IN_FLIGHT);
        vkl_fences_create(&canvas->fences[VKL_FENCE_RENDER_FINISHED]);
        canvas->fences[VKL_FENCES_FLIGHT] = vkl_fences(gpu, canvas->swapchain.img_count);
    }

    // Default transfer commands.
    {
        INSTANCE_NEW(VklCommands, cmds, canvas->commands, canvas->max_commands)
        *cmds = vkl_commands(gpu, VKL_DEFAULT_QUEUE_TRANSFER, 1);
    }

    // Default render commands.
    {
        INSTANCE_NEW(VklCommands, cmds, canvas->commands, canvas->max_commands)
        *cmds = vkl_commands(gpu, VKL_DEFAULT_QUEUE_RENDER, canvas->swapchain.img_count);

        // TODO
        // use REFILL event instead
        blank_commands(canvas, cmds);
    }

    // Default submit instance.
    canvas->submit = vkl_submit(gpu);

    obj_created(&canvas->obj);

    return canvas;
}



void vkl_canvas_recreate(VklCanvas* canvas)
{
    ASSERT(canvas != NULL);
    VklBackend backend = canvas->app->backend;
    VklWindow* window = canvas->window;
    VklGpu* gpu = canvas->gpu;
    VklSwapchain* swapchain = &canvas->swapchain;
    VklFramebuffers* framebuffers = &canvas->framebuffers;
    VklRenderpass* renderpass = &canvas->renderpasses[0];

    ASSERT(window != NULL);
    ASSERT(gpu != NULL);
    ASSERT(swapchain != NULL);
    ASSERT(framebuffers != NULL);

    log_trace("recreate canvas after resize");

    // Wait until the device is ready and the window fully resized.
    // Framebuffer new size.
    uint32_t width, height;
    backend_window_get_size(
        backend, window->backend_window, //
        &window->width, &window->height, //
        &width, &height);
    vkl_gpu_wait(gpu);

    // Destroy swapchain resources.
    vkl_framebuffers_destroy(&canvas->framebuffers);
    vkl_images_destroy(&canvas->depth_image);
    vkl_images_destroy(canvas->swapchain.images);
    vkl_swapchain_destroy(swapchain);

    // Recreate the swapchain. This will automatically set the swapchain->images new
    // size.
    vkl_swapchain_create(swapchain);
    // Find the new framebuffer size as determined by the swapchain recreation.
    width = swapchain->images->width;
    height = swapchain->images->height;

    // Need to recreate the depth image with the new size.
    vkl_images_size(&canvas->depth_image, width, height, 1);
    vkl_images_create(&canvas->depth_image);

    // Recreate the framebuffers with the new size.
    ASSERT(framebuffers->attachments[0]->width == width);
    ASSERT(framebuffers->attachments[0]->height == height);
    vkl_framebuffers_create(framebuffers, renderpass);

    // TODO
    // use REFILL
    blank_commands(canvas, &canvas->commands[VKL_DEFAULT_COMMANDS_RENDER]);
}



/*************************************************************************************************/
/*  Event loop                                                                                   */
/*************************************************************************************************/

void vkl_canvas_frame(VklCanvas* canvas)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas->window != NULL);
    ASSERT(canvas->app != NULL);

    // TODO
    // call EVENT callbacks (for backends only), which may enqueue some events
    // FRAME callbacks (rarely used)

    // Wait for fence.
    vkl_fences_wait(&canvas->fences[VKL_FENCE_RENDER_FINISHED], canvas->cur_frame);

    // TODO
    // check canvas.need_refill (atomic)
    // if refill needed, and call the refill callbacks

    // We acquire the next swapchain image.
    vkl_swapchain_acquire(
        &canvas->swapchain, &canvas->semaphores[VKL_SEMAPHORE_IMG_AVAILABLE], //
        canvas->cur_frame, NULL, 0);
}



void vkl_canvas_frame_submit(VklCanvas* canvas)
{
    ASSERT(canvas != NULL);
    VklGpu* gpu = canvas->gpu;
    ASSERT(gpu != NULL);

    VklSubmit* s = &canvas->submit;
    uint32_t f = canvas->cur_frame;
    uint32_t img_idx = canvas->swapchain.img_idx;

    // Keep track of the fence associated to the current swapchain image.
    vkl_fences_copy(
        &canvas->fences[VKL_FENCE_RENDER_FINISHED], f, //
        &canvas->fences[VKL_FENCES_FLIGHT], img_idx);

    // Reset the Submit instance before adding the command buffers.
    vkl_submit_reset(s);

    // Add the command buffers to the submit instance.
    for (uint32_t i = 0; i < canvas->max_commands; i++)
    {
        if (canvas->commands[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        if (canvas->commands[i].obj.status == VKL_OBJECT_STATUS_INACTIVE)
            continue;
        if (canvas->commands[i].queue_idx == VKL_DEFAULT_QUEUE_RENDER)
        {
            vkl_submit_commands(s, &canvas->commands[i]);
        }
    }

    vkl_submit_wait_semaphores(
        s, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, //
        &canvas->semaphores[VKL_SEMAPHORE_IMG_AVAILABLE], f);
    // Once the render is finished, we signal another semaphore.
    vkl_submit_signal_semaphores(s, &canvas->semaphores[VKL_SEMAPHORE_RENDER_FINISHED], f);
    // Send the Submit instance.
    vkl_submit_send(s, img_idx, &canvas->fences[VKL_FENCE_RENDER_FINISHED], f);

    // TODO
    // call POST_SEND callback

    // Once the image is rendered, we present the swapchain image.
    vkl_swapchain_present(
        &canvas->swapchain, 1, &canvas->semaphores[VKL_SEMAPHORE_RENDER_FINISHED], f);

    canvas->cur_frame = (f + 1) % VKL_MAX_FRAMES_IN_FLIGHT;
}



void vkl_app_begin(VklApp* app)
{
    ASSERT(app != NULL);
    // TODO
    // start timer
    // start timer thread
}



void vkl_app_end(VklApp* app)
{
    ASSERT(app != NULL);

    vkl_app_wait(app);

    // TODO
    // stop timer
    // stop timer thread and join it
    // destroy app
}



void vkl_app_run(VklApp* app, uint64_t frame_count)
{
    ASSERT(app != NULL);
    if (frame_count == 0)
        frame_count = UINT64_MAX;
    ASSERT(frame_count > 0);

    vkl_app_begin(app);

    VklCanvas* canvas = NULL;

    // Main loop.
    uint32_t n_canvas_active = 0;
    for (uint64_t frame_idx = 0; frame_idx < frame_count; frame_idx++)
    {
        log_trace("frame %d/%d", frame_idx, frame_count);
        n_canvas_active = 0;

        // Loop over the canvases.
        for (uint32_t canvas_idx = 0; canvas_idx < app->max_canvases; canvas_idx++)
        {
            // Get the current canvas.
            canvas = &app->canvases[canvas_idx];
            {
                ASSERT(canvas != NULL);
                if (canvas->obj.status == VKL_OBJECT_STATUS_NONE)
                    break;
                if (canvas->obj.status < VKL_OBJECT_STATUS_CREATED)
                    continue;
                ASSERT(canvas->obj.status >= VKL_OBJECT_STATUS_CREATED);
                log_trace("processing frame for canvas #%d", canvas_idx);
            }

            // Poll events.
            ASSERT(canvas->window != NULL);
            vkl_window_poll_events(canvas->window);

            // Frame logic.
            log_trace("frame logic for canvas #%d", canvas_idx);
            // Swapchain image acquisition happens here:
            vkl_canvas_frame(canvas);

            // If there is a problem with swapchain image acquisition, wait and try again later.
            if (canvas->swapchain.obj.status == VKL_OBJECT_STATUS_INVALID)
            {
                vkl_gpu_wait(canvas->gpu);
                break;
            }

            // If the swapchain needs to be recreated (for example, after a resize), do it.
            if (canvas->swapchain.obj.status == VKL_OBJECT_STATUS_NEED_RECREATE)
            {
                log_trace("swapchain image acquisition failed, recreating the canvas");
                // TODO
                // call RESIZE callback

                // Recreate the canvas.
                vkl_canvas_recreate(canvas);
                continue;
            }

            // Destroy the canvas if needed.
            {
                if (backend_window_should_close(app->backend, canvas->window->backend_window))
                    canvas->window->obj.status = VKL_OBJECT_STATUS_NEED_DESTROY;
                if (canvas->obj.status == VKL_OBJECT_STATUS_NEED_DESTROY)
                {
                    log_trace("destroying canvas #%d", canvas_idx);
                    // Wait for all GPUs to be idle.
                    vkl_app_wait(app);

                    // Destroy the canvas.
                    vkl_canvas_destroy(canvas);
                    continue;
                }
            }

            // Submit the command buffers and swapchain logic.
            log_trace("submitting frame for canvas #%d", canvas_idx);
            vkl_canvas_frame_submit(canvas);
            n_canvas_active++;
        }

        // TODO: this has never been tested with multiple GPUs yet.
        VklGpu* gpu = NULL;
        VklContext* ctx = NULL;
        for (uint32_t gpu_idx = 0; gpu_idx < app->gpu_count; gpu_idx++)
        {
            gpu = &app->gpus[gpu_idx];
            if (gpu->obj.status < VKL_OBJECT_STATUS_CREATED)
                break;
            ctx = gpu->context;

            // Process the pending transfer tasks.
            if (ctx->obj.status >= VKL_OBJECT_STATUS_CREATED)
            {
                log_trace("processing transfers for GPU #%d", gpu_idx);
                vkl_transfer_loop(ctx, false);
            }

            // IMPORTANT: we need to wait for the present queue to be idle, otherwise the GPU hangs
            // when waiting for fences (not sure why). The problem only arises when using different
            // queues for command buffer submission and swapchain present.
            if (gpu->queues.queues[VKL_DEFAULT_QUEUE_PRESENT] !=
                gpu->queues.queues[VKL_DEFAULT_QUEUE_RENDER])
            {
                vkl_gpu_queue_wait(gpu, VKL_DEFAULT_QUEUE_PRESENT);
            }
        }

        // Close the application if all canvases have been closed.
        if (n_canvas_active == 0)
        {
            log_trace("no more active canvas, closing the app");
            break;
        }
    }
    log_trace("end main loop");

    vkl_app_end(app);
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
    log_trace("destroying canvas");

    // Destroy the depth image.
    vkl_images_destroy(&canvas->depth_image);

    // Destroy the renderpasses.
    log_trace("canvas destroy renderpass(es)");
    for (uint32_t i = 0; i < canvas->max_renderpasses; i++)
    {
        if (canvas->renderpasses[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_renderpass_destroy(&canvas->renderpasses[i]);
    }
    INSTANCES_DESTROY(canvas->renderpasses)

    // Destroy the swapchain.
    vkl_swapchain_destroy(&canvas->swapchain);

    // Destroy the framebuffers.
    vkl_framebuffers_destroy(&canvas->framebuffers);

    // Destroy the window.
    vkl_window_destroy(canvas->window);

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
