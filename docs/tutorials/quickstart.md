# Quickstart: using Datoviz in Python

In this tutorial, we'll cover the most important features of Datoviz by **creating a simple 2D plot in Python**.

<!-- IMAGE ../images/screenshots/standalone_scene.png -->

We'll cover the following steps:

* how to create an application,
* how to create a canvas,
* how to create a panel with an axes controller,
* how to add a visual,
* how to use a colormap,
* how to set visual data,
* how to run the application,
* how to create a graphical user interface (GUI),
* how to specify event callbacks,
* how to implement mouse picking.

More advanced topics are covered in the **How to** section of the documentation.


## Importing the library

Datoviz can be used from a Python script, or interactively in IPython.

!!! note
    Interactive use in IPython is still experimental.

First, we import NumPy and datoviz:

```python
import numpy as np
import numpy.random as nr

from datoviz import canvas, run, colormap
```


## Creating a canvas

We create a **canvas**:

```python
c = canvas()
```

The `canvas()` function accepts a few keyword arguments, including `width` and `height` for the window's initial size.


## Creating a scene

In order to draw something on the canvas, we need to define a **scene** which implements plotting functionality:

`s = c.scene()`

The scene contains subplots (also known as **panels**) organized in a two-dimensional grid layout. The number of rows and columns may be specified by the `rows` and `cols` keyword arguments to the `c.scene()` method. By default, `rows=1` and `cols=1`.


## Creating a panel

Each "cell" in the grid layout of the scene is a panel. Every panel has a **controller** which defines how we interact with the panel's visuals. There are several built-in controllers. In particular, the `axes` controller displays axes and ticks for 2D graphics:

```python
panel = c.panel(controller='axes')
```


## Choosing one of the existing visuals

The next step is to add a visual.

Here, we'll make a **scatter plot** with random points in different colors and sizes.

Datoviz comes with a [library of built-in visuals](../reference/visuals.md). For our scatter plot, we'll choose a **marker visual**. We could also have used a **point visual**, which is faster and more lightweight, but only supports square markers.

```python
visual = panel.visual('marker')
```


## Preparing the visual data

Once the visual has been created and added to the panel, we need to set its data. Visual data is specified with **visual props** (properties). The documentation lists all props supported by each visual.

Except from the universal `pos` prop which refers to the point positions, most props are optional and come with sensible defaults.

In this example, we'll just set:

* `pos`: the **marker positions**
* `color`: the **marker colors**
* `ms`: the **marker sizes**

We generate the data as NumPy arrays, and we pass them to the visual.


### Random positions

```python
N = 10_000
pos = nr.randn(N, 3)
```

Note that **positions always have three dimensions** in Datoviz. When using 2D plotting, we just set the third component to zero.

Datoviz uses the standard OpenGL 3D coordinate system, with coordinates in `[-1, +1]`:

![Datoviz coordinate system](../images/cds.svg)
*Datoviz coordinate system*

!!! note Vulkan coordinate system
    Vulkan uses a slightly different coordinate system, the main differences are:

    * `y` and `z` go in the opposition direction
    * `z` is in `[0, 1]`

    The conventions chosen in Datoviz are closer to existing graphics libraries.

    The transformation from the Datoviz coordinate system to the Vulkan coordinate system is done at the final stage of data transformation in the vertex shader of all included visuals.

The point positions that are passed to Datoviz are defined in a **data coordinate system**. Datoviz takes care of the transformation to a normalized coordinate system that is more amenable to GPU interactive graphics.

!!! note
    The data transformation pipeline in Datoviz only supports linear transformations at the moment. It will be improved soon.


### Random marker size

We define random marker sizes (in pixels) as an array of floating-point values:

```python
ms = nr.uniform(low=2, high=40, size=N)
```

### Colormaps

Colors are specified as either:

* RGBA components, as `uint8` bytes (four bytes per color),
* **colormaps**.

Datoviz includes [a library of ~150 colormaps commonly used in popular scientific plotting software]((../reference/colormaps.md)). You can also define a custom colormap manually.

Here, we'll use the **viridis colormap**.

