#include "test_builtin_visuals.h"
#include "../include/datoviz/builtin_visuals.h"
#include "../include/datoviz/interact.h"
#include "../include/datoviz/mesh.h"
#include "../src/interact_utils.h"
// #include "../src/mesh_loader.h"
#include "test_visuals.h"
#include "utils.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static DvzBufferRegions br_viewport;
static mat4 MAT4_ID = GLM_MAT4_IDENTITY_INIT;
static float zero = 0;

static void _mouse_callback(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    DvzMouse* mouse = (DvzMouse*)ev.user_data;
    ASSERT(mouse != NULL);
    dvz_mouse_event(mouse, canvas, ev);
}

static void _resize(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas->viewport.viewport.minDepth < canvas->viewport.viewport.maxDepth);
    dvz_upload_buffers(canvas, br_viewport, 0, sizeof(DvzViewport), &canvas->viewport);
}

static void _resize_margins(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas->viewport.viewport.minDepth < canvas->viewport.viewport.maxDepth);
    canvas->viewport.margins[0] = 100;
    canvas->viewport.margins[1] = 100;
    canvas->viewport.margins[2] = 100;
    canvas->viewport.margins[3] = 100;
    dvz_upload_buffers(canvas, br_viewport, 0, sizeof(DvzViewport), &canvas->viewport);
}

static void _common_data(DvzVisual* visual)
{
    ASSERT(visual != NULL);
    DvzCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);
    DvzContext* ctx = canvas->gpu->context;
    ASSERT(ctx != NULL);

    dvz_visual_data(visual, DVZ_PROP_MODEL, 0, 1, MAT4_ID);
    dvz_visual_data(visual, DVZ_PROP_VIEW, 0, 1, MAT4_ID);
    dvz_visual_data(visual, DVZ_PROP_PROJ, 0, 1, MAT4_ID);
    dvz_visual_data(visual, DVZ_PROP_TIME, 0, 1, &zero);

    // Viewport.
    br_viewport = dvz_ctx_buffers(ctx, DVZ_BUFFER_TYPE_UNIFORM, 1, sizeof(DvzViewport));

    // For the tests, share the same viewport buffer region among all graphics pipelines of the
    // visual.
    for (uint32_t pidx = 0; pidx < visual->graphics_count; pidx++)
    {
        dvz_visual_buffer(visual, DVZ_SOURCE_TYPE_VIEWPORT, pidx, br_viewport);
    }
    dvz_upload_buffers(canvas, br_viewport, 0, sizeof(DvzViewport), &canvas->viewport);
    dvz_visual_update(visual, canvas->viewport, (DvzDataCoords){0}, NULL);

    dvz_event_callback(
        canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _visual_canvas_fill, visual);
}

#define INIT                                                                                      \
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);                                                      \
    DvzGpu* gpu = dvz_gpu(app, 0);                                                                \
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);                              \
    DvzContext* ctx = gpu->context;                                                               \
    ASSERT(ctx != NULL);

#define RUN                                                                                       \
    _common_data(&visual);                                                                        \
    dvz_event_callback(canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _resize, NULL);          \
    dvz_app_run(app, N_FRAMES);

#define END                                                                                       \
    dvz_visual_destroy(&visual);                                                                  \
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
        dvz_screenshot_file(canvas, screenshot_path);                                             \
    }



/*************************************************************************************************/
/*  Basic visual tests                                                                           */
/*************************************************************************************************/

int test_visuals_point(TestContext* context)
{
    INIT;

    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_POINT, 0);

    const uint32_t N = 1000;
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

    // Params.
    float param = 20.0f;
    dvz_visual_data(&visual, DVZ_PROP_MARKER_SIZE, 0, 1, &param);

    RUN;
    SCREENSHOT("point")
    FREE(pos);
    FREE(color);
    END;
}



