#include "test_scene.h"
#include "../external/video.h"
#include "../include/datoviz/builtin_visuals.h"
#include "../include/datoviz/scene.h"
#include "../src/ticks.h"
#include "utils.h"

BEGIN_INCL_NO_WARN
#include "../external/gif.h"
END_INCL_NO_WARN


/*************************************************************************************************/
/*  Axes tests                                                                                   */
/*************************************************************************************************/

int test_axes_1(TestContext* context)
{
    DvzAxesContext ctx = {0};
    ctx.coord = DVZ_AXES_COORD_X;
    ctx.size_viewport = 1000;
    ctx.size_glyph = 10;

    DvzAxesTicks ticks = create_ticks(0, 1, 11, ctx);
    ticks.lmin_in = 0;
    ticks.lmax_in = 1;
    ticks.lstep = .1;
    const uint32_t N = tick_count(ticks.lmin_in, ticks.lmax_in, ticks.lstep);
    ticks.value_count = N;

    ticks.format = DVZ_TICK_FORMAT_SCIENTIFIC;
    ticks.precision = 3;
    ticks.labels = calloc(N * MAX_GLYPHS_PER_TICK, sizeof(char));
    make_labels(&ticks, &ctx, false);
    for (uint32_t i = 0; i < N; i++)
    {
        if (strlen(&ticks.labels[i * MAX_GLYPHS_PER_TICK]) == 0)
            break;
        log_debug("%s ", &ticks.labels[i * MAX_GLYPHS_PER_TICK]);
    }
    FREE(ticks.labels);
    return 0;
}



int test_axes_2(TestContext* context)
{
    DvzAxesContext ctx = {0};
    ctx.coord = DVZ_AXES_COORD_X;
    ctx.size_viewport = 2000;
    ctx.size_glyph = 5;

    DvzAxesTicks ticks = dvz_ticks(-10.12, 20.34, ctx);
    for (uint32_t i = 0; i < ticks.value_count; i++)
        log_debug("tick #%02d: %s", i, &ticks.labels[i * MAX_GLYPHS_PER_TICK]);
    AT(!duplicate_labels(&ticks, &ctx));

    ticks = dvz_ticks(.001, .002, ctx);
    for (uint32_t i = 0; i < ticks.value_count; i++)
        log_debug("tick #%02d: %s", i, &ticks.labels[i * MAX_GLYPHS_PER_TICK]);
    AT(!duplicate_labels(&ticks, &ctx));

    ticks = dvz_ticks(-0.131456, -0.124789, ctx);
    for (uint32_t i = 0; i < ticks.value_count; i++)
        log_debug("tick #%02d: %s", i, &ticks.labels[i * MAX_GLYPHS_PER_TICK]);
    AT(!duplicate_labels(&ticks, &ctx));

    dvz_ticks_destroy(&ticks);
    return 0;
}



int test_axes_3(TestContext* context)
{
    DvzAxesContext ctx = {0};
    ctx.coord = DVZ_AXES_COORD_X;
    ctx.size_viewport = 1000;
    ctx.size_glyph = 10;

    DvzAxesTicks ticks = {0};

    // No extensions.
    double x0 = -2.123, x1 = +2.456;
    ctx.extensions = 0;
    ticks = dvz_ticks(x0, x1, ctx);
    for (uint32_t i = 0; i < ticks.value_count; i++)
        log_debug("tick #%02d: %s", i, &ticks.labels[i * MAX_GLYPHS_PER_TICK]);

    // 1 extension on each side.
    ctx.extensions = 1;
    ticks = dvz_ticks(x0, x1, ctx);
    for (uint32_t i = 0; i < ticks.value_count; i++)
        log_debug("tick #%02d: %s", i, &ticks.labels[i * MAX_GLYPHS_PER_TICK]);

    // 2 extension on each side.
    ctx.extensions = 2;
    ticks = dvz_ticks(x0, x1, ctx);
    for (uint32_t i = 0; i < ticks.value_count; i++)
        log_debug(
            "tick #%02d: %s (%f)", i, &ticks.labels[i * MAX_GLYPHS_PER_TICK], ticks.values[i]);

    dvz_ticks_destroy(&ticks);

    return 0;
}



/*************************************************************************************************/
/*  Scene tests                                                                                  */
/*************************************************************************************************/

