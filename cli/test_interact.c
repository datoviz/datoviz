#include "test_interact.h"
#include "../include/visky/interact2.h"
#include "../include/visky/visuals2.h"
#include "test_visuals2.h"
#include "utils.h"



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

typedef struct _TestScene _TestScene;
struct _TestScene
{
    VklMouse mouse;
    VklKeyboard keyboard;
    VklInteract interact;
    VklVisual visual;
};



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

int test_interact_1(VkyTestContext* context)
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

    vkl_app_run(app, 0);

    TEST_END
}



static void _scene_mouse_callback(VklCanvas* canvas, VklEvent ev)
{
    ASSERT(canvas != NULL);
    _TestScene* scene = (_TestScene*)ev.user_data;
    ASSERT(scene != NULL);
    VklViewport viewport = vkl_viewport_full(canvas);
    vkl_mouse_event(&scene->mouse, canvas, ev);
    vkl_interact_update(&scene->interact, viewport, &scene->mouse, &scene->keyboard);

    vkl_visual_data(&scene->visual, VKL_PROP_VIEW, 0, 1, scene->interact.mvp.view);
    vkl_visual_data(&scene->visual, VKL_PROP_PROJ, 0, 1, scene->interact.mvp.proj);
}

static void _scene_keyboard_callback(VklCanvas* canvas, VklEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(canvas != NULL);
    _TestScene* scene = (_TestScene*)ev.user_data;
    ASSERT(scene != NULL);
    VklViewport viewport = vkl_viewport_full(canvas);
    vkl_keyboard_event(&scene->keyboard, canvas, ev);
    vkl_interact_update(&scene->interact, viewport, &scene->mouse, &scene->keyboard);
}

int test_interact_panzoom(VkyTestContext* context)
{
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);
    VklGpu* gpu = vkl_gpu(app, 0);
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);

    _TestScene scene = {0};
    scene.mouse = vkl_mouse();
    scene.keyboard = vkl_keyboard();
    scene.interact = vkl_interact_builtin(canvas, VKL_INTERACT_PANZOOM);
    scene.visual = vkl_visual(canvas);

    vkl_event_callback(canvas, VKL_EVENT_MOUSE_MOVE, 0, _scene_mouse_callback, &scene);
    vkl_event_callback(canvas, VKL_EVENT_MOUSE_BUTTON, 0, _scene_mouse_callback, &scene);
    vkl_event_callback(canvas, VKL_EVENT_MOUSE_WHEEL, 0, _scene_mouse_callback, &scene);
    vkl_event_callback(canvas, VKL_EVENT_KEY, 0, _scene_keyboard_callback, &scene);

    const uint32_t N = 10000;
    mat4 id = GLM_MAT4_IDENTITY_INIT;
    float param = 5.0f;
    VklGraphicsPointsParams params = {.point_size = param};
    VklVertex* vertices = calloc(N, sizeof(VklVertex));
    {
        VklVisual* visual = &scene.visual;
        _marker_visual(visual);

        VklContext* ctx = gpu->context;
        // VklBufferRegions br_mvp =
        //     vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_UNIFORM_MAPPABLE, 1, sizeof(VklMVP));
        VklBufferRegions br_viewport = vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_UNIFORM, 1, 16);
        VklBufferRegions br_params =
            vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_UNIFORM, 1, sizeof(VklGraphicsPointsParams));
        VklTexture* tex_color =
            vkl_ctx_texture(ctx, 2, (uvec3){16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM);

        vkl_upload_buffers(ctx, br_params, 0, sizeof(VklGraphicsPointsParams), &params);

        for (uint32_t i = 0; i < N; i++)
        {
            RANDN_POS(vertices[i].pos)
            RAND_COLOR(vertices[i].color)
        }
        vkl_visual_data_buffer(visual, VKL_SOURCE_VERTEX, 0, 0, N, N, vertices);
        vkl_visual_buffer(visual, VKL_SOURCE_UNIFORM, 1, br_viewport);
        vkl_visual_buffer(visual, VKL_SOURCE_UNIFORM, 2, br_params);
        vkl_visual_texture(visual, VKL_SOURCE_TEXTURE_2D, 0, tex_color);

        // MVP.
        vkl_visual_data(visual, VKL_PROP_MODEL, 0, 1, id);
        vkl_visual_data(visual, VKL_PROP_VIEW, 0, 1, id);
        vkl_visual_data(visual, VKL_PROP_PROJ, 0, 1, id);

        // Upload the data to the GPU.
        VklViewport viewport = vkl_viewport_full(canvas);
        vkl_visual_update(visual, viewport, (VklDataCoords){0}, NULL);

        vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _visual_canvas_fill, visual);
    }

    vkl_app_run(app, 0);

    FREE(vertices);
    TEST_END
}
