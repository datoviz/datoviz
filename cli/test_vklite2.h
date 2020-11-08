#include "../src/vklite2_utils.h"



#define TEST_END                                                                                  \
    vkl_app_destroy(app);                                                                         \
    return n_errors != 0;



static int vklite2_app(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    ASSERT(app->obj.status == VKL_OBJECT_STATUS_CREATED);
    ASSERT(app->gpu_count >= 1);
    ASSERT(app->gpus[0].name != NULL);
    ASSERT(app->gpus[0].obj.status == VKL_OBJECT_STATUS_INIT);

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
        (GLFWwindow*)backend_window(app->instance, VKL_BACKEND_GLFW, 100, 100, &surface);
    vkl_gpu_create(gpu, surface);

    backend_window_destroy(app->instance, VKL_BACKEND_GLFW, window, surface);

    TEST_END
}

static int vklite2_window(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklWindow* window = vkl_window(app, 100, 100);
    ASSERT(window != NULL);

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
    VklSwapchain* swapchain = vkl_swapchain(gpu, window, 3);
    vkl_swapchain_create(swapchain, VK_FORMAT_B8G8R8A8_UNORM, VK_PRESENT_MODE_FIFO_KHR);
    vkl_swapchain_destroy(swapchain);
    vkl_window_destroy(window);

    TEST_END
}

static int vklite2_commands(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_RENDER, 0);
    vkl_gpu_create(gpu, 0);
    VklCommands* commands = vkl_commands(gpu, 0, 3);
    vkl_cmd_begin(commands);
    vkl_cmd_end(commands);
    vkl_cmd_reset(commands);
    vkl_cmd_free(commands);

    TEST_END
}

