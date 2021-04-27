#ifndef DVZ_TEST_PROTO_HEADER
#define DVZ_TEST_PROTO_HEADER

#include "../include/datoviz/context.h"
#include "../include/datoviz/visuals.h"
#include "../include/datoviz/vklite.h"

BEGIN_INCL_NO_WARN
#include "../external/stb_image.h"
END_INCL_NO_WARN



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define DEBUG_TEST (getenv("DVZ_DEBUG") != NULL)

#define N_FRAMES (DEBUG_TEST ? 0 : 5)

#define TRIANGLE_VERTICES                                                                         \
    {                                                                                             \
        {{-1, +1, 0}, {1, 0, 0, 1}}, {{+1, +1, 0}, {0, 1, 0, 1}}, {{+0, -1, 0}, {0, 0, 1, 1}},    \
    }



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



static TestVisual triangle_visual(
    DvzGpu* gpu, DvzRenderpass* renderpass, DvzFramebuffers* framebuffers, const char* suffix)
{
    TestVisual visual = {0};
    visual.gpu = gpu;
    visual.renderpass = renderpass;
    visual.framebuffers = framebuffers;

    // Make the graphics.
    visual.graphics = triangle_graphics(gpu, renderpass, suffix);

    if (strncmp(suffix, "_push", 5) == 0)
        dvz_graphics_push(&visual.graphics, 0, sizeof(vec3), VK_SHADER_STAGE_VERTEX_BIT);

    // Create the bindings.
    visual.bindings = dvz_bindings(&visual.graphics.slots, 1);
    dvz_bindings_update(&visual.bindings);

    // Create the graphics pipeline.
    dvz_graphics_create(&visual.graphics);

    // Create the buffer.
    visual.buffer = dvz_buffer(gpu);
    VkDeviceSize size = 3 * sizeof(TestVertex);
    dvz_buffer_size(&visual.buffer, size);
    dvz_buffer_usage(
        &visual.buffer,                          //
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |      //
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | //
            VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    dvz_buffer_memory(
        &visual.buffer,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    dvz_buffer_create(&visual.buffer);

    // Upload the triangle data.
    TestVertex data[] = TRIANGLE_VERTICES;
    dvz_buffer_upload(&visual.buffer, 0, size, data);
    dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_TRANSFER);

    return visual;
}



static void triangle_commands(
    DvzCommands* cmds, uint32_t idx, DvzRenderpass* renderpass, DvzFramebuffers* framebuffers,
    DvzGraphics* graphics, DvzBindings* bindings, DvzBufferRegions br)
{
    ASSERT(renderpass != NULL);
    ASSERT(renderpass->renderpass != VK_NULL_HANDLE);

    ASSERT(framebuffers != NULL);
    ASSERT(framebuffers->framebuffers[0] != VK_NULL_HANDLE);

    ASSERT(graphics != NULL);
    ASSERT(graphics->pipeline != VK_NULL_HANDLE);

    ASSERT(bindings != NULL);
    ASSERT(bindings->dsets != VK_NULL_HANDLE);

    ASSERT(br.buffer != NULL);
    ASSERT(br.buffer->buffer != VK_NULL_HANDLE);

    uint32_t width = framebuffers->attachments[0]->width;
    uint32_t height = framebuffers->attachments[0]->height;
    uint32_t n_vertices = br.size / sizeof(TestVertex);
    n_vertices = n_vertices > 0 ? n_vertices : 3;
    ASSERT(n_vertices > 0);

    ASSERT(width > 0);
    ASSERT(height > 0);

    // Commands.
    dvz_cmd_begin(cmds, idx);
    dvz_cmd_begin_renderpass(cmds, idx, renderpass, framebuffers);
    dvz_cmd_viewport(cmds, idx, (VkViewport){0, 0, width, height, 0, 1});
    dvz_cmd_bind_vertex_buffer(cmds, idx, br, 0);
    dvz_cmd_bind_graphics(cmds, idx, graphics, bindings, 0);

    if (graphics->slots.push_count > 0)
        dvz_cmd_push(
            cmds, idx, &graphics->slots, VK_SHADER_STAGE_VERTEX_BIT, 0, //
            sizeof(vec3), graphics->user_data);

    dvz_cmd_draw(cmds, idx, 0, n_vertices);
    dvz_cmd_end_renderpass(cmds, idx);
    dvz_cmd_end(cmds, idx);
}



static void destroy_visual(TestVisual* visual)
{
    dvz_graphics_destroy(&visual->graphics);
    dvz_bindings_destroy(&visual->bindings);
    dvz_buffer_destroy(&visual->buffer);
    if (visual->user_data != NULL)
        FREE(visual->user_data);
}



/*************************************************************************************************/
/*  Test textures                                                                                */
/*************************************************************************************************/

static DvzTexture* _earth_texture(DvzContext* context)
{
    DvzGpu* gpu = context->gpu;
    char path[1024];
    snprintf(path, sizeof(path), "%s/textures/earth.jpg", DATA_DIR);
    int width, height, depth;
    uint8_t* tex_data = stbi_load(path, &width, &height, &depth, STBI_rgb_alpha);
    uint32_t tex_size = (uint32_t)(width * height);
    DvzTexture* texture = dvz_ctx_texture(
        gpu->context, 2, (uvec3){(uint32_t)width, (uint32_t)height, 1}, VK_FORMAT_R8G8B8A8_UNORM);
    dvz_upload_texture(
        context, texture, DVZ_ZERO_OFFSET, DVZ_ZERO_OFFSET, tex_size * sizeof(cvec4), tex_data);
    FREE(tex_data)
    return texture;
}


static DvzTexture* _synthetic_texture(DvzContext* context)
{
    ASSERT(context != NULL);

    // Texture.
    const uint32_t S = 1024;
    DvzTexture* texture = dvz_ctx_texture(context, 2, (uvec3){S, S, 1}, VK_FORMAT_R32_SFLOAT);
    VkDeviceSize size = S * S * sizeof(float);

    float* tex_data = malloc(size);
    double x = 0, y = 0;
    uint32_t k = 0;

    for (uint32_t i = 0; i < S; i++)
    {
        x = -1 + 2 * i / (double)(S - 1);
        for (uint32_t j = 0; j < S; j++)
        {
            y = +1 - 2 * j / (double)(S - 1);
            tex_data[k++] = exp(-2 * (x * x + y * y)) * cos(M_2PI * 3 * x) * sin(M_2PI * 3 * y);
        }
    }

    dvz_upload_texture(context, texture, DVZ_ZERO_OFFSET, DVZ_ZERO_OFFSET, size, tex_data);
    FREE(tex_data)

    return texture;
}



static DvzTexture* _mock_texture(DvzContext* context)
{
    ASSERT(context != NULL);

    DvzTexture* texture = dvz_ctx_texture(context, 2, (uvec3){2, 2, 1}, VK_FORMAT_R8G8B8A8_UNORM);
    cvec4 tex_data[] = {
        {255, 0, 0, 255}, //
        {0, 255, 0, 255},
        {0, 0, 255, 255},
        {255, 255, 0, 255},
    };
    dvz_upload_texture(
        context, texture, DVZ_ZERO_OFFSET, DVZ_ZERO_OFFSET, sizeof(tex_data), tex_data);
    return texture;
}



static DvzTexture* _volume_texture(DvzContext* context, int kind)
{
    const uint32_t S = 64;
    VkDeviceSize size = S * S * S * sizeof(uint8_t);
    DvzTexture* texture = dvz_ctx_texture(context, 3, (uvec3){S, S, S}, VK_FORMAT_R8_UNORM);
    uint8_t* tex_data = calloc(S * S * S, sizeof(uint8_t));
    uint32_t l = 0;
    double x, y, z, w;
    double c = S / 2;
    for (uint32_t i = 0; i < S; i++)
    {
        for (uint32_t j = 0; j < S; j++)
        {
            for (uint32_t k = 0; k < S; k++)
            {
                x = ((double)i - c) / c;
                y = ((double)j - c) / c;
                z = ((double)k - c) / c;
                w = exp(-4 * (x * x + y * y + z * z));

                if (kind == 0)
                    tex_data[l++] = TO_BYTE(w);
                else
                    tex_data[l++] = dvz_rand_byte() % 3;

                // tex_data[l++] = (i & j) | (i & k) | (j & k) ? 0 : 32;
            }
        }
    }
    dvz_upload_texture(context, texture, DVZ_ZERO_OFFSET, DVZ_ZERO_OFFSET, size, tex_data);
    FREE(tex_data);
    return texture;
}



/*************************************************************************************************/
/*  Test data                                                                                    */
/*************************************************************************************************/

static void _point_data(DvzVisual* visual)
{
    ASSERT(visual != NULL);

    // Create visual data.
    uint32_t n = 50;
    dvec3* pos = calloc(n, sizeof(dvec3));
    cvec4* color = calloc(n, sizeof(cvec4));
    double t = 0, r = .9;
    // double aspect = dvz_canvas_aspect(visual->canvas);
    for (uint32_t i = 0; i < n; i++)
    {
        t = i / (double)(n);
        pos[i][0] = r * cos(M_2PI * t);
        pos[i][1] = r * sin(M_2PI * t);
        dvz_colormap(DVZ_CMAP_HSV, TO_BYTE(t), color[i]);
        color[i][3] = 128;
    }

    // Set visual data.
    dvz_visual_data(visual, DVZ_PROP_POS, 0, n, pos);
    dvz_visual_data(visual, DVZ_PROP_COLOR, 0, n, color);

    // Free the arrays.
    FREE(pos);
    FREE(color);

    // Params.
    dvz_visual_data(visual, DVZ_PROP_MARKER_SIZE, 0, 1, (float[]){50});
}



#endif
