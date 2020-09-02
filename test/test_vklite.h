#include <visky/vklite.h>

#define N 20


/*************************************************************************************************/
/*  Common                                                                                       */
/*************************************************************************************************/

static int no_destroy(VkyCanvas* canvas) { return 0; }

// Resources used in tests below.
static VkyBufferRegion vbr;
static VkyGraphicsPipeline pipeline;
static VkyBuffer buffer;



/*************************************************************************************************/
/*  Test compute                                                                                 */
/*************************************************************************************************/

static int test_vklite_compute()
{
    VkyGpu gpu = vky_create_device(0, NULL);
    vky_prepare_gpu(&gpu, NULL);

    VkDeviceSize size = N * sizeof(float);
    buffer = vky_create_buffer(
        &gpu, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VkyBufferRegion buffer_region = vky_allocate_buffer(&buffer, size);

    float numbers[N];
    for (uint32_t i = 0; i < N; i++)
    {
        numbers[i] = (float)i + 1;
    }
    vky_upload_buffer(buffer_region, 0, size, numbers);

    VkyResourceLayout resource_layout = vky_create_resource_layout(&gpu, 1);
    vky_add_resource_binding(&resource_layout, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    VkyComputePipeline cpipeline =
        vky_create_compute_pipeline(&gpu, "pow2.comp.spv", resource_layout);

    VkyBufferRegion* resources[] = {&buffer_region};
    vky_bind_resources(cpipeline.resource_layout, cpipeline.descriptor_sets, (void**)resources);

    vky_begin_compute(&gpu);
    vky_compute(&cpipeline, N, 1, 1);
    vky_end_compute(&gpu, 0, NULL, NULL);
    vky_compute_submit(&gpu);
    vky_compute_wait(&gpu);

    vky_download_buffer(&buffer_region, numbers);

    for (uint32_t i = 0; i < N; i++)
    {
        if (numbers[i] != 2 * (i + 1))
            return 1;
    }

    vky_destroy_compute_pipeline(&cpipeline);
    vky_destroy_buffer(&buffer);
    vky_destroy_device(&gpu);

    return 0;
}



static void test_vklite_blank_fill(VkyCanvas* canvas, VkCommandBuffer cmd_buf)
{
    vky_begin_render_pass(cmd_buf, canvas, VKY_CLEAR_COLOR_BLACK);
    vky_end_render_pass(cmd_buf, canvas);
}

static int test_vklite_blank(VkyCanvas* canvas)
{
    // log_set_level_env();
    canvas->cb_fill_command_buffer = test_vklite_blank_fill;
    // vky_run_app(canvas->app);
    // vky_destroy_app(app);
    return 0;
}



/*************************************************************************************************/
/*  Test triangle                                                                                */
/*************************************************************************************************/

static void test_vklite_triangle_fill(VkyCanvas* canvas, VkCommandBuffer cmd_buf)
{
    vky_begin_render_pass(cmd_buf, canvas, (VkClearColorValue){{0, 0, 0, 0}});
    vky_bind_vertex_buffer(cmd_buf, vbr, 0);
    vky_bind_graphics_pipeline(cmd_buf, &pipeline);
    vky_set_viewport(
        cmd_buf, 0, 0, canvas->size.framebuffer_width, canvas->size.framebuffer_height);
    vky_draw(cmd_buf, 0, 3);
    vky_end_render_pass(cmd_buf, canvas);
}

static int test_vklite_triangle(VkyCanvas* canvas)
{
    canvas->cb_fill_command_buffer = test_vklite_triangle_fill;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "default.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "default.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout =
        vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyDefaultVertex));
    vky_add_vertex_attribute(
        &vertex_layout, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VkyDefaultVertex, pos));
    vky_add_vertex_attribute(
        &vertex_layout, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VkyDefaultVertex, color));
    VkyResourceLayout resource_layout =
        vky_create_resource_layout(canvas->gpu, canvas->image_count);

    // Create the graphics pipeline.
    pipeline = vky_create_graphics_pipeline(
        canvas,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, // triangles, 3 vertices per primitive
        shaders, vertex_layout, resource_layout, (VkyGraphicsPipelineParams){true});

    // Create the vertex buffer.
    VkDeviceSize size = 3 * sizeof(VkyDefaultVertex); // 3 vertices
    buffer = vky_create_buffer(canvas->gpu, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0);
    vbr = vky_allocate_buffer(&buffer, size);

    // Make the data and upload it to the GPU.
    VkyDefaultVertex data[3] = {
        {{-1, +1, 0}, {1, 0, 0, 1}}, // vec3 pos, vec4 color
        {{+1, +1, 0}, {0, 1, 0, 1}},
        {{+0, -1, 0}, {0, 0, 1, 1}},
    };
    vky_upload_buffer(vbr, 0, size, data);
    return 0;
}

