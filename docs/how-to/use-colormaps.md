# Use Colormaps

Map scalar data values to colors using a color scale, and optionally display a colorbar legend.

## Overview

Instead of uploading RGBA colors directly, you can upload scalar float values and let a
`DvzScale` translate them to colors at render time. This keeps the raw data on the GPU and
allows the colormap to be changed without re-uploading per-point color arrays.

## Example

=== "C"

    ```c
    #include <stdint.h>
    #include "datoviz/scene.h"

    #define N 5

    int main(void) {
        /* positions and per-point scalar values in [0, 1] */
        vec3 positions[N] = {
            {-0.6f, -0.2f, 0.0f},
            {-0.3f,  0.2f, 0.0f},
            { 0.0f,  0.0f, 0.0f},
            { 0.3f,  0.2f, 0.0f},
            { 0.6f, -0.2f, 0.0f},
        };
        float values[N] = {0.05f, 0.28f, 0.50f, 0.74f, 0.96f};
        float diameters[N] = {48.0f, 56.0f, 64.0f, 56.0f, 48.0f};

        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
        DvzPanel* panel = dvz_panel_full(figure);

        /* create a colormap from named colors */
        DvzColor stops[] = {
            dvz_color_rgb(20, 30, 80),
            dvz_color_rgb(50, 160, 200),
            dvz_color_rgb(240, 220, 80),
        };
        DvzColormap* colormap =
            dvz_colormap_custom(scene, "my_cmap", stops, DVZ_ARRAY_COUNT(stops));

        /* create a continuous scale linking domain values to the colormap */
        DvzScale* scale = dvz_scale(
            scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
                       .kind = DVZ_SCALE_CONTINUOUS,
                       .label = "value",
                   });
        dvz_scale_set_domain(scale, 0.0, 1.0);
        dvz_scale_set_colormap(scale, colormap);

        /* create the point visual and switch its color attribute to scalar input */
        DvzVisual* visual = dvz_point(scene, 0);
        dvz_visual_set_attr_format(visual, "color", DVZ_VISUAL_ATTR_FORMAT_SCALAR_F32);
        dvz_visual_set_scale(visual, "color", scale);

        /* upload positions, scalar values, and diameters */
        dvz_visual_set_data(visual, "position", positions, N);
        dvz_visual_set_data(visual, "color", values, N);
        dvz_visual_set_data(visual, "diameter", diameters, N);
        dvz_panel_add_visual(panel, visual, NULL);

        /* optional: attach a colorbar legend on the right edge of the panel */
        dvz_colorbar(
            panel, scale,
            &(DvzColorbarDesc){DVZ_STRUCT_INIT_FIELDS(DvzColorbarDesc),
                .orientation = DVZ_COLORBAR_ORIENTATION_VERTICAL,
                .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
                .title = "value",
                .reserve_px = 110.0f,
                .ramp_width_px = 28.0f,
            });

        DvzApp* app = dvz_app(scene);
        dvz_view_glfw(app, figure, 800, 600, "Colormaps");
        dvz_app_run(app, 0);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```

<!-- TODO: Python -->

## Step by step

**Create the colormap.** `dvz_colormap_custom()` takes an array of `DvzColor` stop colors and
registers them under a unique name. The GPU interpolates between stops across the `[0, 1]` range.
You can also use a built-in colormap by name instead of creating a custom one.

**Create the scale.** `dvz_scale()` with `DVZ_SCALE_CONTINUOUS` defines the mapping from your
data domain to the colormap. `dvz_scale_set_domain(scale, vmin, vmax)` sets the data range; values
outside this range are clamped. `dvz_scale_set_colormap()` links the colormap to the scale.

**Switch the visual to scalar input.** By default the `"color"` attribute expects RGBA bytes.
Calling `dvz_visual_set_attr_format(visual, "color", DVZ_VISUAL_ATTR_FORMAT_SCALAR_F32)` switches
it to accept `float` scalars. Then `dvz_visual_set_scale(visual, "color", scale)` wires the
scale so the GPU converts scalars to RGBA at render time.

**Upload scalar values.** Pass `float` data to `dvz_visual_set_data(visual, "color", ...)` just
like any other attribute. The values are interpreted relative to the domain set on the scale.

**Add a colorbar (optional).** `dvz_colorbar()` attaches a legend widget to a panel. It reads
tick positions from the same scale so the legend stays in sync with the data range. The
`reserve_px` field carves out pixel space along the chosen anchor edge so the colorbar does not
overlap the plot area.

## Common patterns

### Update the domain at runtime

```c
/* re-clamp the colormap after the data range changes */
dvz_scale_set_domain(scale, new_min, new_max);
```

### Use a built-in colormap

```c
/* look up a named built-in colormap instead of creating a custom one */
DvzColormap* cmap = dvz_colormap_named(scene, "viridis");
dvz_scale_set_colormap(scale, cmap);
```

### Categorical color scale

```c
/* discrete categories: each integer index maps to one color stop */
DvzScale* cat = dvz_scale(
    scene, &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
               .kind = DVZ_SCALE_CATEGORICAL,
           });
```

## See also

- [Use sampled fields](use-sampled-fields.md) — apply a colormap to a 2D or 3D scalar field image
- [Adornments](adornments.md) — colorbar placement and formatting options
- [Add a visual](add-a-visual.md) — visual creation basics
