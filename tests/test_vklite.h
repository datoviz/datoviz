/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_TEST_VKLITE
#define DVZ_HEADER_TEST_VKLITE



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../src/vklite_utils.h"
#include "_glfw.h"
#include "surface.h"
#include "test_resources.h"
#include "testing.h"
#include "vklite.h"
#include "window.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH        800
#define HEIGHT       600
#define PRESENT_MODE VK_PRESENT_MODE_FIFO_KHR
#define FORMAT       VK_FORMAT_B8G8R8A8_UNORM

static const VkClearColorValue BACKGROUND = {{.4f, .6f, .8f, 1.0f}};

#define DEBUG_TEST (getenv("DVZ_DEBUG") != NULL)

#define N_FRAMES (DEBUG_TEST ? 0 : 5)

#define TRIANGLE_VERTICES                                                                         \
    {                                                                                             \
        {{-1, +1, 0}, {1, 0, 0, 1}}, {{+1, +1, 0}, {0, 1, 0, 1}}, {{+0, -1, 0}, {0, 0, 1, 1}},    \
    }



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct TestCanvas TestCanvas;
typedef struct TestVisual TestVisual;
typedef struct TestVertex TestVertex;

typedef void (*FillCallback)(TestCanvas*, DvzCommands*, uint32_t);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct TestCanvas
{
    DvzGpu* gpu;
    bool is_offscreen;

    DvzWindow* window;
    VkSurfaceKHR surface;

    DvzRenderpass renderpass;
    DvzFramebuffers framebuffers;
    DvzSwapchain swapchain;

    DvzImages* images;
    DvzImages* depth;

    DvzCompute* compute;
    DvzBindings* bindings;
    DvzGraphics* graphics;

    // NOTE: this is used in vklite:
    DvzBufferRegions br;

    // NOTE: this is used in canvas tests
    // DvzDat* dat;
    bool always_refill;

    void* data;
};



struct TestVisual
{
    DvzGpu* gpu;
    DvzRenderpass* renderpass;
    DvzFramebuffers* framebuffers;
    DvzGraphics graphics;
    DvzCompute* compute;
    DvzBindings bindings;
    DvzBuffer buffer;

    // NOTE: this is used in vklite:
    DvzBufferRegions br;
    DvzBufferRegions br_u;

    // NOTE: this is used in canvas tests
    // DvzDat *dat, *dat_u;

    uint32_t n_vertices;
    float dt;
    void* data;
    void* data_u;
    void* user_data;
};



struct TestVertex
{
    vec3 pos;
    vec4 color;
};



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void empty_commands(TestCanvas* canvas, DvzCommands* cmds, uint32_t idx)
{
    dvz_cmd_begin(cmds, idx);
    dvz_cmd_begin_renderpass(cmds, idx, &canvas->renderpass, &canvas->framebuffers);
    dvz_cmd_end_renderpass(cmds, idx);
    dvz_cmd_end(cmds, idx);
}



