#include "test_interact.h"
#include "../include/visky/interact.h"
#include "../include/visky/visuals.h"
#include "test_visuals.h"
#include "utils.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



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
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);

    VklMouse mouse = vkl_mouse();
    VklKeyboard keyboard = vkl_keyboard();

    vkl_event_callback(
        canvas, VKL_EVENT_MOUSE_MOVE, 0, VKL_EVENT_MODE_SYNC, _mouse_callback, &mouse);
    vkl_event_callback(
        canvas, VKL_EVENT_MOUSE_BUTTON, 0, VKL_EVENT_MODE_SYNC, _mouse_callback, &mouse);
    vkl_event_callback(
        canvas, VKL_EVENT_MOUSE_WHEEL, 0, VKL_EVENT_MODE_SYNC, _mouse_callback, &mouse);
    vkl_event_callback(
        canvas, VKL_EVENT_KEY, 0, VKL_EVENT_MODE_SYNC, _keyboard_callback, &keyboard);

    vkl_app_run(app, N_FRAMES);

    TEST_END
}



/*************************************************************************************************/
/*  Interact utils                                                                               */
/*************************************************************************************************/

static void _update_interact(VklCanvas* canvas, VklEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas != NULL);
    TestScene* scene = (TestScene*)ev.user_data;
    ASSERT(scene != NULL);

    vkl_interact_update(&scene->interact, canvas->viewport, &canvas->mouse, &canvas->keyboard);
    VklSource* source = vkl_bake_source(&scene->visual, VKL_SOURCE_TYPE_MVP, 0);
    VklBufferRegions* br = &source->u.br;
    // VklPointer pointer = aligned_repeat(br->size, &scene->interact.mvp, 1, br->alignment);
    // vkl_buffer_regions_upload(br, canvas->swapchain.img_idx, pointer.pointer);
    // ALIGNED_FREE(pointer)
    vkl_upload_buffers(canvas, *br, 0, br->size, &scene->interact.mvp);
}

static void _add_visual(
    TestScene* scene, const uint32_t N, VklVertex* vertices, VklGraphicsPointParams* params)
{
    VklVisual* visual = &scene->visual;
    _marker_visual(visual);
    VklCanvas* canvas = scene->visual.canvas;
    VklGpu* gpu = canvas->gpu;
    VklContext* ctx = gpu->context;

    VklBufferRegions br_mvp = vkl_ctx_buffers(
        ctx, VKL_BUFFER_TYPE_UNIFORM_MAPPABLE, canvas->swapchain.img_count, sizeof(VklMVP));
    VklBufferRegions br_viewport = vkl_ctx_buffers(ctx, VKL_BUFFER_TYPE_UNIFORM, 1, 16);
    VklBufferRegions br_params =
        vkl_ctx_buffers(ctx, VKL_BUFFER_TYPE_UNIFORM, 1, sizeof(VklGraphicsPointParams));

    vkl_upload_buffers(canvas, br_params, 0, sizeof(VklGraphicsPointParams), params);

    for (uint32_t i = 0; i < N; i++)
    {
        RANDN_POS(vertices[i].pos)
        RAND_COLOR(vertices[i].color)
    }
    vkl_visual_data_full(visual, VKL_SOURCE_TYPE_VERTEX, 0, 0, N, N, vertices);

    VklMVP mvp = {0};
    glm_mat4_identity(mvp.model);
    glm_mat4_identity(mvp.view);
    glm_mat4_identity(mvp.proj);

    vkl_visual_buffer(visual, VKL_SOURCE_TYPE_MVP, 0, br_mvp);
    vkl_visual_buffer(visual, VKL_SOURCE_TYPE_VIEWPORT, 0, br_viewport);
    vkl_visual_buffer(visual, VKL_SOURCE_TYPE_PARAM, 0, br_params);

    // Upload the data to the GPU.
    vkl_visual_update(visual, canvas->viewport, (VklDataCoords){0}, NULL);

    vkl_event_callback(
        canvas, VKL_EVENT_REFILL, 0, VKL_EVENT_MODE_SYNC, _visual_canvas_fill, visual);
}



/*************************************************************************************************/
/*  Panzoom tests                                                                                */
/*************************************************************************************************/

int test_interact_panzoom(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);

    TestScene scene = {0};
    scene.interact = vkl_interact_builtin(canvas, VKL_INTERACT_PANZOOM);
    scene.visual = vkl_visual(canvas);

    vkl_event_callback(canvas, VKL_EVENT_FRAME, 0, VKL_EVENT_MODE_SYNC, _update_interact, &scene);

    const uint32_t N = 10000;
    float param = 5.0f;
    VklGraphicsPointParams params = {.point_size = param};
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
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, VKL_CANVAS_FLAGS_FPS);

    TestScene scene = {0};
    scene.interact = vkl_interact_builtin(canvas, VKL_INTERACT_ARCBALL);
    scene.visual = vkl_visual(canvas);

    vkl_event_callback(canvas, VKL_EVENT_FRAME, 0, VKL_EVENT_MODE_SYNC, _update_interact, &scene);

    const uint32_t N = 10000;
    float param = 5.0f;
    VklGraphicsPointParams params = {.point_size = param};
    VklVertex* vertices = calloc(N, sizeof(VklVertex));
    _add_visual(&scene, N, vertices, &params);

    vkl_app_run(app, N_FRAMES);
    FREE(vertices);
    TEST_END
}



/*************************************************************************************************/
/*  Camera tests                                                                                 */
/*************************************************************************************************/

int test_interact_camera(TestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);

    TestScene scene = {0};
    scene.interact = vkl_interact_builtin(canvas, VKL_INTERACT_FLY);
    scene.visual = vkl_visual(canvas);

    vkl_event_callback(canvas, VKL_EVENT_FRAME, 0, VKL_EVENT_MODE_SYNC, _update_interact, &scene);

    const uint32_t N = 10000;
    float param = 5.0f;
    VklGraphicsPointParams params = {.point_size = param};
    VklVertex* vertices = calloc(N, sizeof(VklVertex));
    _add_visual(&scene, N, vertices, &params);
    vkl_app_run(app, N_FRAMES);

    FREE(vertices);
    TEST_END
}
