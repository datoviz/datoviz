# Use the Exact Python Binding Call Form

Use `datoviz.raw` when Python code needs to call the generated `ctypes` binding almost exactly as C
would. This is an advanced integration path for the same Python binding. For ordinary scene code
with NumPy array adaptation, use `import datoviz as dvz` instead.

!!! info "At a glance"

    - **Status:** Supported advanced exact-call surface over the generated binding.
    - **Languages:** Python with explicit ctypes pointers/counts/callbacks.
    - **Prerequisites:** Knowledge of the matching C signature, ownership, dtype, layout, and lifetime contract.
    - **Result:** Calls use the public C ABI shape without NumPy count or string adaptation.

## Task workflow

Use the exact call form when you need generated signatures, explicit pointer/count arguments,
callback types, or binding diagnostics. Load `datoviz.raw`, pass C-compatible storage, and follow
the same destroy order as the C API.

For ordinary visual data uploads from NumPy, prefer the main `datoviz` package:

```python
import datoviz as dvz
```

Use `datoviz.raw` only when the exact C call shape matters:

```python
import datoviz.raw as raw
```

## Smallest handle check

This is a complete CPU-side handle/lifetime check, not a rendering example. It intentionally creates
no visual, app, or view.

```python
import datoviz.raw as raw

scene = raw.dvz_scene()
figure = raw.dvz_figure(scene, 800, 600, 0)
panel = raw.dvz_panel_full(figure)

raw.dvz_scene_destroy(scene)
```

This is only a handle-creation check. Use the reference binding page for exact symbol names,
pointer/count calls, callback types, and opaque handle behavior.

For visual attribute uploads, raw calls require explicit byte strings, pointers, and item counts.
This fragment creates the visual it uploads into, but omits the window/app code so the pointer
handling stays visible:

```python
import ctypes
import numpy as np
import datoviz.raw as raw

positions = np.array(
    [[-0.5, -0.5, 0.0], [0.5, -0.5, 0.0], [0.0, 0.5, 0.0]], dtype=np.float32
)
colors = np.array(
    [[255, 80, 80, 255], [80, 255, 160, 255], [80, 160, 255, 255]], dtype=np.uint8
)
diameters = np.full(positions.shape[0], 8.0, dtype=np.float32)

scene = raw.dvz_scene()
points = raw.dvz_point(scene, 0)
raw.dvz_visual_set_data(
    points,
    b"position",
    positions.ctypes.data_as(ctypes.c_void_p),
    positions.shape[0],
)
raw.dvz_visual_set_data(
    points,
    b"color",
    colors.ctypes.data_as(ctypes.c_void_p),
    colors.shape[0],
)
raw.dvz_visual_set_data(
    points,
    b"diameter_px",
    diameters.ctypes.data_as(ctypes.c_void_p),
    diameters.shape[0],
)

raw.dvz_scene_destroy(scene)
```

## Important details

The exact `ctypes` call form is useful for integration checks, low-level debugging, and cases where
the exact C call shape matters. It uses the same `dvz_*` function names and explicit destroy calls
as the C API.

`datoviz.raw` does not infer NumPy dtype, shape, contiguity, string encoding, byte sizes, or item
counts. Convert those values before the call.

Keep Python arrays, ctypes arrays, callback objects, and any other storage passed by pointer alive
for the whole C lifetime documented by the function. Many visual data uploads copy before returning,
but borrowed-storage APIs and callbacks may require a longer lifetime.

Destroy owner handles explicitly. A Python binding handle is not a Python object with semantic cleanup:

```python
app = raw.dvz_app(scene)
# Create a view and render frames here.
raw.dvz_app_destroy(app)
raw.dvz_scene_destroy(scene)
```

Raw function return values are not converted into Python exceptions. Check `NULL`, boolean, and
`DvzResult` values exactly as C code would before using an output or destroying an owner.

Do not import `datoviz._ctypes` from examples or application code. It is generated implementation
detail behind the public `datoviz.raw` module.

## Common mistakes

- Letting temporary NumPy arrays be freed before a raw call finishes.
- Passing wrong pointer types or item counts.
- Passing Python `str` where the raw signature expects `bytes`.
- Passing a non-contiguous view and assuming the pointer describes packed C storage.
- Relying on generated implementation modules instead of `datoviz.raw`.
- Treating Python object lifetime as a substitute for `dvz_app_destroy()` and
  `dvz_scene_destroy()`.

## See also

- [Use from Python](use-python.md)
- [Use from C or C++](c-integration.md)
- [Python API: exact raw calls](../reference/ctypes.md#exact-datovizraw-calls)
- [Diagnose build and platform issues](diagnose-platform.md)

??? example "Complete and related examples"

    - Canonical complete example: `examples/python/raw/offscreen_point.py`
    - Reference: [Python API: exact raw calls](../reference/ctypes.md#exact-datovizraw-calls)
    - Start page: [Quickstart](../start/quickstart.md)
    - Source: `examples/c/start/scatter.c`
