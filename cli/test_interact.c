#include "test_interact.h"
#include "../include/visky/interact.h"
#include "../include/visky/visuals.h"
#include "test_visuals.h"
#include "utils.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

// #define N_FRAMES (getenv("VKY_INTERACT") != NULL ? 0 : 10)



/*************************************************************************************************/
/*  Interact tests                                                                               */
/*************************************************************************************************/

static void _mouse_callback(VklCanvas* canvas, VklEvent ev)
{
    ASSERT(canvas != NULL);
    VklMouse* mouse = (VklMouse*)ev.user_data;
    ASSERT(mouse != NULL);
    vkl_mouse_event(mouse, canvas, ev);
}

static void _keyboard_callback(VklCanvas* canvas, VklEvent ev)
{
    ASSERT(canvas != NULL);
    VklKeyboard* keyboard = (VklKeyboard*)ev.user_data;
    ASSERT(keyboard != NULL);
    vkl_keyboard_event(keyboard, canvas, ev);
}

int test_interact_1(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);

    VklMouse mouse = vkl_mouse();
    VklKeyboard keyboard = vkl_keyboard();

    vkl_event_callback(canvas, VKL_EVENT_MOUSE_MOVE, 0, _mouse_callback, &mouse);
    vkl_event_callback(canvas, VKL_EVENT_MOUSE_BUTTON, 0, _mouse_callback, &mouse);
    vkl_event_callback(canvas, VKL_EVENT_MOUSE_WHEEL, 0, _mouse_callback, &mouse);
    vkl_event_callback(canvas, VKL_EVENT_KEY, 0, _keyboard_callback, &keyboard);

    vkl_app_run(app, N_FRAMES);

    TEST_END
}



/*************************************************************************************************/
/*  Panzoom tests                                                                                */
/*************************************************************************************************/

static void _panzoom_mouse_callback(VklCanvas* canvas, VklEvent ev)
{
    ASSERT(canvas != NULL);
    TestScene* scene = (TestScene*)ev.user_data;
    ASSERT(scene != NULL);
    VklViewport viewport = vkl_viewport_full(canvas);
    vkl_interact_update(&scene->interact, viewport, &canvas->mouse, &canvas->keyboard);

    if (scene->interact.to_update &&
        canvas->clock.elapsed - scene->interact.last_update > VKY_INTERACT_MIN_DELAY)
    {
        vkl_visual_data(&scene->visual, VKL_PROP_MODEL, 0, 1, scene->interact.mvp.model);
        vkl_visual_data(&scene->visual, VKL_PROP_VIEW, 0, 1, scene->interact.mvp.view);
        vkl_visual_data(&scene->visual, VKL_PROP_PROJ, 0, 1, scene->interact.mvp.proj);
        vkl_visual_update(&scene->visual, viewport, (VklDataCoords){0}, NULL);
        scene->interact.last_update = canvas->clock.elapsed;
    }
}

static void _panzoom_keyboard_callback(VklCanvas* canvas, VklEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas != NULL);
    TestScene* scene = (TestScene*)ev.user_data;
    ASSERT(scene != NULL);
    VklViewport viewport = vkl_viewport_full(canvas);
    vkl_interact_update(&scene->interact, viewport, &canvas->mouse, &canvas->keyboard);
}

static void _add_visual(
    TestScene* scene, const uint32_t N, VklVertex* vertices, VklGraphicsPointsParams* params)
{
    VklVisual* visual = &scene->visual;
    _marker_visual(visual);
    VklCanvas* canvas = scene->visual.canvas;
    VklGpu* gpu = canvas->gpu;
    VklContext* ctx = gpu->context;

    VklBufferRegions br_viewport = vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_UNIFORM, 1, 16);
    VklBufferRegions br_params =
        vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_UNIFORM, 1, sizeof(VklGraphicsPointsParams));
    VklTexture* tex_color = vkl_ctx_texture(ctx, 2, (uvec3){16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM);

    vkl_upload_buffers(ctx, br_params, 0, sizeof(VklGraphicsPointsParams), params);

    for (uint32_t i = 0; i < N; i++)
    {
        RANDN_POS(vertices[i].pos)
        RAND_COLOR(vertices[i].color)
    }
    vkl_visual_data_buffer(visual, VKL_SOURCE_VERTEX, 0, 0, N, N, vertices);

    VklMVP mvp = {0};
    glm_mat4_identity(mvp.model);
    glm_mat4_identity(mvp.view);
    glm_mat4_identity(mvp.proj);

    vkl_visual_data_buffer(visual, VKL_SOURCE_UNIFORM, 0, 0, 1, 1, &mvp);
    vkl_visual_buffer(visual, VKL_SOURCE_UNIFORM, 1, br_viewport);
    vkl_visual_buffer(visual, VKL_SOURCE_UNIFORM, 2, br_params);
    vkl_visual_texture(visual, VKL_SOURCE_TEXTURE_2D, 0, tex_color);

    // Upload the data to the GPU.
    VklViewport viewport = vkl_viewport_full(canvas);
    vkl_visual_update(visual, viewport, (VklDataCoords){0}, NULL);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _visual_canvas_fill, visual);
}

