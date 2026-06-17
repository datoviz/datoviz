# Scatter with Probe: Image, Colorbar, and Probe Readout

Combine two linked scalar-field images with a shared colorbar and a moveable probe marker that
reads back data values at the cursor position.

## Overview

This walkthrough builds a two-panel figure: a measurement field on the left and a derived field
on the right. Both panels share a single `DvzScale` so their colors map identically. A vertical
colorbar sits beside the right panel, and a crosshair marker follows the cursor to highlight the
probed position.

The key ideas covered here — grid layout, shared scales, sampled-field visuals, colorbars, and
pixel-query probe — each have their own dedicated how-to pages. This walkthrough shows how to
wire them together.

## Example

=== "C"

    ```c
    #include <math.h>
    #include <stdint.h>
    #include <stdlib.h>
    #include "datoviz/scene.h"

    #define W 256u
    #define H 192u

    /* fill a scalar field with a simple analytical pattern */
    static void fill_field(float* out, uint32_t w, uint32_t h)
    {
        for (uint32_t y = 0; y < h; y++)
        {
            for (uint32_t x = 0; x < w; x++)
            {
                float u = (float)x / (float)(w - 1);
                float v = (float)y / (float)(h - 1);
                out[y * w + x] = 0.5f + 0.4f * sinf(6.28f * (u + 0.7f * v));
            }
        }
    }

    int main(void)
    {
        /* scalar fields */
        float measurement[W * H];
        float derived[W * H];
        fill_field(measurement, W, H);
        for (uint32_t i = 0; i < W * H; i++)
            derived[i] = 0.2f + 0.7f * measurement[i];

        /* scene and figure */
        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 1200, 600, 0);

        /* two-column grid */
        DvzGrid* grid = dvz_figure_grid(figure, 1, 2);
        DvzPanel* left = dvz_grid_panel(grid, 0, 0);
        DvzPanel* right = dvz_grid_panel(grid, 0, 1);

        /* set data domain [0,1]×[0,1] on each panel */
        dvz_panel_set_domain(left,  DVZ_DIM_X, 0.0, 1.0);
        dvz_panel_set_domain(left,  DVZ_DIM_Y, 0.0, 1.0);
        dvz_panel_set_domain(right, DVZ_DIM_X, 0.0, 1.0);
        dvz_panel_set_domain(right, DVZ_DIM_Y, 0.0, 1.0);

        /* shared color scale — maps [0,1] through a colormap */
        DvzScale* scale = dvz_scale(
            scene,
            &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
                .kind = DVZ_SCALE_CONTINUOUS,
                .label = "intensity"});
        dvz_scale_set_domain(scale, 0.0, 1.0);
        dvz_scale_set_view_range(scale, 0.0, 1.0);
        DvzColormap* cmap = dvz_colormap_builtin(scene, DVZ_COLORMAP_VIRIDIS);
        dvz_scale_set_colormap(scale, cmap);

        /* helper: create one sampled-field image visual and add it to a panel */
        #define ADD_IMAGE(panel, values)                                             \
            do {                                                                     \
                vec3 dp[4] = {{0,0,0},{0,1,0},{1,0,0},{1,1,0}};                     \
                vec3 vp[4] = {{0}};                                                  \
                dvz_panel_data_to_visual_positions(panel,(const float*)dp,(float*)vp,4); \
                vec2 tc[4] = {{0,0},{0,1},{1,0},{1,1}};                              \
                DvzVisual* img = dvz_image(scene, 0);                                \
                dvz_visual_set_data(img, "position",  vp, 4);                       \
                dvz_visual_set_data(img, "texcoords", tc, 4);                       \
                dvz_visual_set_scale(img, "color", scale);                           \
                DvzSampledField* sf = dvz_sampled_field(                             \
                    scene,                                                           \
                    &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc), \
                        .dim = DVZ_FIELD_DIM_2D,                                     \
                        .format = DVZ_FIELD_FORMAT_R32_FLOAT,                        \
                        .semantic = DVZ_FIELD_SEMANTIC_SCALAR,                       \
                        .width = W, .height = H, .depth = 1});                      \
                dvz_sampled_field_set_data(                                          \
                    sf,                                                              \
                    &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),    \
                        .data = (values),                                            \
                        .bytes_per_row = W * sizeof(float),                          \
                        .rows_per_image = H});                                       \
                dvz_visual_set_field(img, "field", sf);                              \
                dvz_visual_set_depth_test(img, false);                               \
                dvz_panel_add_visual(panel, img, NULL);                              \
            } while (0)

        ADD_IMAGE(left,  measurement);
        ADD_IMAGE(right, derived);

        /* vertical colorbar on the right panel */
        dvz_colorbar(
            right, scale,
            &(DvzColorbarDesc){DVZ_STRUCT_INIT_FIELDS(DvzColorbarDesc),
                .orientation = DVZ_COLORBAR_ORIENTATION_VERTICAL,
                .anchor      = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
                .title       = "intensity",
                .reserve_px  = 110.0f,
                .ramp_width_px = 26.0f,
                .plot_gap_px   = 12.0f});

        /* probe marker on left panel (target shape, positioned at center initially) */
        DvzVisual* probe = dvz_marker(scene, 0);
        vec3 pdata[1] = {{0.5f, 0.5f, 0.0f}};
        vec3 pvis[1]  = {{0}};
        dvz_panel_data_to_visual_positions(left, (const float*)pdata, (float*)pvis, 1);
        float diameter[1] = {30.0f};
        float angle[1]    = {0.0f};
        uint32_t shape[1] = {DVZ_MARKER_SHAPE_TARGET};
        DvzColor color[1] = {{255, 200, 0, 220}};
        dvz_visual_set_data(probe, "position", pvis, 1);
        dvz_visual_set_data(probe, "diameter", diameter, 1);
        dvz_visual_set_data(probe, "angle",    angle,    1);
        dvz_visual_set_data(probe, "shape",    shape,    1);
        dvz_visual_set_data(probe, "color",    color,    1);
        dvz_visual_set_depth_test(probe, false);
        dvz_panel_add_visual(left, probe, NULL);

        /* shared panzoom links the two panels */
        DvzController* pz = dvz_panzoom(scene, NULL);
        dvz_panel_bind_controller(left,  pz, DVZ_DIM_MASK_XY);
        dvz_panel_bind_controller(right, pz, DVZ_DIM_MASK_XY);

        DvzApp* app = dvz_app(scene);
        dvz_view_glfw(app, figure, 1200, 600, "Image + colorbar + probe");
        dvz_app_run(app, 0);

        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```

