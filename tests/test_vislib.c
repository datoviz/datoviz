#include "../include/datoviz/vislib.h"
#include "../include/datoviz/visuals.h"
#include "proto.h"
#include "tests.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct TestScene TestScene;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct TestScene
{
    DvzCanvas* canvas;
    DvzVisual* visual;
};



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _visual_canvas_fill(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(ev.user_data != NULL);
    DvzVisual* visual = (DvzVisual*)ev.user_data;

    // TODO: choose which of all canvas command buffers need to be filled with the visual
    // For now, update all of them.
    for (uint32_t i = 0; i < ev.u.rf.cmd_count; i++)
    {
        dvz_visual_fill_begin(canvas, ev.u.rf.cmds[i], ev.u.rf.img_idx);
        dvz_cmd_viewport(ev.u.rf.cmds[i], ev.u.rf.img_idx, canvas->viewport.viewport);
        dvz_visual_fill_event(
            visual, ev.u.rf.clear_color, ev.u.rf.cmds[i], ev.u.rf.img_idx, canvas->viewport, NULL);
        dvz_visual_fill_end(canvas, ev.u.rf.cmds[i], ev.u.rf.img_idx);
    }
}

static void _visual_common(DvzVisual* visual)
{
    ASSERT(visual != NULL);

    DvzCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);

    DvzContext* ctx = canvas->gpu->context;
    ASSERT(ctx != NULL);

    dvz_visual_data(visual, DVZ_PROP_MODEL, 0, 1, GLM_MAT4_IDENTITY);
    dvz_visual_data(visual, DVZ_PROP_VIEW, 0, 1, GLM_MAT4_IDENTITY);
    dvz_visual_data(visual, DVZ_PROP_PROJ, 0, 1, GLM_MAT4_IDENTITY);
    dvz_visual_data(visual, DVZ_PROP_TIME, 0, 1, (float[]){0});
    dvz_visual_data(visual, DVZ_PROP_VIEWPORT, 0, 1, &canvas->viewport);

    dvz_event_callback(
        canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _visual_canvas_fill, visual);
}

static int _visual_screenshot(DvzVisual* visual, const char* name)
{
    ASSERT(visual != NULL);
    DvzCanvas* canvas = visual->canvas;

    char path[1024];
    snprintf(path, sizeof(path), "test_vislib_%s", name);
    int res = check_canvas(canvas, path);
    snprintf(path, sizeof(path), "%s/docs/images/visuals/%s.png", ROOT_DIR, name);
    dvz_screenshot_file(canvas, path);
    return res;
}

static int _visual_run(DvzVisual* visual, const char* name)
{
    ASSERT(visual != NULL);

    DvzCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);

    // Update the visual's data.
    dvz_visual_update(visual, canvas->viewport, (DvzDataCoords){0}, NULL);

    // Run app.
    dvz_app_run(canvas->app, N_FRAMES);

    // Screenshot.
    int res = _visual_screenshot(visual, name);

    dvz_visual_destroy(visual);
    return res;
}



/*************************************************************************************************/
/*  Visuals tests                                                                                */
/*************************************************************************************************/

int test_vislib_point(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);

    // Make visual.
    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_POINT, 0);
    _visual_common(&visual);

    // Create visual data.
    uint32_t n = 50;
    dvec3* pos = calloc(n, sizeof(dvec3));
    cvec4* color = calloc(n, sizeof(cvec4));
    double t = 0;
    double aspect = dvz_canvas_aspect(canvas);
    for (uint32_t i = 0; i < n; i++)
    {
        t = i / (double)(n);
        pos[i][0] = .5 * cos(M_2PI * t);
        pos[i][1] = aspect * .5 * sin(M_2PI * t);
        dvz_colormap(DVZ_CMAP_HSV, TO_BYTE(t), color[i]);
        color[i][3] = 128;
    }

    // Set visual data.
    dvz_visual_data(&visual, DVZ_PROP_POS, 0, n, pos);
    dvz_visual_data(&visual, DVZ_PROP_COLOR, 0, n, color);

    // Free the arrays.
    FREE(pos);
    FREE(color);

    // Params.
    dvz_visual_data(&visual, DVZ_PROP_MARKER_SIZE, 0, 1, (float[]){50});

    return _visual_run(&visual, "point");
}



