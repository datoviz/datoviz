#include "test_builtin_visuals.h"
#include "../include/visky/builtin_visuals.h"
#include "../include/visky/interact.h"
#include "../src/mesh_loader.h"
#include "test_visuals.h"
#include "utils.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static VklBufferRegions br_viewport;
static mat4 MAT4_ID = GLM_MAT4_IDENTITY_INIT;
static float zero = 0;

static void _mouse_callback(VklCanvas* canvas, VklEvent ev)
{
    ASSERT(canvas != NULL);
    VklMouse* mouse = (VklMouse*)ev.user_data;
    ASSERT(mouse != NULL);
    vkl_mouse_event(mouse, canvas, ev);
}

static void _resize(VklCanvas* canvas, VklEvent ev)
{
    canvas->viewport.margins[0] = 100;
    canvas->viewport.margins[1] = 100;
    canvas->viewport.margins[2] = 100;
    canvas->viewport.margins[3] = 100;
    vkl_upload_buffers(canvas, br_viewport, 0, sizeof(VklViewport), &canvas->viewport);
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
    vkl_visual_data(visual, VKL_PROP_TIME, 0, 1, &zero);

    // Viewport.
    br_viewport = vkl_ctx_buffers(ctx, VKL_BUFFER_TYPE_UNIFORM, 1, sizeof(VklViewport));

    // For the tests, share the same viewport buffer region among all graphics pipelines of the
    // visual.
    for (uint32_t pidx = 0; pidx < visual->graphics_count; pidx++)
    {
        vkl_visual_buffer(visual, VKL_SOURCE_TYPE_VIEWPORT, pidx, br_viewport);
    }
    vkl_upload_buffers(canvas, br_viewport, 0, sizeof(VklViewport), &canvas->viewport);
    vkl_visual_update(visual, canvas->viewport, (VklDataCoords){0}, NULL);

    vkl_event_callback(
        canvas, VKL_EVENT_REFILL, 0, VKL_EVENT_MODE_SYNC, _visual_canvas_fill, visual);
}

#define INIT                                                                                      \
    VklApp* app = vkl_app(VKL_BACKEND_GLFW);                                                      \
    VklGpu* gpu = vkl_gpu(app, 0);                                                                \
    VklCanvas* canvas = vkl_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);                              \
    VklContext* ctx = gpu->context;                                                               \
    ASSERT(ctx != NULL);

#define RUN                                                                                       \
    _common_data(&visual);                                                                        \
    vkl_event_callback(canvas, VKL_EVENT_REFILL, 0, VKL_EVENT_MODE_SYNC, _resize, NULL);          \
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



int test_visuals_mesh(TestContext* context)
{
    INIT;

    VklVisual visual = vkl_visual(canvas);
    vkl_visual_builtin(&visual, VKL_VISUAL_MESH, 0);

    char path[1024];
    snprintf(path, sizeof(path), "%s/mesh/%s", DATA_DIR, "brain.obj");
    VklMesh mesh = vkl_mesh_obj(path);

    uint32_t nv = mesh.vertices.item_count;
    uint32_t ni = mesh.indices.item_count;

    // Set visual data.
    vkl_visual_data_full(&visual, VKL_SOURCE_TYPE_VERTEX, 0, 0, nv, nv, mesh.vertices.data);
    vkl_visual_data_full(&visual, VKL_SOURCE_TYPE_INDEX, 0, 0, ni, ni, mesh.indices.data);

    vkl_visual_texture(&visual, VKL_SOURCE_TYPE_IMAGE_1, 0, gpu->context->font_atlas.texture);
    vkl_visual_texture(&visual, VKL_SOURCE_TYPE_IMAGE_2, 0, gpu->context->font_atlas.texture);
    vkl_visual_texture(&visual, VKL_SOURCE_TYPE_IMAGE_3, 0, gpu->context->font_atlas.texture);
    vkl_visual_texture(&visual, VKL_SOURCE_TYPE_IMAGE_4, 0, gpu->context->font_atlas.texture);

    // VklGraphicsMeshParams params = {0};
    mat4 lights_params = {0};
    lights_params[0][0] = 0.2;
    lights_params[0][1] = 0.4;
    lights_params[0][2] = 0.4;

    mat4 lights_pos = {0};
    lights_pos[0][0] = -2;
    lights_pos[0][1] = 0.5;
    lights_pos[0][2] = +2;

    vec4 tex_coefs = {0};
    tex_coefs[0] = 1;

    vec4 view_pos = {0};
    view_pos[2] = 3;
    // vkl_visual_data_full(&visual, VKL_SOURCE_TYPE_PARAM, 0, 0, 1, 1, &params);
    vkl_visual_data(&visual, VKL_PROP_LIGHT_PARAMS, 0, 1, lights_params);
    vkl_visual_data(&visual, VKL_PROP_LIGHT_POS, 0, 1, lights_pos);
    vkl_visual_data(&visual, VKL_PROP_TEXCOEFS, 0, 1, tex_coefs);
    vkl_visual_data(&visual, VKL_PROP_VIEW_POS, 0, 1, view_pos);

    RUN;
    vkl_mesh_destroy(&mesh);
    END;
}



