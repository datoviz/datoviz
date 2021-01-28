# Abstractions

Visky defines multiple abstractions that call for some definitions.


## App

The **App** is typically a singleton, the App provides an interface to the GPU and the canvases that may be created within the application.


## Canvas

The **Canvas** is a window displaying visuals, graphics, plots, user interfaces. It has a given width and height, which change when the user resizes the window. At the moment, Visky relies on the [**GLFW**](https://www.glfw.org/) cross-platform windowing library for creating and managing canvases.

The Canvas provides functionality to display graphics using the GPU via [**vklite**, the Vulkan wrapper provided by Visky](../expert/vklite.md).

### Offscreen canvas

Visky also supports **offscreen canvases**: they are typically not handled by the OS windows manager, they only exist in GPU memory. They may be used for automatic testing, video recording, automatic screenshot generation, GUI integration.


## Window

The **Window** is the GLFW object representing the window on screen.

The only difference with the Canvas is that the Canvas is handled by Visky whereas the Window is handled by GLFW. The Canvas provides intermediate-level Vulkan-related functionality.


## Scene

While the Canvas provides an interface to Vulkan functionality, the **Scene** provides an interface to plotting functionality. That includes subplots (called panels in Visky), panel insets, axes, grids, ticks, labels, colorbars, as well as high-level 2D/3D interactivity: panning and zooming, 3D arcball, cameras, and so on.

The Scene is the usual abstraction for most regular users, but the Canvas may also used for more complex applications (interactive animations, video games).


## Panel

The Scene is divided into a single or multiple **Panels** (or "subplots") of various sizes within a **grid** with a given number of rows and columns. Panels may span multiple rows and/or columns. The user can modify the relative size of the rows and columns. One can also add **panel insets** which do not fit within the grid but can be positioned anywhere in the scene.


## Controller

Each panel has an associated **Controller** which defines the way the user interacts with it. Built-in controllers include Panzoom, Axes 2D, Axes 3D, arcball, first-person camera, fly camera, and others.


## Viewport

The **Viewport** is an axis-aligned rectangular part of the canvas. It is defined by the _relative_ coordinates of its upper-left corner (two numbers between 0 and 1) and by its _relative_ width and height (also between 0 and 1). The viewport position and size are defined proportionally to the canvas size.


## Visual

Whereas all objects above relate to the _structure_ of the window, the [**visuals**](visuals.md) define its _contents_. Visky provides [many built-in visuals](visuals.md): markers, segments, paths, triangles, rectangles, polygons (patches), images, text, and so on. While almost all visuals may be used either in 2D or 3D, some of them are typically only used in 2D (images, rectangles...), while others are mostly used in 3D (meshes).

Visuals are added to the different panels.

### Batch rendering

Visuals are designed around the idea of **batch rendering**: for optimal GPU utilization, _similar objects should be rendered together_. Typically, objects are considered similar if they have a similar visual aspect and if they follow the same geometrical transformations. For example, all markers within a scatter plot should be rendered in the same batch: they look similar, and they should move similarly when panning and zooming.

Internally, visuals are rendered with a single draw command on the GPU, which uses a given type of primitive (point, line, triangle) and a given set of shaders. Although Vulkan supports much more complex rendering organizations, visuals provide a significant simplification while still allowing for high performance on the vast majority of use-cases.

**Minimizing the total number of visuals in a scene is the key to achieve optimal GPU performance.** For example, to display a set of discs, triangles, and rectangles in a given panel, one should use _a single Mesh visual_ and use the [mesh API](mesh.md) to create those different objects individually. They will be efficiently rendered in a single batch, even if they look like different disjoint objects.


## GUI

Visky uses the [Dear ImGUI C++ library](https://github.com/ocornut/imgui) to provide support for [simple or complex graphical user interfaces](gui.md) directly integrated within a Canvas, _without the need for massive dependencies like Qt_. These simple user interfaces offer further avenues for user interactivity beyond built-in controllers (inspired by libraries such as [NanoGUI](https://github.com/wjakob/nanogui)).

That being said, integration of a Visky canvas within GUI backends such as Qt is definitely possible, but still a work-in-progress (see `examples/backend_qt.cpp` for a simple example).

### Prompt

Visky also provides an easy way to quickly display a minimal GUI with a single text box at the bottom of a canvas, where the user can enter some arbitrary commands and press Enter. The text may be parsed by the application in an entirely custom way. This is an easy way to implement vim-like commands for power users.
