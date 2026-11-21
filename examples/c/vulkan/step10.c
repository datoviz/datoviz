#include <math.h>
#include <stdbool.h>
#include <stddef.h>
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

// One vertex record: an object-space position and a color.
typedef struct
{
    float position[3];
    float color[3];
} Vertex;

// The push-constant block: one combined model-view-projection matrix, 64 bytes.
typedef struct
{
    mat4 mvp;
} Push;

// The eight corners of the cube, centered on its own origin.
static const Vertex VERTICES[8] = {
    {{-0.65f, -0.65f, -0.65f}, {1.0f, 0.2f, 0.2f}},
    {{ 0.65f, -0.65f, -0.65f}, {1.0f, 0.7f, 0.2f}},
    {{ 0.65f,  0.65f, -0.65f}, {0.2f, 0.9f, 0.3f}},
    {{-0.65f,  0.65f, -0.65f}, {0.2f, 0.5f, 1.0f}},
    {{-0.65f, -0.65f,  0.65f}, {0.5f, 0.2f, 1.0f}},
    {{ 0.65f, -0.65f,  0.65f}, {1.0f, 0.3f, 0.7f}},
    {{ 0.65f,  0.65f,  0.65f}, {0.3f, 1.0f, 0.9f}},
    {{-0.65f,  0.65f,  0.65f}, {0.9f, 0.9f, 0.2f}},
};

// Each face winds counter-clockwise when seen from outside the cube.
static const uint16_t INDICES[36] = {
    0, 2, 1, 0, 3, 2,
    4, 5, 6, 4, 6, 7,
    0, 1, 5, 0, 5, 4,
    3, 7, 6, 3, 6, 2,
    1, 2, 6, 1, 6, 5,
    0, 4, 7, 0, 7, 3,
};

// Shader source files, read at startup and again on every reload.
#define VERTEX_PATH   "shader.vert"
#define FRAGMENT_PATH "shader.frag"

// Renderer state: a borrowed device and the objects our program creates for drawing.
typedef struct
{
    DvzDevice* device;
    VkFormat color_format;
    DvzCommands* commands;
    DvzRendering* rendering;

    // The pipeline group, replaced as a whole on reload.
    DvzSlots* slots;
    DvzShader* vertex_shader;
    DvzShader* fragment_shader;
    DvzGraphics* pipeline;

    // Flags and timing shared by the main loop and the callbacks.
    bool reload_requested;
    uint64_t start_ns;
    float capture_time;
    bool animate;
    bool draw_failed;

    // Geometry, uploaded once at startup.
    DvzBuffer* vertex_buffer;
    DvzBuffer* index_buffer;
} Renderer;



/**
 * Multiply two column-major matrices, allowing output to alias either input.
 * @param left Left factor.
 * @param right Right factor.
 * @param out Product matrix.
 */
static void multiply_mat4(mat4 left, mat4 right, mat4 out)
{
    // Accumulate into a temporary, so out may be the same matrix as left or right.
    mat4 product = {{0}};
    for (uint32_t column = 0; column < 4; column++)
        for (uint32_t row = 0; row < 4; row++)
            for (uint32_t inner = 0; inner < 4; inner++)
                product[column][row] += left[inner][row] * right[column][inner];

    // Copy the finished product out.
    for (uint32_t column = 0; column < 4; column++)
        for (uint32_t row = 0; row < 4; row++)
            out[column][row] = product[column][row];
}



/**
 * Build a rotating model, translated view, and Vulkan perspective projection.
 * @param aspect Framebuffer width divided by height.
 * @param time Animation time in seconds.
 * @param out Combined projection * view * model matrix.
 */
