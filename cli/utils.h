#ifndef VKL_TEST_UTILS_HEADER
#define VKL_TEST_UTILS_HEADER

#include "../include/visky/context.h"
#include "../src/vklite2_utils.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

#define TEST_END return vkl_app_destroy(app);



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define TEST_WIDTH  640
#define TEST_HEIGHT 480

static const VkClearColorValue bgcolor = {{.4f, .6f, .8f, 1.0f}};
#define TEST_FORMAT       VK_FORMAT_B8G8R8A8_UNORM
#define TEST_PRESENT_MODE VK_PRESENT_MODE_FIFO_KHR
// #define TEST_PRESENT_MODE VK_PRESENT_MODE_IMMEDIATE_KHR


/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct
{
    vec3 pos;
    vec4 color;
} TestVertex;



typedef struct
{
    VklGpu* gpu;
    bool is_offscreen;

    VklWindow* window;

    VklRenderpass renderpass;
    VklFramebuffers framebuffers;
    VklSwapchain swapchain;

    VklImages* images;
    VklImages* depth;

    VklCompute* compute;
    VklBindings* bindings;
    VklGraphics* graphics;
    VklBufferRegions* br;

    void* data;
} TestCanvas;



typedef struct
{
    VklGpu* gpu;
    VklRenderpass* renderpass;
    VklFramebuffers* framebuffers;
    VklGraphics graphics;
    VklCompute* compute;
    VklBindings bindings;
    VklBuffer buffer;
    VklBufferRegions br;
    VklBufferRegions br_u;
    uint32_t n_vertices;
    float dt;
    void* data;
    void* data_u;
    void* user_data;
} TestVisual;



typedef void (*FillCallback)(TestCanvas*, VklCommands*, uint32_t);



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



