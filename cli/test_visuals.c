#include "test_visuals.h"
#include "../include/visky/visuals.h"
#include "utils.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Graphics tests                                                                               */
/*************************************************************************************************/

int test_visuals_1(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);
    VklContext* ctx = gpu->context;
    ASSERT(ctx != NULL);
    VklVisual visual = vkl_visual(canvas);
    _marker_visual(&visual);

    // GPU sources.
    const uint32_t N = 10000;

    // Binding resources.
    VklBufferRegions br_mvp =
        vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_UNIFORM_MAPPABLE, 1, sizeof(VklMVP));
    VklBufferRegions br_viewport = vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_UNIFORM, 1, 16);
    VklBufferRegions br_params =
        vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_UNIFORM, 1, sizeof(VklGraphicsPointParams));

    // Binding data.
    VklMVP mvp = {0};
    float param = 5.0f;
    VklGraphicsPointParams params = {.point_size = param};
    {
        glm_mat4_identity(mvp.model);
        glm_mat4_identity(mvp.view);
        glm_mat4_identity(mvp.proj);
        vkl_upload_buffers(ctx, br_mvp, 0, sizeof(VklMVP), &mvp);

        // Upload params.
        vkl_upload_buffers(ctx, br_params, 0, sizeof(VklGraphicsPointParams), &params);
    }

    // Vertex data.
    VklVertex* vertices = calloc(N, sizeof(VklVertex));
    for (uint32_t i = 0; i < N; i++)
    {
        RANDN_POS(vertices[i].pos)
        RAND_COLOR(vertices[i].color)
    }

    // Set visual data.
    {
        // Via a GPU buffer.
        // VklBufferRegions br_vert =
        //     vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_VERTEX, 1, N * sizeof(VklVertex));
        // vkl_upload_buffers(ctx, br_vert, 0, N * sizeof(VklVertex), vertices);
        // visual.vertex_count = N;
        // vkl_visual_buffer(&visual, VKL_SOURCE_TYPE_VERTEX, 0, br_vert);

        // Via user-provided data (underlying vertex buffer created automatically).
        vkl_visual_data_buffer(&visual, VKL_SOURCE_TYPE_VERTEX, 0, 0, N, N, vertices);
    }

    // Set uniform buffers.
    vkl_visual_buffer(&visual, VKL_SOURCE_TYPE_MVP, 0, br_mvp);
    vkl_visual_buffer(&visual, VKL_SOURCE_TYPE_VIEWPORT, 0, br_viewport);
    vkl_visual_buffer(&visual, VKL_SOURCE_TYPE_PARAM, 0, br_params);

    // Upload the data to the GPU.
    vkl_visual_update(&visual, canvas->viewport, (VklDataCoords){0}, NULL);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _visual_canvas_fill, &visual);

    // Run and end.
    vkl_app_run(app, N_FRAMES);

    vkl_visual_destroy(&visual);
    FREE(vertices);
    TEST_END
}



int test_visuals_2(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);
    VklContext* ctx = gpu->context;
    ASSERT(ctx != NULL);
    VklVisual visual = vkl_visual(canvas);
    _marker_visual(&visual);

    // GPU sources.
    const uint32_t N = 10000;

    // Binding resources.
    VklBufferRegions br_viewport = vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_UNIFORM, 1, 16);

    // Vertex data.
    vec3* pos = calloc(N, sizeof(vec3));
    cvec4* color = calloc(N, sizeof(cvec4));
    for (uint32_t i = 0; i < N; i++)
    {
        RANDN_POS(pos[i])
        RAND_COLOR(color[i])
    }

    // Set visual data.
    vkl_visual_data(&visual, VKL_PROP_POS, 0, N, pos);
    vkl_visual_data(&visual, VKL_PROP_COLOR, 0, N, color);

    // MVP.
    mat4 id = GLM_MAT4_IDENTITY_INIT;
    vkl_visual_data(&visual, VKL_PROP_MODEL, 0, 1, id);
    vkl_visual_data(&visual, VKL_PROP_VIEW, 0, 1, id);
    vkl_visual_data(&visual, VKL_PROP_PROJ, 0, 1, id);

    // Param.
    float param = 5.0f;
    vkl_visual_data(&visual, VKL_PROP_MARKER_SIZE, 0, 1, &param);

    // GPU bindings.
    vkl_visual_buffer(&visual, VKL_SOURCE_TYPE_VIEWPORT, 0, br_viewport);

    // Upload the data to the GPU..
    vkl_visual_update(&visual, canvas->viewport, (VklDataCoords){0}, NULL);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _visual_canvas_fill, &visual);

    // Run and end.
    vkl_app_run(app, N_FRAMES);

    vkl_visual_destroy(&visual);
    FREE(pos);
    FREE(color);
    TEST_END
}



