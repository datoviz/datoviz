# Python API

Status: supported generated Python API with NumPy adaptation and an exact `ctypes` call form.

Datoviz has one generated Python binding with two access modes:

| Need | Import |
| --- | --- |
| Normal scene calls, NumPy uploads, and RGBA capture | `import datoviz as dvz` |
| Exact pointers, counts, byte sizes, callbacks, or ABI debugging | `import datoviz.raw as raw` |

Both modes preserve the C `dvz_*` function names. The top-level package adds policy-declared NumPy
adaptation; `datoviz.raw` exposes the exact C-shaped calls underneath it. Neither mode is the old
v0.3 plotting API. High-level plotting and object-oriented Python workflows belong to GSP/VisPy2.


## Quick Example

Create and destroy owner handles with the same lifecycle as C:

```python
import datoviz as dvz

scene = dvz.dvz_scene()
figure = dvz.dvz_figure(scene, 800, 600, 0)
panel = dvz.dvz_panel_full(figure)

# Add visuals, upload data, and create an app or offscreen view.

dvz.dvz_scene_destroy(scene)
```

Quickstart pages may use `dvz.run(scene, figure, title=...)` for brevity. The helper borrows the
retained scene and figure and owns only the app/window resources it creates. See
[Use from terminal IPython](../how-to/use-ipython.md) for the hosted-session lifecycle.


## NumPy-Adapted Calls

For ordinary visual uploads, pass C-contiguous NumPy arrays with explicit dtype and shape. The first
axis is the item count:

```python
import numpy as np
import datoviz as dvz

scene = dvz.dvz_scene()
figure = dvz.dvz_figure(scene, 800, 600, 0)
panel = dvz.dvz_panel_full(figure)
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

`dvz_visual_set_data_many()` checks that all arrays share the same item count. For slice updates
after a full allocation, use:

```python
first_item = 0
positions_chunk = np.array([[0.1, 0.0, 0.0]], dtype=np.float32)
dvz.dvz_visual_set_data_range(points, "position", first_item, positions_chunk)
```

The Python package may copy non-contiguous arrays for the duration of the call. Prefer
`np.asarray(data, dtype=..., order="C")` for predictable behavior. Adaptation is limited to the
pointer/count, pointer/byte-size, string, and capture relationships declared in binding policy;
unannotated calls may still require exact C-shaped arguments.

Adaptation does not validate every visual-family semantic. Check each call's `DvzResult`, and use
the visual-family reference for required dtype, shape, attribute name, and cardinality.

The facade has two explicit sampled-field helpers for common packed layouts:

```python
field = dvz.dvz_sampled_field_from_array(scene, values)
dvz.dvz_sampled_field_update_from_array(field, patch, offset=(x, y))
```

`dvz_sampled_field_from_array()` infers dimensions and row strides from C-contiguous forms:
`(height, width)` and `(depth, height, width)` scalar arrays, plus `(height, width, 4)` RGBA8.
Default format inference accepts `uint8` as R8 UNORM scalar, `uint32` as R32 UINT labels,
`float32` as R32 float scalar, and four-channel `uint8` as RGBA8 sRGB color. Non-contiguous input
is copied to contiguous storage, and Datoviz copies the upload before return.

Use the keyword overrides `format=`, `semantic=`, `color_role=`, and `dim=` only for a packed
layout compatible with the selected C field format. In particular, pass `dim=DVZ_FIELD_DIM_3D`
when a three-dimensional array has a final width of 1, 2, or 4 and would otherwise look like a 2D
channel array. `dvz_sampled_field_update_from_array()` uses the same inference; `offset` is in
sample coordinates and an explicit `extent` must equal the array-derived extent. General padded,
borrowed, or unsupported format layouts still require the exact descriptor/data-view API.


## Offscreen RGBA Capture

Create an app and offscreen view, render one frame, then capture into Python-owned memory:

```python
app = None
try:
    app = dvz.dvz_app(scene)
    if not app:
        raise RuntimeError("dvz_app() failed")

    view = dvz.dvz_view_offscreen(app, figure, 800, 600)
    if not view:
        raise RuntimeError("dvz_view_offscreen() failed")

    status = dvz.dvz_view_render_once(view)
    if status != dvz.DVZ_CANVAS_FRAME_READY:
        raise RuntimeError(f"render failed with status {status}")

    rgba = dvz.dvz_view_capture_rgba(view)
    assert rgba.shape == (600, 800, 4)
    assert rgba.dtype == np.uint8
finally:
    if app:
        dvz.dvz_app_destroy(app)
    dvz.dvz_scene_destroy(scene)
```

Captured pixels are tightly packed sRGB RGBA8 with straight alpha. Their shape is
`(framebuffer_height, framebuffer_width, 4)`, channel order is RGBA, and row `0` is the top row.
They are screenshot/export pixels, not scientific linear-float readback.


## Exact `datoviz.raw` Calls

Use the raw mode when a call requires explicit `ctypes` arguments:

```python
import datoviz.raw as raw

