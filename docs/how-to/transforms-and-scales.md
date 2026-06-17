# Transforms and Scales

Apply a per-visual affine transform or set a panel's data-coordinate domain and display scale.

## Overview

Datoviz provides three independent ways to transform how data appears: a per-visual mat4 transform
applied on the GPU before rendering, a panel domain that maps data coordinates to normalized
viewport coordinates, and a per-view user-scale factor for DPI-aware display scaling.

## Example

=== "C"

    ```c
    #include <string.h>
    #include <stdint.h>
    #include "datoviz/scene.h"

    #define N 5

    int main(void) {
        /* positions in normalized [-1, 1] space */
        float pos[N * 3] = {
            -0.4f, -0.2f, 0,
            -0.2f,  0.2f, 0,
             0.0f, -0.1f, 0,
             0.2f,  0.3f, 0,
             0.4f, -0.2f, 0,
        };
        uint8_t color[N * 4];
        float size[N];
        for (int i = 0; i < N; i++) {
            color[4*i+0] = 100; color[4*i+1] = 180; color[4*i+2] = 240; color[4*i+3] = 255;
            size[i] = 20.0f;
        }

        DvzScene* scene = dvz_scene();
        DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
        DvzPanel* panel = dvz_panel_full(figure);
        DvzController* controller = dvz_panzoom(scene, NULL);
        dvz_panel_bind_controller(panel, controller, DVZ_DIM_MASK_XY);

        DvzVisual* visual = dvz_point(scene, 0);
        dvz_visual_set_data(visual, "position", pos, N);
        dvz_visual_set_data(visual, "color", color, N);
        dvz_visual_set_data(visual, "size", size, N);

        /* apply a 2D affine transform: translate by (0.2, 0.1) and shear slightly */
        mat4 transform = {
            {1.2f,  0.2f, 0.0f, 0.0f},
            {-0.1f, 0.9f, 0.0f, 0.0f},
            {0.0f,  0.0f, 1.0f, 0.0f},
            {0.2f,  0.1f, 0.0f, 1.0f},
        };
        dvz_visual_set_transform(visual, transform);

        dvz_panel_add_visual(panel, visual, NULL);

        DvzApp* app = dvz_app(scene);
        dvz_view_glfw(app, figure, 800, 600, "Visual transform");
        dvz_app_run(app, 0);
        dvz_app_destroy(app);
        dvz_scene_destroy(scene);
        return 0;
    }
    ```

<!-- TODO: Python -->

## Step by step

The scene, figure, panel, and visual are created with the standard pattern from the quickstart. The
visual receives five points with positions, colors, and sizes uploaded via `dvz_visual_set_data`.

The transform is a column-major 4×4 float matrix (`mat4`). The top-left 3×3 block encodes rotation
and scale; the bottom row encodes translation (x, y, z in the last column). Here the matrix
applies a mild shear and a (0.2, 0.1) translation so the transformed visual is visually distinct
from an untransformed copy.

`dvz_visual_set_transform` uploads the matrix to the GPU. The transform is applied per-vertex
before the panel's own coordinate mapping, so it operates in the same normalized space as the
visual's position data. To remove the transform later, call `dvz_visual_clear_transform`.

## Common patterns

**Check and retrieve the current transform:**

```c
if (dvz_visual_has_transform(visual)) {
    mat4 current;
    dvz_visual_get_transform(visual, current);
}
```

**Set a panel's data-coordinate domain** to map physical units to the normalized viewport:

```c
/* data runs from -3.0 to 3.0 on X and -1.5 to 1.5 on Y */
dvz_panel_set_domain(panel, DVZ_DIM_X, -3.0, 3.0);
dvz_panel_set_domain(panel, DVZ_DIM_Y, -1.5, 1.5);
```

**Set a per-view user scale** (DPI multiplier, typically 1.0–2.0):

```c
dvz_view_set_user_scale(view, 1.5f);
```

## See also

- [Coordinate systems](coordinate-systems.md) — how datoviz coordinate spaces relate to each other
- [Create a scene](create-a-scene.md) — scene/figure/panel setup
- [Add a visual](add-a-visual.md) — visual creation and data upload
