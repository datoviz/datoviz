/*************************************************************************************************/
/*  Testing utils                                                                                */
/*************************************************************************************************/

#ifndef DVZ_HEADER_TESTING_UTILS
#define DVZ_HEADER_TESTING_UTILS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "../src/render_utils.h"
#include "../src/vklite_utils.h"
#include "glfw_utils.h"
#include "request.h"
#include "scene/colormaps.h"
#include "scene/graphics.h"
#include "surface.h"
#include "test_resources.h"
#include "testing.h"
#include "vklite.h"
#include "window.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH        800
#define HEIGHT       600
#define DEBUG_TEST   (getenv("DVZ_DEBUG") != NULL)
#define N_FRAMES     (DEBUG_TEST ? 0 : 5)
#define PRESENT_MODE VK_PRESENT_MODE_IMMEDIATE_KHR

#define TRIANGLE_VERTICES                                                                         \
    {                                                                                             \
        {{-1, +1, 0}, {1, 0, 0, 1}}, {{+1, +1, 0}, {0, 1, 0, 1}}, {{+0, -1, 0}, {0, 0, 1, 1}},    \
    }



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct TestCanvas TestCanvas;
typedef struct TestVisual TestVisual;
typedef struct GraphicsWrapper GraphicsWrapper;
typedef struct TestVertex TestVertex;

