# Historical PoC: napari VisPy Canvas Replacement with Datoviz v0.3

Status: historical / retired
Target Datoviz branch: `main` / v0.3.x  
Target napari integration point: `QtViewer(..., canvas_class=DatovizCanvas)`  
Rendering strategy: Datoviz offscreen rendering -> PNG/RGBA readback -> Qt widget paint
Initial layer support: one 2D or 3D `Image` layer only

This note preserves the unique guidance from an old v0.3 offscreen experiment. It is not an active
v0.4 implementation target. For current direction, use [NAPARI.md](NAPARI.md).


## Historical Goal

The PoC tried to prove one narrow claim:

```text
napari can own the model and UI
Datoviz can own rendering
the connection can happen at the canvas boundary
```

It replaced napari's central rendering canvas with a minimal Datoviz-backed canvas while leaving the
rest of napari intact:

```text
napari ViewerModel / layer list / dims slider
    -> custom DatovizCanvas
    -> Datoviz v0.3 offscreen App
    -> Datoviz image visual
    -> PNG or RGBA readback
    -> Qt QWidget paint
```

The PoC was intentionally slow and incomplete. It did not attempt a production backend, Qt/Vulkan
surface embedding, full layer support, labels, points, picking, Dask/Zarr optimization, or exact
napari camera behavior.


## Minimal Demo Shape

The intended demo used standard napari objects:

```python
import numpy as np
from napari.components import ViewerModel
from napari._qt.qt_viewer import QtViewer
from qtpy.QtWidgets import QApplication

from datoviz_canvas import DatovizCanvas

app = QApplication.instance() or QApplication([])

viewer = ViewerModel()
qt_viewer = QtViewer(viewer, canvas_class=DatovizCanvas)
qt_viewer.show()

data = np.random.random((32, 512, 512)).astype("float32")
viewer.add_image(data, name="random volume", colormap="viridis", contrast_limits=(0, 1))

app.exec()
```

Expected historical result:

1. napari opens normally.
2. The central canvas is a custom Datoviz-backed widget.
3. A standard napari `Image` layer is rendered by Datoviz, not VisPy.
4. Moving the Z slider updates the displayed slice.
5. Contrast limits and colormap synchronization can be added after basic rendering works.


## Prototype Responsibilities

| Object | Historical responsibility |
| ------ | ------------------------- |
| `QtViewer` | Owns the napari shell and custom canvas instance. |
| `DatovizCanvas` | Owns the QWidget native, offscreen Datoviz app, figure, panel, texture, and image visual. |
| `ViewerModel` | Owns layers, dims, camera, and napari semantics. |
| Adapter code | Observes napari state, extracts the current slice, updates Datoviz, and repaints Qt. |

The canvas replacement needed to tolerate napari calls such as
`add_layer_visual_mapping(layer, visual)` even if the `visual` argument was a VisPy wrapper created by
napari internals. The PoC ignored that wrapper and observed `viewer.layers` directly.


## Historical Implementation Steps

1. Create a dummy `DatovizCanvas` exposing the subset of the VisPy canvas interface touched by
   `QtViewer`: `native`, `size`, `layer_to_visual`, `add_layer_visual_mapping()`,
   `remove_layer_visual_mapping()`, `screenshot()`, and `destroy()`.
2. Back `native` with a focused `QWidget` that paints a black rectangle.
3. Create a Datoviz v0.3 offscreen app, figure, panel, and image visual.
4. Find the first napari `Image` layer from `viewer.layers`.
5. Use napari dims state to extract the current 2D slice from 2D or 3D image data.
6. Convert the slice to a Datoviz-compatible scalar image, initially with CPU normalization.
7. Render one Datoviz offscreen frame, read it back, and paint it into the Qt widget.
8. Re-render when layer data, dims, contrast limits, or colormap change.


## Known Limitations

| Limitation | Historical next step |
| ---------- | -------------------- |
| PNG readback was slow | Expose direct RGBA NumPy readback such as `render_to_array()`. |
| napari might still create VisPy layer wrappers | Ignore wrapper objects and observe `viewer.layers`. |
| No canvas events | Accept no pan/zoom/picking for the first proof. |
| Image orientation could be flipped or transposed | Test non-symmetric images and adjust transpose, flip, or texture coordinates. |
| Contrast/colormap sync was incomplete | Map common napari colormap names and CPU-normalize contrast first. |
| No resize handling | Ignore resize initially or recreate the offscreen Datoviz target. |
| No RGB or Points support | Add RGB image mode and one Points layer as the next useful demo. |


## Success Criteria

The retired PoC would have been successful if:

1. napari opened with a custom non-VisPy central canvas.
2. A standard napari `Image` layer rendered through Datoviz.
3. The napari dims slider updated the Datoviz-rendered image.
4. No custom napari layer type was required.
5. The rest of the napari Qt shell stayed usable.

Performance did not matter for this v0.3 experiment.


## Relationship To v0.4

The only idea to carry forward is the ownership boundary: napari owns model/UI semantics and Datoviz
owns rendering. The v0.3 details around offscreen PNG readback, dummy canvas methods, and ad hoc image
normalization should not guide the v0.4 scene -> DRP2 -> runtime implementation.
