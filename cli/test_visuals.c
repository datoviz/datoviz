#include "test_visuals.h"
#include "../include/datoviz/visuals.h"
#include "../src/visuals_utils.h"
#include "utils.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _wait(DvzCanvas* canvas, DvzEvent ev) { dvz_sleep(500); }



/*************************************************************************************************/
/*  Visuals tests                                                                                */
/*************************************************************************************************/

int test_visuals_1(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);
    DvzContext* ctx = gpu->context;
    ASSERT(ctx != NULL);
    DvzVisual visual = dvz_visual(canvas);
    _marker_visual(&visual);

    // GPU sources.
    const uint32_t N = 10000;

    // Binding resources.
    DvzBufferRegions br_mvp = dvz_ctx_buffers(
        ctx, DVZ_BUFFER_TYPE_UNIFORM_MAPPABLE, canvas->swapchain.img_count, sizeof(DvzMVP));
    DvzBufferRegions br_viewport = dvz_ctx_buffers(ctx, DVZ_BUFFER_TYPE_UNIFORM, 1, 16);
    DvzBufferRegions br_params =
        dvz_ctx_buffers(ctx, DVZ_BUFFER_TYPE_UNIFORM, 1, sizeof(DvzGraphicsPointParams));

    // Binding data.
    DvzMVP mvp = {0};
    float param = 5.0f;
    DvzGraphicsPointParams params = {.point_size = param};
    {
        glm_mat4_identity(mvp.model);
        glm_mat4_identity(mvp.view);
        glm_mat4_identity(mvp.proj);
        dvz_upload_buffers(canvas, br_mvp, 0, sizeof(DvzMVP), &mvp);

        // Upload params.
        dvz_upload_buffers(canvas, br_params, 0, sizeof(DvzGraphicsPointParams), &params);
    }

    // Vertex data.
    DvzVertex* vertices = calloc(N, sizeof(DvzVertex));
    for (uint32_t i = 0; i < N; i++)
    {
        RANDN_POS(vertices[i].pos)
        RAND_COLOR(vertices[i].color)
    }

    // Set visual data.
    {
        // Via a GPU buffer.
        // DvzBufferRegions br_vert =
        //     dvz_ctx_buffers(ctx, DVZ_BUFFER_TYPE_VERTEX, 1, N * sizeof(DvzVertex));
        // dvz_upload_buffers(canvas, br_vert, 0, N * sizeof(DvzVertex), vertices);
        // visual.vertex_count = N;
        // dvz_visual_buffer(&visual, DVZ_SOURCE_TYPE_VERTEX, 0, br_vert);

        // Via user-provided data (underlying vertex buffer created automatically).
        dvz_visual_data_source(&visual, DVZ_SOURCE_TYPE_VERTEX, 0, 0, N, N, vertices);
    }

    // Set uniform buffers.
    dvz_visual_buffer(&visual, DVZ_SOURCE_TYPE_MVP, 0, br_mvp);
    dvz_visual_buffer(&visual, DVZ_SOURCE_TYPE_VIEWPORT, 0, br_viewport);
    dvz_visual_buffer(&visual, DVZ_SOURCE_TYPE_PARAM, 0, br_params);

    // Upload the data to the GPU.
    dvz_visual_update(&visual, canvas->viewport, (DvzDataCoords){0}, NULL);

    dvz_event_callback(
        canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _visual_canvas_fill, &visual);

    // Run and end.
    dvz_app_run(app, N_FRAMES);

    dvz_visual_destroy(&visual);
    FREE(vertices);
    TEST_END
}