int test_vislib_line_list(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);

    // Make visual.
    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_LINE, 0);
    _visual_common(&visual);

    // Create visual data.
    uint32_t n = 4 * 16;

    double t = 0, r = .75;
    double aspect = dvz_canvas_aspect(canvas);

    dvec3* p0 = calloc(n, sizeof(dvec3));
    dvec3* p1 = calloc(n, sizeof(dvec3));
    cvec4* color = calloc(n, sizeof(cvec4));

    for (uint32_t i = 0; i < n; i++)
    {
        t = .5 * i / (double)n;

        p0[i][0] = r * cos(M_2PI * t);
        p0[i][1] = aspect * r * sin(M_2PI * t);

        p1[i][0] = -p0[i][0];
        p1[i][1] = -p0[i][1];

        dvz_colormap_scale(DVZ_CMAP_HSV, i, 0, n, color[i]);
    }

    // Set visual data.
    dvz_visual_data(&visual, DVZ_PROP_POS, 0, n, p0);
    dvz_visual_data(&visual, DVZ_PROP_POS, 1, n, p1);
    dvz_visual_data(&visual, DVZ_PROP_COLOR, 0, n, color);

    // Free the arrays.
    FREE(p0);
    FREE(p1);
    FREE(color);

    return _visual_run(&visual, "line");
}



int test_vislib_line_strip(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);

    // Make visual.
    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_LINE_STRIP, 0);
    _visual_common(&visual);

    // Create visual data.
    uint32_t n = 10000;
    uint32_t k = 16;

    double t = 0, r = 0;
    double aspect = dvz_canvas_aspect(canvas);

    dvec3* pos = calloc(n, sizeof(dvec3));
    cvec4* color = calloc(n, sizeof(cvec4));

    for (uint32_t i = 0; i < n; i++)
    {
        t = i / (double)n;
        r = .75 * t;
        pos[i][0] = r * cos(M_2PI * k * t);
        pos[i][1] = aspect * r * sin(M_2PI * k * t);

        dvz_colormap_scale(DVZ_CMAP_HSV, t, 0, 1, color[i]);
    }

    // Set visual data.
    dvz_visual_data(&visual, DVZ_PROP_POS, 0, n, pos);
    dvz_visual_data(&visual, DVZ_PROP_COLOR, 0, n, color);

    // Free the arrays.
    FREE(pos);
    FREE(color);

    return _visual_run(&visual, "line_strip");
}



int test_vislib_triangle_list(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);

    // Make visual.
    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_TRIANGLE, 0);
    _visual_common(&visual);

    // Create visual data.
    uint32_t n = 50;

    double t = 0;
    double ms = .1;
    double aspect = dvz_canvas_aspect(canvas);
    double r = .5;

    dvec3* p0 = calloc(n, sizeof(dvec3));
    dvec3* p1 = calloc(n, sizeof(dvec3));
    dvec3* p2 = calloc(n, sizeof(dvec3));
    cvec4* color = calloc(n, sizeof(cvec4));

    for (uint32_t i = 0; i < n; i++)
    {
        t = i / (double)n;

        p0[i][0] = r * cos(M_2PI * t);
        p0[i][1] = r * sin(M_2PI * t);

        dvz_colormap_scale(DVZ_CMAP_HSV, i, 0, n, color[i]);
        color[i][3] = 128;

        // Copy the 2 other points per triangle.
        memcpy(p1[i], p0[i], sizeof(dvec3));
        memcpy(p2[i], p0[i], sizeof(dvec3));

        // Shift the points.
        p0[i][0] -= ms;
        p0[i][1] -= ms;
        p1[i][0] += ms;
        p1[i][1] -= ms;
        p2[i][0] += 0;
        p2[i][1] += ms;

        p0[i][1] *= aspect;
        p1[i][1] *= aspect;
        p2[i][1] *= aspect;
    }

    // Set visual data.
    dvz_visual_data(&visual, DVZ_PROP_POS, 0, n, p0);
    dvz_visual_data(&visual, DVZ_PROP_POS, 1, n, p1);
    dvz_visual_data(&visual, DVZ_PROP_POS, 2, n, p2);
    dvz_visual_data(&visual, DVZ_PROP_COLOR, 0, n, color);

    // Free the arrays.
    FREE(p0);
    FREE(p1);
    FREE(p2);
    FREE(color);

    return _visual_run(&visual, "triangle");
}



