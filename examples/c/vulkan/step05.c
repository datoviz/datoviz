#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <datoviz.h>
#include <datoviz/advanced.h>

#define WIDTH  800
#define HEIGHT 600

// On failure, print the message and jump to the cleanup block at the end of main().
#define COURSE_CHECK(condition, message)      \
    do                                        \
    {                                         \
        if (!(condition))                     \
        {                                     \
            fprintf(stderr, "%s\n", message); \
            goto cleanup;                     \
        }                                     \
    } while (0)

// Shader files, relative to the directory where you launch the program.
#define VERTEX_PATH   "shader.vert"
#define FRAGMENT_PATH "shader.frag"

// Renderer state: a borrowed device and the objects our program creates for drawing.
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
} Renderer;



/**
 * Destroy owned pipeline resources after GPU work has finished.
 * @param renderer Renderer whose pipeline resources are released and cleared.
 */
static void destroy_pipeline(Renderer* renderer)
{
    // Destroy each Vulkan object, free its wrapper, then clear the pointer. NULL means "none".
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
    // Read: we own the returned null-terminated copy of the file.
    DvzSize source_size = 0;
    char* source = dvz_read_text(path, &source_size);
    if (source == NULL)
    {
        fprintf(stderr, "could not read %s\n", path);
        return false;
    }

    // Request: one graphics-stage shader, named after its file, starting at main().
    DvzShaderCompileRequest request = {
        .stage = stage,
        .profile = DVZ_SHADER_PROFILE_GRAPHICS,
        .source = source,
        .source_size = source_size,
        .source_name = path,
        .entry_point = "main",
    };

    // Compile: the result owns its SPIR-V or diagnostics; our source copy is no longer needed.
    DvzShaderCompileStatus status = dvz_shader_compile(&request, result);
    dvz_memory_free(source);

    // Diagnose: print the status name and the compiler's message, which names file and line.
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
    // Compile both files. We own both results and destroy them on every path.
    DvzShaderCompileResult vertex = {0};
    DvzShaderCompileResult fragment = {0};
    if (!compile_file(VERTEX_PATH, DVZ_SHADER_STAGE_VERTEX, &vertex) ||
        !compile_file(FRAGMENT_PATH, DVZ_SHADER_STAGE_FRAGMENT, &fragment))
    {
        dvz_shader_compile_result_destroy(&vertex);
        dvz_shader_compile_result_destroy(&fragment);
        return -1;
    }

    // Allocate four empty wrappers. The renderer owns them, even if a later step fails.
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

    // Copy the SPIR-V into two shader modules, then destroy both compile results.
    int vertex_result =
        dvz_shader(renderer->device, vertex.spirv_size, vertex.spirv, renderer->vertex_shader);
    int fragment_result = dvz_shader(
        renderer->device, fragment.spirv_size, fragment.spirv, renderer->fragment_shader);
    dvz_shader_compile_result_destroy(&vertex);
    dvz_shader_compile_result_destroy(&fragment);
    if (vertex_result != 0 || fragment_result != 0)
        return -1;

    // An empty pipeline layout: these shaders read no resources yet.
    dvz_slots(renderer->device, renderer->slots);
    if (dvz_slots_create(renderer->slots) != 0)
        return -1;

    // Attach the two programmable stages to the pipeline.
    dvz_graphics(renderer->device, renderer->pipeline);
    dvz_graphics_shader(
        renderer->pipeline, VK_SHADER_STAGE_VERTEX_BIT,
        dvz_shader_handle(renderer->vertex_shader));
    dvz_graphics_shader(
        renderer->pipeline, VK_SHADER_STAGE_FRAGMENT_BIT,
        dvz_shader_handle(renderer->fragment_shader));

    // Fixed state: separate triangles, one color attachment in the canvas format, empty layout.
    dvz_graphics_primitive(
        renderer->pipeline, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, DVZ_GRAPHICS_FLAGS_FIXED);
    dvz_graphics_attachment_color(renderer->pipeline, 0, renderer->color_format);
    dvz_graphics_layout(renderer->pipeline, dvz_slots_handle(renderer->slots));

    // Dynamic state: the zeros are placeholders; each frame records the real values.
    dvz_graphics_viewport(renderer->pipeline, 0, 0, 0, 0, 0, 1, DVZ_GRAPHICS_FLAGS_DYNAMIC);
    dvz_graphics_scissor(renderer->pipeline, 0, 0, 0, 0, DVZ_GRAPHICS_FLAGS_DYNAMIC);

    // Create the Vulkan pipeline object from everything above.
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

    // Only record the request; the render loop rebuilds the pipeline between frames.
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

    // A fixed dark background behind the triangle.
    VkClearValue clear = {.color.float32 = {0.04f, 0.05f, 0.08f, 1.0f}};

    // Attach: point our wrapper at the command buffer the canvas lends us for this frame.
    dvz_commands_wrap_borrowed_recording(
        renderer->device, frame->command_buffer, renderer->commands);

    // Describe: render into this frame's image, cleared to the background color.
    dvz_cmd_rendering_default(
        renderer->commands, frame->image_view, frame->extent.width, frame->extent.height,
        clear, renderer->rendering);

    // Begin the pass; the clear happens here.
    dvz_cmd_rendering_begin(renderer->commands, renderer->rendering);

    // Use our pipeline for the draw, then record its dynamic viewport and scissor.
    dvz_cmd_bind_graphics(renderer->commands, renderer->pipeline);
    dvz_cmd_set_viewport_scissor(renderer->commands, frame->extent);

    // First vertex 0, 3 vertices, first instance 0, 1 instance.
    dvz_cmd_draw(renderer->commands, 0, 3, 0, 1);

    // End the pass, then detach from the borrowed command buffer; the canvas submits it.
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
    // Everything the cleanup block releases starts as NULL, meaning "not created yet".
    // Zero-initializing the renderer sets each of its handles to NULL too.
    int exit_code = 1;
    DvzWindowHost* host = NULL;
    DvzGpuCtx* gpu = NULL;
    DvzWindow* window = NULL;
    DvzCanvas* canvas = NULL;
    Renderer renderer = {0};

    // With --png PATH, render offscreen and save an image instead of opening a window.
    const char* png_path = NULL;
    for (int argument_index = 1; argument_index + 1 < argc; argument_index++)
        if (strcmp(argv[argument_index], "--png") == 0)
            png_path = argv[argument_index + 1];
    bool live = png_path == NULL;

    // The backend provides the window; the render mode decides where finished frames go.
    DvzBackend backend = live ? DVZ_BACKEND_GLFW : DVZ_BACKEND_OFFSCREEN;
    DvzCanvasRenderMode mode =
        live ? DVZ_CANVAS_RENDER_MODE_PRESENT : DVZ_CANVAS_RENDER_MODE_OFFSCREEN;

    // The window host talks to the operating system's windowing layer.
    host = dvz_window_host();
    COURSE_CHECK(host != NULL, "window host creation failed");

    // Start from the default GPU configuration and turn on the validation layers.
    DvzGpuCtxConfig gpu_config = dvz_gpu_ctx_config();
    dvz_gpu_ctx_config_validation(&gpu_config, true);

    // Add the extensions and features the canvas needs for this backend and mode.
    DvzResult configure_result =
        dvz_canvas_configure_gpu_ctx(host, backend, mode, &gpu_config);
    COURSE_CHECK(configure_result == DVZ_OK, "GPU configuration failed");

    // Pick a physical device and create the logical device and its queues.
    gpu = dvz_gpu_ctx(&gpu_config);
    COURSE_CHECK(gpu != NULL, "no usable GPU found");

    // Open the window, or a windowless target of the same size with --png.
    DvzWindowConfig window_config = dvz_window_config();
    window_config.width = WIDTH;
    window_config.height = HEIGHT;
    window_config.title = "Shader reload: press R";
    window = dvz_window_create(host, backend, &window_config);
    COURSE_CHECK(window != NULL, "window creation failed");

    // The canvas borrows the window and the device. It owns the swapchain, the per-frame
    // images, and all frame synchronization.
    DvzCanvasConfig canvas_config = dvz_canvas_config();
    canvas_config.window = window;
    canvas_config.device = dvz_gpu_ctx_device(gpu);
    canvas_config.render_mode = mode;
    canvas = dvz_canvas_create(&canvas_config);
    COURSE_CHECK(canvas != NULL, "canvas creation failed");

    // create_pipeline(), draw(), and the cleanup block reach the device through the renderer.
    renderer.device = dvz_gpu_ctx_device(gpu);

    // Acquire and submit one default clear before creating a format-dependent pipeline.
    // While the surface is unavailable, keep polling unless the user closes the window.
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

    // The pipeline must be built for exactly this color format.
    renderer.color_format = dvz_canvas_frame_format(canvas);
    COURSE_CHECK(
        renderer.color_format != VK_FORMAT_UNDEFINED, "canvas color format is unresolved");

    // Allocate the recording wrappers, then build the pipeline before any frame draws.
    renderer.commands = dvz_commands_create_wrapper();
    renderer.rendering = dvz_rendering_create_wrapper();
    COURSE_CHECK(
        renderer.commands != NULL && renderer.rendering != NULL, "renderer allocation failed");
    int pipeline_result = create_pipeline(&renderer);
    COURSE_CHECK(pipeline_result == 0, "graphics pipeline creation failed");

    // Only now may the canvas call draw(): everything it uses exists.
    dvz_canvas_set_draw_callback(canvas, draw, &renderer);

    // On each key event, the window's router calls keyboard() with &renderer.
    DvzCallbackId keyboard_id =
        dvz_input_subscribe_keyboard(dvz_window_router(window), keyboard, &renderer);
    COURSE_CHECK(keyboard_id != DVZ_CALLBACK_ID_NONE, "keyboard subscription failed");

    // A live run loops until the window closes; a --png run renders one frame.
    uint64_t frame_index = 0;
    while (live ? !dvz_window_should_close(window) : frame_index < 1)
    {
        // Handle keyboard, mouse, resize, and close events.
        dvz_window_host_poll(host);

        // Act on a reload request here, between frames, never inside a callback.
        if (renderer.reload_requested)
        {
            // Build a complete candidate. The current pipeline stays untouched meanwhile.
            Renderer candidate = {
                .device = renderer.device,
                .color_format = renderer.color_format,
            };
            pipeline_result = create_pipeline(&candidate);
            if (pipeline_result == 0)
            {
                // Success: wait until no submitted frame uses the old pipeline, then swap.
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
                // Failure: release the partial candidate and keep drawing with the old pipeline.
                destroy_pipeline(&candidate);
                fprintf(stderr, "reload failed; keeping the current pipeline\n");
            }
            renderer.reload_requested = false;
        }

        // Acquire the next image and call draw(). WAIT_SURFACE means "try again later".
        int status = dvz_canvas_frame(canvas);
        if (status == DVZ_CANVAS_FRAME_WAIT_SURFACE)
            continue;
        COURSE_CHECK(status == DVZ_CANVAS_FRAME_READY, "frame preparation failed");

        // Queue the recorded commands on the GPU and, in live mode, present the result.
        int submit_result = dvz_canvas_submit(canvas);
        COURSE_CHECK(submit_result == 0, "frame submission failed");
        frame_index++;
    }

    // With --png, wait for the GPU, read the image back, and save it.
    if (png_path != NULL)
    {
        int capture_result = dvz_canvas_capture_png(canvas, png_path);
        COURSE_CHECK(capture_result == 0, "PNG capture failed");
    }

    exit_code = 0;

cleanup:
    // Submitted frames may still use the pipeline: wait until the GPU is idle.
    if (renderer.device != NULL)
        dvz_device_wait(renderer.device);

    // Destroy the pipeline, shader modules, and layout through the shared helper.
    destroy_pipeline(&renderer);

    // These two wrappers own no Vulkan object, so they are only freed.
    dvz_rendering_free(renderer.rendering);
    dvz_commands_free(renderer.commands);

    // Then the rest in chapter 2's order: canvas, window, window host.
    dvz_canvas_destroy(canvas);
    dvz_window_destroy(window);
    dvz_window_host_destroy(host);

    // Read the validation error count before destroying the GPU context that records it.
    uint32_t validation_errors = gpu != NULL ? dvz_gpu_ctx_error_count(gpu) : 0;
    if (gpu != NULL)
        printf("validation errors: %u\n", validation_errors);
    dvz_gpu_ctx_destroy(gpu);
    return exit_code == 0 && validation_errors == 0 ? 0 : 1;
}