int test_interact_panzoom(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);

    TestScene scene = {0};
    scene.interact = vkl_interact_builtin(canvas, VKL_INTERACT_PANZOOM);
    scene.visual = vkl_visual(canvas);

    vkl_event_callback(canvas, VKL_EVENT_MOUSE_MOVE, 0, _panzoom_mouse_callback, &scene);
    vkl_event_callback(canvas, VKL_EVENT_MOUSE_BUTTON, 0, _panzoom_mouse_callback, &scene);
    vkl_event_callback(canvas, VKL_EVENT_MOUSE_WHEEL, 0, _panzoom_mouse_callback, &scene);
    vkl_event_callback(canvas, VKL_EVENT_KEY, 0, _panzoom_keyboard_callback, &scene);
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_TIMER, 1, _fps, NULL);

    const uint32_t N = 10000;
    float param = 5.0f;
    VklGraphicsPointsParams params = {.point_size = param};
    VklVertex* vertices = calloc(N, sizeof(VklVertex));
    _add_visual(&scene, N, vertices, &params);
    vkl_app_run(app, N_FRAMES);

    FREE(vertices);
    TEST_END
}



/*************************************************************************************************/
/*  Arcball tests                                                                               */
/*************************************************************************************************/

int test_interact_arcball(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);

    TestScene scene = {0};
    scene.interact = vkl_interact_builtin(canvas, VKL_INTERACT_ARCBALL);
    scene.visual = vkl_visual(canvas);

    vkl_event_callback(canvas, VKL_EVENT_MOUSE_MOVE, 0, _panzoom_mouse_callback, &scene);
    vkl_event_callback(canvas, VKL_EVENT_MOUSE_BUTTON, 0, _panzoom_mouse_callback, &scene);
    vkl_event_callback(canvas, VKL_EVENT_MOUSE_WHEEL, 0, _panzoom_mouse_callback, &scene);
    vkl_event_callback(canvas, VKL_EVENT_KEY, 0, _panzoom_keyboard_callback, &scene);
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_TIMER, 1, _fps, NULL);

    const uint32_t N = 10000;
    float param = 5.0f;
    VklGraphicsPointsParams params = {.point_size = param};
    VklVertex* vertices = calloc(N, sizeof(VklVertex));
    _add_visual(&scene, N, vertices, &params);
    vkl_app_run(app, N_FRAMES);

    FREE(vertices);
    TEST_END
}



/*************************************************************************************************/
/*  Camera tests                                                                                 */
/*************************************************************************************************/

static void _update_camera(VklCanvas* canvas, VklPrivateEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas != NULL);
    TestScene* scene = (TestScene*)ev.user_data;
    ASSERT(scene != NULL);

    VklViewport viewport = vkl_viewport_full(canvas);
    // log_debug("camera callback %d", canvas->frame_idx);
    vkl_interact_update(&scene->interact, viewport, &canvas->mouse, &canvas->keyboard);

    if (scene->interact.to_update &&
        canvas->clock.elapsed - scene->interact.last_update > VKY_INTERACT_MIN_DELAY)
    {
        vkl_visual_data(&scene->visual, VKL_PROP_MODEL, 0, 1, scene->interact.mvp.model);
        vkl_visual_data(&scene->visual, VKL_PROP_VIEW, 0, 1, scene->interact.mvp.view);
        vkl_visual_data(&scene->visual, VKL_PROP_PROJ, 0, 1, scene->interact.mvp.proj);
        vkl_visual_update(&scene->visual, viewport, (VklDataCoords){0}, NULL);
        scene->interact.last_update = canvas->clock.elapsed;
    }
}

int test_interact_camera(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);

    TestScene scene = {0};
    scene.interact = vkl_interact_builtin(canvas, VKL_INTERACT_FLY);
    scene.visual = vkl_visual(canvas);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_FRAME, 0, _update_camera, &scene);
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_TIMER, 1, _fps, NULL);

    const uint32_t N = 10000;
    float param = 5.0f;
    VklGraphicsPointsParams params = {.point_size = param};
    VklVertex* vertices = calloc(N, sizeof(VklVertex));
    _add_visual(&scene, N, vertices, &params);
    vkl_app_run(app, N_FRAMES);

    FREE(vertices);
    TEST_END
}
