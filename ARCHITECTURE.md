# Datoviz code architecture overview

This document provides a high-level overview of the Datoviz code architecture.


## Main components

The main components are the following:

* **vklite** (`include/datoviz/vklite.h`): a thin C Vulkan wrapper that provides the main GPU compute/visualization functionality required for scientific visualization.
* **Renderer** (`include/datoviz/renderer.h`): a C/C++, Vulkan-based GPU visualization engine that provides a glfw-based event loop receiving and processing visualization requests in real time.
* **Requests** (`include/datoviz/requests.h`): a C API that generates the visualization requests to be sent to the renderer in real time.
* **Visuals** (`includes/datoviz/scene/visuals/`): a rich C/GLSL library of common high-quality GPU graphical primitives such as points, markers, paths, images, glyphs, meshes, volumes...
* **Scene** (`include/datoviz.h`): a C/C++ library that provides scientific visualization functionality that generates visualization requests to be sent to the renderer.

These components are organized around the **Datoviz Intermediate Protocol**, an **intermediate-level message-based visualization protocol** that decouples the high-level scientific visualization logic from the low-level Vulkan rendering implementation.

While the former may be maintained and contributed by research software engineers and scientists, the former requires deep technical expertise that is more common within the video game industry and among game engine developers than in science.

Another benefit of this architecture is that it ensures the high-level scientific visualization logic can be developed independently from the constant innovations in graphics hardware and graphics APIs (OpenGL, Vulkan, Metal, DirectX, WebGPU, wgpu...). In particular, thanks to a Chan Zuckerberg Initiative (CZI) grant attributed to the VisPy project in 2024, this architecture will allow us in the next couple of years to port the Datoviz technology to non-Vulkan environments, such as WebGPU-enabled web browsers.

This will help us achieve a long-term vision where **high-performance GPU-based 2D/3D scientific visualization** will be available in local and remote multi-platform environments (distributed rendering, web-based visualization), working with either local or cloud-based data, with language-agnostic visualization code (C/C++, Python, Julia, Rust...).


## Renderer

The bottomest layer is the raw Vulkan C API, which is known for its extreme verbosity and complexity.

### vklite

We built vklite, a thin wrapper on top of the Vulkan C API, that provides the most essential Vulkan functionality (`vklite.h`):

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
  * Graphics pipelines (`dvz_graphics`) with fixed pipeline and custom shaders
  * Slots and bindings or descriptors (`dvz_slots`, `dvz_descriptors`)
* **Command buffers:**
  * Command buffer creation and submission (`dvz_commands`, `dvz_submit`)
  * Command buffer recording (`dvz_cmd`)

Broadly speaking, these low-level functions allow one to create objects on the GPU, mostly data buffers and textures (images and samplers), to define GPU compute and graphics pipelines via custom SPIR-V shaders (compiled from GLSL), and to send compute and rendering jobs to the GPU by asynchronously submitting recorded command buffers to a dedicated event loop displaying graphics on screen.

### GPU resources and data transfers

A major part of the renderer is device management of GPU-stored data and CPU-GPU **data transfers** (`resources.h`, `context.h`, `alloc.h`, `transfers.h`...).

Vulkan and vklite support two main types of data: **buffers** (linear contiguous blocks of memory) and **images** (1D, 2D, or 3D).

**Buffers** store vertex positions and other attributes, as well as shader parameters. Arbitrary GPU buffers may also be created for direct access from the shaders (storage buffers).

**Images** typically store textures or rendered images.

Vulkan only provides low-level primitives for **CPU-GPU data transfers** (creation and deletion of buffers and images, memory mapping techniques, submission of command buffers with buffer/image copy operations...). Datoviz provides a dedicated data transfer engine that provides a simpler interface that mostly deals with "upload" (CPU->GPU), "download" (GPU->CPU), and "copy" (GPU->GPU) of buffers and images.

