# The scene

The **scene** provides facilities to create **panels** (subplots) within a canvas, add visual elements to the panels in various coordinate systems, and define **controllers** (how to interact with the visuals in the panels).


## Coordinate system

Datoviz uses the standard OpenGL 3D coordinate system:

![Datoviz coordinate system](../images/cds.svg)
*Datoviz coordinate system*

Note that this is different from the Vulkan coordinate system, where y and z go in the opposite direction. The other difference is that in Datoviz, all axes range in the interval `[-1, +1]`. In the original Vulkan coordinate system, `z` goes from 0 to 1 instead.

This convention makes it possible to use existing camera matrix routines implemented in the cglm library. The GPU code of all included shaders include the final OpenGL->Vulkan transformation right before the vertex shader output.

Other conventions for `x, y, z` axes will be supported in the future.


## Data transforms

Position props are specified in the original data coordinate system corresponding to the scientific data to be visualized. Yet, Datoviz requires vertex positions to be in normalized coordinates (between -1 and 1) when sent to the GPU. Since the GPU only deals with single-precision floating point numbers, doing data normalization on the GPU would result in significant loss of precision and would harm performance.

Therefore, Datoviz provides a system to make transformations on the CPU **in double precision** before uploading the data to the GPU. By default, the data is linearly transformed to fit the [-1, +1] cube. Other types of transformations will soon be implemented (polar coordinates, geographic coordinate systems, and so on).


## Controllers

When creating a new panel, one needs to specify a **Controller**. This object defines how the user interacts with the panel.

=== "Python"
    ```python
    panel = c.panel(row=0, col=0, controller='axes')
    ```
=== "C"
    ```c
    DvzPanel* panel = dvz_scene_panel(scene, 0, 0, DVZ_CONTROLLER_AXES_2D, 0);
    ```

The controllers currently implemented are:

* **static**: no interactivity,
* **panzoom**: pan and zoom with the mouse,
* **axes**: axes with ticks, tick labels, grid, and interactivity with pan and zoom,
* **arcbcall**: static 3D camera, model rotation with the mouse,
* **camera**: first-person 3D camera.

More controllers will be implemented in the future. The C interface used to create custom controllers will be refined too.

### Panzoom

The **panzoom controller** provides mouse interaction patterns for panning and zooming:

* **Mouse dragging with left button**: pan
* **Mouse dragging with right button**: zoom in x and y axis independently
* **Mouse wheel**: zoom in and out in both axes simultaneously
* **Double-click with left button**: reset to initial view

### Axes 2D

The **axes 2D controller** displays ticks, tick labels, grid and provides panzoom interaction.

### Arcball

The arcball controller is used to rotate a 3D object in all directions using the mouse. It is implemented with quaternions.

### First-person camera

Left-dragging controls the camera, the arrow keys control the position, the Z is controlled by the mouse wheel.


## Subplots

Panels are organized within a **grid** layout. Each panel is indexed by its row and column. By default, there is a single panel spanning the entire canvas.

By default, a regular grid is created. You can customize the size of each colum, row, and make panels span multiple grid cells. Only the C interface is implemented at the moment.

=== "C"
    ```c
    dvz_panel_size(panel, DVZ_GRID_HORIZONTAL, 0.5); // proportion of the width
    dvz_panel_span(panel, DVZ_GRID_HORIZONTAL, 2); // the panel spans 2 horizontal cells
    ```
