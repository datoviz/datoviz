# Minimal PoC: Replacing napari's VisPy canvas with a Datoviz v0.3 offscreen canvas

Status: proof-of-concept design  
Target Datoviz branch: `main` / v0.3.x  
Target napari integration point: `QtViewer(..., canvas_class=DatovizCanvas)`  
Rendering strategy: Datoviz offscreen rendering → PNG/RGBA readback → Qt widget paint  
Initial layer support: one 2D or 3D `Image` layer only

---

## 1. Goal

The goal is to demonstrate the smallest credible proof of concept that Datoviz can act as a napari rendering backend.

This PoC should replace napari's central rendering canvas with a minimal Datoviz-backed canvas, while keeping the rest of napari intact:

```text
napari ViewerModel
napari layer list
napari dims slider
napari layer controls
napari Image layer
        ↓
custom DatovizCanvas
        ↓
Datoviz v0.3 offscreen App
        ↓
Datoviz image visual
        ↓
PNG/RGBA readback
        ↓
Qt QWidget paint
````

The PoC is intentionally narrow. It does not aim to be fast, complete, or production-quality.

The only claim it should prove is:

> A napari `QtViewer` can run with a non-VisPy canvas object, and that object can render a standard napari `Image` layer through Datoviz.

---

## 2. Non-goals

Do not attempt the following in the first PoC:

* no full napari backend abstraction,
* no direct Qt/Vulkan surface embedding,
* no replacement of napari layer classes,
* no support for all layer types,
* no labels,
* no points,
* no shapes,
* no 3D volume rendering,
* no picking,
* no async Dask/Zarr optimization,
* no perfect napari camera behavior,
* no exact layer blending semantics,
* no plugin compatibility testing beyond standard `viewer.add_image()`.

This PoC is a canvas replacement experiment, not a full renderer implementation.

---

## 3. Why use Datoviz offscreen rendering first?

Datoviz v0.3 is Vulkan/GLFW-based and does not yet provide a production Qt backend.

Embedding a Vulkan swapchain directly inside a Qt widget is possible but not the simplest first step. It would mix several difficult problems:

* Qt native window handles,
* Vulkan surface creation,
* swapchain lifecycle,
* resize synchronization,
* device-pixel ratio handling,
* event routing,
* napari canvas expectations.

For the first PoC, avoid all of that.

Instead:

```text
Datoviz renders offscreen
Datoviz writes a screenshot to PNG
Qt loads the PNG into a QImage/QPixmap
Qt paints it in the central napari canvas widget
```

This is inefficient but simple and robust enough to prove the architecture seam.

---

## 4. Minimal visible demo

The intended user-facing demo is:

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

Expected behavior:

1. napari opens normally,
2. the central canvas is a Datoviz-backed widget,
3. the image is rendered by Datoviz, not VisPy,
4. moving the napari Z slider updates the displayed slice,
5. changing contrast limits or colormap eventually updates Datoviz rendering.

---

## 5. Architecture

### 5.1 Object ownership

```text
QtViewer
  owns DatovizCanvas

DatovizCanvas
  owns QWidget native
  owns Datoviz offscreen App
  owns Datoviz Figure
  owns Datoviz Panel
  owns Datoviz Texture
  owns Datoviz Image visual

napari ViewerModel
  owns layers, dims, camera, layer state
```

DatovizCanvas observes napari state. It should not modify napari layer semantics.

### 5.2 Rendering pipeline

```text
napari Image layer data
    ↓
extract current 2D slice
    ↓
normalize or pass scalar image to Datoviz
    ↓
Datoviz texture_2D update
    ↓
Datoviz offscreen render one frame
    ↓
Datoviz screenshot PNG
    ↓
Qt QPixmap display
```

---

## 6. Implementation plan

### Step 1 — Create a dummy replacement canvas

Before using Datoviz, prove that napari accepts the custom canvas class.

Create `datoviz_canvas.py`:

```python
import numpy as np

from qtpy.QtCore import Qt
from qtpy.QtGui import QImage, QPainter
from qtpy.QtWidgets import QWidget