<!-- TODO: Python -->

## Step by step

**Grid layout.** `dvz_figure_grid(figure, 1, 2)` divides the figure into one row and two columns,
returning a `DvzGrid*`. `dvz_grid_panel(grid, row, col)` retrieves each panel. Each panel gets
its data domain set to `[0, 1] × [0, 1]` so the image fills the full unit square.

**Shared color scale.** A `DvzScale` created with `DVZ_SCALE_CONTINUOUS` maps scalar values in
`[0, 1]` through a colormap. Passing the same `scale` pointer to both image visuals via
`dvz_visual_set_scale(img, "color", scale)` ensures identical color mapping across panels.

**Sampled-field image.** `dvz_sampled_field` uploads a 2-D float array as a GPU texture.
`dvz_image` renders a quad that samples that texture. The `position` attribute holds four corners
in visual space (obtained from `dvz_panel_data_to_visual_positions`) and `texcoords` maps the
texture corners.

**Colorbar.** `dvz_colorbar` attaches a retained colorbar widget to the right panel. Anchoring
it to `DVZ_SCENE_ANCHOR_PANEL_RIGHT` and setting `reserve_px` carves out space for the ramp
and tick labels so they don't overlap the plot.

**Probe marker.** A `dvz_marker` visual with `DVZ_MARKER_SHAPE_TARGET` acts as a crosshair.
Its position is updated by calling `dvz_visual_set_data(probe, "position", ...)` in a pointer
callback (not shown here) using `dvz_panel_data_to_visual_positions` to convert data coordinates
to visual space. See [Input events](input-events.md) for the callback wiring.

**Linked panzoom.** A single panzoom controller bound to both panels with
`dvz_panel_bind_controller` keeps the two images synchronized when the user zooms or pans.

## See also

- [Use sampled fields](use-sampled-fields.md)
- [Use colormaps](use-colormaps.md)
- [Adornments](adornments.md)
- [Create multiple panels](create-multiple-panels.md)
- [Input events](input-events.md)
- [Use panzoom](use-panzoom.md)
