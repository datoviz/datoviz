#include "test_builtin_visuals.h"
#include "../include/visky/builtin_visuals.h"
#include "../include/visky/interact.h"
#include "../src/interact_utils.h"
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
    ASSERT(canvas->viewport.viewport.minDepth < canvas->viewport.viewport.maxDepth);
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

// NOTE: avoid screenshot in interactive mode, otherwise the canvas is destroyed *before* taking
// the screenshot, leading to a segfault.
#define SCREENSHOT(name)                                                                          \
    if (N_FRAMES != 0)                                                                            \
    {                                                                                             \
        char screenshot_path[1024];                                                               \
        snprintf(                                                                                 \
            screenshot_path, sizeof(screenshot_path), "%s/docs/images/visuals/%s.png", ROOT_DIR,  \
            name);                                                                                \
        vkl_screenshot_file(canvas, screenshot_path);                                             \
    }



/*************************************************************************************************/
/*  Builtin visual tests                                                                         */
/*************************************************************************************************/

int test_visuals_point(TestContext* context)
{
    INIT;

    VklVisual visual = vkl_visual(canvas);
    vkl_visual_builtin(&visual, VKL_VISUAL_POINT, 0);

    const uint32_t N = 1000;
    dvec3* pos = calloc(N, sizeof(dvec3));
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
    SCREENSHOT("point")
    FREE(pos);
    FREE(color);
    END;
}



int test_visuals_marker(TestContext* context)
{
    INIT;

    VklVisual visual = vkl_visual(canvas);
    vkl_visual_builtin(&visual, VKL_VISUAL_MARKER, 0);

    const uint32_t N = 1000;
    dvec3* pos = calloc(N, sizeof(dvec3));
    cvec4* color = calloc(N, sizeof(cvec4));
    float* size = calloc(N, sizeof(size));
    float t = 0;
    for (uint32_t i = 0; i < N; i++)
    {
        t = -1 + 2 * i / (float)(N - 1);
        pos[i][0] = t;
        pos[i][1] = .25 * sin(M_2PI * t);
        pos[i][1] += .25 * randn();
        vkl_colormap(VKL_CPAL256_GLASBEY, i % 256, color[i]);
        color[i][3] = 192;
        size[i] = 5 + 45 * rand_float();
    }

    // Set visual data.
    vkl_visual_data(&visual, VKL_PROP_POS, 0, N, pos);
    vkl_visual_data(&visual, VKL_PROP_COLOR, 0, N, color);
    vkl_visual_data(&visual, VKL_PROP_MARKER_SIZE, 0, N, size);

    RUN;
    SCREENSHOT("marker")
    FREE(pos);
    FREE(color);
    FREE(size);
    END;
}



int test_visuals_line(TestContext* context)
{
    INIT;

    VklVisual visual = vkl_visual(canvas);
    vkl_visual_builtin(&visual, VKL_VISUAL_SEGMENT, 0);

    const uint32_t N = 100;
    dvec3* pos0 = calloc(N, sizeof(dvec3));
    dvec3* pos1 = calloc(N, sizeof(dvec3));
    cvec4* color = calloc(N, sizeof(cvec4));
    float t = 0;
    float y = canvas->swapchain.images->width / (float)canvas->swapchain.images->height;
    for (uint32_t i = 0; i < N; i++)
    {
        t = M_2PI * (float)i / N;

        pos0[i][0] = .25 * cos(t);
        pos0[i][1] = y * .25 * sin(t);

        pos1[i][0] = .75 * cos(t);
        pos1[i][1] = y * .75 * sin(t);

        vkl_colormap_scale(VKL_CMAP_HSV, i, 0, N, color[i]);
    }

    // Set visual data.
    vkl_visual_data(&visual, VKL_PROP_POS, 0, N, pos0);
    vkl_visual_data(&visual, VKL_PROP_POS, 1, N, pos1);
    vkl_visual_data(&visual, VKL_PROP_COLOR, 0, N, color);

    RUN;
    FREE(pos0);
    FREE(pos1);
    FREE(color);
    SCREENSHOT("line")
    END;
}



