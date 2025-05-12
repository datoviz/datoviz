# Architecture

This document provides a high-level overview of the Datoviz v0.3 code architecture.

**⚠️ Warning:** Some architectural details described here are expected to change in Datoviz v0.4.

## Main components

The main components are:

* **vklite** (`include/datoviz/vklite.h`): A lightweight C wrapper around Vulkan, offering essential GPU compute and rendering functionality for scientific visualization.
* **Renderer** (`include/datoviz/renderer.h`): A C/C++ Vulkan-based visualization engine featuring a GLFW-based event loop that processes real-time visualization requests.
* **Requests** (`include/datoviz/requests.h`): A C API for generating real-time visualization requests to be handled by the renderer.
* **Visuals** (`includes/datoviz/scene/visuals/`): A comprehensive C/GLSL library of high-quality GPU graphical primitives—points, markers, paths, images, glyphs, meshes, and volumes.
* **Scene** (`include/datoviz.h`): A C/C++ library for scientific visualization logic that generates renderer-bound requests.

These components are structured around the **Datoviz Intermediate Protocol**: an **intermediate-level, message-based visualization protocol** that decouples high-level scientific logic from low-level Vulkan rendering.

This separation allows the high-level components to be developed and maintained by research software engineers and scientists, while the low-level GPU rendering code—requiring deep technical expertise more typical of the video game industry—can evolve independently.

The architecture also ensures that scientific visualization logic remains portable and insulated from changes in graphics APIs (OpenGL, Vulkan, Metal, DirectX, WebGPU, etc.). We are working toward porting Datoviz to non-Vulkan backends, including WebGPU-enabled web browsers.

This supports our long-term vision of making **high-performance GPU-based 2D/3D scientific visualization** broadly accessible across platforms—desktop, web, and remote/cloud environments—with support for both local and distributed data, and across multiple programming languages (C/C++, Python, Julia, Rust, etc.).


## Renderer

At the lowest level, Datoviz relies on the raw Vulkan C API, known for its extreme verbosity and complexity.

### vklite

We built **vklite**, a thin wrapper over the Vulkan C API, to expose the most essential GPU functionality in a more accessible form (`vklite.h`):

* **Device control and event loops:**
    * Device discovery (`dvz_gpu`)
    * Swapchain presentation (`dvz_swapchain`, `dvz_renderpass`, `dvz_framebuffers`, `dvz_surface`)
    * Synchronization primitives (`dvz_barrier`, `dvz_semaphores`, `dvz_fences`)
* **GPU memory:**
    * GPU data buffers (`dvz_buffers`)
    * GPU images (`dvz_images`)
    * Samplers (`dvz_sampler`)
* **Pipelines and shaders:**
    * Compute pipelines (`dvz_compute`)
    * Graphics pipelines (`dvz_graphics`) with fixed pipelines and custom shaders
    * Slots and descriptors (`dvz_slots`, `dvz_descriptors`)
* **Command buffers:**
    * Command buffer creation and submission (`dvz_commands`, `dvz_submit`)
    * Command buffer recording (`dvz_cmd`)

In broad terms, these low-level components allow you to:

- Create GPU-side objects such as data buffers and textures (images and samplers)
- Define compute and graphics pipelines via SPIR-V shaders (compiled from GLSL)
- Submit compute or rendering jobs asynchronously through recorded command buffers, which are processed in an event loop that handles on-screen rendering.

### GPU resources and data transfers

A significant part of the renderer is dedicated to managing GPU-resident data and facilitating **CPU-GPU data transfers** (`resources.h`, `context.h`, `alloc.h`, `transfers.h`, ...).

Vulkan and vklite support two main types of GPU data:

- **Buffers**: Linear, contiguous memory blocks used for vertex data, attributes, shader parameters, or general-purpose storage buffers accessible by shaders.
- **Images**: Used to store textures or rendered output (1D, 2D, or 3D).

Vulkan provides only low-level primitives for handling **CPU-GPU data transfers**—such as creating/deleting buffers and images, memory mapping, and recording/submitting copy commands. Datoviz builds on this by offering a dedicated data transfer engine with a simpler interface focused on "upload" (CPU → GPU), "download" (GPU → CPU), and "copy" (GPU → GPU) of both buffers and images.