Datoviz also implements a custom **memory allocator** that avoids the overhead of creating and managing a large number of GPU buffers when handling small amounts of data. It is indeed good practice in Vulkan to define a few large buffers of different types and manually handle chunks of data in these buffers, although this comes with a somewhat increased internal complexity.

Datoviz simply defines **dats** (chunks of data on the GPU) and **texs** (images on the GPU) that can be created, resized, deleted, and to which one can upload/download data.


### Datoviz Intermediate Protocol Renderer

The **Renderer** processes Datoviz Intermediate Protocol requests using the low-level components discussed earlier (vklite, data transfers...).

It implements an object hash table mapping unique IDs identifiying each object created in the requests, to the actual underlying objects.


### Client

Datoviz implements a GPU-agnostic glfw-based **client** providing basic interactive event loop functionality: window creation and management, mouse and keyboard interactivity (`window.h`, `client.h`, `fifo.h`, `mouse.h`, `keyboard.h`, `input.h`...).

The client provides a thread-safe FIFO queue on which window and input events are sent.

The GPU-agnostic client and the client-agnostic (static) renderer are linked together via the **presenter**, which allows dynamic processing of the incoming requests by the renderer and appropriate event loop synchronization.


## Datoviz Intermediate Protocol

The **Datoviz Intermediate Protocol** is entirely defined in `request.h`.

It provides a generic intermediate-level GPU visualization library that is somewhat similar to the WebGPU API. It deals exclusively with **GPU objects**, NOT visual objects. The protocol comes with NO graphical primitives, it supports arbitrary shaders and graphics pipelines. Graphical primitives and higher-level visualization constructs are implemented in the Visuals library and the Scene API described below

> *Note*: although compute shaders are already mostly supported in the renderer, they are not yet implemented in the Datoviz Intermediate Protocol. They will be in the future, depending on user feedback. This should be fairly straightforward (adding functions in `requests.h`, implementing them in `renderer.cpp`, writing tests and documentation...).

Requests are linearly collected in a **batch**, which is then sent to the renderer for dynamic processing at the next event loop iteration.

The main objects and notions defined in the Datoviz Intermediate Protocol are:

* **canvas**: a window associated to a Vulkan surface for GPU rendering and presentation
* **board**: an offscreen canvas for static rendering independently from the event loop
* **dat**: a GPU memory buffer (actually implemented as a data chunk of a larger Datoviz-handled buffer)
* **tex**: a GPU image buffer (1D, 2D, or 3D)
* **sampler**: a GPU object providing nearest or linear filtering on a tex
* **shader**: a SPIR-V shader
* **graphics**: a graphics pipeline defined by a fixed state pipeline, a vertex shader, a fragment shader...
* **bindings**: bindings between dats and shader uniforms
* **command buffer recording**: essentially viewport definition and graphics pipeline drawing commands (compute buffers to come later)


## Scene API

The Scene API provides higher-level scientific visualization constructs that are directly exposed to users of the Datoviz C API (contrary to the underlying machinery).


### Visuals API

A **Visual** is an abstraction representing a graphical object, or a collection of similar objects, and encapsulating a graphics pipeline defined by pair of vertex and fragment shaders, along with descriptor bindings for uniform buffers, an associated vertex buffer, and custom visual-specific logic for CPU-based data "baking".

Visuals typically support natively collections of objects: points, markers, glyphs, images, meshes, and so on (see the Visuals library below for more details). This is a crucially important notion: **batch-rendering** of many objects of the same type, but with various data attributes (positions, colors, sizes...), is the key to achieving high-performance rendering on the GPU.