class DatovizCanvas:
    """Minimal napari canvas replacement.

    This class intentionally implements only the subset of the VispyCanvas
    interface that QtViewer touches in this PoC.
    """

    def __init__(self, viewer, parent=None, size=(800, 600), **kwargs):
        self.viewer = viewer
        self.native = _DatovizCanvasWidget(parent=parent)
        self.native.resize(*size)
        self.size = size

        # QtViewer expects to be able to store layer-to-visual mappings.
        self.layer_to_visual = {}

    def add_layer_visual_mapping(self, layer, visual):
        self.layer_to_visual[layer] = visual

    def remove_layer_visual_mapping(self, layer):
        self.layer_to_visual.pop(layer, None)

    def screenshot(self, *args, **kwargs):
        w, h = self.size
        return np.zeros((h, w, 4), dtype=np.uint8)

    def destroy(self):
        pass


class _DatovizCanvasWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMinimumSize(512, 512)
        self.setFocusPolicy(Qt.FocusPolicy.StrongFocus)

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.fillRect(self.rect(), Qt.GlobalColor.black)
        painter.end()
```

Test:

```python
from qtpy.QtWidgets import QApplication
from napari.components import ViewerModel
from napari._qt.qt_viewer import QtViewer

from datoviz_canvas import DatovizCanvas

app = QApplication.instance() or QApplication([])

viewer = ViewerModel()
qt_viewer = QtViewer(viewer, canvas_class=DatovizCanvas)
qt_viewer.show()

viewer.add_image([[0, 1], [1, 0]])

app.exec()
```

Success criterion:

* napari opens,
* the central canvas is black,
* the layer list and controls still appear,
* no VisPy canvas is required for the central widget.

---

### Step 2 — Add Datoviz offscreen app creation

Extend `DatovizCanvas`:

```python
import datoviz as dvz


class DatovizCanvas:
    def __init__(self, viewer, parent=None, size=(800, 600), **kwargs):
        self.viewer = viewer
        self.native = _DatovizCanvasWidget(parent=parent)
        self.native.resize(*size)
        self.size = tuple(size)

        self.layer_to_visual = {}

        self.dvz_app = dvz.App(offscreen=True, background="white")
        self.dvz_figure = self.dvz_app.figure(self.size[0], self.size[1])
        self.dvz_panel = self.dvz_figure.panel()

        self.dvz_texture = None
        self.dvz_image = None

        self._connect_napari_events()
        self._sync_from_viewer()
```

Important:

* `offscreen=True` avoids native window embedding.
* Use one Datoviz figure and one panel.
* Initially ignore napari camera and use a full-panel image.

---

### Step 3 — Find the first napari Image layer

Add helper methods:

```python
def _first_image_layer(self):
    try:
        from napari.layers import Image
    except Exception:
        Image = None

    for layer in self.viewer.layers:
        if Image is None or isinstance(layer, Image):
            if hasattr(layer, "data"):
                return layer
    return None


def _connect_napari_events(self):
    self.viewer.layers.events.inserted.connect(lambda event: self._sync_from_viewer())
    self.viewer.layers.events.removed.connect(lambda event: self._sync_from_viewer())

    # Different napari versions expose dims events slightly differently.
    for name in ("current_step", "point", "ndisplay", "order"):
        ev = getattr(self.viewer.dims.events, name, None)
        if ev is not None:
            ev.connect(lambda event: self._sync_from_viewer())
```

For the first PoC, only the first image layer is rendered.

---

### Step 4 — Extract the currently displayed 2D slice

Add:

```python
def _current_2d_slice(self, layer):
    data = layer.data

    arr = np.asarray(data)

    if arr.ndim == 2:
        return arr

    if arr.ndim == 3:
        # Minimal assumption for PoC:
        # data is (z, y, x), and napari dims current_step[0] is z.
        try:
            z = int(self.viewer.dims.current_step[0])
        except Exception:
            z = 0
        z = max(0, min(z, arr.shape[0] - 1))
        return arr[z]

    raise NotImplementedError(
        f"PoC only supports 2D or 3D image data, got shape {arr.shape}"
    )
