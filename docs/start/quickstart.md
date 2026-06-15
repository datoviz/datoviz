# Quickstart: Rendering in 10 Minutes

**Prerequisites:** Datoviz installed — see [Install](install.md).

This page renders 10,000 random 3D points as a scatter plot in an interactive window with
pan-and-zoom navigation. No external data files. Both a C version and a Python ctypes version
are shown; they call the same underlying API.


## The example

A scene holds one figure. The figure has one full-size panel. A `point` visual receives three
data arrays — positions, colors, and diameters — and the panel gets a panzoom controller so the
user can explore the data with the mouse.


## C version

Build and run:

```sh
just example-c visuals/point
./build/examples/c/visuals/point --live
```

For a self-contained program without the scenario runner, the structure is:

```c
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "datoviz/app.h"
#include "datoviz/scene.h"

#define N 10000

int main(void)
{
    srand((unsigned)time(NULL));

    float positions[N * 3];
    float diameters[N];
    for (int i = 0; i < N; i++) {
        positions[i * 3 + 0] = 2.0f * ((float)rand() / RAND_MAX) - 1.0f;
        positions[i * 3 + 1] = 2.0f * ((float)rand() / RAND_MAX) - 1.0f;
        positions[i * 3 + 2] = 0.0f;
        diameters[i] = 4.0f + 8.0f * ((float)rand() / RAND_MAX);
    }

    DvzScene* scene = dvz_scene();
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
    DvzPanel* panel = dvz_panel_full(figure);

    DvzVisual* point = dvz_point(scene, 0);
    dvz_visual_set_data(point, "position", positions, N);
    dvz_visual_set_data(point, "diameter", diameters, N);
    dvz_panel_add_visual(panel, point, NULL);

    DvzApp* app = dvz_app(scene);
    DvzView* view = dvz_view_glfw(app, figure, 800, 600, "Scatter plot");
    dvz_view_panzoom(view, panel, NULL);
    dvz_app_run(app, 0);

    dvz_app_destroy(app);
    dvz_scene_destroy(scene);
    return 0;
}
```

`dvz_app_run(app, 0)` blocks until the user closes the window. Passing a positive integer
instead runs that many frames and returns, which is useful for offscreen capture.


## Python ctypes version

```python
import ctypes
import numpy as np
import datoviz.raw as dvz

N = 10_000
rng = np.random.default_rng(0)

pos = np.zeros((N, 3), dtype=np.float32)
pos[:, 0] = rng.uniform(-1, 1, N).astype(np.float32)
pos[:, 1] = rng.uniform(-1, 1, N).astype(np.float32)
diameters = rng.uniform(4, 12, N).astype(np.float32)

scene = dvz.dvz_scene()
figure = dvz.dvz_figure(scene, 800, 600, 0)
panel = dvz.dvz_panel_full(figure)

visual = dvz.dvz_point(scene, 0)
dvz.dvz_visual_set_data(
    visual, b"position",
    ctypes.cast(pos.ctypes.data, ctypes.c_void_p), N)
dvz.dvz_visual_set_data(
    visual, b"diameter",
    ctypes.cast(diameters.ctypes.data, ctypes.c_void_p), N)
dvz.dvz_panel_add_visual(panel, visual, None)

app = dvz.dvz_app(scene)
view = dvz.dvz_view_glfw(app, figure, 800, 600, b"Scatter plot")
dvz.dvz_view_panzoom(view, panel, None)
dvz.dvz_app_run(app, 0)

dvz.dvz_app_destroy(app)
dvz.dvz_scene_destroy(scene)
```

`import datoviz.raw as dvz` loads the generated ctypes bindings. Every function name and
argument type is identical to the C API. NumPy arrays are passed as `c_void_p` pointers.


## What you should see

A dark window containing 10,000 colored dots of varying size. Drag to pan, scroll to zoom.

<!-- SCREENSHOT_PLACEHOLDER -->


## Next steps

- Browse the [Examples gallery](../examples/index.md) for visual families, features, and
  showcase scenes.
- See [Start Here](index.md) for a full capability and task map.
- To render without a window, see [Render offscreen](../how-to/render-offscreen.md).
- To add a 3D rotation controller instead of panzoom, replace `dvz_view_panzoom` with
  `dvz_view_arcball` — see [Use arcball](../how-to/use-arcball.md).