Each visual comes with a pair of custom shaders (vertex shader and fragment shader), originally contributed by [Nicolas Rougier](https://www.labri.fr/perso/nrougier/)'s research in computer graphics (rendering high-quality visual primitives efficiently on the GPU is an active area of research). The vertex shader typically requires preprocessing of the data to make it amenable to GPU rendering: this is the so-called **"baking"** of the data.

A visual involves several consecutive abstractions:

* The **array** holds a 1D data buffer on the CPU. It has a data type (scalar or multiple components, floating-point or integer) and a size.
* The **dual** associates a CPU-based array with a GPU-based **dat** (chunk of a GPU buffer). It handles synchronization between the two and emits a data upload request when the CPU part of the array changes.
* The **baker** provides facilities for generating one (or several) multiplexed vertex buffer(s) from the original user-supplied visual data (positions, colors, other attributes).
* The **generic visual** provides facilities for creating a visual bundling together a graphics primitive, shaders, a baker, a vertex buffer, optional uniform-based parameters.
* The **custom visual** leverages the generic visual to define a dedicated visual API to create a specific visual (marker, image, mesh...), set its data, and render it when rendering a command buffer.


### Visuals library

Datoviz comes with a built-in library of visuals commonly used in scientific rendering:

* **Basic visuals** rely on the built-in OpenGL/Vulkan graphical primitives:
  * **Basic point**: pixel or plain square
  * **Basic line**: aliased thin lines: line list, line strip
  * **Basic triangle**: aliased triangles: triangle list, triangle strip, triangle fan (warning: not supported on macOS)

* **0D visuals** represent points in a 2D or 3D space:
    * **Pixel**: collection of pixels (position, color)
    * **Point**: collection of discs (position, color, size)
    * **Marker**: collection of markers (position, color, size, angle, shape, filled/stroke/outline, edge color, edge width)
    * **Glyph**: collection of text glyphs (position, color, size, angle...)

* **1D visuals** represent lines:
    * **Segment**: collection of rigid line segments (initial and terminal position, color, line width, cap type)
    * **Path**: collection of variable-size curved paths (position, line width, color)

* **2D visuals** represent images:
    * **Image**: collection of textured squares (corner positions, sampler filtering, texture image)

* **3D visuals** represent meshes (or "fake" meshes, volumes):
    * **Mesh**: triangular meshes (vertex position, normal, color, texture coordinates, face indices, light position and parameters)
    * **Fake sphere**: collection of "fake" spheres rendered as 2D sprites with 3D illusion (position, color, size, light position)
    * **Volume**: volume rendering
    * **Slice**: images in 3D space showing slices of a 3D volume (corner position, texture coordinates)


### Transforms

Datoviz provides a basic transform system, currently limited to standard matrix-view-projection linear transforms (this is still a work in progress, support for more complex, e.g. nonlinear, transforms may be implemented later).

The system provides standalone components implementing pan-zoom (2D) and arcball (3D rotations) interactivity. Future components may implement a subjective camera (e.g. fly mode, FPS-type camera).


### Scene

The Scene puts all components together in the main public API of Datoviz:

* The **Scene** is the root object.
* The **Figure** represents a scene-aware window.
* The **Panel** represents a full or partial rectangular portion of a Figure (subplot).
* The Panel is specified a **transform** and **interactivity mode** (panzoom, arcball).
* One or several **visuals** are added to a Panel.

The **viewset** takes care of tracking all of these objects and building a command buffer for drawing all visuals in all panels.


### Specialized components

The Scene API also comes with a set of specialized components that are used by some visuals or transforms.

* **Text components**:
    * The **Font** wraps the [freetype library](https://freetype.org/) to handle text composition on the basis of a builtin or custom TTF font.
    * The **Atlas** wraps
Viktor Chlumský's [msdfgen-atlas library](https://github.com/Chlumsky/msdf-atlas-gen/) to generate an atlas texture with multi-channel signed distance field (MSDF) representing font glyphs to be rendered on the GPU in the fragment shader.

* **GUI components**:
    * The **GUI** wraps [Omar Cornut's Dear ImGui library](https://github.com/ocornut/imgui/) to provide basic interactive GUI components. It should be straightforward to support the entire Dear ImGui API when using Datoviz in C++ (testing required).

* **Axis components**: (*note*: work in progress)
    * **Ticks**: implement an automatic tick positioning system (extended Wilkinson algorithm)
    * **Labels**: generate tick labels
    * **Axis**: handle an axis with ticks and labels
    * **Axes**: handle multiple Axis components
