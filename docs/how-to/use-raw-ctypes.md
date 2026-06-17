# Use Raw ctypes

Call the Datoviz shared library directly from Python when you need the low-level binding surface.

## Task Workflow

Load the binding, declare or use generated signatures, pass arrays with exact C-compatible dtypes,
and keep ownership identical to the C API.

## Minimal Call Sequence

```python
import numpy as np
import datoviz as dvz

scene = dvz.dvz_scene()
figure = dvz.dvz_figure(scene, 800, 600, 0)
panel = dvz.dvz_panel_full(figure)
```

Use the reference binding page for exact symbol names and type adapters.


## Important Details

Raw ctypes is not the high-level plotting layer. It is useful for smoke tests, integration glue, and
binding development.

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

    - Reference: [Python binding](../reference/ctypes.md)
    - Start page: [Quickstart](../start/quickstart.md)
    - Source: `examples/c/start/scatter.c`
