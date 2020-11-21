#include "../include/visky/canvas.h"
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
} VklVertex;



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

    VklGraphics* graphics;
    VklBufferRegions buffer_regions;
    VklBufferRegions uniform_buffer_regions;
    VklBindings* bindings;

    void* data;
} TestCanvas;



typedef struct
{
    VklGraphics* graphics;
    VklSlots slots;
    VklBindings bindings;
    VklBuffer buffer;
    // VklCommands cmds;
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
    VklImages* images = calloc(1, sizeof(VklImages));
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
    VklImages* depth = calloc(1, sizeof(VklImages));
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
    VklImages* depth = calloc(1, sizeof(VklImages));
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
    VklImages* staging = calloc(1, sizeof(VklImages));
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
    uint8_t* rgba = calloc(images->width * images->height, 3);
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
    vkl_fences_create(&fences);
    VklFences bak_fences = vkl_fences(gpu, swapchain->img_count);
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



static void _triangle_graphics(TestCanvas* canvas, TestVisual* visual, const char* suffix)
{
    VklGpu* gpu = canvas->gpu;
    VklGraphics* graphics = vkl_graphics(gpu);
    visual->graphics = graphics;
    canvas->graphics = graphics;

    vkl_graphics_renderpass(graphics, &canvas->renderpass, 0);
    vkl_graphics_topology(graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    vkl_graphics_polygon_mode(graphics, VK_POLYGON_MODE_FILL);

    char path[1024];
    snprintf(path, sizeof(path), "%s/spirv/test_triangle%s.vert.spv", DATA_DIR, suffix);
    vkl_graphics_shader(graphics, VK_SHADER_STAGE_VERTEX_BIT, path);
    snprintf(path, sizeof(path), "%s/spirv/test_triangle%s.frag.spv", DATA_DIR, suffix);
    vkl_graphics_shader(graphics, VK_SHADER_STAGE_FRAGMENT_BIT, path);
    vkl_graphics_vertex_binding(graphics, 0, sizeof(VklVertex));
    vkl_graphics_vertex_attr(graphics, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VklVertex, pos));
    vkl_graphics_vertex_attr(
        graphics, 0, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VklVertex, color));
}



static void _triangle_buffer(TestCanvas* canvas, TestVisual* visual)
{
    VklGpu* gpu = canvas->gpu;

    // Create the buffer.
    visual->buffer = vkl_buffer(gpu);
    VkDeviceSize size = 3 * sizeof(VklVertex);
    vkl_buffer_size(&visual->buffer, size);
    vkl_buffer_usage(&visual->buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    vkl_buffer_memory(
        &visual->buffer,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkl_buffer_create(&visual->buffer);

    // Upload the triangle data.
    VklVertex data[3] = {
        {{-1, +1, 0}, {1, 0, 0, 1}},
        {{+1, +1, 0}, {0, 1, 0, 1}},
        {{+0, -1, 0}, {0, 0, 1, 1}},
    };
    vkl_buffer_upload(&visual->buffer, 0, size, data);

    canvas->buffer_regions.buffer = &visual->buffer;
    canvas->buffer_regions.size = size;
    canvas->buffer_regions.count = 1;
}



static void test_triangle(TestCanvas* canvas, TestVisual* visual, const char* suffix)
{
    VklGpu* gpu = canvas->gpu;

    _triangle_graphics(canvas, visual, suffix);

    // Create the slots.
    visual->slots = vkl_slots(gpu);
    vkl_slots_create(&visual->slots);
    vkl_graphics_slots(visual->graphics, &visual->slots);

    // Create the bindings.
    visual->bindings = vkl_bindings(&visual->slots);
    vkl_bindings_create(&visual->bindings, 1);
    vkl_bindings_update(&visual->bindings);
    canvas->bindings = &visual->bindings;

    // Create the graphics pipeline.
    vkl_graphics_create(visual->graphics);

    _triangle_buffer(canvas, visual);
}



static void destroy_visual(TestVisual* visual)
{
    vkl_bindings_destroy(&visual->bindings);
    vkl_slots_destroy(&visual->slots);
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
    // Commands.
    vkl_cmd_begin(cmds, idx);
    vkl_cmd_begin_renderpass(cmds, idx, &canvas->renderpass, &canvas->framebuffers);
    vkl_cmd_viewport(
        cmds, idx,
        (VkViewport){
            0, 0, canvas->framebuffers.attachments[0]->width,
            canvas->framebuffers.attachments[0]->height, 0, 1});
    vkl_cmd_bind_vertex_buffer(cmds, idx, &canvas->buffer_regions, 0);
    vkl_cmd_bind_graphics(cmds, idx, canvas->graphics, canvas->bindings, 0);
    vkl_cmd_draw(cmds, idx, 0, 3);
    vkl_cmd_end_renderpass(cmds, idx);
    vkl_cmd_end(cmds, idx);
}



/*************************************************************************************************/
/*  vklite2                                                                                      */
/*************************************************************************************************/

static int vklite2_app(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    AT(app->obj.status == VKL_OBJECT_STATUS_CREATED);
    AT(app->gpu_count >= 1);
    AT(app->gpus[0].name != NULL);
    AT(app->gpus[0].obj.status == VKL_OBJECT_STATUS_INIT);

    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_TRANSFER, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_GRAPHICS | VKL_QUEUE_COMPUTE, 1);
    vkl_gpu_queue(gpu, VKL_QUEUE_COMPUTE, 2);
    vkl_gpu_create(gpu, 0);

    TEST_END
}



static int vklite2_surface(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_ALL, 0);

    // Create a GLFW window and surface.
    VkSurfaceKHR surface = 0;
    GLFWwindow* window =
        (GLFWwindow*)backend_window(app->instance, VKL_BACKEND_GLFW, 100, 100, NULL, &surface);
    vkl_gpu_create(gpu, surface);

    backend_window_destroy(app->instance, VKL_BACKEND_GLFW, window, surface);

    TEST_END
}



static int vklite2_window(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklWindow* window = vkl_window(app, 100, 100);
    AT(window != NULL);

    TEST_END
}



