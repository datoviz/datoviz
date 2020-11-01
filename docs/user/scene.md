# The scene

Once a Canvas is created, one typically creates a Scene as well (in Python, this is done automatically). The scene gives the functionality required by most scientific applications: subplots, axes, labels, and ready-to-use graphical elements.


## The grid

The **grid** is the two-dimensional organization of the different **panels** (subplots) within the canvas. The canvas is divided into N rows and M columns (by default, N=M=1). Panels are indexed following the mathematical matrix convention: (0, 0) is the index of the top-left panel.

=== "C"
    ```c
    // Create a scene with a white background and 2 rows and 3 columns.
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_WHITE, 2, 3);
    ```

Each cell of the grid may contain a panel. A panel may also span multiple consecutive cells, in which case its position is specified, by convention, by its upper-left cell.

TODO: schematic of a grid with different panels


## Changing column widths and row heights

=== "Python"
    ```python
    # TODO: not implemented yet
    ```

=== "C"
    ```c
    vky_set_grid_widths(scene, (float[]){.25, .5, .25});
    vky_set_grid_heights(scene, (float[]){5, 2});
    ```


## Get a panel

Once a Scene is created, here is how to get a **Panel** object, which is the object used when creating visuals.

=== "Python"
    ```python
    # TODO: not implemented yet
    ```

=== "C"
    ```c
    // Get a panel from its index (row and column) within the grid.
    VkyPanel* panel = vky_get_panel(scene, 0, 0);
    ```


## Horizontal and vertical spanning

Once a panel is created, it can span multiple grid cells toward the right and the bottom.

=== "Python"
    ```python
    # TODO: not implemented yet
    ```

=== "C"
    ```c
    vky_set_panel_span(panel, hspan, vspan);
    ```


## Controllers

Once a panel is created, one needs to specify a **Controller**. This object defines how the user interacts with the panel. This section describes all built-in controllers.

### Panzoom

The **Panzoom controller** doesn't change anything to the contents of the scene (contrary to the axes controllers, see below). This controller only implements mouse interaction patterns for panning and zooming, as described here:

* **Mouse dragging with left button**: pan
* **Mouse dragging with right button**: zoom in x and y axis independently
* **Mouse wheel**: zoom in and out in both axes simultaneously
* **Double-click with left button**: reset to initial view

Here is how to specify this controller for a panel:

=== "C"
    ```c
    // Set the Panzoom controller on a panel.
    // The last argument is a pointer to the controller's parameters.
    // Using NULL means using the default parameters.
    vky_set_controller(panel, VKY_CONTROLLER_PANZOOM, NULL);
    ```


### Axes 2D

The **Axes 2D controller** is similar to the Panzoom controller, but it adds X and Y axes with ticks, grid, optional labels and colorbar, and so on. It is the most complex controller of the library and it supports many options.

!!! note "Inner and outer viewport"
    The Axes 2D controller motivates the introduction of the **inner viewport and outer viewport**.

    To each panel is associated a viewport that specifies the rectangular region of the canvas in which the panel is defined. The Axes 2D controller is special as this viewport contains contains axes and possible labels and colorbars _that are not part_ of the panel's actual contents.

    The smaller viewport within the panel corresponding to the panel's contents is called the **inner viewport**. The larger one is the **outer viewport**.


=== "C"
    ```c
    VkyAxes2DParams params = vky_default_axes_2D_params();
    vky_set_controller(panel, VKY_CONTROLLER_AXES_2D, &params);
    ```


### Arcball

The arcball controller is used to rotate a 3D object in all directions using the mouse. It is implemented with quaternions.

=== "C"
    ```c
    vky_set_controller(panel, VKY_CONTROLLER_ARCBALL, NULL);
    ```


### Axes 3D

The **Axes 3D** controller is similar to the arcball controller, but it displays 3D axes as well.

=== "C"
    ```c
    vky_set_controller(panel, VKY_CONTROLLER_AXES_3D, NULL);
    ```


### First-person cameras

Visky provides two first-person cameras at the moment:

* **FPS camera**: left-dragging controls the camera, the arrow keys control the position, the Z is controlled by the mouse wheel.
* **Fly camera**: like the FPS camera, but the Up and Down keys advance the camera in the 3D direction determined by the mouse. The Z axis has no special role.


=== "C"
    ```c
    vky_set_controller(panel, VKY_CONTROLLER_FLY, NULL);
    vky_set_controller(panel, VKY_CONTROLLER_FPS, NULL);
    ```


### Others


## Panel linking

Panels with an Axes 2D controller can be linked such that panning and zooming on one panel automatically updates both panels simultaneously. The panels can be linked in both X and Y dimensions, or only one of the two.

=== "Python"
    ```python
    # TODO: not implemented yet
    ```

=== "C"
    ```c
    vky_link_panels(panel1, panel2, VKY_PANEL_LINK_ALL);
    ```
