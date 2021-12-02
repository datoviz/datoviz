/*************************************************************************************************/
/*  Testing graphics                                                                             */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

// #include "mesh.h"
// #include "../src/interact_utils.h"
// #include "proto.h"

#include "test_graphics.h"
#include "context.h"
#include "fileio.h"
#include "graphics.h"
#include "host.h"
#include "test.h"
#include "test_resources.h"
#include "test_vklite.h"
#include "testing.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

static DvzViewport viewport_default(uint32_t width, uint32_t height)
{
    DvzViewport viewport = {0};

    viewport.viewport.x = 0;
    viewport.viewport.y = 0;
    viewport.viewport.minDepth = +0;
    viewport.viewport.maxDepth = +1;

    viewport.size_framebuffer[0] = viewport.viewport.width = (float)width;
    viewport.size_framebuffer[1] = viewport.viewport.height = (float)height;
    viewport.size_screen[0] = viewport.size_framebuffer[0];
    viewport.size_screen[1] = viewport.size_framebuffer[1];

    return viewport;
}



/*************************************************************************************************/
/*  Graphics tests                                                                               */
/*************************************************************************************************/

int test_graphics_point(TstSuite* suite)
{
    ASSERT(suite != NULL);

    // Host.
    DvzHost* host = dvz_host(DVZ_BACKEND_GLFW);

    // GPU.
    DvzGpu* gpu = dvz_gpu_best(host);
    _default_queues(gpu, false);
    dvz_gpu_request_features(gpu, (VkPhysicalDeviceFeatures){.independentBlend = true});
    dvz_gpu_create(gpu, 0);

    DvzContext* ctx = dvz_context(gpu);
    ASSERT(ctx != NULL);


    DvzRenderpass renderpass = dvz_renderpass(gpu);
    VkFormat format = VK_FORMAT_B8G8R8A8_UNORM;
    VkImageLayout layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    VkClearValue clear_color = {0};
    VkClearValue clear_depth = {0};
    clear_depth.depthStencil.depth = 1.0f;
    dvz_renderpass_clear(&renderpass, clear_color);
    dvz_renderpass_clear(&renderpass, clear_depth);

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
        &renderpass, 1, //
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    dvz_renderpass_attachment_ops(
        &renderpass, 1, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE);

    // Subpass.
    dvz_renderpass_subpass_attachment(&renderpass, 0, 0);
    dvz_renderpass_subpass_attachment(&renderpass, 0, 1);

    // Color attachment
    DvzImages* images = (DvzImages*)calloc(1, sizeof(DvzImages));
    ASSERT(images != NULL);
    *images = dvz_images(gpu, VK_IMAGE_TYPE_2D, 1);
    dvz_images_format(images, renderpass.attachments[0].format);
    dvz_images_size(images, (uvec3){WIDTH, HEIGHT, 1});
    dvz_images_tiling(images, VK_IMAGE_TILING_OPTIMAL);
    dvz_images_usage(
        images, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    dvz_images_memory(images, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    dvz_images_aspect(images, VK_IMAGE_ASPECT_COLOR_BIT);
    dvz_images_layout(images, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    dvz_images_queue_access(images, DVZ_DEFAULT_QUEUE_RENDER);
    dvz_images_create(images);

    // Depth attachment.
    DvzImages* depth = (DvzImages*)calloc(1, sizeof(DvzImages));
    ASSERT(depth != NULL);
    *depth = dvz_images(gpu, VK_IMAGE_TYPE_2D, 1);
    dvz_images_format(depth, renderpass.attachments[1].format);
    dvz_images_size(depth, (uvec3){WIDTH, HEIGHT, 1});
    dvz_images_tiling(depth, VK_IMAGE_TILING_OPTIMAL);
    dvz_images_usage(depth, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    dvz_images_memory(depth, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    dvz_images_layout(depth, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    dvz_images_aspect(depth, VK_IMAGE_ASPECT_DEPTH_BIT);
    dvz_images_queue_access(depth, 0);
    dvz_images_create(depth);

    // Create renderpass.
    dvz_renderpass_create(&renderpass);

    // Create framebuffers.
    DvzFramebuffers framebuffers = dvz_framebuffers(gpu);
    dvz_framebuffers_attachment(&framebuffers, 0, images);
    dvz_framebuffers_attachment(&framebuffers, 1, depth);
    dvz_framebuffers_create(&framebuffers, &renderpass);



    DvzGraphics graphics = dvz_graphics(gpu);
    dvz_graphics_builtin(&renderpass, &graphics, DVZ_GRAPHICS_TRIANGLE, 0);

    DvzDat* dat_vertex = dvz_dat(ctx, DVZ_BUFFER_TYPE_VERTEX, 3 * sizeof(DvzVertex), 0);
    ASSERT(dat_vertex != NULL);

    DvzDat* dat_mvp = dvz_dat(ctx, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzMVP), 0);
    ASSERT(dat_mvp != NULL);
    DvzMVP mvp = {0};
    glm_mat4_identity(mvp.model);
    glm_mat4_identity(mvp.view);
    glm_mat4_identity(mvp.proj);
    dvz_dat_upload(dat_mvp, 0, sizeof(mvp), &mvp, true);

    DvzDat* dat_viewport = dvz_dat(ctx, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzViewport), 0);
    ASSERT(dat_viewport != NULL);
    DvzViewport viewport = viewport_default(WIDTH, HEIGHT);
    dvz_dat_upload(dat_viewport, 0, sizeof(viewport), &viewport, true);


    // Create the bindings.
    DvzBindings bindings = dvz_bindings(&graphics.slots, 1);
    dvz_bindings_buffer(&bindings, 0, dat_mvp->br);
    dvz_bindings_buffer(&bindings, 1, dat_viewport->br);
    dvz_bindings_update(&bindings);

    // Upload the triangle data.
    DvzVertex data[] = {
        {{-1, -1, 0}, {255, 0, 0, 255}},
        {{+1, -1, 0}, {0, 255, 0, 255}},
        {{+0, +1, 0}, {0, 0, 255, 255}},
    };
    dvz_dat_upload(dat_vertex, 0, sizeof(data), data, true);

    // Commands.
    DvzCommands cmds = dvz_commands(gpu, DVZ_DEFAULT_QUEUE_RENDER, 1);
    uint32_t idx = 0;
    uint32_t n_vertices = 3;
    dvz_cmd_begin(&cmds, idx);
    dvz_cmd_begin_renderpass(&cmds, idx, &renderpass, &framebuffers);
    dvz_cmd_viewport(&cmds, idx, (VkViewport){0, 0, (float)WIDTH, (float)HEIGHT, 0, 1});
    dvz_cmd_bind_vertex_buffer(&cmds, idx, dat_vertex->br, 0);
    dvz_cmd_bind_graphics(&cmds, idx, &graphics, &bindings, 0);
    dvz_cmd_draw(&cmds, idx, 0, n_vertices);
    dvz_cmd_end_renderpass(&cmds, idx);
    dvz_cmd_end(&cmds, idx);
    dvz_cmd_submit_sync(&cmds, DVZ_DEFAULT_QUEUE_RENDER);


    char imgpath[1024];
    snprintf(imgpath, sizeof(imgpath), "%s/screenshot.ppm", ARTIFACTS_DIR);

    log_info("saving screenshot to %s", imgpath);
    // Make a screenshot of the color attachment.
    uint8_t* rgba = (uint8_t*)screenshot(images, 1);
    dvz_write_ppm(imgpath, images->shape[0], images->shape[1], rgba);
    FREE(rgba);


    dvz_graphics_destroy(&graphics);
    dvz_bindings_destroy(&bindings);

    dvz_images_destroy(images);
    dvz_images_destroy(depth);
    dvz_framebuffers_destroy(&framebuffers);
    dvz_renderpass_destroy(&renderpass);

    dvz_dat_destroy(dat_vertex);
    dvz_dat_destroy(dat_mvp);
    dvz_dat_destroy(dat_viewport);

    dvz_context_destroy(ctx);
    dvz_gpu_destroy(gpu);
    dvz_host_destroy(host);
    FREE(images);
    FREE(depth);

    return 0;
}
