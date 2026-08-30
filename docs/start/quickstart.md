# Quickstart: rendering in 10 minutes

**Prerequisite:** Datoviz v0.4 installed from source or from PyPI with `python -m pip install --pre datoviz==0.4.0rc2`. Check [Install](install.md) for platform details.

This page builds one complete visualization: 10,000 random points in an interactive window. You can drag to pan and scroll to zoom. No data files are needed.

The example has five parts: create the data arrays, create the scene layout, upload the arrays to one point visual, bind panzoom interaction, and open the window.

The displayed programs use the same visual contract as the `examples/c/start/scatter.c` gallery scenario: a 1280 by 720 canvas, seeded random positions and colors, translucent 4–12 px points, and XY panzoom. Each language uses its standard local pseudorandom number generator, so the individual coordinates differ but the result has the same appearance.


## Complete runnable programs

Both tabs contain complete programs, including imports or headers, data generation, rendering, and the appropriate session or cleanup path. The source comes from checked files in `examples/docs/`.

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

    From a source checkout, run the displayed program:

    ```sh
    python examples/docs/quickstart.py
    ```

    From another directory, save the Python tab as `quickstart.py` and run it in the environment where Datoviz and NumPy are installed:

    ```sh
    python quickstart.py
    ```

=== "C"

    From a source checkout, compile and run the displayed program:

    ```sh
    just quickstart-c
    ./build/examples/docs/quickstart
    ```

    For a standalone C file outside the repository, use an installed Datoviz package and its exported CMake package or `datoviz-config` helper. See [Use from C or C++](../how-to/c-integration.md).


## What you should see

A dark window containing 10,000 colored dots. Drag to pan, scroll to zoom.

=== "Screenshot"

    ![10 000 colored dots in an interactive window with pan-and-zoom](../assets/gallery/v0.4/start/start_scatter.webp)

=== "Live WebGPU"

    <div class="dvz-webgpu-live" markdown="1">
    <iframe src="../../examples/webgpu/live.html?id=start_scatter" title="Scatter Plot WebGPU live example" loading="lazy" allow="fullscreen; webgpu"></iframe>
    </div>

    <a href="../../examples/webgpu/live.html?id=start_scatter">Open the live WebGPU example</a>.


## How it works

**Data arrays** — The example creates three arrays with the same length. `position` stores the `x`, `y`, and `z` coordinates of each point. `color` stores red, green, blue, and alpha values. `diameter_px` stores point sizes in screen pixels. They are NumPy arrays in Python and ordinary arrays in C.

**Scene, figure, panel** — A `scene` holds the visualization. A `figure` is the complete image area, here 1280 by 720 pixels. A `panel` is a drawing region inside the figure. This example uses one panel that fills the figure.

**Controller** — `dvz_panzoom` adds mouse interaction. `dvz_panel_bind_controller` connects it to the panel and limits the interaction to the X and Y axes.

**Visual** — A visual is a collection rendered in one way, such as points, lines, an image, a mesh, or text labels. Here, `dvz_point` creates one point visual for all 10,000 points. Each `dvz_visual_set_data` call fills one named attribute of that visual: `"position"`, `"color"`, or `"diameter_px"`.

!!! important "Keep the points in one visual"
    Datoviz batches the 10,000 points through this single visual. Do not create 10,000 one-point visuals. A small number of visuals containing many related items is the intended fast path.

**Panel attachment** — Data upload prepares the visual, but it does not place it in the figure. `dvz_panel_add_visual` attaches the visual to the panel so it will be drawn.

**Run and cleanup** — In Python, `dvz.run(scene, figure)` opens the window and blocks while it is open; the helper manages the ordinary app session. In C, the program explicitly creates the app and window view, runs the app, then destroys the app before the scene. Close the window to end either interactive run.


## Change the example safely

Keep the three point-attribute arrays aligned: row `i` of `position`, `color`, and `diameter_px` describes the same point. If you change `n`, regenerate all three arrays with that length. Positions are `float32` with shape `(n, 3)`, colors are `uint8` RGBA with shape `(n, 4)`, and diameters are `float32` with shape `(n,)`.

To adapt this example, replace only the data-generation block at first. Once your arrays work, use [Choose a visual family](../how-to/choose-a-visual-family.md) if points are not the right representation, or [Update visual data](../how-to/update-visual-data.md) to change arrays after the first frame.


## Next steps

- Browse the [examples gallery](../examples/index.md) for visual families, features, and showcase scenes.
- C and C++ users can continue with [First C program](first-c-program.md) for the call order and installed-project path.
- Read [Core concepts](core-concepts.md) for the reusable object and data model.
- To render without a window, see [Render offscreen](../how-to/render-offscreen.md).
- To add 3D rotation instead of panzoom, see [Use 3D controllers](../how-to/3d-navigation.md).
