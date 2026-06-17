# Adornments

Add overlay annotations — colorbars, scalebars, and legends — to a panel.

## Overview

Adornments are retained overlay elements attached to a panel. They are created once and update
automatically as the panel's scale or domain changes. The three main kinds are: continuous
colorbars, physical scalebars, and categorical legends.

## Example

=== "C"

    ```c
    #include <stdint.h>
    #include <stdlib.h>
    #include "datoviz/scene.h"

    #define N 500

    int main(void) {
        /* data: points with scalar values in [0, 1] */
        float pos[N * 3];
        float values[N];
        for (int i = 0; i < N; i++) {
            pos[3*i+0] = (float)rand() / RAND_MAX * 2.0f - 1.0f;
            pos[3*i+1] = (float)rand() / RAND_MAX * 2.0f - 1.0f;
            pos[3*i+2] = 0.0f;
            values[i]  = (float)i / (float)(N - 1);
        }

        /* scene */
        DvzScene*  scene  = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
        DvzPanel*  panel  = dvz_panel_full(figure);

        /* continuous color scale mapped to a built-in colormap */
        DvzScale* scale = dvz_scale(
            scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
                       .kind  = DVZ_SCALE_CONTINUOUS,
                       .label = "value",
                   });
        dvz_scale_set_domain(scale, 0.0, 1.0);
        dvz_scale_set_view_range(scale, 0.0, 1.0);

        /* colormap: use the built-in "viridis" colormap */
        DvzColormap* colormap = dvz_colormap_builtin(scene, DVZ_COLORMAP_VIRIDIS);
        dvz_scale_set_colormap(scale, colormap);

        /* point visual colored by the scale */
        DvzVisual* visual = dvz_point(scene, 0);
        dvz_visual_set_data(visual, "position", pos, N);
        dvz_visual_set_data(visual, "value", values, N);
        dvz_visual_set_scale(visual, "color", scale);
        dvz_panel_add_visual(panel, visual, NULL);

        /* colorbar anchored to the right edge of the panel */
        dvz_colorbar(
            panel, scale,
            &(DvzColorbarDesc){DVZ_STRUCT_INIT_FIELDS(DvzColorbarDesc),
                .orientation = DVZ_COLORBAR_ORIENTATION_VERTICAL,
                .anchor      = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
                .title       = "value",
                .reserve_px  = 100.0f,
                .ramp_width_px = 24.0f,
                .plot_gap_px   = 12.0f,
                .tick_length_px = 5.0f,
                .label_gap_px   = 6.0f,
            });

        DvzApp* app = dvz_app(scene);
        dvz_view_glfw(app, figure, 800, 600, "Colorbar");
        dvz_app_run(app, 0);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```

<!-- TODO: Python -->

## Step by step

**Scale.** `dvz_scale()` creates a retained scale object that links a data range to visual
properties. For a continuous colorbar pass `DVZ_SCALE_CONTINUOUS`; set the domain (data min/max)
with `dvz_scale_set_domain` and the currently visible range with `dvz_scale_set_view_range`.

**Colormap.** `dvz_colormap_builtin()` returns one of the built-in palettes (e.g.
`DVZ_COLORMAP_VIRIDIS`, `DVZ_COLORMAP_PLASMA`, `DVZ_COLORMAP_INFERNO`). Attach it to the scale
with `dvz_scale_set_colormap`.

**Binding the scale to a visual.** `dvz_visual_set_scale(visual, "color", scale)` tells the
point visual to derive its per-point color from the `"value"` data attribute through the scale,
instead of using a raw RGBA array.

**Colorbar.** `dvz_colorbar()` takes the panel and scale and a `DvzColorbarDesc` descriptor.
`anchor` controls which panel edge it attaches to; `reserve_px` reserves that many pixels of
panel space so the plot area shrinks to make room.

## Common patterns / Variants

**Scalebar** — a physical length indicator that updates as the user pans/zooms:

```c
/* attach a metric length unit to the panel */
DvzUnits* units = dvz_units_builtin(scene, DVZ_UNIT_LADDER_METRIC_LENGTH, 0.001);
DvzAnnotation* sb = dvz_annotation_scalebar(
    panel,
    &(DvzScaleBarDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleBarDesc),
        .dimension       = DVZ_DIM_X,
        .anchor          = DVZ_SCENE_ANCHOR_BOTTOM_LEFT,
        .target_length_px = 200.0f,
        .min_length_px    = 140.0f,
        .max_length_px    = 280.0f,
        .offset_px        = {60.0f, 70.0f},
        .tick_length_px   = 16.0f,
        .line_width_px    = 3.0f,
    });
dvz_scalebar_set_units((DvzScaleBar*)sb, units);
```

**Categorical legend** — for discrete groups mapped to colors and marker shapes:

```c
/* categorical scale */
DvzScale* cat_scale = dvz_scale(
    scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
               .kind  = DVZ_SCALE_CATEGORICAL,
               .label = "group",
           });
DvzScaleCategory categories[3] = {
    {.category_id = 1, .order = 0, .label = "A", .color = {220, 80,  80,  255}},
    {.category_id = 2, .order = 1, .label = "B", .color = {80,  180, 80,  255}},
    {.category_id = 3, .order = 2, .label = "C", .color = {80,  120, 220, 255}},
};
dvz_scale_set_categories(cat_scale, categories, 3);

/* legend attached to the right edge */
DvzLegendDesc ldesc = dvz_legend_desc();
ldesc.anchor       = DVZ_SCENE_ANCHOR_PANEL_RIGHT;
ldesc.reserve_px   = 180.0f;
ldesc.title        = "group";
dvz_legend(panel, cat_scale, &ldesc);
```

## See also

- [Use colormaps](use-colormaps.md) — colormap selection and custom palettes
- [Axes](axes.md) — axis tick marks and labels
- [Coordinate systems](coordinate-systems.md) — data domain and panel extents
