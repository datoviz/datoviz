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
    // DvzMVP mvp;
    // vec3 eye, center, up;

    uvec3 n_vert_comp;
    DvzArray vertices;
    DvzArray indices;

    float param;
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

static void _common_bindings(TestGraphics* tg)
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



/*************************************************************************************************/
/*  Graphics tests                                                                               */
/*************************************************************************************************/

int test_graphics_point(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    DvzContext* context = tc->context;

    ASSERT(canvas != NULL);
    ASSERT(context != NULL);

    DvzGraphics* graphics = dvz_graphics_builtin(canvas, DVZ_GRAPHICS_POINT, 0);
    ASSERT(graphics != NULL);

    TestGraphics tg = {0};
    tg.canvas = canvas;
    tg.graphics = graphics;
    tg.vertices = dvz_array_struct(0, sizeof(DvzVertex));
    tg.indices = dvz_array_struct(0, sizeof(DvzIndex));

    uint32_t n = 20;
    tg.param = 100.0f;

    DvzGraphicsData data = dvz_graphics_data(graphics, &tg.vertices, &tg.indices, NULL);
    dvz_graphics_alloc(&data, n);
    uint32_t item_count = n;
    uint32_t vertex_count = tg.vertices.item_count;
    uint32_t index_count = tg.indices.item_count;
    tg.br_vert =
        dvz_ctx_buffers(context, DVZ_BUFFER_TYPE_VERTEX, 1, vertex_count * sizeof(DvzVertex));
    if (index_count > 0)
        tg.br_index = dvz_ctx_buffers(
            context, DVZ_BUFFER_TYPE_INDEX, 1, index_count * sizeof(DvzIndex));
    DvzVertex* vertices = tg.vertices.data;

    double t = 0;
    for (uint32_t i = 0; i < vertex_count; i++)
    {
        t = i / (float)(n);
        vertices[i].pos[0] = .75 * cos(M_2PI * t);
        vertices[i].pos[1] = .75 * sin(M_2PI * t);
        dvz_colormap(DVZ_CMAP_HSV, TO_BYTE(t), vertices[i].color);
    }
    ASSERT(item_count > 0);
    ASSERT(vertex_count > 0);
    ASSERT(index_count == 0 || index_count > 0);
    ASSERT(vertices != NULL);
    dvz_upload_buffer(
        context, tg.br_vert, 0, vertex_count* tg.vertices.item_size, tg.vertices.data);
    if (index_count > 0)
        dvz_upload_buffer(
            context, tg.br_index, 0, index_count* tg.indices.item_size, tg.indices.data);

    tg.br_params =
        dvz_ctx_buffers(context, DVZ_BUFFER_TYPE_UNIFORM, 1, sizeof(DvzGraphicsPointParams));
    _common_bindings(&tg);
    dvz_bindings_buffer(&tg.bindings, DVZ_USER_BINDING, tg.br_params);
    dvz_bindings_update(&tg.bindings);

    DvzGraphicsPointParams params = {.point_size = tg.param};
    dvz_upload_buffer(context, tg.br_params, 0, sizeof(DvzGraphicsPointParams), &params);

    tg.interact = dvz_interact_builtin(canvas, DVZ_INTERACT_PANZOOM);
    dvz_event_callback(canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _interact_callback, &tg);

    dvz_event_callback(canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _graphics_refill, &tg);
    dvz_app_run(tc->app, 0);

    dvz_array_destroy(&tg.vertices);
    dvz_array_destroy(&tg.indices);
    return 0;
}