int test_visuals_marker(TestContext* context)
{
    INIT;

    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_MARKER, 0);

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
        pos[i][1] += .25 * dvz_rand_normal();
        dvz_colormap(DVZ_CPAL256_GLASBEY, i % 256, color[i]);
        color[i][3] = 192;
        size[i] = 5 + 45 * dvz_rand_float();
    }

    // Set visual data.
    dvz_visual_data(&visual, DVZ_PROP_POS, 0, N, pos);
    dvz_visual_data(&visual, DVZ_PROP_COLOR, 0, N, color);
    dvz_visual_data(&visual, DVZ_PROP_MARKER_SIZE, 0, N, size);

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

    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_LINE, 0);

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

        dvz_colormap_scale(DVZ_CMAP_HSV, i, 0, N, color[i]);
    }

    // Set visual data.
    dvz_visual_data(&visual, DVZ_PROP_POS, 0, N, pos0);
    dvz_visual_data(&visual, DVZ_PROP_POS, 1, N, pos1);
    dvz_visual_data(&visual, DVZ_PROP_COLOR, 0, N, color);

    RUN;
    FREE(pos0);
    FREE(pos1);
    FREE(color);
    SCREENSHOT("line")
    END;
}



int test_visuals_line_strip(TestContext* context)
{
    INIT;

    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_LINE_STRIP, 0);

    const uint32_t N = 1000;
    const uint32_t nreps = 5;
    dvec3* pos = calloc(nreps * N, sizeof(dvec3));
    cvec4* color = calloc(nreps * N, sizeof(cvec4));
    float t = 0;

    // 2 disjoint line strips.
    for (uint32_t i = 0; i < nreps * N; i++)
    {
        t = -1 + 2 * (float)(i % N) / (N - 1);
        pos[i][0] = .9 * t;
        pos[i][1] = .5 * sin(2 * M_2PI * t) - .25 + .5 / (nreps - 1) * (i / N);
        dvz_colormap_scale(DVZ_CMAP_RAINBOW, t, -1, 1, color[i]);
    }

    // Set visual data.
    dvz_visual_data(&visual, DVZ_PROP_POS, 0, nreps * N, pos);
    dvz_visual_data(&visual, DVZ_PROP_COLOR, 0, nreps * N, color);
    uint32_t* reps = calloc(nreps, sizeof(uint32_t));
    for (uint32_t i = 0; i < nreps; i++)
        reps[i] = N;
    dvz_visual_data(&visual, DVZ_PROP_LENGTH, 0, nreps, reps);

    RUN;
    FREE(reps);
    FREE(pos);
    FREE(color);
    SCREENSHOT("line_strip")
    END;
}



int test_visuals_triangle(TestContext* context)
{
    INIT;

    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_TRIANGLE, 0);

    const uint32_t N = 40;
    dvec3* pos0 = calloc(N, sizeof(dvec3));
    dvec3* pos1 = calloc(N, sizeof(dvec3));
    dvec3* pos2 = calloc(N, sizeof(dvec3));
    cvec4* color = calloc(N, sizeof(cvec4));
    float t = 0;
    float ms = 0;
    for (uint32_t i = 0; i < N; i++)
    {
        t = i / (float)N;
        pos0[i][0] = -.75 + 1.5 * t * t;
        pos0[i][1] = +.75 - 1.5 * t;
        dvz_colormap_scale(DVZ_CMAP_HSV, i, 0, N, color[i]);
        color[i][3] = 128;

        // Copy the 2 other points per triangle.
        _dvec3_copy(pos0[i], pos1[i]);
        _dvec3_copy(pos0[i], pos2[i]);

        // Shift the points.
        ms = .02 + .2 * t * t;
        pos0[i][0] -= ms;
        pos1[i][0] += ms;
        pos0[i][1] -= ms;
        pos1[i][1] -= ms;
        pos2[i][1] += ms;
    }

    // Set visual data.
    dvz_visual_data(&visual, DVZ_PROP_POS, 0, N, pos0);
    dvz_visual_data(&visual, DVZ_PROP_POS, 1, N, pos1);
    dvz_visual_data(&visual, DVZ_PROP_POS, 2, N, pos2);
    dvz_visual_data(&visual, DVZ_PROP_COLOR, 0, N, color);

    RUN;
    FREE(pos0);
    FREE(pos1);
    FREE(pos2);
    FREE(color);
    SCREENSHOT("triangle")
    END;
}



