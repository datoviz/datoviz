# Hello triangle

This example shows you how to quickly make a window with a triangle.

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
    ```

This example is pretty self-explanatory. The `vky_basic_mesh()` function automatically sets up an arcball controller: you can rotate the triangle with the mouse in 3D.

The Basic API shown here (`vky_basic_*()` functions) provides a quick way to make the most simplest plots possible. Read the [User manual](../user/index.md) to learn more about the full API provided by Visky.

!!! warning
    The Basic API only serves learning, teaching, and demonstration purposes. It should not be used in production.
