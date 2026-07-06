# Use raw ctypes from Python

Use `datoviz.raw` when Python code needs to call the exported C API almost exactly as C would. This
is an advanced integration path. For ordinary Python scene code with NumPy array adaptation, use
`import datoviz as dvz` instead.

## Task Workflow

Use raw ctypes when you need exact generated signatures, explicit pointer/count arguments, callback
types, or binding diagnostics. Load `datoviz.raw`, pass C-compatible storage, and follow the same
destroy order as the C API.

For ordinary visual data uploads from NumPy, prefer the main `datoviz` package:

```python
import datoviz as dvz
```

Use raw only when the exact C call shape matters:

```python
import datoviz.raw as raw
```

## Smallest Handle Check

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

## Important Details

Python raw `ctypes` is useful for integration checks, low-level debugging, and cases where the exact
C call shape matters. It uses the same `dvz_*` function names and explicit destroy calls as the C
API.

The raw binding does not infer NumPy dtype, shape, contiguity, string encoding, byte sizes, or item
counts. Convert those values before the call.

Keep Python arrays, ctypes arrays, callback objects, and any other storage passed by pointer alive
for the whole C lifetime documented by the function. Many visual data uploads copy before returning,
but borrowed-storage APIs and callbacks may require a longer lifetime.

Destroy owner handles explicitly. A raw Python handle is not a Python object with semantic cleanup:

```python
app = raw.dvz_app(scene)
# Create a view and render frames here.
raw.dvz_app_destroy(app)
raw.dvz_scene_destroy(scene)
```

Do not import `datoviz._ctypes` from examples or application code. It is generated implementation
detail behind the public `datoviz.raw` module.

## Common Mistakes

- Letting temporary NumPy arrays be freed before a raw call finishes.
- Passing wrong pointer types or item counts.
- Passing Python `str` where the raw signature expects `bytes`.
- Passing a non-contiguous view and assuming the pointer describes packed C storage.
- Relying on generated implementation modules instead of `datoviz.raw`.
- Treating Python object lifetime as a substitute for `dvz_app_destroy()` and
  `dvz_scene_destroy()`.

## See Also

- [Use from Python](use-python.md)
- [Use from C or C++](c-integration.md)
- [Python raw ctypes reference](../reference/ctypes.md)
- [Diagnose build and platform issues](diagnose-platform.md)

??? example "Related examples"

    - Reference: [Python raw ctypes](../reference/ctypes.md)
    - Start page: [Quickstart](../start/quickstart.md)
    - Source: `examples/c/start/scatter.c`
