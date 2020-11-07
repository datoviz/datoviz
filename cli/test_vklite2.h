#include "../src/vklite2_utils.h"



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

    vkl_app_destroy(app);
    return 0;
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
    vkl_app_destroy(app);
    return 0;
}

static int vklite2_window(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklWindow* window = vkl_window(app, 100, 100);
    ASSERT(window != NULL);
    vkl_app_destroy(app);
    return 0;
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
    vkl_app_destroy(app);
    return 0;
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
    vkl_app_destroy(app);
    return 0;
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
    VklBufferRegions br = {0};
    br.buffer = buffer;
    br.count = 1;
    br.offsets[0] = 0;
    br.sizes[0] = size;
    void* mapped = vkl_buffer_regions_map(&br, 0);
    ASSERT(mapped != NULL);
    uint8_t* data = calloc(size, 1);
    for (uint32_t i = 0; i < size; i++)
        data[i] = i;
    memcpy(mapped, data, size);
    vkl_buffer_regions_unmap(&br, 0);

    // Recover the data.
    mapped = vkl_buffer_regions_map(&br, 0);
    void* data2 = calloc(size, 1);
    memcpy(data2, mapped, size);
    vkl_buffer_regions_unmap(&br, 0);

    // Check that the data downloaded from the GPU is the same.
    ASSERT(memcmp(data2, data, size) == 0);

    FREE(data);
    vkl_app_destroy(app);
    return 0;
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
    VklBufferRegions br = {0};
    br.buffer = buffer;
    br.count = 1;
    br.offsets[0] = 0;
    br.sizes[0] = size;
    void* mapped = vkl_buffer_regions_map(&br, 0);
    ASSERT(mapped != NULL);
    float* data = calloc(n, sizeof(float));
    for (uint32_t i = 0; i < n; i++)
        data[i] = (float)i;
    memcpy(mapped, data, size);
    vkl_buffer_regions_unmap(&br, 0);

    // Create the bindings.
    VklBindings* bindings = vkl_bindings(gpu);
    vkl_bindings_slot(bindings, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    vkl_bindings_create(bindings, 1);
    vkl_bindings_buffer(bindings, 0, &br);

    // TODO: should be called automatically and transparently. The implementation should keep track
    // of binding changes and update the bindings before cmd dset binding.
    // For now in this test, calling this manually.
    update_bindings(bindings);

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
    mapped = vkl_buffer_regions_map(&br, 0);
    float* data2 = calloc(n, sizeof(float));
    memcpy(data2, mapped, size);
    vkl_buffer_regions_unmap(&br, 0);
    for (uint32_t i = 0; i < n; i++)
        ASSERT(data2[i] == 2 * data[i]);

    vkl_app_destroy(app);
    return 0;
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
