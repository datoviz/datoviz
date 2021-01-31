# The scene

The **scene** provides facilities to create **panels** (subplots) within a canvas, add visual elements to the panels in various coordinate systems, and define **controllers** (how to interact with the visuals in the panels).


## Coordinate system

Visky uses the standard OpenGL 3D coordinate system, which is different from the Vulkan coordinate system:

--> IMAGE OPENGL=VISKY / VULKAN

This convention makes it possible to use existing camera matrix routines implemented in the cglm library. The GPU code of all included shaders include the final OpenGL->Vulkan transformation right before the vertex shader output.

Other conventions for x, y, z axes will be supported in the future.


## Data transforms

Position props are specified in the original data coordinate system corresponding to the scientific data to be visualized. Yet, Visky requires vertex positions to be in normalized coordinates (between -1 and 1) when sent to the GPU. Since the GPU only deals with single-precision floating point numbers, doing data normalization on the GPU would result in significant loss of precision and would harm performance.

Therefore, Visky provides a system to make transformations on the CPU **in double precision** before uploading the data to the GPU. By default, the data is linearly transformed to fit the [-1, +1] cube. Other types of transformations will soon be implemented (polar coordinates, geographic coordinate systems, and so on).


## Controllers

Once a panel is created, one needs to specify a **Controller**. This object defines how the user interacts with the panel.

--> CODE EXAMPLE python and C showing how to define a controller

There are several builtin controllers.

### Panzoom

The **Panzoom controller** provides mouse interaction patterns for panning and zooming:

* **Mouse dragging with left button**: pan
* **Mouse dragging with right button**: zoom in x and y axis independently
* **Mouse wheel**: zoom in and out in both axes simultaneously
* **Double-click with left button**: reset to initial view



### Axes 2D

The **axes 2D controller** displays ticks, tick labels, grid and provides panzoom interaction.



### Arcball

The arcball controller is used to rotate a 3D object in all directions using the mouse. It is implemented with quaternions.


### First-person cameras

Visky provides two first-person cameras at the moment:

* **FPS camera**: left-dragging controls the camera, the arrow keys control the position, the Z is controlled by the mouse wheel.
* **Fly camera**: like the FPS camera, but the Up and Down keys advance the camera in the 3D direction determined by the mouse. The Z axis has no special role.



## Subplots

Panels are organized within a **grid** layout. Each panel is indexed by its row and column. By default, there is a single panel spanning the entire canvas.

TODO: widths, heights, span
