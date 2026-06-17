# Add a Visual

Choose a visual type, upload data to its named attributes, and attach it to a panel.

## Overview

A visual is a GPU-accelerated renderer for one type of primitive (points, markers, lines, meshes, etc.). Each visual exposes named attributes — `"position"`, `"color"`, `"size"`, etc. — that accept arrays of a specific type and count. Once attached to a panel, the visual is rendered every frame.

## Example

=== "C"

    ```c
    #include <stdint.h>
    #include <stdlib.h>
    #include "datoviz/scene.h"

    #define N 1000

    int main(void) {
        /* data */
        float pos[N * 3];
        uint8_t color[N * 4];
        float size[N];
        for (int i = 0; i < N; i++) {
            pos[3*i+0] = (float)rand()/RAND_MAX * 2 - 1;
            pos[3*i+1] = (float)rand()/RAND_MAX * 2 - 1;
            pos[3*i+2] = 0.0f;
            color[4*i+0] = rand() % 256;
            color[4*i+1] = rand() % 256;
            color[4*i+2] = rand() % 256;
            color[4*i+3] = 255;
            size[i] = 8.0f;
        }

        /* scene */
        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
        DvzPanel* panel = dvz_panel_full(figure);

        /* create a point visual and upload data */
        DvzVisual* visual = dvz_point(scene, 0);
        dvz_visual_set_data(visual, "position", pos, N);
        dvz_visual_set_data(visual, "color", color, N);
        dvz_visual_set_data(visual, "size", size, N);

        /* attach the visual to the panel */
        dvz_panel_add_visual(panel, visual, NULL);

        DvzApp* app = dvz_app(scene);
        dvz_view_glfw(app, figure, 800, 600, "Add a visual");
        dvz_app_run(app, 0);

        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```

<!-- TODO: Python -->

## Step by step

**Create the visual.** Call a visual constructor such as `dvz_point(scene, 0)`, passing the scene and flags (0 for defaults). The visual is allocated on the GPU but holds no data yet.

**Upload named attributes.** `dvz_visual_set_data(visual, name, data, count)` uploads a CPU array to the named GPU attribute. `"position"` expects a flat `float[N*3]` array in normalized coordinates `[-1, 1]`. `"color"` expects `uint8_t[N*4]` RGBA in `[0, 255]`. `"size"` expects `float[N]` pixel diameters. Every attribute must receive the same item count `N`.

**Attach to a panel.** `dvz_panel_add_visual(panel, visual, NULL)` registers the visual with the panel. After this call the visual is drawn on every frame. The third argument is an optional transform override; pass `NULL` to use the panel's default transform.

## Common patterns

Upload multiple attributes in one call to avoid redundant GPU synchronization:

```c
DvzVisualDataUpdate updates[] = {
    {"position", pos, N},
    {"color",    color, N},
    {"size",     size, N},
};
dvz_visual_set_data_many(visual, updates, 3);
```

To use a marker visual instead of plain points, replace `dvz_point` with `dvz_marker` and supply a `"symbol"` attribute (`uint32_t[N]`) with values from `DVZ_SYMBOL_*`:

```c
DvzVisual* visual = dvz_marker(scene, 0);
uint32_t symbols[N];
for (int i = 0; i < N; i++) symbols[i] = DVZ_SYMBOL_DISC;
dvz_visual_set_data(visual, "symbol", symbols, N);
```

## See also

- [Create a scene](create-a-scene.md) — set up the scene, figure, and panel hierarchy
- [Update visual data](update-visual-data.md) — change attribute data after the first upload
- [Choose a visual family](choose-a-visual-family.md) — survey of all available visual types
