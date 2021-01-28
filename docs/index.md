# Visky documentation

**Visky** is an open-source **high-performance interactive scientific data visualization library** leveraging the graphics processing unit (**GPU**) for speed, visual quality, and scalability. It supports both 2D and 3D rendering.

Visky is **written in C** and provides native Python bindings (based on Cython). It has been tested on Linux, macOS, and to a lesser extent, Windows. Bindings to other languages could be developed thanks to community efforts (Julia, R, MATLAB, Rust, C#...). Visky uses the **Vulkan graphics API** created by the Khronos consortium, successor of OpenGL. Support of other modern graphics API, such as WebGPU, would constitute interesting developments.

Visky is currently being developed mostly by [Cyrille Rossant](https://cyrille.rossant.net), staff member of the **International Brain Laboratory**, a consortium of neuroscience research labs around the world. Large amounts of data are being collected and need to be visualized, processed, and analyzed efficiently.

Visky is at an early stage of development. The library is quite usable but evolves quickly. Many more features will come later and the documentation will be improved. Contributions are highly welcome!


## Screenshots



## Features

* **High-quality antialiased visuals**: markers, paths, lines, text, arrows, polygons, 3D meshes, volumes, and more
* **Mixing 2D and 3D** plots seamlessly in the same window
* **Colormaps** (coming from matplotlib, colorcet, MATLAB)
* **High-level interactivity**: pan & zoom, arcball, first-person cameras
* **Axes**, ticks, grids, labels
* **Subplots** organized in a grid layout and potentially spanning multiple rows and/or columns
* **DPI-aware**: partial support for high-resolution monitors
* **Integrated GUIs** via the **Dear ImGUI** C++ library (Qt or other backends not required)
* Dedicated API for writing **custom visuals** with custom shaders and data processing pipelines
* Multiple canvases
* Customizable keyboard and mouse interactivity

Upcoming features:

* More visuals: triangulations, planar straight-line graphs (PSLG), histograms, areas, graphs...
* Colorbars
* Further data transformations: logarithmic, polar, basic Earth coordinate systems for geographical data
* Mouse picking
* Built-in triangulation algorithms for polygons and PSLGs
* Screencast and video recording with ffmpeg (optional dependency)
* Better support of multiple GPUs
* A basic API for demo and teaching purposes

## Documentation

The documentation is divided into:

* [**Quick start**](quickstart/index.md): start here!
* [**User manual**](user/index.md): for regular users who are mainly interested in scientific 2D/3D plotting
* [**Expert manual**](expert/index.md): for advanced users who need to create their own visuals by writing **custom GPU shaders in GLSL**
* [**Developer manual**](developer/index.md): for expert users who want to contribute back to Visky
* [**C API reference**](api/index.md): for anyone who needs to use the C API

See also the [**gallery**](gallery.md).

*[Vulkan]: Low-level graphics API created by Khronos, and successor of OpenGL
*[shaders]: code written in GLSL and executed on the GPU to customize the graphics pipeline
*[GLSL]: OpenGL shading language, the C-like language used to write shaders
