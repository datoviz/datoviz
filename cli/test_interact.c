#include "test_interact.h"
#include "../include/datoviz/interact.h"
#include "../include/datoviz/visuals.h"
#include "test_visuals.h"
#include "utils.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Interact tests                                                                               */
/*************************************************************************************************/

static void _mouse_callback(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    DvzMouse* mouse = (DvzMouse*)ev.user_data;
    ASSERT(mouse != NULL);
    dvz_mouse_event(mouse, canvas, ev);
}

static void _keyboard_callback(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    DvzKeyboard* keyboard = (DvzKeyboard*)ev.user_data;
    ASSERT(keyboard != NULL);
    dvz_keyboard_event(keyboard, canvas, ev);
}

int test_interact_1(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu_best(app);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);

    DvzMouse mouse = dvz_mouse();
    DvzKeyboard keyboard = dvz_keyboard();

    dvz_event_callback(
        canvas, DVZ_EVENT_MOUSE_MOVE, 0, DVZ_EVENT_MODE_SYNC, _mouse_callback, &mouse);
    dvz_event_callback(
        canvas, DVZ_EVENT_MOUSE_PRESS, 0, DVZ_EVENT_MODE_SYNC, _mouse_callback, &mouse);
    dvz_event_callback(
        canvas, DVZ_EVENT_MOUSE_WHEEL, 0, DVZ_EVENT_MODE_SYNC, _mouse_callback, &mouse);
    dvz_event_callback(
        canvas, DVZ_EVENT_KEY_PRESS, 0, DVZ_EVENT_MODE_SYNC, _keyboard_callback, &keyboard);

    dvz_app_run(app, N_FRAMES);

    TEST_END
}



/*************************************************************************************************/
/*  Interact utils                                                                               */
/*************************************************************************************************/

static void _update_interact(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas != NULL);
    TestScene* scene = (TestScene*)ev.user_data;
    ASSERT(scene != NULL);

    dvz_interact_update(&scene->interact, canvas->viewport, &canvas->mouse, &canvas->keyboard);
    DvzSource* source = dvz_source_get(&scene->visual, DVZ_SOURCE_TYPE_MVP, 0);
    DvzBufferRegions* br = &source->u.br;
    dvz_upload_buffers(canvas, *br, 0, br->size, &scene->interact.mvp);
}

static void _add_visual(
    TestScene* scene, const uint32_t N, DvzVertex* vertices, DvzGraphicsPointParams* params)
{
    DvzVisual* visual = &scene->visual;
    _marker_visual(visual);
    DvzCanvas* canvas = scene->visual.canvas;
    DvzGpu* gpu = canvas->gpu;
    DvzContext* ctx = gpu->context;

    DvzBufferRegions br_mvp = dvz_ctx_buffers(
        ctx, DVZ_BUFFER_TYPE_UNIFORM_MAPPABLE, canvas->swapchain.img_count, sizeof(DvzMVP));
    DvzBufferRegions br_viewport = dvz_ctx_buffers(ctx, DVZ_BUFFER_TYPE_UNIFORM, 1, 16);
    DvzBufferRegions br_params =
        dvz_ctx_buffers(ctx, DVZ_BUFFER_TYPE_UNIFORM, 1, sizeof(DvzGraphicsPointParams));

    dvz_upload_buffers(canvas, br_params, 0, sizeof(DvzGraphicsPointParams), params);

    for (uint32_t i = 0; i < N; i++)
    {
        RANDN_POS(vertices[i].pos)
        RAND_COLOR(vertices[i].color)
    }
    dvz_visual_data_source(visual, DVZ_SOURCE_TYPE_VERTEX, 0, 0, N, N, vertices);

    DvzMVP mvp = {0};
    glm_mat4_identity(mvp.model);
    glm_mat4_identity(mvp.view);
    glm_mat4_identity(mvp.proj);

    dvz_visual_buffer(visual, DVZ_SOURCE_TYPE_MVP, 0, br_mvp);
    dvz_visual_buffer(visual, DVZ_SOURCE_TYPE_VIEWPORT, 0, br_viewport);
    dvz_visual_buffer(visual, DVZ_SOURCE_TYPE_PARAM, 0, br_params);

    // Upload the data to the GPU.
    dvz_visual_update(visual, canvas->viewport, (DvzDataCoords){0}, NULL);

    dvz_event_callback(
        canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _visual_canvas_fill, visual);
}



/*************************************************************************************************/
/*  Panzoom tests                                                                                */
/*************************************************************************************************/

int test_interact_panzoom(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu_best(app);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);

    TestScene scene = {0};
    scene.interact = dvz_interact_builtin(canvas, DVZ_INTERACT_PANZOOM);
    scene.visual = dvz_visual(canvas);

    dvz_event_callback(canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _update_interact, &scene);

    const uint32_t N = 10000;
    float param = 5.0f;
    DvzGraphicsPointParams params = {.point_size = param};
    DvzVertex* vertices = calloc(N, sizeof(DvzVertex));
    _add_visual(&scene, N, vertices, &params);
    dvz_app_run(app, N_FRAMES);

    FREE(vertices);
    TEST_END
}



/*************************************************************************************************/
/*  Arcball tests                                                                               */
/*************************************************************************************************/

int test_interact_arcball(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu_best(app);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, DVZ_CANVAS_FLAGS_FPS);

    TestScene scene = {0};
    scene.interact = dvz_interact_builtin(canvas, DVZ_INTERACT_ARCBALL);
    scene.visual = dvz_visual(canvas);

    dvz_event_callback(canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _update_interact, &scene);

    const uint32_t N = 10000;
    float param = 5.0f;
    DvzGraphicsPointParams params = {.point_size = param};
    DvzVertex* vertices = calloc(N, sizeof(DvzVertex));
    _add_visual(&scene, N, vertices, &params);

    dvz_app_run(app, N_FRAMES);
    FREE(vertices);
    TEST_END
}



/*************************************************************************************************/
/*  Camera tests                                                                                 */
/*************************************************************************************************/

int test_interact_camera(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu_best(app);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);

    TestScene scene = {0};
    scene.interact = dvz_interact_builtin(canvas, DVZ_INTERACT_FLY);
    scene.visual = dvz_visual(canvas);

    dvz_event_callback(canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _update_interact, &scene);

    const uint32_t N = 10000;
    float param = 5.0f;
    DvzGraphicsPointParams params = {.point_size = param};
    DvzVertex* vertices = calloc(N, sizeof(DvzVertex));
    _add_visual(&scene, N, vertices, &params);
    dvz_app_run(app, N_FRAMES);

    FREE(vertices);
    TEST_END
}
