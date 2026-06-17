# Multi-Panel Figure

Build a scientific figure with multiple panels arranged in a grid, each with independent axes and a shared linked X controller.

## Overview

Datoviz panels live inside a figure and are positioned with a grid layout. Each panel gets its own
panzoom controller for the Y axis, while a controller link synchronises the X axis across all
panels so that panning or zooming in one row is reflected in every other row.

## Example

=== "C"

    ```c
    #include <math.h>
    #include <stdint.h>
    #include <stdlib.h>
    #include "datoviz/scene.h"

    #define N 128

    static const float TAU = 6.28318530718f;

    int main(void) {
        /* scene and figure */
        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 1200, 800, 0);

        /* two-row grid layout */
        DvzGrid* grid = dvz_figure_grid(figure, 2, 1);
        dvz_grid_set_margins(
            grid, &(DvzPanelReserve){
                      .left_px = 80.0f, .right_px = 40.0f,
                      .top_px  = 60.0f, .bottom_px = 60.0f});
        dvz_grid_set_gutter(grid, 0.0f, 30.0f);

        DvzPanel* top    = dvz_grid_panel(grid, 0, 0);
        DvzPanel* bottom = dvz_grid_panel(grid, 1, 0);

        /* shared X domain [0, 10], independent Y domains */
        dvz_panel_set_domain(top,    DVZ_DIM_X, 0.0, 10.0);
        dvz_panel_set_domain(top,    DVZ_DIM_Y, -1.2, 1.2);
        dvz_panel_set_domain(bottom, DVZ_DIM_X, 0.0, 10.0);
        dvz_panel_set_domain(bottom, DVZ_DIM_Y, -2.0, 2.0);

        /* fill top panel with a sine path */
        vec3 top_pos[N], top_vis[N];
        DvzColor top_col[N];
        float top_wid[N];
        for (int i = 0; i < N; i++) {
            float t = (float)i / (N - 1);
            top_pos[i][0] = 10.0f * t;
            top_pos[i][1] = sinf(TAU * 2.0f * t);
            top_pos[i][2] = 0.0f;
            top_col[i]    = dvz_color_rgba(70, 200, 230, 240);
            top_wid[i]    = 2.5f;
        }
        dvz_panel_data_to_visual_positions(top, (float*)top_pos, (float*)top_vis, N);
        DvzVisual* path_top = dvz_path(scene, 0);
        DvzVisualDataUpdate upd_top[] = {
            {"position",     top_vis, N},
            {"color",        top_col, N},
            {"stroke_width", top_wid, N},
        };
        dvz_visual_set_data_many(path_top, upd_top, 3);
        dvz_panel_add_visual(top, path_top, NULL);

        /* fill bottom panel with a cosine path */
        vec3 bot_pos[N], bot_vis[N];
        DvzColor bot_col[N];
        float bot_wid[N];
        for (int i = 0; i < N; i++) {
            float t = (float)i / (N - 1);
            bot_pos[i][0] = 10.0f * t;
            bot_pos[i][1] = 1.6f * cosf(TAU * t + 0.4f);
            bot_pos[i][2] = 0.0f;
            bot_col[i]    = dvz_color_rgba(230, 140, 70, 240);
            bot_wid[i]    = 2.5f;
        }
        dvz_panel_data_to_visual_positions(bottom, (float*)bot_pos, (float*)bot_vis, N);
        DvzVisual* path_bot = dvz_path(scene, 0);
        DvzVisualDataUpdate upd_bot[] = {
            {"position",     bot_vis, N},
            {"color",        bot_col, N},
            {"stroke_width", bot_wid, N},
        };
        dvz_visual_set_data_many(path_bot, upd_bot, 3);
        dvz_panel_add_visual(bottom, path_bot, NULL);

        /* independent panzoom controllers for each axis of each panel */
        DvzController* top_x    = dvz_panzoom(scene, NULL);
        DvzController* top_y    = dvz_panzoom(scene, NULL);
        DvzController* bottom_x = dvz_panzoom(scene, NULL);
        DvzController* bottom_y = dvz_panzoom(scene, NULL);

        /* link X axes bidirectionally so panning one row pans both */
        dvz_controller_link(
            scene, top_x, bottom_x,
            DVZ_CONTROLLER_LINK_EXTENT_X, DVZ_CONTROLLER_LINK_ONE_WAY);
        dvz_controller_link(
            scene, bottom_x, top_x,
            DVZ_CONTROLLER_LINK_EXTENT_X, DVZ_CONTROLLER_LINK_ONE_WAY);

        /* bind controllers to panels */
        dvz_panel_bind_controller(top,    top_x,    DVZ_DIM_MASK_X);
        dvz_panel_bind_controller(top,    top_y,    DVZ_DIM_MASK_Y);
        dvz_panel_bind_controller(bottom, bottom_x, DVZ_DIM_MASK_X);
        dvz_panel_bind_controller(bottom, bottom_y, DVZ_DIM_MASK_Y);

        /* run */
        DvzApp* app = dvz_app(scene);
        DvzView* view = dvz_view_glfw(app, figure, "Multi-panel figure", 0);
        dvz_app_run(app, 0);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```

<!-- TODO: Python -->

## Step by step

Create the scene, figure, and a 2×1 grid with `dvz_figure_grid`. The grid manages panel placement;
`dvz_grid_set_margins` reserves space for axes labels and `dvz_grid_set_gutter` adds a gap between
rows.

Call `dvz_panel_set_domain` on each panel to establish data-space coordinates. Panels share the
same X domain (`[0, 10]`) but use different Y ranges. This is what allows a single linked
controller to drive the X axis while each panel retains an independent Y scale.

Convert data positions to visual (NDC) positions with `dvz_panel_data_to_visual_positions` before
uploading them to the path visual. This step maps your data coordinates through the panel's domain
into the `[-1, 1]` space the GPU shader expects.

Allocate four separate `dvz_panzoom` controllers — one per axis per panel. Linking only the X
controllers (`DVZ_CONTROLLER_LINK_EXTENT_X`) with two one-way links creates bidirectional
synchronisation: panning or zooming in either panel updates the other's X view while Y remains
independent.

Bind each controller to its panel with `dvz_panel_bind_controller`, restricting it to the
appropriate dimension mask (`DVZ_DIM_MASK_X` or `DVZ_DIM_MASK_Y`). Finally, open an interactive
GLFW window with `dvz_view_glfw` and enter the event loop.

## Common patterns / Variants

**Three or more rows** — extend the grid to `dvz_figure_grid(figure, 3, 1)` and link all X
controllers in a chain; each new panel needs its own X and Y controller.

**Column layout** — use `dvz_figure_grid(figure, 1, N)` and link Y controllers instead of X.

**Unlinked panels** — omit `dvz_controller_link` entirely; each panel pans and zooms
independently.

## See also

- [Create multiple panels](create-multiple-panels.md) — grid layout and manual panel placement
- [Use panzoom](use-panzoom.md) — controller options, dimension masking, aspect lock
- [Axes](axes.md) — adding axis ticks and labels to each panel
