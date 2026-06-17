# Coordinate Systems

Understand how datoviz maps your data values to screen positions in both 2D and 3D.

## Overview

Datoviz uses **normalized coordinates** (NDC) by default: the visible range on each axis is `[-1, 1]`. You can override this with `dvz_panel_set_domain()` to map arbitrary data ranges onto the panel. In 3D, datoviz uses a right-handed coordinate system with Y pointing up.

## Example

=== "C"

    ```c
    #include <stdint.h>
    #include <stdlib.h>
    #include "datoviz/scene.h"

    #define N 5

    int main(void) {
        /* data in physical units, e.g. microseconds on X, millivolts on Y */
        float pos[N * 3] = {
            0.0f, -0.8f, 0.0f,
            1.0f,  0.3f, 0.0f,
            2.0f,  1.2f, 0.0f,
            3.0f, -0.5f, 0.0f,
            4.0f,  0.9f, 0.0f,
        };
        uint8_t color[N * 4];
        float size[N];
        for (int i = 0; i < N; i++) {
            color[4*i+0] = 80; color[4*i+1] = 160; color[4*i+2] = 240; color[4*i+3] = 255;
            size[i] = 12.0f;
        }

        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
        DvzPanel* panel = dvz_panel_full(figure);

        /* map data range [0, 4] on X and [-1.5, 1.5] on Y */
        dvz_panel_set_domain(panel, DVZ_DIM_X, 0.0, 4.0);
        dvz_panel_set_domain(panel, DVZ_DIM_Y, -1.5, 1.5);

        /* enable panzoom so the domain is navigable */
        DvzController* controller = dvz_panzoom(scene, NULL);
        dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XY);

        DvzVisual* visual = dvz_point(scene, 0);
        dvz_visual_set_data(visual, "position", pos, N);
        dvz_visual_set_data(visual, "color", color, N);
        dvz_visual_set_data(visual, "size", size, N);

        /* attach the visual using data coordinate space */
        DvzVisualAttachDesc attach = {0};
        attach.coord_space = DVZ_COORD_DATA;
        dvz_panel_add_visual(panel, visual, &attach);

        DvzApp* app = dvz_app(scene);
        dvz_view_glfw(app, figure, 800, 600, "Coordinate systems");
        dvz_app_run(app, 0);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```

<!-- TODO: Python -->

## Step by step

**Set the data domain.** Call `dvz_panel_set_domain(panel, DVZ_DIM_X, min, max)` and the equivalent for `DVZ_DIM_Y` (and `DVZ_DIM_Z` in 3D) to declare what range of physical values the panel should display. Datoviz maps this range linearly onto the NDC `[-1, 1]` range used by the GPU.

**Use data coordinate space when adding the visual.** Fill a `DvzVisualAttachDesc` struct and set `coord_space = DVZ_COORD_DATA` before calling `dvz_panel_add_visual`. This tells the panel to transform the visual's position attribute through the domain mapping before rendering. Without this, positions are interpreted as raw NDC values.

**Bind a controller.** `dvz_panzoom` lets the user explore the data domain interactively. The domain limits set with `dvz_panel_set_domain` define the initial view; panning and zooming adjust the visible sub-range but do not change the underlying domain mapping.

## Common patterns / Variants

**Default NDC (no domain set).** If you skip `dvz_panel_set_domain`, positions must already be in `[-1, 1]`. This is fine for normalized data or when you compute the mapping yourself.

**Equal-aspect 2D.** To prevent distortion when X and Y represent the same physical unit (e.g., spatial coordinates), use `DVZ_PANZOOM_FLAGS_KEEP_ASPECT` when creating the panzoom controller:

```c
DvzPanzoomDesc desc = dvz_panzoom_desc();
desc.controller_flags = DVZ_PANZOOM_FLAGS_KEEP_ASPECT;
DvzController* controller = dvz_panzoom_with_desc(scene, &desc);
```

**3D right-handed convention.** Datoviz uses a right-handed coordinate system: X points right (red), Y points up (green), Z points toward the viewer (blue). Set a camera with `dvz_panel_set_camera()` and bind an orbit or arcball controller to navigate around the scene.

## See also

- [Transforms and scales](transforms-and-scales.md) — non-linear axis mappings (log, datetime, custom)
- [Use panzoom](use-panzoom.md) — interactive 2D navigation
- [3D navigation](3d-navigation.md) — orbit, arcball, and fly controllers
- [Axes](axes.md) — tick marks and labels that follow the domain