int test_visuals_2(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);
    DvzContext* ctx = gpu->context;
    ASSERT(ctx != NULL);
    DvzVisual visual = dvz_visual(canvas);
    _marker_visual(&visual);

    // GPU sources.
    const uint32_t N = 10000;

    // Binding resources.
    DvzBufferRegions br_viewport = dvz_ctx_buffers(ctx, DVZ_BUFFER_TYPE_UNIFORM, 1, 16);

    // Vertex data.
    dvec3* pos = calloc(N, sizeof(dvec3));
    cvec4* color = calloc(N, sizeof(cvec4));
    for (uint32_t i = 0; i < N; i++)
    {
        RANDN_POS(pos[i])
        RAND_COLOR(color[i])
    }

    // Set visual data.
    dvz_visual_data(&visual, DVZ_PROP_POS, 0, N, pos);
    dvz_visual_data(&visual, DVZ_PROP_COLOR, 0, N, color);

    // MVP.
    mat4 id = GLM_MAT4_IDENTITY_INIT;
    dvz_visual_data(&visual, DVZ_PROP_MODEL, 0, 1, id);
    dvz_visual_data(&visual, DVZ_PROP_VIEW, 0, 1, id);
    dvz_visual_data(&visual, DVZ_PROP_PROJ, 0, 1, id);
    // Param.
    float param = 5.0f;
    dvz_visual_data(&visual, DVZ_PROP_MARKER_SIZE, 0, 1, &param);

    // GPU bindings.
    dvz_visual_buffer(&visual, DVZ_SOURCE_TYPE_VIEWPORT, 0, br_viewport);

    // Upload the data to the GPU manually.
    dvz_visual_update(&visual, canvas->viewport, (DvzDataCoords){0}, NULL);

    dvz_event_callback(
        canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _visual_canvas_fill, &visual);

    // Run and end.
    dvz_app_run(app, N_FRAMES);

    dvz_visual_destroy(&visual);
    FREE(pos);
    FREE(color);
    TEST_END
}



static DvzMVP mvp;
static DvzBufferRegions br_mvp;

static void _timer_callback(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    DvzVisual* visual = ev.user_data;
    ASSERT(visual != NULL);
    float t = .1 * ev.u.f.time;
    mvp.view[0][0] = cos(t);
    mvp.view[0][1] = sin(t);
    mvp.view[1][0] = -sin(t);
    mvp.view[1][1] = cos(t);
    dvz_upload_buffers(canvas, br_mvp, 0, br_mvp.size, &mvp);
}

int test_visuals_3(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);
    DvzContext* ctx = gpu->context;
    ASSERT(ctx != NULL);
    DvzVisual visual = dvz_visual(canvas);
    _marker_visual(&visual);

    // GPU sources.
    const uint32_t N = 10000;

    // Binding resources.
    DvzBufferRegions br_viewport = dvz_ctx_buffers(ctx, DVZ_BUFFER_TYPE_UNIFORM, 1, 16);

    // Vertex data.
    dvec3* pos = calloc(N, sizeof(dvec3));
    cvec4* color = calloc(N, sizeof(cvec4));
    for (uint32_t i = 0; i < N; i++)
    {
        RANDN_POS(pos[i])
        RAND_COLOR(color[i])
    }

    // Set visual data.
    dvz_visual_data(&visual, DVZ_PROP_POS, 0, N, pos);
    dvz_visual_data(&visual, DVZ_PROP_COLOR, 0, N, color);

    br_mvp = dvz_ctx_buffers(
        ctx, DVZ_BUFFER_TYPE_UNIFORM_MAPPABLE, canvas->swapchain.img_count, sizeof(DvzMVP));
    dvz_visual_buffer(&visual, DVZ_SOURCE_TYPE_MVP, 0, br_mvp);

    // MVP.
    mat4 id = GLM_MAT4_IDENTITY_INIT;
    glm_mat4_identity(mvp.model);
    glm_mat4_identity(mvp.view);
    glm_mat4_identity(mvp.proj);
    dvz_visual_data(&visual, DVZ_PROP_MODEL, 0, 1, id);
    dvz_visual_data(&visual, DVZ_PROP_VIEW, 0, 1, id);
    dvz_visual_data(&visual, DVZ_PROP_PROJ, 0, 1, id);

    // Param.
    float param = 5.0f;
    dvz_visual_data(&visual, DVZ_PROP_MARKER_SIZE, 0, 1, &param);

    // GPU bindings.
    dvz_visual_buffer(&visual, DVZ_SOURCE_TYPE_VIEWPORT, 0, br_viewport);

    // Upload the data to the GPU.
    dvz_visual_update(&visual, canvas->viewport, (DvzDataCoords){0}, NULL);

    dvz_event_callback(
        canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _visual_canvas_fill, &visual);
    dvz_event_callback(canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _timer_callback, &visual);

    // Run and end.
    dvz_app_run(app, N_FRAMES);

    dvz_visual_destroy(&visual);
    FREE(pos);
    FREE(color);
    // FREE(colormaps);
    TEST_END
}



static void _visual_update(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    DvzVisual* visual = ev.user_data;
    ASSERT(visual != NULL);

    const uint32_t N = 2 + (ev.u.t.idx % 10);
    dvec3* pos = calloc(N, sizeof(dvec3));
    for (uint32_t i = 0; i < N; i++)
    {
        pos[i][0] = -.75 + 1.5 / (N - 1) * i;
    }
    dvz_visual_data(visual, DVZ_PROP_POS, 0, N, pos);
    dvz_visual_update(visual, canvas->viewport, (DvzDataCoords){0}, NULL);

    // Need explicit refill because the number of vertices changes.
    dvz_canvas_to_refill(visual->canvas);
    FREE(pos);
}

