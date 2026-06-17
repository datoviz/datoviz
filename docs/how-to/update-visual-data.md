# Update Visual Data

How to replace or partially update a visual's attribute arrays after the initial upload.

## Overview

Datoviz uses a retained-mode GPU model: data lives on the GPU and you push updates to it
explicitly. You can replace an entire attribute with `dvz_visual_set_data`, update a contiguous
subrange with `dvz_visual_set_data_range`, or batch multiple attribute uploads in one call with
`dvz_visual_set_data_many`. Visibility can be toggled independently of data.

## Example

=== "C"

    ```c
    #include <stdbool.h>
    #include <stdint.h>
    #include "datoviz/scene.h"

    #define N 5

    /* initial and updated positions */
    static const float pos_initial[N * 3] = {
        -0.8f, 0.0f, 0.0f,
        -0.4f, 0.0f, 0.0f,
         0.0f, 0.0f, 0.0f,
         0.4f, 0.0f, 0.0f,
         0.8f, 0.0f, 0.0f,
    };
    static const float pos_updated[N * 3] = {
        -0.8f, -0.4f, 0.0f,
        -0.4f,  0.4f, 0.0f,
         0.0f, -0.4f, 0.0f,
         0.4f,  0.4f, 0.0f,
         0.8f, -0.4f, 0.0f,
    };
    static const float sizes[N] = {20.0f, 20.0f, 20.0f, 20.0f, 20.0f};
    static const uint8_t colors[N * 4] = {
        255, 100,  50, 255,
        255, 100,  50, 255,
        255, 100,  50, 255,
        255, 100,  50, 255,
        255, 100,  50, 255,
    };

    int main(void) {
        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
        DvzPanel* panel = dvz_panel_full(figure);

        DvzVisual* visual = dvz_point(scene, 0);

        /* upload all attributes at once */
        DvzVisualDataUpdate updates[] = {
            {.attr_name = "position", .data = pos_initial, .item_count = N},
            {.attr_name = "color",    .data = colors,      .item_count = N},
            {.attr_name = "diameter", .data = sizes,       .item_count = N},
        };
        dvz_visual_set_data_many(visual, updates, 3);

        dvz_panel_add_visual(panel, visual, NULL);

        DvzApp* app = dvz_app(scene);
        dvz_view_glfw(app, figure, 800, 600, "Update visual data");

        /* replace all positions after one second */
        /* call this from a timer or frame callback in a real app */
        dvz_visual_set_data(visual, "position", pos_updated, N);

        /* update only the middle point (index 2, count 1) */
        static const float mid[3] = {0.0f, 0.6f, 0.0f};
        dvz_visual_set_data_range(visual, "position", mid, 1, 2);

        dvz_app_run(app, 0);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```

<!-- TODO: Python -->

## Step by step

**Batch upload with `dvz_visual_set_data_many`.** Fill an array of `DvzVisualDataUpdate` structs,
one per attribute, each naming the attribute (`attr_name`), pointing to the data buffer (`data`),
and giving the item count. Pass the array and its length to `dvz_visual_set_data_many`. This is
more efficient than calling `dvz_visual_set_data` in a loop because it issues a single GPU upload.

**Full replacement with `dvz_visual_set_data`.** Call `dvz_visual_set_data(visual, "attr_name",
data_ptr, N)` at any time after the initial upload. The item count can differ from the original;
the GPU buffer is resized if needed.

**Partial update with `dvz_visual_set_data_range`.** When only a contiguous slice of items
changes, use `dvz_visual_set_data_range(visual, "attr_name", data_ptr, item_count, first_item)`.
This transfers only the changed region to the GPU, which is faster for large visuals with small
hotspots.

**Visibility toggle.** Call `dvz_visual_set_visible(visual, false)` to hide a visual without
destroying or re-uploading it. Call `dvz_visual_set_visible(visual, true)` to show it again. This
is cheaper than setting all alpha values to zero.

## Common patterns

Hide a visual temporarily:

```c
dvz_visual_set_visible(visual, false);
/* ... later ... */
dvz_visual_set_visible(visual, true);
```

Update a single item in a large point cloud (e.g. index 42):

```c
float new_pos[3] = {0.5f, -0.3f, 0.0f};
dvz_visual_set_data_range(visual, "position", new_pos, 1, 42);
```

## See also

- [Add a visual](add-a-visual.md)
- [Create a scene](create-a-scene.md)
- [Animation](../how-to/animation.md)
