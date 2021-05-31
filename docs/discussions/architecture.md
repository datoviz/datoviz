# Library architecture

This page provides an overview of the internal architecture of Datoviz.


## Overview

Datoviz is made of several modules that are relatively loosely-coupled. They can be classified into three broad categories:

* **Core modules** implement low-level functionality:
    * **vklite** is a thin Vulkan wrapper,
    * **canvas** implements a blank canvas providing rendering capabilities via vklite.
* **High-level modules** implement scientific visualization functionality:
    * **graphics** provides a library of builtin primitive graphics (markers, paths, images, text, meshes, volumes, and so on),
    * **visuals** wrap primitive graphics into higher-level ready-to-use visual elements,
    * **panels** provide a way to manage multiple rectangular subplots in a 2D grid,
    * **interacts** provide a library of common interactivity patterns (panzoom, arcball, camera),
    * **GUI** provides a way to use the Dear ImGui library,
    * **scene** brings all the modules above together.
* **Independent modules** provide useful utility functionality to the other modules.


## Core modules

These modules are essentially used internally.

## Vklite

**Vklite** is a thin wrapper on top of the Vulkan C API. It proposes a simpler, but more limited API. Only the features that are essential to scientific visualization are wrapped. The vklite API happens to be relatively close to the WebGPU specification, but it is Vulkan-specific, whereas WebGPU is compatible with other low-level graphics APIs (Metal, DirectX 12...).


## Context

The **context** is attached to a given GPU. It is an interface to the GPU data, that can be either:

* **GPU buffers**: 1D arrays containing arbitrary binary data,
* **GPU textures**: 1D, 2D, or 3D textures.

**Buffer regions** represent a part of a given GPU buffer. They are used extensively in Datoviz, as there is only one large buffer of every type (one vertex buffer, one uniform buffer) for all visuals and all canvases. A simplistic memory allocation system allows to allocate buffer regions depending on the needs of the Datoviz objects (graphics, visuals...).

The context is also used to define **compute shaders** on a given GPU.

The context is shared between all canvases running on a given GPU.

The context provides functions to upload, download, and copy data of GPU buffers and textures.


## Canvas

The **canvas** is a Vulkan-aware window with the following functionality:

* **swapchain** and default renderpass, command buffers, support for window resizing,
* **main rendering event loop**,
* **synchronized data transfers**,
* **event system** with synchronous or asynchronous (background thread) callback functions,
* **mouse and keyboard events**,
* **support for Dear ImGui**,
* **video screencast** with ffmpeg (optional).

If there are multiple GPUs on the system, a canvas is attached to a given GPU.


## High-level modules

The high-level modules are key components of the scene interface.

### Graphics

A **graphics** pipeline wraps a vertex shader, a fragment shader, fixed GPU state, and possibly other shaders. A graphics is attached to a given canvas.

Datoviz includes a set of common builtin graphics pipelines frequently used in scientific visualization.

All graphics share a set of conventions that make it possible for the visuals and scene modules to use them properly:

* Every shader has `#include "common.glsl"`, which is a way to share GLSL code between all shaders.
* **Binding point #0** of every shader is a uniform buffer containing the **model-view-projection matrices**.
* **Binding point #1** of every shader is a uniform buffer containing the **viewport**.
* In the vertex shader, the vertex position is transformed with the `transform()` function implemented in `common.glsl`. This function basically applies the model-view-projection matrices to its input.

!!! note
    This set of conventions may change slightly in the future.


### Visual

A **visual** represents a visual element. It is based on one or several graphics pipelines, and possibly compute shaders. It is attached to a given canvas and therefore to a given GPU.

A visual provides a set of visual properties, aka **props**. Props are used to set the visual data. For example, the `marker` visual has props for marker position, size, color, shape, angle, and so on. The visual is responsible for transforming the user data into a vertex buffer corresponding to the vertex attributes of the underlying graphics pipeline(s).

The visual module provides the machinery to automatically update GPU objects when the visual data changes.


### Panel

A **panel** is a rectangular area in a canvas. It defines a specific viewport. The **grid** arranges panels in a 2D, possibly heterogeneous, grid layout.


### Interact

An **interact** provides functions that update model-view-projection matrices as a function of user events (mouse, keyboard).

This module implements a few common interacts: panzoom, arcball, camera.


### GUI

A **GUI** is a dialog that can be added to any canvas. It contains a set of **controls**. Datoviz implements a few common GUI controls that wrap Dear ImGUI functionality in an object-oriented interface.


### Scene

The **scene** attaches visual, panel, and interact functionality to a canvas. It allows the user to easily define panels, specify interacts, add a visual, change visual data, and so on.

A **controller** bundles together an interact and, optionally, a set of visuals. For example, the **panzoom** controller is just a panzoom interact. However, the **axes 2D** controller has also an axes visual.


## Independent modules

Independent modules provide useful utility functions.

### Colormaps

This module implements a [library of ~150 common colormaps](../reference/colormaps.md) coming from existing scientific visualization libraries (MATLAB, matplotlib, and others).


### Array

This module implements an extremely basic and limited 1D array interface that is used by the visual baking functionality. Future versions of Datoviz might instead incorporate a proper C/C++ array module.


### FIFO

This module implements thread-safe FIFO queues that are used by the canvas event system and other systems internally.
