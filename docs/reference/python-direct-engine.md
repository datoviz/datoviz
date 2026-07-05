# Python Direct-Engine Facade

Status: supported low-level Python entry point for direct Datoviz engine use.

Use this page when you want C-shaped Datoviz calls from Python with NumPy array adaptation. This is
not the legacy v0.3 Python plotting API and not a high-level object model. GSP/VisPy2 owns
Pythonic plotting and scientific workflow APIs above Datoviz.

| Need | Import |
| --- | --- |
| Direct-engine calls with NumPy arrays. | `import datoviz as dvz` |
| Exact pointer/count `ctypes` ABI calls. | `import datoviz.raw as raw` |
| High-level plotting, notebooks, or OO scene objects. | GSP/VisPy2, when available. |


## Retained Scene Basics

The top-level package preserves C names. Create and destroy owner handles with the same lifecycle as
C:

```python
import datoviz as dvz

scene = dvz.dvz_scene()
figure = dvz.dvz_figure(scene, 800, 600, 0)
panel = dvz.dvz_panel_full(figure)

# Add visuals, create an app/view, render frames.

dvz.dvz_scene_destroy(scene)
```

Use `datoviz.raw` only when a call needs exact `ctypes` pointer behavior. The generated
implementation modules, such as `datoviz._ctypes` and `datoviz._array_facade`, are not public import
surfaces.


## Dense Visual Data

For ordinary visual uploads, pass C-contiguous NumPy arrays with explicit dtype and shape. The first
axis is the item count.

```python
import numpy as np
import datoviz as dvz

points = dvz.dvz_point(scene, 0)

positions = np.array([[0.0, 0.0, 0.0]], dtype=np.float32)
colors = np.array([[255, 255, 255, 255]], dtype=np.uint8)
diameters = np.array([12.0], dtype=np.float32)

dvz.dvz_visual_set_data_many(
    points,
    {
        "position": positions,
        "color": colors,
        "diameter_px": diameters,
    },
)
dvz.dvz_panel_add_visual(panel, points, None)
```

`dvz_visual_set_data_many()` validates that all arrays share the same item count before calling the
raw C function. For slice updates after a full allocation, use:

```python
dvz.dvz_visual_set_data_range(points, "position", first_item, positions_chunk)
```

The facade may copy non-contiguous arrays for the duration of the call. Prefer preparing arrays with
`np.asarray(data, dtype=..., order="C")` for predictable behavior.


## Offscreen RGBA Capture

Create an app and offscreen view, render one frame, then capture the framebuffer into Python-owned
memory:

```python
app = dvz.dvz_app(scene)
view = dvz.dvz_view_offscreen(app, figure, 800, 600)

if dvz.dvz_view_render_once(view) != 0:
    raise RuntimeError("render failed")

rgba = dvz.dvz_view_capture_rgba(view)
assert rgba.shape == (600, 800, 4)
assert rgba.dtype == np.uint8

dvz.dvz_app_destroy(app)
dvz.dvz_scene_destroy(scene)
```

The returned pixels are screenshot/export pixels: tightly packed sRGB RGBA8 with straight alpha.
The array shape is `(framebuffer_height, framebuffer_width, 4)`, channel order is `RGBA`, and row
`0` is the top row of the captured image. The dimensions are framebuffer pixels, not logical view
pixels. These pixels are appropriate for display, screenshots, and backend integration tests, not
scientific linear-float readback.


## Example

A complete direct-engine smoke example lives at:

```text
examples/python/direct/offscreen_point.py
```

It creates a retained point visual with `dvz_visual_set_data_many()`, renders an offscreen frame, and
captures RGBA memory with `dvz_view_capture_rgba()`.


## Limitations

- No prefixless plotting helpers such as `scatter()` or `imshow()` are provided by Datoviz v0.4.
- Attribute names, dtypes, and shapes follow each visual family's C contract.
- Offscreen capture still needs a usable native GPU/runtime context.
- PNG bytes are not yet exposed as an alpha-preserving Python memory helper.


## See Also

- [Python raw ctypes](ctypes.md)
- [Visual attributes](visual-attributes.md)
- [Objects and lifetimes](objects-and-lifetimes.md)
- [Feature status](feature-status.md)
