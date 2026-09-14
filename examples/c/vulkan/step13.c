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
#include <datoviz/controller.h>
#include <datoviz/fileio.h>
#include <datoviz/math/types.h>
#include <datoviz/shader.h>
#include <datoviz/stream/frame_stream.h>
#include <datoviz/vk/gpu_ctx.h>
#include <datoviz/vklite.h>
#include <datoviz/window.h>

#define WIDTH        800
#define HEIGHT       600
#define TEXTURE_SIZE 64

#define COURSE_CHECK(condition, message)                                                          \
    do                                                                                            \
    {                                                                                             \
        if (!(condition))                                                                         \
        {                                                                                         \
            fprintf(stderr, "%s\n", message);                                                     \
            goto cleanup;                                                                         \
        }                                                                                         \
    } while (0)

typedef struct
{
    float position[3];
    float uv[2];
} Vertex;

typedef struct
{
    mat4 mvp;
} Push;

static const Vertex VERTICES[24] = {
    // Back face.
    {{0.65f, -0.65f, -0.65f}, {0.0f, 0.0f}},
    {{-0.65f, -0.65f, -0.65f}, {1.0f, 0.0f}},
    {{-0.65f, 0.65f, -0.65f}, {1.0f, 1.0f}},
    {{0.65f, 0.65f, -0.65f}, {0.0f, 1.0f}},
    // Front face.
    {{-0.65f, -0.65f, 0.65f}, {0.0f, 0.0f}},
    {{0.65f, -0.65f, 0.65f}, {1.0f, 0.0f}},
    {{0.65f, 0.65f, 0.65f}, {1.0f, 1.0f}},
    {{-0.65f, 0.65f, 0.65f}, {0.0f, 1.0f}},
    // Bottom face.
    {{-0.65f, -0.65f, -0.65f}, {0.0f, 0.0f}},
    {{0.65f, -0.65f, -0.65f}, {1.0f, 0.0f}},
    {{0.65f, -0.65f, 0.65f}, {1.0f, 1.0f}},
    {{-0.65f, -0.65f, 0.65f}, {0.0f, 1.0f}},
    // Top face.
    {{-0.65f, 0.65f, -0.65f}, {0.0f, 0.0f}},
    {{-0.65f, 0.65f, 0.65f}, {1.0f, 0.0f}},
    {{0.65f, 0.65f, 0.65f}, {1.0f, 1.0f}},
    {{0.65f, 0.65f, -0.65f}, {0.0f, 1.0f}},
    // Right face.
    {{0.65f, -0.65f, 0.65f}, {0.0f, 0.0f}},
    {{0.65f, -0.65f, -0.65f}, {1.0f, 0.0f}},
    {{0.65f, 0.65f, -0.65f}, {1.0f, 1.0f}},
    {{0.65f, 0.65f, 0.65f}, {0.0f, 1.0f}},
    // Left face.
    {{-0.65f, -0.65f, -0.65f}, {0.0f, 0.0f}},
    {{-0.65f, -0.65f, 0.65f}, {1.0f, 0.0f}},
    {{-0.65f, 0.65f, 0.65f}, {1.0f, 1.0f}},
    {{-0.65f, 0.65f, -0.65f}, {0.0f, 1.0f}},
};

// Each face winds counter-clockwise when seen from outside the cube.
static const uint16_t INDICES[36] = {
    0,  1,  2,  0,  2,  3,  4,  5,  6,  4,  6,  7,  8,  9,  10, 8,  10, 11,
    12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23,
};

#define VERTEX_PATH   "shader.vert"
#define FRAGMENT_PATH "shader.frag"

typedef struct
{
    DvzDevice* device;
    VkFormat color_format;
    DvzVma* allocator;
    DvzCommands* commands;
    DvzRendering* rendering;
    DvzSlots* slots;
    DvzShader* vertex_shader;
    DvzShader* fragment_shader;
    DvzGraphics* pipeline;
    bool reload_requested;
    DvzArcball* arcball;
    DvzCamera* camera;
    bool draw_failed;
    DvzBuffer* vertex_buffer;
    DvzBuffer* index_buffer;
    DvzImages* texture;
    DvzImageViews* texture_view;
    DvzSampler* sampler;
    DvzDescriptors* descriptors;
} Renderer;



