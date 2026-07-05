/*
 * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
 * SPDX-License-Identifier: MIT
 */

/* Minimal scale-bar example with one panzoom panel and no data visual.
 *
 * Run:    just example-c regression/scalebar_minimal
 * Bitmap: just example-c regression/scalebar_minimal bitmap
 * Auto:   just example-c regression/scalebar_minimal auto 120
 * Smoke:  just example-c regression/scalebar_minimal 120
 *         just example-c regression/scalebar_minimal bitmap 120
 */



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include <stdbool.h>

#include "_assertions.h"
#include "datoviz/app.h"
#include "datoviz/scene.h"
#include "example_common.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define WIDTH  720u
#define HEIGHT 420u



/*************************************************************************************************/
/*  Helpers                                                                                      */
/*************************************************************************************************/

typedef struct ScaleBarMinimalState ScaleBarMinimalState;

struct ScaleBarMinimalState
{
    DvzPanzoom* panzoom;
};



/**
 * Apply a small programmatic zoom on every frame when the auto diagnostic mode is enabled.
 *
 * @param win view receiving frame callbacks
 * @param user_data callback state
 */
static void _frame_callback(DvzView* win, void* user_data)
{
    ANN(win);
    ScaleBarMinimalState* state = (ScaleBarMinimalState*)user_data;
    if (state == NULL || state->panzoom == NULL)
        return;

    dvz_panzoom_zoom_wheel(
        state->panzoom, (vec2){0.0f, 1.0f}, (vec2){0.5f * WIDTH, 0.5f * HEIGHT});
    dvz_view_request_frame(win);
}



/**
 * Attach one bottom-left scale bar to an otherwise empty 2D panel.
 *
 * @param panel panel receiving the annotation
 * @param renderer label text renderer
 * @return true on success, false on error
 */
static bool _add_scalebar(DvzPanel* panel, DvzTextRenderer renderer)
{
    ANN(panel);

    int rc = dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 0.010);
    if (rc != 0)
        return false;
    rc = dvz_panel_set_domain(panel, DVZ_DIM_Y, 0.0, 0.006);
    if (rc != 0)
        return false;

    DvzScaleBar* scalebar = dvz_scale_bar(
        panel,
        &(DvzScaleBarDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleBarDesc),
            .dimension = DVZ_DIM_X,
            .anchor = DVZ_SCENE_ANCHOR_BOTTOM_LEFT,
            .label_position = DVZ_SCALEBAR_LABEL_ABOVE,
            .target_length_px = 160.0f,
            .min_length_px = 90.0f,
            .max_length_px = 240.0f,
            .offset_px = {36.0f, 34.0f},
            .tick_length_px = 12.0f,
            .line_width_px = 3.0f,
            .line_color = {245, 248, 252, 255},
            .unit = "m",
            .data_to_unit = 1.0,
            .label_style = {DVZ_STRUCT_INIT_FIELDS(DvzTextStyle),
                .size_px = 22.0f,
                .renderer = renderer,
                .color = {255, 236, 176, 255},
            },
        });
    return scalebar != NULL;
}



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

/**
 * Run a single-panel panzoom window containing only a retained scale bar.
 *
 * @param argc argument count
 * @param argv argument values
 * @return process exit code
 */
int main(int argc, char** argv)
{
    int ret = 1;
    DvzScene* scene = NULL;
    DvzApp* app = NULL;

    scene = dvz_scene();
    EXAMPLE_CHECK(scene != NULL, "dvz_scene() failed");

    DvzFigure* figure = dvz_figure(scene, WIDTH, HEIGHT, 0);
    EXAMPLE_CHECK(figure != NULL, "dvz_figure() failed");

    DvzPanel* panel = dvz_panel_full(figure);
    EXAMPLE_CHECK(panel != NULL, "dvz_panel_full() failed");
    dvz_panel_set_background_color(panel, dvz_color_from_unit(0.040f, 0.050f, 0.060f, 1.0f));

    DvzTextRenderer renderer = example_arg_has(argc, argv, "bitmap")
                                   ? DVZ_TEXT_RENDERER_SMALL_BITMAP_ATLAS
                                   : DVZ_TEXT_RENDERER_MSDF_ATLAS;
    bool ok = _add_scalebar(panel, renderer);
    EXAMPLE_CHECK(ok, "_add_scalebar() failed");

    app = dvz_app(scene);
    EXAMPLE_CHECK(app != NULL, "dvz_app() failed (no GPU or display?)");

    DvzView* win = dvz_view_glfw(app, figure, WIDTH, HEIGHT, "scalebar_minimal");
    EXAMPLE_CHECK(win != NULL, "dvz_view_glfw() failed (GLFW unavailable?)");

    DvzPanzoom* panzoom = dvz_view_panzoom(win, panel, NULL);
    EXAMPLE_CHECK(panzoom != NULL, "failed to create or bind panzoom controller");

    ScaleBarMinimalState state = {.panzoom = panzoom};
    if (example_arg_has(argc, argv, "auto"))
        dvz_view_set_frame_callback(win, _frame_callback, &state);

    dvz_view_request_frame(win);
    dvz_app_run(app, example_frame_count_any(argc, argv));
    ret = 0;

cleanup:
    if (app != NULL)
        dvz_app_destroy(app);
    else if (scene != NULL)
        dvz_scene_destroy(scene);
    return ret;
}