scene = raw.dvz_scene()
raw.dvz_scene_destroy(scene)
```

For example, the top-level call:

```python
dvz.dvz_visual_set_data(points, "position", positions)
```

maps to explicit pointer and count arguments:

```python
import ctypes
import numpy as np
import datoviz.raw as raw

positions = np.array(
    [[-0.5, -0.5, 0.0], [0.5, -0.5, 0.0], [0.0, 0.5, 0.0]], dtype=np.float32
)

raw.dvz_visual_set_data(
    points,
    b"position",
    positions.ctypes.data_as(ctypes.c_void_p),
    positions.shape[0],
)
```

Keep Python storage alive for the documented borrowed lifetime. `datoviz._ctypes` and
`datoviz._array_facade` are generated implementation details and must not be imported by examples
or applications.

Raw calls do not translate `NULL`, `false`, `DVZ_ERROR`, or status values into Python exceptions.
Check them explicitly before consuming outputs or continuing ownership-sensitive setup.


## Naming And Types

| C surface | Exact Python surface |
| --- | --- |
| `dvz_*` functions | `raw.dvz_*` functions |
| `Dvz*` structs and handles | generated `ctypes` classes or pointer-like handles |
| `DVZ_*` constants and enum values | generated constants or enum-compatible values |
| callback typedefs | generated `ctypes.CFUNCTYPE` types where supported |

Opaque handles remain opaque. Verified records may become `ctypes.Structure` or `ctypes.Union`
layouts; layout-sensitive records remain opaque until the generator has an explicit alignment
policy.


## Ownership And Lifetimes

A Python handle does not turn a C object into an automatically managed Python object. Unless a
specific function documents a narrower contract:

1. destroy owned objects with the matching C destroy function;
2. treat borrowed pointers and strings as temporary;
3. copy reused borrowed payloads before the next mutating call;
4. keep Python callback objects alive while they are registered with C;
5. keep arrays alive for any documented borrowed-storage lifetime.

Owned `char*` returns use `ctypes.c_void_p` so the original pointer is preserved. Cast it when a
generated destroy signature accepts `ctypes.c_char_p`:

```python
ptr = raw.dvz_scene_json(scene)
try:
    text = ctypes.string_at(ptr).decode("utf8") if ptr else None
finally:
    if ptr:
        raw.dvz_scene_json_destroy(ctypes.cast(ptr, ctypes.c_char_p))
```


## Generated Pipeline

The binding is generated from the exported public C API:

```text
public C headers -> build/bindings/datoviz_api.json
               -> datoviz/_ctypes.py -> datoviz.raw
               -> datoviz/_array_facade.py -> datoviz
```

Do not edit generated modules by hand. Binding policy lives under `spec/bindings/`; extraction and
generation tools live under `tools/bindings/`. Regenerate with:

```sh
just ctypes
```


## Examples

| Example | Purpose |
| --- | --- |
| `examples/python/direct/offscreen_point.py` | NumPy upload, offscreen render, and RGBA capture |
| `examples/python/raw/lifecycle.py` | exact-call import and scene lifecycle |
| `examples/python/raw/offscreen_point.py` | exact-call offscreen rendering |
| `examples/python/raw/async_click.py` | callback and host-helper smoke path |


## Limitations

- Datoviz v0.4 provides no prefixless plotting helpers such as `scatter()` or `imshow()`.
- Attribute names, dtypes, and shapes follow each visual family's C contract.
- Offscreen capture requires a usable native GPU/runtime context.
- PNG bytes are not yet exposed as an alpha-preserving Python memory helper.
- Callback and host-helper ergonomics remain low-level: keep `CFUNCTYPE` objects and user-data
  storage alive until explicit unregistration or owner destruction.

The optional `datoviz.experimental.cuda` module is packaged alongside the generated binding but is not part of its supported API contract. It loads CUDA lazily and reports unavailable platform/toolkit/provider conditions without making base `datoviz` imports depend on CUDA. See [GPU array interoperability](gpu-array-interop.md).


## Validation

Use the narrowest relevant command:

```sh
just ctypes
just ctypes-check
just ctypes-smoke
just ctypes-python-smoke
just ctypes-render-smoke
just ctypes-package-smoke
```

The full binding validation path is `just bindings`. Rendering smoke tests may skip graphics work
when runtime support is unavailable.


## See Also

- [Use Python](../how-to/use-python.md)
- [Use exact raw ctypes](../how-to/use-raw-ctypes.md)
- [Visual attributes](visual-attributes.md)
- [Objects and lifetimes](objects-and-lifetimes.md)
- [C API](c-api/index.md)