/**
 * Multiply two column-major matrices, allowing output to alias either input.
 * @param left Left factor.
 * @param right Right factor.
 * @param out Product matrix.
 */
static void multiply_mat4(mat4 left, mat4 right, mat4 out)
{
    mat4 product = {{0}};
    for (uint32_t column = 0; column < 4; column++)
        for (uint32_t row = 0; row < 4; row++)
            for (uint32_t inner = 0; inner < 4; inner++)
                product[column][row] += left[inner][row] * right[column][inner];
    for (uint32_t column = 0; column < 4; column++)
        for (uint32_t row = 0; row < 4; row++)
            out[column][row] = product[column][row];
}



/**
 * Destroy owned pipeline resources after GPU work has finished.
 * @param renderer Renderer whose pipeline resources are released and cleared.
 */
static void destroy_pipeline(Renderer* renderer)
{
    dvz_descriptors_free(renderer->descriptors);
    renderer->descriptors = NULL;
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
static bool compile_file(const char* path, DvzShaderStage stage, DvzShaderCompileResult* result)
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
    int fragment_result = dvz_shader(
        renderer->device, fragment.spirv_size, fragment.spirv, renderer->fragment_shader);
    dvz_shader_compile_result_destroy(&vertex);
    dvz_shader_compile_result_destroy(&fragment);
    if (vertex_result != 0 || fragment_result != 0)
        return -1;

    dvz_slots(renderer->device, renderer->slots);
    dvz_slots_binding(
        renderer->slots, 0, 0, 1, VK_SHADER_STAGE_FRAGMENT_BIT,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
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
        renderer->pipeline, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position));
    dvz_graphics_vertex_attr(
        renderer->pipeline, 0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv));
    dvz_graphics_primitive(
        renderer->pipeline, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, DVZ_GRAPHICS_FLAGS_FIXED);
    dvz_graphics_attachment_color(renderer->pipeline, 0, renderer->color_format);
    dvz_graphics_attachment_depth(renderer->pipeline, VK_FORMAT_D32_SFLOAT);
    dvz_graphics_depth(
        renderer->pipeline, false, true, VK_COMPARE_OP_LESS, DVZ_GRAPHICS_FLAGS_FIXED);
    dvz_graphics_cull_mode(renderer->pipeline, VK_CULL_MODE_BACK_BIT, DVZ_GRAPHICS_FLAGS_FIXED);
    dvz_graphics_front_face(
        renderer->pipeline, VK_FRONT_FACE_COUNTER_CLOCKWISE, DVZ_GRAPHICS_FLAGS_FIXED);
    dvz_graphics_layout(renderer->pipeline, dvz_slots_handle(renderer->slots));
    dvz_graphics_viewport(renderer->pipeline, 0, 0, 0, 0, 0, 1, DVZ_GRAPHICS_FLAGS_DYNAMIC);
    dvz_graphics_scissor(renderer->pipeline, 0, 0, 0, 0, DVZ_GRAPHICS_FLAGS_DYNAMIC);
    if (dvz_graphics_create(renderer->pipeline) != 0)
        return -1;
    renderer->descriptors = dvz_descriptors_create_wrapper();
    if (renderer->descriptors == NULL)
        return -1;
    dvz_descriptors(renderer->slots, renderer->descriptors);
    dvz_descriptors_image(
        renderer->descriptors, 0, 0, 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        dvz_image_views_handle(renderer->texture_view, 0), dvz_sampler_handle(renderer->sampler));
    return dvz_descriptors_handle(renderer->descriptors, 0) != VK_NULL_HANDLE ? 0 : -1;
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
 * Upload a small checkerboard into a device-local image.
 * @param renderer Renderer that owns the image.
 * @return Zero on success.
 */