static VklMVP mvp;
static VklBufferRegions br_mvp;

static void _timer_callback(VklCanvas* canvas, VklPrivateEvent ev)
{
    ASSERT(canvas != NULL);
    VklVisual* visual = ev.user_data;
    ASSERT(visual != NULL);
    float t = .1 * ev.u.f.time;
    mvp.view[0][0] = cos(t);
    mvp.view[0][1] = sin(t);
    mvp.view[1][0] = -sin(t);
    mvp.view[1][1] = cos(t);
    vkl_upload_buffers_immediate(canvas, br_mvp, true, 0, br_mvp.size, &mvp);
}

int test_visuals_3(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);
    VklContext* ctx = gpu->context;
    ASSERT(ctx != NULL);
    VklVisual visual = vkl_visual(canvas);
    _marker_visual(&visual);

    // GPU sources.
    const uint32_t N = 10000;

    // Binding resources.
    VklBufferRegions br_viewport = vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_UNIFORM, 1, 16);

    // Vertex data.
    vec3* pos = calloc(N, sizeof(vec3));
    cvec4* color = calloc(N, sizeof(cvec4));
    for (uint32_t i = 0; i < N; i++)
    {
        RANDN_POS(pos[i])
        RAND_COLOR(color[i])
    }

    // Set visual data.
    vkl_visual_data(&visual, VKL_PROP_POS, 0, N, pos);
    vkl_visual_data(&visual, VKL_PROP_COLOR, 0, N, color);

    br_mvp = vkl_ctx_buffers(
        ctx, VKL_DEFAULT_BUFFER_UNIFORM_MAPPABLE, canvas->swapchain.img_count, sizeof(VklMVP));
    vkl_visual_buffer(&visual, VKL_SOURCE_TYPE_MVP, 0, br_mvp);

    // MVP.
    mat4 id = GLM_MAT4_IDENTITY_INIT;
    glm_mat4_identity(mvp.model);
    glm_mat4_identity(mvp.view);
    glm_mat4_identity(mvp.proj);
    vkl_visual_data(&visual, VKL_PROP_MODEL, 0, 1, id);
    vkl_visual_data(&visual, VKL_PROP_VIEW, 0, 1, id);
    vkl_visual_data(&visual, VKL_PROP_PROJ, 0, 1, id);

    // Param.
    float param = 5.0f;
    vkl_visual_data(&visual, VKL_PROP_MARKER_SIZE, 0, 1, &param);

    // GPU bindings.
    vkl_visual_buffer(&visual, VKL_SOURCE_TYPE_VIEWPORT, 0, br_viewport);

    // Upload the data to the GPU.
    vkl_visual_update(&visual, canvas->viewport, (VklDataCoords){0}, NULL);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _visual_canvas_fill, &visual);
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_TIMER, 1.0 / 60, _timer_callback, &visual);

    // Run and end.
    vkl_app_run(app, N_FRAMES);

    vkl_visual_destroy(&visual);
    FREE(pos);
    FREE(color);
    // FREE(colormaps);
    TEST_END
}



static void _visual_update(VklCanvas* canvas, VklPrivateEvent ev)
{
    ASSERT(canvas != NULL);
    VklVisual* visual = ev.user_data;
    ASSERT(visual != NULL);

    const uint32_t N = 2 + (ev.u.t.idx % 10);
    vec3* pos = calloc(N, sizeof(vec3));
    for (uint32_t i = 0; i < N; i++)
    {
        pos[i][0] = -.75 + 1.5 / (N - 1) * i;
    }
    vkl_visual_data(visual, VKL_PROP_POS, 0, N, pos);
    vkl_visual_update(visual, canvas->viewport, (VklDataCoords){0}, NULL);

    vkl_canvas_to_refill(visual->canvas, true);
    FREE(pos);
}

