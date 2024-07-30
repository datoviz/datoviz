# User guide

Creating a GPU-based interactive visualization script with Datoviz in Python typically involves the following steps:

1. Creating an `app` and a `scene`.
    * The `app` handles the window, user events, event loop.
    * The `scene` handles the panels, visuals, and data.
2. Creating one or several `figures` (window).
3. Creating one or several `panels` (subplots) in each figure, defined by their offset and size in pixels.
4. Creating `visuals` of predefined types (more to come later depending on user feedback):
    * **0D visuals**: pixels, discs, markers, text glyphs
    * **1D visuals**: line segments, paths
    * **2D visuals**: images
    * **3D visuals**: meshes, spheres, volume rendering, volume image slices
5. Setting the visual data (position, size, color, groups...).
6. Optionally, seting up event callbacks (mouse, keyboard, timers...).
7. Optionally, creating GUIs.
8. Running the application.
9. Close and destroy the `scene` and `app`.

> The user guide is still a work in progress. In the meantime, please look at the [examples](docs/examples.md) and [API reference](docs/api.md).