int test_scene_0(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, CANVAS_FLAGS);
    DvzContext* ctx = gpu->context;
    ASSERT(ctx != NULL);

    DvzScene* scene = dvz_scene(canvas, 1, 1);
    DvzPanel* panel = dvz_scene_panel(scene, 0, 0, DVZ_CONTROLLER_PANZOOM, 0);
    DvzVisual* visual = dvz_scene_visual(panel, DVZ_VISUAL_POINT, 0);

    // Visual data.
    const uint32_t N = 1000;
    dvec3* pos = calloc(N, sizeof(dvec3));
    cvec4* color = calloc(N, sizeof(cvec4));
    float param = 10.0f;
    for (uint32_t i = 0; i < N; i++)
    {
        RANDN_POS(pos[i])
        RAND_COLOR(color[i])
        pos[i][0] *= 10; // NOTE: check automatic data normalization
    }

    dvz_visual_data(visual, DVZ_PROP_POS, 0, N, pos);
    dvz_visual_data(visual, DVZ_PROP_COLOR, 0, N, color);
    dvz_visual_data(visual, DVZ_PROP_MARKER_SIZE, 0, 1, &param);

    dvz_app_run(app, N_FRAMES);
    dvz_visual_destroy(visual);
    dvz_scene_destroy(scene);
    FREE(pos);
    FREE(color);
    TEST_END
}



static void _panzoom(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    DvzScene* scene = ev.user_data;
    ASSERT(scene != NULL);
    DvzGrid* grid = &scene->grid;
    ASSERT(grid != NULL);

    DvzController* controller = NULL;
    DvzInteract* interact = NULL;
    // DvzTransformOLD tr = {0};
    // dvec2 ll = {-1, -1};
    // dvec2 ur = {+1, +1};
    // dvec2 pos_ll = {0};
    // dvec2 pos_ur = {0};


    DvzContainerIterator iter = dvz_container_iterator(&grid->panels);
    DvzPanel* panel = NULL;
    DvzPanel* other = NULL;
    uint32_t i = 0;
    while (iter.item != NULL)
    {
        panel = iter.item;
        controller = panel->controller;
        if (controller == NULL || controller->obj.status == DVZ_OBJECT_STATUS_NONE)
            break;
        if (controller->interacts[0].is_active && controller->type == DVZ_CONTROLLER_PANZOOM)
        {
            // TODO
            // // Get box of active panel.
            // tr = dvz_transform_old(panel, DVZ_CDS_PANZOOM, DVZ_CDS_GPU);
            // dvz_transform_apply(&tr, ll, pos_ll);
            // dvz_transform_apply(&tr, ur, pos_ur);
            // log_debug("(%.3f, %.3f) (%.3f, %.3f)", pos_ll[0], pos_ll[1], pos_ur[0], pos_ur[1]);

            // Set box of other panel.
            other = (DvzPanel*)grid->panels.items[1 - i];
            interact = &other->controller->interacts[0];
            DvzPanzoom* panzoom = &interact->u.p;
            // tr = dvz_transform_inv(tr);
            // panzoom->camera_pos[0] = tr.shift[0];
            // panzoom->camera_pos[1] = tr.shift[1];
            // panzoom->zoom[0] = tr.scale[0];
            // panzoom->zoom[1] = tr.scale[1];

            // View matrix (depends on the pan).
            {
                vec3 center;
                glm_vec3_copy(panzoom->camera_pos, center);
                center[2] = 0.0f; // only the z coord changes between panel and center.
                vec3 lookup = {0, 1, 0};
                glm_lookat(panzoom->camera_pos, center, lookup, interact->mvp.view);
            }
            // Proj matrix (depends on the zoom).
            {
                float zx = panzoom->zoom[0];
                float zy = panzoom->zoom[1];
                glm_ortho(
                    -1.0f / zx, +1.0f / zx, -1.0f / zy, 1.0f / zy, -10.0f, 10.0f,
                    interact->mvp.proj);
            }
        }

        dvz_container_iter(&iter);
        i++;
    }
}

static void _change_pos(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    DvzVisual* visual = ev.user_data;
    ASSERT(visual != NULL);

    const uint32_t N = 1000;
    dvec3* pos = calloc(N, sizeof(dvec3));
    cvec4* color = calloc(N, sizeof(cvec4));
    for (uint32_t i = 0; i < N; i++)
    {
        RANDN_POS(pos[i])
        RAND_COLOR(color[i])
        pos[i][0] *= 10; // NOTE: check automatic data normalization
    }

    dvz_visual_data(visual, DVZ_PROP_POS, 0, N, pos);
    // dvz_visual_data(visual, DVZ_PROP_COLOR, 0, N, color);
    FREE(pos);
    FREE(color);
}