typedef void (*FillCallback)(TestCanvas* canvas, DvzCommands* cmds, uint32_t cmd_idx);



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct TestCanvas
{
    DvzGpu* gpu;
    bool is_offscreen;

    DvzWindow* window;
    DvzSurface surface;

    DvzRenderpass renderpass;
    DvzFramebuffers framebuffers;
    DvzSwapchain swapchain;

    DvzImages* images;
    DvzImages* depth;

    DvzCompute* compute;
    DvzBindings* bindings;
    DvzGraphics* graphics;

    // NOTE: this is used in vklite:
    DvzBufferRegions br;

    // NOTE: this is used in canvas tests
    // DvzDat* dat;
    bool always_refill;

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

    // NOTE: this is used in vklite:
    DvzBufferRegions br;
    DvzBufferRegions br_u;

    // NOTE: this is used in canvas tests
    // DvzDat *dat, *dat_u;

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



struct GraphicsWrapper
{
    DvzId canvas_id, graphics_id, dat_id, mvp_id, viewport_id;
    DvzViewport viewport;
    DvzMVP mvp;
    void* data;
};



/*************************************************************************************************/
/*  Test canvas                                                                                  */
/*************************************************************************************************/

static TestCanvas offscreen_canvas(DvzGpu* gpu)
{
    TestCanvas canvas = {0};
    canvas.gpu = gpu;
    canvas.is_offscreen = true;

    // Make the renderpass.
    canvas.renderpass = offscreen_renderpass(gpu);

    // Color attachment
    DvzImages images_struct = dvz_images(canvas.renderpass.gpu, VK_IMAGE_TYPE_2D, 1);
    DvzImages* images = (DvzImages*)calloc(1, sizeof(DvzImages));
    ANN(images);
    *images = images_struct;
    dvz_images_format(images, canvas.renderpass.attachments[0].format);
    dvz_images_size(images, (uvec3){WIDTH, HEIGHT, 1});
    dvz_images_tiling(images, VK_IMAGE_TILING_OPTIMAL);
    dvz_images_usage(
        images, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    dvz_images_memory(images, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    dvz_images_aspect(images, VK_IMAGE_ASPECT_COLOR_BIT);
    dvz_images_layout(images, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    dvz_images_queue_access(images, 0);
    dvz_images_create(images);
    dvz_images_transition(images);
    canvas.images = images;

    // Depth attachment.
    DvzImages depth_struct = dvz_images(gpu, VK_IMAGE_TYPE_2D, 1);
    DvzImages* depth = (DvzImages*)calloc(1, sizeof(DvzImages));
    ANN(depth);
    *depth = depth_struct;
    make_depth(gpu, depth, WIDTH, HEIGHT);
    canvas.depth = depth;

    // Create renderpass.
    // dvz_renderpass_create(&canvas.renderpass);

    // Create framebuffers.
    canvas.framebuffers = dvz_framebuffers(canvas.renderpass.gpu);
    dvz_framebuffers_attachment(&canvas.framebuffers, 0, images);
    dvz_framebuffers_attachment(&canvas.framebuffers, 1, depth);
    dvz_framebuffers_create(&canvas.framebuffers, &canvas.renderpass);

    return canvas;
}



static TestCanvas desktop_canvas(DvzGpu* gpu, DvzWindow* window, DvzSurface surface)
{
    ANN(gpu);
    ANN(window);
    DvzHost* host = gpu->host;
    ANN(host);

    TestCanvas canvas = {0};
    canvas.is_offscreen = false;
    canvas.gpu = gpu;
    canvas.window = window;

    canvas.surface = surface;
    ASSERT(canvas.surface.surface != VK_NULL_HANDLE);

    // uint32_t framebuffer_width = 0, framebuffer_height = 0;
    // backend_get_framebuffer_size(window, &framebuffer_width, &framebuffer_height);
    // ASSERT(framebuffer_width > 0);
    // ASSERT(framebuffer_height > 0);

    // Make the renderpass.
    canvas.renderpass = desktop_renderpass(gpu);

    canvas.swapchain = dvz_swapchain(canvas.renderpass.gpu, canvas.surface.surface, 3);
    dvz_swapchain_format(&canvas.swapchain, VK_FORMAT_B8G8R8A8_UNORM);
    dvz_swapchain_present_mode(&canvas.swapchain, PRESENT_MODE);
    dvz_swapchain_create(&canvas.swapchain);
    canvas.images = canvas.swapchain.images;

    // Depth attachment.
    DvzImages depth_struct = dvz_images(gpu, VK_IMAGE_TYPE_2D, 1);
    DvzImages* depth = (DvzImages*)calloc(1, sizeof(DvzImages));
    ANN(depth);
    *depth = depth_struct;
    make_depth(gpu, depth, canvas.images->shape[0], canvas.images->shape[1]);
    canvas.depth = depth;

    // Create renderpass.
    // dvz_renderpass_create(renderpass);

    // Create framebuffers.
    canvas.framebuffers = dvz_framebuffers(canvas.renderpass.gpu);
    dvz_framebuffers_attachment(&canvas.framebuffers, 0, canvas.swapchain.images);
    dvz_framebuffers_attachment(&canvas.framebuffers, 1, depth);
    dvz_framebuffers_create(&canvas.framebuffers, &canvas.renderpass);

    return canvas;
}



static void canvas_destroy(TestCanvas* canvas)
{
    log_trace("destroy canvas");

    if (canvas->is_offscreen)
    {
        dvz_images_destroy(canvas->images);
        FREE(canvas->images);
    }

    dvz_images_destroy(canvas->depth);
    FREE(canvas->depth);

    dvz_swapchain_destroy(&canvas->swapchain);
    dvz_framebuffers_destroy(&canvas->framebuffers);
    dvz_renderpass_destroy(&canvas->renderpass);
    dvz_surface_destroy(canvas->gpu->host, canvas->surface);
    dvz_window_destroy(canvas->window);
}



/*************************************************************************************************/
/*  Test graphics                                                                                */
/*************************************************************************************************/

static DvzGraphics triangle_graphics(DvzGpu* gpu, DvzRenderpass* renderpass)
{
    DvzGraphics graphics = dvz_graphics(gpu);

    dvz_graphics_renderpass(&graphics, renderpass, 0);
    dvz_graphics_topology(&graphics, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    dvz_graphics_polygon_mode(&graphics, VK_POLYGON_MODE_FILL);
    dvz_graphics_depth_test(&graphics, DVZ_DEPTH_TEST_ENABLE);

    char path[1024];
    snprintf(path, sizeof(path), "%s/test_triangle.vert.spv", SPIRV_DIR);
    dvz_graphics_shader(&graphics, VK_SHADER_STAGE_VERTEX_BIT, path);
    snprintf(path, sizeof(path), "%s/test_triangle.frag.spv", SPIRV_DIR);
    dvz_graphics_shader(&graphics, VK_SHADER_STAGE_FRAGMENT_BIT, path);
    dvz_graphics_vertex_binding(&graphics, 0, sizeof(TestVertex), VK_VERTEX_INPUT_RATE_VERTEX);
    dvz_graphics_vertex_attr(
        &graphics, 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(TestVertex, pos));
    dvz_graphics_vertex_attr(
        &graphics, 0, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(TestVertex, color));

    return graphics;
}



static void triangle_commands(
    DvzCommands* cmds, uint32_t idx, DvzRenderpass* renderpass, DvzFramebuffers* framebuffers,
    DvzGraphics* graphics, DvzBindings* bindings, DvzBufferRegions br)
{
    ANN(renderpass);
    ASSERT(renderpass->renderpass != VK_NULL_HANDLE);

    ANN(framebuffers);
    ASSERT(framebuffers->framebuffers[0] != VK_NULL_HANDLE);

    ANN(graphics);
    ASSERT(graphics->pipeline != VK_NULL_HANDLE);

    ANN(bindings);
    ASSERT(bindings->dsets != VK_NULL_HANDLE);

    ANN(br.buffer);
    ASSERT(br.buffer->buffer != VK_NULL_HANDLE);

    uint32_t width = framebuffers->attachments[0]->shape[0];
    uint32_t height = framebuffers->attachments[0]->shape[1];
    uint32_t n_vertices = 3;
    // (uint32_t)(br.size / sizeof(TestVertex));
    // n_vertices = n_vertices > 0 ? n_vertices : 3;
    // log_debug("refill n vertices: %d", n_vertices);
    // ASSERT(n_vertices > 0);

    ASSERT(width > 0);
    ASSERT(height > 0);

    // Commands.
    dvz_cmd_begin(cmds, idx);
    dvz_cmd_begin_renderpass(cmds, idx, renderpass, framebuffers);
    dvz_cmd_viewport(cmds, idx, (VkViewport){0, 0, (float)width, (float)height, 0, 1});
    dvz_cmd_bind_vertex_buffer(cmds, idx, 1, (DvzBufferRegions[]){br}, (DvzSize[]){0});
    dvz_cmd_bind_graphics(cmds, idx, graphics, bindings, 0);

    if (graphics->slots.push_count > 0)
        dvz_cmd_push(
            cmds, idx, &graphics->slots, VK_SHADER_STAGE_VERTEX_BIT, 0, //
            sizeof(vec3), graphics->user_data);

    dvz_cmd_draw(cmds, idx, 0, n_vertices, 0, 1);
    dvz_cmd_end_renderpass(cmds, idx);
    dvz_cmd_end(cmds, idx);
}



/*************************************************************************************************/
/*  Graphics wrapper                                                                             */
/*************************************************************************************************/

static void
graphics_request(DvzRequester* rqr, const uint32_t n, GraphicsWrapper* wrapper, int flags)
{
    // Make a canvas creation request.
    DvzRequest req = dvz_create_canvas(rqr, WIDTH, HEIGHT, DVZ_DEFAULT_CLEAR_COLOR, flags);

    // Canvas id.
    wrapper->canvas_id = req.id;

    // Create a graphics.
    req = dvz_create_graphics(rqr, DVZ_GRAPHICS_POINT, 0);
    wrapper->graphics_id = req.id;

    // Create the vertex buffer dat.
    req = dvz_create_dat(
        rqr, DVZ_BUFFER_TYPE_VERTEX, n * sizeof(DvzGraphicsPointVertex),
        DVZ_DAT_FLAGS_PERSISTENT_STAGING);
    wrapper->dat_id = req.id;

    // Bind the vertex buffer dat to the graphics pipe.
    req = dvz_set_vertex(rqr, wrapper->graphics_id, wrapper->dat_id);

    // Binding #0: MVP.
    req = dvz_create_dat(rqr, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzMVP), DVZ_DAT_FLAGS_MAPPABLE);
    wrapper->mvp_id = req.id;

    req = dvz_bind_dat(rqr, wrapper->graphics_id, 0, wrapper->mvp_id);

    wrapper->mvp = dvz_mvp_default();
    req = dvz_upload_dat(rqr, wrapper->mvp_id, 0, sizeof(DvzMVP), &wrapper->mvp);

    // Binding #1: viewport.
    req = dvz_create_dat(
        rqr, DVZ_BUFFER_TYPE_UNIFORM, sizeof(DvzViewport), DVZ_DAT_FLAGS_PERSISTENT_STAGING);
    wrapper->viewport_id = req.id;

    req = dvz_bind_dat(rqr, wrapper->graphics_id, 1, wrapper->viewport_id);

    wrapper->viewport = dvz_viewport_default(WIDTH, HEIGHT);
    req = dvz_upload_dat(rqr, wrapper->viewport_id, 0, sizeof(DvzViewport), &wrapper->viewport);

    // Command buffer.
    req = dvz_record_begin(rqr, wrapper->canvas_id);

    req = dvz_record_viewport(rqr, wrapper->canvas_id, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);

    req = dvz_record_draw(rqr, wrapper->canvas_id, wrapper->graphics_id, 0, n, 0, 1);

    req = dvz_record_end(rqr, wrapper->canvas_id);
}



// NOTE: the caller needs to free the output pointer.
static void* graphics_scatter(DvzRequester* rqr, DvzId dat_id, const uint32_t n)
{
    // Upload the data.
    DvzGraphicsPointVertex* data =
        (DvzGraphicsPointVertex*)calloc(n, sizeof(DvzGraphicsPointVertex));
    double t = 0;
    double aspect = WIDTH / (double)HEIGHT;
    for (uint32_t i = 0; i < n; i++)
    {
        t = i / (double)(n);
        data[i].pos[0] = .5 * cos(M_2PI * t);
        data[i].pos[1] = aspect * .5 * sin(M_2PI * t);

        data[i].size = 50;

        dvz_colormap(DVZ_CMAP_HSV, TO_BYTE(t), data[i].color);
        data[i].color[3] = 128;
    }

    dvz_upload_dat(rqr, dat_id, 0, n * sizeof(DvzGraphicsPointVertex), data);
    return data;
}



#endif
