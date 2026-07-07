# Choose Your Layer

Datoviz has several entry points because different users need different levels of control. Start
with the highest-level path that exists for your task, then move lower only when you need the extra
control.

The high-level plotting layer for the broader GSP/VisPy2 project is still work in progress. Datoviz
v0.4 is the rendering engine underneath that direction. Today, Python users who want to use Datoviz
directly use one generated `ctypes` binding: `import datoviz as dvz` is the normal call form with
documented NumPy array adaptation, and `datoviz.raw` is the exact pointer/count call form for the
same binding.

Most users should start with the Python or C scene API. Lower-level runtime layers are useful for
embedding, backend work, and contributors, but they are not the shortest path to a first
visualization.

| Need | Use | Status |
| --- | --- | --- |
| Write Python code with NumPy arrays | `import datoviz as dvz` | supported for documented calls |
| Build a native C or C++ application | C `scene` and `app` APIs | supported by feature |
| Render without a window, capture images, or integrate a native view | C `scene` and `app` APIs | supported by feature |
| Call the Python binding with explicit pointers and counts | `datoviz.raw` | low-level exact call form |
| Run selected examples in the browser | WebGPU/WASM example routes | experimental subset |
| Work on advanced rendering, replay, or backend portability | Advanced rendering/runtime APIs | advanced/unstable |
| Use high-level scientific plotting | GSP/VisPy2 when available | external WIP layer |


## Python With NumPy

Choose the main Python package when you want to create Datoviz scenes from Python and upload NumPy
arrays to visual attributes:

```python
import datoviz as dvz
```

This path keeps the same `dvz_*` function names as the C examples and adapts supported data-upload
calls so you can pass NumPy arrays directly. It is the best starting point for scientists who are
comfortable with Python but need lower-level rendering control than a plotting library gives.


## C Or C++

Choose the C API when your application owns the window, rendering loop, offscreen target, capture
path, or integration with another native system. The C examples show the supported native API
patterns for visual families, interaction, capture, and runtime behavior.

Most public C examples follow the same visible sequence:

1. create a scene, figure, and panel;
2. create a visual;
3. attach arrays to visual attributes such as positions, colors, sizes, or image data;
4. add the visual to a panel;
5. open a window or create an offscreen target;
6. run the app or capture a frame.


## WebGPU In The Browser

Some examples have live browser routes using the experimental WebGPU/WASM subset. Use these routes
to inspect supported examples in a browser or test portability. Do not assume every native Vulkan
feature is available in WebGPU; each example and status page says whether browser support is live,
planned, deferred, or native-only.


## Advanced Runtime Layers

Use the advanced runtime pages when you are working on Datoviz internals, backend portability,
render replay, or specialized embedding. These pages are documented under
[Runtime internals](../advanced/runtime-internals.md) and are secondary to the user-facing scene API.


## High-Level Plotting

Datoviz v0.4 is not a high-level plotting library and does not restore the old Datoviz v0.3
Pythonic plotting API. Use GSP/VisPy2, when available, for higher-level scientific plotting. Use
Datoviz directly when you need the lower-level engine surface or while the high-level layer is still
being developed.
