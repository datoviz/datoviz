# Use Panzoom

Enable interactive pan and zoom on a 2D panel using mouse drag and scroll.

## Overview

Panzoom is a 2D controller that maps mouse drag to translation and scroll wheel to zoom. It
operates in normalized device coordinates by default, but can be combined with
`dvz_panel_set_domain` to work in data coordinates. Each panel gets its own controller instance;
multiple panels can share a panzoom by binding the same controller to each.

## Example

=== "C"

    ```c
    #include <stdint.h>
    #include <stdlib.h>
    #include "datoviz/scene.h"

    #define N 500

    int main(void) {
        /* scene and panel */
        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
        DvzPanel* panel = dvz_panel_full(figure);

        /* set data-space domain */
        dvz_panel_set_domain(panel, DVZ_DIM_X, -1.0, 1.0);
        dvz_panel_set_domain(panel, DVZ_DIM_Y, -1.0, 1.0);

        /* attach panzoom controller */
        DvzController* controller = dvz_panzoom(scene, NULL);
        dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XY);

        /* point visual */
        float pos[N * 3];
        uint8_t color[N * 4];
        float size[N];
        for (int i = 0; i < N; i++) {
            pos[3*i+0] = (float)rand() / RAND_MAX * 2.0f - 1.0f;
            pos[3*i+1] = (float)rand() / RAND_MAX * 2.0f - 1.0f;
            pos[3*i+2] = 0.0f;
            color[4*i+0] = rand() % 256;
            color[4*i+1] = rand() % 256;
            color[4*i+2] = rand() % 256;
            color[4*i+3] = 200;
            size[i] = 8.0f;
        }
        DvzVisual* visual = dvz_point(scene, 0);
        dvz_visual_set_data(visual, "position", pos, N);
        dvz_visual_set_data(visual, "color", color, N);
        dvz_visual_set_data(visual, "size", size, N);
        dvz_panel_add_visual(panel, visual, NULL);

        /* run */
        DvzApp* app = dvz_app(scene);
        dvz_view_glfw(app, figure, 800, 600, "Panzoom");
        dvz_app_run(app, 0);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```

<!-- TODO: Python -->

## Step by step

Call `dvz_panel_set_domain` on both axes before attaching the controller. This defines the
data-space extent the panel covers at rest; the panzoom controller will map user interactions to
translations and scales within that space.

Create the controller with `dvz_panzoom(scene, NULL)`. The second argument accepts an optional
`DvzPanzoomDesc` for advanced options such as `DVZ_PANZOOM_FLAGS_KEEP_ASPECT`, which locks the
aspect ratio so X and Y zoom identically.

Bind the controller to the panel with `dvz_panel_bind_controller`, passing a dimension mask. Use
`DVZ_DIM_MASK_XY` for free 2D navigation, `DVZ_DIM_MASK_X` to restrict movement to the
horizontal axis only, or `DVZ_DIM_MASK_Y` for vertical only.

Add visuals to the panel normally after the controller is bound. Their positions are interpreted
in the data coordinate space established by `dvz_panel_set_domain`.

## Common patterns / Variants

**Locked aspect ratio** — keep X and Y scales equal (useful for geographic or scientific data):

```c
DvzPanzoomDesc desc = dvz_panzoom_desc();
desc.controller_flags = DVZ_PANZOOM_FLAGS_KEEP_ASPECT;
DvzController* controller = dvz_panzoom_ex(scene, &desc);
dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XY);
```

**X-axis only** — lock vertical position, zoom and pan horizontally only:

```c
dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_X);
```

**Shared controller across panels** — bind the same controller to two panels so they pan and
zoom in sync:

```c
dvz_panel_bind_controller(panel_a, controller, DVZ_DIM_MASK_XY);
dvz_panel_bind_controller(panel_b, controller, DVZ_DIM_MASK_XY);
```

## See also

- [Create a scene](create-a-scene.md) — scene, figure, and panel hierarchy
- [3D navigation](3d-navigation.md) — arcball, turntable, and fly controllers for 3D
- [Create multiple panels](create-multiple-panels.md) — linking panels with a shared controller