static void _update_interact(VklCanvas* canvas, VklEvent ev)
{
    ASSERT(canvas != NULL);
    VklVisual* visual = (VklVisual*)ev.user_data;
    ASSERT(visual != NULL);

    VklInteract* interact = visual->user_data;
    vkl_interact_update(interact, canvas->viewport, &canvas->mouse, &canvas->keyboard);
    VklSource* source = vkl_source_get(visual, VKL_SOURCE_TYPE_MVP, 0);
    VklBufferRegions* br = &source->u.br;
    vkl_upload_buffers(canvas, *br, 0, br->size, &interact->mvp);
}

int test_visuals_mesh(TestContext* context)
{
    INIT;

    VklVisual visual = vkl_visual(canvas);
    vkl_visual_builtin(&visual, VKL_VISUAL_MESH, 0);

    // Load a mesh file.
    char path[1024];
    snprintf(path, sizeof(path), "%s/mesh/%s", DATA_DIR, "brain.obj");
    VklMesh mesh = vkl_mesh_obj(path);
    vkl_mesh_rotate(&mesh, M_PI, (vec3){1, 0, 0});
    vkl_mesh_transform(&mesh);

    uint32_t nv = mesh.vertices.item_count;
    uint32_t ni = mesh.indices.item_count;

    // Set visual data.
    vkl_visual_data_source(&visual, VKL_SOURCE_TYPE_VERTEX, 0, 0, nv, nv, mesh.vertices.data);
    vkl_visual_data_source(&visual, VKL_SOURCE_TYPE_INDEX, 0, 0, ni, ni, mesh.indices.data);

    VklGraphicsMeshParams params = default_graphics_mesh_params(VKL_CAMERA_EYE);
    vkl_visual_data(&visual, VKL_PROP_LIGHT_PARAMS, 0, 1, &params.lights_params_0);
    vkl_visual_data(&visual, VKL_PROP_LIGHT_POS, 0, 1, &params.lights_pos_0);
    vkl_visual_data(&visual, VKL_PROP_TEXCOEFS, 0, 1, &params.tex_coefs);
    vkl_visual_data(&visual, VKL_PROP_VIEW_POS, 0, 1, &params.view_pos);

    VklInteract interact = vkl_interact_builtin(canvas, VKL_INTERACT_ARCBALL);
    visual.user_data = &interact;
    vkl_event_callback(canvas, VKL_EVENT_FRAME, 0, VKL_EVENT_MODE_SYNC, _update_interact, &visual);

    VklArcball* arcball = &interact.u.a;
    versor q;
    glm_quatv(q, +M_PI / 6, (vec3){1, 0, 0});
    glm_quat_mul(arcball->rotation, q, arcball->rotation);
    glm_quatv(q, +M_PI / 6, (vec3){0, 1, 0});
    glm_quat_mul(arcball->rotation, q, arcball->rotation);
    arcball->camera.eye[2] = 3;
    _arcball_update_mvp(arcball, &interact.mvp);

    RUN;
    SCREENSHOT("mesh")
    END;
}