```

This deliberately avoids napari's internal slicing machinery.

For a better second version, use the layer's displayed/sliced data after napari slicing is ready. The first version can assume `(z, y, x)` arrays.

---

### Step 5 — Convert data to a Datoviz-compatible scalar image

Datoviz v0.3 `image(..., mode="colormap")` can use a single-channel texture with a colormap.

Normalize to `float32` and preserve the contrast range separately.

```python
def _prepare_scalar_image(self, image):
    image = np.asarray(image)

    if image.ndim != 2:
        raise ValueError("Expected a 2D scalar image")

    # Datoviz texture data should be contiguous.
    image = np.ascontiguousarray(image.astype(np.float32, copy=False))
    return image
```

For a first PoC, do not implement RGB/RGBA images. Add that later.

---

### Step 6 — Create or update the Datoviz image visual

Use Datoviz v0.3 high-level API:

```python
def _ensure_datoviz_image(self, image):
    h, w = image.shape

    if self.dvz_texture is None or self._texture_shape != image.shape:
        self._texture_shape = image.shape

        self.dvz_texture = self.dvz_app.texture_2D(
            image,
            interpolation="nearest",
            address_mode="clamp_to_edge",
        )

        position = np.array([[0.0, 0.0, 0.0]], dtype=np.float32)
        size = np.array([[2.0, 2.0]], dtype=np.float32)
        anchor = np.array([[0.0, 0.0]], dtype=np.float32)
        texcoords = np.array([[0.0, 0.0, 1.0, 1.0]], dtype=np.float32)

        self.dvz_image = self.dvz_app.image(
            position=position,
            size=size,
            anchor=anchor,
            texcoords=texcoords,
            texture=self.dvz_texture,
            mode="colormap",
            colormap="viridis",
            unit="ndc",
        )

        self.dvz_panel.add(self.dvz_image)
        self.dvz_panel.update()
        self.dvz_figure.update()

    else:
        self.dvz_texture.data(image)
```

Notes:

* `unit="ndc"` makes the image cover the normalized device coordinate panel.
* `size = (2, 2)` covers the full panel in NDC.
* `mode="colormap"` uses scalar texture display.
* `texture.data(image)` updates the existing texture.

Potential adjustment:

If the image appears transposed or flipped, try:

```python
permutation=(1, 0)
```

or adjust `texcoords`.

Image orientation is one of the things this PoC should explicitly test.

---

### Step 7 — Render one Datoviz frame and copy into Qt

The simplest v0.3 path is `App.screenshot(figure, png_path)`, which runs one frame and writes a PNG.

```python
import tempfile
from pathlib import Path

from qtpy.QtGui import QImage, QPixmap


def _render_datoviz_to_qimage(self):
    with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as f:
        path = Path(f.name)

    try:
        self.dvz_app.screenshot(self.dvz_figure, str(path))
        img = QImage(str(path))
        if img.isNull():
            raise RuntimeError("Datoviz screenshot produced an unreadable image")
        return img.convertToFormat(QImage.Format.Format_RGBA8888)
    finally:
        try:
            path.unlink()
        except OSError:
            pass
```

Then update the widget:

```python
def _sync_from_viewer(self):
    layer = self._first_image_layer()
    if layer is None:
        self.native.set_image(None)
        return

    image = self._current_2d_slice(layer)
    image = self._prepare_scalar_image(image)

    self._ensure_datoviz_image(image)

    qimage = self._render_datoviz_to_qimage()
    self.native.set_image(qimage)
```

Widget paint code:

```python
class _DatovizCanvasWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self._image = None
        self.setMinimumSize(512, 512)
        self.setFocusPolicy(Qt.FocusPolicy.StrongFocus)

    def set_image(self, image):
        self._image = image
        self.update()

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.fillRect(self.rect(), Qt.GlobalColor.black)

        if self._image is not None:
            pixmap = QPixmap.fromImage(self._image)
            painter.drawPixmap(self.rect(), pixmap)

        painter.end()
```

This is crude but sufficient.

---

## 7. Complete first-file prototype

Create `datoviz_canvas.py`:

```python
from __future__ import annotations

import tempfile
from pathlib import Path

import numpy as np
import datoviz as dvz

from qtpy.QtCore import Qt
from qtpy.QtGui import QImage, QPainter, QPixmap
from qtpy.QtWidgets import QWidget


