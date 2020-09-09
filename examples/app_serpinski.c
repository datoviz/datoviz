#include <visky/visky.h>

#define POINT_COUNT 100000

typedef struct FractalParams FractalParams;
struct FractalParams
{
    float point_size;
    mat4 control_points;
};

static VkyDefaultVertex data[POINT_COUNT];

static VkyGraphicsPipeline pipeline;
static VkyBuffer buffer;
static VkyBufferRegion vertex_buffer;
static VkyUniformBuffer dynamic_buffer;
static VkyUniformBuffer params_buffer;

static void fill_command_buffer(VkyCanvas* canvas, VkCommandBuffer cmd_buf)
{
    // Begin the render pass.
    vky_begin_render_pass(cmd_buf, canvas, VKY_CLEAR_COLOR_BLACK);

    // Bind the vertex buffer.
    vky_bind_vertex_buffer(cmd_buf, vertex_buffer, 0);

    // Bind the graphics pipeline.
    vky_bind_graphics_pipeline(cmd_buf, &pipeline);

    // Bind the dynamic uniform with the panel's MVP.
    vky_bind_dynamic_uniform(
        cmd_buf, &pipeline, &dynamic_buffer, canvas->current_command_buffer_index, 0);

    // Set the full viewport.
    vky_set_viewport(
        cmd_buf, 0, 0, canvas->size.framebuffer_width, canvas->size.framebuffer_height);

    // Draw the vertices.
    vky_draw(cmd_buf, 0, POINT_COUNT);

    // End the render pass.
    vky_end_render_pass(cmd_buf, canvas);
}

int main()
{
    log_set_level_env();

    // Create the app and canvas.
    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);

    // This callback function is called when the command buffers need to be recreated,
    // at initialization and resize.
    canvas->cb_fill_command_buffer = fill_command_buffer;

    // Graphics pipeline.
    {
        // Shaders.
        VkyShaders shaders = vky_create_shaders(canvas->gpu);
        vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "serpinski.vert.spv");
        vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "serpinski.frag.spv");

        // Vertex layout.
        VkyVertexLayout vertex_layout =
            vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyDefaultVertex));

        // GLSL: layout (location = 0) in vec3 pos;
        vky_add_vertex_attribute(
            &vertex_layout, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VkyDefaultVertex, pos));

        // GLSL: layout (location = 1) in vec4 color;
        vky_add_vertex_attribute(
            &vertex_layout, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VkyDefaultVertex, color));

        // Resource layout.
        VkyResourceLayout resource_layout =
            vky_create_resource_layout(canvas->gpu, canvas->image_count);
        vky_add_resource_binding(&resource_layout, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
        vky_add_resource_binding(&resource_layout, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

        // Create the graphics pipeline.
        pipeline = vky_create_graphics_pipeline(
            canvas, VK_PRIMITIVE_TOPOLOGY_POINT_LIST, shaders, vertex_layout, resource_layout,
            (VkyGraphicsPipelineParams){true});
    }

    // Resource #1: MVP dynamic uniform buffer, normally handled by the Scene API.
    {
        dynamic_buffer = vky_create_dynamic_uniform_buffer(
            app->gpu, canvas->image_count, 1, VKY_MVP_BUFFER_SIZE);
        void* pointer = vky_get_dynamic_uniform_pointer(&dynamic_buffer, 0);
        ASSERT(pointer != NULL);

        // Update the 3 MVP matrices.
        mat4* p_model = (mat4*)pointer;
        glm_mat4_copy(GLM_MAT4_IDENTITY, *(p_model + 0)); // model
        glm_mat4_copy(GLM_MAT4_IDENTITY, *(p_model + 1)); // view
        glm_mat4_copy(GLM_MAT4_IDENTITY, *(p_model + 2)); // proj

        // Update the vec4 vector.
        vec4* p_viewport = (vec4*)(p_model + 3);
        vec4 vec_viewport = {
            0, 0, canvas->size.framebuffer_width, canvas->size.framebuffer_height};
        glm_vec4_copy(vec_viewport, *p_viewport);

        // Update the Window size.
        vec4 win_size = {
            (float)canvas->size.framebuffer_width, (float)canvas->size.framebuffer_height, 0, 0};
        glm_vec4_copy(win_size, *(p_viewport + 2));

        // Upload that to the GPU.
        for (uint32_t image_index = 0; image_index < canvas->image_count; image_index++)
        {
            vky_upload_dynamic_uniform_buffer(&dynamic_buffer, image_index);
        }
    }

    // Resource #2: Fractal shader UBO params.
    {
        FractalParams params = {0};
        params.point_size = 1.0f;
        // Control points.
        uint32_t N = 3;
        for (uint32_t i = 0; i < N; i++)
        {
            float angle = M_PI / 6 + 2 * M_PI * i / (float)N;
            params.control_points[i / 2][2 * (i % 2) + 0] = cos(angle);
            params.control_points[i / 2][2 * (i % 2) + 1] = sin(angle) + .2f;
        }
        VkDeviceSize params_size = sizeof(FractalParams);
        params_buffer = vky_create_uniform_buffer(app->gpu, canvas->image_count, params_size);
        void* p_params = malloc(params_size);
        memcpy(p_params, &params, params_size);
        for (uint32_t image_index = 0; image_index < canvas->image_count; image_index++)
        {
            vky_upload_uniform_buffer(&params_buffer, image_index, &params);
        }
    }

    // Resource binding (dynamic UBO for MVP first, params UBO second).
    {
        VkyUniformBuffer* resources[2] = {&dynamic_buffer, &params_buffer};
        vky_bind_resources(pipeline.resource_layout, pipeline.descriptor_sets, (void**)resources);
    }

    // Vertex buffer.
    {
        VkDeviceSize size = POINT_COUNT * sizeof(VkyDefaultVertex);
        buffer = vky_create_buffer(canvas->gpu, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0);
        vertex_buffer = vky_allocate_buffer(&buffer, size);
        for (uint32_t i = 0; i < POINT_COUNT; i++)
        {
            data[i] = (VkyDefaultVertex){
                {rand_float(), rand_float(), 0.0f},
                {1, 1, 1, 0.5f},
            };
        }
        vky_upload_buffer(vertex_buffer, 0, size, data);
    }

    // Main loop.
    vky_run_app(app);

    // Destroy the resources.
    vky_destroy_uniform_buffer(&params_buffer);
    vky_destroy_dynamic_uniform_buffer(&dynamic_buffer);
    vky_destroy_buffer(&buffer);
    vky_destroy_vertex_layout(&pipeline.vertex_layout);
    vky_destroy_resource_layout(&pipeline.resource_layout);
    vky_destroy_shaders(&pipeline.shaders);
    vky_destroy_graphics_pipeline(&pipeline);

    // Destroy the app and canvas.
    vky_destroy_app(app);
    return 0;
}
