# Debug Rendering Output

Narrow down blank frames, wrong colors, missing geometry, unexpected depth behavior, and screenshots
that differ from an interactive view.

Rendering bugs are easiest to diagnose when you separate scene construction from runtime execution.
First prove that the retained scene contains the expected figure, panel, visual, attributes, and
item counts. Only then inspect command streams, Vulkan/WebGPU behavior, screenshots, or platform
state.

If the project does not build, Python does not import, or no graphics device can be created, start
with [Diagnose build and platform issues](diagnose-platform.md). If only a browser route fails, use
[Diagnose WebGPU support](debug-webgpu.md).

## Diagnostic order

Use this order when a scene does not look right:

```text
canonical example
-> scene hierarchy
-> visual attachment
-> required attributes and counts
-> coordinate range and camera/domain
-> color, alpha, depth, and blending state
-> offscreen/native frame
-> DRP2/runtime diagnostics
```

Do not start with the backend unless the same minimal scene fails after the scene-level checks pass.
Most blank frames are caused by a missing visual attachment, zero items, an attribute shape mismatch,
transparent colors, a camera/domain mismatch, or depth/blend state hiding valid geometry.

## Start with a known-good baseline

Build and run a tiny scene before debugging a complex showcase:

```sh
just example-c features/basic_scene
./build/examples/c/features/basic_scene
```

If the interactive path is fragile on the current machine, use a fixed offscreen capture:

```sh
just example-c runtime/offscreen_capture
./build/examples/c/runtime/offscreen_capture
```

If the baseline fails too, switch to [Diagnose build and platform issues](diagnose-platform.md).
If the baseline works, reduce your scene until the smallest failing version remains.

## Verify the scene hierarchy

A renderable scene needs this chain:

```text
scene -> figure -> panel -> visual -> visual attributes -> panel attachment -> view -> frame
```

Check these facts before inspecting GPU logs:

| Check | Why it matters |
| --- | --- |
| A `DvzScene` exists for the lifetime of the app/view. | Destroying the scene too early invalidates retained objects. |
| The figure and panel are created before the view. | The view renders a figure; unattached objects are invisible. |
| Each visual is added to a panel with `dvz_panel_add_visual()`. | Creating a visual alone does not make it part of the panel. |
| Required visual attributes are set. | A visual with missing position, color, size, texture, or index data may emit no draw. |
| Attribute item counts match the visual contract. | Count mismatches can produce validation failures or partial draws. |
| The first frame actually renders. | Capture and readback before a frame usually reads stale or empty pixels. |

When in doubt, comment out all but one visual and make it large, centered, and opaque. A single
visible point, triangle, or image is a better diagnostic than a full scene with many moving parts.

## Check visual data

Common data-side causes of missing geometry:

| Symptom | First checks |
| --- | --- |
| Nothing appears. | Position/count attributes, panel attachment, coordinate range, color alpha, and camera/domain. |
| Some items are missing. | Attribute counts, item ranges, index buffers, clipping, NaNs, and negative or zero sizes. |
| Geometry is tiny or huge. | Data units, transform, controller domain, point/marker diameter units, and framebuffer size. |
| Mesh or primitive looks broken. | Vertex/index count, topology, winding, normals, and whether depth testing hides back faces. |
| Image or field is blank. | Texture dimensions, row stride, field format, color scale, sampler binding, and alpha. |

Use deterministic data while debugging. Fix the random seed, freeze animation state, and set explicit
camera/controller state. Avoid comparing screenshots from randomized data or time-dependent
callbacks.

## Check coordinates and cameras

If the visual is attached and has data, the next likely issue is that valid geometry is outside the
visible domain.

For 2D scenes:

- confirm that positions use the coordinate system expected by the panel and visual;
- set a known panzoom domain around the data bounds;
- avoid a first test where all points lie exactly on the clipping edge;
- check aspect ratio if the scene looks stretched or offset.

For 3D scenes:

- place a small test object at the origin;
- choose a camera distance and near/far range that contains the data;
- check whether model transforms or unit conversions moved the object behind the camera;
- disable complex lighting while proving geometry placement.

For screen-space visuals, labels, and overlays, verify whether positions are in data, view, panel,
or pixel space. A coordinate-space mismatch can make a valid visual appear far from the intended
panel.

## Check color, alpha, depth, and blending

Use a deliberately boring render state for the first proof:

- opaque colors with alpha `255`;
- a dark or light background that contrasts with the visual;
- one visual family at a time;
- depth testing disabled unless depth is the behavior under test;
- blending disabled unless transparency is the behavior under test.

Then add depth and blending back one feature at a time.