static TestCanvas offscreen(VklGpu* gpu)
{
    TestCanvas canvas = {0};
    canvas.gpu = gpu;
    canvas.is_offscreen = true;

    canvas.renderpass =
        default_renderpass(gpu, bgcolor, TEST_FORMAT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    // Color attachment
    VklImages images_struct = vkl_images(canvas.renderpass.gpu, VK_IMAGE_TYPE_2D, 1);
    VklImages* images = (VklImages*)calloc(1, sizeof(VklImages));
    *images = images_struct;
    vkl_images_format(images, canvas.renderpass.attachments[0].format);
    vkl_images_size(images, TEST_WIDTH, TEST_HEIGHT, 1);
    vkl_images_tiling(images, VK_IMAGE_TILING_OPTIMAL);
    vkl_images_usage(
        images, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    vkl_images_memory(images, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkl_images_aspect(images, VK_IMAGE_ASPECT_COLOR_BIT);
    vkl_images_layout(images, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vkl_images_queue_access(images, 0);
    vkl_images_create(images);
    canvas.images = images;

    // Depth attachment.
    VklImages depth_struct = vkl_images(gpu, VK_IMAGE_TYPE_2D, 1);
    VklImages* depth = (VklImages*)calloc(1, sizeof(VklImages));
    *depth = depth_struct;
    depth_image(depth, &canvas.renderpass, TEST_WIDTH, TEST_HEIGHT);
    canvas.depth = depth;

    // Create renderpass.
    vkl_renderpass_create(&canvas.renderpass);

    // Create framebuffers.
    canvas.framebuffers = vkl_framebuffers(canvas.renderpass.gpu);
    vkl_framebuffers_attachment(&canvas.framebuffers, 0, images);
    vkl_framebuffers_attachment(&canvas.framebuffers, 1, depth);
    vkl_framebuffers_create(&canvas.framebuffers, &canvas.renderpass);

    return canvas;
}



static TestCanvas glfw_canvas(VklGpu* gpu, VklWindow* window)
{
    TestCanvas canvas = {0};
    canvas.is_offscreen = false;
    canvas.gpu = gpu;
    canvas.window = window;

    uint32_t framebuffer_width, framebuffer_height;
    vkl_window_get_size(window, &framebuffer_width, &framebuffer_height);
    ASSERT(framebuffer_width > 0);
    ASSERT(framebuffer_height > 0);

    VklRenderpass renderpass =
        default_renderpass(gpu, bgcolor, TEST_FORMAT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    canvas.renderpass = renderpass;

    canvas.swapchain = vkl_swapchain(canvas.renderpass.gpu, window, 3);
    vkl_swapchain_format(&canvas.swapchain, VK_FORMAT_B8G8R8A8_UNORM);
    vkl_swapchain_present_mode(&canvas.swapchain, TEST_PRESENT_MODE);
    vkl_swapchain_create(&canvas.swapchain);
    canvas.images = canvas.swapchain.images;

    // Depth attachment.
    VklImages depth_struct = vkl_images(gpu, VK_IMAGE_TYPE_2D, 1);
    VklImages* depth = (VklImages*)calloc(1, sizeof(VklImages));
    *depth = depth_struct;
    depth_image(depth, &canvas.renderpass, canvas.images->width, canvas.images->height);
    canvas.depth = depth;

    // Create renderpass.
    vkl_renderpass_create(&canvas.renderpass);

    // Create framebuffers.
    canvas.framebuffers = vkl_framebuffers(canvas.renderpass.gpu);
    vkl_framebuffers_attachment(&canvas.framebuffers, 0, canvas.swapchain.images);
    vkl_framebuffers_attachment(&canvas.framebuffers, 1, depth);
    vkl_framebuffers_create(&canvas.framebuffers, &canvas.renderpass);

    return canvas;
}



static uint8_t* screenshot(VklImages* images)
{
    // NOTE: the caller must free the output

    VklGpu* gpu = images->gpu;

    // Create the staging image.
    log_debug("starting creation of staging image");
    VklImages staging_struct = vkl_images(gpu, VK_IMAGE_TYPE_2D, 1);
    VklImages* staging = (VklImages*)calloc(1, sizeof(VklImages));
    *staging = staging_struct;
    vkl_images_format(staging, images->format);
    vkl_images_size(staging, images->width, images->height, images->depth);
    vkl_images_tiling(staging, VK_IMAGE_TILING_LINEAR);
    vkl_images_usage(staging, VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    vkl_images_layout(staging, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkl_images_memory(
        staging, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkl_images_create(staging);

    // Start the image transition command buffers.
    VklCommands cmds = vkl_commands(gpu, 0, 1);
    vkl_cmd_begin(&cmds, 0);

    VklBarrier barrier = vkl_barrier(gpu);
    vkl_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    vkl_barrier_images(&barrier, staging);
    vkl_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkl_barrier_images_access(&barrier, 0, VK_ACCESS_TRANSFER_WRITE_BIT);
    vkl_cmd_barrier(&cmds, 0, &barrier);

    // Copy the image to the staging image.
    vkl_cmd_copy_image(&cmds, 0, images, staging);

    vkl_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
    vkl_barrier_images_access(&barrier, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT);
    vkl_cmd_barrier(&cmds, 0, &barrier);

    // End the cmds and submit them.
    vkl_cmd_end(&cmds, 0);
    vkl_cmd_submit_sync(&cmds, 0);

    // Now, copy the staging image into CPU memory.
    uint8_t* rgba = (uint8_t*)calloc(images->width * images->height, 3);
    vkl_images_download(staging, 0, true, rgba);

    vkl_images_destroy(staging);

    return rgba;
}



static void save_screenshot(VklFramebuffers* framebuffers, const char* path)
{
    log_debug("saving screenshot to %s", path);
    // Make a screenshot of the color attachment.
    VklImages* images = framebuffers->attachments[0];
    uint8_t* rgba = screenshot(images);
    write_ppm(path, images->width, images->height, rgba);
    FREE(rgba);
}



static void show_canvas(TestCanvas canvas, FillCallback fill_commands, uint32_t n_frames)
{
    VklGpu* gpu = canvas.gpu;
    VklWindow* window = canvas.window;
    VklRenderpass* renderpass = &canvas.renderpass;
    VklFramebuffers* framebuffers = &canvas.framebuffers;
    VklSwapchain* swapchain = &canvas.swapchain;

    ASSERT(swapchain != NULL);
    ASSERT(swapchain->img_count > 0);

    VklCommands cmds = vkl_commands(gpu, 0, swapchain->img_count);
    for (uint32_t i = 0; i < cmds.count; i++)
        fill_commands(&canvas, &cmds, i);

    // Sync objects.
    VklSemaphores sem_img_available = vkl_semaphores(gpu, VKY_MAX_FRAMES_IN_FLIGHT);
    VklSemaphores sem_render_finished = vkl_semaphores(gpu, VKY_MAX_FRAMES_IN_FLIGHT);
    VklFences fences = vkl_fences(gpu, VKY_MAX_FRAMES_IN_FLIGHT);
    VklFences bak_fences = {0};
    bak_fences.gpu = gpu;
    bak_fences.count = swapchain->img_count;
    uint32_t cur_frame = 0;
    VklBackend backend = VKL_BACKEND_GLFW;

    for (uint32_t frame = 0; frame < n_frames; frame++)
    {
        log_debug("iteration %d", frame);

        glfwPollEvents();

        if (backend_window_should_close(backend, window->backend_window) ||
            window->obj.status == VKL_OBJECT_STATUS_NEED_DESTROY)
            break;

        // Wait for fence.
        vkl_fences_wait(&fences, cur_frame);

        // We acquire the next swapchain image.
        vkl_swapchain_acquire(swapchain, &sem_img_available, cur_frame, NULL, 0);
        if (swapchain->obj.status == VKL_OBJECT_STATUS_INVALID)
        {
            vkl_gpu_wait(gpu);
            break;
        }
        // Handle resizing.
        else if (swapchain->obj.status == VKL_OBJECT_STATUS_NEED_RECREATE)
        {
            log_trace("recreating the swapchain");

            // Wait until the device is ready and the window fully resized.
            // Framebuffer new size.
            uint32_t width, height;
            backend_window_get_size(
                backend, window->backend_window, //
                &window->width, &window->height, //
                &width, &height);
            vkl_gpu_wait(gpu);

            // Destroy swapchain resources.
            vkl_framebuffers_destroy(framebuffers);
            vkl_images_destroy(canvas.depth);
            vkl_images_destroy(canvas.images);
            vkl_swapchain_destroy(swapchain);

            // Recreate the swapchain. This will automatically set the swapchain->images new
            // size.
            vkl_swapchain_create(swapchain);
            // Find the new framebuffer size as determined by the swapchain recreation.
            width = swapchain->images->width;
            height = swapchain->images->height;

            // The instance should be the same.
            ASSERT(swapchain->images == canvas.images);

            // Need to recreate the depth image with the new size.
            vkl_images_size(canvas.depth, width, height, 1);
            vkl_images_create(canvas.depth);

            // Recreate the framebuffers with the new size.
            ASSERT(framebuffers->attachments[0]->width == width);
            ASSERT(framebuffers->attachments[0]->height == height);
            vkl_framebuffers_create(framebuffers, renderpass);

            // Need to refill the command buffers.
            for (uint32_t i = 0; i < cmds.count; i++)
            {
                vkl_cmd_reset(&cmds, i);
                fill_commands(&canvas, &cmds, i);
            }
        }
        else
        {
            vkl_fences_copy(&fences, cur_frame, &bak_fences, swapchain->img_idx);

            // Then, we submit the cmds on that image
            VklSubmit submit = vkl_submit(gpu);
            vkl_submit_commands(&submit, &cmds);
            vkl_submit_wait_semaphores(
                &submit, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, &sem_img_available,
                cur_frame);
            // Once the render is finished, we signal another semaphore.
            vkl_submit_signal_semaphores(&submit, &sem_render_finished, cur_frame);
            vkl_submit_send(&submit, swapchain->img_idx, &fences, cur_frame);

            // Once the image is rendered, we present the swapchain image.
            vkl_swapchain_present(swapchain, 1, &sem_render_finished, cur_frame);

            cur_frame = (cur_frame + 1) % VKY_MAX_FRAMES_IN_FLIGHT;
        }

        // IMPORTANT: we need to wait for the present queue to be idle, otherwise the GPU hangs
        // when waiting for fences (not sure why). The problem only arises when using different
        // queues for command buffer submission and swapchain present.
        vkl_queue_wait(gpu, 1);
    }
    log_trace("end of main loop");
    vkl_gpu_wait(gpu);

    vkl_semaphores_destroy(&sem_img_available);
    vkl_semaphores_destroy(&sem_render_finished);
    vkl_fences_destroy(&fences);
}



static void destroy_canvas(TestCanvas* canvas)
{
    log_trace("destroy canvas");
    if (canvas->is_offscreen)
    {
        vkl_images_destroy(canvas->images);
    }
    vkl_images_destroy(canvas->depth);

    vkl_renderpass_destroy(&canvas->renderpass);
    vkl_swapchain_destroy(&canvas->swapchain);
    vkl_framebuffers_destroy(&canvas->framebuffers);
    vkl_window_destroy(canvas->window);
}



static void _triangle_graphics(TestVisual* visual, const char* suffix)
{
    VklGpu* gpu = visual->gpu;
    visual->graphics = vkl_graphics(gpu);
    ASSERT(visual->renderpass != NULL);
    VklGraphics* graphics = &visual->graphics;

    vkl_graphics_renderpass(graphics, visual->renderpass, 0);
    vkl_graphics_topology(graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    vkl_graphics_polygon_mode(graphics, VK_POLYGON_MODE_FILL);

    char path[1024];
    snprintf(path, sizeof(path), "%s/spirv/test_triangle%s.vert.spv", DATA_DIR, suffix);
    vkl_graphics_shader(graphics, VK_SHADER_STAGE_VERTEX_BIT, path);
    snprintf(path, sizeof(path), "%s/spirv/test_triangle%s.frag.spv", DATA_DIR, suffix);
    vkl_graphics_shader(graphics, VK_SHADER_STAGE_FRAGMENT_BIT, path);
    vkl_graphics_vertex_binding(graphics, 0, sizeof(TestVertex));
    vkl_graphics_vertex_attr(
        graphics, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TestVertex, pos));
    vkl_graphics_vertex_attr(
        graphics, 0, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(TestVertex, color));
}



static void _triangle_buffer(TestVisual* visual)
{
    VklGpu* gpu = visual->gpu;

    // Create the buffer.
    visual->buffer = vkl_buffer(gpu);
    VkDeviceSize size = 3 * sizeof(TestVertex);
    vkl_buffer_size(&visual->buffer, size);
    vkl_buffer_usage(
        &visual->buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    vkl_buffer_memory(
        &visual->buffer,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkl_buffer_create(&visual->buffer);

    // Upload the triangle data.
    TestVertex data[3] = {
        {{-1, +1, 0}, {1, 0, 0, 1}},
        {{+1, +1, 0}, {0, 1, 0, 1}},
        {{+0, -1, 0}, {0, 0, 1, 1}},
    };
    vkl_buffer_upload(&visual->buffer, 0, size, data);

    visual->br.buffer = &visual->buffer;
    visual->br.size = size;
    visual->br.count = 1;
}



static void test_triangle(TestVisual* visual, const char* suffix)
{
    _triangle_graphics(visual, suffix);

    // Create the bindings.
    visual->bindings = vkl_bindings(&visual->graphics.slots);
    vkl_bindings_create(&visual->bindings, 1);
    vkl_bindings_update(&visual->bindings);

    // Create the graphics pipeline.
    vkl_graphics_create(&visual->graphics);

    _triangle_buffer(visual);
}



static void destroy_visual(TestVisual* visual)
{
    vkl_graphics_destroy(&visual->graphics);
    vkl_bindings_destroy(&visual->bindings);
    vkl_buffer_destroy(&visual->buffer);
}



/*************************************************************************************************/
/*  Commands filling                                                                             */
/*************************************************************************************************/

static void empty_commands(TestCanvas* canvas, VklCommands* cmds, uint32_t idx)
{
    vkl_cmd_begin(cmds, idx);
    vkl_cmd_begin_renderpass(cmds, idx, &canvas->renderpass, &canvas->framebuffers);
    vkl_cmd_end_renderpass(cmds, idx);
    vkl_cmd_end(cmds, idx);
}



static void triangle_commands(TestCanvas* canvas, VklCommands* cmds, uint32_t idx)
{
    ASSERT(canvas->br != NULL);
    ASSERT(canvas->graphics != NULL);
    ASSERT(canvas->bindings != NULL);

    // Commands.
    vkl_cmd_begin(cmds, idx);
    vkl_cmd_begin_renderpass(cmds, idx, &canvas->renderpass, &canvas->framebuffers);
    vkl_cmd_viewport(
        cmds, idx,
        (VkViewport){
            0, 0, canvas->framebuffers.attachments[0]->width,
            canvas->framebuffers.attachments[0]->height, 0, 1});
    vkl_cmd_bind_vertex_buffer(cmds, idx, canvas->br, 0);
    vkl_cmd_bind_graphics(cmds, idx, canvas->graphics, canvas->bindings, 0);
    vkl_cmd_draw(cmds, idx, 0, 3);
    vkl_cmd_end_renderpass(cmds, idx);
    vkl_cmd_end(cmds, idx);
}


#endif
