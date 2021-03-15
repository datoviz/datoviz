#include "../include/datoviz/common.h"
#include "../include/datoviz/vklite.h"
#include "../src/vklite_utils.h"
#include "tests.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

static const VkClearColorValue BACKGROUND = {{.4f, .6f, .8f, 1.0f}};

#define FORMAT VK_FORMAT_B8G8R8A8_UNORM



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct TestCanvas TestCanvas;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct TestCanvas
{
    DvzGpu* gpu;
    bool is_offscreen;

    DvzWindow* window;

    DvzRenderpass renderpass;
    DvzFramebuffers framebuffers;
    DvzSwapchain swapchain;

    DvzImages* images;
    DvzImages* depth;

    DvzCompute* compute;
    DvzBindings* bindings;
    DvzGraphics* graphics;
    DvzBufferRegions br;

    void* data;
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



static DvzRenderpass _renderpass(
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
        &renderpass, 1, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    dvz_renderpass_attachment_ops(
        &renderpass, 1, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE);

    // Subpass.
    dvz_renderpass_subpass_attachment(&renderpass, 0, 0);
    dvz_renderpass_subpass_attachment(&renderpass, 0, 1);
    dvz_renderpass_subpass_dependency(&renderpass, 0, VK_SUBPASS_EXTERNAL, 0);
    dvz_renderpass_subpass_dependency_stage(
        &renderpass, 0, //
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    dvz_renderpass_subpass_dependency_access(
        &renderpass, 0, 0,
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

    return renderpass;
}



static void
_depth_image(DvzImages* depth_images, DvzRenderpass* renderpass, uint32_t width, uint32_t height)
{
    // Depth attachment
    dvz_images_format(depth_images, renderpass->attachments[1].format);
    dvz_images_size(depth_images, width, height, 1);
    dvz_images_tiling(depth_images, VK_IMAGE_TILING_OPTIMAL);
    dvz_images_usage(depth_images, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    dvz_images_memory(depth_images, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    dvz_images_layout(depth_images, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    dvz_images_aspect(depth_images, VK_IMAGE_ASPECT_DEPTH_BIT);
    dvz_images_queue_access(depth_images, 0);
    dvz_images_create(depth_images);
}



static TestCanvas offscreen(DvzGpu* gpu)
{
    TestCanvas canvas = {0};
    canvas.gpu = gpu;
    canvas.is_offscreen = true;

    canvas.renderpass = _renderpass(gpu, BACKGROUND, FORMAT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    // Color attachment
    DvzImages images_struct = dvz_images(canvas.renderpass.gpu, VK_IMAGE_TYPE_2D, 1);
    DvzImages* images = (DvzImages*)calloc(1, sizeof(DvzImages));
    *images = images_struct;
    dvz_images_format(images, canvas.renderpass.attachments[0].format);
    dvz_images_size(images, WIDTH, HEIGHT, 1);
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
    *depth = depth_struct;
    _depth_image(depth, &canvas.renderpass, WIDTH, HEIGHT);
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
    dvz_images_size(staging, images->width, images->height, images->depth);
    dvz_images_tiling(staging, VK_IMAGE_TILING_LINEAR);
    dvz_images_usage(staging, VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    dvz_images_layout(staging, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    dvz_images_memory(
        staging, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
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
    void* rgb = calloc(images->width * images->height, 3 * bytes_per_component);
    dvz_images_download(staging, 0, bytes_per_component, true, false, rgb);

    dvz_images_destroy(staging);

    return rgb;
}



static void destroy_canvas(TestCanvas* canvas)
{
    log_trace("destroy canvas");
    if (canvas->is_offscreen)
    {
        dvz_images_destroy(canvas->images);
    }
    dvz_images_destroy(canvas->depth);

    dvz_renderpass_destroy(&canvas->renderpass);
    dvz_swapchain_destroy(&canvas->swapchain);
    dvz_framebuffers_destroy(&canvas->framebuffers);
    dvz_window_destroy(canvas->window);
}



/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

int test_vklite_app(TestContext* tc)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);

    AT(app->obj.status == DVZ_OBJECT_STATUS_CREATED);
    AT(app->gpus.count >= 1);
    AT(((DvzGpu*)(app->gpus.items[0]))->name != NULL);
    AT(((DvzGpu*)(app->gpus.items[0]))->obj.status == DVZ_OBJECT_STATUS_INIT);

    DvzGpu* gpu = dvz_gpu_best(app);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_TRANSFER);
    dvz_gpu_queue(gpu, 1, DVZ_QUEUE_GRAPHICS | DVZ_QUEUE_COMPUTE);
    dvz_gpu_queue(gpu, 2, DVZ_QUEUE_COMPUTE);
    dvz_gpu_create(gpu, 0);

    gpu = dvz_gpu_best(app);
    ASSERT(gpu != NULL);
    log_info("Best GPU is %s with %s VRAM", gpu->name, pretty_size(gpu->vram));
    ASSERT(gpu->name != NULL);

    dvz_app_destroy(app);
    return 0;
}



int test_vklite_commands(TestContext* tc)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu_best(app);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_create(gpu, 0);
    DvzCommands cmds = dvz_commands(gpu, 0, 3);
    dvz_cmd_begin(&cmds, 0);
    dvz_cmd_end(&cmds, 0);
    dvz_cmd_reset(&cmds, 0);
    dvz_cmd_free(&cmds);

    dvz_app_destroy(app);
    return 0;
}



int test_vklite_buffer_1(TestContext* tc)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu_best(app);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_create(gpu, 0);

    DvzBuffer buffer = dvz_buffer(gpu);
    const VkDeviceSize size = 256;
    dvz_buffer_size(&buffer, size);
    dvz_buffer_usage(&buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    dvz_buffer_memory(
        &buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    dvz_buffer_queue_access(&buffer, 0);
    dvz_buffer_create(&buffer);

    // Send some data to the GPU.
    uint8_t* data = calloc(size, 1);
    for (uint32_t i = 0; i < size; i++)
        data[i] = i;
    dvz_buffer_upload(&buffer, 0, size, data);

    // Recover the data.
    void* data2 = calloc(size, 1);
    dvz_buffer_download(&buffer, 0, size, data2);

    // Check that the data downloaded from the GPU is the same.
    AT(memcmp(data2, data, size) == 0);

    FREE(data);
    FREE(data2);

    dvz_buffer_destroy(&buffer);

    dvz_app_destroy(app);
    return 0;
}



int test_vklite_buffer_resize(TestContext* tc)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu_best(app);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_create(gpu, 0);

    DvzBuffer buffer = dvz_buffer(gpu);
    const VkDeviceSize size = 256;
    dvz_buffer_size(&buffer, size);
    dvz_buffer_usage(&buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    dvz_buffer_memory(
        &buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    dvz_buffer_queue_access(&buffer, 0);
    dvz_buffer_create(&buffer);

    // Map the buffer.
    buffer.mmap = dvz_buffer_map(&buffer, 0, VK_WHOLE_SIZE);

    // Send some data to the GPU.
    uint8_t* data = calloc(size, 1);
    for (uint32_t i = 0; i < size; i++)
        data[i] = i;
    dvz_buffer_upload(&buffer, 0, size, data);
    ASSERT(buffer.mmap != NULL);
    void* old_mmap = buffer.mmap;

    // Resize the buffer.
    DvzCommands cmds = dvz_commands(gpu, 0, 1);
    // NOTE: this should automatically unmap, delete, create, remap, copy old data to new.
    dvz_buffer_resize(&buffer, 2 * size, &cmds);
    ASSERT(buffer.size == 2 * size);
    ASSERT(buffer.mmap != NULL);
    ASSERT(buffer.mmap != old_mmap);

    // Recover the data.
    void* data2 = calloc(size, 1);
    dvz_buffer_download(&buffer, 0, size, data2);

    // Check that the data downloaded from the GPU is the same.
    AT(memcmp(data2, data, size) == 0);

    FREE(data);
    FREE(data2);

    // Unmap the buffer.
    dvz_buffer_unmap(&buffer);
    buffer.mmap = NULL;
    dvz_buffer_destroy(&buffer);

    dvz_app_destroy(app);
    return 0;
}



int test_vklite_compute(TestContext* tc)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu_best(app);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_COMPUTE);
    dvz_gpu_create(gpu, 0);

    // Create the compute pipeline.
    char path[1024];
    snprintf(path, sizeof(path), "%s/test_square.comp.spv", SPIRV_DIR);
    DvzCompute compute = dvz_compute(gpu, path);

    // Create the buffers
    DvzBuffer buffer = dvz_buffer(gpu);
    const uint32_t n = 20;
    const VkDeviceSize size = n * sizeof(float);
    dvz_buffer_size(&buffer, size);
    dvz_buffer_usage(
        &buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    dvz_buffer_memory(
        &buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    dvz_buffer_queue_access(&buffer, 0);
    dvz_buffer_create(&buffer);

    // Send some data to the GPU.
    float* data = calloc(n, sizeof(float));
    for (uint32_t i = 0; i < n; i++)
        data[i] = (float)i;
    dvz_buffer_upload(&buffer, 0, size, data);

    // Create the slots.
    dvz_compute_slot(&compute, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    // Create the bindings.
    DvzBindings bindings = dvz_bindings(&compute.slots, 1);
    DvzBufferRegions br = {.buffer = &buffer, .size = size, .count = 1};
    dvz_bindings_buffer(&bindings, 0, br);
    dvz_bindings_update(&bindings);

    // Link the bindings to the compute pipeline and create it.
    dvz_compute_bindings(&compute, &bindings);
    dvz_compute_create(&compute);

    // Command buffers.
    DvzCommands cmds = dvz_commands(gpu, 0, 1);
    dvz_cmd_begin(&cmds, 0);
    dvz_cmd_compute(&cmds, 0, &compute, (uvec3){20, 1, 1});
    dvz_cmd_end(&cmds, 0);
    dvz_cmd_submit_sync(&cmds, 0);

    // Get back the data.
    float* data2 = calloc(n, sizeof(float));
    dvz_buffer_download(&buffer, 0, size, data2);
    for (uint32_t i = 0; i < n; i++)
        AT(data2[i] == 2 * data[i]);

    dvz_bindings_destroy(&bindings);
    dvz_compute_destroy(&compute);
    dvz_buffer_destroy(&buffer);

    dvz_app_destroy(app);
    return 0;
}



int test_vklite_push(TestContext* tc)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu_best(app);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_COMPUTE);
    dvz_gpu_create(gpu, 0);

    // Create the compute pipeline.
    char path[1024];
    snprintf(path, sizeof(path), "%s/test_pow.comp.spv", SPIRV_DIR);
    DvzCompute compute = dvz_compute(gpu, path);

    // Create the buffers
    DvzBuffer buffer = dvz_buffer(gpu);
    const uint32_t n = 20;
    const VkDeviceSize size = n * sizeof(float);
    dvz_buffer_size(&buffer, size);
    dvz_buffer_usage(
        &buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    dvz_buffer_memory(
        &buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    dvz_buffer_queue_access(&buffer, 0);
    dvz_buffer_create(&buffer);

    // Send some data to the GPU.
    float* data = calloc(n, sizeof(float));
    for (uint32_t i = 0; i < n; i++)
        data[i] = (float)i;
    dvz_buffer_upload(&buffer, 0, size, data);

    // Create the slots.
    dvz_compute_slot(&compute, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    dvz_compute_push(&compute, 0, sizeof(float), VK_SHADER_STAGE_COMPUTE_BIT);

    // Create the bindings.
    DvzBindings bindings = dvz_bindings(&compute.slots, 1);
    DvzBufferRegions br = {.buffer = &buffer, .size = size, .count = 1};
    dvz_bindings_buffer(&bindings, 0, br);
    dvz_bindings_update(&bindings);

    // Link the bindings to the compute pipeline and create it.
    dvz_compute_bindings(&compute, &bindings);
    dvz_compute_create(&compute);

    // Command buffers.
    DvzCommands cmds = dvz_commands(gpu, 0, 1);
    dvz_cmd_begin(&cmds, 0);
    float power = 2.0f;
    dvz_cmd_push(&cmds, 0, &compute.slots, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float), &power);
    dvz_cmd_compute(&cmds, 0, &compute, (uvec3){20, 1, 1});
    dvz_cmd_end(&cmds, 0);
    dvz_cmd_submit_sync(&cmds, 0);

    // Get back the data.
    float* data2 = calloc(n, sizeof(float));
    dvz_buffer_download(&buffer, 0, size, data2);
    for (uint32_t i = 0; i < n; i++)
        AT(fabs(data2[i] - pow(data[i], power)) < .01);

    dvz_bindings_destroy(&bindings);
    dvz_compute_destroy(&compute);
    dvz_buffer_destroy(&buffer);

    dvz_app_destroy(app);
    return 0;
}



int test_vklite_images(TestContext* tc)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu_best(app);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_create(gpu, 0);

    DvzImages images = dvz_images(gpu, VK_IMAGE_TYPE_2D, 1);
    dvz_images_format(&images, VK_FORMAT_R8G8B8A8_UINT);
    dvz_images_size(&images, 16, 16, 1);
    dvz_images_tiling(&images, VK_IMAGE_TILING_OPTIMAL);
    dvz_images_usage(&images, VK_IMAGE_USAGE_STORAGE_BIT);
    dvz_images_memory(&images, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    dvz_images_queue_access(&images, 0);
    dvz_images_create(&images);

    dvz_images_destroy(&images);

    dvz_app_destroy(app);
    return 0;
}



int test_vklite_sampler(TestContext* tc)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu_best(app);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_create(gpu, 0);

    DvzSampler sampler = dvz_sampler(gpu);
    dvz_sampler_min_filter(&sampler, VK_FILTER_LINEAR);
    dvz_sampler_mag_filter(&sampler, VK_FILTER_LINEAR);
    dvz_sampler_address_mode(&sampler, DVZ_TEXTURE_AXIS_U, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    dvz_sampler_create(&sampler);

    dvz_sampler_destroy(&sampler);

    dvz_app_destroy(app);
    return 0;
}



int test_vklite_barrier(TestContext* tc)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu_best(app);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_create(gpu, 0);

    // Image.
    const uint32_t img_size = 16;
    DvzImages images = dvz_images(gpu, VK_IMAGE_TYPE_2D, 1);
    dvz_images_format(&images, VK_FORMAT_R8G8B8A8_UINT);
    dvz_images_size(&images, img_size, img_size, 1);
    dvz_images_tiling(&images, VK_IMAGE_TILING_OPTIMAL);
    dvz_images_usage(&images, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    dvz_images_memory(&images, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    dvz_images_queue_access(&images, 0);
    dvz_images_create(&images);

    // Staging buffer.
    DvzBuffer buffer = dvz_buffer(gpu);
    const VkDeviceSize size = img_size * img_size * 4;
    dvz_buffer_size(&buffer, size);
    dvz_buffer_usage(&buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    dvz_buffer_memory(
        &buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    dvz_buffer_queue_access(&buffer, 0);
    dvz_buffer_create(&buffer);

    // Send some data to the staging buffer.
    uint8_t* data = calloc(size, 1);
    for (uint32_t i = 0; i < size; i++)
        data[i] = i % 256;
    dvz_buffer_upload(&buffer, 0, size, data);

    // Image transition.
    DvzBarrier barrier = dvz_barrier(gpu);
    dvz_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    dvz_barrier_images(&barrier, &images);
    dvz_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Transfer the data from the staging buffer to the image.
    DvzCommands cmds = dvz_commands(gpu, 0, 1);
    dvz_cmd_begin(&cmds, 0);
    dvz_cmd_barrier(&cmds, 0, &barrier);
    dvz_cmd_copy_buffer_to_image(&cmds, 0, &buffer, &images);
    dvz_cmd_end(&cmds, 0);
    dvz_cmd_submit_sync(&cmds, 0);

    dvz_buffer_destroy(&buffer);
    dvz_images_destroy(&images);

    dvz_app_destroy(app);
    return 0;
}



int test_vklite_submit(TestContext* tc)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu_best(app);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_COMPUTE);
    dvz_gpu_queue(gpu, 1, DVZ_QUEUE_COMPUTE);
    dvz_gpu_create(gpu, 0);

    // Create the compute pipeline.
    char path[1024];
    snprintf(path, sizeof(path), "%s/test_square.comp.spv", SPIRV_DIR);
    DvzCompute compute1 = dvz_compute(gpu, path);

    snprintf(path, sizeof(path), "%s/test_sum.comp.spv", SPIRV_DIR);
    DvzCompute compute2 = dvz_compute(gpu, path);

    // Create the buffer
    DvzBuffer buffer = dvz_buffer(gpu);
    const uint32_t n = 20;
    const VkDeviceSize size = n * sizeof(float);
    dvz_buffer_size(&buffer, size);
    dvz_buffer_usage(
        &buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    dvz_buffer_memory(
        &buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    dvz_buffer_queue_access(&buffer, 0);
    dvz_buffer_queue_access(&buffer, 1);
    dvz_buffer_create(&buffer);

    // Send some data to the GPU.
    float* data = calloc(n, sizeof(float));
    for (uint32_t i = 0; i < n; i++)
        data[i] = (float)i;
    dvz_buffer_upload(&buffer, 0, size, data);

    // Create the slots.
    dvz_compute_slot(&compute1, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    dvz_compute_slot(&compute2, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    // Create the bindings.
    DvzBindings bindings1 = dvz_bindings(&compute1.slots, 1);
    DvzBufferRegions br1 = {.buffer = &buffer, .size = size, .count = 1};
    dvz_bindings_buffer(&bindings1, 0, br1);
    dvz_bindings_update(&bindings1);

    DvzBindings bindings2 = dvz_bindings(&compute2.slots, 1);
    DvzBufferRegions br2 = {.buffer = &buffer, .size = size, .count = 1};
    dvz_bindings_buffer(&bindings2, 0, br2);
    dvz_bindings_update(&bindings2);

    // Link the bindings1 to the compute1 pipeline and create it.
    dvz_compute_bindings(&compute1, &bindings1);
    dvz_compute_create(&compute1);

    // Link the bindings1 to the compute2 pipeline and create it.
    dvz_compute_bindings(&compute2, &bindings2);
    dvz_compute_create(&compute2);

    // Command buffers.
    DvzCommands cmds1 = dvz_commands(gpu, 0, 1);
    dvz_cmd_begin(&cmds1, 0);
    dvz_cmd_compute(&cmds1, 0, &compute1, (uvec3){20, 1, 1});
    dvz_cmd_end(&cmds1, 0);

    DvzCommands cmds2 = dvz_commands(gpu, 0, 1);
    dvz_cmd_begin(&cmds2, 0);
    dvz_cmd_compute(&cmds2, 0, &compute2, (uvec3){20, 1, 1});
    dvz_cmd_end(&cmds2, 0);

    // Semaphores
    DvzSemaphores semaphores = dvz_semaphores(gpu, 1);

    // Submit.
    DvzSubmit submit1 = dvz_submit(gpu);
    dvz_submit_commands(&submit1, &cmds1);
    dvz_submit_signal_semaphores(&submit1, &semaphores, 0);
    dvz_submit_send(&submit1, 0, NULL, 0);

    DvzSubmit submit2 = dvz_submit(gpu);
    dvz_submit_commands(&submit2, &cmds2);
    dvz_submit_wait_semaphores(&submit2, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, &semaphores, 0);
    dvz_submit_send(&submit2, 0, NULL, 0);

    dvz_gpu_wait(gpu);

    // Get back the data.
    float* data2 = calloc(n, sizeof(float));
    dvz_buffer_download(&buffer, 0, size, data2);
    for (uint32_t i = 0; i < n; i++)
        AT(data2[i] == 2 * i + 1);


    dvz_semaphores_destroy(&semaphores);
    dvz_bindings_destroy(&bindings1);
    dvz_bindings_destroy(&bindings2);
    dvz_buffer_destroy(&buffer);
    dvz_compute_destroy(&compute1);
    dvz_compute_destroy(&compute2);

    dvz_app_destroy(app);
    return 0;
}



int test_vklite_offscreen(TestContext* tc)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu_best(app);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_create(gpu, 0);

    TestCanvas canvas = offscreen(gpu);
    DvzFramebuffers* framebuffers = &canvas.framebuffers;

    DvzCommands cmds = dvz_commands(gpu, 0, 1);
    empty_commands(&canvas, &cmds, 0);
    dvz_cmd_submit_sync(&cmds, 0);

    uint8_t* rgba = screenshot(framebuffers->attachments[0], 1);

    for (uint32_t i = 0; i < WIDTH * HEIGHT * 3; i++)
        AT(rgba[i] >= 100);

    FREE(rgba);

    destroy_canvas(&canvas);

    dvz_app_destroy(app);
    return 0;
}



/*************************************************************************************************/
/*  Tests with window                                                                            */
/*************************************************************************************************/

int test_vklite_surface(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu_best(app);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_ALL);

    // Create a GLFW window and surface.
    VkSurfaceKHR surface = 0;
    GLFWwindow* window =
        (GLFWwindow*)backend_window(app->instance, DVZ_BACKEND_GLFW, 100, 100, NULL, &surface);
    dvz_gpu_create(gpu, surface);

    backend_window_destroy(app->instance, DVZ_BACKEND_GLFW, window, surface);

    dvz_app_destroy(app);
    return 0;
}



int test_vklite_window(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzWindow* window = dvz_window(app, 100, 100);
    AT(window != NULL);
    AT(window->app != NULL);

    DvzWindow* window2 = dvz_window(app, 100, 100);
    AT(window2 != NULL);
    AT(window2->app != NULL);

    dvz_app_destroy(app);
    return 0;
}



int test_vklite_swapchain(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzWindow* window = dvz_window(app, 100, 100);
    DvzGpu* gpu = dvz_gpu_best(app);
    dvz_gpu_queue(gpu, 0, DVZ_QUEUE_RENDER);
    dvz_gpu_queue(gpu, 1, DVZ_QUEUE_PRESENT);
    dvz_gpu_create(gpu, window->surface);

    DvzSwapchain swapchain = dvz_swapchain(gpu, window, 3);
    dvz_swapchain_format(&swapchain, VK_FORMAT_B8G8R8A8_UNORM);
    dvz_swapchain_present_mode(&swapchain, VK_PRESENT_MODE_FIFO_KHR);
    dvz_swapchain_create(&swapchain);
    dvz_swapchain_destroy(&swapchain);
    dvz_window_destroy(window);

    dvz_app_destroy(app);
    return 0;
}
