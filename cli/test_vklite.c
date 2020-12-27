#include "test_vklite.h"
#include "../include/visky/context.h"
#include "../src/spirv.h"
#include "../src/vklite_utils.h"
#include "utils.h"



/*************************************************************************************************/
/*  vklite2                                                                                      */
/*************************************************************************************************/

int test_vklite_app(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    AT(app->obj.status == VKL_OBJECT_STATUS_CREATED);
    AT(app->gpus.count >= 1);
    AT(((VklGpu*)(app->gpus.items[0]))->name != NULL);
    AT(((VklGpu*)(app->gpus.items[0]))->obj.status == VKL_OBJECT_STATUS_INIT);

    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_TRANSFER, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_GRAPHICS | VKL_QUEUE_COMPUTE, 1);
    vkl_gpu_queue(gpu, VKL_QUEUE_COMPUTE, 2);
    vkl_gpu_create(gpu, 0);

    TEST_END
}



int test_vklite_surface(TestContext* context)
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



int test_vklite_window(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklWindow* window = vkl_window(app, 100, 100);
    AT(window != NULL);
    AT(window->app != NULL);

    VklWindow* window2 = vkl_window(app, 100, 100);
    AT(window2 != NULL);
    AT(window2->app != NULL);

    TEST_END
}



int test_vklite_swapchain(TestContext* context)
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



int test_vklite_commands(TestContext* context)
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



int test_vklite_buffer(TestContext* context)
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



int test_vklite_compute(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_COMPUTE, 0);
    vkl_gpu_create(gpu, 0);

    // Create the compute pipeline.
    char path[1024];
    snprintf(path, sizeof(path), "%s/test_square.comp.spv", SPIRV_DIR);
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
    vkl_compute_slot(&compute, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    // Create the bindings.
    VklBindings bindings = vkl_bindings(&compute.slots, 1);
    VklBufferRegions br = {.buffer = &buffer, .size = size, .count = 1};
    vkl_bindings_buffer(&bindings, 0, br);
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

    vkl_bindings_destroy(&bindings);
    vkl_compute_destroy(&compute);
    vkl_buffer_destroy(&buffer);

    TEST_END
}



int test_vklite_push(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_COMPUTE, 0);
    vkl_gpu_create(gpu, 0);

    // Create the compute pipeline.
    char path[1024];
    snprintf(path, sizeof(path), "%s/test_pow.comp.spv", SPIRV_DIR);
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
    vkl_compute_slot(&compute, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    vkl_compute_push(&compute, 0, sizeof(float), VK_SHADER_STAGE_COMPUTE_BIT);

    // Create the bindings.
    VklBindings bindings = vkl_bindings(&compute.slots, 1);
    VklBufferRegions br = {.buffer = &buffer, .size = size, .count = 1};
    vkl_bindings_buffer(&bindings, 0, br);
    vkl_bindings_update(&bindings);

    // Link the bindings to the compute pipeline and create it.
    vkl_compute_bindings(&compute, &bindings);
    vkl_compute_create(&compute);

    // Command buffers.
    VklCommands cmds = vkl_commands(gpu, 0, 1);
    vkl_cmd_begin(&cmds, 0);
    float power = 2.0f;
    vkl_cmd_push(&cmds, 0, &compute.slots, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(float), &power);
    vkl_cmd_compute(&cmds, 0, &compute, (uvec3){20, 1, 1});
    vkl_cmd_end(&cmds, 0);
    vkl_cmd_submit_sync(&cmds, 0);

    // Get back the data.
    float* data2 = calloc(n, sizeof(float));
    vkl_buffer_download(&buffer, 0, size, data2);
    for (uint32_t i = 0; i < n; i++)
        AT(fabs(data2[i] - pow(data[i], power)) < .01);

    vkl_bindings_destroy(&bindings);
    vkl_compute_destroy(&compute);
    vkl_buffer_destroy(&buffer);

    TEST_END
}



int test_vklite_images(TestContext* context)
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



int test_vklite_sampler(TestContext* context)
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



int test_vklite_barrier(TestContext* context)
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