int test_scene_1(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, CANVAS_FLAGS);
    DvzContext* ctx = gpu->context;
    ASSERT(ctx != NULL);

    DvzScene* scene = dvz_scene(canvas, 2, 3);
    DvzPanel* panel = dvz_scene_panel(scene, 0, 0, DVZ_CONTROLLER_PANZOOM, 0);
    DvzVisual* visual = dvz_scene_visual(panel, DVZ_VISUAL_POINT, 0);

    // Visual data.
    const uint32_t N = 1000;
    dvec3* pos = calloc(N, sizeof(dvec3));
    cvec4* color = calloc(N, sizeof(cvec4));
    float param = 10.0f;
    for (uint32_t i = 0; i < N; i++)
    {
        RANDN_POS(pos[i])
        RAND_COLOR(color[i])
        pos[i][0] *= 10; // NOTE: check automatic data normalization
    }

    dvz_visual_data(visual, DVZ_PROP_POS, 0, N, pos);
    dvz_visual_data(visual, DVZ_PROP_COLOR, 0, N, color);
    dvz_visual_data(visual, DVZ_PROP_MARKER_SIZE, 0, 1, &param);

    // Second panel.
    DvzPanel* panel2 = dvz_scene_panel(scene, 1, 1, DVZ_CONTROLLER_PANZOOM, 0);
    dvz_panel_span(panel2, DVZ_GRID_HORIZONTAL, 2);

    DvzVisual* visual2 = dvz_scene_visual(panel2, DVZ_VISUAL_POINT, 0);
    dvz_visual_data(visual2, DVZ_PROP_POS, 0, N, pos);
    dvz_visual_data(visual2, DVZ_PROP_COLOR, 0, N, color);
    dvz_visual_data(visual2, DVZ_PROP_MARKER_SIZE, 0, 1, &param);

    dvz_event_callback(canvas, DVZ_EVENT_TIMER, .5, DVZ_EVENT_MODE_SYNC, _change_pos, visual2);

    dvz_app_run(app, N_FRAMES);
    dvz_visual_destroy(visual);
    dvz_visual_destroy(visual2);
    dvz_scene_destroy(scene);
    FREE(pos);
    FREE(color);
    TEST_END
}



static void _rotate(DvzCanvas* canvas, DvzEvent ev)
{
    DvzPanel* panel = (DvzPanel*)ev.user_data;
    float angle = ev.u.t.time;
    vec3 axis = {0, 1, 0};
    dvz_arcball_rotate(panel, angle, axis);
}

int test_scene_mesh(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);
    DvzContext* ctx = gpu->context;
    ASSERT(ctx != NULL);

    DvzScene* scene = dvz_scene(canvas, 1, 1);
    DvzPanel* panel = dvz_scene_panel(scene, 0, 0, DVZ_CONTROLLER_ARCBALL, 0);
    DvzVisual* visual = dvz_scene_visual(panel, DVZ_VISUAL_MESH, 0);

    uint32_t N = 1000;
    uint32_t nv = 3 * N;
    DvzGraphicsMeshVertex* vertices = calloc(3 * N, sizeof(DvzGraphicsMeshVertex));
    _depth_vertices(N, vertices, true);
    dvz_visual_data_source(visual, DVZ_SOURCE_TYPE_VERTEX, 0, 0, nv, nv, vertices);
    FREE(vertices);

    dvz_visual_texture(visual, DVZ_SOURCE_TYPE_IMAGE, 0, gpu->context->color_texture.texture);

    // dvz_event_callback(canvas, DVZ_EVENT_TIMER, 1. / 60, DVZ_EVENT_MODE_SYNC, _rotate, panel);

    dvz_app_run(app, N_FRAMES);
    dvz_visual_destroy(visual);
    dvz_scene_destroy(scene);
    TEST_END
}



static void _screencast_callback(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    log_debug("screencast frame #%d", ev.u.sc.idx);
    add_frame((Video*)ev.user_data, ev.u.sc.rgba);
    FREE(ev.u.sc.rgba);
}

