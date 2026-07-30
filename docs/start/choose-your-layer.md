# Choose your layer

Datoviz has several entry points because a Python analysis, native application, browser example,
and rendering backend need different levels of control. Start with the scene API unless your host
or task requires a lower layer.

The high-level plotting layer for the broader GSP/VisPy2 project is still work in progress. Datoviz
v0.4 is the rendering engine underneath that direction. Today, Python users who want to use Datoviz
directly use one generated `ctypes` binding: `import datoviz as dvz` is the normal call form with
documented NumPy array adaptation, and `datoviz.raw` is the exact pointer/count call form for the
same binding.

<div class="dvz-context-strip">
  <span>Scene API recommended</span>
  <span>Python and C</span>
  <span>Browser subset experimental</span>
  <span>Runtime layers unstable</span>
</div>

<div class="dvz-entry-map" aria-label="Datoviz integration layer decision map">
  <div class="dvz-entry-map__item dvz-entry-map__item--external">
    <span class="dvz-status-label dvz-status-label--external">external / work in progress</span>
    <strong>High-level plotting · GSP / VisPy2</strong>
    <span>Choose this layer when you need plotting semantics above the Datoviz engine.</span>
  </div>
  <div class="dvz-entry-map__item dvz-entry-map__item--primary">
    <span class="dvz-status-label dvz-status-label--supported">recommended · supported by feature</span>
    <strong>Scene API · Python with NumPy or native C/C++</strong>
    <span>Start here for retained scenes, visuals, controllers, windows, and offscreen output.</span>
  </div>
  <div class="dvz-entry-map__branches">
    <div class="dvz-entry-map__item dvz-entry-map__item--experimental">
      <span class="dvz-status-label dvz-status-label--experimental">experimental subset</span>
      <strong>Browser WebGPU</strong>
      <span>Use only manifest-promoted live example routes.</span>
    </div>
    <div class="dvz-entry-map__item dvz-entry-map__item--advanced">
      <span class="dvz-status-label dvz-status-label--advanced">advanced / unstable</span>
      <strong>App · canvas · stream · DRP2 · vklite</strong>
      <span>Use for hosting, runtime, backend, replay, or engine work—not as an ordinary application entry point.</span>
    </div>
  </div>
</div>

| Need | Use | Status |
| --- | --- | --- |
| Explore scientific arrays from Python | [`import datoviz as dvz`](../reference/ctypes.md#numpy-adapted-calls) | supported for documented calls |
| Build a native C or C++ application | [C `scene` and `app` APIs](first-c-program.md) | supported by feature |
| Render without a window or capture images | [Offscreen rendering](../how-to/render-offscreen.md) | supported feature |
| Embed a native view or own the event loop | [C/C++ integration](../how-to/c-integration.md) | supported by feature |
| Call the Python binding with explicit pointers and counts | [`datoviz.raw`](../reference/ctypes.md#exact-datovizraw-calls) | low-level exact call form |
| Run selected examples in the browser | [WebGPU/WASM example routes](../reference/webgpu-subset.md) | experimental subset |
| Work on advanced rendering, replay, or backend portability | [Advanced runtime APIs](../advanced/runtime-internals.md) | advanced/unstable |
| Use high-level scientific plotting | [GSP/VisPy2 scope](../reference/project-status.md) | external WIP layer |


## Python with NumPy

Choose the main Python package when you want to create scenes from Python and upload NumPy arrays to
visual attributes:

```python
import datoviz as dvz
```

This path keeps the `dvz_*` function names used by the C API and adapts documented data-upload calls
so they accept NumPy arrays. The `dvz.run(...)` helper manages the ordinary native-window session.
Use this path first when your analysis already lives in NumPy.

[Start with the Python-first Quickstart](quickstart.md).


## C or C++

Choose the C API when your application owns the window, rendering loop, offscreen target, capture
path, or integration with another native system. C provides the same scene and visual model as
Python, with explicit error handling and object destruction.

Most public C examples follow the same visible sequence:

1. create a scene, figure, and panel;
2. create a visual;
3. attach arrays to visual attributes such as positions, colors, sizes, or image data;
4. add the visual to a panel;
5. open a window or create an offscreen target;
6. run the app or capture a frame.

[Run the First C Program](first-c-program.md), then see
[C/C++ integration](../how-to/c-integration.md) for installed projects.


## WebGPU in the browser

Some examples have live browser routes using the experimental WebGPU/WASM subset. Use these routes
to inspect supported examples in a browser or test portability. Do not assume every native Vulkan
feature is available in WebGPU; each example and status page says whether browser support is live,
planned, deferred, or native-only.

[Browse examples](../examples/index.md) and check the
[WebGPU subset](../reference/webgpu-subset.md) before choosing a live route.


## Exact Python calls and advanced runtime layers

`datoviz.raw` is not a separate plotting API. It exposes exact generated C-shaped calls for cases
that need explicit pointers, counts, callbacks, or ABI diagnosis. Prefer the normal NumPy-adapted
package until a documented operation requires the exact form.

Use the advanced runtime pages when you are working on Datoviz internals, backend portability,
render replay, or specialized embedding. These pages are documented under
[Runtime internals](../advanced/runtime-internals.md) and are secondary to the user-facing scene API.

[Open the advanced runtime overview](../advanced/runtime-internals.md).


## High-level plotting

Datoviz v0.4 is not a high-level plotting library and does not restore the old Datoviz v0.3
Pythonic plotting API. Use GSP/VisPy2, when available, for higher-level scientific plotting. Use
Datoviz directly when you need the lower-level engine surface or while the high-level layer is still
being developed.

Check [project status](../reference/project-status.md) for the current `external/GSP` boundary. Do
not plan around a stable high-level package until that status changes.


## Next steps

Choose one action above and complete its first example:

- Python: [Quickstart](quickstart.md)
- C/C++: [First C Program](first-c-program.md)
- Browser: [Examples with WebGPU status](../examples/webgpu-matrix.md)
- Embedding or backend work: [Advanced](../advanced/index.md)