class DatovizCanvas:
    """Minimal Datoviz-backed replacement for napari's VisPy canvas.

    This proof of concept supports only the first napari Image layer.
    Rendering is intentionally inefficient:
    Datoviz offscreen render -> PNG screenshot -> QImage -> QWidget paint.
    """

    def __init__(self, viewer, parent=None, size=(800, 600), **kwargs):
        self.viewer = viewer
        self.size = tuple(int(x) for x in size)

        self.native = _DatovizCanvasWidget(parent=parent)
        self.native.resize(*self.size)

        # Compatibility with QtViewer expectations.
        self.layer_to_visual = {}

        # Datoviz state.
        self.dvz_app = dvz.App(offscreen=True, background="white")
        self.dvz_figure = self.dvz_app.figure(self.size[0], self.size[1])
        self.dvz_panel = self.dvz_figure.panel()

        self.dvz_texture = None
        self.dvz_image = None
        self._texture_shape = None

        self._connect_napari_events()
        self._sync_from_viewer()

    # -------------------------------------------------------------------------
    # Minimal napari canvas API
    # -------------------------------------------------------------------------

    def add_layer_visual_mapping(self, layer, visual):
        self.layer_to_visual[layer] = visual

    def remove_layer_visual_mapping(self, layer):
        self.layer_to_visual.pop(layer, None)

    def screenshot(self, *args, **kwargs):
        if self.native._image is None:
            h, w = self.size[1], self.size[0]
            return np.zeros((h, w, 4), dtype=np.uint8)

        img = self.native._image.convertToFormat(QImage.Format.Format_RGBA8888)
        w, h = img.width(), img.height()
        ptr = img.bits()
        ptr.setsize(h * w * 4)
        arr = np.frombuffer(ptr, np.uint8).reshape((h, w, 4)).copy()
        return arr

    def destroy(self):
        if self.dvz_app is not None:
            self.dvz_app.destroy()
            self.dvz_app = None

    # -------------------------------------------------------------------------
    # napari state observation
    # -------------------------------------------------------------------------

    def _connect_napari_events(self):
        self.viewer.layers.events.inserted.connect(lambda event: self._sync_from_viewer())
        self.viewer.layers.events.removed.connect(lambda event: self._sync_from_viewer())
        self.viewer.layers.events.reordered.connect(lambda event: self._sync_from_viewer())

        for name in ("current_step", "point", "ndisplay", "order"):
            ev = getattr(self.viewer.dims.events, name, None)
            if ev is not None:
                ev.connect(lambda event: self._sync_from_viewer())

    def _connect_layer_events(self, layer):
        # Connect only events that exist in the current napari version.
        for name in (
            "data",
            "visible",
            "opacity",
            "contrast_limits",
            "colormap",
            "interpolation2d",
        ):
            ev = getattr(layer.events, name, None)
            if ev is not None:
                ev.connect(lambda event: self._sync_from_viewer())

    def _first_image_layer(self):
        try:
            from napari.layers import Image
        except Exception:
            Image = None

        for layer in self.viewer.layers:
            if Image is None or isinstance(layer, Image):
                if hasattr(layer, "data"):
                    self._connect_layer_events(layer)
                    return layer
        return None

    # -------------------------------------------------------------------------
    # Data conversion
    # -------------------------------------------------------------------------

    def _current_2d_slice(self, layer):
        arr = np.asarray(layer.data)

        if arr.ndim == 2:
            return arr

        if arr.ndim == 3:
            # Minimal PoC assumption: data is (z, y, x).
            try:
                z = int(self.viewer.dims.current_step[0])
            except Exception:
                z = 0
            z = max(0, min(z, arr.shape[0] - 1))
            return arr[z]

        raise NotImplementedError(
            f"DatovizCanvas PoC only supports 2D or 3D Image layers, got {arr.shape}"
        )

    def _prepare_scalar_image(self, image):
        image = np.asarray(image)

        if image.ndim != 2:
            raise ValueError(f"Expected 2D scalar image, got {image.shape}")

        image = image.astype(np.float32, copy=False)
        image = np.ascontiguousarray(image)
        return image

    # -------------------------------------------------------------------------
    # Datoviz rendering
    # -------------------------------------------------------------------------

    def _ensure_datoviz_image(self, image):
        if self.dvz_texture is None or self._texture_shape != image.shape:
            self._texture_shape = image.shape

            self.dvz_texture = self.dvz_app.texture_2D(
                image,
                interpolation="nearest",
                address_mode="clamp_to_edge",
            )

            position = np.array([[0.0, 0.0, 0.0]], dtype=np.float32)
            size = np.array([[2.0, 2.0]], dtype=np.float32)
            anchor = np.array([[0.0, 0.0]], dtype=np.float32)
            texcoords = np.array([[0.0, 0.0, 1.0, 1.0]], dtype=np.float32)

            self.dvz_image = self.dvz_app.image(
                position=position,
                size=size,
                anchor=anchor,
                texcoords=texcoords,
                texture=self.dvz_texture,
                mode="colormap",
                colormap="viridis",
                unit="ndc",
            )

            self.dvz_panel.add(self.dvz_image)
            self.dvz_panel.update()
            self.dvz_figure.update()

        else:
            self.dvz_texture.data(image)

    def _render_datoviz_to_qimage(self):
        with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as f:
            path = Path(f.name)

        try:
            self.dvz_app.screenshot(self.dvz_figure, str(path))
            qimg = QImage(str(path))
            if qimg.isNull():
                raise RuntimeError("Datoviz screenshot failed or produced invalid PNG")
            return qimg.convertToFormat(QImage.Format.Format_RGBA8888)
        finally:
            try:
                path.unlink()
            except OSError:
                pass

    def _sync_from_viewer(self):
        layer = self._first_image_layer()
        if layer is None:
            self.native.set_image(None)
            return

        if not getattr(layer, "visible", True):
            self.native.set_image(None)
            return

        image = self._current_2d_slice(layer)
        image = self._prepare_scalar_image(image)

        self._ensure_datoviz_image(image)
        qimage = self._render_datoviz_to_qimage()
        self.native.set_image(qimage)


