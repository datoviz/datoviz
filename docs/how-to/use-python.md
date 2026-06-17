# Use from Python

Use the current low-level Python binding without assuming a v0.3 plotting API.

## Task Workflow

Use Python for smoke tests, integration experiments, and array-aware calls where the binding exposes
the needed C surface. For high-level scientific plotting, use the VisPy2/GSP layer when it is the
right tool instead of recreating it in Datoviz docs.

## Minimal Call Sequence

```python
import numpy as np
import datoviz as dvz

scene = dvz.dvz_scene()
figure = dvz.dvz_figure(scene, 800, 600, 0)
panel = dvz.dvz_panel_full(figure)
```

Adapt the C examples one call at a time. Keep NumPy array dtype and shape matched to the C
attribute contract.

The top-level `datoviz` module uses the generated array-aware facade for C-shaped `dvz_*` names.
Use `datoviz.raw` only when you need the exact generated `ctypes` layer.


## Important Details

The v0.4 Python surface is low-level and close to the C API. Do not expect the legacy v0.3 Pythonic
plotting interface to be present.

## Common Mistakes

- Passing default `float64` NumPy arrays where the C API expects `float32`.
- Passing non-contiguous arrays to raw pointer calls.
- Adding placeholder Python tabs when a Python path is not implemented.

## See Also

- [Use Python raw ctypes](use-raw-ctypes.md)
- [Use from C or C++](c-integration.md)
- [Choose a visual family](choose-a-visual-family.md)

??? example "Related examples"

    - Start page: [Quickstart](../start/quickstart.md)
    - Reference: [Python raw ctypes](../reference/ctypes.md)
    - Gallery: [Scatter Plot](../examples/gallery/start/start_scatter.md)
    - Source: `examples/c/start/scatter.c`