To reduce overhead when dealing with small data chunks, Datoviz includes a custom **memory allocator**. Rather than creating numerous small buffers—which is inefficient in Vulkan—it allocates a few large shared buffers and manages sub-regions internally. This adds some complexity but significantly improves performance and scalability.

Datoviz exposes this abstraction through:
- **dats**: Contiguous data chunks stored on the GPU
- **texs**: GPU images for texture and render targets
These can be created, resized, deleted, and used for data uploads/downloads.

### Datoviz Intermediate Protocol Renderer

The **Renderer** executes requests defined by the Datoviz Intermediate Protocol using the low-level machinery described above (vklite, data transfer engine, etc.).

It maintains an object hash table that maps unique request-level object IDs to their corresponding low-level GPU representations.

### Client

Datoviz includes a GPU-agnostic, GLFW-based **client** responsible for basic interactive behavior: window creation, input handling, and event loop integration (`window.h`, `client.h`, `fifo.h`, `mouse.h`, `keyboard.h`, `input.h`, ...).

The client provides a thread-safe FIFO queue for communicating window and input events.

This GPU-agnostic client is linked to the renderer via the **presenter**, which dynamically processes incoming rendering requests and synchronizes them with the event loop.


## Datoviz Intermediate Protocol

The **Datoviz Intermediate Protocol** is fully defined in `datoviz_protocol.h`.

It provides a generic, intermediate-level GPU visualization interface that resembles the WebGPU API in spirit. This protocol operates **exclusively on GPU objects**, not on visual or graphical primitives. It offers no built-in visuals; instead, it supports arbitrary shaders and graphics pipelines. Graphical primitives and higher-level visualization constructs are implemented separately in the **Visuals** library and the **Scene API** (described below).

> *Note:* The protocol will be expanded in Datoviz v0.4 to support compute shaders, multiple rendering passes, and additional low-level features.

Requests are collected sequentially in a **batch**, which is submitted to the renderer for processing during the next iteration of the event loop.

The main objects and concepts defined in the Datoviz Intermediate Protocol include:

* **canvas** — A window tied to a Vulkan surface, used for real-time GPU rendering and presentation.
* **board** — An offscreen canvas for static rendering, independent of the event loop.
* **dat** — A chunk of GPU memory (implemented as a subregion of a larger buffer managed by Datoviz).
* **tex** — A GPU image (1D, 2D, or 3D).
* **sampler** — A GPU object that applies filtering (nearest or linear) to textures.
* **shader** — A SPIR-V shader module.
* **graphics** — A graphics pipeline, consisting of a fixed-state configuration, vertex shader, fragment shader, and other components.
* **bindings** — Descriptors linking `dats` to shader uniforms.
* **command buffer recording** — Viewport definitions and graphics pipeline draw commands (support for compute commands will be added later).

## Scene API

The Scene API provides higher-level scientific visualization constructs that are directly exposed to users of the Datoviz C API, unlike the lower-level internal components.

### Visuals API

A **Visual** is an abstraction representing a graphical object—or a collection of similar objects—that encapsulates a graphics pipeline. Each visual is defined by a pair of vertex and fragment shaders, descriptor bindings for uniforms, a vertex buffer, and visual-specific logic for **CPU-side data "baking"**.

Visuals are designed to handle collections of similar objects—points, markers, glyphs, images, meshes, etc.—which is key to **batch rendering**. Batch rendering of many objects of the same type (but with varying attributes such as position, color, or size) is essential for achieving high GPU performance.