int test_vislib_triangle_strip(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);

    // Make visual.
    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_TRIANGLE_STRIP, 0);
    _visual_common(&visual);

    // Create visual data.
    uint32_t n = 40;

    double t = 0, a = 0;
    double m = .1;
    double aspect = dvz_canvas_aspect(canvas);

    dvec3* pos = calloc(n, sizeof(dvec3));
    cvec4* color = calloc(n, sizeof(cvec4));

    for (uint32_t i = 0; i < n; i++)
    {
        t = i / (double)(n - 1);
        a = M_2PI * t;
        pos[i][0] = (.5 + (i % 2 == 0 ? +m : -m)) * cos(a);
        pos[i][1] = aspect * (.5 + (i % 2 == 0 ? +m : -m)) * sin(a);
        dvz_colormap_scale(DVZ_CMAP_HSV, t, 0, 1, color[i]);
    }

    // Set visual data.
    dvz_visual_data(&visual, DVZ_PROP_POS, 0, n, pos);
    dvz_visual_data(&visual, DVZ_PROP_COLOR, 0, n, color);

    // Free the arrays.
    FREE(pos);
    FREE(color);

    return _visual_run(&visual, "triangle_strip");
}



int test_vislib_triangle_fan(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);

    // Make visual.
    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_TRIANGLE_FAN, 0);
    _visual_common(&visual);

    // Create visual data.
    uint32_t n = 30;

    double t = 0, a = 0;
    double aspect = dvz_canvas_aspect(canvas);

    dvec3* pos = calloc(n, sizeof(dvec3));
    cvec4* color = calloc(n, sizeof(cvec4));

    for (uint32_t i = 0; i < n; i++)
    {
        t = i / (double)(n - 1);
        a = M_2PI * t;

        pos[i][0] = .5 * cos(a);
        pos[i][1] = aspect * .5 * sin(a);

        dvz_colormap_scale(DVZ_CMAP_HSV, t, 0, 1, color[i]);
    }

    // Set visual data.
    dvz_visual_data(&visual, DVZ_PROP_POS, 0, n, pos);
    dvz_visual_data(&visual, DVZ_PROP_COLOR, 0, n, color);

    // Free the arrays.
    FREE(pos);
    FREE(color);

    return _visual_run(&visual, "triangle_fan");
}



int test_vislib_marker(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);

    // Make visual.
    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_MARKER, 0);
    _visual_common(&visual);

    // Create visual data.
    uint32_t n_sizes = 10;
    uint32_t n_markers = DVZ_MARKER_COUNT;
    uint32_t n = n_sizes * n_markers;
    DvzMarkerType marker = DVZ_MARKER_DISC;
    uint32_t k = 0;
    double x = 0, y = 0;

    dvec3* pos = calloc(n, sizeof(dvec3));
    cvec4* color = calloc(n, sizeof(cvec4));
    float* ms = calloc(n, sizeof(float));
    char* angle = calloc(n, sizeof(char));
    char* markers = calloc(n, sizeof(char));

    for (uint32_t i = 0; i < n_markers; i++)
    {
        marker = (DvzMarkerType)i;
        x = .9 * (-1 + 2 * i / (float)(n_markers - 1));
        for (uint32_t j = 0; j < n_sizes; j++)
        {
            ASSERT(k < n);
            y = .9 * (+1 - 2 * j / (float)(n_sizes - 1));

            pos[k][0] = x;
            pos[k][1] = y;

            dvz_colormap_scale(DVZ_CMAP_HSV, i, 0, n_markers, color[k]);
            ms[k] = 10 + 3 * j;
            angle[k] = (j * 64) % 256;
            markers[k] = marker;
            k++;
        }
    }
    ASSERT(k == n);

    // Set visual data.
    dvz_visual_data(&visual, DVZ_PROP_POS, 0, n, pos);
    dvz_visual_data(&visual, DVZ_PROP_COLOR, 0, n, color);
    dvz_visual_data(&visual, DVZ_PROP_MARKER_SIZE, 0, n, ms);
    dvz_visual_data(&visual, DVZ_PROP_ANGLE, 0, n, angle);
    dvz_visual_data(&visual, DVZ_PROP_MARKER_TYPE, 0, n, markers);

    // Free the arrays.
    FREE(pos);
    FREE(color);
    FREE(ms);
    FREE(angle);
    FREE(markers);

    // Params.
    dvz_visual_data(&visual, DVZ_PROP_LINE_WIDTH, 0, 1, (float[]){2});
    dvz_visual_data(&visual, DVZ_PROP_COLOR, 1, 1, (vec4){1, 1, 1, 1});

    return _visual_run(&visual, "marker");
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

