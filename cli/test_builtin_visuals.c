#include "test_builtin_visuals.h"
#include "../include/visky/builtin_visuals.h"
#include "../include/visky/interact.h"
#include "test_visuals.h"
#include "utils.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static VklViewport viewport;
static VklBufferRegions br_viewport;
static mat4 MAT4_ID = GLM_MAT4_IDENTITY_INIT;

static void _mouse_callback(VklCanvas* canvas, VklEvent ev)
{
    ASSERT(canvas != NULL);
    VklMouse* mouse = (VklMouse*)ev.user_data;
    ASSERT(mouse != NULL);
    vkl_mouse_event(mouse, canvas, ev);
}

static void _resize(VklCanvas* canvas, VklPrivateEvent ev)
{
    VklContext* ctx = canvas->gpu->context;
    viewport = vkl_viewport_full(canvas);
    vkl_upload_buffers(ctx, br_viewport, 0, sizeof(VklViewport), &viewport);
}

static void _common_data(VklVisual* visual)
{
    ASSERT(visual != NULL);
    VklCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    VklContext* ctx = canvas->gpu->context;
    ASSERT(ctx != NULL);

    vkl_visual_data(visual, VKL_PROP_MODEL, 0, 1, MAT4_ID);
    vkl_visual_data(visual, VKL_PROP_VIEW, 0, 1, MAT4_ID);
    vkl_visual_data(visual, VKL_PROP_PROJ, 0, 1, MAT4_ID);

    // Viewport.
    br_viewport = vkl_ctx_buffers(ctx, VKL_DEFAULT_BUFFER_UNIFORM, 1, sizeof(VklViewport));
    viewport = vkl_viewport_full(canvas);

    vkl_visual_buffer(visual, VKL_SOURCE_TYPE_VIEWPORT, 0, br_viewport);
    vkl_upload_buffers(ctx, br_viewport, 0, sizeof(VklViewport), &viewport);
    vkl_visual_update(visual, viewport, (VklDataCoords){0}, NULL);

    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _visual_canvas_fill, visual);
}

#define INIT                                                                                      \
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);                                                      \
    VklGpu* gpu = vkl_gpu(app, 0);                                                                \
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT);                                 \
    VklContext* ctx = gpu->context;                                                               \
    ASSERT(ctx != NULL);

#define RUN                                                                                       \
    _common_data(&visual);                                                                        \
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _resize, NULL);                      \
    vkl_app_run(app, N_FRAMES);

#define END                                                                                       \
    vkl_visual_destroy(&visual);                                                                  \
    TEST_END



/*************************************************************************************************/
/*  Builtin visual tests                                                                         */
/*************************************************************************************************/

int test_visuals_marker_raw(TestContext* context)
{
    INIT;

    VklVisual visual = vkl_visual(canvas);
    vkl_visual_builtin(&visual, VKL_VISUAL_MARKER, 0);

    const uint32_t N = 1000;
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

    // Params.
    float param = 20.0f;
    vkl_visual_data(&visual, VKL_PROP_MARKER_SIZE, 0, 1, &param);

    RUN;
    FREE(pos);
    FREE(color);
    END;
}



int test_visuals_segment_raw(TestContext* context)
{
    INIT;
    // vkl_canvas_clear_color(canvas, (VkClearColorValue){{1, 1, 1, 1}});

    VklVisual visual = vkl_visual(canvas);
    vkl_visual_builtin(&visual, VKL_VISUAL_SEGMENT, 0);

    const uint32_t N = 100;
    vec3* pos0 = calloc(N, sizeof(vec3));
    vec3* pos1 = calloc(N, sizeof(vec3));
    cvec4* color = calloc(N, sizeof(cvec4));
    float t = 0;
    for (uint32_t i = 0; i < N; i++)
    {
        t = M_2PI * (float)i / N;

        pos0[i][0] = .25 * cos(t);
        pos0[i][1] = .25 * sin(t);

        pos1[i][0] = .75 * cos(t);
        pos1[i][1] = .75 * sin(t);

        RAND_COLOR(color[i])
        RAND_COLOR(color[i])
    }

    // Set visual data.
    vkl_visual_data(&visual, VKL_PROP_POS, 0, N, pos0);
    vkl_visual_data(&visual, VKL_PROP_POS, 1, N, pos1);
    vkl_visual_data(&visual, VKL_PROP_COLOR, 0, N, color);

    RUN;
    FREE(pos0);
    FREE(pos1);
    FREE(color);
    END;
}



int test_visuals_axes_2D(TestContext* context)
{
    INIT;
    vkl_canvas_clear_color(canvas, (VkClearColorValue){{1, 1, 1, 1}});

    VklFontAtlas font_atlas = _font_texture(gpu->context);
    ASSERT(strlen(font_atlas.font_str) > 0);

    VklVisual visual = vkl_visual(canvas);

    vkl_visual_builtin(&visual, VKL_VISUAL_AXES_2D, 0);

    vkl_visual_texture(&visual, VKL_SOURCE_TYPE_FONT_ATLAS, 1, font_atlas.texture);

    const uint32_t N = 10;
    float* xticks = calloc(N, sizeof(float));
    float* yticks = calloc(N, sizeof(float));
    char* hello = "ABCDEF";
    char** text = calloc(N, sizeof(char*));
    float t = 0;
    for (uint32_t i = 0; i < N; i++)
    {
        t = -1 + 2 * (float)i / (N - 1);
        xticks[i] = t;
        yticks[i] = t;
        text[i] = hello;
    }

    // Set visual data.
    vkl_visual_data(&visual, VKL_PROP_POS, VKL_AXES_LEVEL_GRID, N, xticks);
    cvec4 color = {255, 0, 0, 255};
    vkl_visual_data(&visual, VKL_PROP_COLOR, 0, 1, color);

    // Text.
    vkl_visual_data(&visual, VKL_PROP_TEXT, 0, N, text);

    // Text params.
    VklGraphicsTextParams params = {0};
    params.grid_size[0] = (int32_t)font_atlas.rows;
    params.grid_size[1] = (int32_t)font_atlas.cols;
    params.tex_size[0] = (int32_t)font_atlas.width;
    params.tex_size[1] = (int32_t)font_atlas.height;
    vkl_visual_data_buffer(&visual, VKL_SOURCE_TYPE_PARAM, 1, 0, 1, 1, &params);

    _common_data(&visual);
    vkl_canvas_callback(canvas, VKL_PRIVATE_EVENT_REFILL, 0, _resize, NULL);
    vkl_app_run(app, N_FRAMES);
    FREE(xticks);
    FREE(yticks);
    vkl_visual_destroy(&visual);
    TEST_END
}
