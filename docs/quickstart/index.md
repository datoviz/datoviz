# Quick start

This quick start guide will introduce Visky and will bring you through the installation of the library and your first basic plots (scatter plots, lines, images...).


## What is Visky?

**Visky** is an **interactive scientific data visualization library** leveraging the graphics processing unit (**GPU**) for high performance and scalability. It targets **3D rendering** as well as high-quality antialiased **2D graphics**. Visky is **written in C** and provides native Python bindings (based on Cython).


## Features

* **High-quality antialiased visuals** (leveraging code from the Glumpy library):
    * markers, paths, line segments, text, images, arrows, polygons, meshes, triangulations, planar straight-line graphs (PSLG), histograms, areas, graphs...
* **Mixing 2D and 3D** plots seamlessly in the same canvas
* A comprehensive collection of built-in **colormaps** (popular MATLAB and matplotlib colormaps including viridis, jet, etc.)
* **High-level interactivity**: pan & zoom, arcball, first-person cameras...
* **Axes**, ticks, grids, labels, colorbars...
* **Data transformation**: linear, logarithmic, polar, basic Earth coordinate systems for geographical data visualization
* **Subplots** organized in a grid layout and potentially spanning multiple rows and/or columns
* **Mouse picking** with automatic data coordinate transformation
* Built-in **triangulation algorithms** (Delaunay) via the **triangle** C library, used automatically for polygons and patches
* **DPI-aware**: support for high-resolution monitors
* **Integrated GUIs** via the **Dear ImGUI** C++ library (Qt or other backends not required)
* Easy **video export** (requires compilation with ffmpeg)
* Dedicated API for writing **custom visuals** with custom shaders and data processing pipelines
* Multiple canvases
* Customizable keyboard and mouse interactivity
* An extremely simple basic API for demo and teaching purposes


## Screenshots

TODO


## Hello world example

Here is a basic example displaying a colored triangle:

=== "Python"
    ```python
    import numpy as np
    from visky import basic

    basic.mesh(
        pos=np.array([[-1, -1, 0], [+1, -1, 0], [0, +1, 0]]),
        color=['r', 'g', 'b'])
    basic.run()
    ```

=== "C"
    ```c
    #include <visky/visky.h>

    int main() {
        // An array with 3 vectors.
        vec3 positions[] = {
            {-1, -1, 0}, {+1, -1, 0}, {0, +1, 0}};

        // An array with 3 colors: red, gree, blue, a full opacity.
        VkyColor colors[] = {
            {{255, 0, 0}, 255}, {{0, 255, 0}, 255}, {{0, 0, 255}, 255}};

        // This function automatically creates a canvas and shows a mesh visual
        // with 3 vertices, that is, a triangle.
        vky_basic_mesh(3, positions, colors);

        // Run the example, start the event loop until the window is closed or
        // the Escape key is pressed.
        vky_basic_run();

        return 0;
    }
    ```


## History and motivations

There is a large number of visualization libraries in many languages. Why the need for a new one?

In our experience, most existing libraries do not scale well to large datasets, and performance is frequently an afterthought. Usage of the graphics processor for visualization is typically restricted to 3D, whereas 2D visualization would highly benefit from GPU when displaying millions of points. Additionally, many libraries target a single programming language. We felt there was a lack of a high-performance, GPU-accelerated, language-agnostic scientific visualization library.


### VisPy

Visky borrows ideas and code from several existing projects, the main one being **VisPy**.

This Python scientific visualization library was created in 2013 by Luke Campagnola (developer of **pyqtgraph**), Almar Klein (developer of **visvis**), Nicolas Rougier (developer of **glumpy**), and myself (Cyrille Rossant, developer of **galry**). We had decided to join forces to create a single library borrowing ideas from the four projects. There is today a small community of developers and users around VisPy, although the four initial developers are no longer actively involved in the project. VisPy has recently received [funding from the **Chan Zuckerberg Initiative**](https://chanzuckerberg.com/eoss/proposals/rebuilding-the-community-behind-vispys-fast-interactive-visualizations/), thanks to the efforts of David Hoese, the new maintainer of the library. VisPy is also extensively used by the [napari image viewer software](https://napari.org/).

VisPy is written in pure Python and leverages OpenGL. Visky owes a lot to VisPy, beyond its very name. Many concepts have matured in VisPy and made it to Visky.

However, VisPy had several limitations:

* **Performance**: VisPy's performance is limited by (1) the fact that it is written in pure Python, (2) the fact that it is based on OpenGL, an API that shows its age, and (3) the fact that it accesses OpenGL via Python. With OpenGL, multiple graphics calls need to be made at every event loop iteration, and the Python overheads add up quickly. Making performance acceptable despite these limitations led to a high level of complexity, both in the implementation and in the programming interfaces provided by VisPy.

* **Python**: although Python is a leading language in data visualization and scientific computing, other platform may benefit from a high-performance visualization library, including MATLAB, Julia, R, C++, Rust, Java, and others. VisPy is de facto limited to Python applications.

* **OpenGL**: OpenGL was created in 1992, almost 30 years ago. It has been hugely popular, in particular thanks to a relatively accessible API. However, more modern, low-level graphics APIs such as Vulkan are much more powerful and allow for more efficient usage of modern graphics hardware.


### Vulkan

**Vulkan**, the successor of OpenGL, was announced in 2015 by the Khronos Consortium (who also develops OpenGL), and the first version was released in early 2016. As an open, cross-platform, low-level, modern, high-performance graphics API with good support from most vendors, it seemed like a compelling candidate for the next generation of scientific visualization technology.

As Vulkan provides a C API, C was one possible choice for a development language of this new visualization library. C++ or Rust would have been other good options. C is appealling for its simplicity and portability, although it demands great care and responsibility because of its unsafe nature.


### Credits and related projects

Visky is developed primarily by Cyrille Rossant at the International Brain Laboratory.

Beyond VisPy, prior work used directly or indirectly by Visky include:

* glumpy: Visky uses glumpy's GPU antialiased graphics code, contributed by Nicolas Rougier and published in several computer graphics papers
* antigrain geometry
* freetype
* triangle
* GLFW
* mayavi
* VTK
* ffmpeg
