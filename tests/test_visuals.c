#include "../include/datoviz/colormaps.h"
#include "../include/datoviz/visuals.h"
#include "proto.h"
#include "tests.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _marker_visual(DvzVisual* visual)
{
    DvzCanvas* canvas = visual->canvas;
    DvzProp* prop = NULL;

    // Graphics.
    dvz_visual_graphics(visual, dvz_graphics_builtin(canvas, DVZ_GRAPHICS_POINT, 0));

    // Sources.
    {
        // Vertex buffer.
        dvz_visual_source( //
            visual, DVZ_SOURCE_TYPE_VERTEX, 0, DVZ_PIPELINE_GRAPHICS, 0, 0, sizeof(DvzVertex), 0);



        // Binding #0: uniform buffer MVP
        dvz_visual_source( //
            visual, DVZ_SOURCE_TYPE_MVP, 0, DVZ_PIPELINE_GRAPHICS, 0, 0, sizeof(DvzMVP),
            DVZ_SOURCE_FLAG_MAPPABLE);

        // Binding #1: uniform buffer viewport
        dvz_visual_source(
            visual, DVZ_SOURCE_TYPE_VIEWPORT, 0, DVZ_PIPELINE_GRAPHICS, 0, 1, sizeof(DvzViewport),
            0);

        // Binding #2: uniform buffer params
        dvz_visual_source(
            visual, DVZ_SOURCE_TYPE_PARAM, 0, DVZ_PIPELINE_GRAPHICS, 0, 2,
            sizeof(DvzGraphicsPointParams), 0);
    }

    // Props.
    {
        // Vertex pos.
        prop =
            dvz_visual_prop(visual, DVZ_PROP_POS, 0, DVZ_DTYPE_DVEC3, DVZ_SOURCE_TYPE_VERTEX, 0);
        dvz_visual_prop_cast(
            prop, 0, offsetof(DvzVertex, pos),
            DVZ_DTYPE_VEC3, // NOTE: cast to float
            DVZ_ARRAY_COPY_SINGLE, 1);

        // Vertex color.
        prop =
            dvz_visual_prop(visual, DVZ_PROP_COLOR, 0, DVZ_DTYPE_CVEC4, DVZ_SOURCE_TYPE_VERTEX, 0);
        dvz_visual_prop_copy(prop, 1, offsetof(DvzVertex, color), DVZ_ARRAY_COPY_SINGLE, 1);

        // MVP
        // Model.
        prop = dvz_visual_prop(visual, DVZ_PROP_MODEL, 0, DVZ_DTYPE_MAT4, DVZ_SOURCE_TYPE_MVP, 0);
        dvz_visual_prop_copy(prop, 0, offsetof(DvzMVP, model), DVZ_ARRAY_COPY_SINGLE, 1);

        // View.
        prop = dvz_visual_prop(visual, DVZ_PROP_VIEW, 0, DVZ_DTYPE_MAT4, DVZ_SOURCE_TYPE_MVP, 0);
        dvz_visual_prop_copy(prop, 1, offsetof(DvzMVP, view), DVZ_ARRAY_COPY_SINGLE, 1);

        // Proj.
        prop = dvz_visual_prop(visual, DVZ_PROP_PROJ, 0, DVZ_DTYPE_MAT4, DVZ_SOURCE_TYPE_MVP, 0);
        dvz_visual_prop_copy(prop, 2, offsetof(DvzMVP, proj), DVZ_ARRAY_COPY_SINGLE, 1);

        // // Viewport.
        // prop = dvz_visual_prop(visual, DVZ_PROP_VIEWPORT, 0, DVZ_DTYPE_CUSTOM,
        // DVZ_SOURCE_TYPE_VIEWPORT, 0); dvz_visual_prop_copy(prop, 0, 0, DVZ_ARRAY_COPY_SINGLE,
        // 1);



        // Param: marker size.
        prop = dvz_visual_prop(
            visual, DVZ_PROP_MARKER_SIZE, 0, DVZ_DTYPE_FLOAT, DVZ_SOURCE_TYPE_PARAM, 0);
        dvz_visual_prop_copy(
            prop, 0, offsetof(DvzGraphicsPointParams, point_size), DVZ_ARRAY_COPY_SINGLE, 1);
    }
}

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



/*************************************************************************************************/
/*  Visuals tests                                                                                */
/*************************************************************************************************/