int test_visuals_volume_slice(TestContext* context)
{
    INIT;

    VklVisual visual = vkl_visual(canvas);
    vkl_visual_builtin(&visual, VKL_VISUAL_VOLUME_SLICE, 0);

    float x = MOUSE_VOLUME_DEPTH / (float)MOUSE_VOLUME_HEIGHT;
    float y = 1;
    float z = 0;
    float t = 0;
    dvec3 p0[8], p1[8], p2[8], p3[8];
    vec3 uvw0[8], uvw1[8], uvw2[8], uvw3[8];
    for (uint32_t i = 0; i < 8; i++)
    {
        t = 1 - i / (float)(8 - 1);
        z = -1 + 2 * t;
        t = .1 + .8 * t;

        p0[i][0] = -x, p0[i][1] = -y, p0[i][2] = +z;
        p1[i][0] = +x, p1[i][1] = -y, p1[i][2] = +z;
        p2[i][0] = +x, p2[i][1] = +y, p2[i][2] = +z;
        p3[i][0] = -x, p3[i][1] = +y, p3[i][2] = +z;

        uvw0[i][0] = 1, uvw0[i][1] = 0, uvw0[i][2] = t;
        uvw1[i][0] = 1, uvw1[i][1] = 1, uvw1[i][2] = t;
        uvw2[i][0] = 0, uvw2[i][1] = 1, uvw2[i][2] = t;
        uvw3[i][0] = 0, uvw3[i][1] = 0, uvw3[i][2] = t;
    }

    vkl_visual_data(&visual, VKL_PROP_POS, 0, 8, p0);
    vkl_visual_data(&visual, VKL_PROP_POS, 1, 8, p1);
    vkl_visual_data(&visual, VKL_PROP_POS, 2, 8, p2);
    vkl_visual_data(&visual, VKL_PROP_POS, 3, 8, p3);

    vkl_visual_data(&visual, VKL_PROP_TEXCOORDS, 0, 8, uvw0);
    vkl_visual_data(&visual, VKL_PROP_TEXCOORDS, 1, 8, uvw1);
    vkl_visual_data(&visual, VKL_PROP_TEXCOORDS, 2, 8, uvw2);
    vkl_visual_data(&visual, VKL_PROP_TEXCOORDS, 3, 8, uvw3);

    vkl_visual_data(&visual, VKL_PROP_TRANSFER_X, 0, 1, (vec4[]){{0, 1, 1, 1}});
    vkl_visual_data(&visual, VKL_PROP_TRANSFER_Y, 0, 1, (vec4[]){{0, 1, 1, 1}});
    vkl_visual_data(&visual, VKL_PROP_TRANSFER_X, 1, 1, (vec4[]){{0, .05, .051, 1}});
    vkl_visual_data(&visual, VKL_PROP_TRANSFER_Y, 1, 1, (vec4[]){{0, 0, .75, .75}});

    float scale = 13;
    vkl_visual_data(&visual, VKL_PROP_SCALE, 0, 1, &scale);

    VklColormap cmap = VKL_CMAP_BONE;
    vkl_visual_data(&visual, VKL_PROP_COLORMAP, 0, 1, &cmap);

    // Texture.
    VklTexture* volume = _mouse_volume(canvas);
    vkl_visual_texture(
        &visual, VKL_SOURCE_TYPE_COLOR_TEXTURE, 0, gpu->context->color_texture.texture);
    vkl_visual_texture(&visual, VKL_SOURCE_TYPE_VOLUME, 0, volume);

    VklInteract interact = vkl_interact_builtin(canvas, VKL_INTERACT_ARCBALL);
    visual.user_data = &interact;
    vkl_event_callback(canvas, VKL_EVENT_FRAME, 0, VKL_EVENT_MODE_SYNC, _update_interact, &visual);

    VklArcball* arcball = &interact.u.a;
    versor q;
    glm_quatv(q, +M_PI / 8, (vec3){1, 0, 0});
    glm_quat_mul(arcball->rotation, q, arcball->rotation);
    glm_quatv(q, M_PI - M_PI / 6, (vec3){0, 1, 0});
    glm_quat_mul(arcball->rotation, q, arcball->rotation);
    arcball->camera.eye[2] = 3;
    _arcball_update_mvp(arcball, &interact.mvp);

    RUN;
    SCREENSHOT("volume_slice")
    END;
}