int test_vislib_polygon(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);

    // Make visual.
    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_POLYGON, 0);
    _visual_common(&visual);

    // Set polygons.
    const uint32_t n0 = 4, n1 = 5, n2 = 6;
    uint32_t point_count = n0 + n1 + n2;
    dvec3 points[4 + 5 + 6];
    double aspect = dvz_canvas_aspect(canvas);
    _add_polygon(points, n0, M_PI / 2, (dvec3){-.65, 0, 0}, aspect);
    _add_polygon(points + n0, n1, M_PI / 4, (dvec3){0, 0, 0}, aspect);
    _add_polygon(points + n0 + n1, n2, M_PI / 2, (dvec3){+.65, 0, 0}, aspect);

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

    return _visual_run(&visual, "polygon");
}



int test_vislib_path(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);

    // Make visual.
    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_PATH, 0);
    _visual_common(&visual);

    // Set paths.
    uint32_t N = 1000;
    uint32_t n_paths = 11;

    // Allocations.
    dvec3* points = calloc(N * n_paths, sizeof(dvec3));
    cvec4* colors = calloc(N * n_paths, sizeof(cvec4));
    uint32_t* path_lengths = calloc(n_paths, sizeof(uint32_t));

    // Make data
    double t = 0;
    double d = 1.0 / (double)(N - 1);
    double a = .15;
    double offset = 0;
    int32_t n = (int32_t)N;
    uint32_t k = 0;

    for (uint32_t j = 0; j < n_paths; j++)
    {
        offset = -.75 + 1.5 * j / (double)(n_paths - 1);
        for (int32_t i = 0; i < n; i++)
        {
            t = -.9 + 1.8 * i * d;

            points[k][0] = t;
            points[k][1] = a * sin(M_2PI * t / .9) + offset;

            if (j == 0)
                dvz_colormap_scale(DVZ_CMAP_HSV, i, 0, N - 1, colors[k]);
            else
                dvz_colormap_scale(DVZ_CMAP_HSV, j, 1, n_paths, colors[k]);

            k++;
        }
    }
    ASSERT(k == N * n_paths);

    // Path lengths.
    for (uint32_t i = 0; i < n_paths; i++)
        path_lengths[i] = N;

    // Set visual data.
    dvz_visual_data(&visual, DVZ_PROP_POS, 0, N * n_paths, points);
    dvz_visual_data(&visual, DVZ_PROP_COLOR, 0, N * n_paths, colors);
    dvz_visual_data(&visual, DVZ_PROP_LENGTH, 0, n_paths, path_lengths);

    // Free the arrays.
    FREE(points);
    FREE(colors);
    FREE(path_lengths);

    // Params.
    dvz_visual_data(&visual, DVZ_PROP_LINE_WIDTH, 0, 1, (float[]){10});
    dvz_visual_data(&visual, DVZ_PROP_CAP_TYPE, 0, 1, (int32_t[]){DVZ_CAP_ROUND});
    dvz_visual_data(&visual, DVZ_PROP_MITER_LIMIT, 0, 1, (float[]){4});
    dvz_visual_data(&visual, DVZ_PROP_JOIN_TYPE, 0, 1, (int32_t[]){DVZ_JOIN_ROUND});

    return _visual_run(&visual, "path");
}



