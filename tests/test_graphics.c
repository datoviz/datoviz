#include "../include/datoviz/graphics.h"
#include "../include/datoviz/interact.h"
#include "proto.h"
#include "tests.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct TestGraphics TestGraphics;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct TestGraphics
{
    DvzCanvas* canvas;
    DvzGraphics* graphics;
    DvzCompute* compute;

    DvzBufferRegions br_vert;
    DvzBufferRegions br_vert_comp;
    DvzBufferRegions br_index;
    DvzBufferRegions br_mvp;
    DvzBufferRegions br_viewport;
    DvzBufferRegions br_params;
    DvzBufferRegions br_comp;

    DvzTexture* texture;
    DvzBindings bindings;
    DvzBindings bindings_comp;

    DvzInteract interact;

    uint32_t item_count;
    uvec3 n_vert_comp;
    DvzArray vertices;
    DvzArray indices;

    // float param;
    void* data;
    void* params_data;
};



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _graphics_refill(DvzCanvas* canvas, DvzEvent ev)
{
    TestGraphics* tg = (TestGraphics*)ev.user_data;
    DvzCommands* cmds = ev.u.rf.cmds[0];
    DvzBufferRegions* br = &tg->br_vert;
    DvzBufferRegions* br_index = &tg->br_index;
    DvzBindings* bindings = &tg->bindings;
    DvzGraphics* graphics = tg->graphics;
    uint32_t idx = ev.u.rf.img_idx;

    // Commands.
    dvz_cmd_begin(cmds, idx);
    dvz_cmd_begin_renderpass(cmds, idx, &canvas->renderpass, &canvas->framebuffers);
    dvz_cmd_viewport(cmds, idx, canvas->viewport.viewport);
    dvz_cmd_bind_vertex_buffer(cmds, idx, *br, 0);
    if (br_index->buffer != NULL)
        dvz_cmd_bind_index_buffer(cmds, idx, *br_index, 0);
    dvz_cmd_bind_graphics(cmds, idx, graphics, bindings, 0);
    if (graphics->pipeline != VK_NULL_HANDLE)
    {
        if (br_index->buffer != VK_NULL_HANDLE)
        {
            log_debug("draw indexed %d", tg->indices.item_count);
            dvz_cmd_draw_indexed(cmds, idx, 0, 0, tg->indices.item_count);
        }
        else
        {
            log_debug("draw non-indexed %d", tg->vertices.item_count);
            dvz_cmd_draw(cmds, idx, 0, tg->vertices.item_count);
        }
    }
    dvz_cmd_end_renderpass(cmds, idx);
    dvz_cmd_end(cmds, idx);
}

static void _graphics_bindings(TestGraphics* tg)
{
    DvzGpu* gpu = tg->graphics->gpu;
    DvzGraphics* graphics = tg->graphics;
    DvzCanvas* canvas = tg->canvas;
    DvzContext* context = gpu->context;

    ASSERT(gpu != NULL);
    ASSERT(graphics != NULL);
    ASSERT(canvas != NULL);
    ASSERT(context != NULL);

    // Create the bindings.
    tg->bindings = dvz_bindings(&graphics->slots, canvas->swapchain.img_count);

    // Binding resources.
    tg->br_mvp = dvz_ctx_buffers(
        context, DVZ_BUFFER_TYPE_UNIFORM_MAPPABLE, canvas->swapchain.img_count,
        sizeof(DvzMVP));
    tg->br_viewport =
        dvz_ctx_buffers(context, DVZ_BUFFER_TYPE_UNIFORM, 1, sizeof(DvzViewport));

    // Bindings
    dvz_bindings_buffer(&tg->bindings, 0, tg->br_mvp);
    dvz_bindings_buffer(&tg->bindings, 1, tg->br_viewport);
}

static void _interact_callback(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    TestGraphics* tg = ev.user_data;
    ASSERT(tg != NULL);

    dvz_interact_update(&tg->interact, canvas->viewport, &canvas->mouse, &canvas->keyboard);
    dvz_canvas_buffers(canvas, tg->br_mvp, 0, sizeof(DvzMVP), &tg->interact.mvp);
}

