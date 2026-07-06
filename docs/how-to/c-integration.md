# Use from C or C++

Use this page when you want to put a Datoviz scene inside a native C or C++ program. This is the
right path for desktop tools, acquisition software, simulation viewers, or any application that
needs its own native build system.

The basic workflow is the same as in the examples:

1. prepare the arrays you want to draw;
2. create a scene, figure, and panel;
3. create a visual and attach arrays to its attributes;
4. add the visual to the panel;
5. open a window or render offscreen;
6. run the app or capture a frame.

If you are writing C++, call the same C API from your C++ code. You can wrap Datoviz pointers in
your own classes, but keep the same creation and destruction order shown here.

## Core Call Sequence Fragment

This fragment shows the C calls that connect a visual to a window. It assumes that `positions`,
`colors`, `diameters`, and `n` were prepared earlier. For a complete runnable program, use
`examples/c/start/scatter.c`.

```c
#include "datoviz/app.h"
#include "datoviz/scene.h"

DvzScene* scene = dvz_scene();
DvzFigure* figure = dvz_figure(scene, 800, 600, 0);
DvzPanel* panel = dvz_panel_full(figure);
DvzApp* app = NULL;

DvzVisual* points = dvz_point(scene, 0);
DvzVisualDataUpdate updates[] = {
    {.attr_name = "position", .data = positions, .item_count = n},
    {.attr_name = "color",    .data = colors,    .item_count = n},
    {.attr_name = "diameter_px", .data = diameters, .item_count = n},
};
if (dvz_visual_set_data_many(points, updates, 3) != 0)
    goto cleanup;
if (dvz_panel_add_visual(panel, points, NULL) != 0)
    goto cleanup;

app = dvz_app(scene);
dvz_view_window(app, figure, 800, 600, "Datoviz");
dvz_app_run(app, 0);

cleanup:
if (app != NULL)
    dvz_app_destroy(app);
dvz_scene_destroy(scene);
```

Create and populate figures, panels, visuals, controllers, and callbacks before `dvz_app_run()`.
Pass `0` to run until the user closes the window; pass a positive frame count for smoke tests.

## Choose A Build Path

If a v0.4 package is available for your platform, use the installed-package path. If not, build
Datoviz from source and link against that local build. Use the repository examples or
`just c-integration-smoke` to check a local C integration.

After a v0.4 package provides C/C++ integration files, you have two main choices.

### Installed package

Use this path when Datoviz has been installed into the environment where you build your
application. A small helper named `datoviz-config` can print the include path, library path, and
CMake package directory from the active Python environment:

```sh
cc main.c $(datoviz-config --cflags --libs) -o my_datoviz_app
./my_datoviz_app
```

This path is intended for GCC-compatible shells on Linux, macOS, and MSYS2/MinGW. On Windows with
MSVC, use CMake instead of `datoviz-config`; the script does not emit `/I` or `/LIBPATH:` flags.

For CMake projects, use the exported Datoviz package:

```cmake
find_package(datoviz CONFIG REQUIRED)

add_executable(my_datoviz_app main.c)
target_link_libraries(my_datoviz_app PRIVATE datoviz::datoviz)
```

After installing a v0.4 package, point CMake at the package directory when it is not on the normal
prefix path:

```sh
cmake -S . -B build -Ddatoviz_DIR="$(datoviz-config --cmake-dir)"
cmake --build build
```

### Source checkout

Use this path when packages are not available yet, when you need the current development branch, or
when you want Datoviz to be built as part of your project. CMake `FetchContent` is one option:

```cmake
include(FetchContent)
FetchContent_Declare(datoviz
    GIT_REPOSITORY https://github.com/datoviz/datoviz.git
    GIT_TAG v0.4-dev  # replace with an exact release tag or commit when available
)
FetchContent_MakeAvailable(datoviz)

add_executable(my_datoviz_app main.c)
target_link_libraries(my_datoviz_app PRIVATE datoviz::datoviz)
```

On Windows, copy the Datoviz DLL next to your executable after build:

```cmake
add_custom_command(TARGET my_datoviz_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:datoviz::datoviz>"
        "$<TARGET_FILE_DIR:my_datoviz_app>"
)
```

See [Build options](../reference/build-options.md) for install exports, package smoke presets, and
the `just c-integration-smoke` validation target.

## Important Details

The public headers live in `include/datoviz/`. Application code should include those headers only;
files under `src/` are implementation files and can change without public compatibility promises.

Figures, panels, visuals, and controllers are destroyed with the scene unless a specific API says
otherwise. Destroy the app before destroying the scene it was created from.

For repeated updates, keep the same visual and replace only the attributes that changed. For
example, if 100 points move each frame, keep one point visual with 100 positions and update its
`position` attribute. Creating a new visual for every point or every frame is usually harder to
manage and slower.

Use `dvz_visual_set_data_many()` when changing dense per-item attributes together, especially when
the item count changes. It validates the batch before replacing existing visual payloads.

If another toolkit drives the event loop, such as Qt, use the hosted rendering path for that toolkit
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
    - [GLFW App](../examples/gallery/runtime/feature_app_glfw.md) - Source: `examples/c/runtime/app_glfw.c`
    - Source manifest: `examples/c/MANIFEST.yaml`
