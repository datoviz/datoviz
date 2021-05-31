#include "../include/datoviz/colormaps.h"
#include "../include/datoviz/interact.h"
#include "../include/datoviz/visuals.h"
#include "proto.h"
#include "tests.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct TestScene TestScene;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct TestScene
{
    DvzBufferRegions br_mvp;
    // DvzMVP mvp;
    DvzInteract interact;
};



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _visual_create(DvzVisual* visual)
{
    ASSERT(visual != NULL);

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

        // Viewport.
        prop = dvz_visual_prop(
            visual, DVZ_PROP_VIEWPORT, 0, DVZ_DTYPE_CUSTOM, DVZ_SOURCE_TYPE_VIEWPORT, 0);
        dvz_visual_prop_size(prop, sizeof(DvzViewport));
        dvz_visual_prop_copy(prop, 0, 0, DVZ_ARRAY_COPY_SINGLE, 1);


        // Param: marker size.
        prop = dvz_visual_prop(
            visual, DVZ_PROP_MARKER_SIZE, 0, DVZ_DTYPE_FLOAT, DVZ_SOURCE_TYPE_PARAM, 0);
        dvz_visual_prop_copy(
            prop, 0, offsetof(DvzGraphicsPointParams, point_size), DVZ_ARRAY_COPY_SINGLE, 1);
    }

    visual->user_data = calloc(1, sizeof(TestScene));
    ((TestScene*)visual->user_data)->interact = dvz_interact_builtin(canvas, DVZ_INTERACT_PANZOOM);
}

