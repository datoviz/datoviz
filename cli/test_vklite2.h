#include "../src/vklite2_utils.h"



static int vklite2_app(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    ASSERT(app->obj.status == VKL_OBJECT_STATUS_CREATED);
    ASSERT(app->gpu_count >= 1);
    ASSERT(app->gpus[0]->name != NULL);
    ASSERT(app->gpus[0]->obj.status == VKL_OBJECT_STATUS_INIT);

    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_create(gpu, 0);

    vkl_app_destroy(app);
    return 0;
}

static int vklite2_surface(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);

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
    // vkl_window_destroy(window);
    vkl_app_destroy(app);
    return 0;
}

static int vklite2_swapchain(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklWindow* window = vkl_window(app, 100, 100);
    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_create(gpu, window->surface);
    VklSwapchain* swapchain = vkl_swapchain(gpu, window, 3);
    vkl_swapchain_create(swapchain, VK_FORMAT_B8G8R8A8_UNORM, VK_PRESENT_MODE_FIFO_KHR);
    vkl_swapchain_destroy(swapchain);

    // HACK: this function should never be called manually, because the GPU keeps a reference
    // to the destroyed pointer and will try to destroy again the window (which will cause
    // a segfault)
    app->windows[0] = NULL;
    vkl_window_destroy(window);

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