static void make_mvp(float aspect, float time, mat4 out)
{
    // Model: spin about the vertical y axis and pulse in size. Each inner brace is a column.
    float angle = time * 0.7f;
    float cosine = cosf(angle);
    float sine = sinf(angle);
    float pulse = 1.0f + 0.08f * sinf(time * 3.0f);
    mat4 model = {
        {pulse * cosine, 0, -pulse * sine, 0},
        {0, pulse, 0, 0},
        {pulse * sine, 0, pulse * cosine, 0},
        {0, 0, 0, 1},
    };

    // Tilt the cube so that three faces can be seen.
    float tilt = -0.35f;
    mat4 tilt_matrix = {
        {1, 0, 0, 0},
        {0, cosf(tilt), sinf(tilt), 0},
        {0, -sinf(tilt), cosf(tilt), 0},
        {0, 0, 0, 1},
    };
    multiply_mat4(tilt_matrix, model, model);

    // View: the camera stands 3 units along +z, so the world moves 3 units along -z.
    mat4 view = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, -3, 1}};

    // Projection: a 60-degree vertical field of view, Vulkan's y-down NDC, depth 0 to 1.
    float near_clip = 0.1f;
    float far_clip = 100.0f;
    float fov_y = 1.0471976f;
    float focal_length = 1.0f / tanf(0.5f * fov_y);
    mat4 projection = {
        {focal_length / aspect, 0, 0, 0},
        {0, -focal_length, 0, 0},
        {0, 0, far_clip / (near_clip - far_clip), -1},
        {0, 0, near_clip * far_clip / (near_clip - far_clip), 0},
    };

    // Combine right to left: projection * (view * model).
    mat4 view_model = {{0}};
    multiply_mat4(view, model, view_model);
    multiply_mat4(projection, view_model, out);
}



/**
 * Destroy owned pipeline resources after GPU work has finished.
 * @param renderer Renderer whose pipeline resources are released and cleared.
 */
