#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <datoviz.h>
#include <datoviz/canvas.h>
#include <datoviz/common/functions.h>
#include <datoviz/fileio.h>
#include <datoviz/shader.h>
#include <datoviz/stream/frame_stream.h>
#include <datoviz/vk/gpu_ctx.h>
#include <datoviz/vklite.h>
#include <datoviz/window.h>

#define WIDTH  800
#define HEIGHT 600

#define COURSE_CHECK(condition, message)                                                         \
    do                                                                                           \
    {                                                                                            \
        if (!(condition))                                                                        \
        {                                                                                        \
            fprintf(stderr, "%s\n", message);                                                   \
            goto cleanup;                                                                        \
        }                                                                                        \
    } while (0)

typedef struct
{
    float position[2];
    float color[3];
} Vertex;

typedef struct
{
    float time;
    float pulse;
} Push;

static const Vertex VERTICES[4] = {
    {{-0.65f, -0.65f}, {1.0f, 0.2f, 0.2f}},
    {{ 0.65f, -0.65f}, {0.2f, 1.0f, 0.2f}},
    {{ 0.65f,  0.65f}, {0.2f, 0.3f, 1.0f}},
    {{-0.65f,  0.65f}, {1.0f, 0.8f, 0.2f}},
};

static const uint16_t INDICES[6] = {0, 1, 2, 0, 2, 3};

#define VERTEX_PATH   "shader.vert"
#define FRAGMENT_PATH "shader.frag"

typedef struct
{
    DvzDevice* device;
    VkFormat color_format;
    DvzCommands* commands;
    DvzRendering* rendering;
    DvzSlots* slots;
    DvzShader* vertex_shader;
    DvzShader* fragment_shader;
    DvzGraphics* pipeline;
    bool reload_requested;
    uint64_t start_ns;
    float capture_time;
    bool animate;
    bool draw_failed;
    DvzBuffer* vertex_buffer;
    DvzBuffer* index_buffer;
} Renderer;



/**
 * Destroy owned pipeline resources after GPU work has finished.
 * @param renderer Renderer whose pipeline resources are released and cleared.
 */
static void destroy_pipeline(Renderer* renderer)
{
    if (renderer->pipeline != NULL)
        dvz_graphics_destroy(renderer->pipeline);
    dvz_graphics_free(renderer->pipeline);
    renderer->pipeline = NULL;
    if (renderer->fragment_shader != NULL)
        dvz_shader_destroy(renderer->fragment_shader);
    dvz_shader_free(renderer->fragment_shader);
    renderer->fragment_shader = NULL;
    if (renderer->vertex_shader != NULL)
        dvz_shader_destroy(renderer->vertex_shader);
    dvz_shader_free(renderer->vertex_shader);
    renderer->vertex_shader = NULL;
    if (renderer->slots != NULL)
        dvz_slots_destroy(renderer->slots);
    dvz_slots_free(renderer->slots);
    renderer->slots = NULL;
}



/**
 * Read and compile a shader file, preserving diagnostics for the caller.
 * @param path Reader-local shader file.
 * @param stage Shader stage.
 * @param result Compilation result that the caller must destroy.
 * @return Whether compilation succeeded.
 */
static bool compile_file(
    const char* path, DvzShaderStage stage, DvzShaderCompileResult* result)
{
    DvzSize source_size = 0;
    char* source = dvz_read_text(path, &source_size);
    if (source == NULL)
    {
        fprintf(stderr, "could not read %s\n", path);
        return false;
    }
    DvzShaderCompileRequest request = {
        .stage = stage,
        .profile = DVZ_SHADER_PROFILE_GRAPHICS,
        .source = source,
        .source_size = source_size,
        .source_name = path,
        .entry_point = "main",
    };
    DvzShaderCompileStatus status = dvz_shader_compile(&request, result);
    dvz_memory_free(source);
    if (status != DVZ_SHADER_COMPILE_SUCCESS)
    {
        fprintf(
            stderr, "%s: %s\n", dvz_shader_compile_status_name(status),
            result->diagnostics != NULL ? result->diagnostics : "shader compilation failed");
        return false;
    }
    return true;
}



/**
 * Build a complete candidate pipeline from the current shader files.
 * @param renderer Renderer that owns the candidate resources, including partial failures.
 * @return Zero on success.
 */
