# Quickstart: using Datoviz in Python

Once Datoviz has been properly installed, you can start to use it in a few lines of code!

In this tutorial, we'll show **how to make a simple 2D plot with Datoviz in Python**, and we'll go through the most important features of the library.

<!-- IMAGE ../images/screenshots/standalone_scene.png -->

We'll cover the following steps:

* how to create an application,
* how to create a canvas,
* how to create a panel with an axes controller,
* how to add a visual,
* how to use colormaps,
* how to set visual data,
* how to run the application,
* how to create a minimal GUI,
* how to specify event callbacks,
* how to use mouse picking,
* how to make a live screencast video (*requires compilation with ffmpeg*),
* and more.

Creating custom visuals, creating standalone C applications with Datoviz are advanced topics, they are covered in the **How to** section of the documentation.


!!! note
    The Python bindings are at an early stage of development. They will be significantly improved in the near future.


## Importing the library

!!! note
    For now, Datoviz should be used from a Python script. Integration with IPython and Jupyter is still a work in progress.

First, we import NumPy and datoviz:

```python
import numpy as np
import numpy.random as nr

from datoviz import canvas, run, colormap
```


## Creating a canvas

We create a **canvas**:

```python
c = canvas(show_fps=False)
```

We can also specify the initial width and height of the window using keyword arguments to `canvas()`.


## Creating a panel

Next, we create a **panel**, which is another word for "subplot". By default, there is only one panel spanning the entire canvas, but we can also define multiple panels.

We also specify the panel's **controller**, which defines how we interact with it. The `axes` controller displays axes and ticks for 2D graphics.

```python
panel = c.panel(controller='axes')
```



## Choosing one of the existing visuals

We'll make a simple **scatter plot** with 2D random points, and different colors and marker sizes.

We refer to [the list of all included visuals](../reference/visuals.md) provided by the Datoviz documentation, and we find that the **marker visual** is what we want for our scatter plot. We look at the **visual properties** (or **props**) for this visual: this is the data we'll need to feed to our visual.

But first, we create our visual object by specifying its type:

```python
visual = panel.visual('marker')
```



## Preparing the visual data

We'll set:

* the **marker positions**: `pos` prop,
* the **marker colors**: `color` prop,
* the **marker sizes**: `ms` prop.

First, we generate the data for these props.

### Random positions

```python
N = 100_000
pos = nr.randn(N, 3)
```

Note that **positions always have three dimensions** in Datoviz. When using 2D plotting, we set the third component to zero.

Datoviz uses the standard OpenGL 3D coordinate system:

![Datoviz coordinate system](../images/cds.svg)
*Datoviz coordinate system*

!!! note
    Note that this is different from the Vulkan coordinate system, where y and z go in the opposite direction. The other difference is that in Datoviz, all axes range in the interval `[-1, +1]`. In the original Vulkan coordinate system, `z` goes from 0 to 1 instead. The convention used in Datoviz makes it possible to use existing camera matrix routines implemented in the cglm library. The GPU code of all included shaders include the final OpenGL->Vulkan transformation right before the vertex shader output. Other conventions for `x, y, z` axes will be supported in the future.

Position props are specified in the original data coordinate system corresponding to the scientific data to be visualized. Yet, Datoviz requires vertex positions to be in normalized coordinates (between -1 and 1) when sent to the GPU. Since the GPU only deals with single-precision floating point numbers, doing data normalization on the GPU would result in significant loss of precision and would harm performance.

Therefore, Datoviz provides a system to make transformations on the CPU **in double precision** before uploading the data to the GPU. By default, the data is linearly transformed to fit the [-1, +1] cube. Other types of transformations will soon be implemented (polar coordinates, geographic coordinate systems, and so on).




### Random marker size

We define random marker sizes as an array of floating-point values:

```python
ms = nr.uniform(low=2, high=40, size=N)
```

### Colormap

Let's define the colors. We could use random RGBA values for the colors, but we'll use one of the built-in **colormap** instead.


```python
color = colormap(nr.rand(N), vmin=0, vmax=1, alpha=.75 * np.ones(N), cmap='viridis')
```

The variable `color` is an `(N, 4)` array of `uint8` (byte values between 0 and 255).

This line involves the following steps:

* Choosing a colormap, here **viridis** (see the [colormap reference page](../reference/colormaps.md) with the list of ~150 included colormaps),
* Defining an array of scalar values to be fed to the colormap (random values between 0 and 1 here),
* (Optional) Defining the colormap range (`[0, 1]` here),
* (Optional) Setting an alpha transparency channel (0.75 here).



## Set the visual data

Finally, the most important bit is to **set the visual prop data with the arrays we just created**:

```python
visual.data('pos', pos)
visual.data('color', color)
visual.data('ms', ms)
```