int test_scene_axes(TestContext* context)
{
    uint32_t width = 1280;
    uint32_t height = 1024;

    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, width, height, CANVAS_FLAGS | DVZ_CANVAS_FLAGS_FPS);
    dvz_canvas_clear_color(canvas, 1, 1, 1);
    DvzContext* ctx = gpu->context;
    ASSERT(ctx != NULL);

    DvzScene* scene = dvz_scene(canvas, 1, 1);
    DvzPanel* panel = dvz_scene_panel(scene, 0, 0, DVZ_CONTROLLER_AXES_2D, 0);

    // Markers.
    DvzVisual* visual = dvz_scene_visual(panel, DVZ_VISUAL_MARKER, 0);
    const uint32_t N = 10000;
    dvec3* pos = calloc(N, sizeof(dvec3));
    cvec4* color = calloc(N, sizeof(cvec4));
    float* ms = calloc(N, sizeof(float));
    for (uint32_t i = 0; i < N; i++)
    {
        RANDN_POS(pos[i])
        RAND_COLOR(color[i])
        dvz_colormap_scale(DVZ_CMAP_VIRIDIS, dvz_rand_float(), 0, 1, color[i]);
        ms[i] = 5 + 50 * dvz_rand_float();

        color[i][3] = 196;
        pos[i][0] += 10;
    }

    dvz_visual_data(visual, DVZ_PROP_POS, 0, N, pos);
    dvz_visual_data(visual, DVZ_PROP_COLOR, 0, N, color);
    dvz_visual_data(visual, DVZ_PROP_MARKER_SIZE, 0, N, ms);

    // Record a video.
    char path[1024];
    snprintf(path, sizeof(path), "%s/scene.mp4", ARTIFACTS_DIR);
    // dvz_canvas_video(canvas, 30, 10000000, path);

    // dvz_axes_flags(panel, 0);

    dvz_app_run(app, N_FRAMES);

    dvz_scene_destroy(scene);
    FREE(pos);
    FREE(color);
    FREE(ms);
    TEST_END
}



static void _logistic(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    DvzCommands* cmds = ev.user_data;
    ASSERT(cmds != NULL);
    dvz_queue_wait(canvas->gpu, DVZ_DEFAULT_QUEUE_RENDER);
    dvz_cmd_submit_sync(cmds, 0);
    dvz_queue_wait(canvas->gpu, DVZ_DEFAULT_QUEUE_COMPUTE);
}

int test_scene_logistic(TestContext* context)
{
    DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
    DvzGpu* gpu = dvz_gpu(app, 0);
    DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, CANVAS_FLAGS);
    dvz_canvas_clear_color(canvas, 1, 1, 1);
    DvzContext* ctx = gpu->context;
    ASSERT(ctx != NULL);

    DvzScene* scene = dvz_scene(canvas, 1, 1);
    DvzPanel* panel = dvz_scene_panel(scene, 0, 0, DVZ_CONTROLLER_PANZOOM, 0);

    // Markers.
    DvzVisual* visual =
        dvz_scene_visual(panel, DVZ_VISUAL_POINT, DVZ_GRAPHICS_FLAGS_DEPTH_TEST_DISABLE);

    const uint32_t N = 1000000;
    dvec3* pos = calloc(N, sizeof(dvec3));
    cvec4* color = calloc(N, sizeof(cvec4));
    for (uint32_t i = 0; i < N; i++)
    {
        pos[i][0] = -1 + 2 * i / (double)(N - 1);
        pos[i][1] = -1 + 2 * dvz_rand_float();
        dvz_colormap_scale(DVZ_CMAP_HSV, i, 0, N, color[i]);
        color[i][3] = 20;
    }

    dvz_visual_data(visual, DVZ_PROP_POS, 0, N, pos);
    dvz_visual_data(visual, DVZ_PROP_COLOR, 0, N, color);
    float param = 2.0f;
    dvz_visual_data(visual, DVZ_PROP_MARKER_SIZE, 0, 1, &param);

    // Create compute object.
    // TODO: make a nicer wrapper for this use case
    char path[1024];
    snprintf(path, sizeof(path), "%s/test_logistic.comp.spv", SPIRV_DIR);
    DvzCompute* compute = dvz_ctx_compute(gpu->context, path);
    dvz_compute_slot(compute, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    dvz_compute_push(compute, 0, sizeof(N), VK_SHADER_STAGE_COMPUTE_BIT);

    DvzBindings bindings = dvz_bindings(&compute->slots, 1);
    DvzSource* source = dvz_source_get(visual, DVZ_SOURCE_TYPE_VERTEX, 0);
    dvz_visual_update(visual, panel->viewport, (DvzDataCoords){0}, NULL);
    dvz_bindings_buffer(&bindings, 0, source->u.br);
    dvz_bindings_update(&bindings);
    dvz_compute_bindings(compute, &bindings);
    dvz_compute_create(compute);

    DvzCommands* cmds = dvz_canvas_commands(canvas, DVZ_DEFAULT_QUEUE_COMPUTE, 1);
    dvz_cmd_begin(cmds, 0);
    dvz_cmd_push(cmds, 0, &compute->slots, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(N), &N);
    dvz_cmd_compute(cmds, 0, compute, (uvec3){N, 1, 1});
    dvz_cmd_end(cmds, 0);
    dvz_event_callback(canvas, DVZ_EVENT_TIMER, 1. / 10, DVZ_EVENT_MODE_SYNC, _logistic, cmds);

    dvz_app_run(app, N_FRAMES);
    dvz_compute_destroy(compute);
    dvz_bindings_destroy(&bindings);
    dvz_scene_destroy(scene);
    TEST_END
}