static int vklite2_swapchain(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklWindow* window = vkl_window(app, 100, 100);
    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_RENDER, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_PRESENT, 1);
    vkl_gpu_create(gpu, window->surface);
    VklSwapchain swapchain = vkl_swapchain(gpu, window, 3);
    vkl_swapchain_format(&swapchain, VK_FORMAT_B8G8R8A8_UNORM);
    vkl_swapchain_present_mode(&swapchain, TEST_PRESENT_MODE);
    vkl_swapchain_create(&swapchain);
    vkl_swapchain_destroy(&swapchain);
    vkl_window_destroy(window);

    TEST_END
}



static int vklite2_commands(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_RENDER, 0);
    vkl_gpu_create(gpu, 0);
    VklCommands cmds = vkl_commands(gpu, 0, 3);
    vkl_cmd_begin(&cmds, 0);
    vkl_cmd_end(&cmds, 0);
    vkl_cmd_reset(&cmds, 0);
    vkl_cmd_free(&cmds);

    TEST_END
}



static int vklite2_buffer(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_RENDER, 0);
    vkl_gpu_create(gpu, 0);

    VklBuffer buffer = vkl_buffer(gpu);
    const VkDeviceSize size = 256;
    vkl_buffer_size(&buffer, size);
    vkl_buffer_usage(&buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    vkl_buffer_memory(
        &buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkl_buffer_queue_access(&buffer, 0);
    vkl_buffer_create(&buffer);

    // Send some data to the GPU.
    uint8_t* data = calloc(size, 1);
    for (uint32_t i = 0; i < size; i++)
        data[i] = i;
    vkl_buffer_upload(&buffer, 0, size, data);

    // Recover the data.
    void* data2 = calloc(size, 1);
    vkl_buffer_download(&buffer, 0, size, data2);

    // Check that the data downloaded from the GPU is the same.
    AT(memcmp(data2, data, size) == 0);

    FREE(data);
    FREE(data2);

    vkl_buffer_destroy(&buffer);

    TEST_END
}



static int vklite2_compute(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_COMPUTE, 0);
    vkl_gpu_create(gpu, 0);

    // Create the compute pipeline.
    char path[1024];
    snprintf(path, sizeof(path), "%s/spirv/test_square.comp.spv", DATA_DIR);
    VklCompute compute = vkl_compute(gpu, path);

    // Create the buffers
    VklBuffer buffer = vkl_buffer(gpu);
    const uint32_t n = 20;
    const VkDeviceSize size = n * sizeof(float);
    vkl_buffer_size(&buffer, size);
    vkl_buffer_usage(
        &buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    vkl_buffer_memory(
        &buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkl_buffer_queue_access(&buffer, 0);
    vkl_buffer_create(&buffer);

    // Send some data to the GPU.
    float* data = calloc(n, sizeof(float));
    for (uint32_t i = 0; i < n; i++)
        data[i] = (float)i;
    vkl_buffer_upload(&buffer, 0, size, data);

    // Create the slots.
    VklSlots slots = vkl_slots(gpu);
    vkl_slots_binding(&slots, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    vkl_slots_create(&slots);
    vkl_compute_slots(&compute, &slots);

    // Create the bindings.
    VklBindings bindings = vkl_bindings(&slots);
    VklBufferRegions br = {.buffer = &buffer, .size = size, .count = 1};
    vkl_bindings_buffer(&bindings, 0, &br);
    vkl_bindings_create(&bindings, 1);
    vkl_bindings_update(&bindings);

    // Link the bindings to the compute pipeline and create it.
    vkl_compute_bindings(&compute, &bindings);
    vkl_compute_create(&compute);

    // Command buffers.
    VklCommands cmds = vkl_commands(gpu, 0, 1);
    vkl_cmd_begin(&cmds, 0);
    vkl_cmd_compute(&cmds, 0, &compute, (uvec3){20, 1, 1});
    vkl_cmd_end(&cmds, 0);
    vkl_cmd_submit_sync(&cmds, 0);

    // Get back the data.
    float* data2 = calloc(n, sizeof(float));
    vkl_buffer_download(&buffer, 0, size, data2);
    for (uint32_t i = 0; i < n; i++)
        AT(data2[i] == 2 * data[i]);

    vkl_slots_destroy(&slots);
    vkl_bindings_destroy(&bindings);
    vkl_compute_destroy(&compute);
    vkl_buffer_destroy(&buffer);

    TEST_END
}



static int vklite2_push(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_COMPUTE, 0);
    vkl_gpu_create(gpu, 0);

    // Create the compute pipeline.
    char path[1024];
    snprintf(path, sizeof(path), "%s/spirv/test_pow.comp.spv", DATA_DIR);
    VklCompute compute = vkl_compute(gpu, path);

    // Create the buffers
    VklBuffer buffer = vkl_buffer(gpu);
    const uint32_t n = 20;
    const VkDeviceSize size = n * sizeof(float);
    vkl_buffer_size(&buffer, size);
    vkl_buffer_usage(
        &buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    vkl_buffer_memory(
        &buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkl_buffer_queue_access(&buffer, 0);
    vkl_buffer_create(&buffer);

    // Send some data to the GPU.
    float* data = calloc(n, sizeof(float));
    for (uint32_t i = 0; i < n; i++)
        data[i] = (float)i;
    vkl_buffer_upload(&buffer, 0, size, data);

    // Create the slots.
    VklSlots slots = vkl_slots(gpu);
    vkl_slots_binding(&slots, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    vkl_slots_push_constant(&slots, 0, sizeof(float), VK_SHADER_STAGE_COMPUTE_BIT);
    vkl_slots_create(&slots);
    vkl_compute_slots(&compute, &slots);

    // Create the bindings.
    VklBindings bindings = vkl_bindings(&slots);
    VklBufferRegions br = {.buffer = &buffer, .size = size, .count = 1};
    vkl_bindings_buffer(&bindings, 0, &br);
    vkl_bindings_create(&bindings, 1);
    vkl_bindings_update(&bindings);

    // Link the bindings to the compute pipeline and create it.
    vkl_compute_bindings(&compute, &bindings);
    vkl_compute_create(&compute);

    // Command buffers.
    VklCommands cmds = vkl_commands(gpu, 0, 1);
    vkl_cmd_begin(&cmds, 0);
    float power = 2.0f;
    vkl_cmd_push_constants(
        &cmds, 0, &slots, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float), &power);
    vkl_cmd_compute(&cmds, 0, &compute, (uvec3){20, 1, 1});
    vkl_cmd_end(&cmds, 0);
    vkl_cmd_submit_sync(&cmds, 0);

    // Get back the data.
    float* data2 = calloc(n, sizeof(float));
    vkl_buffer_download(&buffer, 0, size, data2);
    for (uint32_t i = 0; i < n; i++)
        AT(fabs(data2[i] - pow(data[i], power)) < .01);

    vkl_slots_destroy(&slots);
    vkl_bindings_destroy(&bindings);
    vkl_compute_destroy(&compute);
    vkl_buffer_destroy(&buffer);

    TEST_END
}



static int vklite2_images(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_RENDER, 0);
    vkl_gpu_create(gpu, 0);

    VklImages images = vkl_images(gpu, VK_IMAGE_TYPE_2D, 1);
    vkl_images_format(&images, VK_FORMAT_R8G8B8A8_UINT);
    vkl_images_size(&images, 16, 16, 1);
    vkl_images_tiling(&images, VK_IMAGE_TILING_OPTIMAL);
    vkl_images_usage(&images, VK_IMAGE_USAGE_STORAGE_BIT);
    vkl_images_memory(&images, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkl_images_queue_access(&images, 0);
    vkl_images_create(&images);

    vkl_images_destroy(&images);

    TEST_END
}



static int vklite2_sampler(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_RENDER, 0);
    vkl_gpu_create(gpu, 0);

    VklSampler sampler = vkl_sampler(gpu);
    vkl_sampler_min_filter(&sampler, VK_FILTER_LINEAR);
    vkl_sampler_mag_filter(&sampler, VK_FILTER_LINEAR);
    vkl_sampler_address_mode(&sampler, VKL_TEXTURE_AXIS_U, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    vkl_sampler_create(&sampler);

    vkl_sampler_destroy(&sampler);

    TEST_END
}



static int vklite2_barrier(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_RENDER, 0);
    vkl_gpu_create(gpu, 0);

    // Image.
    const uint32_t img_size = 16;
    VklImages images = vkl_images(gpu, VK_IMAGE_TYPE_2D, 1);
    vkl_images_format(&images, VK_FORMAT_R8G8B8A8_UINT);
    vkl_images_size(&images, img_size, img_size, 1);
    vkl_images_tiling(&images, VK_IMAGE_TILING_OPTIMAL);
    vkl_images_usage(&images, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    vkl_images_memory(&images, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkl_images_queue_access(&images, 0);
    vkl_images_create(&images);

    // Staging buffer.
    VklBuffer buffer = vkl_buffer(gpu);
    const VkDeviceSize size = img_size * img_size * 4;
    vkl_buffer_size(&buffer, size);
    vkl_buffer_usage(&buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    vkl_buffer_memory(
        &buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkl_buffer_queue_access(&buffer, 0);
    vkl_buffer_create(&buffer);

    // Send some data to the staging buffer.
    uint8_t* data = calloc(size, 1);
    for (uint32_t i = 0; i < size; i++)
        data[i] = i % 256;
    vkl_buffer_upload(&buffer, 0, size, data);

    // Image transition.
    VklBarrier barrier = vkl_barrier(gpu);
    vkl_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    vkl_barrier_images(&barrier, &images);
    vkl_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Transfer the data from the staging buffer to the image.
    VklCommands cmds = vkl_commands(gpu, 0, 1);
    vkl_cmd_begin(&cmds, 0);
    vkl_cmd_barrier(&cmds, 0, &barrier);
    vkl_cmd_copy_buffer_to_image(&cmds, 0, &buffer, &images);
    vkl_cmd_end(&cmds, 0);
    vkl_cmd_submit_sync(&cmds, 0);

    vkl_buffer_destroy(&buffer);
    vkl_images_destroy(&images);

    TEST_END
}



static int vklite2_submit(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_COMPUTE, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_COMPUTE, 1);
    vkl_gpu_create(gpu, 0);

    // Create the compute pipeline.
    char path[1024];
    snprintf(path, sizeof(path), "%s/spirv/test_square.comp.spv", DATA_DIR);
    VklCompute compute1 = vkl_compute(gpu, path);

    snprintf(path, sizeof(path), "%s/spirv/test_sum.comp.spv", DATA_DIR);
    VklCompute compute2 = vkl_compute(gpu, path);

    // Create the buffer
    VklBuffer buffer = vkl_buffer(gpu);
    const uint32_t n = 20;
    const VkDeviceSize size = n * sizeof(float);
    vkl_buffer_size(&buffer, size);
    vkl_buffer_usage(
        &buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    vkl_buffer_memory(
        &buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkl_buffer_queue_access(&buffer, 0);
    vkl_buffer_queue_access(&buffer, 1);
    vkl_buffer_create(&buffer);

    // Send some data to the GPU.
    float* data = calloc(n, sizeof(float));
    for (uint32_t i = 0; i < n; i++)
        data[i] = (float)i;
    vkl_buffer_upload(&buffer, 0, size, data);

    // Create the slots.
    VklSlots slots = vkl_slots(gpu);
    vkl_slots_binding(&slots, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    vkl_slots_create(&slots);
    vkl_compute_slots(&compute1, &slots);
    vkl_compute_slots(&compute2, &slots);

    // Create the bindings.
    VklBindings bindings1 = vkl_bindings(&slots);
    vkl_bindings_create(&bindings1, 1);
    VklBufferRegions br1 = {.buffer = &buffer, .size = size, .count = 1};
    vkl_bindings_buffer(&bindings1, 0, &br1);

    VklBindings bindings2 = vkl_bindings(&slots);
    vkl_bindings_create(&bindings2, 1);
    VklBufferRegions br2 = {.buffer = &buffer, .size = size, .count = 1};
    vkl_bindings_buffer(&bindings2, 0, &br2);

    vkl_bindings_update(&bindings1);
    vkl_bindings_update(&bindings2);

    // Link the bindings1 to the compute1 pipeline and create it.
    vkl_compute_bindings(&compute1, &bindings1);
    vkl_compute_create(&compute1);

    // Link the bindings1 to the compute2 pipeline and create it.
    vkl_compute_bindings(&compute2, &bindings2);
    vkl_compute_create(&compute2);

    // Command buffers.
    VklCommands cmds1 = vkl_commands(gpu, 0, 1);
    vkl_cmd_begin(&cmds1, 0);
    vkl_cmd_compute(&cmds1, 0, &compute1, (uvec3){20, 1, 1});
    vkl_cmd_end(&cmds1, 0);

    VklCommands cmds2 = vkl_commands(gpu, 0, 1);
    vkl_cmd_begin(&cmds2, 0);
    vkl_cmd_compute(&cmds2, 0, &compute2, (uvec3){20, 1, 1});
    vkl_cmd_end(&cmds2, 0);

    // Semaphores
    VklSemaphores semaphores = vkl_semaphores(gpu, 1);

    // Submit.
    VklSubmit submit1 = vkl_submit(gpu);
    vkl_submit_commands(&submit1, &cmds1);
    vkl_submit_signal_semaphores(&submit1, &semaphores, 0);
    vkl_submit_send(&submit1, 0, NULL, 0);

    VklSubmit submit2 = vkl_submit(gpu);
    vkl_submit_commands(&submit2, &cmds2);
    vkl_submit_wait_semaphores(&submit2, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, &semaphores, 0);
    vkl_submit_send(&submit2, 0, NULL, 0);

    vkl_gpu_wait(gpu);

    // Get back the data.
    float* data2 = calloc(n, sizeof(float));
    vkl_buffer_download(&buffer, 0, size, data2);
    for (uint32_t i = 0; i < n; i++)
        AT(data2[i] == 2 * i + 1);


    vkl_semaphores_destroy(&semaphores);
    vkl_slots_destroy(&slots);
    vkl_bindings_destroy(&bindings1);
    vkl_bindings_destroy(&bindings2);
    vkl_buffer_destroy(&buffer);
    vkl_compute_destroy(&compute1);
    vkl_compute_destroy(&compute2);

    TEST_END
}



static int vklite2_blank(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_RENDER, 0);
    vkl_gpu_create(gpu, 0);

    TestCanvas canvas = offscreen(gpu);
    VklFramebuffers* framebuffers = &canvas.framebuffers;

    VklCommands cmds = vkl_commands(gpu, 0, 1);
    empty_commands(&canvas, &cmds, 0);
    vkl_cmd_submit_sync(&cmds, 0);

    uint8_t* rgba = screenshot(framebuffers->attachments[0]);

    for (uint32_t i = 0; i < TEST_WIDTH * TEST_HEIGHT * 3; i++)
        AT(rgba[i] >= 100);

    FREE(rgba);

    destroy_canvas(&canvas);

    TEST_END
}



static int vklite2_graphics(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_RENDER, 0);
    vkl_gpu_create(gpu, 0);

    TestCanvas canvas = offscreen(gpu);
    VklFramebuffers* framebuffers = &canvas.framebuffers;

    TestVisual visual = {0};
    test_triangle(&canvas, &visual, "");
    VklCommands cmds = vkl_commands(gpu, 0, 1);
    triangle_commands(&canvas, &cmds, 0);
    vkl_cmd_submit_sync(&cmds, 0);

    save_screenshot(framebuffers, "screenshot.ppm");

    destroy_visual(&visual);
    destroy_canvas(&canvas);
    TEST_END
}



static int vklite2_basic_canvas(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);

    VklWindow* window = vkl_window(app, TEST_WIDTH, TEST_HEIGHT);

    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_RENDER, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_PRESENT, 1);
    vkl_gpu_create(gpu, window->surface);

    TestCanvas canvas = glfw_canvas(gpu, window);

    show_canvas(canvas, empty_commands, 10);

    destroy_canvas(&canvas);

    TEST_END
}



static int vklite2_basic_canvas_triangle(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);

    VklWindow* window = vkl_window(app, TEST_WIDTH, TEST_HEIGHT);

    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_RENDER, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_PRESENT, 1);
    vkl_gpu_create(gpu, window->surface);

    TestCanvas canvas = glfw_canvas(gpu, window);

    TestVisual visual = {0};
    test_triangle(&canvas, &visual, "");
    VklCommands cmds = vkl_commands(gpu, 0, 1);
    triangle_commands(&canvas, &cmds, 0);
    vkl_cmd_submit_sync(&cmds, 0);

    show_canvas(canvas, triangle_commands, 10);

    destroy_visual(&visual);
    destroy_canvas(&canvas);

    TEST_END
}



/*************************************************************************************************/
/*  FIFO queue                                                                                   */
/*************************************************************************************************/

static void* _fifo_thread_1(void* arg)
{
    VklFifo* fifo = arg;
    uint8_t* data = vkl_fifo_dequeue(fifo, true);
    ASSERT(*data == 12);
    // Signal to the caller thread that the dequeue was successfull.
    fifo->user_data = data;
    return NULL;
}



static void* _fifo_thread_2(void* arg)
{
    VklFifo* fifo = arg;
    uint8_t* numbers = calloc(5, sizeof(uint8_t));
    fifo->user_data = numbers;
    for (uint32_t i = 0; i < 5; i++)
    {
        numbers[i] = i;
        vkl_fifo_enqueue(fifo, &numbers[i]);
        vkl_sleep(10);
    }
    vkl_fifo_enqueue(fifo, NULL);
    return NULL;
}



static int vklite2_fifo(VkyTestContext* context)
{
    VklFifo fifo = vkl_fifo(8);
    uint8_t item = 12;

    // Enqueue + dequeue in the same thread.
    vkl_fifo_enqueue(&fifo, &item);
    ASSERT(fifo.head == 1);
    ASSERT(fifo.tail == 0);
    uint8_t* data = vkl_fifo_dequeue(&fifo, true);
    ASSERT(*data = item);

    // Enqueue in the main thread, dequeue in a background thread.
    pthread_t thread = {0};
    ASSERT(fifo.user_data == NULL);
    pthread_create(&thread, NULL, _fifo_thread_1, &fifo);
    vkl_fifo_enqueue(&fifo, &item);
    pthread_join(thread, NULL);
    ASSERT(fifo.user_data != NULL);
    ASSERT(fifo.user_data == &item);

    // Multiple enqueues in the background thread, dequeue in the main thread.
    pthread_create(&thread, NULL, _fifo_thread_2, &fifo);
    uint8_t* dequeued = NULL;
    uint32_t i = 0;
    do
    {
        dequeued = vkl_fifo_dequeue(&fifo, true);
        if (dequeued == NULL)
            break;
        AT(*dequeued == i);
        i++;
    } while (dequeued != NULL);
    pthread_join(thread, NULL);
    FREE(fifo.user_data);

    return 0;
}



/*************************************************************************************************/
/*  Context                                                                                   */
/*************************************************************************************************/

static int vklite2_context_buffer(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklContext* ctx = vkl_context(gpu, NULL);

    // Create a buffer.
    VklBuffer buffer_struct = vkl_buffer(gpu);
    ctx->buffers[VKL_DEFAULT_BUFFER_COUNT] = buffer_struct;
    VklBuffer* buffer = &ctx->buffers[VKL_DEFAULT_BUFFER_COUNT];
    vkl_buffer_queue_access(buffer, 0);
    vkl_buffer_size(buffer, 256);
    vkl_buffer_usage(buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    vkl_buffer_memory(
        buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkl_buffer_create(buffer);

    // Send some data to the GPU.
    uint8_t* data = calloc(256, 1);
    for (uint32_t i = 0; i < 256; i++)
        data[i] = i;
    vkl_buffer_upload(buffer, 0, 256, data);

    // Allocate buffer regions.
    VklBufferRegions br = vkl_alloc_buffers(ctx, VKL_DEFAULT_BUFFER_COUNT, 3, 64);
    AT(br.count == 3);
    AT(br.offsets[0] == 0);
    AT(br.offsets[1] == 64);
    AT(br.offsets[2] == 128);
    AT(br.size == 64);
    AT(buffer->size == 256);

    // This allocation will trigger a buffer resize.
    br = vkl_alloc_buffers(ctx, VKL_DEFAULT_BUFFER_COUNT, 2, 64);
    AT(br.count == 2);
    AT(br.offsets[0] == 192);
    AT(br.offsets[1] == 256);
    AT(br.size == 64);
    AT(buffer->size == 512);

    // Recover the data.
    void* data2 = calloc(256, 1);
    vkl_buffer_download(buffer, 0, 256, data2);

    // Check that the data downloaded from the GPU is the same.
    // This also checks that the data on the initial buffer was successfully copied to the new
    // buffer during reallocation
    AT(memcmp(data2, data, 256) == 0);

    vkl_buffer_destroy(buffer);

    vkl_context_reset(ctx);

    FREE(data);
    FREE(data2);
    TEST_END
}



static int vklite2_context_texture(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklContext* ctx = vkl_context(gpu, NULL);

    vkl_new_texture(ctx, 2, (uvec3){16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM);

    TEST_END
}



static int vklite2_context_transfer_sync(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklContext* ctx = vkl_context(gpu, NULL);

    VklBufferRegions br = vkl_alloc_buffers(ctx, VKL_DEFAULT_BUFFER_VERTEX, 1, 16);
    uint8_t data[16] = {0};
    memset(data, 12, 16);
    vkl_buffer_regions_upload(ctx, &br, 0, 16, data);

    uint8_t data2[16] = {0};
    vkl_buffer_regions_download(ctx, &br, 0, 16, data2);
    AT(memcmp(data, data2, 16) == 0);

    uint8_t* img_data = calloc(16 * 16 * 4, sizeof(uint8_t));
    VklTexture* tex = vkl_new_texture(ctx, 2, (uvec3){16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM);
    vkl_texture_upload(ctx, tex, 16 * 16 * 4, img_data);

    uint8_t* img_data2 = calloc(16 * 16 * 4, sizeof(uint8_t));
    vkl_texture_download(ctx, tex, 16 * 16 * 4, img_data2);
    AT(memcmp(img_data, img_data2, 16 * 16 * 4) == 0);

    FREE(img_data2);
    FREE(img_data);
    TEST_END
}



static int vklite2_context_transfer_async_nothread(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklContext* ctx = vkl_context(gpu, NULL);
    vkl_transfer_mode(ctx, VKL_TRANSFER_MODE_ASYNC);

    VklBufferRegions br = vkl_alloc_buffers(ctx, VKL_DEFAULT_BUFFER_VERTEX, 1, 16);
    uint8_t data[16] = {0};
    memset(data, 12, 16);
    vkl_buffer_regions_upload(ctx, &br, 0, 16, data);
    vkl_transfer_loop(ctx, false);

    uint8_t data2[16] = {0};
    vkl_buffer_regions_download(ctx, &br, 0, 16, data2);
    vkl_transfer_loop(ctx, false);
    AT(memcmp(data, data2, 16) == 0);

    TEST_END
}



typedef struct _TestTransfer _TestTransfer;
struct _TestTransfer
{
    VklContext* ctx;

    VklBufferRegions br;
    VklTexture* tex;

    uint8_t data[16];
    uint8_t img_data[16 * 16 * 4];
    int status;
};



static void* _thread_enqueue(void* arg)
{
    _TestTransfer* tt = arg;

    // Upload data from a background thread.
    vkl_buffer_regions_upload(tt->ctx, &tt->br, 0, 16, tt->data);
    vkl_texture_upload(tt->ctx, tt->tex, 16 * 16 * 4, tt->img_data);

    // Cause the transfer loop to end.
    vkl_transfer_stop(tt->ctx);

    return NULL;
}

static int vklite2_context_transfer_async_thread(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklContext* ctx = vkl_context(gpu, NULL);
    vkl_transfer_mode(ctx, VKL_TRANSFER_MODE_ASYNC);

    // Resources.
    _TestTransfer tt = {0};
    tt.ctx = ctx;
    tt.br = vkl_alloc_buffers(ctx, VKL_DEFAULT_BUFFER_VERTEX, 1, 16);
    memset(tt.data, 12, 16);
    memset(tt.img_data, 23, 16);
    tt.tex = vkl_new_texture(ctx, 2, (uvec3){16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM);

    // Background thread.
    pthread_t thread = {0};
    // Launch the thread that enqueues transfer tasks.
    pthread_create(&thread, NULL, _thread_enqueue, &tt);
    // Run the transfer task dequeuing loop in the main thread.
    vkl_transfer_loop(ctx, true);
    pthread_join(thread, NULL);

    // Check.
    vkl_transfer_mode(ctx, VKL_TRANSFER_MODE_SYNC);
    uint8_t data2[16] = {0};
    vkl_buffer_regions_download(ctx, &tt.br, 0, 16, data2);
    AT(memcmp(tt.data, data2, 16) == 0);

    uint8_t img_data2[16 * 16 * 4];
    vkl_texture_download(ctx, tt.tex, 16 * 16 * 4, img_data2);
    AT(memcmp(tt.img_data, img_data2, 16 * 16 * 4) == 0);

    TEST_END
}



static void* _thread_download(void* arg)
{
    _TestTransfer* tt = arg;
    VklContext* ctx = tt->ctx;
    uint8_t data3[16] = {0};
    vkl_buffer_regions_download(ctx, &tt->br, 0, 16, data3);
    if (memcmp(tt->data, data3, 16) != 0)
        tt->status = 1;
    vkl_transfer_wait(ctx, 10);
    if (memcmp(tt->data, data3, 16) == 0)
        tt->status = 1;
    tt->status = 0;

    vkl_transfer_stop(ctx);

    return NULL;
}

static int vklite2_context_download(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklContext* ctx = vkl_context(gpu, NULL);
    vkl_transfer_mode(ctx, VKL_TRANSFER_MODE_ASYNC);

    // Resources.
    _TestTransfer tt = {0};
    tt.status = -1;
    tt.ctx = ctx;
    tt.br = vkl_alloc_buffers(ctx, VKL_DEFAULT_BUFFER_VERTEX, 1, 16);
    memset(tt.data, 12, 16);
    memset(tt.img_data, 23, 16);
    tt.tex = vkl_new_texture(ctx, 2, (uvec3){16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM);

    // Background thread.
    pthread_t thread = {0};
    ASSERT(tt.status == -1);
    // The thread will launch a download task and wait until the download has finished.
    pthread_create(&thread, NULL, _thread_download, &tt);
    // The download task will be processed in the main thread.
    vkl_transfer_loop(tt.ctx, true);
    pthread_join(thread, NULL);
    ASSERT(tt.status == 0);

    TEST_END
}



static int vklite2_default_app(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklContext* ctx = vkl_context(gpu, NULL);

    char path[1024];
    snprintf(path, sizeof(path), "%s/spirv/pow2.comp.spv", DATA_DIR);
    vkl_new_compute(ctx, path);

    TEST_END
}



/*************************************************************************************************/
/*  Canvas 1                                                                                     */
/*************************************************************************************************/

static void _frame_callback(VklCanvas* canvas, VklPrivateEvent ev)
{
    log_debug(
        "canvas #%d, frame callback #%d, time %.6f, interval %.6f", //
        canvas->obj.id, ev.u.f.idx, ev.u.f.time, ev.u.f.interval);
}

static void _key_callback(VklCanvas* canvas, VklEvent ev)
{
    if (ev.u.k.type == VKL_KEY_PRESS)
        log_debug("key code %d", ev.u.k.key_code);
}

static int vklite2_canvas_1(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);

    uvec2 size = {0};

    // Framebuffer size.
    vkl_canvas_size(canvas, VKL_CANVAS_SIZE_FRAMEBUFFER, size);
    log_debug("canvas framebuffer size is %dx%d", size[0], size[1]);
    ASSERT(size[0] > 0);
    ASSERT(size[1] > 0);

    // Screen size.
    vkl_canvas_size(canvas, VKL_CANVAS_SIZE_SCREEN, size);
    log_debug("canvas screen size is %dx%d", size[0], size[1]);
    ASSERT(size[0] > 0);
    ASSERT(size[1] > 0);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_FRAME, 0, _frame_callback, NULL);

    vkl_app_run(app, 8);

    // Send a mock key press event.
    vkl_event_callback(canvas, VKL_EVENT_KEY, 0, _key_callback, NULL);
    vkl_event_key(canvas, VKL_KEY_PRESS, VKL_KEY_A);

    // Second canvas.
    log_debug("global clock elapsed %.6f interval %.6f", app->clock.elapsed, app->clock.interval);
    log_debug(
        "local clock elapsed %.6f interval %.6f", canvas->clock.elapsed, canvas->clock.interval);

    // Second canvas.
    VklCanvas* canvas2 = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
    vkl_canvas_clear_color(canvas, (VkClearColorValue){{1, 0, 0, 1}});
    vkl_canvas_clear_color(canvas2, (VkClearColorValue){{0, 1, 0, 1}});
    vkl_app_run(app, 5);

    TEST_END
}



/*************************************************************************************************/
/*  Canvas 2                                                                                     */
/*************************************************************************************************/

static void _wheel_callback(VklCanvas* canvas, VklEvent ev)
{
    log_debug("wheel %.3f", ev.u.w.dir[1]);
}

static void _button_callback(VklCanvas* canvas, VklEvent ev)
{
    if (ev.u.b.type == VKL_MOUSE_PRESS)
        log_debug("clicked %d mods %d", ev.u.b.button, ev.u.b.modifiers);
}

static void _cursor_callback(VklCanvas* canvas, VklEvent ev)
{
    uvec2 size = {0};
    vkl_canvas_size(canvas, VKL_CANVAS_SIZE_SCREEN, size);
    double x = ev.u.m.pos[0] / (double)size[0];
    double y = ev.u.m.pos[1] / (double)size[1];
    vkl_canvas_clear_color(canvas, (VkClearColorValue){{x, 0, y, 1}});
}

static void _timer_callback(VklCanvas* canvas, VklPrivateEvent ev)
{
    log_trace("timer callback #%d time %.3f", ev.u.t.idx, ev.u.t.time);
    float x = exp(-.01 * (float)ev.u.t.idx);
    vkl_canvas_clear_color(canvas, (VkClearColorValue){{x, 0, 0, 1}});
}

static int vklite2_canvas_2(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);

    vkl_event_callback(canvas, VKL_EVENT_KEY, 0, _key_callback, NULL);
    vkl_event_callback(canvas, VKL_EVENT_MOUSE_WHEEL, 0, _wheel_callback, NULL);
    vkl_event_callback(canvas, VKL_EVENT_MOUSE_BUTTON, 0, _button_callback, NULL);
    vkl_event_callback(canvas, VKL_EVENT_MOUSE_MOVE, 0, _cursor_callback, NULL);

    // vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_TIMER, .05, _timer_callback, NULL);

    vkl_app_run(app, 0);
    TEST_END
}



/*************************************************************************************************/
/*  Canvas triangle                                                                              */
/*************************************************************************************************/

static void _triangle_refill(VklCanvas* canvas, VklPrivateEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(ev.u.rf.cmd_count == 1);
    VklCommands* cmds = ev.u.rf.cmds[0];
    ASSERT(cmds->queue_idx == VKL_DEFAULT_QUEUE_RENDER);
    TestCanvas* c = ev.user_data;

    // HACK
    c->renderpass = canvas->renderpasses[0];
    c->framebuffers = canvas->framebuffers;

    triangle_commands(c, cmds, ev.u.rf.img_idx);
}

// Triangle canvas
static int vklite2_canvas_3(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
    AT(canvas != NULL);

    TestVisual visual = {0};
    TestCanvas c = {0};
    c.gpu = gpu;

    // HACK: 2 copies of the renderpass
    c.renderpass = canvas->renderpasses[0];
    c.framebuffers = canvas->framebuffers;

    test_triangle(&c, &visual, "");
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _triangle_refill, &c);

    vkl_app_run(app, 0);

    destroy_visual(&visual);
    TEST_END
}



/*************************************************************************************************/
/*  Canvas triangle with push constant                                                           */
/*************************************************************************************************/

static vec3 push_vec; // NOTE: not thread-safe

static void _triangle_push_refill(VklCanvas* canvas, VklPrivateEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(ev.u.rf.cmd_count == 1);
    VklCommands* cmds = ev.u.rf.cmds[0];
    ASSERT(cmds->queue_idx == VKL_DEFAULT_QUEUE_RENDER);
    TestCanvas* c = ev.user_data;
    uint32_t idx = ev.u.rf.img_idx;

    // HACK
    c->renderpass = canvas->renderpasses[0];
    c->framebuffers = canvas->framebuffers;

    vkl_cmd_begin(cmds, idx);
    vkl_cmd_begin_renderpass(cmds, idx, &c->renderpass, &canvas->framebuffers);
    vkl_cmd_viewport(
        cmds, idx,
        (VkViewport){
            0, 0, c->framebuffers.attachments[0]->width, c->framebuffers.attachments[0]->height, 0,
            1});
    vkl_cmd_bind_vertex_buffer(cmds, idx, &c->buffer_regions, 0);
    vkl_cmd_bind_graphics(cmds, idx, c->graphics, c->bindings, 0);

    // Push constants.
    vkl_cmd_push_constants(
        cmds, idx, c->graphics->slots, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vec3), push_vec);

    vkl_cmd_draw(cmds, idx, 0, 3);
    vkl_cmd_end_renderpass(cmds, idx);
    vkl_cmd_end(cmds, idx);
}

static void _push_cursor_callback(VklCanvas* canvas, VklEvent ev)
{
    uvec2 size = {0};
    vkl_canvas_size(canvas, VKL_CANVAS_SIZE_SCREEN, size);
    double x = ev.u.m.pos[0] / (double)size[0];
    double y = ev.u.m.pos[1] / (double)size[1];
    push_vec[0] = x;
    push_vec[1] = y;
    push_vec[2] = 1;
    vkl_canvas_to_refill(canvas, true);
}

static int vklite2_canvas_4(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
    AT(canvas != NULL);

    TestVisual visual = {0};
    TestCanvas c = {0};
    c.gpu = gpu;

    // HACK: 2 copies of the renderpass
    c.renderpass = canvas->renderpasses[0];
    c.framebuffers = canvas->framebuffers;

    // Triangle graphics.
    _triangle_graphics(&c, &visual, "_push");

    // Create the slots.
    visual.slots = vkl_slots(gpu);
    vkl_slots_push_constant(&visual.slots, 0, sizeof(vec3), VK_SHADER_STAGE_VERTEX_BIT);
    vkl_slots_create(&visual.slots);
    vkl_graphics_slots(visual.graphics, &visual.slots);

    // Create the bindings.
    visual.bindings = vkl_bindings(&visual.slots);
    vkl_bindings_create(&visual.bindings, 1);
    vkl_bindings_update(&visual.bindings);
    c.bindings = &visual.bindings;

    // Create the graphics pipeline.
    vkl_graphics_create(visual.graphics);

    // Triangle buffer.
    visual.buffer = vkl_buffer(gpu);
    VkDeviceSize size = 3 * sizeof(VklVertex);
    vkl_buffer_size(&visual.buffer, size);
    vkl_buffer_usage(&visual.buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    vkl_buffer_memory(
        &visual.buffer,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkl_buffer_create(&visual.buffer);

    // Upload the triangle data.
    VklVertex data[3] = {
        {{-1, +1, 0}, {1, 0, 0, 1}},
        {{+1, +1, 0}, {0, 1, 0, 1}},
        {{+0, -1, 0}, {0, 0, 1, 1}},
    };
    vkl_buffer_upload(&visual.buffer, 0, size, data);

    c.buffer_regions.buffer = &visual.buffer;
    c.buffer_regions.size = size;
    c.buffer_regions.count = 1;

    // Refill callback
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _triangle_push_refill, &c);

    // Cursor callback.
    vkl_event_callback(canvas, VKL_EVENT_MOUSE_MOVE, 0, _push_cursor_callback, NULL);

    vkl_app_run(app, 0);
    destroy_visual(&visual);
    TEST_END
}



/*************************************************************************************************/
/*  Canvas triangle with vertex buffer update                                                    */
/*************************************************************************************************/

static void _vertex_cursor_callback(VklCanvas* canvas, VklEvent ev)
{
    uvec2 size = {0};
    vkl_canvas_size(canvas, VKL_CANVAS_SIZE_SCREEN, size);
    double x = ev.u.m.pos[0] / (double)size[0];
    double y = ev.u.m.pos[1] / (double)size[1];

    TestCanvas* c = ev.user_data;
    VklVertex* data = (VklVertex*)c->data;
    for (uint32_t i = 0; i < 3; i++)
    {
        data[i].color[0] = x;
        data[i].color[1] = y;
        data[i].color[2] = 1;
    }
    vkl_buffer_regions_upload(
        canvas->gpu->context, &c->buffer_regions, 0, 3 * sizeof(VklVertex), data);
}

static int vklite2_canvas_5(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
    AT(canvas != NULL);

    TestVisual visual = {0};
    TestCanvas c = {0};
    c.gpu = gpu;
    c.data = calloc(3, sizeof(VklVertex));

    // HACK: 2 copies of the renderpass
    c.renderpass = canvas->renderpasses[0];
    c.framebuffers = canvas->framebuffers;

    // Triangle graphics.
    _triangle_graphics(&c, &visual, "");

    // Create the slots.
    visual.slots = vkl_slots(gpu);
    vkl_slots_create(&visual.slots);
    vkl_graphics_slots(visual.graphics, &visual.slots);

    // Create the bindings.
    visual.bindings = vkl_bindings(&visual.slots);
    vkl_bindings_create(&visual.bindings, 1);
    vkl_bindings_update(&visual.bindings);
    c.bindings = &visual.bindings;

    // Create the graphics pipeline.
    vkl_graphics_create(visual.graphics);

    // Triangle buffer.
    VkDeviceSize size = 3 * sizeof(VklVertex);
    c.buffer_regions = vkl_alloc_buffers(gpu->context, VKL_DEFAULT_BUFFER_VERTEX, 1, size);

    // Upload the triangle data.
    VklVertex data[3] = {
        {{-1, +1, 0}, {1, 0, 0, 1}},
        {{+1, +1, 0}, {0, 1, 0, 1}},
        {{+0, -1, 0}, {0, 0, 1, 1}},
    };
    memcpy(c.data, data, sizeof(data));
    vkl_buffer_regions_upload(canvas->gpu->context, &c.buffer_regions, 0, size, data);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _triangle_refill, &c);

    // Cursor callback.
    vkl_event_callback(canvas, VKL_EVENT_MOUSE_MOVE, 0, _vertex_cursor_callback, &c);

    vkl_app_run(app, 0);

    destroy_visual(&visual);
    FREE(c.data);
    TEST_END
}



/*************************************************************************************************/
/*  Canvas triangle with uniform buffer update                                                   */
/*************************************************************************************************/

vec4 vec = {1, 0, 1, 1};

static void _uniform_cursor_callback(VklCanvas* canvas, VklEvent ev)
{
    uvec2 size = {0};
    vkl_canvas_size(canvas, VKL_CANVAS_SIZE_SCREEN, size);
    double x = ev.u.m.pos[0] / (double)size[0];
    double y = ev.u.m.pos[1] / (double)size[1];
    TestCanvas* c = ev.user_data;

    vec[0] = x;
    vec[1] = y;
    vec[2] = 1;
    vec[3] = 1;
    vkl_buffer_regions_upload(
        canvas->gpu->context, &c->uniform_buffer_regions, 0, sizeof(vec4), vec);
}

static int vklite2_canvas_6(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);
    AT(canvas != NULL);
    uint32_t img_count = canvas->swapchain.img_count;
    ASSERT(img_count > 0);

    TestVisual visual = {0};
    TestCanvas c = {0};
    c.gpu = gpu;
    c.data = calloc(3, sizeof(VklVertex));

    // HACK: 2 copies of the renderpass
    c.renderpass = canvas->renderpasses[0];
    c.framebuffers = canvas->framebuffers;

    // Triangle graphics.
    _triangle_graphics(&c, &visual, "_ubo");

    // Create the slots.
    visual.slots = vkl_slots(gpu);
    vkl_slots_binding(&visual.slots, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    vkl_slots_create(&visual.slots);
    vkl_graphics_slots(visual.graphics, &visual.slots);

    // Uniform buffer.
    c.uniform_buffer_regions =
        vkl_alloc_buffers(gpu->context, VKL_DEFAULT_BUFFER_UNIFORM, img_count, sizeof(vec4));
    ASSERT(c.uniform_buffer_regions.aligned_size >= c.uniform_buffer_regions.size);
    vkl_buffer_regions_upload(
        canvas->gpu->context, &c.uniform_buffer_regions, 0, sizeof(vec4), vec);

    // Create the bindings.
    visual.bindings = vkl_bindings(&visual.slots);
    ASSERT(c.uniform_buffer_regions.buffer != VK_NULL_HANDLE);
    vkl_bindings_create(&visual.bindings, img_count);
    vkl_bindings_buffer(&visual.bindings, 0, &c.uniform_buffer_regions);
    vkl_bindings_update(&visual.bindings);
    c.bindings = &visual.bindings;

    // Create the graphics pipeline.
    vkl_graphics_create(visual.graphics);

    // Triangle buffer.
    VkDeviceSize size = 3 * sizeof(VklVertex);
    c.buffer_regions = vkl_alloc_buffers(gpu->context, VKL_DEFAULT_BUFFER_VERTEX, 1, size);

    // Upload the triangle data.
    VklVertex data[3] = {
        {{-1, +1, 0}, {1, 0, 0, 1}},
        {{+1, +1, 0}, {0, 1, 0, 1}},
        {{+0, -1, 0}, {0, 0, 1, 1}},
    };
    memcpy(c.data, data, sizeof(data));
    vkl_buffer_regions_upload(canvas->gpu->context, &c.buffer_regions, 0, size, data);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _triangle_refill, &c);

    // Cursor callback.
    vkl_event_callback(canvas, VKL_EVENT_MOUSE_MOVE, 0, _uniform_cursor_callback, &c);

    vkl_app_run(app, 0);

    destroy_visual(&visual);
    FREE(c.data);
    TEST_END
}