int test_visuals_triangle_strip(TestContext* context)
{
    INIT;

    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_TRIANGLE_STRIP, 0);

    const uint32_t N = 40;
    dvec3* pos = calloc(N, sizeof(dvec3));
    cvec4* color = calloc(N, sizeof(cvec4));
    float m = .1;
    float t = 0;
    float a = 0;
    float y = canvas->swapchain.images->width / (float)canvas->swapchain.images->height;
    for (uint32_t i = 0; i < N; i++)
    {
        t = .9 * (float)i / (float)(N - 1);
        a = M_2PI * t;
        pos[i][0] = (.5 + (i % 2 == 0 ? +m : -m)) * cos(a);
        pos[i][1] = y * (.5 + (i % 2 == 0 ? +m : -m)) * sin(a);
        dvz_colormap_scale(DVZ_CMAP_HSV, t, 0, 1, color[i]);
    }

    // Set visual data.
    dvz_visual_data(&visual, DVZ_PROP_POS, 0, N, pos);
    dvz_visual_data(&visual, DVZ_PROP_COLOR, 0, N, color);

    RUN;
    FREE(pos);
    FREE(color);
    SCREENSHOT("triangle_strip")
    END;
}



int test_visuals_triangle_fan(TestContext* context)
{
    INIT;

    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_TRIANGLE_FAN, 0);

    const uint32_t N = 30;
    dvec3* pos = calloc(N, sizeof(dvec3));
    cvec4* color = calloc(N, sizeof(cvec4));
    float t = 0;
    float a = 0;
    float y = canvas->swapchain.images->width / (float)canvas->swapchain.images->height;
    for (uint32_t i = 1; i < N; i++)
    {
        t = (float)i / (float)(N - 1);
        a = M_2PI * t;
        pos[i][0] = .5 * cos(a);
        pos[i][1] = y * .5 * sin(a);
        dvz_colormap_scale(DVZ_CMAP_HSV, t, 0, 1, color[i]);
    }

    // Set visual data.
    dvz_visual_data(&visual, DVZ_PROP_POS, 0, N, pos);
    dvz_visual_data(&visual, DVZ_PROP_COLOR, 0, N, color);

    RUN;
    FREE(pos);
    FREE(color);
    SCREENSHOT("triangle_fan")
    END;
}



/*************************************************************************************************/
/*  2D visual tests                                                                              */
/*************************************************************************************************/

int test_visuals_axes_2D_1(TestContext* context)
{
    INIT;
    dvz_canvas_clear_color(canvas, 1, 1, 1);

    DvzFontAtlas* atlas = &gpu->context->font_atlas;
    ASSERT(strlen(atlas->font_str) > 0);

    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_AXES_2D, DVZ_AXES_COORD_X);

    // Font atlas texture.
    dvz_visual_texture(&visual, DVZ_SOURCE_TYPE_FONT_ATLAS, 0, atlas->texture);

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
    dvz_visual_data(&visual, DVZ_PROP_POS, DVZ_AXES_LEVEL_MINOR, N_minor, xticks_minor);
    dvz_visual_data(&visual, DVZ_PROP_POS, DVZ_AXES_LEVEL_MAJOR, N, xticks);
    dvz_visual_data(&visual, DVZ_PROP_POS, DVZ_AXES_LEVEL_GRID, N, xticks);
    dvz_visual_data(&visual, DVZ_PROP_TEXT, 0, N, strings);

    // Text params.
    DvzGraphicsTextParams params = {0};
    params.grid_size[0] = (int32_t)atlas->rows;
    params.grid_size[1] = (int32_t)atlas->cols;
    params.tex_size[0] = (int32_t)atlas->width;
    params.tex_size[1] = (int32_t)atlas->height;
    dvz_visual_data_source(&visual, DVZ_SOURCE_TYPE_PARAM, 0, 0, 1, 1, &params);

    _common_data(&visual);
    dvz_event_callback(canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _resize_margins, NULL);

    dvz_app_run(app, N_FRAMES);
    SCREENSHOT("axes")
    FREE(xticks);
    FREE(xticks_minor);
    FREE(text);
    dvz_visual_destroy(&visual);
    TEST_END
}