## Running the application

Finally, we run the application by starting the main event loop:

```python
run()
```


### Making a screenshot

We can easily make a screenshot of *the first frame* of the canvas.

!!! note
    Screenshot support will be improved soon.

```python
run(screenshot="screenshot.png")
```


### Recording a screencast video

If the library was compiled with ffmpeg, we can easily make a live mp4 screencast of the canvas.

!!! warning
    The canvas should NOT be resized when doing a screencast, or the video will be corrupted.

```python
run(video="screencast.mp4")
```

This command **does not** start the video recording, one needs to press the *Play* button at the bottom right corner. We can pause and resume at any time. When we're done, we press the *Stop* button and the video will be saved to disk.

![Playback buttons for screencast](https://user-images.githubusercontent.com/1942359/107956919-2cae4100-6fa0-11eb-97d5-e83a70e01183.png)



## Event callbacks and mouse picking

!!! important
    From now on, all the code snippets below needs to be added **before** calling `run()`.

We'll write a callback function that is called when the user clicks in the canvas, and that prints the coordinates of the clicked point in the original data coordinate system.

```python
# We define an event callback to implement mouse picking
@c.connect
def on_mouse_click(x, y, button, modifiers=()):
    # x, y are in pixel coordinates
    # First, we find the picked panel
    p = c.panel_at(x, y)
    if not p:
        return
    # Then, we transform into the data coordinate system
    # Supported coordinate systems:
    #   target_cds='data' / 'scene' / 'vulkan' / 'framebuffer' / 'window'
    xd, yd = p.pick(x, y)
    print(f"Pick at ({xd:.4f}, {yd:.4f}), modifiers={modifiers}")
```

Clicking somewhere shows in the terminal output: `Pick at (0.4605, -0.1992), modifiers=()`

### Coordinate systems

By default, the `panel.pick()` function converts coordinates from the window coordinate system (used by the event callbacks) to the data coordinate system. There are other coordinate systems that you can convert to using the `target_cds` keyword argument to `pick()`:

| Name | Description |
| ---- | ---- |
| `data` | original coordinates of the data |
| `scene` | the coordinates *before* controller transformation (panzoom etc) in `[-1, +1]` |
| `vulkan` | the coordinates *after* controller transformation, in `[-1, +1]` |
| `framebuffer` | the coordinates in framebuffer pixel coordinates |
| `window` | the coordinates in screen pixel coordinates |

A few technical notes:

* The `scene` coordinate system corresponds to the **vertex shader input**.
* The `vulkan` coordinate system corresponds to the **vertex shader output**.
* There's a difference between the `framebuffer` and `window` systems with high-DPI monitors. This depends on the OS.

For now, DPI support is semi-manual. Datoviz supports a special `dpi_scaling` variable that rescales the visual elements depending on this value, and that can be adjusted manually (to be documented later).


## Adding a simple GUI

Datoviz integrates the [Dear ImGUI library](https://github.com/ocornut/imgui) which allows one to create GUIs directly in a Datoviz canvas, without using external dependencies such as Qt.

### Adding a GUI dialog

We create a new GUI dialog.

```python
# We create a GUI dialog.
gui = c.gui("Test GUI")
```


### Adding a control to the GUI

We add a slider to change the visual marker size.

```python
# We add a control, a slider controlling a float
@gui.control("slider_float", "marker size", vmin=.5, vmax=2)
def on_change(value):
    # Every time the slider value changes, we update the visual's marker size
    visual.data('ms', ms * value)
    # NOTE: an upcoming version will support partial updates
```

![float slider](https://user-images.githubusercontent.com/1942359/107957858-82cfb400-6fa1-11eb-8369-effe0e0c1950.png)



We add another slider, using integers this time, to change the colormap.

```python
# We add another control, a slider controlling an int between 1 and 4, to change the colormap.
# NOTE: an upcoming version will provide a dropdown menu control
cmaps = ['viridis', 'cividis', 'autumn', 'winter']

@gui.control("slider_int", "colormap", vmin=0, vmax=3)
def on_change(value):
    # Recompute the colors.
    color = colormap(
        color_values, vmin=0, vmax=1, alpha=.75 * np.ones(N), cmap=cmaps[value])
    # Update the color visual
    visual.data('color', color)
```

![int slider](https://user-images.githubusercontent.com/1942359/107957994-ac88db00-6fa1-11eb-9b00-62fc41e77a35.png)



Finally we add a button to regenerate the marker positions.

```python
# We add a button to regenerate the marker positions
@gui.control("button", "new positions")
def on_change(value):
    pos = nr.randn(N, 3)
    visual.data('pos', pos)
```

![](https://user-images.githubusercontent.com/1942359/107958095-d17d4e00-6fa1-11eb-9a90-b1b6ba07785d.png)