class _DatovizCanvasWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self._image = None
        self.setMinimumSize(512, 512)
        self.setFocusPolicy(Qt.FocusPolicy.StrongFocus)

    def set_image(self, image):
        self._image = image
        self.update()

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.fillRect(self.rect(), Qt.GlobalColor.black)

        if self._image is not None:
            pixmap = QPixmap.fromImage(self._image)
            painter.drawPixmap(self.rect(), pixmap)

        painter.end()
```

---

## 8. Test script

Create `poc_napari_datoviz_canvas.py`:

```python
import numpy as np

from qtpy.QtWidgets import QApplication
from napari.components import ViewerModel
from napari._qt.qt_viewer import QtViewer

from datoviz_canvas import DatovizCanvas


def main():
    app = QApplication.instance() or QApplication([])

    viewer = ViewerModel()
    qt_viewer = QtViewer(viewer, canvas_class=DatovizCanvas)
    qt_viewer.resize(1000, 800)
    qt_viewer.show()

    z, h, w = 32, 512, 512
    y, x = np.mgrid[-1:1:complex(h), -1:1:complex(w)]
    data = np.empty((z, h, w), dtype=np.float32)

    for k in range(z):
        r = np.sqrt((x - 0.5 * np.sin(k / 5)) ** 2 + (y - 0.5 * np.cos(k / 7)) ** 2)
        data[k] = np.exp(-20 * r**2)

    viewer.add_image(
        data,
        name="Datoviz-rendered image",
        colormap="viridis",
        contrast_limits=(0, 1),
    )

    app.exec()


if __name__ == "__main__":
    main()
