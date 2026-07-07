# Use from Python

Create Datoviz scenes from Python and upload NumPy arrays to visual attributes.

## Task Workflow

Use the main Python package when you want the current v0.4 scene workflow from Python. Datoviz has
one generated `ctypes` binding. The top-level package follows the same `dvz_*` function names as the
C examples and accepts NumPy arrays for supported visual-data uploads.

Choose the import surface first:

| Need | Import |
| --- | --- |
| Normal binding calls with NumPy array adaptation. | `import datoviz as dvz` |
| The same binding with explicit pointers, counts, bytes, or callbacks. | `import datoviz.raw as raw` |
| High-level object-oriented plotting. | Use the external GSP/VisPy2 layer when available. |

## Minimal Call Sequence

```python
import numpy as np
import datoviz as dvz

positions = np.array(
    [[-0.5, -0.5, 0.0], [0.5, -0.5, 0.0], [0.0, 0.5, 0.0]], dtype=np.float32
)
colors = np.array(
    [[255, 80, 80, 255], [80, 255, 160, 255], [80, 160, 255, 255]], dtype=np.uint8
)
diameters = np.array([18.0, 18.0, 18.0], dtype=np.float32)

scene = dvz.dvz_scene()
figure = dvz.dvz_figure(scene, 800, 600, 0)
panel = dvz.dvz_panel_full(figure)

points = dvz.dvz_point(scene, 0)

dvz.dvz_visual_set_data(points, "position", positions)
dvz.dvz_visual_set_data(points, "color", colors)
dvz.dvz_visual_set_data(points, "diameter_px", diameters)
dvz.dvz_panel_add_visual(panel, points, None)
```

Adapt the C examples one call at a time. Keep NumPy array dtype and shape matched to the C
attribute contract.

The top-level `datoviz` module accepts NumPy arrays for the calls covered by the binding policy. Use
`datoviz.raw` only when you need the exact C-shaped call form. Calls that are not covered by
the policy may still expect explicit pointer/count arguments, so consult the C and Python binding
references when needed.

Destroy owner handles with the same C lifecycle calls:

```python
app = dvz.dvz_app(scene)
# Create a view and run or render frames here.
dvz.dvz_app_destroy(app)
dvz.dvz_scene_destroy(scene)
```

For terminal IPython, use `session = dvz.run(scene, figure)`. The prompt remains usable while the
native window stays responsive, so you can update NumPy arrays, upload them with
`dvz_visual_set_data_range()`, and call `session.request_frame()`. See
[Use from terminal IPython](use-ipython.md).

## Important Details

The v0.4 Python surface is close to the C API. It is meant for explicit scene, panel, visual, and
data-upload code.

The top-level package adapts arrays; it is not a plotting wrapper. It does not rename APIs into
Pythonic objects, infer visual families, or replace the retained scene model.

Use NumPy arrays with explicit dtype, shape, and layout. For dense visual attributes, the first
dimension is the item count. Common examples include `float32` positions shaped `(n, 3)`, `uint8`
RGBA colors shaped `(n, 4)`, and `float32` diameters shaped `(n,)`.

The top-level package may copy non-contiguous arrays during the call. For predictable performance
and exact pointer compatibility, pass C-contiguous arrays yourself with
`np.asarray(data, dtype=..., order="C")`.

When porting a C example:

1. keep the same `dvz_*` call order;
2. convert each dense attribute to the C dtype and shape before upload;
3. use `import datoviz as dvz` first;
4. switch only the calls that need exact pointer/count behavior to `datoviz.raw`;
5. keep explicit destroy calls for owner handles.

## Common Mistakes

- Passing default `float64` NumPy arrays where the C API expects `float32`.
- Passing non-contiguous arrays to exact pointer calls.
- Expecting `datoviz` to provide high-level plotting functions such as `scatter()` or `imshow()`.
- Importing generated implementation modules such as `datoviz._ctypes` directly.
- Treating Python object lifetime as a substitute for `dvz_app_destroy()` and
  `dvz_scene_destroy()`.
- Adding empty Python tabs when a Python path is not implemented.

## See Also

- [Python binding with NumPy arrays](../reference/python-direct-engine.md)
- [Use from terminal IPython](use-ipython.md)
- [Use the exact Python binding call form](use-raw-ctypes.md)
- [Use from C or C++](c-integration.md)
- [Choose a visual family](choose-a-visual-family.md)
- [Python binding exact call form](../reference/ctypes.md)

??? example "Related examples"

    - Start page: [Quickstart](../start/quickstart.md)
    - Reference: [Python binding exact call form](../reference/ctypes.md)
    - [Scatter Plot](../examples/gallery/start/start_scatter.md) - Source: `examples/c/start/scatter.c`
