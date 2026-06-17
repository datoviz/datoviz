# Use Sampled Fields

Bind a CPU-side scalar array to a GPU texture so an image or volume visual samples it with a colormap.

## Overview

A sampled field is a scene-owned GPU texture created from a flat float or uint8 array. It decouples the raw data from the colormap: you set a `DvzScale` on the visual's color channel and attach the field, then the GPU maps scalar values to colors at render time. 2D fields back `dvz_image`; 3D fields back `dvz_volume`.

## Example

=== "C"

    ```c
    #include <math.h>
    #include <stdint.h>
    #include "datoviz/scene.h"

    #define W 64u
    #define H 48u

    int main(void) {
        /* fill a scalar field in [0, 1] */
        float values[W * H];
        for (uint32_t y = 0; y < H; y++) {
            for (uint32_t x = 0; x < W; x++) {
                float u = (float)x / (float)(W - 1);
                float v = (float)y / (float)(H - 1);
                values[y * W + x] = 0.5f + 0.5f * sinf(6.28f * (2 * u + v));
            }
        }

        /* scene */
        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
        DvzPanel* panel = dvz_panel_full(figure);

        /* continuous color scale with a custom colormap */
        DvzScale* scale = dvz_scale(scene,
            &(DvzScaleDesc){DVZ_STRUCT_INIT_FIELDS(DvzScaleDesc),
                .kind = DVZ_SCALE_CONTINUOUS});
        dvz_scale_set_domain(scale, 0.0, 1.0);

        DvzColormap* cmap = dvz_colormap(scene, NULL);
        DvzColormapStop stops[3] = {
            {.position = 0.0, .rgba = {0,   0,   128, 255}},
            {.position = 0.5, .rgba = {0,   200, 200, 255}},
            {.position = 1.0, .rgba = {255, 200, 0,   255}},
        };
        dvz_colormap_set_stops(cmap, stops, 3);
        dvz_scale_set_colormap(scale, cmap);

        /* image visual covering the full panel */
        DvzVisual* image = dvz_image(scene, 0);
        vec3 positions[4] = {
            {-1, -1, 0}, {-1, 1, 0}, {1, -1, 0}, {1, 1, 0}};
        vec2 texcoords[4] = {
            {0, 0}, {0, 1}, {1, 0}, {1, 1}};
        dvz_visual_set_data(image, "position",  positions,  4);
        dvz_visual_set_data(image, "texcoords", texcoords,  4);
        /* attach the scale — GPU will map field values to colors */
        dvz_visual_set_scale(image, "color", scale);

        /* 2D sampled field */
        DvzSampledField* field = dvz_sampled_field(scene,
            &(DvzSampledFieldDesc){DVZ_STRUCT_INIT_FIELDS(DvzSampledFieldDesc),
                .dim    = DVZ_FIELD_DIM_2D,
                .format = DVZ_FIELD_FORMAT_R32_FLOAT,
                .semantic = DVZ_FIELD_SEMANTIC_SCALAR,
                .width  = W,
                .height = H,
                .depth  = 1});
        dvz_sampled_field_set_data(field,
            &(DvzFieldDataView){DVZ_STRUCT_INIT_FIELDS(DvzFieldDataView),
                .data          = values,
                .bytes_per_row = W * sizeof(float),
                .rows_per_image = H});
        dvz_visual_set_field(image, "field", field);

        dvz_panel_add_visual(panel, image, NULL);

        DvzApp* app = dvz_app(scene);
        dvz_view_glfw(app, figure, 800, 600, "Sampled field");
        dvz_app_run(app, 0);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```

<!-- TODO: Python -->

## Step by step

**Fill the scalar array.** The field data is a flat row-major array of `float` (or `uint8_t` for compact volumes). Values should lie within the domain you will set on the scale — here `[0, 1]`.

**Create a continuous scale.** `dvz_scale` with `DVZ_SCALE_CONTINUOUS` owns the colormap. Set the scalar domain with `dvz_scale_set_domain`, then attach a `DvzColormap` built from explicit `DvzColormapStop` entries. The GPU uses this mapping at every fragment.

**Create the image visual and attach the scale.** `dvz_image` takes four corner positions and corresponding texture coordinates. Call `dvz_visual_set_scale(image, "color", scale)` to wire the colormap channel — the visual will read the field texture and map each sample through the scale.

**Create and upload the sampled field.** `dvz_sampled_field` allocates a GPU texture. The `DvzSampledFieldDesc` sets the dimensionality (`DVZ_FIELD_DIM_2D`), the texel format (`DVZ_FIELD_FORMAT_R32_FLOAT` for 32-bit float, `DVZ_FIELD_FORMAT_R8_UNORM` for uint8), and the width/height. `dvz_sampled_field_set_data` uploads the array using a `DvzFieldDataView` that describes the row stride in bytes and the number of rows.

**Bind the field to the visual.** `dvz_visual_set_field(image, "field", field)` links the GPU texture to the visual's sampler slot. After this the visual and the field are owned by the scene; do not free them manually.

## Common patterns / Variants

**3D volume field.** Use `dvz_volume` instead of `dvz_image`, set `DVZ_FIELD_DIM_3D` and provide `depth`, and use `DVZ_FIELD_FORMAT_R8_UNORM` with a `uint8_t` array for compact storage. Add alpha stops via `dvz_volume_set_alpha_stops` for transfer-function opacity:

```c
DvzVisual* volume = dvz_volume(scene, 0);
dvz_visual_set_field(volume, "field", field);
dvz_volume_set_opacity(volume, 0.8f);
dvz_volume_set_step_count(volume, 96u);
```

**Updating field data at runtime.** Call `dvz_sampled_field_set_data` again with new values inside a frame callback; the GPU texture is re-uploaded on the next render.

## See also

- [Use colormaps](use-colormaps.md) — building scales and colormaps
- [Coordinate systems](coordinate-systems.md) — normalized vs. data coordinates
- [Render offscreen](render-offscreen.md) — capturing the result to a PNG