static void _visual_update(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    DvzVisual* visual = ev.user_data;
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
    dvz_visual_data(visual, DVZ_PROP_POS, DVZ_AXES_LEVEL_MAJOR, N, xticks);
    dvz_visual_data(visual, DVZ_PROP_POS, DVZ_AXES_LEVEL_GRID, N, xticks);
    dvz_visual_data(visual, DVZ_PROP_TEXT, 0, N, text);

    dvz_visual_update(visual, canvas->viewport, (DvzDataCoords){0}, NULL);
    // Manual trigger of full refill in canvas main loop. Normally this is automatically handled
    // by the scene API, which is not used in this test.
    dvz_canvas_to_refill(canvas);

    FREE(xticks);
    FREE(yticks);
    FREE(text);
}

int test_visuals_axes_2D_update(TestContext* context)
{
    INIT;
    dvz_canvas_clear_color(canvas, 1, 1, 1);

    DvzFontAtlas* atlas = &gpu->context->font_atlas;
    ASSERT(strlen(atlas->font_str) > 0);

    DvzVisual visual = dvz_visual(canvas);

    dvz_visual_builtin(&visual, DVZ_VISUAL_AXES_2D, DVZ_AXES_COORD_X);

    dvz_visual_texture(&visual, DVZ_SOURCE_TYPE_FONT_ATLAS, 0, atlas->texture);

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
    dvz_visual_data(&visual, DVZ_PROP_POS, DVZ_AXES_LEVEL_MAJOR, N, xticks);
    dvz_visual_data(&visual, DVZ_PROP_POS, DVZ_AXES_LEVEL_GRID, N, xticks);
    dvz_visual_data(&visual, DVZ_PROP_TEXT, 0, N, text);

    cvec4 color = {255, 0, 0, 255};
    dvz_visual_data(&visual, DVZ_PROP_COLOR, DVZ_AXES_LEVEL_MAJOR, 1, color);
    color[0] = 0;
    color[1] = 255;
    dvz_visual_data(&visual, DVZ_PROP_COLOR, DVZ_AXES_LEVEL_GRID, 1, color);

    float lw = 10;
    dvz_visual_data(&visual, DVZ_PROP_LINE_WIDTH, DVZ_AXES_LEVEL_MAJOR, 1, &lw);
    lw = 5;
    dvz_visual_data(&visual, DVZ_PROP_LINE_WIDTH, DVZ_AXES_LEVEL_GRID, 1, &lw);

    // Text params.
    DvzGraphicsTextParams params = {0};
    params.grid_size[0] = (int32_t)atlas->rows;
    params.grid_size[1] = (int32_t)atlas->cols;
    params.tex_size[0] = (int32_t)atlas->width;
    params.tex_size[1] = (int32_t)atlas->height;
    dvz_visual_data_source(&visual, DVZ_SOURCE_TYPE_PARAM, 0, 0, 1, 1, &params);

    _common_data(&visual);

    dvz_event_callback(canvas, DVZ_EVENT_TIMER, .1, DVZ_EVENT_MODE_SYNC, _visual_update, &visual);
    dvz_event_callback(canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _resize_margins, NULL);
    dvz_app_run(app, N_FRAMES);
    FREE(xticks);
    FREE(yticks);
    FREE(text);
    dvz_visual_destroy(&visual);
    TEST_END
}



static void _add_polygon(dvec3* points, uint32_t n, double angle, dvec3 offset, double ratio)
{
    for (uint32_t i = 0; i < n; i++)
    {
        points[i][0] = offset[0] + .25 * cos(angle + M_2PI * (float)i / (n - 1));
        points[i][1] = offset[1] + ratio * .25 * sin(angle + M_2PI * (float)i / (n - 1));
        points[i][2] = offset[2];
    }
}