static int test_vklite_triangle_destroy(VkyCanvas* canvas)
{
    vky_destroy_buffer(&buffer);
    vky_destroy_vertex_layout(&pipeline.vertex_layout);
    vky_destroy_resource_layout(&pipeline.resource_layout);
    vky_destroy_shaders(&pipeline.shaders);
    vky_destroy_graphics_pipeline(&pipeline);

    return 0;
}



/*************************************************************************************************/
/*  Test push constant                                                                           */
/*************************************************************************************************/

static void test_vklite_push_fill(VkyCanvas* canvas, VkCommandBuffer cmd_buf)
{
    // Begin the render pass.
    vky_begin_render_pass(cmd_buf, canvas, (VkClearColorValue){{0, 0, 0, 0}});

    // Bind the vertex buffer.
    vky_bind_vertex_buffer(cmd_buf, vbr, 0);

    // Bind the graphics pipeline.
    vky_bind_graphics_pipeline(cmd_buf, &pipeline);

    // Set the full viewport.
    vky_set_viewport(
        cmd_buf, 0, 0, canvas->size.framebuffer_width, canvas->size.framebuffer_height);

    // Push constants.
    vky_push_constants(cmd_buf, &pipeline, sizeof(vec2), (vec2){1, 1});

    // Draw 3 vertices = 1 triangle.
    vky_draw(cmd_buf, 0, 3);

    // End the render pass.
    vky_end_render_pass(cmd_buf, canvas);
}

static int test_vklite_push(VkyCanvas* canvas)
{
    // This callback function is called when the command buffers need to be recreated,
    // at initialization and resize.
    canvas->cb_fill_command_buffer = test_vklite_push_fill;

    // Shaders.
    VkyShaders shaders = vky_create_shaders(canvas->gpu);
    vky_add_shader(&shaders, VK_SHADER_STAGE_VERTEX_BIT, "push.vert.spv");
    vky_add_shader(&shaders, VK_SHADER_STAGE_FRAGMENT_BIT, "push.frag.spv");

    // Vertex layout.
    VkyVertexLayout vertex_layout =
        vky_create_vertex_layout(canvas->gpu, 0, sizeof(VkyDefaultVertex));

    // GLSL: layout (location = 0) in vec3 pos;
    vky_add_vertex_attribute(
        &vertex_layout, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VkyDefaultVertex, pos));

    // GLSL: layout (location = 1) in vec4 color;
    vky_add_vertex_attribute(
        &vertex_layout, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VkyDefaultVertex, color));

    // Resource layout. None here, but one could add uniform buffers, textures...
    VkyResourceLayout resource_layout =
        vky_create_resource_layout(canvas->gpu, canvas->image_count);

    // Create the graphics pipeline.
    pipeline = vky_create_graphics_pipeline(
        canvas,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, // triangles, 3 vertices per primitive
        shaders, vertex_layout, resource_layout,
        (VkyGraphicsPipelineParams){
            true,        // depth test
            sizeof(vec2) // push constant size, 0 if no push constants
        });

    // Create the vertex buffer.
    VkDeviceSize size = 3 * sizeof(VkyDefaultVertex); // 3 vertices
    // Typically one uses one giant vertex buffer for the whole application and allocates parts of
    // the buffer to different visuals.
    buffer = vky_create_buffer(canvas->gpu, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0);
    vbr = vky_allocate_buffer(&buffer, size);

    // Make the data and upload it to the GPU.
    VkyDefaultVertex data[3] = {
        {{-1, +1, 0}, {0, 0, 0, 1}}, // vec3 pos, vec4 color
        {{+1, +1, 0}, {0, 0, 0, 1}}, // NOTE: will be filled by the push constant
        {{+0, -1, 0}, {0, 0, 1, 1}},
    };
    vky_upload_buffer(vbr, 0, size, data);

    return 0;
}

static int test_vklite_push_destroy(VkyCanvas* canvas)
{
    // Destroy the resources.
    vky_destroy_buffer(&buffer);
    vky_destroy_vertex_layout(&pipeline.vertex_layout);
    vky_destroy_resource_layout(&pipeline.resource_layout);
    vky_destroy_shaders(&pipeline.shaders);
    vky_destroy_graphics_pipeline(&pipeline);

    return 0;
}
