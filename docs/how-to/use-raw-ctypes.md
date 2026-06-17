# Use Python Raw ctypes

Call the Datoviz shared library directly from Python when you need the low-level binding surface.
For normal direct-engine Python calls with NumPy array adaptation, use `import datoviz as dvz`
instead.

## Task Workflow

Load the binding, declare or use generated signatures, pass arrays with exact C-compatible dtypes,
and keep ownership identical to the C API.

## Minimal Call Sequence

```python
import datoviz.raw as raw

scene = raw.dvz_scene()
figure = raw.dvz_figure(scene, 800, 600, 0)
panel = raw.dvz_panel_full(figure)

raw.dvz_scene_destroy(scene)
```

Use the reference binding page for exact symbol names, pointer/count calls, and type adapters.


## Important Details

Python raw `ctypes` is not the high-level plotting layer. It is useful for smoke tests, integration
glue, and binding development. It preserves C-shaped names and C ownership rules.

## Common Mistakes

- Letting temporary NumPy arrays be freed before a raw call finishes.
- Passing wrong pointer types or item counts.
- Treating Python object lifetime as a substitute for `dvz_app_destroy()` and
  `dvz_scene_destroy()`.

## See Also

- [Use from Python](use-python.md)
- [Use from C or C++](c-integration.md)
- [Diagnose build and platform issues](diagnose-platform.md)

??? example "Related examples"

    - Reference: [Python raw ctypes](../reference/ctypes.md)
    - Start page: [Quickstart](../start/quickstart.md)
    - Source: `examples/c/start/scatter.c`
