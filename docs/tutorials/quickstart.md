# Quickstart: using Datoviz in Python

Once Datoviz has been properly installed, you can start to use it in a few lines of code!

In this tutorial, we'll show **how to make simple 2D and 3D plots with Datoviz in Python**, and we'll go through the most important notions in the library.

<!-- IMAGE ../images/screenshots/standalone_scene.png -->

We'll cover the following steps:

* how to create an application,
* how to create a canvas,
* how to create a panel with an axes controller,
* how to add a visual,
* how to use colormaps,
* how to set visual data,
* how to run the application,
* how to create subplots,
* and more.

Creating custom visuals, creating standalone C applications with Datoviz are advanced topics, they are covered in the **How to** section of the documentation.


!!! note
    The Python bindings are at an early stage of development. They will be significantly improved in the near future.


## Opening IPython

Datoviz can be used in a Python script, or interactively in an IPython terminal. Datoviz allows for interactive use of a canvas while using the IPython terminal (integrated event loop integration). Jupyter notebook integration has not been tested or implemented yet.

In this tutorial, we'll use IPython. **Open an IPython terminal.**

```bash
$ ipython
```


## Importing the library

First, we import NumPy and datoviz:

```python
import numpy as np
import numpy.random as nr

from datoviz import canvas, run, colormap
```


## Enabling the IPython event loop integration

If using IPython interactively, we should enable the IPython event loop integration *after* importing datoviz:

```python
%gui datoviz
```

Otherwise we won't be able to use the IPython terminal while a canvas is open.


## Creating a canvas

We create a **canvas**:

```python
c = canvas(show_fps=False)
```

This should open a blank window. We can also specify the initial width and height of the window using keyword arguments to `canvas()`.



## Creating a panel

Next, we create a **panel**, which is another word for "subplot". By default, there is only one panel spanning the entire canvas, but we can also define multiple panels.

```python
panel = c.panel(controller='axes')
```



## Choosing one of the existing visuals

We'll make a simple **scatter plot** with 2D random points, and different colors and marker sizes.

We refer to [the list of all included visuals](../reference/visuals.md) provided by the Datoviz documentation, and we find that the **marker visual** is what we need for our scatter plot. We look at the **visual properties** (or **props**) for this visual: this is the data we'll need to feed to our visual.

But first, we create our visual object by specifying its type:

```python
visual = panel.visual('marker')
```



## Preparing the visual data

We'll set:

* the **marker positions**: `pos` prop,
* the **marker colors**: `color` prop,
* the **marker sizes**: `ms` prop.

First, we generate the data for this props.

### Random positions

```python
N = 100_000
pos = nr.randn(N, 3)
```

Note that **positions always have three dimensions** in Datoviz. When using 2D plotting, we can set the third component to zero.

Datoviz uses the standard OpenGL 3D coordinate system:

![Datoviz coordinate system](../images/cds.svg)
*Datoviz coordinate system*

!!! note
    Note that this is different from the Vulkan coordinate system, where y and z go in the opposite direction. The other difference is that in Datoviz, all axes range in the interval `[-1, +1]`. In the original Vulkan coordinate system, `z` goes from 0 to 1 instead. This convention makes it possible to use existing camera matrix routines implemented in the cglm library. The GPU code of all included shaders include the final OpenGL->Vulkan transformation right before the vertex shader output. Other conventions for `x, y, z` axes will be supported in the future.

Position props are specified in the original data coordinate system corresponding to the scientific data to be visualized. Yet, Datoviz requires vertex positions to be in normalized coordinates (between -1 and 1) when sent to the GPU. Since the GPU only deals with single-precision floating point numbers, doing data normalization on the GPU would result in significant loss of precision and would harm performance.

Therefore, Datoviz provides a system to make transformations on the CPU **in double precision** before uploading the data to the GPU. By default, the data is linearly transformed to fit the [-1, +1] cube. Other types of transformations will soon be implemented (polar coordinates, geographic coordinate systems, and so on).




### Random marker size

We define random marker sizes as an array of floating-point values:

```python
ms = nr.uniform(low=2, high=40, size=N)
```

### Colormap

Let's define the colors. We could use random RGB values for the colors, but we'll use a **colormap** instead.


```python
color = colormap(nr.rand(N), vmin=0, vmax=1, alpha=.75 * np.ones(N), cmap='viridis')
```

The variable `color` is an `(N, 4)` array of `uint8` (byte values between 0 and 255).

This line involves the following steps:

* Choosing a colormap, here **viridis** (see the [colormap reference page](../reference/colormaps.md) with the list of ~150 included colormaps),
* Defining an array of scalar values to be feed to the colormap (random values between 0 and 1 here),
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




## Adding another panel

Coming soon.

```python
```


## Specifying another controller type

Coming soon.



## Writing event callbacks


The canvas provides an event system where the user can specify callback functions to be called at specific times during the time course of the canvas.

An event callback may be registered as a sync or async callback.

* **Sync callbacks** are called directly when an event is raised by the library.
* **Async callbacks** are called in a background thread managed by the library. The events are pushed to a thread-safe FIFO queue, and they are consumed by the background thread.

It is recommended to only use async callbacks when needed, as they come with some overhead. They are mostly useful with I/O-bound operations, for example, making a network request in response to a keyboard key press.


#### Registering a callback function

The callback function receives two arguments: the canvas, and a special struct containing information about the event. The special field `u` is a union containing information specific to the event type.

=== "C"
    ```c
    void void callback(DvzCanvas* canvas, DvzEvent ev)
    {
        double time = ev.u.t.time;
        // ...
    }

    // ...

    dvz_event_callback(canvas, DVZ_EVENT_TIMER, 1, DVZ_EVENT_MODE_SYNC, callback, NULL);
    ```




## Adding a simple GUI

Datoviz integrates the [Dear ImGUI library](https://github.com/ocornut/imgui) which allows one to create GUIs directly in a Datoviz canvas, without using external dependencies such as Qt.

Coming soon.

```python
```
