# Quick start

This quick start guide will get you through the installation of the library and your first plots.


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