static void _graphics_create(
    TestGraphics* tg, VkDeviceSize size, uint32_t n, DvzInteractType interact_type)
{
    ASSERT(tg != NULL);

    DvzContext* context = tg->canvas->gpu->context;
    ASSERT(context != NULL);

    tg->interact = dvz_interact_builtin(tg->canvas, interact_type);
    tg->vertices = dvz_array_struct(0, sizeof(DvzVertex));
    tg->indices = dvz_array_struct(0, sizeof(DvzIndex));

    DvzGraphicsData data = dvz_graphics_data(tg->graphics, &tg->vertices, &tg->indices, NULL);
    dvz_graphics_alloc(&data, n);
    tg->item_count = n;
    uint32_t item_count = n;
    uint32_t vertex_count = tg->vertices.item_count;
    uint32_t index_count = tg->indices.item_count;

    ASSERT(item_count > 0);
    ASSERT(vertex_count > 0);
    ASSERT(index_count == 0 || index_count > 0);
    ASSERT(tg->vertices.data != NULL);

    tg->br_vert =
        dvz_ctx_buffers(context, DVZ_BUFFER_TYPE_VERTEX, 1, vertex_count * sizeof(DvzVertex));
    if (index_count > 0)
        tg->br_index = dvz_ctx_buffers(
            context, DVZ_BUFFER_TYPE_INDEX, 1, index_count * sizeof(DvzIndex));
}

static void _graphics_upload(TestGraphics* tg)
{
    ASSERT(tg != NULL);

    DvzContext* context = tg->canvas->gpu->context;
    ASSERT(context != NULL);

    uint32_t vertex_count = tg->vertices.item_count;
    uint32_t index_count = tg->indices.item_count;

    dvz_upload_buffer(
        context, tg->br_vert, 0, vertex_count * tg->vertices.item_size, tg->vertices.data);
    if (index_count > 0)
        dvz_upload_buffer(
            context, tg->br_index, 0, index_count * tg->indices.item_size, tg->indices.data);
}

static void _graphics_params(TestGraphics* tg, VkDeviceSize size, void* data)
{
    ASSERT(tg != NULL);

    DvzContext* context = tg->canvas->gpu->context;
    ASSERT(context != NULL);

    tg->br_params =
        dvz_ctx_buffers(context, DVZ_BUFFER_TYPE_UNIFORM, 1, size);
    dvz_bindings_buffer(&tg->bindings, DVZ_USER_BINDING, tg->br_params);
    dvz_upload_buffer(context, tg->br_params, 0, size, data);
}

static void _graphics_run(TestGraphics* tg, uint32_t n_frames)
{
    ASSERT(tg != NULL);
    DvzCanvas* canvas = tg->canvas;
    ASSERT(canvas != NULL);

    dvz_bindings_update(&tg->bindings);
    dvz_event_callback(canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _interact_callback, tg);
    dvz_event_callback(canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _graphics_refill, tg);
    dvz_app_run(canvas->app, n_frames);

    dvz_array_destroy(&tg->vertices);
    dvz_array_destroy(&tg->indices);
}

static int _graphics_screenshot(TestGraphics* tg, const char* name)
{
    ASSERT(tg != NULL);
    ASSERT(tg->canvas != NULL);

    char path[1024];
    snprintf(path, sizeof(path), "test_graphics_%s", name);
    int res = check_canvas(tg->canvas, path);
    snprintf(path, sizeof(path), "%s/docs/images/graphics/%s.png", ROOT_DIR, name);
    dvz_screenshot_file(tg->canvas, path);
    return res;
}



/*************************************************************************************************/
/*  Graphics tests                                                                               */
/*************************************************************************************************/

int test_graphics_point(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    DvzContext* context = tc->context;

    ASSERT(canvas != NULL);
    ASSERT(context != NULL);

    // Create the graphics pipeline.
    DvzGraphics* graphics = dvz_graphics_builtin(canvas, DVZ_GRAPHICS_POINT, 0);
    ASSERT(graphics != NULL);

    // Vertex count and params.
    uint32_t n = 20;
    DvzGraphicsPointParams params = {.point_size = 50};

    // Create the graphics struct.
    TestGraphics tg = {.canvas = canvas, .graphics = graphics};
    _graphics_create(&tg, sizeof(DvzVertex), n, DVZ_INTERACT_PANZOOM);

    // Graphics data.
    DvzVertex* vertices = tg.vertices.data;
    double t = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        t = i / (float)(n);
        vertices[i].pos[0] = .75 * cos(M_2PI * t);
        vertices[i].pos[1] = .75 * sin(M_2PI * t);
        dvz_colormap(DVZ_CMAP_HSV, TO_BYTE(t), vertices[i].color);
    }
    _graphics_upload(&tg);

    // Graphics bindings.
    _graphics_bindings(&tg);
    _graphics_params(&tg, sizeof(DvzGraphicsPointParams), &params);

    // Run the test.
    _graphics_run(&tg, N_FRAMES);

    // Check screenshot and save it for the documentation.
    int res = _graphics_screenshot(&tg, "point");

    return res;
}