```

Run:

```bash
python poc_napari_datoviz_canvas.py
```

---

## 9. Expected result

The napari shell should appear with:

* layer list,
* layer controls,
* dims slider,
* one image layer,
* central canvas rendered by `DatovizCanvas`.

Moving the Z slider should update the Datoviz-rendered image.

---

## 10. Known limitations

### 10.1 PNG readback is slow

This PoC uses:

```text
Datoviz screenshot → temporary PNG → Qt QImage
```

This is intentionally inefficient.

A better second version should expose a Datoviz v0.3 Python readback path returning an RGBA NumPy array directly.

Desired future API:

```python
rgba = app.screenshot_array(figure)
```

or:

```python
rgba = app.render_to_array(figure)
```

### 10.2 It may still create VisPy layer wrappers internally

Depending on the napari version, `QtViewer` may still call internal layer-to-visual machinery when layers are added.

The custom canvas should tolerate calls such as:

```python
canvas.add_layer_visual_mapping(layer, vispy_layer)
```

For the PoC, ignore the `vispy_layer` argument. The Datoviz canvas observes `viewer.layers` directly.

### 10.3 No true canvas events

The PoC does not implement napari mouse events, pan/zoom, or picking.

The first version can be considered successful even without interaction.

### 10.4 Image orientation may need adjustment

The first image visual may appear flipped or transposed.

Test with a non-symmetric image:

```python
data = np.zeros((512, 512), dtype=np.float32)
data[50:150, 100:200] = 1
```

If necessary, adjust:

```python
texcoords
permutation
image.T
np.flipud(image)
```

The correct convention should be documented after testing.

### 10.5 Contrast limits and colormap are not fully implemented

The skeleton uses `colormap="viridis"` but does not yet synchronize napari's colormap and contrast limits.

Add this after basic rendering works.

---

## 11. Next incremental improvements

### 11.1 Synchronize colormap

Map napari colormap names to Datoviz names:

```python
def _napari_colormap_to_datoviz(layer):
    name = getattr(layer.colormap, "name", "viridis")
    aliases = {
        "gray": "gray",
        "grey": "gray",
        "viridis": "viridis",
        "magma": "magma",
        "inferno": "inferno",
        "plasma": "plasma",
        "cividis": "cividis",
    }
    return aliases.get(name, "viridis")
```

Then recreate or update the Datoviz image visual when the colormap changes.

### 11.2 Synchronize contrast limits

Simplest version:

```python
lo, hi = layer.contrast_limits
image = np.clip((image - lo) / (hi - lo), 0, 1)
```

This does CPU-side normalization before upload.

Better version:

* keep original scalar texture,
* update Datoviz colormap range if supported through v0.3 image/colorbar APIs.

For the PoC, CPU-side normalization is acceptable.

### 11.3 Avoid recreating layer event connections

The current `_first_image_layer()` reconnects layer events repeatedly. Track connected layers:

```python
self._connected_layers = set()
```

and connect each layer only once.

### 11.4 Add resize support

When the Qt widget resizes:

```python
def resizeEvent(self, event):
    ...
```

For the first PoC, ignore dynamic resize or recreate the Datoviz figure if the size changes.

### 11.5 Add RGB image support

If `image.ndim == 3 and image.shape[-1] in (3, 4)`, use Datoviz `mode="rgba"` instead of `mode="colormap"`.

### 11.6 Add one Points layer

A strong second demo is:

```text
napari Image layer → Datoviz image visual
napari Points layer → Datoviz point visual
```

This shows that the canvas can mirror more than one napari layer type.

---

## 12. Success criteria

The PoC is successful if all of the following are true:

* napari opens with a custom non-VisPy canvas,
* the central widget is provided by `DatovizCanvas`,
* a standard napari `Image` layer is rendered through Datoviz,
* the napari dims slider updates the Datoviz-rendered image,
* no custom napari layer type is required,
* the rest of the napari Qt shell remains usable.

Performance does not matter yet.

---

## 13. Why this is the right first PoC

This is the smallest experiment that demonstrates the relevant architectural claim:

```text
napari can own the model and UI
Datoviz can own the rendering
the connection can happen at the canvas boundary
```

A side-by-side mirror window would prove only that Datoviz can display napari data.

This PoC proves something stronger:

```text
Datoviz can occupy the rendering slot where VisPy normally sits.
```

That is the right first step before proposing a serious Datoviz backend for napari.

```
::contentReference[oaicite:5]{index=5}
```

[1]: https://napari.org/stable/api/napari.qt.QtViewer.html?utm_source=chatgpt.com "napari.qt.QtViewer — napari"