static void destroy_pipeline(Renderer* renderer)
{
    // Destroy each Vulkan object, free its wrapper, and clear the field.
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
    // Read the whole file. We own the returned text.
    DvzSize source_size = 0;
    char* source = dvz_read_text(path, &source_size);
    if (source == NULL)
    {
        fprintf(stderr, "could not read %s\n", path);
        return false;
    }

    // Compile it to SPIR-V for one graphics stage, then free the text.
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

    // On failure, report the status and the compiler's diagnostics with the file name.
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
    // Compile both shader files. We own both results, even when compilation fails.
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

    // Copy the SPIR-V into two shader modules, then destroy the compile results.
    int vertex_result =
        dvz_shader(renderer->device, vertex.spirv_size, vertex.spirv, renderer->vertex_shader);
    int fragment_result = dvz_shader(
        renderer->device, fragment.spirv_size, fragment.spirv, renderer->fragment_shader);
    dvz_shader_compile_result_destroy(&vertex);
    dvz_shader_compile_result_destroy(&fragment);
    if (vertex_result != 0 || fragment_result != 0)
        return -1;

    // The pipeline layout: one push-constant range of sizeof(Push) bytes for the vertex stage.
    dvz_slots(renderer->device, renderer->slots);
    dvz_slots_push(renderer->slots, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Push));
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

    // Vertex input: binding 0 holds Vertex records; locations 0 and 1 read their two fields.
    dvz_graphics_vertex_binding(
        renderer->pipeline, 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);
    dvz_graphics_vertex_attr(
        renderer->pipeline, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position));
    dvz_graphics_vertex_attr(
        renderer->pipeline, 0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color));

    // Fixed state: separate triangles and one color attachment in the canvas format.
    dvz_graphics_primitive(
        renderer->pipeline, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, DVZ_GRAPHICS_FLAGS_FIXED);
    dvz_graphics_attachment_color(renderer->pipeline, 0, renderer->color_format);

    // Depth: the canvas's depth format; test and write, keeping the nearer fragment.
    dvz_graphics_attachment_depth(renderer->pipeline, VK_FORMAT_D32_SFLOAT);
    dvz_graphics_depth(
        renderer->pipeline, false, true, VK_COMPARE_OP_LESS, DVZ_GRAPHICS_FLAGS_FIXED);

    // Culling: drop back faces; counter-clockwise on screen is the front.
    dvz_graphics_cull_mode(
        renderer->pipeline, VK_CULL_MODE_BACK_BIT, DVZ_GRAPHICS_FLAGS_FIXED);
    dvz_graphics_front_face(
        renderer->pipeline, VK_FRONT_FACE_COUNTER_CLOCKWISE, DVZ_GRAPHICS_FLAGS_FIXED);

    // The pipeline layout built above.
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

    // Only record the request; the main loop rebuilds the pipeline between frames.
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

    // Elapsed seconds since startup; offscreen captures use the fixed capture time.
    float time = renderer->capture_time;
    if (renderer->animate)
        time = (float)(dvz_time_monotonic_ns() - renderer->start_ns) * 1e-9f;

    // Build this frame's matrix from the current framebuffer aspect ratio.
    Push push = {0};
    float aspect = (float)frame->extent.width / (float)frame->extent.height;
    make_mvp(aspect, time, push.mvp);

    // A fixed dark background behind the cube.
    VkClearValue clear = {.color.float32 = {0.04f, 0.05f, 0.08f, 1.0f}};

    // Attach: point our wrapper at the command buffer the canvas lends us for this frame.
    dvz_commands_wrap_borrowed_recording(
        renderer->device, frame->command_buffer, renderer->commands);

    // Describe: render into this frame's image, cleared to the background color.
    // This resets the whole description, so the depth attachment is set after it.
    dvz_cmd_rendering_default(
        renderer->commands, frame->image_view, frame->extent.width, frame->extent.height,
        clear, renderer->rendering);

    // Depth: this frame's borrowed depth view, cleared to 1.0, the far plane.
    DvzAttachment* depth = dvz_rendering_depth(renderer->rendering);
    dvz_attachment_image(depth, frame->depth_view, frame->depth_layout);
    dvz_attachment_ops(depth, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
    dvz_attachment_clear(depth, (VkClearValue){.depthStencil = {1.0f, 0}});

    // Begin the pass; both clears happen here.
    dvz_cmd_rendering_begin(renderer->commands, renderer->rendering);

    // Use our pipeline for the draw, then record its dynamic viewport and scissor.
    dvz_cmd_bind_graphics(renderer->commands, renderer->pipeline);
    dvz_cmd_set_viewport_scissor(renderer->commands, frame->extent);

    // Copy the matrix into the command buffer; the loop checks draw_failed before submitting.
    DvzResult push_result = dvz_cmd_push_constants(
        renderer->commands, renderer->slots, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push), &push);
    if (push_result != DVZ_OK)
        renderer->draw_failed = true;

    // Read vertex records from binding 0 and 16-bit indices from the index buffer.
    DvzSize vertex_offset = 0;
    dvz_cmd_bind_vertex_buffers(
        renderer->commands, 0, 1, renderer->vertex_buffer, &vertex_offset);
    dvz_cmd_bind_index_buffer(
        renderer->commands, renderer->index_buffer, 0, VK_INDEX_TYPE_UINT16);

    // First index 0, vertex offset 0, 36 indices, first instance 0, 1 instance.
    dvz_cmd_draw_indexed(renderer->commands, 0, 0, 36, 0, 1);

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
    // With --time SECONDS, choose the instant of the animation that --png captures.
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
    window_config.title = "Depth and culling: press R to reload";
    window = dvz_window_create(host, backend, &window_config);
    COURSE_CHECK(window != NULL, "window creation failed");

    // The canvas borrows the window and the device. It owns the swapchain, the per-frame
    // images, and all frame synchronization.
    DvzCanvasConfig canvas_config = dvz_canvas_config();
    canvas_config.window = window;
    canvas_config.device = dvz_gpu_ctx_device(gpu);
    canvas_config.render_mode = mode;

    // The canvas also owns depth images in exactly this format, recreated on resize.
    canvas_config.depth_format = VK_FORMAT_D32_SFLOAT;
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

    // Allocate two empty buffer wrappers. The renderer owns them from here.
    renderer.vertex_buffer = dvz_buffer_create_wrapper();
    renderer.index_buffer = dvz_buffer_create_wrapper();
    COURSE_CHECK(
        renderer.vertex_buffer != NULL && renderer.index_buffer != NULL,
        "buffer allocation failed");

    // Vertex buffer in CPU-writable memory. The upload copies VERTICES into it.
    dvz_buffer(renderer.device, dvz_gpu_ctx_alloc(gpu), renderer.vertex_buffer);
    dvz_buffer_size(renderer.vertex_buffer, sizeof(VERTICES));
    dvz_buffer_usage(renderer.vertex_buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    dvz_buffer_flags(
        renderer.vertex_buffer, DVZ_ALLOC_MAPPED | DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
    int vertex_buffer_result = dvz_buffer_create(renderer.vertex_buffer);
    COURSE_CHECK(vertex_buffer_result == 0, "vertex buffer creation failed");
    dvz_buffer_upload(renderer.vertex_buffer, 0, sizeof(VERTICES), VERTICES);

    // Index buffer: the same pattern with the index usage flag.
    dvz_buffer(renderer.device, dvz_gpu_ctx_alloc(gpu), renderer.index_buffer);
    dvz_buffer_size(renderer.index_buffer, sizeof(INDICES));
    dvz_buffer_usage(renderer.index_buffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    dvz_buffer_flags(
        renderer.index_buffer, DVZ_ALLOC_MAPPED | DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
    int index_buffer_result = dvz_buffer_create(renderer.index_buffer);
    COURSE_CHECK(index_buffer_result == 0, "index buffer creation failed");
    dvz_buffer_upload(renderer.index_buffer, 0, sizeof(INDICES), INDICES);

    // Live runs animate from the clock; --png runs freeze time at capture_time.
    renderer.start_ns = dvz_time_monotonic_ns();
    renderer.capture_time = capture_time;
    renderer.animate = live;

    // Only now may the canvas call draw(): everything it uses exists.
    dvz_canvas_set_draw_callback(canvas, draw, &renderer);

    // Pressing R only sets a flag; the loop below performs the reload.
    DvzCallbackId keyboard_id =
        dvz_input_subscribe_keyboard(dvz_window_router(window), keyboard, &renderer);
    COURSE_CHECK(keyboard_id != DVZ_CALLBACK_ID_NONE, "keyboard subscription failed");

    // A live run loops until the window closes; a --png run renders one frame.
    uint64_t frame_index = 0;
    while (live ? !dvz_window_should_close(window) : frame_index < 1)
    {
        // Handle keyboard, mouse, resize, and close events.
        dvz_window_host_poll(host);

        // Between frames, build a complete candidate pipeline before touching the current one.
        if (renderer.reload_requested)
        {
            Renderer candidate = {
                .device = renderer.device,
                .color_format = renderer.color_format,
            };
            pipeline_result = create_pipeline(&candidate);
            if (pipeline_result == 0)
            {
                // Wait until no submitted frame uses the old pipeline, then take the new one.
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
                // Release the partial candidate and keep drawing with the current pipeline.
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

        // draw() cannot return a status, so check the flag it sets before submitting.
        COURSE_CHECK(!renderer.draw_failed, "push-constant update failed");

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
    // Submitted frames may still use these objects: wait until the GPU is idle.
    if (renderer.device != NULL)
        dvz_device_wait(renderer.device);

    // Destroy each buffer, then free its wrapper. A NULL wrapper was never allocated.
    if (renderer.index_buffer != NULL)
        dvz_buffer_destroy(renderer.index_buffer);
    dvz_buffer_free(renderer.index_buffer);
    if (renderer.vertex_buffer != NULL)
        dvz_buffer_destroy(renderer.vertex_buffer);
    dvz_buffer_free(renderer.vertex_buffer);

    // The pipeline group, then the two recording wrappers, which own no Vulkan object.
    destroy_pipeline(&renderer);
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