Each visual uses a pair of custom shaders (vertex and fragment), originally developed by [Nicolas Rougier](https://www.labri.fr/perso/nrougier/) as part of his research in computer graphics. Rendering high-quality graphical primitives efficiently on the GPU remains an active area of research. Typically, the vertex shader requires the input data to be preprocessed in a way that suits GPU rendering—this preprocessing step is referred to as **baking**.

A visual consists of several layered abstractions:

* The **array** — A 1D data buffer on the CPU, with a specific data type (scalar or multi-component, floating-point or integer) and size.
* The **dual** — Links a CPU-side array with a GPU-side **dat** (a chunk of GPU memory). It manages synchronization and triggers data upload requests when the CPU-side array is modified.
* The **baker** — Generates one or more multiplexed vertex buffers from the user's original visual data (e.g. positions, colors, other attributes).
* The **generic visual** — A flexible mechanism for defining a visual by bundling together a graphics primitive, shaders, a baker, a vertex buffer, and optional uniform parameters.
* The **custom visual** — A specialized visual that builds on the generic visual to expose a dedicated API for specific visual types (e.g. marker, image, mesh). It includes functions to create the visual, set its data, and render it within a command buffer.


### Visuals library

Datoviz includes a built-in library of visuals commonly used in scientific visualization:

* **Basic visuals** rely on built-in OpenGL/Vulkan graphical primitives:
  * **Basic point** — A pixel or plain square.
  * **Basic line** — Aliased thin lines: line list, line strip.
  * **Basic triangle** — Aliased triangles: triangle list, triangle strip, triangle fan
    *(Note: triangle fan is not supported on macOS.)*

* **0D visuals** represent collections of points in 2D or 3D space:
  * **Pixel** — Collection of single pixels (position, color).
  * **Point** — Collection of discs (position, color, size).
  * **Marker** — Collection of markers with rich styling (position, color, size, angle, shape, fill/stroke/outline, edge color, edge width).
  * **Glyph** — Collection of text glyphs (position, color, size, angle, etc.).

* **1D visuals** represent lines:
  * **Segment** — Collection of rigid line segments (start/end positions, color, line width, cap style).
  * **Path** — Collection of curved paths with variable size (position, line width, color).

* **2D visuals** represent images:
  * **Image** — Collection of textured squares (corner positions, sampler filtering, texture image).

* **3D visuals** represent meshes or volume-like objects:
  * **Mesh** — Triangular meshes (vertex position, normal, color, texture coordinates, face indices, lighting parameters).
  * **Sphere** — Collection of "fake" spheres rendered as 2D sprites with 3D appearance (position, color, size, lighting).
  * **Volume** — Full 3D volume rendering.
  * **Slice** — 2D images embedded in 3D space, showing slices of a volume (corner positions, texture coordinates).


### Transforms

Datoviz includes a basic transform system, currently limited to standard model-view-projection linear transforms.
This is still a work in progress; support for more advanced transforms (e.g. nonlinear mappings) may be added in the future.

The system currently provides standalone components for common interaction modes:
- **Pan-zoom** (2D)
- **Arcball** (3D rotation)

Future extensions may include more subjective camera controls such as fly mode or first-person navigation.

### Scene

The **Scene** is the central public API in Datoviz, assembling all components into a hierarchical visualization structure:

* **Scene** — The root object of the system.
* **Figure** — A scene-aware window.
* **Panel** — A rectangular region within a Figure (i.e. subplot).
    * Each Panel has an associated **transform** and **interactivity mode** (e.g. panzoom, arcball).
    * One or more **visuals** can be added to a Panel.

The **viewset** is responsible for tracking all these objects and assembling a command buffer that renders all visuals in all panels.

### Specialized components

The Scene API includes several specialized components used internally by visuals or transforms.

* **Text components:**
    * **Font** — Wraps the [FreeType library](https://freetype.org/) to handle text composition using a built-in or custom TTF font.
    * **Atlas** — Wraps Viktor Chlumský’s [msdfgen-atlas library](https://github.com/Chlumsky/msdf-atlas-gen/) to generate a texture atlas of font glyphs using Multi-channel Signed Distance Fields (MSDF) for high-quality GPU text rendering in the fragment shader.

* **GUI components:**
    * **GUI** — Wraps Omar Cornut’s [Dear ImGui library](https://github.com/ocornut/imgui/) to provide basic interactive GUI elements. Full ImGui API support is expected to work in C++ mode (pending further testing).

* **Axis components:**
    * **Ticks** — Automatic tick positioning.
    * **Labels** — Tick label generation.
    * **Axis** — Manages a single axis with ticks and labels.
    * **Axes** — Manages multiple axes in a panel.