int test_visuals_polygon(TestContext* context)
{
    INIT;

    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_POLYGON, 0);

    // Set polygons.
    const uint32_t n0 = 4, n1 = 5, n2 = 6;
    uint32_t point_count = n0 + n1 + n2;
    dvec3 points[4 + 5 + 6];
    double ratio = canvas->swapchain.images->width / (float)canvas->swapchain.images->height;
    _add_polygon(points, n0, M_PI / 2, (dvec3){-.65, 0, 0}, ratio);
    _add_polygon(points + n0, n1, M_PI / 4, (dvec3){0, 0, 0}, ratio);
    _add_polygon(points + n0 + n1, n2, M_PI / 2, (dvec3){+.65, 0, 0}, ratio);

    // Polygon lengths.
    uint32_t poly_lengths[3] = {0};
    poly_lengths[0] = n0;
    poly_lengths[1] = n1;
    poly_lengths[2] = n2;

    // Polygon colors.
    cvec4 color[3] = {0};
    DvzColormap cmap = DVZ_CPAL256_GLASBEY;
    dvz_colormap(cmap, 0, color[0]);
    dvz_colormap(cmap, 1, color[1]);
    dvz_colormap(cmap, 2, color[2]);

    // Set visual data.
    dvz_visual_data(&visual, DVZ_PROP_POS, 0, point_count, points);
    dvz_visual_data(&visual, DVZ_PROP_LENGTH, 0, 3, poly_lengths);
    dvz_visual_data(&visual, DVZ_PROP_COLOR, 0, 3, color);

    RUN;
    SCREENSHOT("polygon")
    END;
}



int test_visuals_image_1(TestContext* context)
{
    INIT;

    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_IMAGE, 0);

    // Top left, top right, bottom right, bottom left
    dvz_visual_data(&visual, DVZ_PROP_POS, 0, 2, (dvec3[]){{-1, +1, 0}, {0, 0, 0}});
    dvz_visual_data(&visual, DVZ_PROP_POS, 1, 2, (dvec3[]){{0, +1, 0}, {1, 0, 0}});
    dvz_visual_data(&visual, DVZ_PROP_POS, 2, 2, (dvec3[]){{0, 0, 0}, {1, -1, 0}});
    dvz_visual_data(&visual, DVZ_PROP_POS, 3, 2, (dvec3[]){{-1, 0, 0}, {0, -1, 0}});

    dvz_visual_data(&visual, DVZ_PROP_TEXCOORDS, 0, 2, (vec2[]){{0, 0}, {0, 0}});
    dvz_visual_data(&visual, DVZ_PROP_TEXCOORDS, 1, 2, (vec2[]){{1, 0}, {1, 0}});
    dvz_visual_data(&visual, DVZ_PROP_TEXCOORDS, 2, 2, (vec2[]){{1, 1}, {1, 1}});
    dvz_visual_data(&visual, DVZ_PROP_TEXCOORDS, 3, 2, (vec2[]){{0, 1}, {0, 1}});

    DvzTexture* texture = _earth_texture(canvas);
    dvz_visual_texture(&visual, DVZ_SOURCE_TYPE_IMAGE, 0, texture);

    RUN;
    SCREENSHOT("image")
    END;
}