static void _visual_mvp_buffer(DvzVisual* visual)
{
    DvzCanvas* canvas = visual->canvas;
    DvzContext* context = canvas->gpu->context;

    // Binding data.
    DvzMVP mvp = {0};
    DvzBufferRegions br_mvp = dvz_ctx_buffers(
        context, DVZ_BUFFER_TYPE_UNIFORM_MAPPABLE, canvas->swapchain.img_count, sizeof(DvzMVP));
    glm_mat4_identity(mvp.model);
    glm_mat4_identity(mvp.view);
    glm_mat4_identity(mvp.proj);
    dvz_canvas_buffers(canvas, br_mvp, 0, sizeof(DvzMVP), &mvp);
    dvz_visual_buffer(visual, DVZ_SOURCE_TYPE_MVP, 0, br_mvp);
}

static void _visual_params_buffer(DvzVisual* visual)
{
    DvzCanvas* canvas = visual->canvas;
    DvzContext* context = canvas->gpu->context;

    // Upload params.
    DvzGraphicsPointParams params = {.point_size = 50};
    DvzBufferRegions br_params =
        dvz_ctx_buffers(context, DVZ_BUFFER_TYPE_UNIFORM, 1, sizeof(DvzGraphicsPointParams));
    dvz_upload_buffer(context, br_params, 0, sizeof(DvzGraphicsPointParams), &params);
    dvz_visual_buffer(visual, DVZ_SOURCE_TYPE_PARAM, 0, br_params);
}

static void _visual_viewport_buffer(DvzVisual* visual)
{
    DvzCanvas* canvas = visual->canvas;
    DvzContext* context = canvas->gpu->context;

    // Upload the data to the GPU.
    DvzBufferRegions br_viewport = dvz_ctx_buffers(context, DVZ_BUFFER_TYPE_UNIFORM, 1, 16);
    dvz_visual_buffer(visual, DVZ_SOURCE_TYPE_VIEWPORT, 0, br_viewport);
}

static DvzVertex* _visual_data(DvzVisual* visual, uint32_t N)
{
    DvzVertex* vertices = calloc(N, sizeof(DvzVertex));
    for (uint32_t i = 0; i < N; i++)
    {
        vertices[i].pos[0] = .9 * (-1 + 2 * i / ((float)N - 1));
        dvz_colormap_scale(DVZ_CMAP_HSV, i, 0, N, vertices[i].color);
    }

    return vertices;
}

static void _visual_run(DvzVisual* visual)
{
    DvzCanvas* canvas = visual->canvas;

    dvz_visual_update(visual, canvas->viewport, (DvzDataCoords){0}, NULL);
    dvz_event_callback(
        canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _visual_canvas_fill, visual);
    dvz_app_run(canvas->app, N_FRAMES);
}

int test_visuals_1(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    DvzContext* context = tc->context;

    ASSERT(canvas != NULL);
    ASSERT(context != NULL);

    // Create the visual.
    DvzVisual visual = dvz_visual(canvas);
    _marker_visual(&visual);

    // Binding resources.
    _visual_mvp_buffer(&visual);
    _visual_params_buffer(&visual);
    _visual_viewport_buffer(&visual);

    // Vertex data.
    const uint32_t N = 12;
    DvzVertex* vertices = _visual_data(&visual, N);

    // Set visual data ia user-provided data (underlying vertex buffer created automatically).
    dvz_visual_data_source(&visual, DVZ_SOURCE_TYPE_VERTEX, 0, 0, N, N, vertices);
    FREE(vertices);

    // Run the app.
    _visual_run(&visual);

    // Check screenshot.
    int res = check_canvas(canvas, "test_visuals_1");

    return res;
}



int test_visuals_2(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    DvzContext* context = tc->context;

    ASSERT(canvas != NULL);
    ASSERT(context != NULL);

    // Create the visual.
    DvzVisual visual = dvz_visual(canvas);
    _marker_visual(&visual);

    // Binding resources.
    mat4 id = GLM_MAT4_IDENTITY_INIT;
    dvz_visual_data(&visual, DVZ_PROP_MODEL, 0, 1, id);
    dvz_visual_data(&visual, DVZ_PROP_VIEW, 0, 1, id);
    dvz_visual_data(&visual, DVZ_PROP_PROJ, 0, 1, id);
    dvz_visual_data_source(&visual, DVZ_SOURCE_TYPE_VIEWPORT, 0, 0, 1, 1, &canvas->viewport);
    dvz_visual_data(&visual, DVZ_PROP_MARKER_SIZE, 0, 1, (float[]){50});

    // Vertex data.
    const uint32_t N = 12;
    DvzVertex* vertices = _visual_data(&visual, N);

    // Set visual data as user-provided data (underlying vertex buffer created automatically).
    dvz_visual_data_source(&visual, DVZ_SOURCE_TYPE_VERTEX, 0, 0, N, N, vertices);
    FREE(vertices);

    // Run the app.
    _visual_run(&visual);

    // Check screenshot.
    int res = check_canvas(canvas, "test_visuals_2");

    return res;
}
