# Use from C or C++

Build native applications against the Datoviz C API.

## Task Workflow

Include the public headers, link `libdatoviz`, create the scene/app hierarchy, and keep ownership
rules explicit. In C++, wrap Datoviz pointers in your own RAII types only if the wrapper preserves
Datoviz lifetimes.

## Minimal Call Sequence

```c
#include "datoviz/app.h"
#include "datoviz/scene.h"

DvzScene* scene = dvz_scene();
DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
DvzPanel* panel = dvz_panel_full(figure);

DvzVisual* visual = dvz_point(scene, 0);
dvz_visual_set_data(visual, "position", pos, n);
dvz_visual_set_data(visual, "color", color, n);
dvz_visual_set_data(visual, "diameter", diameter, n);
dvz_panel_add_visual(panel, visual, NULL);

DvzApp* app = dvz_app(scene);
dvz_view_glfw(app, figure, 800, 600, "Datoviz");
dvz_app_run(app, 0);
dvz_app_destroy(app);
dvz_scene_destroy(scene);
```


## Important Details

The C API is the primary v0.4 surface. Public headers live in `include/datoviz/`; implementation
details live under `src/` and should not be included by applications.

## Common Mistakes

- Including private headers from `src/`.
- Destroying scene-owned objects manually unless the API grants that ownership.
- Copying generated gallery source from the built docs instead of using `examples/c/...`.

## See Also

- [Create a scene](create-a-scene.md)
- [Open an interactive window](create-a-window.md)
- [Use raw ctypes](use-raw-ctypes.md)

??? example "Related examples"

    - Gallery: [Scatter Plot](../examples/gallery/start/start_scatter.md)
    - Source: `examples/c/start/scatter.c`
    - Gallery: [GLFW App](../examples/gallery/features/feature_app_glfw.md)
    - Source: `examples/c/features/app_glfw.c`
    - Source manifest: `examples/c/MANIFEST.yaml`
