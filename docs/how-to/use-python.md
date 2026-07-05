# Use from Python

Use the current low-level Python binding without assuming a v0.3 plotting API.

## Task Workflow

Use Python for smoke tests, integration experiments, and low-level v0.4 scene code. Prefer
`import datoviz as dvz` for normal Python use: it follows the same `dvz_*` function names as the C
examples and accepts NumPy arrays for supported visual-data uploads.

Choose the import surface first:

| Need | Import |
| --- | --- |
| Scene and visual calls with NumPy array adaptation. | `import datoviz as dvz` |
| Exact generated `ctypes` signatures, pointers, counts, and ABI behavior. | `import datoviz.raw as raw` |
| High-level object-oriented plotting. | Use the external GSP/VisPy2 layer when available. |

## Minimal Call Sequence

```python
import numpy as np
import datoviz as dvz

scene = dvz.dvz_scene()
figure = dvz.dvz_figure(scene, 800, 600, 0)
panel = dvz.dvz_panel_full(figure)

points = dvz.dvz_point(scene, 0)
positions = np.asarray(positions, dtype=np.float32, order="C")
colors = np.asarray(colors, dtype=np.uint8, order="C")
diameters = np.asarray(diameters, dtype=np.float32, order="C")

dvz.dvz_visual_set_data(points, "position", positions)
dvz.dvz_visual_set_data(points, "color", colors)
dvz.dvz_visual_set_data(points, "diameter_px", diameters)
dvz.dvz_panel_add_visual(panel, points, None)
```

Adapt the C examples one call at a time. Keep NumPy array dtype and shape matched to the C
attribute contract.

The top-level `datoviz` module accepts NumPy arrays for the calls covered by the binding policy. Use
`datoviz.raw` only when you need the exact generated `ctypes` layer. Calls that are not covered by
the policy may still expect explicit pointer/count arguments, so consult the C and raw ctypes
references when needed.

Destroy owner handles with the same C lifecycle calls:

```python
app = dvz.dvz_app(scene)
# Create a view and run or render frames here.
dvz.dvz_app_destroy(app)
dvz.dvz_scene_destroy(scene)
```

## Important Details

The v0.4 Python surface is low-level and close to the C API. Do not expect the legacy v0.3 Pythonic
plotting interface to be present.

The facade is an array adapter, not a plotting wrapper. It does not rename APIs into Pythonic
objects, infer visual families, or replace the retained scene model.

Use NumPy arrays with explicit dtype, shape, and layout. For dense visual attributes, the first
dimension is the item count. Common examples include `float32` positions shaped `(n, 3)`, `uint8`
RGBA colors shaped `(n, 4)`, and `float32` diameters shaped `(n,)`.

The facade may copy non-contiguous arrays during the call. For predictable performance and raw
pointer compatibility, pass C-contiguous arrays yourself with `np.asarray(data, dtype=..., order="C")`.

When porting a C example:

1. keep the same `dvz_*` call order;
2. convert each dense attribute to the C dtype and shape before upload;
3. use `import datoviz as dvz` first;
4. switch only the calls that need exact pointer/count behavior to `datoviz.raw`;
5. keep explicit destroy calls for owner handles.

## Common Mistakes

- Passing default `float64` NumPy arrays where the C API expects `float32`.
- Passing non-contiguous arrays to raw pointer calls.
- Expecting `datoviz` to provide the old v0.3 plotting API.
- Importing generated implementation modules such as `datoviz._ctypes` directly.
- Treating Python object lifetime as a substitute for `dvz_app_destroy()` and
  `dvz_scene_destroy()`.
- Adding placeholder Python tabs when a Python path is not implemented.

## See Also

- [Python with NumPy arrays](../reference/python-direct-engine.md)
- [Use Python raw ctypes](use-raw-ctypes.md)
- [Use from C or C++](c-integration.md)
- [Choose a visual family](choose-a-visual-family.md)
- [Python raw ctypes reference](../reference/ctypes.md)

??? example "Related examples"

    - Start page: [Quickstart](../start/quickstart.md)
    - Reference: [Python raw ctypes](../reference/ctypes.md)
    - [Scatter Plot](../examples/gallery/start/start_scatter.md) - Source: `examples/c/start/scatter.c`