int test_vklite_submit(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_COMPUTE, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_COMPUTE, 1);
    vkl_gpu_create(gpu, 0);

    // Create the compute pipeline.
    char path[1024];
    snprintf(path, sizeof(path), "%s/test_square.comp.spv", SPIRV_DIR);
    VklCompute compute1 = vkl_compute(gpu, path);

    snprintf(path, sizeof(path), "%s/test_sum.comp.spv", SPIRV_DIR);
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
    vkl_compute_slot(&compute1, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    vkl_compute_slot(&compute2, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    // Create the bindings.
    VklBindings bindings1 = vkl_bindings(&compute1.slots, 1);
    VklBufferRegions br1 = {.buffer = &buffer, .size = size, .count = 1};
    vkl_bindings_buffer(&bindings1, 0, br1);
    vkl_bindings_update(&bindings1);

    VklBindings bindings2 = vkl_bindings(&compute2.slots, 1);
    VklBufferRegions br2 = {.buffer = &buffer, .size = size, .count = 1};
    vkl_bindings_buffer(&bindings2, 0, br2);
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
    vkl_bindings_destroy(&bindings1);
    vkl_bindings_destroy(&bindings2);
    vkl_buffer_destroy(&buffer);
    vkl_compute_destroy(&compute1);
    vkl_compute_destroy(&compute2);

    TEST_END
}



int test_vklite_blank(TestContext* context)
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



static void _make_triangle(TestCanvas* canvas, TestVisual* visual)
{
    visual->gpu = canvas->gpu;
    visual->renderpass = &canvas->renderpass;
    visual->framebuffers = &canvas->framebuffers;
    test_triangle(visual, "");
    canvas->data = visual;
    canvas->br = &visual->br;
    canvas->graphics = &visual->graphics;
    canvas->bindings = &visual->bindings;
}



int test_vklite_graphics(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_RENDER, 0);
    vkl_gpu_create(gpu, 0);

    TestCanvas canvas = offscreen(gpu);
    TestVisual visual = {0};
    _make_triangle(&canvas, &visual);

    VklCommands cmds = vkl_commands(gpu, 0, 1);
    triangle_commands(&canvas, &cmds, 0);
    vkl_cmd_submit_sync(&cmds, 0);

    save_screenshot(visual.framebuffers, "screenshot.ppm");

    destroy_visual(&visual);
    destroy_canvas(&canvas);
    TEST_END
}



int test_basic_canvas_1(TestContext* context)
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



int test_basic_canvas_triangle(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);

    VklWindow* window = vkl_window(app, TEST_WIDTH, TEST_HEIGHT);

    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_RENDER, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_PRESENT, 1);
    vkl_gpu_create(gpu, window->surface);

    TestCanvas canvas = glfw_canvas(gpu, window);
    TestVisual visual = {0};
    _make_triangle(&canvas, &visual);

    show_canvas(canvas, triangle_commands, 10);

    destroy_visual(&visual);
    destroy_canvas(&canvas);

    TEST_END
}



int test_shader_compile(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_OFFSCREEN);

    VklGpu* gpu = vkl_gpu(app, 0);
    vkl_gpu_queue(gpu, VKL_QUEUE_RENDER, 0);
    vkl_gpu_create(gpu, VK_NULL_HANDLE);

    VkShaderModule module = vkl_shader_compile(
        gpu,
        "#version 450\n"
        "layout (location = 0) in vec3 pos;\n"
        "layout (location = 1) in vec4 color;\n"
        "layout (location = 0) out vec4 out_color;\n"
        "void main() {\n"
        "    gl_Position = vec4(pos, 1.0);\n"
        "    out_color = color;\n"
        "}",
        VK_SHADER_STAGE_VERTEX_BIT);
    vkDestroyShaderModule(gpu->device, module, NULL);

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



int test_fifo(TestContext* context)
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

int test_context_buffer(TestContext* context)
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
    VklBufferRegions br = vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_COUNT, 3, 64);
    AT(br.count == 3);
    AT(br.offsets[0] == 0);
    AT(br.offsets[1] == 64);
    AT(br.offsets[2] == 128);
    AT(br.size == 64);
    AT(buffer->size == 256);

    // This allocation will trigger a buffer resize.
    br = vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_COUNT, 2, 64);
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