static int create_texture(Renderer* renderer)
{
    uint8_t pixels[TEXTURE_SIZE * TEXTURE_SIZE * 4];
    for (uint32_t y = 0; y < TEXTURE_SIZE; y++)
    {
        for (uint32_t x = 0; x < TEXTURE_SIZE; x++)
        {
            bool check = ((x / 8 + y / 8) & 1) != 0;
            size_t i = ((size_t)y * TEXTURE_SIZE + x) * 4;
            pixels[i + 0] = check ? 235 : 35;
            pixels[i + 1] = check ? 170 : 80;
            pixels[i + 2] = check ? 55 : 190;
            pixels[i + 3] = 255;
        }
    }

    DvzBuffer* staging = dvz_buffer_create_wrapper();
    DvzCommands* upload = dvz_commands_create_wrapper();
    renderer->texture = dvz_images_create_wrapper();
    if (staging == NULL || upload == NULL || renderer->texture == NULL)
        goto error;

    dvz_buffer(renderer->device, renderer->allocator, staging);
    dvz_buffer_size(staging, sizeof(pixels));
    dvz_buffer_usage(staging, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    dvz_buffer_flags(staging, DVZ_ALLOC_MAPPED | DVZ_ALLOC_HOST_ACCESS_SEQUENTIAL_WRITE);
    if (dvz_buffer_create(staging) != 0)
        goto error;
    dvz_buffer_upload(staging, 0, sizeof(pixels), pixels);

    dvz_images(renderer->device, renderer->allocator, VK_IMAGE_TYPE_2D, 1, renderer->texture);
    dvz_images_format(renderer->texture, VK_FORMAT_R8G8B8A8_SRGB);
    dvz_images_size(renderer->texture, TEXTURE_SIZE, TEXTURE_SIZE, 1);
    dvz_images_usage(
        renderer->texture, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    if (dvz_images_create(renderer->texture) != 0)
        goto error;

    dvz_commands(renderer->device, dvz_device_queue(renderer->device, DVZ_QUEUE_MAIN), 1, upload);
    if (dvz_cmd_begin_result(upload) != 0)
        goto error;
    DvzBarriers barriers = {0};
    dvz_barriers(&barriers);
    DvzBarrierImage* image_barrier =
        dvz_barriers_image(&barriers, dvz_image_handle(renderer->texture, 0));
    if (image_barrier == NULL)
        goto error;
    dvz_barrier_image_stage(image_barrier, VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_COPY_BIT);
    dvz_barrier_image_access(image_barrier, 0, VK_ACCESS_2_TRANSFER_WRITE_BIT);
    dvz_barrier_image_layout(
        image_barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    dvz_barrier_image_aspect(image_barrier, VK_IMAGE_ASPECT_COLOR_BIT);
    dvz_cmd_barriers(upload, &barriers);
    DvzImageRegion region = {0};
    dvz_image_region(&region);
    dvz_image_region_extent(&region, TEXTURE_SIZE, TEXTURE_SIZE, 1);
    dvz_cmd_copy_buffer_to_image(
        upload, dvz_buffer_handle(staging), 0, dvz_image_handle(renderer->texture, 0),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &region);
    dvz_barrier_image_stage(
        image_barrier, VK_PIPELINE_STAGE_2_COPY_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
    dvz_barrier_image_access(
        image_barrier, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
    dvz_barrier_image_layout(
        image_barrier, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    dvz_cmd_barriers(upload, &barriers);
    if (dvz_cmd_end_result(upload) != 0 || dvz_cmd_submit_result(upload) != 0)
        goto error;
    renderer->texture_view = dvz_image_views_create_wrapper();
    renderer->sampler = dvz_sampler_create_wrapper();
    if (renderer->texture_view == NULL || renderer->sampler == NULL)
        goto error;
    dvz_image_views(renderer->texture, renderer->texture_view);
    if (dvz_image_views_create(renderer->texture_view) != 0)
        goto error;
    dvz_sampler(renderer->device, renderer->sampler);
    dvz_sampler_min_filter(renderer->sampler, VK_FILTER_LINEAR);
    dvz_sampler_mag_filter(renderer->sampler, VK_FILTER_LINEAR);
    dvz_sampler_address_mode(
        renderer->sampler, DVZ_SAMPLER_AXIS_U, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    dvz_sampler_address_mode(
        renderer->sampler, DVZ_SAMPLER_AXIS_V, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    if (dvz_sampler_create(renderer->sampler) != 0)
        goto error;
    dvz_commands_destroy(upload);
    dvz_commands_free(upload);
    dvz_buffer_destroy(staging);
    dvz_buffer_free(staging);
    return 0;

error:
    if (upload != NULL)
    {
        dvz_commands_destroy(upload);
        dvz_commands_free(upload);
    }
    if (staging != NULL)
    {
        dvz_buffer_destroy(staging);
        dvz_buffer_free(staging);
    }
    return -1;
}



/**
 * Record one draw into the canvas-owned command buffer.
 * @param canvas Borrowed canvas.
 * @param frame Borrowed current frame and its attachments.
 * @param user_data Renderer with resources that outlive this frame.
 */
static void draw(DvzCanvas* canvas, const DvzStreamFrame* frame, void* user_data)
{
    Renderer* renderer = (Renderer*)user_data;
    Push push = {0};
    DvzMVP mvp = {0};
    DvzResult camera_resize = dvz_camera_resize(
        renderer->camera, (float)frame->extent.width, (float)frame->extent.height);
    // Pointer positions use window coordinates, which may differ from framebuffer pixels.
    DvzInputResizeEvent input_size = {0};
    float pointer_width = (float)frame->extent.width;
    float pointer_height = (float)frame->extent.height;
    if (dvz_input_router_last_resize(dvz_canvas_input(canvas), &input_size) &&
        input_size.window_width > 0 && input_size.window_height > 0)
    {
        pointer_width = (float)input_size.window_width;
        pointer_height = (float)input_size.window_height;
    }
    DvzResult arcball_resize =
        dvz_arcball_resize(renderer->arcball, pointer_width, pointer_height);
    if (camera_resize != DVZ_OK || arcball_resize != DVZ_OK)
        renderer->draw_failed = true;
    dvz_camera_mvp(renderer->camera, &mvp);
    dvz_arcball_mvp(renderer->arcball, &mvp);
    // Camera matrices use OpenGL clip coordinates; convert y and z for this Vulkan viewport.
    mat4 clip_correction = {{1, 0, 0, 0}, {0, -1, 0, 0}, {0, 0, 0.5f, 0}, {0, 0, 0.5f, 1}};
    mat4 view_model = {{0}};
    mat4 projection_view_model = {{0}};
    multiply_mat4(mvp.view, mvp.model, view_model);
    multiply_mat4(mvp.proj, view_model, projection_view_model);
    multiply_mat4(clip_correction, projection_view_model, push.mvp);
    VkClearValue clear = {.color.float32 = {0.04f, 0.05f, 0.08f, 1.0f}};

    dvz_commands_wrap_borrowed_recording(
        renderer->device, frame->command_buffer, renderer->commands);
    dvz_cmd_rendering_default(
        renderer->commands, frame->image_view, frame->extent.width, frame->extent.height, clear,
        renderer->rendering);
    DvzAttachment* depth = dvz_rendering_depth(renderer->rendering);
    dvz_attachment_image(depth, frame->depth_view, frame->depth_layout);
    dvz_attachment_ops(depth, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
    dvz_attachment_clear(depth, (VkClearValue){.depthStencil = {1.0f, 0}});
    dvz_cmd_rendering_begin(renderer->commands, renderer->rendering);
    dvz_cmd_bind_graphics(renderer->commands, renderer->pipeline);
    dvz_cmd_set_viewport_scissor(renderer->commands, frame->extent);
    dvz_cmd_bind_descriptors(
        renderer->commands, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->descriptors, 0, 1, 0, NULL);
    DvzResult push_result = dvz_cmd_push_constants(
        renderer->commands, renderer->slots, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push), &push);
    if (push_result != DVZ_OK)
        renderer->draw_failed = true;
    DvzSize vertex_offset = 0;
    dvz_cmd_bind_vertex_buffers(renderer->commands, 0, 1, renderer->vertex_buffer, &vertex_offset);
    dvz_cmd_bind_index_buffer(renderer->commands, renderer->index_buffer, 0, VK_INDEX_TYPE_UINT16);
    dvz_cmd_draw_indexed(renderer->commands, 0, 0, 36, 0, 1);
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
    for (int argument_index = 1; argument_index + 1 < argc; argument_index++)
    {
        if (strcmp(argv[argument_index], "--png") == 0)
            png_path = argv[++argument_index];
    }
    bool live = png_path == NULL;
    DvzBackend backend = live ? DVZ_BACKEND_GLFW : DVZ_BACKEND_OFFSCREEN;
    DvzCanvasRenderMode mode =
        live ? DVZ_CANVAS_RENDER_MODE_PRESENT : DVZ_CANVAS_RENDER_MODE_OFFSCREEN;

    host = dvz_window_host();
    COURSE_CHECK(host != NULL, "window host creation failed");
    DvzGpuCtxConfig gpu_config = dvz_gpu_ctx_config();
    dvz_gpu_ctx_config_validation(&gpu_config, true);
    DvzResult configure_result = dvz_canvas_configure_gpu_ctx(host, backend, mode, &gpu_config);
    COURSE_CHECK(configure_result == DVZ_OK, "GPU configuration failed");
    gpu = dvz_gpu_ctx(&gpu_config);
    COURSE_CHECK(gpu != NULL, "no usable GPU found");

    DvzWindowConfig window_config = dvz_window_config();
    window_config.width = WIDTH;
    window_config.height = HEIGHT;
    window_config.title = "Texture sampling: drag to rotate, scroll to zoom, R to reload";
    window = dvz_window_create(host, backend, &window_config);
    COURSE_CHECK(window != NULL, "window creation failed");

    DvzCanvasConfig canvas_config = dvz_canvas_config();
    canvas_config.window = window;
    canvas_config.device = dvz_gpu_ctx_device(gpu);
    canvas_config.render_mode = mode;
    canvas_config.depth_format = VK_FORMAT_D32_SFLOAT;
    canvas = dvz_canvas_create(&canvas_config);
    COURSE_CHECK(canvas != NULL, "canvas creation failed");

    renderer.device = dvz_gpu_ctx_device(gpu);
    renderer.allocator = dvz_gpu_ctx_alloc(gpu);
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
    int texture_result = create_texture(&renderer);
    COURSE_CHECK(texture_result == 0, "texture upload failed");
    int pipeline_result = create_pipeline(&renderer);
    COURSE_CHECK(pipeline_result == 0, "graphics pipeline creation failed");
    DvzCameraDesc camera_desc = dvz_camera_desc();
    camera_desc.projection.fov_y = 1.0471976f;
    renderer.camera = dvz_camera_create(&camera_desc);
    renderer.arcball = dvz_arcball_create(NULL);
    COURSE_CHECK(
        renderer.camera != NULL && renderer.arcball != NULL, "controller creation failed");
    DvzResult initial_result = dvz_arcball_initial(renderer.arcball, (vec3){-0.35f, 0.65f, 0.0f});
    COURSE_CHECK(initial_result == DVZ_OK, "initial rotation failed");
    DvzResult connect_result = dvz_arcball_connect(renderer.arcball, dvz_canvas_input(canvas));
    COURSE_CHECK(connect_result == DVZ_OK, "arcball connection failed");
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
                .texture_view = renderer.texture_view,
                .sampler = renderer.sampler,
            };
            pipeline_result = create_pipeline(&candidate);
            if (pipeline_result == 0)
            {
                dvz_device_wait(renderer.device);
                destroy_pipeline(&renderer);
                renderer.descriptors = candidate.descriptors;
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
        COURSE_CHECK(!renderer.draw_failed, "matrix or push-constant update failed");
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
    destroy_pipeline(&renderer);
    if (renderer.sampler != NULL)
        dvz_sampler_destroy(renderer.sampler);
    dvz_sampler_free(renderer.sampler);
    if (renderer.texture_view != NULL)
        dvz_image_views_destroy(renderer.texture_view);
    dvz_image_views_free(renderer.texture_view);
    if (renderer.texture != NULL)
        dvz_images_destroy(renderer.texture);
    dvz_images_free(renderer.texture);
    if (renderer.index_buffer != NULL)
        dvz_buffer_destroy(renderer.index_buffer);
    dvz_buffer_free(renderer.index_buffer);
    if (renderer.vertex_buffer != NULL)
        dvz_buffer_destroy(renderer.vertex_buffer);
    dvz_buffer_free(renderer.vertex_buffer);
    if (renderer.arcball != NULL && canvas != NULL)
        dvz_arcball_disconnect(renderer.arcball, dvz_canvas_input(canvas));
    if (renderer.arcball != NULL)
        dvz_arcball_destroy(renderer.arcball);
    dvz_camera_destroy(renderer.camera);
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
