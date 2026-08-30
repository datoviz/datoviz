#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <datoviz.h>
#include <datoviz/canvas.h>
#include <datoviz/stream/frame_stream.h>
#include <datoviz/vk/gpu_ctx.h>
#include <datoviz/vklite.h>
#include <datoviz/window.h>

#define WIDTH  800
#define HEIGHT 600

// Everything our program owns. It will grow in every chapter.
typedef struct
{
    DvzDevice* device;
    DvzCommands* commands;
    DvzRendering* rendering;
    uint64_t start_ns;
    float capture_time;
    bool animate;
} Renderer;

// Called once per frame by the canvas, with a command buffer we may record into.
static void draw(DvzCanvas* canvas, const DvzStreamFrame* frame, void* user_data)
{
    (void)canvas;
    Renderer* renderer = (Renderer*)user_data;

    // Elapsed seconds since startup. Offscreen captures use a fixed time so the image never
    // depends on how fast the machine ran.
    float t = renderer->capture_time;
    if (renderer->animate)
        t = (float)(dvz_time_monotonic_ns() - renderer->start_ns) * 1e-9f;

    VkClearValue clear = {.color.float32 = {
                              0.10f + 0.10f * sinf(t * 1.7f),
                              0.12f + 0.12f * sinf(t * 2.3f),
                              0.18f + 0.18f * sinf(t * 1.1f),
                              1.00f,
                          }};

    dvz_commands_wrap_borrowed_recording(renderer->device, frame->command_buffer, renderer->commands);
    dvz_cmd_rendering_default(
        renderer->commands, frame->image_view, frame->extent.width, frame->extent.height,
        clear, renderer->rendering);
    dvz_cmd_rendering_begin(renderer->commands, renderer->rendering);
    dvz_cmd_set_viewport_scissor(renderer->commands, frame->extent);
    dvz_cmd_rendering_end(renderer->commands);
    dvz_commands_unwrap(renderer->commands);
}

int main(int argc, char** argv)
{
    // With --png PATH the program renders offscreen and saves an image instead of opening a window.
    const char* png_path = NULL;
    float capture_time = 0.5f;
    for (int i = 1; i < argc - 1; i++)
    {
        if (strcmp(argv[i], "--png") == 0)
            png_path = argv[i + 1];
        else if (strcmp(argv[i], "--time") == 0)
            capture_time = strtof(argv[i + 1], NULL);
    }
    bool live = png_path == NULL;

    DvzBackend backend = live ? DVZ_BACKEND_GLFW : DVZ_BACKEND_OFFSCREEN;
    DvzCanvasRenderMode mode =
        live ? DVZ_CANVAS_RENDER_MODE_PRESENT : DVZ_CANVAS_RENDER_MODE_OFFSCREEN;

    // The window host talks to the operating system's windowing layer.
    DvzWindowHost* host = dvz_window_host();

    // The GPU context picks a physical device and creates the logical device and its queues.
    DvzGpuCtxConfig gpu_config = dvz_gpu_ctx_config();
    dvz_gpu_ctx_config_validation(&gpu_config, true);
    dvz_canvas_configure_gpu_ctx(host, backend, mode, &gpu_config);
    DvzGpuCtx* gpu = dvz_gpu_ctx(&gpu_config);
    if (gpu == NULL)
    {
        fprintf(stderr, "no usable GPU found\n");
        return 1;
    }

    DvzGpuInfo info = {0};
    if (dvz_gpu_ctx_gpu_info(gpu, &info))
        printf("GPU: %s\n", info.name);

    DvzWindowConfig window_config = dvz_window_config();
    window_config.width = WIDTH;
    window_config.height = HEIGHT;
    window_config.title = "Modern GPU Graphics in Vulkan";
    DvzWindow* window = dvz_window_create(host, backend, &window_config);

    // The canvas owns the swapchain, the per-frame images, and all frame synchronization.
    DvzCanvasConfig canvas_config = dvz_canvas_config();
    canvas_config.window = window;
    canvas_config.device = dvz_gpu_ctx_device(gpu);
    canvas_config.render_mode = mode;
    DvzCanvas* canvas = dvz_canvas_create(&canvas_config);
    if (canvas == NULL)
    {
        fprintf(stderr, "canvas creation failed\n");
        return 1;
    }

    Renderer renderer = {
        .device = dvz_gpu_ctx_device(gpu),
        .commands = dvz_commands_create_wrapper(),
        .rendering = dvz_rendering_create_wrapper(),
        .start_ns = dvz_time_monotonic_ns(),
        .capture_time = capture_time,
        .animate = live,
    };
    dvz_canvas_set_draw_callback(canvas, draw, &renderer);

    // Process one frame attempt; READY attempts are submitted below.
    uint64_t frame_index = 0;
    while (live ? !dvz_window_should_close(window) : frame_index < 1)
    {
        dvz_window_host_poll(host);
        int status = dvz_canvas_frame(canvas);
        if (status == DVZ_CANVAS_FRAME_WAIT_SURFACE)
            continue;
        if (status != DVZ_CANVAS_FRAME_READY)
            break;
        dvz_canvas_submit(canvas);
        frame_index++;
    }
    printf("rendered %llu frames\n", (unsigned long long)frame_index);

    if (png_path != NULL)
        dvz_canvas_capture_png(canvas, png_path);

    // Destroy resources in dependency-safe order.
    dvz_rendering_free(renderer.rendering);
    dvz_commands_free(renderer.commands);
    dvz_canvas_destroy(canvas);
    dvz_window_destroy(window);
    dvz_window_host_destroy(host);
    printf("validation errors: %u\n", dvz_gpu_ctx_error_count(gpu));
    dvz_gpu_ctx_destroy(gpu);
    return 0;
}