First, we define a scalar value for each point, which will be mapped to the colormap afterwards:

```python
color_values = nr.rand(N)
```

We also prepare the alpha channel for transparency, from 0 (invisible) to 1 (opaque):

```python
alpha = .75 * np.ones(N)
```

Then, we get the RGBA colors with the colormap:

```python
color = colormap(
    color_values, vmin=0, vmax=1, alpha=alpha, cmap='viridis')
```

The variable `color` is an `(N, 4)` array of `uint8` (byte values between 0 and 255). It can be passed directly to the `color` prop of our visual.


## Set the visual data

Once the data has been prepared, we can pass it to the visual:

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

!!! note "IPython integration"
    If you are in IPython's interactive terminal, you should do `%gui datoviz` first, and omit the call to `run()`.


## Event callbacks and mouse picking

!!! important
    From now on, all of the code snippets need to be added **before** calling `run()`.

We'll write a callback function that runs whenever the user clicks somewhere in the canvas. It will display the coordinates of the clicked point in the original data coordinate system.

```python
# We define an event callback to implement mouse picking
@c.connect
def on_mouse_click(x, y, button, modifiers=()):
    # x, y are in pixel coordinates
    # First, we find the picked panel
    p = s.panel_at(x, y)
    if not p:
        return
    # Then, we transform the mouse positions into the data coordinate system.
    # Supported coordinate systems:
    #   target_cds='data' / 'scene' / 'vulkan' / 'framebuffer' / 'window'
    xd, yd = p.pick(x, y)
    print(f"Pick at ({xd:.4f}, {yd:.4f}), {'+'.join(modifiers)} {button} click")
```

Clicking somewhere shows in the terminal output:

```
Pick at (0.4605, -0.1992), modifiers=()
```


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


## Adding a graphical user interface

Datoviz supports built-in GUIs via the [Dear ImGui library](https://github.com/ocornut/imgui).

!!! note
    Dear ImGui allows you to create minimal GUIs without needing large dependencies such as Qt. However, full Qt support is also planned in the near future.


### Adding a GUI dialog

We create a new GUI dialog:

```python
gui = c.gui("Test GUI")  # the argument is the dialog's title
```


### Adding a control to the GUI

We add a slider to change the visual marker size.

```python
sf = gui.control("slider_float", "marker size", vmin=.5, vmax=2)

# We write the Python callback function for when the slider's value changes.
@sf.connect
def on_change(value):
    # Every time the slider value changes, we update the visual's marker size.
    visual.data('ms', ms * value)
```

![float slider](https://user-images.githubusercontent.com/1942359/107957858-82cfb400-6fa1-11eb-8369-effe0e0c1950.png)


We add another slider, using integers this time, to change the colormap.

```python
# We add a second, slider controlling an integer between 1 and 4, to change the colormap.
# NOTE: an upcoming version will provide a dropdown menu control.
si = gui.control("slider_int", "colormap", vmin=0, vmax=3)

# Predefined list of colormaps.
cmaps = ['viridis', 'cividis', 'autumn', 'winter']

@si.connect
def on_change(value):
    # When the slider changes, we recompute the colors.
    color = colormap(color_values, vmin=0, vmax=1, alpha=.75 * np.ones(N), cmap=cmaps[value])
    # We update the color visual.
    visual.data('color', color)
```

![int slider](https://user-images.githubusercontent.com/1942359/107957994-ac88db00-6fa1-11eb-9b00-62fc41e77a35.png)


Finally we add a button to regenerate the marker positions.

```python
b = gui.control("button", "new positions")

@b.connect
def on_change(value):
    # We update the marker positions.
    pos = nr.randn(N, 3)
    visual.data('pos', pos)
```

![](https://user-images.githubusercontent.com/1942359/107958095-d17d4e00-6fa1-11eb-9a90-b1b6ba07785d.png)


!!! note "Asynchronous callbacks"
    By default, callbacks are synchronous and run in the main thread. As such, they will block the user interface is they take too long to run. Currently, the only way of implementing an asynchronous callback is to use the `asyncio` event loop. [Look at the eventloop code example](../examples/eventloop.md).