int test_visuals_4(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);
    VklContext* ctx = gpu->context;
    ASSERT(ctx != NULL);
    VklVisual visual = vkl_visual(canvas);
    _marker_visual(&visual);

    // Vertex data.
    const uint32_t N = 5;
    vec3* pos = calloc(N, sizeof(vec3));
    cvec4* color = calloc(N, sizeof(cvec4));
    for (uint32_t i = 0; i < N; i++)
    {
        pos[i][0] = -.75 + 1.5 / (N - 1) * i;
        color[i][0] = 255;
        color[i][3] = 255;
    }

    // Set visual data.
    vkl_visual_data(&visual, VKL_PROP_POS, 0, N, pos);
    vkl_visual_data(&visual, VKL_PROP_COLOR, 0, N, color);

    // MVP.
    mat4 id = GLM_MAT4_IDENTITY_INIT;
    vkl_visual_data(&visual, VKL_PROP_MODEL, 0, 1, id);
    vkl_visual_data(&visual, VKL_PROP_VIEW, 0, 1, id);
    vkl_visual_data(&visual, VKL_PROP_PROJ, 0, 1, id);

    // Param.
    float param = 50.0f;
    vkl_visual_data(&visual, VKL_PROP_MARKER_SIZE, 0, 1, &param);

    // Upload the data to the GPU..
    vkl_visual_data_buffer(&visual, VKL_SOURCE_TYPE_VIEWPORT, 0, 0, 1, 1, &canvas->viewport);
    vkl_visual_update(&visual, canvas->viewport, (VklDataCoords){0}, NULL);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_TIMER, .1, _visual_update, &visual);
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _visual_canvas_fill, &visual);

    // Run and end.
    vkl_app_run(app, N_FRAMES);

    vkl_visual_destroy(&visual);
    FREE(pos);
    FREE(color);
    TEST_END
}



static void _append(VklVisual* visual)
{
    ASSERT(visual != NULL);
    vec3 pos = {0};
    cvec4 color = {0};
    RANDN_POS(pos);
    RAND_COLOR(color);
    vkl_visual_data_append(visual, VKL_PROP_POS, 0, 1, &pos);
    vkl_visual_data_append(visual, VKL_PROP_COLOR, 0, 1, &color);
}

static void _visual_append(VklCanvas* canvas, VklPrivateEvent ev)
{
    ASSERT(canvas != NULL);
    VklVisual* visual = ev.user_data;
    ASSERT(visual != NULL);
    _append(visual);
    vkl_visual_update(visual, visual->canvas->viewport, (VklDataCoords){0}, NULL);
    vkl_canvas_to_refill(visual->canvas, true);
}

int test_visuals_5(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);
    VklContext* ctx = gpu->context;
    ASSERT(ctx != NULL);
    VklVisual visual = vkl_visual(canvas);
    _marker_visual(&visual);
    _append(&visual);

    // MVP.
    mat4 id = GLM_MAT4_IDENTITY_INIT;
    vkl_visual_data(&visual, VKL_PROP_MODEL, 0, 1, id);
    vkl_visual_data(&visual, VKL_PROP_VIEW, 0, 1, id);
    vkl_visual_data(&visual, VKL_PROP_PROJ, 0, 1, id);

    // Param.
    float param = 50.0f;
    vkl_visual_data(&visual, VKL_PROP_MARKER_SIZE, 0, 1, &param);

    // Upload the data to the GPU..
    vkl_visual_data_buffer(&visual, VKL_SOURCE_TYPE_VIEWPORT, 0, 0, 1, 1, &canvas->viewport);
    vkl_visual_update(&visual, canvas->viewport, (VklDataCoords){0}, NULL);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_TIMER, .1, _visual_append, &visual);
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _visual_canvas_fill, &visual);

    // Run and end.
    vkl_app_run(app, N_FRAMES);

    vkl_visual_destroy(&visual);
    TEST_END
}
