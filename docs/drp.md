# Datoviz Rendering Protocol

The main Datoviz API is the scene API.
It offers tools to create built-in visuals, manage interactivity controllers, and organize them within a figure.

The scene API is built on top of a lower-level API, the **Datoviz Rendering Protocol (DRP)**.
This API, similar to WebGPU, provides an asynchronous interface for creating GPU objects such as shaders, graphics pipelines, data buffers, and textures, as well as for generating command buffers.
Although currently implemented for desktop use, it can also be adapted for distributed environments.

The DRP is defined in the `datoviz_protocol.h` header file and documented in the [API reference](api.md#datoviz-rendering-protocol-functions).
A "hello world" example can be found in `examples/drp.c` and `examples/drp.py`.

Using DRP, you typically generate requests, group them into a batch, and a separate renderer processes these pending requests during the event loop.
The architecture is designed with multithreading in mind, though this is not yet officially supported.
Requests can be issued at initialization or in response to user events, such as mouse and keyboard inputs or timers.

## Usage example

```c
#include <datoviz_protocol.h>

// ...

// Create an application (managing the event loop, renderer, and GPU access).
DvzApp* app = dvz_app(0);

// Retrieve the batch which all pending requests until they are processed by the app's renderer.
DvzBatch* batch = dvz_app_batch(app);

// Create requests here.

// Start the event loop. The second argument is the number of frames in the event loop.
// The application stops afterwards, except if this number is 0, corresponding to an infinite loop.
dvz_app_run(app, 0);
dvz_app_destroy(app);
```

## Types of requests

The [API documentation](api.md#datoviz-rendering-protocol-functions) lists all functions that can be used to create requests.
They all take the batch as a first argument.

### Object creation/deletion

These functions, that start with `dvz_create_`, are used to create/delete:

* a board (an offscreen canvas)
* a canvas (a canvas on screen)
* a dat (GPU data buffer)
* a tex (GPU 1D, 2D, or 3D texture)
* a sampler (shader access to images)
* a shader, either in GLSL (will be compiled by Datoviz via libshaderc) or directly in SPIU-V
* a graphics pipeline

(Support for compute shaders coming soon).

### Resizing

These functions, that start with `dvz_resize_`, are used to resize:

* a board
* a dat
* a tex

### Uploading

These functions, that start with `dvz_upload_`, are used to upload to the GPU:

* a dat
* a tex

### Graphics pipelines

These functions, that start with `dvz_set_`, are used to specify the fixed state function of graphics pipelines and other attributes:

* the primitive topology
* the blend mode
* the depth test
* the polygon mode
* the cull mode
* the front face
* the shaders
* the vertex bindings
* the vertex attributes
* the binding dslots (descriptors)
* the specialization constants.

### Bindings

These functions, that start with `dvz_bind_`, are used to bind to graphics pipelines:

* a vertex buffer
* an index buffer
* a dat as uniform or storage buffer
* a tex and sampler as a texture

### Command buffer recording

These functions, that start with `dvz_record_`, are used to record in the a canvas' command buffer the following commands:

* begin a command buffer
* set a viewport for the next drawing calls
* draw a graphics pipeline (indexed or not, direct or indirect)
* end a command buffer

Support for more complex command buffers will be implemented later (in particular, compute pipelines, synchronization mechanisms, multipass rendering).