int test_visuals_image_cmap(TestContext* context)
{
    INIT;

    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_IMAGE_CMAP, 0);

    // Top left, top right, bottom right, bottom left
    dvz_visual_data(&visual, DVZ_PROP_POS, 0, 2, (dvec3[]){{-1, +1, 0}, {0, 0, 0}});
    dvz_visual_data(&visual, DVZ_PROP_POS, 1, 2, (dvec3[]){{0, +1, 0}, {1, 0, 0}});
    dvz_visual_data(&visual, DVZ_PROP_POS, 2, 2, (dvec3[]){{0, 0, 0}, {1, -1, 0}});
    dvz_visual_data(&visual, DVZ_PROP_POS, 3, 2, (dvec3[]){{-1, 0, 0}, {0, -1, 0}});

    dvz_visual_data(&visual, DVZ_PROP_TEXCOORDS, 0, 2, (vec2[]){{0, 0}, {1, 0}});
    dvz_visual_data(&visual, DVZ_PROP_TEXCOORDS, 1, 2, (vec2[]){{1, 0}, {0, 0}});
    dvz_visual_data(&visual, DVZ_PROP_TEXCOORDS, 2, 2, (vec2[]){{1, 1}, {0, 1}});
    dvz_visual_data(&visual, DVZ_PROP_TEXCOORDS, 3, 2, (vec2[]){{0, 1}, {1, 1}});

    // First texture.
    dvz_visual_texture(
        &visual, DVZ_SOURCE_TYPE_COLOR_TEXTURE, 0, gpu->context->color_texture.texture);

    // Random texture.
    const uint32_t S = 16;
    VkDeviceSize size = S * S * 1;
    uint8_t* tex_data = calloc(size, sizeof(uint8_t));
    for (uint32_t i = 0; i < size; i++)
        tex_data[i] = (uint8_t)((i * 12) % 256);
    uvec3 shape = {S, S, 1};
    DvzTexture* texture = dvz_ctx_texture(gpu->context, 2, shape, VK_FORMAT_R8_UNORM);
    dvz_upload_texture(
        canvas, texture, DVZ_ZERO_OFFSET, DVZ_ZERO_OFFSET, size * sizeof(uint8_t), tex_data);

    // Second texture.
    dvz_visual_texture(&visual, DVZ_SOURCE_TYPE_IMAGE, 0, texture);

    RUN;
    FREE(tex_data);
    // SCREENSHOT("image_cmap")
    END;
}



/*************************************************************************************************/
/*  3D visual tests                                                                              */
/*************************************************************************************************/

static void _update_interact(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    DvzVisual* visual = (DvzVisual*)ev.user_data;
    ASSERT(visual != NULL);

    DvzInteract* interact = visual->user_data;
    dvz_interact_update(interact, canvas->viewport, &canvas->mouse, &canvas->keyboard);
    DvzSource* source = dvz_source_get(visual, DVZ_SOURCE_TYPE_MVP, 0);
    DvzBufferRegions* br = &source->u.br;
    dvz_upload_buffers(canvas, *br, 0, br->size, &interact->mvp);
}

int test_visuals_mesh(TestContext* context)
{
    INIT;

    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_MESH, 0);

    // Load a mesh file.
    char path[1024];
    snprintf(path, sizeof(path), "%s/mesh/%s", DATA_DIR, "brain.obj");
    DvzMesh mesh = dvz_mesh_obj(path);
    dvz_mesh_rotate(&mesh, M_PI, (vec3){1, 0, 0});
    dvz_mesh_transform(&mesh);

    uint32_t nv = mesh.vertices.item_count;
    uint32_t ni = mesh.indices.item_count;

    // Set visual data.
    dvz_visual_data_source(&visual, DVZ_SOURCE_TYPE_VERTEX, 0, 0, nv, nv, mesh.vertices.data);
    dvz_visual_data_source(&visual, DVZ_SOURCE_TYPE_INDEX, 0, 0, ni, ni, mesh.indices.data);

    DvzGraphicsMeshParams params = default_graphics_mesh_params(DVZ_CAMERA_EYE);
    dvz_visual_data(&visual, DVZ_PROP_LIGHT_PARAMS, 0, 1, &params.lights_params_0);
    dvz_visual_data(&visual, DVZ_PROP_LIGHT_POS, 0, 1, &params.lights_pos_0);
    dvz_visual_data(&visual, DVZ_PROP_TEXCOEFS, 0, 1, &params.tex_coefs);
    // dvz_visual_data(&visual, DVZ_PROP_VIEW_POS, 0, 1, &params.view_pos);

    DvzInteract interact = dvz_interact_builtin(canvas, DVZ_INTERACT_ARCBALL);
    visual.user_data = &interact;
    dvz_event_callback(canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _update_interact, &visual);

    DvzArcball* arcball = &interact.u.a;
    versor q;
    glm_quatv(q, +M_PI / 6, (vec3){1, 0, 0});
    glm_quat_mul(arcball->rotation, q, arcball->rotation);
    glm_quatv(q, +M_PI / 6, (vec3){0, 1, 0});
    glm_quat_mul(arcball->rotation, q, arcball->rotation);
    arcball->camera.eye[2] = 3;
    _arcball_update_mvp(canvas->viewport, arcball, &interact.mvp);

    RUN;
    SCREENSHOT("mesh")
    END;
}



