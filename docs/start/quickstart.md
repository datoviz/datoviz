# Quickstart: Rendering in 10 Minutes

**Prerequisites:** Datoviz v0.4 installed from source or from a v0.4 package named in the release
notes. During the pre-RC phase, check [Install](install.md) before using `pip install datoviz`.

This page builds one complete visualization: 10,000 random points in an interactive window. You can
drag to pan and scroll to zoom. No data files are needed.

Read the example in five blocks: create the data arrays, create the scene layout, add interaction,
upload the arrays to one point visual, then open the window.

The displayed programs are standalone teaching fixtures. The repository's
`examples/c/start/scatter.c` gallery scenario produces the same result through the internal example
runner used for screenshot and smoke-test automation.


## Full example

=== "Python"

    ```python
    --8<-- "examples/docs/quickstart.py"
    ```

=== "C"

    ```c
    --8<-- "examples/docs/quickstart.c"
    ```


## Build and run

=== "Python"

    Run the displayed fixture:

    ```sh
    python examples/docs/quickstart.py
    ```

=== "C"

    From a source checkout, compile and run the displayed fixture:

    ```sh
    just quickstart-c
    ./build/examples/docs/quickstart
    ```

    For a standalone C file outside the repository, use an installed Datoviz package and its
    exported CMake package or `datoviz-config` helper. See [Use from C or C++](../how-to/c-integration.md).


## What you should see

A dark window containing 10,000 colored dots. Drag to pan, scroll to zoom.

![10 000 colored dots in an interactive window with pan-and-zoom](../assets/gallery/v0.4/start/start_scatter.webp)


## How it works

**Data arrays** - The example creates three arrays with the same length. `position` stores `x`,
`y`, and `z` coordinates for each point. `color` stores red, green, blue, and alpha values.
`diameter_px` stores the point size in screen pixels. In Python, these are NumPy arrays. In C, they
are ordinary C arrays.

**Scene, figure, panel** - A `scene` is the whole visualization. A `figure` is the image area, here
800 by 600 pixels. A `panel` is the part of the figure where the scatter plot is drawn. This
quickstart uses one full-size panel.

**Controller** - `dvz_panzoom` adds mouse interaction. `dvz_panel_bind_controller` connects it to
the panel and limits the interaction to the X and Y axes.

**Visual** - A visual is a renderable collection, such as points, lines, an image, a mesh, or text
labels. Here, `dvz_point` creates one point visual for all 10,000 points. Each
`dvz_visual_set_data` call fills one named attribute of that visual: `"position"`, `"color"`, or
`"diameter_px"`.

**Panel attachment** - Data upload prepares the visual, but it does not place it in the figure.
`dvz_panel_add_visual` attaches the visual to the panel so it will be drawn.

**Run** - `dvz.run(scene, figure)` opens the window in the Python example. The script waits while
the window is open; close the window when you are done.


## Next steps

- Browse the [Examples gallery](../examples/index.md) for visual families, features, and showcase scenes.
- To render without a window, see [Render offscreen](../how-to/render-offscreen.md).
- To add 3D rotation instead of panzoom, see [Use 3D controllers](../how-to/3d-navigation.md).
