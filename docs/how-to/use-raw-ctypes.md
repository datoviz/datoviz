# Use Python Raw ctypes

Call the Datoviz shared library directly from Python when you need the low-level binding surface.
For normal direct-engine Python calls with NumPy array adaptation, use `import datoviz as dvz`
instead.

## Task Workflow

Use raw ctypes when you need exact generated signatures, explicit pointer/count arguments, callback
types, ABI behavior, or binding-generation diagnostics. Load `datoviz.raw`, pass C-compatible
storage, and keep ownership identical to the C API.

For ordinary visual data uploads from NumPy, prefer the top-level array-aware facade:

```python
import datoviz as dvz
```

Use raw only when the exact C call shape matters:

```python
import datoviz.raw as raw
```

## Minimal Call Sequence

```python
import datoviz.raw as raw

scene = raw.dvz_scene()
figure = raw.dvz_figure(scene, 800, 600, 0)
panel = raw.dvz_panel_full(figure)

raw.dvz_scene_destroy(scene)
```

Use the reference binding page for exact symbol names, pointer/count calls, callback types, and
opaque handle behavior.

For visual attribute uploads, raw calls require explicit byte strings, pointers, and item counts:

```python
import ctypes
import numpy as np
import datoviz.raw as raw

positions = np.asarray(positions, dtype=np.float32, order="C")
colors = np.asarray(colors, dtype=np.uint8, order="C")

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
```

## Important Details

Python raw `ctypes` is not the high-level plotting layer. It is useful for smoke tests, integration
glue, and binding development. It preserves C-shaped names and C ownership rules.

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