static int create_pipeline(Renderer* renderer)
{
    DvzShaderCompileResult vertex = {0};
    DvzShaderCompileResult fragment = {0};
    if (!compile_file(VERTEX_PATH, DVZ_SHADER_STAGE_VERTEX, &vertex) ||
        !compile_file(FRAGMENT_PATH, DVZ_SHADER_STAGE_FRAGMENT, &fragment))
    {
        dvz_shader_compile_result_destroy(&vertex);
        dvz_shader_compile_result_destroy(&fragment);
        return -1;
    }



    renderer->vertex_shader = dvz_shader_create_wrapper();
    renderer->fragment_shader = dvz_shader_create_wrapper();
    renderer->slots = dvz_slots_create_wrapper();
    renderer->pipeline = dvz_graphics_create_wrapper();
    if (renderer->vertex_shader == NULL || renderer->fragment_shader == NULL ||
        renderer->slots == NULL || renderer->pipeline == NULL)
    {
        dvz_shader_compile_result_destroy(&vertex);
        dvz_shader_compile_result_destroy(&fragment);
        return -1;
    }



    int vertex_result =
        dvz_shader(renderer->device, vertex.spirv_size, vertex.spirv, renderer->vertex_shader);
    int fragment_result =
        dvz_shader(renderer->device, fragment.spirv_size, fragment.spirv, renderer->fragment_shader);
    dvz_shader_compile_result_destroy(&vertex);
    dvz_shader_compile_result_destroy(&fragment);
    if (vertex_result != 0 || fragment_result != 0)
        return -1;

    dvz_slots(renderer->device, renderer->slots);
    dvz_slots_push(renderer->slots, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Push));
    if (dvz_slots_create(renderer->slots) != 0)
        return -1;

    dvz_graphics(renderer->device, renderer->pipeline);
    dvz_graphics_shader(
        renderer->pipeline, VK_SHADER_STAGE_VERTEX_BIT,
        dvz_shader_handle(renderer->vertex_shader));
    dvz_graphics_shader(
        renderer->pipeline, VK_SHADER_STAGE_FRAGMENT_BIT,
        dvz_shader_handle(renderer->fragment_shader));
    dvz_graphics_vertex_binding(
        renderer->pipeline, 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);
    dvz_graphics_vertex_attr(
        renderer->pipeline, 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, position));
    dvz_graphics_vertex_attr(
        renderer->pipeline, 0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color));
    dvz_graphics_primitive(
        renderer->pipeline, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, DVZ_GRAPHICS_FLAGS_FIXED);
    dvz_graphics_attachment_color(renderer->pipeline, 0, renderer->color_format);
    dvz_graphics_layout(renderer->pipeline, dvz_slots_handle(renderer->slots));
    dvz_graphics_viewport(renderer->pipeline, 0, 0, 0, 0, 0, 1, DVZ_GRAPHICS_FLAGS_DYNAMIC);
    dvz_graphics_scissor(renderer->pipeline, 0, 0, 0, 0, DVZ_GRAPHICS_FLAGS_DYNAMIC);
    return dvz_graphics_create(renderer->pipeline);
}



/**
 * Request a shader reload without changing resources inside an input callback.
 * @param router Borrowed input router.
 * @param event Keyboard event.
 * @param user_data Renderer receiving the request.
 */
static void keyboard(DvzInputRouter* router, const DvzKeyboardEvent* event, void* user_data)
{
    (void)router;
    Renderer* renderer = (Renderer*)user_data;
    if (event->type == DVZ_KEYBOARD_EVENT_PRESS && event->key == DVZ_KEY_R)
        renderer->reload_requested = true;
}



/**
 * Record one draw into the canvas-owned command buffer.
 * @param canvas Borrowed canvas.
 * @param frame Borrowed current frame and its attachments.
 * @param user_data Renderer with resources that outlive this frame.
 */
static void draw(DvzCanvas* canvas, const DvzStreamFrame* frame, void* user_data)
{
    (void)canvas;
    Renderer* renderer = (Renderer*)user_data;
    float time = renderer->capture_time;
    if (renderer->animate)
        time = (float)(dvz_time_monotonic_ns() - renderer->start_ns) * 1e-9f;
    Push push = {.time = time, .pulse = 1.0f + 0.08f * sinf(time * 3.0f)};
    VkClearValue clear = {.color.float32 = {0.04f, 0.05f, 0.08f, 1.0f}};

    dvz_commands_wrap_borrowed_recording(
        renderer->device, frame->command_buffer, renderer->commands);
    dvz_cmd_rendering_default(
        renderer->commands, frame->image_view, frame->extent.width, frame->extent.height, clear,
        renderer->rendering);
    dvz_cmd_rendering_begin(renderer->commands, renderer->rendering);
    dvz_cmd_bind_graphics(renderer->commands, renderer->pipeline);
    dvz_cmd_set_viewport_scissor(renderer->commands, frame->extent);
    DvzResult push_result = dvz_cmd_push_constants(
        renderer->commands, renderer->slots, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push), &push);
    if (push_result != DVZ_OK)
        renderer->draw_failed = true;
    DvzSize vertex_offset = 0;
    dvz_cmd_bind_vertex_buffers(
        renderer->commands, 0, 1, renderer->vertex_buffer, &vertex_offset);
    dvz_cmd_bind_index_buffer(
        renderer->commands, renderer->index_buffer, 0, VK_INDEX_TYPE_UINT16);
    dvz_cmd_draw_indexed(renderer->commands, 0, 0, 6, 0, 1);
    dvz_cmd_rendering_end(renderer->commands);
    dvz_commands_unwrap(renderer->commands);
}



