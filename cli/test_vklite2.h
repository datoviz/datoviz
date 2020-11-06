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

static int vklite2_buffers(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_RENDER, 0);
    vkl_gpu_create(gpu, 0);
    VklBuffers* buffers = vkl_buffers(gpu, 3);
    const VkDeviceSize size = 256;
    vkl_buffers_size(buffers, size, 0);
    vkl_buffers_usage(
        buffers, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    vkl_buffers_memory(
        buffers, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkl_buffers_queue_access(buffers, 0);
    vkl_buffers_create(buffers);

    ASSERT(vkl_buffers_region(buffers, 0, 0, size).buffers == buffers);

    // Send some data to the GPU.
    void* buffer = vkl_buffers_map(buffers, 0, 0, size);
    ASSERT(buffer != NULL);
    void* data = calloc(size, 1);
    for (uint32_t i = 0; i < size; i++)
        ((uint8_t*)data)[i] = i;
    memcpy(buffer, data, size);
    vkl_buffers_unmap(buffers, 0);

    // Recover the data.
    buffer = vkl_buffers_map(buffers, 0, 0, size);
    void* data2 = calloc(size, 1);
    memcpy(data2, buffer, size);
    vkl_buffers_unmap(buffers, 0);

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

    char path[1024];
    snprintf(path, sizeof(path), "%s/spirv/pow2.comp.spv", DATA_DIR);
    VklCompute* compute = vkl_compute(gpu, path);
    vkl_compute_create(compute);

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