static void _image_data(DvzVisual* visual, uint32_t n)
{
    ASSERT(visual != NULL);
    ASSERT(n > 0);

    float x0 = 0, x1 = 0, z = 0, w = 2.0 / (float)n;

    for (uint32_t i = 0; i < n; i++)
    {
        x0 = -1 + i * w;
        x1 = -1 + (i + 1) * w;

        dvz_visual_data_append(visual, DVZ_PROP_POS, 0, 1, (double[]){x0, x1, z});
        dvz_visual_data_append(visual, DVZ_PROP_POS, 1, 1, (double[]){x1, x1, z});
        dvz_visual_data_append(visual, DVZ_PROP_POS, 2, 1, (double[]){x1, x0, z});
        dvz_visual_data_append(visual, DVZ_PROP_POS, 3, 1, (double[]){x0, x0, z});

        dvz_visual_data_append(visual, DVZ_PROP_TEXCOORDS, 0, 1, (float[]){0, 0});
        dvz_visual_data_append(visual, DVZ_PROP_TEXCOORDS, 1, 1, (float[]){1, 0});
        dvz_visual_data_append(visual, DVZ_PROP_TEXCOORDS, 2, 1, (float[]){1, 1});
        dvz_visual_data_append(visual, DVZ_PROP_TEXCOORDS, 3, 1, (float[]){0, 1});
    }
}

int test_vislib_image_1(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);

    // Make visual.
    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_IMAGE, 0);
    _visual_common(&visual);

    // Create visual data.
    _image_data(&visual, 1);

    // Texture.
    // https://pixabay.com/illustrations/earth-planet-world-globe-space-1617121/
    DvzTexture* texture = _earth_texture(canvas->gpu->context);
    dvz_visual_texture(&visual, DVZ_SOURCE_TYPE_IMAGE, 0, texture);

    return _visual_run(&visual, "image");
}



int test_vislib_image_cmap(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);

    // Make visual.
    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_IMAGE_CMAP, 0);
    _visual_common(&visual);

    // Create visual data.
    _image_data(&visual, 1);

    // Texture.
    DvzTexture* texture = _synthetic_texture(canvas->gpu->context);
    dvz_visual_texture(&visual, DVZ_SOURCE_TYPE_IMAGE, 0, texture);

    // Params
    dvz_visual_data(&visual, DVZ_PROP_RANGE, 0, 1, (float[]){-1, +1});

    return _visual_run(&visual, "image_cmap");
}



static int _vislib_axes(TestContext* tc, DvzAxisCoord coord, const char* name)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);
    dvz_canvas_clear_color(canvas, 1, 1, 1);
    canvas->viewport.margins[0] = 25;
    canvas->viewport.margins[1] = 25;
    canvas->viewport.margins[2] = 100;
    canvas->viewport.margins[3] = 100;

    // Make visual.
    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_AXES_2D, coord);
    _visual_common(&visual);
    dvz_visual_data(&visual, DVZ_PROP_VIEWPORT, 1, 1, &canvas->viewport);


    // Font atlas.
    DvzFontAtlas* atlas = &canvas->gpu->context->font_atlas;
    ASSERT(atlas != NULL);
    ASSERT(atlas->font_str != NULL);
    ASSERT(strlen(atlas->font_str) > 0);
    ASSERT(atlas->texture != NULL);

    dvz_visual_texture(&visual, DVZ_SOURCE_TYPE_FONT_ATLAS, 0, atlas->texture);


    // Prepare the data.
    const uint32_t N = 5;
    const uint32_t MAX_GLYPHS = 12;
    const uint32_t N_minor = 3 * (N - 1);
    double t = 0;

    double* xticks = calloc(N, sizeof(double));
    double* xticks_minor = calloc(N_minor, sizeof(double));
    char** strings = calloc(N, sizeof(char*));
    // NOTE: the text array must live through the entire test.
    char* text = calloc(N * MAX_GLYPHS, sizeof(char));

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



    int res = _visual_run(&visual, name);

    FREE(xticks);
    FREE(xticks_minor);
    FREE(text);
    FREE(strings);

    dvz_canvas_clear_color(canvas, 0, 0, 0);
    return res;
}



int test_vislib_axes_2D_x(TestContext* tc)
{
    return _vislib_axes(tc, DVZ_AXES_COORD_X, "axes_2D_x");
}



int test_vislib_axes_2D_y(TestContext* tc)
{
    return _vislib_axes(tc, DVZ_AXES_COORD_Y, "axes_2D_y");
}



int test_vislib_mesh(TestContext* tc) { return 0; }



int test_vislib_volume(TestContext* tc) { return 0; }



int test_vislib_volume_slice(TestContext* tc) { return 0; }