static void _volume_interact(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    DvzVisual* visual = (DvzVisual*)ev.user_data;
    ASSERT(visual != NULL);

    DvzInteract* interact = visual->user_data;
    dvz_interact_update(interact, canvas->viewport, &canvas->mouse, &canvas->keyboard);

    DvzSource* source = dvz_source_get(visual, DVZ_SOURCE_TYPE_MVP, 0);
    DvzBufferRegions* br = &source->u.br;
    dvz_upload_buffers(canvas, *br, 0, br->size, &interact->mvp);

    // DvzArcball* arcball = &interact->u.a;
    // dvz_visual_data(visual, DVZ_PROP_VIEW_POS, 0, 1, arcball->camera.eye);

    dvz_visual_update(visual, canvas->viewport, (DvzDataCoords){0}, NULL);
}

int test_visuals_volume_1(TestContext* context)
{
    INIT;

    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_VOLUME, 0);

    const uint32_t ni = MOUSE_VOLUME_WIDTH;
    const uint32_t nj = MOUSE_VOLUME_HEIGHT;
    const uint32_t nk = MOUSE_VOLUME_DEPTH;

    float c = .004;
    vec3 box_size = {c * ni, c * nj, c * nk};
    dvec3 p0 = {-c * ni / 2., -c * nj / 2., -c * nk / 2.};
    dvec3 p1 = {+c * ni / 2., +c * nj / 2., +c * nk / 2.};

    vec3 uvw0, uvw1;
    uvw0[0] = 0, uvw0[1] = 0, uvw0[2] = 0;
    uvw1[0] = 1, uvw1[1] = 1, uvw1[2] = 1;

    // Visual data.
    dvz_visual_data(&visual, DVZ_PROP_POS, 0, 1, p0);
    dvz_visual_data(&visual, DVZ_PROP_POS, 1, 1, p1);

    dvz_visual_data(&visual, DVZ_PROP_TEXCOORDS, 0, 1, uvw0);
    dvz_visual_data(&visual, DVZ_PROP_TEXCOORDS, 1, 1, uvw1);

    dvz_visual_data(&visual, DVZ_PROP_LENGTH, 0, 1, box_size);

    // Colormap texture.
    dvz_visual_texture(
        &visual, DVZ_SOURCE_TYPE_COLOR_TEXTURE, 0, gpu->context->color_texture.texture);

    // Volume texture.
    DvzTexture* volume = _mouse_volume(canvas);
    dvz_visual_texture(&visual, DVZ_SOURCE_TYPE_VOLUME, 0, volume);

    // Arcball.
    DvzInteract interact = dvz_interact_builtin(canvas, DVZ_INTERACT_ARCBALL);
    visual.user_data = &interact;
    dvz_event_callback(canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _volume_interact, &visual);

    // Params.
    DvzArcball* arcball = &interact.u.a;
    // dvz_visual_data(&visual, DVZ_PROP_VIEW_POS, 0, 1, arcball->camera.eye);

    DvzColormap cmap = DVZ_CMAP_BONE;
    dvz_visual_data(&visual, DVZ_PROP_COLORMAP, 0, 1, &cmap);

    versor q;
    glm_quatv(q, +M_PI / 2, (vec3){0, 0, 1});
    glm_quat_mul(arcball->rotation, q, arcball->rotation);
    glm_quatv(q, -M_PI / 8, (vec3){1, 0, 0});
    glm_quat_mul(arcball->rotation, q, arcball->rotation);
    glm_quatv(q, M_PI - M_PI / 6, (vec3){0, 1, 0});
    glm_quat_mul(arcball->rotation, q, arcball->rotation);
    arcball->camera.eye[2] = 3;
    _arcball_update_mvp(canvas->viewport, arcball, &interact.mvp);

    RUN;
    SCREENSHOT("volume")
    END;
}