static int vklite2_buffer(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_RENDER, 0);
    vkl_gpu_create(gpu, 0);

    VklBuffer* buffer = vkl_buffer(gpu);
    const VkDeviceSize size = 256;
    vkl_buffer_size(buffer, size, 0);
    vkl_buffer_usage(buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    vkl_buffer_memory(
        buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkl_buffer_queue_access(buffer, 0);
    vkl_buffer_create(buffer);

    // Send some data to the GPU.
    uint8_t* data = calloc(size, 1);
    for (uint32_t i = 0; i < size; i++)
        data[i] = i;
    vkl_buffer_upload(buffer, 0, size, data);

    // Recover the data.
    void* data2 = calloc(size, 1);
    vkl_buffer_download(buffer, 0, size, data2);

    // Check that the data downloaded from the GPU is the same.
    ASSERT(memcmp(data2, data, size) == 0);

    FREE(data);
    FREE(data2);

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
    snprintf(path, sizeof(path), "%s/spirv/pow2.comp.spv", DATA_DIR);
    VklCompute* compute = vkl_compute(gpu, path);

    // Create the buffers
    VklBuffer* buffer = vkl_buffer(gpu);
    const uint32_t n = 20;
    const VkDeviceSize size = n * sizeof(float);
    vkl_buffer_size(buffer, size, 0);
    vkl_buffer_usage(
        buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    vkl_buffer_memory(
        buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkl_buffer_queue_access(buffer, 0);
    vkl_buffer_create(buffer);

    // Send some data to the GPU.
    float* data = calloc(n, sizeof(float));
    for (uint32_t i = 0; i < n; i++)
        data[i] = (float)i;
    vkl_buffer_upload(buffer, 0, size, data);

    // Create the bindings.
    VklBindings* bindings = vkl_bindings(gpu);
    vkl_bindings_slot(bindings, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    vkl_bindings_create(bindings, 1);
    VklBufferRegions br = {.buffer = buffer, .size = size, .count = 1};
    vkl_bindings_buffer(bindings, 0, &br);

    vkl_bindings_update(bindings);

    // Link the bindings to the compute pipeline and create it.
    vkl_compute_bindings(compute, bindings);
    vkl_compute_create(compute);

    // Command buffers.
    VklCommands* cmds = vkl_commands(gpu, 0, 1);
    vkl_cmd_begin(cmds);
    vkl_cmd_compute(cmds, compute, (uvec3){20, 1, 1});
    vkl_cmd_end(cmds);
    vkl_cmd_submit_sync(cmds, 0);

    // Get back the data.
    float* data2 = calloc(n, sizeof(float));
    vkl_buffer_download(buffer, 0, size, data2);
    for (uint32_t i = 0; i < n; i++)
        ASSERT(data2[i] == 2 * data[i]);

    TEST_END
}

static int vklite2_images(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_RENDER, 0);
    vkl_gpu_create(gpu, 0);

    VklImages* images = vkl_images(gpu, VK_IMAGE_TYPE_2D, 1);
    vkl_images_format(images, VK_FORMAT_R8G8B8A8_UINT);
    vkl_images_size(images, 16, 16, 1);
    vkl_images_tiling(images, VK_IMAGE_TILING_OPTIMAL);
    vkl_images_usage(images, VK_IMAGE_USAGE_STORAGE_BIT);
    vkl_images_memory(images, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkl_images_queue_access(images, 0);
    vkl_images_create(images);

    TEST_END
}

static int vklite2_sampler(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_RENDER, 0);
    vkl_gpu_create(gpu, 0);

    VklSampler* sampler = vkl_sampler(gpu);
    vkl_sampler_min_filter(sampler, VK_FILTER_LINEAR);
    vkl_sampler_mag_filter(sampler, VK_FILTER_LINEAR);
    vkl_sampler_address_mode(sampler, VKL_TEXTURE_AXIS_U, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    vkl_sampler_create(sampler);

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
    VklImages* images = vkl_images(gpu, VK_IMAGE_TYPE_2D, 1);
    vkl_images_format(images, VK_FORMAT_R8G8B8A8_UINT);
    vkl_images_size(images, img_size, img_size, 1);
    vkl_images_tiling(images, VK_IMAGE_TILING_OPTIMAL);
    vkl_images_usage(images, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    vkl_images_memory(images, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkl_images_queue_access(images, 0);
    vkl_images_create(images);

    // Staging buffer.
    VklBuffer* buffer = vkl_buffer(gpu);
    const VkDeviceSize size = img_size * img_size * 4;
    vkl_buffer_size(buffer, size, 0);
    vkl_buffer_usage(buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    vkl_buffer_memory(
        buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkl_buffer_queue_access(buffer, 0);
    vkl_buffer_create(buffer);

    // Send some data to the staging buffer.
    uint8_t* data = calloc(size, 1);
    for (uint32_t i = 0; i < size; i++)
        data[i] = i % 256;
    vkl_buffer_upload(buffer, 0, size, data);

    // Image transition.
    VklBarrier barrier = vkl_barrier(gpu);
    vkl_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    vkl_barrier_images(&barrier, images);
    vkl_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Transfer the data from the staging buffer to the image.
    VklCommands* cmds = vkl_commands(gpu, 0, 1);
    vkl_cmd_begin(cmds);
    vkl_cmd_barrier(cmds, &barrier);
    vkl_cmd_copy_buffer_to_image(cmds, buffer, images);
    vkl_cmd_end(cmds);
    vkl_cmd_submit_sync(cmds, 0);

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
    snprintf(path, sizeof(path), "%s/spirv/pow2.comp.spv", DATA_DIR);
    VklCompute* compute1 = vkl_compute(gpu, path);

    snprintf(path, sizeof(path), "%s/spirv/sum.comp.spv", DATA_DIR);
    VklCompute* compute2 = vkl_compute(gpu, path);

    // Create the buffer
    VklBuffer* buffer = vkl_buffer(gpu);
    const uint32_t n = 20;
    const VkDeviceSize size = n * sizeof(float);
    vkl_buffer_size(buffer, size, 0);
    vkl_buffer_usage(
        buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    vkl_buffer_memory(
        buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkl_buffer_queue_access(buffer, 0);
    vkl_buffer_queue_access(buffer, 1);
    vkl_buffer_create(buffer);

    // Send some data to the GPU.
    float* data = calloc(n, sizeof(float));
    for (uint32_t i = 0; i < n; i++)
        data[i] = (float)i;
    vkl_buffer_upload(buffer, 0, size, data);

    // Create the bindings.
    VklBindings* bindings1 = vkl_bindings(gpu);
    vkl_bindings_slot(bindings1, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    vkl_bindings_create(bindings1, 1);
    VklBufferRegions br1 = {.buffer = buffer, .size = size, .count = 1};
    vkl_bindings_buffer(bindings1, 0, &br1);

    VklBindings* bindings2 = vkl_bindings(gpu);
    vkl_bindings_slot(bindings2, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    vkl_bindings_create(bindings2, 1);
    VklBufferRegions br2 = {.buffer = buffer, .size = size, .count = 1};
    vkl_bindings_buffer(bindings2, 0, &br2);

    vkl_bindings_update(bindings1);
    vkl_bindings_update(bindings2);

    // Link the bindings1 to the compute1 pipeline and create it.
    vkl_compute_bindings(compute1, bindings1);
    vkl_compute_create(compute1);

    // Link the bindings1 to the compute2 pipeline and create it.
    vkl_compute_bindings(compute2, bindings2);
    vkl_compute_create(compute2);

    // Command buffers.
    VklCommands* cmds1 = vkl_commands(gpu, 0, 1);
    vkl_cmd_begin(cmds1);
    vkl_cmd_compute(cmds1, compute1, (uvec3){20, 1, 1});
    vkl_cmd_end(cmds1);

    VklCommands* cmds2 = vkl_commands(gpu, 0, 1);
    vkl_cmd_begin(cmds2);
    vkl_cmd_compute(cmds2, compute2, (uvec3){20, 1, 1});
    vkl_cmd_end(cmds2);

    // Semaphores
    VklSemaphores* semaphores = vkl_semaphores(gpu, 1);

    // Submit.
    VklSubmit submit1 = vkl_submit(gpu);
    vkl_submit_commands(&submit1, cmds1, 0);
    vkl_submit_signal_semaphores(&submit1, semaphores, 0);
    vkl_submit_send(&submit1, 0, NULL, 0);

    VklSubmit submit2 = vkl_submit(gpu);
    vkl_submit_commands(&submit2, cmds2, 0);
    vkl_submit_wait_semaphores(&submit2, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, semaphores, 0);
    vkl_submit_send(&submit2, 0, NULL, 0);

    vkl_gpu_wait(gpu);

    // Get back the data.
    float* data2 = calloc(n, sizeof(float));
    vkl_buffer_download(buffer, 0, size, data2);
    for (uint32_t i = 0; i < n; i++)
        ASSERT(data2[i] == 2 * i + 1);

    TEST_END
}



static int vklite2_test_compute_only(VkyTestContext* context)
{
    // VkyApp* app = vky_app();
    // VkyCompute* compute = vky_compute(app->gpu, "compute.spv");
    // VkyBuffer* buffer =
    // VkyCommands* commands = vky_commands(app->gpu, VKY_COMMAND_COMPUTE);
    // vky_cmd_begin(commands);
    // vky_cmd_compute(commands, compute, uvec3 size);
    // vky_cmd_end(commands);
    // VkySubmit* sub = vky_submit(app-> gpu, VKY_QUEUE_COMPUTE);
    // vky_submit_send(sub, NULL);
    // vky_app_destroy(app);
    return 0;
}

static int vklite2_test_offscreen(VkyTestContext* context) { return 0; }

static int vklite2_test_offscreen_gui(VkyTestContext* context) { return 0; }

static int vklite2_test_offscreen_compute(VkyTestContext* context) { return 0; }
