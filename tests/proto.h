#ifndef DVZ_TEST_PROTO_HEADER
#define DVZ_TEST_PROTO_HEADER

#include "../include/datoviz/vklite.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define N_FRAMES 5


/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct TestCanvas TestCanvas;
typedef struct TestVisual TestVisual;
typedef struct TestVertex TestVertex;

typedef void (*FillCallback)(TestCanvas*, DvzCommands*, uint32_t);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct TestCanvas
{
    DvzGpu* gpu;
    bool is_offscreen;

    DvzWindow* window;

    DvzRenderpass renderpass;
    DvzFramebuffers framebuffers;
    DvzSwapchain swapchain;

    DvzImages* images;
    DvzImages* depth;

    DvzCompute* compute;
    DvzBindings* bindings;
    DvzGraphics* graphics;
    DvzBufferRegions br;

    void* data;
};


struct TestVisual
{
    DvzGpu* gpu;
    DvzRenderpass* renderpass;
    DvzFramebuffers* framebuffers;
    DvzGraphics graphics;
    DvzCompute* compute;
    DvzBindings bindings;
    DvzBuffer buffer;
    DvzBufferRegions br;
    DvzBufferRegions br_u;
    uint32_t n_vertices;
    float dt;
    void* data;
    void* data_u;
    void* user_data;
};



struct TestVertex
{
    vec3 pos;
    vec4 color;
};



/*************************************************************************************************/
/*  Test graphics                                                                                */
/*************************************************************************************************/

static DvzGraphics triangle_graphics(DvzGpu* gpu, DvzRenderpass* renderpass, const char* suffix)
{
    DvzGraphics graphics = dvz_graphics(gpu);

    dvz_graphics_renderpass(&graphics, renderpass, 0);
    dvz_graphics_topology(&graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    dvz_graphics_polygon_mode(&graphics, VK_POLYGON_MODE_FILL);
    dvz_graphics_depth_test(&graphics, DVZ_DEPTH_TEST_ENABLE);

    char path[1024];
    snprintf(path, sizeof(path), "%s/test_triangle%s.vert.spv", SPIRV_DIR, suffix);
    dvz_graphics_shader(&graphics, VK_SHADER_STAGE_VERTEX_BIT, path);
    snprintf(path, sizeof(path), "%s/test_triangle%s.frag.spv", SPIRV_DIR, suffix);
    dvz_graphics_shader(&graphics, VK_SHADER_STAGE_FRAGMENT_BIT, path);
    dvz_graphics_vertex_binding(&graphics, 0, sizeof(TestVertex));
    dvz_graphics_vertex_attr(
        &graphics, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TestVertex, pos));
    dvz_graphics_vertex_attr(
        &graphics, 0, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(TestVertex, color));

    return graphics;
}



static void triangle_visual(TestCanvas* canvas, TestVisual* visual, const char* suffix)
{
    visual->gpu = canvas->gpu;
    visual->renderpass = &canvas->renderpass;
    visual->framebuffers = &canvas->framebuffers;

    // Make the graphics.
    visual->graphics = triangle_graphics(visual->gpu, visual->renderpass, suffix);

    // Create the bindings.
    visual->bindings = dvz_bindings(&visual->graphics.slots, 1);
    dvz_bindings_update(&visual->bindings);

    // Create the graphics pipeline.
    dvz_graphics_create(&visual->graphics);

    // Create the buffer.
    visual->buffer = dvz_buffer(canvas->gpu);
    VkDeviceSize size = 3 * sizeof(TestVertex);
    dvz_buffer_size(&visual->buffer, size);
    dvz_buffer_usage(
        &visual->buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    dvz_buffer_memory(
        &visual->buffer,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    dvz_buffer_create(&visual->buffer);

    // Upload the triangle data.
    TestVertex data[3] = {
        {{-1, +1, 0}, {1, 0, 0, 1}},
        {{+1, +1, 0}, {0, 1, 0, 1}},
        {{+0, -1, 0}, {0, 0, 1, 1}},
    };
    dvz_buffer_upload(&visual->buffer, 0, size, data);

    visual->br.buffer = &visual->buffer;
    visual->br.size = size;
    visual->br.count = 1;

    canvas->data = visual;
    canvas->br = visual->br;
    canvas->graphics = &visual->graphics;
    canvas->bindings = &visual->bindings;
}



static void triangle_commands(TestCanvas* canvas, DvzCommands* cmds, uint32_t idx)
{
    ASSERT(canvas->br.buffer != NULL);
    ASSERT(canvas->graphics != NULL);
    ASSERT(canvas->bindings != NULL);

    // Commands.
    dvz_cmd_begin(cmds, idx);
    dvz_cmd_begin_renderpass(cmds, idx, &canvas->renderpass, &canvas->framebuffers);
    dvz_cmd_viewport(
        cmds, idx,
        (VkViewport){
            0, 0, canvas->framebuffers.attachments[0]->width,
            canvas->framebuffers.attachments[0]->height, 0, 1});
    dvz_cmd_bind_vertex_buffer(cmds, idx, canvas->br, 0);
    dvz_cmd_bind_graphics(cmds, idx, canvas->graphics, canvas->bindings, 0);
    dvz_cmd_draw(cmds, idx, 0, 3);
    dvz_cmd_end_renderpass(cmds, idx);
    dvz_cmd_end(cmds, idx);
}



static void destroy_visual(TestVisual* visual)
{
    dvz_graphics_destroy(&visual->graphics);
    dvz_bindings_destroy(&visual->bindings);
    dvz_buffer_destroy(&visual->buffer);
}



#endif
