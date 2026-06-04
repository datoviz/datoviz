# Use Raw `ctypes`

The recommended direct-engine Python import is the array-aware facade:

```python
import datoviz as dvz
```

Use raw `ctypes` when you need exact low-level FFI behavior:

```python
import datoviz.raw as raw
```

The raw layer preserves C signatures. That means Python callers provide explicit bytes, pointers,
counts, and `ctypes` objects:

```python
import ctypes
import numpy as np
import datoviz.raw as raw

scene = raw.dvz_scene()
points = raw.dvz_point(scene, 0)

positions = np.array([[0.0, 0.0, 0.0]], dtype=np.float32)
raw.dvz_visual_set_data(
    points,
    b"position",
    positions.ctypes.data_as(ctypes.c_void_p),
    positions.shape[0],
)

raw.dvz_scene_destroy(scene)
```

For normal data uploads, prefer the facade:

```python
import datoviz as dvz

dvz.dvz_visual_set_data(points, "position", positions)
```

Raw `ctypes` is for ABI validation, debugging, advanced FFI integration, and exact C-shaped call
sites. It is not the v0.3 Python object model and not a plotting API.
