# Create Multiple Panels

Place two or more independent viewports in a single figure, optionally arranged in a grid or with linked navigation.

## Overview

A figure can contain multiple panels. Each panel has its own viewport, controller, and set of visuals. Panels can be placed manually by fractional coordinates, or arranged automatically with a grid helper. Controllers from separate panels can be linked so that panning or zooming one panel updates the other.

## Example

=== "C"

    ```c
    #include <stdint.h>
    #include <stdlib.h>
    #include "datoviz/scene.h"

    #define N 64

    int main(void) {
        float pos[N * 3] = {0};
        uint8_t color[N * 4];
        float size[N];
        for (int i = 0; i < N; i++) {
            pos[3*i+0] = (float)i / N * 2.0f - 1.0f;
            pos[3*i+1] = 0.0f;
            pos[3*i+2] = 0.0f;
            color[4*i+0] = 80; color[4*i+1] = 180; color[4*i+2] = 240; color[4*i+3] = 220;
            size[i] = 8.0f;
        }

        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 1200, 500, 0);

        /* two panels side by side, placed by fractional [0,1] coordinates */
        DvzPanel* left = dvz_panel(
            figure, (DvzPanelDesc){.x = 0.05f, .y = 0.1f, .width = 0.40f, .height = 0.80f});
        DvzPanel* right = dvz_panel(
            figure, (DvzPanelDesc){.x = 0.55f, .y = 0.1f, .width = 0.40f, .height = 0.80f});

        /* independent panzoom on each panel */
        DvzController* ctrl_left = dvz_panzoom(scene, NULL);
        DvzController* ctrl_right = dvz_panzoom(scene, NULL);
        dvz_panel_bind_controller(left, ctrl_left, DVZ_DIM_MASK_XY);
        dvz_panel_bind_controller(right, ctrl_right, DVZ_DIM_MASK_XY);

        /* add a point visual to each panel */
        DvzVisual* vis_left = dvz_point(scene, 0);
        dvz_visual_set_data(vis_left, "position", pos, N);
        dvz_visual_set_data(vis_left, "color", color, N);
        dvz_visual_set_data(vis_left, "size", size, N);
        dvz_panel_add_visual(left, vis_left, NULL);

        DvzVisual* vis_right = dvz_point(scene, 0);
        dvz_visual_set_data(vis_right, "position", pos, N);
        dvz_visual_set_data(vis_right, "color", color, N);
        dvz_visual_set_data(vis_right, "size", size, N);
        dvz_panel_add_visual(right, vis_right, NULL);

        DvzApp* app = dvz_app(scene);
        dvz_view_glfw(app, figure, 1200, 500, "Multiple panels");
        dvz_app_run(app, 0);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```

<!-- TODO: Python -->

## Step by step

`dvz_panel()` places a panel inside a figure using fractional coordinates in `[0, 1]`. The `DvzPanelDesc` struct specifies the top-left corner (`.x`, `.y`) and size (`.width`, `.height`) as fractions of the figure's pixel dimensions. In the example, two panels fill the left and right halves with a small margin.

Each panel gets its own controller via `dvz_panzoom(scene, NULL)` and `dvz_panel_bind_controller`. The `DVZ_DIM_MASK_XY` flag enables pan and zoom on both axes. Because each panel owns a separate controller, dragging in one panel does not affect the other.

Visuals are created with `dvz_point` and uploaded with `dvz_visual_set_data`, then attached to their respective panel with `dvz_panel_add_visual`. Each panel can hold any number of visuals independently.

## Common patterns / Variants

**Grid layout** — use `dvz_figure_grid` to place panels in a regular grid instead of positioning them manually:

```c
DvzGrid* grid = dvz_figure_grid(figure, 2, 2);
dvz_grid_set_margins(grid, &(DvzPanelReserve){
    .left_px = 80.0f, .right_px = 80.0f,
    .top_px = 60.0f, .bottom_px = 60.0f});
dvz_grid_set_gutter(grid, 30.0f, 30.0f);
DvzPanel* p00 = dvz_grid_panel(grid, 0, 0);
DvzPanel* p01 = dvz_grid_panel(grid, 0, 1);
DvzPanel* p10 = dvz_grid_panel(grid, 1, 0);
DvzPanel* p11 = dvz_grid_panel(grid, 1, 1);
```

**Linked panels** — synchronize the X axis of two panels so panning one pans the other:

```c
DvzController* ctrl_a = dvz_panzoom(scene, NULL);
DvzController* ctrl_b = dvz_panzoom(scene, NULL);
/* bidirectional link on X extent */
dvz_controller_link(scene, ctrl_a, ctrl_b,
    DVZ_CONTROLLER_LINK_EXTENT_X, DVZ_CONTROLLER_LINK_ONE_WAY);
dvz_controller_link(scene, ctrl_b, ctrl_a,
    DVZ_CONTROLLER_LINK_EXTENT_X, DVZ_CONTROLLER_LINK_ONE_WAY);
dvz_panel_bind_controller(panel_a, ctrl_a, DVZ_DIM_MASK_X);
dvz_panel_bind_controller(panel_b, ctrl_b, DVZ_DIM_MASK_X);
```

## See also

- [Create a scene](create-a-scene.md) — scene / figure / panel hierarchy
- [Use panzoom](use-panzoom.md) — controller options
- [Axes](axes.md) — adding axis decorations to panels