int test_context_texture(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklContext* ctx = vkl_context(gpu, NULL);

    vkl_ctx_texture(ctx, 2, (uvec3){16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM);

    TEST_END
}



int test_context_transfer_sync(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklContext* ctx = vkl_context(gpu, NULL);

    VklBufferRegions br = vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_VERTEX, 1, 16);
    uint8_t data[16] = {0};
    memset(data, 12, 16);
    vkl_upload_buffers(ctx, br, 0, 16, data);

    uint8_t data2[16] = {0};
    vkl_download_buffers(ctx, br, 0, 16, data2);
    AT(memcmp(data, data2, 16) == 0);

    uint8_t* img_data = calloc(16 * 16 * 4, sizeof(uint8_t));
    VklTexture* tex = vkl_ctx_texture(ctx, 2, (uvec3){16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM);
    vkl_upload_texture(ctx, tex, 16 * 16 * 4, img_data);

    uint8_t* img_data2 = calloc(16 * 16 * 4, sizeof(uint8_t));
    vkl_download_texture(ctx, tex, 16 * 16 * 4, img_data2);
    AT(memcmp(img_data, img_data2, 16 * 16 * 4) == 0);

    FREE(img_data2);
    FREE(img_data);
    TEST_END
}



int test_context_copy(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklContext* ctx = vkl_context(gpu, NULL);


    // Upload.
    VklBufferRegions br = vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_VERTEX, 1, 16);
    uint8_t data[16] = {0};
    memset(data, 12, 16);
    vkl_upload_buffers(ctx, br, 0, 16, data);

    uint8_t* img_data = calloc(16 * 16 * 4, sizeof(uint8_t));
    memset(img_data, 12, 16);
    VklTexture* tex = vkl_ctx_texture(ctx, 2, (uvec3){16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM);
    vkl_upload_texture(ctx, tex, 16 * 16 * 4, img_data);


    // Copy
    VklBufferRegions br2 = vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_VERTEX, 1, 16);
    vkl_copy_buffers(ctx, br, 0, br2, 0, 16);

    VklTexture* tex2 = vkl_ctx_texture(ctx, 2, (uvec3){16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM);
    vkl_copy_textures(ctx, tex, (uvec3){0}, tex2, (uvec3){0}, (uvec3){16, 16, 1});


    // Download.
    uint8_t data2[16] = {0};
    vkl_download_buffers(ctx, br2, 0, 16, data2);
    AT(memcmp(data, data2, 16) == 0);

    uint8_t* img_data2 = calloc(16 * 16 * 4, sizeof(uint8_t));
    vkl_download_texture(ctx, tex2, 16 * 16 * 4, img_data2);
    AT(memcmp(img_data, img_data2, 16 * 16 * 4) == 0);


    FREE(img_data2);
    FREE(img_data);
    TEST_END
}



int test_context_transfer_async_nothread(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklContext* ctx = vkl_context(gpu, NULL);
    vkl_transfer_mode(ctx, VKL_TRANSFER_MODE_ASYNC);

    VklBufferRegions br = vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_VERTEX, 1, 16);
    uint8_t data[16] = {0};
    memset(data, 12, 16);
    vkl_upload_buffers(ctx, br, 0, 16, data);
    vkl_transfer_loop(ctx, false);

    uint8_t data2[16] = {0};
    vkl_download_buffers(ctx, br, 0, 16, data2);
    vkl_transfer_loop(ctx, false);
    AT(memcmp(data, data2, 16) == 0);

    TEST_END
}



typedef struct TestTransfer TestTransfer;
struct TestTransfer
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
    TestTransfer* tt = arg;

    // Upload data from a background thread.
    vkl_upload_buffers(tt->ctx, tt->br, 0, 16, tt->data);
    vkl_upload_texture(tt->ctx, tt->tex, 16 * 16 * 4, tt->img_data);

    // Cause the transfer loop to end.
    vkl_transfer_stop(tt->ctx);

    return NULL;
}

int test_context_transfer_async_thread(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklContext* ctx = vkl_context(gpu, NULL);
    vkl_transfer_mode(ctx, VKL_TRANSFER_MODE_ASYNC);

    // Resources.
    TestTransfer tt = {0};
    tt.ctx = ctx;
    tt.br = vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_VERTEX, 1, 16);
    memset(tt.data, 12, 16);
    memset(tt.img_data, 23, 16);
    tt.tex = vkl_ctx_texture(ctx, 2, (uvec3){16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM);

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
    vkl_download_buffers(ctx, tt.br, 0, 16, data2);
    AT(memcmp(tt.data, data2, 16) == 0);

    uint8_t img_data2[16 * 16 * 4];
    vkl_download_texture(ctx, tt.tex, 16 * 16 * 4, img_data2);
    AT(memcmp(tt.img_data, img_data2, 16 * 16 * 4) == 0);

    TEST_END
}



static void* _thread_download(void* arg)
{
    TestTransfer* tt = arg;
    VklContext* ctx = tt->ctx;
    uint8_t data3[16] = {0};
    vkl_download_buffers(ctx, tt->br, 0, 16, data3);
    if (memcmp(tt->data, data3, 16) != 0)
        tt->status = 1;
    vkl_transfer_wait(ctx, 10);
    if (memcmp(tt->data, data3, 16) == 0)
        tt->status = 1;
    tt->status = 0;

    vkl_transfer_stop(ctx);

    return NULL;
}

int test_context_download(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklContext* ctx = vkl_context(gpu, NULL);
    vkl_transfer_mode(ctx, VKL_TRANSFER_MODE_ASYNC);

    // Resources.
    TestTransfer tt = {0};
    tt.status = -1;
    tt.ctx = ctx;
    tt.br = vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_VERTEX, 1, 16);
    memset(tt.data, 12, 16);
    memset(tt.img_data, 23, 16);
    tt.tex = vkl_ctx_texture(ctx, 2, (uvec3){16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM);

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



int test_default_app(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklContext* ctx = vkl_context(gpu, NULL);

    char path[1024];
    snprintf(path, sizeof(path), "%s/pow2.comp.spv", SPIRV_DIR);
    vkl_ctx_compute(ctx, path);

    TEST_END
}