static DvzRenderpass make_renderpass(
    DvzGpu* gpu, VkClearColorValue clear_color_value, VkFormat format, VkImageLayout layout)
{
    DvzRenderpass renderpass = dvz_renderpass(gpu);

    VkClearValue clear_color = {0};
    clear_color.color = clear_color_value;

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
    // dvz_renderpass_subpass_dependency(&renderpass, 0, VK_SUBPASS_EXTERNAL, 0);
    // dvz_renderpass_subpass_dependency_stage(
    //     &renderpass, 0, //
    //     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    //     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    // dvz_renderpass_subpass_dependency_access(
    //     &renderpass, 0, 0,
    //     VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

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



static void* screenshot(DvzImages* images, VkDeviceSize bytes_per_component)
{
    // NOTE: the caller must free the output

    DvzGpu* gpu = images->gpu;

    // Create the staging image.
    log_debug("starting creation of staging image");
    DvzImages staging_struct = dvz_images(gpu, VK_IMAGE_TYPE_2D, 1);
    DvzImages* staging = (DvzImages*)calloc(1, sizeof(DvzImages));
    *staging = staging_struct;
    dvz_images_format(staging, images->format);
    dvz_images_size(staging, images->shape);
    dvz_images_tiling(staging, VK_IMAGE_TILING_LINEAR);
    dvz_images_usage(staging, VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    dvz_images_layout(staging, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    // dvz_images_memory(
    //     staging, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    dvz_images_vma_usage(staging, VMA_MEMORY_USAGE_CPU_ONLY);
    dvz_images_create(staging);

    // Start the image transition command buffers.
    DvzCommands cmds = dvz_commands(gpu, 0, 1);
    dvz_cmd_begin(&cmds, 0);

    DvzBarrier barrier = dvz_barrier(gpu);
    dvz_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    dvz_barrier_images(&barrier, staging);
    dvz_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    dvz_barrier_images_access(&barrier, 0, VK_ACCESS_TRANSFER_WRITE_BIT);
    dvz_cmd_barrier(&cmds, 0, &barrier);

    // Copy the image to the staging image.
    dvz_cmd_copy_image(&cmds, 0, images, staging);

    dvz_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
    dvz_barrier_images_access(&barrier, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT);
    dvz_cmd_barrier(&cmds, 0, &barrier);

    // End the cmds and submit them.
    dvz_cmd_end(&cmds, 0);
    dvz_cmd_submit_sync(&cmds, 0);

    // Now, copy the staging image into CPU memory.
    void* rgb = calloc(images->shape[0] * images->shape[1], 3 * bytes_per_component);
    dvz_images_download(staging, 0, bytes_per_component, true, false, rgb);

    dvz_images_destroy(staging);
    FREE(staging);
    return rgb;
}



static DvzGpu* make_gpu(DvzHost* host)
{
    ASSERT(host != NULL);

    DvzGpu* gpu = dvz_gpu_best(host);
    _default_queues(gpu, true);
    dvz_gpu_request_features(gpu, (VkPhysicalDeviceFeatures){.independentBlend = true});
    dvz_gpu_create_with_surface(gpu);

    return gpu;
}



/*************************************************************************************************/
/*  Test offscreen canvas                                                                        */
/*************************************************************************************************/

static TestCanvas offscreen(DvzGpu* gpu)
{
    TestCanvas canvas = {0};
    canvas.gpu = gpu;
    canvas.is_offscreen = true;

    canvas.renderpass =
        make_renderpass(gpu, BACKGROUND, FORMAT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    // Color attachment
    DvzImages images_struct = dvz_images(canvas.renderpass.gpu, VK_IMAGE_TYPE_2D, 1);
    DvzImages* images = (DvzImages*)calloc(1, sizeof(DvzImages));
    ASSERT(images != NULL);
    *images = images_struct;
    dvz_images_format(images, canvas.renderpass.attachments[0].format);
    dvz_images_size(images, (uvec3){WIDTH, HEIGHT, 1});
    dvz_images_tiling(images, VK_IMAGE_TILING_OPTIMAL);
    dvz_images_usage(
        images, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    dvz_images_memory(images, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    dvz_images_aspect(images, VK_IMAGE_ASPECT_COLOR_BIT);
    dvz_images_layout(images, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    dvz_images_queue_access(images, 0);
    dvz_images_create(images);
    canvas.images = images;

    // Depth attachment.
    DvzImages depth_struct = dvz_images(gpu, VK_IMAGE_TYPE_2D, 1);
    DvzImages* depth = (DvzImages*)calloc(1, sizeof(DvzImages));
    ASSERT(depth != NULL);
    *depth = depth_struct;
    depth_image(depth, &canvas.renderpass, WIDTH, HEIGHT);
    canvas.depth = depth;

    // Create renderpass.
    dvz_renderpass_create(&canvas.renderpass);

    // Create framebuffers.
    canvas.framebuffers = dvz_framebuffers(canvas.renderpass.gpu);
    dvz_framebuffers_attachment(&canvas.framebuffers, 0, images);
    dvz_framebuffers_attachment(&canvas.framebuffers, 1, depth);
    dvz_framebuffers_create(&canvas.framebuffers, &canvas.renderpass);

    return canvas;
}



/*************************************************************************************************/
/*  Test canvas                                                                                  */
/*************************************************************************************************/

static TestCanvas test_canvas_create(DvzGpu* gpu, DvzWindow* window, VkSurfaceKHR surface)
{
    ASSERT(gpu != NULL);
    ASSERT(window != NULL);
    DvzHost* host = gpu->host;
    ASSERT(host != NULL);

    TestCanvas canvas = {0};
    canvas.is_offscreen = false;
    canvas.gpu = gpu;
    canvas.window = window;

    canvas.surface = surface;
    ASSERT(canvas.surface != VK_NULL_HANDLE);

    // uint32_t framebuffer_width = 0, framebuffer_height = 0;
    // backend_get_framebuffer_size(window, &framebuffer_width, &framebuffer_height);
    // ASSERT(framebuffer_width > 0);
    // ASSERT(framebuffer_height > 0);

    DvzRenderpass renderpass =
        make_renderpass(gpu, BACKGROUND, FORMAT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    canvas.renderpass = renderpass;

    canvas.swapchain = dvz_swapchain(canvas.renderpass.gpu, canvas.surface, 3);
    dvz_swapchain_format(&canvas.swapchain, VK_FORMAT_B8G8R8A8_UNORM);
    dvz_swapchain_present_mode(&canvas.swapchain, PRESENT_MODE);
    dvz_swapchain_create(&canvas.swapchain);
    canvas.images = canvas.swapchain.images;

    // Depth attachment.
    DvzImages depth_struct = dvz_images(gpu, VK_IMAGE_TYPE_2D, 1);
    DvzImages* depth = (DvzImages*)calloc(1, sizeof(DvzImages));
    ASSERT(depth != NULL);
    *depth = depth_struct;
    depth_image(depth, &canvas.renderpass, canvas.images->shape[0], canvas.images->shape[1]);
    canvas.depth = depth;

    // Create renderpass.
    dvz_renderpass_create(&canvas.renderpass);

    // Create framebuffers.
    canvas.framebuffers = dvz_framebuffers(canvas.renderpass.gpu);
    dvz_framebuffers_attachment(&canvas.framebuffers, 0, canvas.swapchain.images);
    dvz_framebuffers_attachment(&canvas.framebuffers, 1, depth);
    dvz_framebuffers_create(&canvas.framebuffers, &canvas.renderpass);

    return canvas;
}



static void test_canvas_show(TestCanvas* canvas, FillCallback fill_commands, uint32_t n_frames)
{
    DvzGpu* gpu = canvas->gpu;
    DvzWindow* window = canvas->window;
    DvzRenderpass* renderpass = &canvas->renderpass;
    DvzFramebuffers* framebuffers = &canvas->framebuffers;
    DvzSwapchain* swapchain = &canvas->swapchain;
    DvzHost* host = gpu->host;
    ASSERT(host != NULL);
    DvzBackend backend = host->backend;

    ASSERT(swapchain != NULL);
    ASSERT(swapchain->img_count > 0);

    DvzCommands cmds = dvz_commands(gpu, 0, swapchain->img_count);
    for (uint32_t i = 0; i < cmds.count; i++)
        fill_commands(canvas, &cmds, i);

    // Sync objects.
    DvzSemaphores sem_img_available = dvz_semaphores(gpu, DVZ_MAX_FRAMES_IN_FLIGHT);
    DvzSemaphores sem_render_finished = dvz_semaphores(gpu, DVZ_MAX_FRAMES_IN_FLIGHT);
    DvzFences fences = dvz_fences(gpu, DVZ_MAX_FRAMES_IN_FLIGHT, true);
    DvzFences bak_fences = {0};
    bak_fences.gpu = gpu;
    bak_fences.count = swapchain->img_count;
    uint32_t cur_frame = 0;

    n_frames = n_frames == 0 ? INFINITY : n_frames;
    for (uint32_t frame = 0; frame < n_frames; frame++)
    {
        log_debug("iteration %d", frame);
        ASSERT(swapchain->images == canvas->images);

        backend_poll_events(backend);

        if (backend_should_close(window) || window->obj.status == DVZ_OBJECT_STATUS_NEED_DESTROY)
            break;

        // Wait for fence.
        dvz_fences_wait(&fences, cur_frame);

        // We acquire the next swapchain image.
        dvz_swapchain_acquire(swapchain, &sem_img_available, cur_frame, NULL, 0);
        if (swapchain->obj.status == DVZ_OBJECT_STATUS_INVALID)
        {
            dvz_gpu_wait(gpu);
            break;
        }
        // Handle resizing.
        else if (swapchain->obj.status == DVZ_OBJECT_STATUS_NEED_RECREATE)
        {
            log_trace("recreating the swapchain");

            // Wait until the device is ready and the window fully resized.
            // Framebuffer new size.
            backend_get_window_size(window, &window->width, &window->height);
            // backend_get_framebuffer_size(window, &width, &height);
            dvz_gpu_wait(gpu);

            // Destroy swapchain resources.
            dvz_framebuffers_destroy(framebuffers);
            dvz_images_destroy(canvas->depth);
            dvz_images_destroy(canvas->images);
            // dvz_swapchain_destroy(swapchain);

            // Recreate the swapchain. This will automatically set the swapchain->images new
            // size.
            dvz_swapchain_recreate(swapchain);

            // Find the new framebuffer size as determined by the swapchain recreation.
            uint32_t width, height;
            width = swapchain->images->shape[0];
            height = swapchain->images->shape[1];

            // The instance should be the same.
            ASSERT(swapchain->images == canvas->images);

            // Need to recreate the depth image with the new size.
            dvz_images_size(canvas->depth, (uvec3){width, height, 1});
            dvz_images_create(canvas->depth);

            // Recreate the framebuffers with the new size.
            ASSERT(framebuffers->attachments[0]->shape[0] == width);
            ASSERT(framebuffers->attachments[0]->shape[1] == height);
            dvz_framebuffers_create(framebuffers, renderpass);

            // Need to refill the command buffers.
            for (uint32_t i = 0; i < cmds.count; i++)
            {
                dvz_cmd_reset(&cmds, i);
                fill_commands(canvas, &cmds, i);
            }
        }
        else
        {
            dvz_fences_copy(&fences, cur_frame, &bak_fences, swapchain->img_idx);

            if (canvas->always_refill)
            {
                // Refill the command buffer.
                dvz_cmd_reset(&cmds, swapchain->img_idx);
                fill_commands(canvas, &cmds, swapchain->img_idx);
            }

            // Then, we submit the cmds on that image
            DvzSubmit submit = dvz_submit(gpu);
            dvz_submit_commands(&submit, &cmds);
            dvz_submit_wait_semaphores(
                &submit, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, &sem_img_available,
                cur_frame);
            // Once the render is finished, we signal another semaphore.
            dvz_submit_signal_semaphores(&submit, &sem_render_finished, cur_frame);
            dvz_submit_send(&submit, swapchain->img_idx, &fences, cur_frame);

            // Once the image is rendered, we present the swapchain image.
            dvz_swapchain_present(swapchain, 1, &sem_render_finished, cur_frame);

            cur_frame = (cur_frame + 1) % DVZ_MAX_FRAMES_IN_FLIGHT;
        }

        // IMPORTANT: we need to wait for the present queue to be idle, otherwise the GPU hangs
        // when waiting for fences (not sure why). The problem only arises when using different
        // queues for command buffer submission and swapchain present.
        dvz_queue_wait(gpu, 1);
    }
    log_trace("end of main loop");
    dvz_gpu_wait(gpu);

    dvz_semaphores_destroy(&sem_img_available);
    dvz_semaphores_destroy(&sem_render_finished);
    dvz_fences_destroy(&fences);
}



static void test_canvas_destroy(TestCanvas* canvas)
{
    log_trace("destroy canvas");

    if (canvas->is_offscreen)
    {
        dvz_images_destroy(canvas->images);
        FREE(canvas->images);
    }

    dvz_images_destroy(canvas->depth);
    FREE(canvas->depth);

    dvz_renderpass_destroy(&canvas->renderpass);
    dvz_swapchain_destroy(&canvas->swapchain);
    dvz_framebuffers_destroy(&canvas->framebuffers);
    dvz_window_destroy(canvas->window);
    dvz_surface_destroy(canvas->gpu->host, canvas->surface);
}



/*************************************************************************************************/
/*  Test graphics                                                                                */
/*************************************************************************************************/

static DvzGraphics triangle_graphics(DvzGpu* gpu, DvzRenderpass* renderpass, const char* suffix)
{
    DvzGraphics graphics = dvz_graphics(gpu);

    dvz_graphics_renderpass(&graphics, renderpass, 0);
    dvz_graphics_topology(&graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    dvz_graphics_polygon_mode(&graphics, VK_POLYGON_MODE_FILL);
    dvz_graphics_depth_test(&graphics, DVZ_DEPTH_TEST_ENABLE);

    char path[1024];
    snprintf(path, sizeof(path), "%s/test_triangle%s.vert.spv", SPIRV_DIR, suffix);
    dvz_graphics_shader(&graphics, VK_SHADER_STAGE_VERTEX_BIT, path);
    snprintf(path, sizeof(path), "%s/test_triangle%s.frag.spv", SPIRV_DIR, suffix);
    dvz_graphics_shader(&graphics, VK_SHADER_STAGE_FRAGMENT_BIT, path);
    dvz_graphics_vertex_binding(&graphics, 0, sizeof(TestVertex));
    dvz_graphics_vertex_attr(
        &graphics, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TestVertex, pos));
    dvz_graphics_vertex_attr(
        &graphics, 0, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(TestVertex, color));

    return graphics;
}



static TestVisual triangle_visual(
    DvzGpu* gpu, DvzRenderpass* renderpass, DvzFramebuffers* framebuffers, const char* suffix)
{
    TestVisual visual = {0};
    visual.gpu = gpu;
    visual.renderpass = renderpass;
    visual.framebuffers = framebuffers;

    // Make the graphics.
    visual.graphics = triangle_graphics(gpu, renderpass, suffix);

    if (strncmp(suffix, "_push", 5) == 0)
        dvz_graphics_push(&visual.graphics, 0, sizeof(vec3), VK_SHADER_STAGE_VERTEX_BIT);

    // Create the bindings.
    visual.bindings = dvz_bindings(&visual.graphics.slots, 1);
    dvz_bindings_update(&visual.bindings);

    // Create the graphics pipeline.
    dvz_graphics_create(&visual.graphics);

    // Create the buffer.
    visual.buffer = dvz_buffer(gpu);
    VkDeviceSize size = 3 * sizeof(TestVertex);
    dvz_buffer_size(&visual.buffer, size);
    dvz_buffer_usage(
        &visual.buffer,                          //
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |      //
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | //
            VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    // dvz_buffer_memory(
    //     &visual.buffer,
    //     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    dvz_buffer_vma_usage(&visual.buffer, VMA_MEMORY_USAGE_CPU_ONLY);
    dvz_buffer_create(&visual.buffer);

    // Upload the triangle data.
    TestVertex data[] = TRIANGLE_VERTICES;
    dvz_buffer_upload(&visual.buffer, 0, size, data);
    dvz_queue_wait(gpu, 0); // DVZ_DEFAULT_QUEUE_TRANSFER

    return visual;
}



static void triangle_commands(
    DvzCommands* cmds, uint32_t idx, DvzRenderpass* renderpass, DvzFramebuffers* framebuffers,
    DvzGraphics* graphics, DvzBindings* bindings, DvzBufferRegions br)
{
    ASSERT(renderpass != NULL);
    ASSERT(renderpass->renderpass != VK_NULL_HANDLE);

    ASSERT(framebuffers != NULL);
    ASSERT(framebuffers->framebuffers[0] != VK_NULL_HANDLE);

    ASSERT(graphics != NULL);
    ASSERT(graphics->pipeline != VK_NULL_HANDLE);

    ASSERT(bindings != NULL);
    ASSERT(bindings->dsets != VK_NULL_HANDLE);

    ASSERT(br.buffer != NULL);
    ASSERT(br.buffer->buffer != VK_NULL_HANDLE);

    uint32_t width = framebuffers->attachments[0]->shape[0];
    uint32_t height = framebuffers->attachments[0]->shape[1];
    uint32_t n_vertices = 3;
    // (uint32_t)(br.size / sizeof(TestVertex));
    // n_vertices = n_vertices > 0 ? n_vertices : 3;
    // log_debug("refill n vertices: %d", n_vertices);
    // ASSERT(n_vertices > 0);

    ASSERT(width > 0);
    ASSERT(height > 0);

    // Commands.
    dvz_cmd_begin(cmds, idx);
    dvz_cmd_begin_renderpass(cmds, idx, renderpass, framebuffers);
    dvz_cmd_viewport(cmds, idx, (VkViewport){0, 0, (float)width, (float)height, 0, 1});
    dvz_cmd_bind_vertex_buffer(cmds, idx, br, 0);
    dvz_cmd_bind_graphics(cmds, idx, graphics, bindings, 0);

    if (graphics->slots.push_count > 0)
        dvz_cmd_push(
            cmds, idx, &graphics->slots, VK_SHADER_STAGE_VERTEX_BIT, 0, //
            sizeof(vec3), graphics->user_data);

    dvz_cmd_draw(cmds, idx, 0, n_vertices);
    dvz_cmd_end_renderpass(cmds, idx);
    dvz_cmd_end(cmds, idx);
}



static void destroy_visual(TestVisual* visual)
{
    dvz_graphics_destroy(&visual->graphics);
    dvz_bindings_destroy(&visual->bindings);
    dvz_buffer_destroy(&visual->buffer);
    FREE(visual->user_data);
    FREE(visual->data);
}



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_vklite_host(TstSuite*);
int test_vklite_commands(TstSuite*);
int test_vklite_buffer_1(TstSuite*);
int test_vklite_buffer_resize(TstSuite*);
int test_vklite_load_shader(TstSuite*);
int test_vklite_compute(TstSuite*);
int test_vklite_push(TstSuite*);
int test_vklite_images(TstSuite*);
int test_vklite_sampler(TstSuite*);
int test_vklite_barrier_buffer(TstSuite*);
int test_vklite_barrier_image(TstSuite*);
int test_vklite_submit(TstSuite*);
int test_vklite_offscreen(TstSuite*);
int test_vklite_shader(TstSuite*);
int test_vklite_surface(TstSuite*);
int test_vklite_swapchain(TstSuite*);
int test_vklite_graphics(TstSuite*);
int test_vklite_gui(TstSuite*);

int test_vklite_canvas_blank(TstSuite*);
int test_vklite_canvas_triangle(TstSuite*);
int test_vklite_canvas_gui(TstSuite*);


#endif