static void _visual_update(VklCanvas* canvas, VklEvent ev)
{
    ASSERT(canvas != NULL);
    VklVisual* visual = ev.user_data;
    ASSERT(visual != NULL);

    const uint32_t N = 2 + (ev.u.t.idx % 16);
    float* xticks = calloc(N, sizeof(float));
    float* yticks = calloc(N, sizeof(float));
    char* hello = "ABC";
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
    vkl_visual_data(visual, VKL_PROP_POS, VKL_AXES_LEVEL_MAJOR, N, xticks);
    vkl_visual_data(visual, VKL_PROP_POS, VKL_AXES_LEVEL_GRID, N, xticks);
    vkl_visual_data(visual, VKL_PROP_TEXT, 0, N, text);

    vkl_visual_update(visual, canvas->viewport, (VklDataCoords){0}, NULL);
    // Manual trigger of full refill in canvas main loop. Normally this is automatically handled
    // by the scene API, which is not used in this test.
    vkl_canvas_to_refill(canvas);

    FREE(xticks);
    FREE(yticks);
    FREE(text);
}

int test_visuals_axes_2D(TestContext* context)
{
    INIT;
    vkl_canvas_clear_color(canvas, (VkClearColorValue){{1, 1, 1, 1}});

    VklFontAtlas* atlas = &gpu->context->font_atlas;
    ASSERT(strlen(atlas->font_str) > 0);

    VklVisual visual = vkl_visual(canvas);

    vkl_visual_builtin(&visual, VKL_VISUAL_AXES_2D, VKL_AXES_COORD_X);

    vkl_visual_texture(&visual, VKL_SOURCE_TYPE_FONT_ATLAS, 1, atlas->texture);

    const uint32_t N = 5;
    float* xticks = calloc(N, sizeof(float));
    float* yticks = calloc(N, sizeof(float));
    char* hello = "ABC";
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
    vkl_visual_data(&visual, VKL_PROP_POS, VKL_AXES_LEVEL_MAJOR, N, xticks);
    vkl_visual_data(&visual, VKL_PROP_POS, VKL_AXES_LEVEL_GRID, N, xticks);
    vkl_visual_data(&visual, VKL_PROP_TEXT, 0, N, text);

    cvec4 color = {255, 0, 0, 255};
    vkl_visual_data(&visual, VKL_PROP_COLOR, VKL_AXES_LEVEL_MAJOR, 1, color);
    color[0] = 0;
    color[1] = 255;
    vkl_visual_data(&visual, VKL_PROP_COLOR, VKL_AXES_LEVEL_GRID, 1, color);

    float lw = 10;
    vkl_visual_data(&visual, VKL_PROP_LINE_WIDTH, VKL_AXES_LEVEL_MAJOR, 1, &lw);
    lw = 5;
    vkl_visual_data(&visual, VKL_PROP_LINE_WIDTH, VKL_AXES_LEVEL_GRID, 1, &lw);

    // Text params.
    VklGraphicsTextParams params = {0};
    params.grid_size[0] = (int32_t)atlas->rows;
    params.grid_size[1] = (int32_t)atlas->cols;
    params.tex_size[0] = (int32_t)atlas->width;
    params.tex_size[1] = (int32_t)atlas->height;
    vkl_visual_data_full(&visual, VKL_SOURCE_TYPE_PARAM, 1, 0, 1, 1, &params);

    _common_data(&visual);

    vkl_event_callback(canvas, VKL_EVENT_TIMER, .25, VKL_EVENT_MODE_SYNC, _visual_update, &visual);
    vkl_event_callback(canvas, VKL_EVENT_REFILL, 0, VKL_EVENT_MODE_SYNC, _resize, NULL);
    // vkl_event_callback(canvas, VKL_EVENT_FRAME, 0, VKL_EVENT_MODE_SYNC, _wait, NULL);
    vkl_app_run(app, N_FRAMES);
    FREE(xticks);
    FREE(yticks);
    FREE(text);
    vkl_visual_destroy(&visual);
    TEST_END
}