| Symptom | Likely cause | First action |
| --- | --- | --- |
| Frame is fully transparent. | Transparent visual colors or screenshot alpha expectations. | Render one opaque object on a contrasting background. |
| Far objects hide near objects. | Depth compare, camera range, or wrong coordinate convention. | Test with depth disabled, then restore depth with a simple scene. |
| Transparent objects look sorted incorrectly. | Blend mode, depth write, or draw order. | Reduce to two transparent objects and compare alpha modes. |
| Colors look washed out or too dark. | sRGB/linear expectation, color scale, or screenshot path. | Compare a solid known RGBA color before testing colormaps. |
| Only screenshots look wrong. | Capture timing, alpha handling, or framebuffer size. | Render once, check dimensions, then capture immediately. |

PNG screenshots are sRGB RGBA8 visual snapshots. They are not linear-float numeric readback.

## Remove window variables with offscreen rendering

Offscreen rendering is the preferred diagnostic when the scene should be static:

```sh
just example-c runtime/offscreen_capture
./build/examples/c/runtime/offscreen_capture
```

For your own scene, render one frame with `dvz_view_render_once()` before capture. Check the
framebuffer size with `dvz_view_framebuffer_size()` when pixel dimensions matter.

If offscreen rendering works but the interactive window does not, focus on native window, swapchain,
resize, controller, or event-loop behavior. If both fail in the same way, focus on scene data,
runtime command generation, or platform graphics support.

## Inspect runtime command flow

After scene-level checks pass, enable DRP2 trace output:

```sh
DVZ_DRP2_TRACE=normal ./build/examples/c/features/basic_scene
```

Use the fuller dump only when the normal trace does not show enough context:

```sh
DVZ_DRP2_TRACE=full DVZ_DRP2_TRACE_COLOR=0 ./build/examples/c/features/basic_scene
```

Trace output is useful for confirming that a frame emits resource creation, upload, render-pass,
pipeline, bind, draw, and presentation commands. It is not a substitute for checking the retained
scene first; an empty or malformed scene can still produce a technically valid but visually empty
frame.

## Compare native, offscreen, and browser paths

Keep these paths separate:

| Path | Use it to answer |
| --- | --- |
| Native interactive window | Does the desktop app path present and respond to input? |
| Native offscreen view | Does the scene render deterministically without visible window state? |
| Screenshot/capture | Does the exported visual snapshot match the rendered frame size and colors? |
| WebGPU live route | Does the promoted browser subset render this scene on this browser/adapter? |

A scene can be valid natively while its WebGPU route is unsupported, planned, or blocked by a
browser adapter. Use [Diagnose WebGPU support](debug-webgpu.md) for browser-specific failures.

## Triage by symptom

| Symptom | Most useful first step |
| --- | --- |
| Blank frame in every path. | Check panel attachment, required attributes, item counts, alpha, and camera/domain. |
| Blank native window, offscreen works. | Check window creation, swapchain/presentation, resize, and event loop. |
| Offscreen output has wrong dimensions. | Check the requested framebuffer size and view creation result. |
| Only one panel is wrong. | Check panel bounds, controller/domain, visual attachment, and render area. |
| Only one visual family is wrong. | Reduce to that visual's minimal gallery example and compare attributes. |
| Only complex examples fail. | Remove controllers, labels, colorbars, lighting, depth, blending, and animation in stages. |
| First frame is wrong, later frames recover. | Check upload timing, initial controller state, and first-frame callbacks. |
| Later frames degrade. | Check retained updates, item ranges, resize handling, and resource churn. |

## Report template

Use this shape when filing or handing off a rendering issue:

```text
Platform/OS:
GPU/driver:
Datoviz commit:
Native/offscreen/WebGPU:
Example or source file:
Framebuffer size:
Visual family:
Item count:
Controller/camera:
Depth/blend/alpha state:
Expected output:
Actual output:
Smallest reproducer:
Relevant logs or DRP2 trace:
```

Include a screenshot only after recording the exact example, size, data seed, and camera state.

## Common mistakes

- Debugging Vulkan before checking whether `dvz_panel_add_visual()` was called.
- Testing with transparent colors and no known opaque baseline.
- Comparing screenshots with random data or non-deterministic camera state.
- Capturing before rendering a frame.
- Assuming a WebGPU route exists because the native example works.
- Treating a depth/blending issue as a geometry upload issue before testing opaque geometry.

## Next steps

- [Render offscreen](render-offscreen.md)
- [Save screenshots](screenshots.md)
- [Control depth, blending, and transparency](depth-blending.md)
- [Diagnose WebGPU support](debug-webgpu.md)
- [Diagnose build and platform issues](diagnose-platform.md)
- [Errors and logging](../reference/errors-and-logging.md)

??? example "Related examples"

    - [Basic Scene](../examples/gallery/features/features_basic_scene.md) - Source: `examples/c/features/basic_scene.c`
    - [Depth Test Toggle](../examples/gallery/features/features_technique_depth_test.md) - Source: `examples/c/features/technique_depth_test.c`
    - [Offscreen Capture](../examples/gallery/runtime/runtime_offscreen_capture.md) - Source: `examples/c/runtime/offscreen_capture.c`
