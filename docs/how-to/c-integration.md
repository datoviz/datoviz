# Use from C or C++

Build native applications against the Datoviz C API.

## Task Workflow

Use this path when your application owns the native event loop, render window, offscreen capture, or
host integration and needs direct access to the v0.4 engine.

Include only public headers, link `libdatoviz`, create the scene hierarchy before running the app,
and keep ownership rules explicit. In C++, wrap Datoviz pointers in your own RAII types only when
the wrapper preserves the same C ownership and destruction order.

## Minimal Call Sequence

```c
#include "datoviz/app.h"
#include "datoviz/scene.h"

DvzScene* scene = dvz_scene();
DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
DvzPanel* panel = dvz_panel_full(figure);

DvzVisual* points = dvz_point(scene, 0);
DvzVisualDataUpdate updates[] = {
    {.attr_name = "position", .data = positions, .item_count = n},
    {.attr_name = "color",    .data = colors,    .item_count = n},
    {.attr_name = "diameter", .data = diameters, .item_count = n},
};
if (dvz_visual_set_data_many(points, updates, 3) != 0)
    return -1;
if (dvz_panel_add_visual(panel, points, NULL) != 0)
    return -1;

DvzApp* app = dvz_app(scene);
dvz_view_glfw(app, figure, 800, 600, "Datoviz");
dvz_app_run(app, 0);

dvz_app_destroy(app);
dvz_scene_destroy(scene);
```

Create and populate figures, panels, visuals, controllers, and callbacks before `dvz_app_run()`.
Pass `0` to run until the user closes the window; pass a positive frame count for smoke tests.

## Build Integration

Installed CMake consumers should use the exported Datoviz package:

```cmake
find_package(datoviz CONFIG REQUIRED)

add_executable(my_datoviz_app main.c)
target_link_libraries(my_datoviz_app PRIVATE datoviz::datoviz)
```

Datoviz can also be embedded with CMake `FetchContent` when source integration is preferred. See
[Build options](../reference/build-options.md) for install exports, package smoke presets, and the
`just c-integration-smoke` validation target.

## Important Details

The C API is the primary v0.4 surface. Public headers live in `include/datoviz/`; implementation
details live under `src/` and should not be included by applications.

Scene-owned objects such as figures, panels, visuals, controllers, and retained scene resources are
destroyed with the scene unless a specific API says otherwise. The app owns runtime presentation
state and should be destroyed before the scene it was created from.

For fixed-size updates, keep the same visual and replace only changed attributes. Recreating visuals
inside frame callbacks defeats the retained scene model and usually makes performance worse.

Use `dvz_visual_set_data_many()` when changing dense per-item attributes together, especially when
the item count changes. It validates the batch before replacing existing visual payloads.

If a host toolkit drives the event loop, call the hosted rendering primitive for that integration
instead of entering `dvz_app_run()`. The Qt/PyQt guide and hosted examples show that pattern.

## Common Mistakes

- Including private headers from `src/`.
- Destroying scene-owned objects manually unless the API grants that ownership.
- Destroying the scene before `dvz_app_destroy()`.
- Creating one visual per item instead of one batched visual with many items.
- Changing one dense attribute's item count without updating the other dense attributes.
- Copying generated gallery source from the built docs instead of using `examples/c/...`.

## See Also

- [Create a scene](create-a-scene.md)
- [Open an interactive window](create-a-window.md)
- [Profile rendering performance](profile-performance.md)
- [Use raw ctypes](use-raw-ctypes.md)
- [Build options](../reference/build-options.md)

??? example "Related examples"

    - [Scatter Plot](../examples/gallery/start/start_scatter.md) - Source: `examples/c/start/scatter.c`
    - [GLFW App](../examples/gallery/features/feature_app_glfw.md) - Source: `examples/c/features/app_glfw.c`
    - Source manifest: `examples/c/MANIFEST.yaml`