static void _visual_destroy(DvzVisual* visual)
{
    ASSERT(visual != NULL);
    dvz_visual_destroy(visual);
    FREE(visual->user_data);
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

static void _visual_mvp_buffer(DvzVisual* visual)
{
    DvzCanvas* canvas = visual->canvas;
    DvzContext* context = canvas->gpu->context;
    TestScene* scene = visual->user_data;
    ASSERT(scene != NULL);

    // Binding data.
    scene->br_mvp = dvz_ctx_buffers(
        context, DVZ_BUFFER_TYPE_UNIFORM_MAPPABLE, canvas->swapchain.img_count, sizeof(DvzMVP));
    glm_mat4_identity(scene->interact.mvp.model);
    glm_mat4_identity(scene->interact.mvp.view);
    glm_mat4_identity(scene->interact.mvp.proj);
    dvz_canvas_buffers(canvas, scene->br_mvp, 0, sizeof(DvzMVP), &scene->interact.mvp);
    dvz_visual_buffer(visual, DVZ_SOURCE_TYPE_MVP, 0, scene->br_mvp);
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
    dvz_upload_buffer(context, br_viewport, 0, sizeof(DvzViewport), &canvas->viewport);
    dvz_visual_buffer(visual, DVZ_SOURCE_TYPE_VIEWPORT, 0, br_viewport);
}

static void _visual_bindings(DvzVisual* visual)
{
    ASSERT(visual != NULL);
    DvzCanvas* canvas = visual->canvas;

    // Binding resources.
    // mat4 id = GLM_MAT4_IDENTITY_INIT;
    // dvz_visual_data(visual, DVZ_PROP_MODEL, 0, 1, id);
    // dvz_visual_data(visual, DVZ_PROP_VIEW, 0, 1, id);
    // dvz_visual_data(visual, DVZ_PROP_PROJ, 0, 1, id);
    dvz_visual_data(visual, DVZ_PROP_MARKER_SIZE, 0, 1, (float[]){50});
    dvz_visual_data(visual, DVZ_PROP_VIEWPORT, 0, 1, &canvas->viewport);

    _visual_mvp_buffer(visual);
}

static inline void _visual_pos(DvzVisual* visual, uint32_t i, uint32_t N, vec3* pos)
{
    pos[0][0] = .9 * (-1 + 2 * i / ((float)N - 1));
}

static inline void _visual_dpos(DvzVisual* visual, uint32_t i, uint32_t N, dvec3* pos)
{
    pos[0][0] = .9 * (-1 + 2 * i / ((float)N - 1));
}

static inline void _visual_color(DvzVisual* visual, uint32_t i, uint32_t N, cvec4* color)
{
    dvz_colormap_scale(DVZ_CMAP_HSV, i, 0, N, color[0]);
}

static DvzVertex* _visual_data_source(DvzVisual* visual, uint32_t N)
{
    DvzVertex* vertices = calloc(N, sizeof(DvzVertex));
    for (uint32_t i = 0; i < N; i++)
    {
        _visual_pos(visual, i, N, &vertices[i].pos);
        _visual_color(visual, i, N, &vertices[i].color);
    }

    return vertices;
}

static void _visual_data(DvzVisual* visual, uint32_t N)
{
    dvec3* pos = calloc(N, sizeof(dvec3));
    cvec4* color = calloc(N, sizeof(cvec4));

    for (uint32_t i = 0; i < N; i++)
    {
        _visual_dpos(visual, i, N, &pos[i]);
        _visual_color(visual, i, N, &color[i]);
    }

    dvz_visual_data(visual, DVZ_PROP_POS, 0, N, pos);
    dvz_visual_data(visual, DVZ_PROP_COLOR, 0, N, color);

    FREE(pos);
    FREE(color);
}

static void _visual_frame(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    DvzVisual* visual = ev.user_data;
    ASSERT(visual != NULL);

    TestScene* scene = visual->user_data;
    ASSERT(scene != NULL);

    dvz_interact_update(&scene->interact, canvas->viewport, &canvas->mouse, &canvas->keyboard);
    dvz_canvas_buffers(canvas, scene->br_mvp, 0, sizeof(DvzMVP), &scene->interact.mvp);
}

static void _visual_run(DvzVisual* visual, uint32_t n_frames)
{
    DvzCanvas* canvas = visual->canvas;

    dvz_visual_update(visual, canvas->viewport, (DvzDataCoords){0}, NULL);
    dvz_event_callback(
        canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _visual_canvas_fill, visual);
    dvz_event_callback(canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _visual_frame, visual);
    dvz_app_run(canvas->app, n_frames);
    _visual_destroy(visual);
}



/*************************************************************************************************/
/*  Visuals tests                                                                                */
/*************************************************************************************************/

int test_visuals_sources(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    DvzContext* context = tc->context;

    ASSERT(canvas != NULL);
    ASSERT(context != NULL);

    // Create the visual.
    DvzVisual visual = dvz_visual(canvas);
    _visual_create(&visual);

    // Binding resources.
    _visual_mvp_buffer(&visual);
    _visual_params_buffer(&visual);
    _visual_viewport_buffer(&visual);

    // Vertex data.
    const uint32_t N = 12;
    DvzVertex* vertices = _visual_data_source(&visual, N);

    // Set visual data ia user-provided data (underlying vertex buffer created automatically).
    dvz_visual_data_source(&visual, DVZ_SOURCE_TYPE_VERTEX, 0, 0, N, N, vertices);
    FREE(vertices);

    // Run the app.
    _visual_run(&visual, N_FRAMES);

    // Check screenshot.
    int res = check_canvas(canvas, "test_visuals_sources");

    return res;
}



int test_visuals_props(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    DvzContext* context = tc->context;

    ASSERT(canvas != NULL);
    ASSERT(context != NULL);

    // Create the visual.
    DvzVisual visual = dvz_visual(canvas);
    _visual_create(&visual);
    _visual_bindings(&visual);

    // Vertex data.
    const uint32_t N = 12;
    _visual_data(&visual, N);

    // Run the app.
    _visual_run(&visual, N_FRAMES);

    // Check screenshot.
    int res = check_canvas(canvas, "test_visuals_props");

    return res;
}



static void _visual_frame_color(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    DvzVisual* visual = ev.user_data;
    ASSERT(visual != NULL);

    uint32_t N = dvz_visual_item_count(visual);

    cvec4* color = calloc(N, sizeof(cvec4));
    for (uint32_t i = 0; i < N; i++)
        _visual_color(visual, (i + ev.u.f.idx) % N, N, &color[i]);
    dvz_visual_data(visual, DVZ_PROP_COLOR, 0, N, color);
    FREE(color);

    dvz_visual_update(visual, canvas->viewport, (DvzDataCoords){0}, NULL);
}

int test_visuals_update_color(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    DvzContext* context = tc->context;

    ASSERT(canvas != NULL);
    ASSERT(context != NULL);

    // Create the visual.
    DvzVisual visual = dvz_visual(canvas);
    _visual_create(&visual);
    _visual_bindings(&visual);

    // Vertex data.
    const uint32_t N = 12;
    _visual_data(&visual, N);

    dvz_event_callback(
        canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _visual_frame_color, &visual);

    // Run the app.
    _visual_run(&visual, DEBUG_TEST ? 0 : 3 + N / 2);

    // Check screenshot.
    int res = check_canvas(canvas, "test_visuals_update_color");

    return res;
}



static void _visual_frame_pos(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    DvzVisual* visual = ev.user_data;
    ASSERT(visual != NULL);

    uint32_t k = 1;

    if (ev.u.f.idx % k != 0)
        return;

    uint32_t N = 3 + (ev.u.f.idx / k) % 12;
    _visual_data(visual, N);

    dvz_visual_update(visual, canvas->viewport, (DvzDataCoords){0}, NULL);
    dvz_canvas_to_refill(visual->canvas);
}

int test_visuals_update_pos(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    DvzContext* context = tc->context;

    ASSERT(canvas != NULL);
    ASSERT(context != NULL);

    // Create the visual.
    DvzVisual visual = dvz_visual(canvas);
    _visual_create(&visual);
    _visual_bindings(&visual);

    // Vertex data.
    const uint32_t N = 3;
    _visual_data(&visual, N);

    dvz_event_callback(
        canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _visual_frame_pos, &visual);

    // Run the app.
    _visual_run(&visual, N_FRAMES);

    // Check screenshot.
    int res = check_canvas(canvas, "test_visuals_update_pos");

    return res;
}



int test_visuals_partial(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    DvzContext* context = tc->context;

    ASSERT(canvas != NULL);
    ASSERT(context != NULL);

    // Create the visual.
    DvzVisual visual = dvz_visual(canvas);
    _visual_create(&visual);
    _visual_bindings(&visual);

    // Vertex data.
    const uint32_t N = 12;
    _visual_data(&visual, N);

    // Partial data update.
    dvec3 pos = {0, .5, 0};
    dvz_visual_data_partial(&visual, DVZ_PROP_POS, 0, 3, 6, 1, pos);

    // Run the app.
    _visual_run(&visual, N_FRAMES);

    // Check screenshot.
    int res = check_canvas(canvas, "test_visuals_partial");

    return res;
}



static void _visual_append(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    DvzVisual* visual = ev.user_data;
    ASSERT(visual != NULL);

    uint32_t N = 12;
    uint32_t i = ev.u.f.idx;
    if (i >= N)
        return;

    // Append an element.
    dvec3 pos = {0};
    _visual_dpos(visual, i, N, &pos);
    dvz_visual_data_append(visual, DVZ_PROP_POS, 0, 1, &pos);

    cvec4 color = {0};
    _visual_color(visual, i, N, &color);
    dvz_visual_data_append(visual, DVZ_PROP_COLOR, 0, 1, &color);

    dvz_visual_update(visual, canvas->viewport, (DvzDataCoords){0}, NULL);
    dvz_canvas_to_refill(visual->canvas);
}

int test_visuals_append(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    DvzContext* context = tc->context;

    ASSERT(canvas != NULL);
    ASSERT(context != NULL);

    // Create the visual.
    DvzVisual visual = dvz_visual(canvas);
    _visual_create(&visual);
    _visual_bindings(&visual);

    // Vertex data.
    const uint32_t N = 1;
    _visual_data(&visual, N);

    dvz_event_callback(canvas, DVZ_EVENT_FRAME, 0, DVZ_EVENT_MODE_SYNC, _visual_append, &visual);

    // Run the app.
    _visual_run(&visual, 13);

    // Check screenshot.
    int res = check_canvas(canvas, "test_visuals_append");

    return res;
}



int test_visuals_shared(TestContext* tc)
{
    /* In this test, we create a visual with 2 identical triangle graphics pipelines with
     * independent vertex buffers, but that share the same GPU buffers for MVP and viewport.
     */
    DvzCanvas* canvas = tc->canvas;
    DvzContext* context = tc->context;

    ASSERT(canvas != NULL);
    ASSERT(context != NULL);

    // Create the visual.
    DvzVisual visual_ = dvz_visual(canvas);
    DvzVisual* visual = &visual_;

    // Graphics.
    dvz_visual_graphics(visual, dvz_graphics_builtin(canvas, DVZ_GRAPHICS_TRIANGLE, 0));
    dvz_visual_graphics(visual, dvz_graphics_builtin(canvas, DVZ_GRAPHICS_TRIANGLE, 0));

    // Sources for graphics #0.
    {
        // Vertex buffer.
        dvz_visual_source(                                               //
            visual, DVZ_SOURCE_TYPE_VERTEX, 0, DVZ_PIPELINE_GRAPHICS, 0, //
            0, sizeof(DvzVertex), 0);

        // Binding #0: uniform buffer MVP
        dvz_visual_source(                                            //
            visual, DVZ_SOURCE_TYPE_MVP, 0, DVZ_PIPELINE_GRAPHICS, 0, //
            0, sizeof(DvzMVP), DVZ_SOURCE_FLAG_MAPPABLE);

        // Binding #1: uniform buffer viewport
        dvz_visual_source(
            visual, DVZ_SOURCE_TYPE_VIEWPORT, 0, DVZ_PIPELINE_GRAPHICS, 0, //
            1, sizeof(DvzViewport), 0);
    }

    // Sources for graphics #1.
    {
        // Vertex buffer.
        dvz_visual_source(                                               //
            visual, DVZ_SOURCE_TYPE_VERTEX, 1, DVZ_PIPELINE_GRAPHICS, 1, //
            0, sizeof(DvzVertex), 0);

        // Binding #0: shared uniform buffer MVP
        dvz_visual_source(
            visual, DVZ_SOURCE_TYPE_MVP, 1, DVZ_PIPELINE_GRAPHICS, 1, //
            0, sizeof(DvzMVP), DVZ_SOURCE_FLAG_MAPPABLE);
        dvz_visual_source_share(visual, DVZ_SOURCE_TYPE_MVP, 0, 1);

        // Binding #1: shared uniform buffer viewport
        dvz_visual_source(
            visual, DVZ_SOURCE_TYPE_VIEWPORT, 1, DVZ_PIPELINE_GRAPHICS, 1, //
            1, sizeof(DvzViewport), 0);
        dvz_visual_source_share(visual, DVZ_SOURCE_TYPE_VIEWPORT, 0, 1);
    }

    DvzProp* prop = NULL;

    // Props for graphics #0.
    {
        // Vertex pos.
        prop =
            dvz_visual_prop(visual, DVZ_PROP_POS, 0, DVZ_DTYPE_DVEC3, DVZ_SOURCE_TYPE_VERTEX, 0);
        dvz_visual_prop_cast(
            prop, 0, offsetof(DvzVertex, pos), DVZ_DTYPE_VEC3, DVZ_ARRAY_COPY_SINGLE, 1);

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

        // Viewport.
        prop = dvz_visual_prop(
            visual, DVZ_PROP_VIEWPORT, 0, DVZ_DTYPE_CUSTOM, DVZ_SOURCE_TYPE_VIEWPORT, 0);
        dvz_visual_prop_size(prop, sizeof(DvzViewport));
        dvz_visual_prop_copy(prop, 0, 0, DVZ_ARRAY_COPY_SINGLE, 1);
    }

    // Props for graphics #1.
    {
        // Vertex pos.
        prop =
            dvz_visual_prop(visual, DVZ_PROP_POS, 1, DVZ_DTYPE_DVEC3, DVZ_SOURCE_TYPE_VERTEX, 1);
        dvz_visual_prop_cast(
            prop, 0, offsetof(DvzVertex, pos), DVZ_DTYPE_VEC3, DVZ_ARRAY_COPY_SINGLE, 1);

        // Vertex color.
        prop =
            dvz_visual_prop(visual, DVZ_PROP_COLOR, 1, DVZ_DTYPE_CVEC4, DVZ_SOURCE_TYPE_VERTEX, 1);
        dvz_visual_prop_copy(prop, 1, offsetof(DvzVertex, color), DVZ_ARRAY_COPY_SINGLE, 1);
    }

    visual->user_data = calloc(1, sizeof(TestScene));
    TestScene* scene = (TestScene*)visual->user_data;
    ((TestScene*)visual->user_data)->interact = dvz_interact_builtin(canvas, DVZ_INTERACT_PANZOOM);

    // Vertex data.
    cvec4 colors[] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};

    // Data props.
    dvz_visual_data(visual, DVZ_PROP_POS, 0, 3, (dvec3[]){{-1, 0, 0}, {0, 0, 0}, {-.5, 1, 0}});
    dvz_visual_data(visual, DVZ_PROP_COLOR, 0, 3, colors);

    dvz_visual_data(visual, DVZ_PROP_POS, 1, 3, (dvec3[]){{0, -1, 0}, {1, -1, 0}, {+.5, 0, 0}});
    dvz_visual_data(visual, DVZ_PROP_COLOR, 1, 3, colors);

    // Shared props.
    // NOTE: this data is set to the first graphics pipeline, but since the MVP and viewport
    // buffers are shared, they will be used by the second graphics pipeline too.
    dvz_visual_data(visual, DVZ_PROP_MODEL, 0, 1, GLM_MAT4_IDENTITY);
    dvz_visual_data(visual, DVZ_PROP_VIEW, 0, 1, GLM_MAT4_IDENTITY);
    dvz_visual_data(visual, DVZ_PROP_PROJ, 0, 1, GLM_MAT4_IDENTITY);
    dvz_visual_data(visual, DVZ_PROP_VIEWPORT, 0, 1, &canvas->viewport);

    // HACK: this buffer is not used by the visual, but we need here because the testing code here
    // updates this buffer at every frame. Therefore the interactivity will not work in this test.
    scene->br_mvp = dvz_ctx_buffers(
        context, DVZ_BUFFER_TYPE_UNIFORM_MAPPABLE, canvas->swapchain.img_count, sizeof(DvzMVP));
    glm_mat4_identity(scene->interact.mvp.model);
    glm_mat4_identity(scene->interact.mvp.view);
    glm_mat4_identity(scene->interact.mvp.proj);
    dvz_canvas_buffers(canvas, scene->br_mvp, 0, sizeof(DvzMVP), &scene->interact.mvp);

    // Run the app.
    _visual_run(visual, N_FRAMES);

    // Check screenshot.
    int res = check_canvas(canvas, "test_visuals_shared");

    return res;
}
