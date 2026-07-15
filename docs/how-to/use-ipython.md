# Use from Terminal IPython

Use terminal IPython when you want to keep a native Datoviz window open while editing arrays and
calling Datoviz functions at the prompt.

This workflow is for the terminal `ipython` program. It is not the Jupyter notebook or browser
WebGPU path.

!!! warning "Active macOS close issue"

    On the current v0.4 development branch, closing a hosted Datoviz window from terminal IPython
    can leave the native window unresponsive on macOS. Live updates work, but close and reopen are
    not yet release-proven on that path. Save any prompt-side state before closing the window and
    do not rely on repeated close/reopen until this known issue is resolved. Regular blocking
    Python scripts and the native C app loop use a different run-loop path.

## Start IPython

From an editable checkout, make sure Python loads this checkout and its freshly built library:

```sh
PYTHONPATH=. ipython
```

## Create a live scene

Paste this into IPython:

```python
import numpy as np
import datoviz as dvz

n = 5000
rng = np.random.default_rng(1)

positions = np.zeros((n, 3), dtype=np.float32)
positions[:, :2] = rng.normal(0.0, 0.35, size=(n, 2)).astype(np.float32)

colors = np.empty((n, 4), dtype=np.uint8)
colors[:, 0] = np.linspace(40, 255, n).astype(np.uint8)
colors[:, 1] = 180
colors[:, 2] = 255 - colors[:, 0]
colors[:, 3] = 255

diameters = np.full(n, 5.0, dtype=np.float32)

scene = dvz.dvz_scene()
figure = dvz.dvz_figure(scene, 800, 600, 0)
panel = dvz.dvz_panel_full(figure)
points = dvz.dvz_point(scene, 0)

dvz.dvz_visual_set_data_many(
    points,
    {
        "position": positions,
        "color": colors,
        "diameter_px": diameters,
    },
)
dvz.dvz_panel_add_visual(panel, points, None)

session = dvz.run(scene, figure, title="IPython live update")
session
```

In a regular Python script, `dvz.run(scene, figure)` blocks until the window closes. In terminal
IPython, it returns a `RunSession` and the prompt stays usable while the native window remains
responsive. The macOS close limitation above applies when ending this hosted session.

## Update data at the prompt

Modify the NumPy arrays and upload the changed slice. Then request a redraw:

```python
positions[:, 0] += 0.1 * np.sin(np.linspace(0.0, 12.0, n)).astype(np.float32)
dvz.dvz_visual_set_data_range(points, "position", 0, positions)
session.request_frame()
```

Change point sizes:

```python
diameters[:] = 3.0 + 8.0 * rng.random(n, dtype=np.float32)
dvz.dvz_visual_set_data_range(points, "diameter_px", 0, diameters)
session.request_frame()
```

Change colors:

```python
colors[:, 0] = rng.integers(40, 255, size=n, dtype=np.uint8)
colors[:, 1] = rng.integers(40, 255, size=n, dtype=np.uint8)
colors[:, 2] = rng.integers(40, 255, size=n, dtype=np.uint8)
dvz.dvz_visual_set_data_range(points, "color", 0, colors)
session.request_frame()
```

For repeated prompt-side updates, keep using this pattern:

1. mutate or replace a C-contiguous NumPy array with the right dtype and shape;
2. upload it with `dvz_visual_set_data_range()`;
3. call `session.request_frame()`.

## Close and reopen

The intended lifecycle is shown below, but close and reopen are not yet release-proven in terminal
IPython on macOS. On that platform, treat this section as the target contract rather than a reliable
workflow until the active close issue is resolved.

Close the native window normally, or close it from Python:

```python
session.close()
```

Closing the window destroys the app and window resources owned by that session. It does not destroy
the retained scene, figure, panel, or visual objects. You can reopen the same scene:

```python
session = dvz.run(scene, figure, title="IPython live update")
```

When you are finished with the retained scene state:

```python
session.close()
dvz.dvz_scene_destroy(scene)
```

## Common checks

If the window does not update after changing arrays, make sure you uploaded the changed data and
requested a redraw:

```python
dvz.dvz_visual_set_data_range(points, "position", 0, positions)
session.request_frame()
```

If you are testing from an editable checkout, start IPython with `PYTHONPATH=.` so Python imports
the checkout instead of an installed package. If the session object says it is closed, reopen the
scene with `session = dvz.run(scene, figure)`.

## See also

- [Use from Python](use-python.md)
- [Update visual data](update-visual-data.md)
- [Python API](../reference/ctypes.md)