int test_visuals_4(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);
    DvzContext* ctx = gpu->context;
    ASSERT(ctx != NULL);
    DvzVisual visual = dvz_visual(canvas);
    _marker_visual(&visual);

    // Vertex data.
    const uint32_t N = 5;
    dvec3* pos = calloc(N, sizeof(dvec3));
    cvec4* color = calloc(N, sizeof(cvec4));
    for (uint32_t i = 0; i < N; i++)
    {
        pos[i][0] = -.75 + 1.5 / (N - 1) * i;
        color[i][0] = 255;
        color[i][3] = 255;
    }

    // Set visual data.
    dvz_visual_data(&visual, DVZ_PROP_POS, 0, N, pos);
    dvz_visual_data(&visual, DVZ_PROP_COLOR, 0, N, color);

    // MVP.
    mat4 id = GLM_MAT4_IDENTITY_INIT;
    dvz_visual_data(&visual, DVZ_PROP_MODEL, 0, 1, id);
    dvz_visual_data(&visual, DVZ_PROP_VIEW, 0, 1, id);
    dvz_visual_data(&visual, DVZ_PROP_PROJ, 0, 1, id);

    // Param.
    float param = 50.0f;
    dvz_visual_data(&visual, DVZ_PROP_MARKER_SIZE, 0, 1, &param);

    // Upload the data to the GPU..
    dvz_visual_data_source(&visual, DVZ_SOURCE_TYPE_VIEWPORT, 0, 0, 1, 1, &canvas->viewport);
    dvz_visual_update(&visual, canvas->viewport, (DvzDataCoords){0}, NULL);

    dvz_event_callback(canvas, DVZ_EVENT_TIMER, .1, DVZ_EVENT_MODE_SYNC, _visual_update, &visual);
    dvz_event_callback(
        canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _visual_canvas_fill, &visual);

    // Run and end.
    dvz_app_run(app, N_FRAMES);

    dvz_visual_destroy(&visual);
    FREE(pos);
    FREE(color);
    TEST_END
}



static void _append(DvzVisual* visual)
{
    ASSERT(visual != NULL);
    dvec3 pos = {0};
    cvec4 color = {0};
    RANDN_POS(pos);
    RAND_COLOR(color);
    dvz_visual_data_append(visual, DVZ_PROP_POS, 0, 1, &pos);
    dvz_visual_data_append(visual, DVZ_PROP_COLOR, 0, 1, &color);
}

static void _visual_append(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    DvzVisual* visual = ev.user_data;
    ASSERT(visual != NULL);
    _append(visual);
    dvz_visual_update(visual, visual->canvas->viewport, (DvzDataCoords){0}, NULL);
    dvz_canvas_to_refill(visual->canvas);
}

int test_visuals_5(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);
    DvzContext* ctx = gpu->context;
    ASSERT(ctx != NULL);
    DvzVisual visual = dvz_visual(canvas);
    _marker_visual(&visual);
    _append(&visual);

    // MVP.
    mat4 id = GLM_MAT4_IDENTITY_INIT;
    dvz_visual_data(&visual, DVZ_PROP_MODEL, 0, 1, id);
    dvz_visual_data(&visual, DVZ_PROP_VIEW, 0, 1, id);
    dvz_visual_data(&visual, DVZ_PROP_PROJ, 0, 1, id);

    // Param.
    float param = 50.0f;
    dvz_visual_data(&visual, DVZ_PROP_MARKER_SIZE, 0, 1, &param);

    // Upload the data to the GPU..
    dvz_visual_data_source(&visual, DVZ_SOURCE_TYPE_VIEWPORT, 0, 0, 1, 1, &canvas->viewport);
    dvz_visual_update(&visual, canvas->viewport, (DvzDataCoords){0}, NULL);

    dvz_event_callback(canvas, DVZ_EVENT_TIMER, .05, DVZ_EVENT_MODE_SYNC, _visual_append, &visual);
    // dvz_event_callback(canvas, DVZ_EVENT_FRAME, 0, _wait, &visual);
    dvz_event_callback(
        canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _visual_canvas_fill, &visual);

    // Run and end.
    dvz_app_run(app, N_FRAMES);

    dvz_visual_destroy(&visual);
    TEST_END
}