int test_visuals_axes_2D_1(TestContext* context)
{
    INIT;
    vkl_canvas_clear_color(canvas, (VkClearColorValue){{1, 1, 1, 1}});

    VklFontAtlas* atlas = &gpu->context->font_atlas;
    ASSERT(strlen(atlas->font_str) > 0);

    VklVisual visual = vkl_visual(canvas);
    vkl_visual_builtin(&visual, VKL_VISUAL_AXES_2D, VKL_AXES_COORD_X);

    // Font atlas texture.
    vkl_visual_texture(&visual, VKL_SOURCE_TYPE_FONT_ATLAS, 0, atlas->texture);

    // Prepare the data.
    const uint32_t N = 5;
    const uint32_t MAX_GLYPHS = 12;
    const uint32_t N_minor = 3 * (N - 1);
    double* xticks = calloc(N, sizeof(double));
    double* xticks_minor = calloc(N_minor, sizeof(double));
    char** strings = calloc(N, sizeof(char*));
    char* text = calloc(N * MAX_GLYPHS, sizeof(char));
    double t = 0;
    for (uint32_t i = 0; i < N; i++)
    {
        t = -1 + 2 * (double)i / (N - 1);
        xticks[i] = t;
        if (i < N - 1)
            for (uint32_t j = 0; j < 3; j++)
                xticks_minor[3 * i + j] = t + (j + 1) * .5 / (N - 1);
        strings[i] = &text[MAX_GLYPHS * i];
        snprintf(strings[i], MAX_GLYPHS, "%.3f", t);
    }

    // Set the visual data.
    vkl_visual_data(&visual, VKL_PROP_POS, VKL_AXES_LEVEL_MINOR, N_minor, xticks_minor);
    vkl_visual_data(&visual, VKL_PROP_POS, VKL_AXES_LEVEL_MAJOR, N, xticks);
    vkl_visual_data(&visual, VKL_PROP_POS, VKL_AXES_LEVEL_GRID, N, xticks);
    vkl_visual_data(&visual, VKL_PROP_TEXT, 0, N, strings);

    // Text params.
    VklGraphicsTextParams params = {0};
    params.grid_size[0] = (int32_t)atlas->rows;
    params.grid_size[1] = (int32_t)atlas->cols;
    params.tex_size[0] = (int32_t)atlas->width;
    params.tex_size[1] = (int32_t)atlas->height;
    vkl_visual_data_source(&visual, VKL_SOURCE_TYPE_PARAM, 0, 0, 1, 1, &params);

    _common_data(&visual);
    vkl_event_callback(canvas, VKL_EVENT_REFILL, 0, VKL_EVENT_MODE_SYNC, _resize, NULL);

    vkl_app_run(app, N_FRAMES);
    SCREENSHOT("axis")
    FREE(xticks);
    FREE(xticks_minor);
    FREE(text);
    vkl_visual_destroy(&visual);
    TEST_END
}



static void _visual_update(VklCanvas* canvas, VklEvent ev)
{
    ASSERT(canvas != NULL);
    VklVisual* visual = ev.user_data;
    ASSERT(visual != NULL);

    const uint32_t N = 2 + (ev.u.t.idx % 16);
    double* xticks = calloc(N, sizeof(double));
    double* yticks = calloc(N, sizeof(double));
    char* hello = "ABC";
    char** text = calloc(N, sizeof(char*));
    double t = 0;
    for (uint32_t i = 0; i < N; i++)
    {
        t = -1 + 2 * (double)i / (N - 1);
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

int test_visuals_axes_2D_update(TestContext* context)
{
    INIT;
    vkl_canvas_clear_color(canvas, (VkClearColorValue){{1, 1, 1, 1}});

    VklFontAtlas* atlas = &gpu->context->font_atlas;
    ASSERT(strlen(atlas->font_str) > 0);

    VklVisual visual = vkl_visual(canvas);

    vkl_visual_builtin(&visual, VKL_VISUAL_AXES_2D, VKL_AXES_COORD_X);

    vkl_visual_texture(&visual, VKL_SOURCE_TYPE_FONT_ATLAS, 0, atlas->texture);

    const uint32_t N = 5;
    double* xticks = calloc(N, sizeof(double));
    double* yticks = calloc(N, sizeof(double));
    char* hello = "ABC";
    char** text = calloc(N, sizeof(char*));
    double t = 0;
    for (uint32_t i = 0; i < N; i++)
    {
        t = -1 + 2 * (double)i / (N - 1);
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
    vkl_visual_data_source(&visual, VKL_SOURCE_TYPE_PARAM, 0, 0, 1, 1, &params);

    _common_data(&visual);

    vkl_event_callback(canvas, VKL_EVENT_TIMER, .1, VKL_EVENT_MODE_SYNC, _visual_update, &visual);
    vkl_event_callback(canvas, VKL_EVENT_REFILL, 0, VKL_EVENT_MODE_SYNC, _resize, NULL);
    vkl_app_run(app, N_FRAMES);
    FREE(xticks);
    FREE(yticks);
    FREE(text);
    vkl_visual_destroy(&visual);
    TEST_END
}