int test_visuals_volume_slice(TestContext* context)
{
    INIT;

    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_VOLUME_SLICE, 0);

    float x = MOUSE_VOLUME_DEPTH / (float)MOUSE_VOLUME_HEIGHT;
    float y = 1;
    float z = 0;
    float t = 0;
    dvec3 p0[8], p1[8], p2[8], p3[8];
    vec3 uvw0[8], uvw1[8], uvw2[8], uvw3[8];
    // Top left, top right, bottom right, bottom left
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

    dvz_visual_data(&visual, DVZ_PROP_POS, 0, 8, p0);
    dvz_visual_data(&visual, DVZ_PROP_POS, 1, 8, p1);
    dvz_visual_data(&visual, DVZ_PROP_POS, 2, 8, p2);
    dvz_visual_data(&visual, DVZ_PROP_POS, 3, 8, p3);

    dvz_visual_data(&visual, DVZ_PROP_TEXCOORDS, 0, 8, uvw0);
    dvz_visual_data(&visual, DVZ_PROP_TEXCOORDS, 1, 8, uvw1);
    dvz_visual_data(&visual, DVZ_PROP_TEXCOORDS, 2, 8, uvw2);
    dvz_visual_data(&visual, DVZ_PROP_TEXCOORDS, 3, 8, uvw3);

    dvz_visual_data(&visual, DVZ_PROP_TRANSFER_X, 0, 1, (vec4[]){{0, 1, 1, 1}});
    dvz_visual_data(&visual, DVZ_PROP_TRANSFER_Y, 0, 1, (vec4[]){{0, 1, 1, 1}});
    dvz_visual_data(&visual, DVZ_PROP_TRANSFER_X, 1, 1, (vec4[]){{0, .05, .051, 1}});
    dvz_visual_data(&visual, DVZ_PROP_TRANSFER_Y, 1, 1, (vec4[]){{0, 0, .75, .75}});

    float scale = 13;
    dvz_visual_data(&visual, DVZ_PROP_SCALE, 0, 1, &scale);

    DvzColormap cmap = DVZ_CMAP_BONE;
    dvz_visual_data(&visual, DVZ_PROP_COLORMAP, 0, 1, &cmap);

    // Texture.
    DvzTexture* volume = _mouse_volume(canvas);
    dvz_visual_texture(
        &visual, DVZ_SOURCE_TYPE_COLOR_TEXTURE, 0, gpu->context->color_texture.texture);
    dvz_visual_texture(&visual, DVZ_SOURCE_TYPE_VOLUME, 0, volume);

    DvzInteract interact = dvz_interact_builtin(canvas, DVZ_INTERACT_ARCBALL);
    visual.user_data = &interact;
    dvz_event_callback(canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _update_interact, &visual);

    DvzArcball* arcball = &interact.u.a;
    versor q;
    glm_quatv(q, +M_PI / 8, (vec3){1, 0, 0});
    glm_quat_mul(arcball->rotation, q, arcball->rotation);
    glm_quatv(q, M_PI - M_PI / 6, (vec3){0, 1, 0});
    glm_quat_mul(arcball->rotation, q, arcball->rotation);
    arcball->camera.eye[2] = 3;
    _arcball_update_mvp(canvas->viewport, arcball, &interact.mvp);

    RUN;
    SCREENSHOT("volume_slice")
    END;
}