/**
 * Run the live lesson, or capture one deterministic frame with --png.
 * @param argc Argument count.
 * @param argv Command-line arguments.
 * @return Zero when rendering and Vulkan validation succeed.
 */
int main(int argc, char** argv)
{
    int exit_code = 1;
    DvzWindowHost* host = NULL;
    DvzGpuCtx* gpu = NULL;
    DvzWindow* window = NULL;
    DvzCanvas* canvas = NULL;
    Renderer renderer = {0};

    const char* png_path = NULL;
    float capture_time = 0.75f;
    for (int argument_index = 1; argument_index + 1 < argc; argument_index++)
    {
        if (strcmp(argv[argument_index], "--png") == 0)
            png_path = argv[++argument_index];
        else if (strcmp(argv[argument_index], "--time") == 0)
            capture_time = strtof(argv[++argument_index], NULL);
    }
    COURSE_CHECK(isfinite(capture_time), "capture time must be finite");
    bool live = png_path == NULL;
    DvzBackend backend = live ? DVZ_BACKEND_GLFW : DVZ_BACKEND_OFFSCREEN;
    DvzCanvasRenderMode mode =
        live ? DVZ_CANVAS_RENDER_MODE_PRESENT : DVZ_CANVAS_RENDER_MODE_OFFSCREEN;

    host = dvz_window_host();
    COURSE_CHECK(host != NULL, "window host creation failed");
    DvzGpuCtxConfig gpu_config = dvz_gpu_ctx_config();
    dvz_gpu_ctx_config_validation(&gpu_config, true);
    DvzResult configure_result =
        dvz_canvas_configure_gpu_ctx(host, backend, mode, &gpu_config);
    COURSE_CHECK(configure_result == DVZ_OK, "GPU configuration failed");
    gpu = dvz_gpu_ctx(&gpu_config);
    COURSE_CHECK(gpu != NULL, "no usable GPU found");

    DvzWindowConfig window_config = dvz_window_config();
    window_config.width = WIDTH;
    window_config.height = HEIGHT;
    window_config.title = "Push constants: press R to reload";
    window = dvz_window_create(host, backend, &window_config);
    COURSE_CHECK(window != NULL, "window creation failed");

    DvzCanvasConfig canvas_config = dvz_canvas_config();
    canvas_config.window = window;
    canvas_config.device = dvz_gpu_ctx_device(gpu);
    canvas_config.render_mode = mode;
    canvas = dvz_canvas_create(&canvas_config);
    COURSE_CHECK(canvas != NULL, "canvas creation failed");

    renderer.device = dvz_gpu_ctx_device(gpu);
    // Acquire and submit one default clear before creating a format-dependent pipeline.
    int initial_status = DVZ_CANVAS_FRAME_WAIT_SURFACE;
    while (initial_status == DVZ_CANVAS_FRAME_WAIT_SURFACE)
    {
        dvz_window_host_poll(host);
        if (live && dvz_window_should_close(window))
        {
            exit_code = 0;
            goto cleanup;
        }
        initial_status = dvz_canvas_frame(canvas);
    }
    COURSE_CHECK(initial_status == DVZ_CANVAS_FRAME_READY, "initial frame preparation failed");
    int initial_submit_result = dvz_canvas_submit(canvas);
    COURSE_CHECK(initial_submit_result == 0, "initial frame submission failed");
    renderer.color_format = dvz_canvas_frame_format(canvas);
    COURSE_CHECK(renderer.color_format != VK_FORMAT_UNDEFINED, "canvas color format is unresolved");

    renderer.commands = dvz_commands_create_wrapper();
    renderer.rendering = dvz_rendering_create_wrapper();
    COURSE_CHECK(
        renderer.commands != NULL && renderer.rendering != NULL, "renderer allocation failed");
    int pipeline_result = create_pipeline(&renderer);
    COURSE_CHECK(pipeline_result == 0, "graphics pipeline creation failed");
    renderer.vertex_buffer = dvz_buffer_create_wrapper();
    renderer.index_buffer = dvz_buffer_create_wrapper();
    COURSE_CHECK(
        renderer.vertex_buffer != NULL && renderer.index_buffer != NULL,
        "buffer allocation failed");

    dvz_buffer(renderer.device, dvz_gpu_ctx_alloc(gpu), renderer.vertex_buffer);
    dvz_buffer_size(renderer.vertex_buffer, sizeof(VERTICES));
    dvz_buffer_usage(renderer.vertex_buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    dvz_buffer_flags(
        renderer.vertex_buffer, DVZ_ALLOC_MAPPED | DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
    int vertex_buffer_result = dvz_buffer_create(renderer.vertex_buffer);
    COURSE_CHECK(vertex_buffer_result == 0, "vertex buffer creation failed");
    dvz_buffer_upload(renderer.vertex_buffer, 0, sizeof(VERTICES), VERTICES);

    dvz_buffer(renderer.device, dvz_gpu_ctx_alloc(gpu), renderer.index_buffer);
    dvz_buffer_size(renderer.index_buffer, sizeof(INDICES));
    dvz_buffer_usage(renderer.index_buffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    dvz_buffer_flags(
        renderer.index_buffer, DVZ_ALLOC_MAPPED | DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
    int index_buffer_result = dvz_buffer_create(renderer.index_buffer);
    COURSE_CHECK(index_buffer_result == 0, "index buffer creation failed");
    dvz_buffer_upload(renderer.index_buffer, 0, sizeof(INDICES), INDICES);
    renderer.start_ns = dvz_time_monotonic_ns();
    renderer.capture_time = capture_time;
    renderer.animate = live;
    dvz_canvas_set_draw_callback(canvas, draw, &renderer);

    DvzCallbackId keyboard_id =
        dvz_input_subscribe_keyboard(dvz_window_router(window), keyboard, &renderer);
    COURSE_CHECK(keyboard_id != DVZ_CALLBACK_ID_NONE, "keyboard subscription failed");

    uint64_t frame_index = 0;
    while (live ? !dvz_window_should_close(window) : frame_index < 1)
    {
        dvz_window_host_poll(host);
        if (renderer.reload_requested)
        {
            Renderer candidate = {
                .device = renderer.device,
                .color_format = renderer.color_format,
            };
            pipeline_result = create_pipeline(&candidate);
            if (pipeline_result == 0)
            {
                dvz_device_wait(renderer.device);
                destroy_pipeline(&renderer);
                renderer.slots = candidate.slots;
                renderer.vertex_shader = candidate.vertex_shader;
                renderer.fragment_shader = candidate.fragment_shader;
                renderer.pipeline = candidate.pipeline;
                printf("reloaded %s and %s\n", VERTEX_PATH, FRAGMENT_PATH);
            }
            else
            {
                destroy_pipeline(&candidate);
                fprintf(stderr, "reload failed; keeping the current pipeline\n");
            }
            renderer.reload_requested = false;
        }
        int status = dvz_canvas_frame(canvas);
        if (status == DVZ_CANVAS_FRAME_WAIT_SURFACE)
            continue;
        COURSE_CHECK(status == DVZ_CANVAS_FRAME_READY, "frame preparation failed");
        COURSE_CHECK(!renderer.draw_failed, "push-constant update failed");
        int submit_result = dvz_canvas_submit(canvas);
        COURSE_CHECK(submit_result == 0, "frame submission failed");
        frame_index++;
    }
    if (png_path != NULL)
    {
        int capture_result = dvz_canvas_capture_png(canvas, png_path);
        COURSE_CHECK(capture_result == 0, "PNG capture failed");
    }
    exit_code = 0;

cleanup:
    if (renderer.device != NULL)
        dvz_device_wait(renderer.device);
    if (renderer.index_buffer != NULL)
        dvz_buffer_destroy(renderer.index_buffer);
    dvz_buffer_free(renderer.index_buffer);
    if (renderer.vertex_buffer != NULL)
        dvz_buffer_destroy(renderer.vertex_buffer);
    dvz_buffer_free(renderer.vertex_buffer);
    destroy_pipeline(&renderer);
    dvz_rendering_free(renderer.rendering);
    dvz_commands_free(renderer.commands);
    dvz_canvas_destroy(canvas);
    dvz_window_destroy(window);
    dvz_window_host_destroy(host);
    uint32_t validation_errors = gpu != NULL ? dvz_gpu_ctx_error_count(gpu) : 0;
    if (gpu != NULL)
        printf("validation errors: %u\n", validation_errors);
    dvz_gpu_ctx_destroy(gpu);
    return exit_code == 0 && validation_errors == 0 ? 0 : 1;
}
