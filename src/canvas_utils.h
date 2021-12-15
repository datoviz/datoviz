/*************************************************************************************************/
/*  Canvas utils                                                                                 */
/*************************************************************************************************/

#ifndef DVZ_HEADER_CANVAS_UTILS
#define DVZ_HEADER_CANVAS_UTILS



/*************************************************************************************************/
/*  Imports                                                                                      */
/*************************************************************************************************/

#include "canvas.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static DvzRenderpass default_renderpass(
    DvzGpu* gpu, VkClearColorValue clear_color_value, VkFormat format, //
    bool overlay, bool pick)
{
    DvzRenderpass renderpass = dvz_renderpass(gpu);

    VkClearValue clear_color = {0};
    clear_color.color = clear_color_value;

    VkClearValue clear_depth = {0};
    clear_depth.depthStencil.depth = 1.0f;

    VkClearValue clear_color_pick = {0};

    dvz_renderpass_clear(&renderpass, clear_color);
    dvz_renderpass_clear(&renderpass, clear_depth);
    if (pick)
        dvz_renderpass_clear(&renderpass, clear_color_pick);

    VkImageLayout layout =
        overlay ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Color attachment.
    dvz_renderpass_attachment(
        &renderpass, 0, //
        DVZ_RENDERPASS_ATTACHMENT_COLOR, format, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    dvz_renderpass_attachment_layout(&renderpass, 0, VK_IMAGE_LAYOUT_UNDEFINED, layout);
    dvz_renderpass_attachment_ops(
        &renderpass, 0, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);

    // Depth attachment.
    dvz_renderpass_attachment(
        &renderpass, 1, //
        DVZ_RENDERPASS_ATTACHMENT_DEPTH, VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    dvz_renderpass_attachment_layout(
        &renderpass, 1, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    dvz_renderpass_attachment_ops(
        &renderpass, 1, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE);

    // Pick attachment.
    if (pick)
    {
        dvz_renderpass_attachment(
            &renderpass, 2, //
            DVZ_RENDERPASS_ATTACHMENT_PICK, DVZ_PICK_IMAGE_FORMAT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        dvz_renderpass_attachment_layout(
            &renderpass, 2, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        dvz_renderpass_attachment_ops(
            &renderpass, 2, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
    }

    // Subpass.
    dvz_renderpass_subpass_attachment(&renderpass, 0, 0);
    dvz_renderpass_subpass_attachment(&renderpass, 0, 1);
    if (pick)
        dvz_renderpass_subpass_attachment(&renderpass, 0, 2);
    // dvz_renderpass_subpass_dependency(&renderpass, 0, VK_SUBPASS_EXTERNAL, 0);
    // dvz_renderpass_subpass_dependency_stage(
    //     &renderpass, 0, //
    //     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    //     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    // dvz_renderpass_subpass_dependency_access(
    //     &renderpass, 0, //
    //     0, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

    return renderpass;
}



static DvzRenderpass default_renderpass_overlay(DvzGpu* gpu, VkFormat format, VkImageLayout layout)
{
    DvzRenderpass renderpass = dvz_renderpass(gpu);

    // Color attachment.
    dvz_renderpass_attachment(
        &renderpass, 0, //
        DVZ_RENDERPASS_ATTACHMENT_COLOR, format, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    dvz_renderpass_attachment_layout(
        &renderpass, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, layout);
    dvz_renderpass_attachment_ops(
        &renderpass, 0, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE);

    // Subpass.
    dvz_renderpass_subpass_attachment(&renderpass, 0, 0);
    // dvz_renderpass_subpass_dependency(&renderpass, 0, VK_SUBPASS_EXTERNAL, 0);
    // dvz_renderpass_subpass_dependency_stage(
    //     &renderpass, 0, //
    //     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    //     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    // dvz_renderpass_subpass_dependency_access(
    //     &renderpass, 0, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

    return renderpass;
}



static void
depth_image(DvzImages* depth_images, DvzRenderpass* renderpass, uint32_t width, uint32_t height)
{
    // Depth attachment
    dvz_images_format(depth_images, renderpass->attachments[1].format);
    dvz_images_size(depth_images, (uvec3){width, height, 1});
    dvz_images_tiling(depth_images, VK_IMAGE_TILING_OPTIMAL);
    dvz_images_usage(depth_images, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    dvz_images_memory(depth_images, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    dvz_images_layout(depth_images, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    dvz_images_aspect(depth_images, VK_IMAGE_ASPECT_DEPTH_BIT);
    dvz_images_queue_access(depth_images, 0);
    dvz_images_create(depth_images);
}



static void pick_image(DvzImages* pick, DvzRenderpass* renderpass, uint32_t width, uint32_t height)
{
    dvz_images_format(pick, renderpass->attachments[2].format);
    dvz_images_size(pick, (uvec3){width, height, 1});
    dvz_images_tiling(pick, VK_IMAGE_TILING_OPTIMAL);
    dvz_images_usage(pick, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    dvz_images_memory(pick, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    dvz_images_layout(pick, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    dvz_images_aspect(pick, VK_IMAGE_ASPECT_COLOR_BIT);
    dvz_images_queue_access(pick, 0);
    dvz_images_create(pick);
}



static void blank_commands(DvzCanvas* canvas, DvzCommands* cmds, uint32_t cmd_idx)
{
    dvz_cmd_begin(cmds, cmd_idx);
    dvz_cmd_begin_renderpass(
        cmds, cmd_idx, &canvas->render.renderpass, &canvas->render.framebuffers);
    dvz_cmd_end_renderpass(cmds, cmd_idx);
    dvz_cmd_end(cmds, cmd_idx);
}



// Submit the command buffers, + swapchain synchronization + presentation if not offscreen.
static void canvas_render(DvzCanvas* canvas)
{
    ASSERT(canvas != NULL);

    DvzSubmit* s = &canvas->render.submit;
    uint32_t f = canvas->cur_frame;
    uint32_t img_idx = canvas->render.swapchain.img_idx;
    log_trace("render canvas frame #%d", img_idx);

    // Keep track of the fence associated to the current swapchain image.
    dvz_fences_copy(
        &canvas->sync.fences_render_finished, f, //
        &canvas->sync.fences_flight, img_idx);

    // Reset the Submit instance before adding the command buffers.
    dvz_submit_reset(s);

    // Render command buffers empty? Fill them with blank color by default.
    if (canvas->cmds.obj.status != DVZ_OBJECT_STATUS_CREATED)
    {
        log_debug("empty command buffers, filling with blank color");
        for (uint32_t i = 0; i < canvas->render.swapchain.img_count; i++)
            blank_commands(canvas, &canvas->cmds, i);
    }

    ASSERT(canvas->cmds.obj.status == DVZ_OBJECT_STATUS_CREATED);
    // Add the command buffers to the submit instance.
    dvz_submit_commands(s, &canvas->cmds);

    if (s->commands_count == 0)
    {
        log_error("no recorded command buffers");
        return;
    }

    dvz_submit_wait_semaphores(
        s, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, //
        &canvas->sync.sem_img_available, f);

    // Once the render is finished, we signal another semaphore.
    dvz_submit_signal_semaphores(s, &canvas->sync.sem_render_finished, f);

    // SEND callbacks and send the Submit instance.
    // Call PRE_SEND callbacks
    // _event_presend(canvas);

    // Send the Submit instance.
    dvz_submit_send(s, img_idx, &canvas->sync.fences_render_finished, f);

    // Call POST_SEND callbacks
    // _event_postsend(canvas);

    // Once the image is rendered, we present the swapchain image.
    // The semaphore used for waiting during presentation may be changed by the canvas
    // callbacks.
    dvz_swapchain_present(
        &canvas->render.swapchain, 1,    //
        canvas->sync.present_semaphores, // waiting semaphore, alias to sem_render_finished
        CLIP(f, 0, canvas->sync.present_semaphores->count - 1));

    canvas->cur_frame = (f + 1) % canvas->sync.fences_render_finished.count;
}



#endif
