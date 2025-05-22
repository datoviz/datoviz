# Interactivity

Datoviz supports basic 2D and 3D interactivity on panels, including:

- Mouse-based pan and zoom
- 3D arcball rotation
- Orthographic projection control
- Full 3D camera navigation

These modes are attached to a panel and control the panel's internal transformation matrices.

!!! warning

    Interactivity features are still limited and evolving. Some functionality available in the C API is not yet exposed in Python.

---

## 2D Interactivity

### `panel.panzoom()`

Adds standard 2D pan and zoom behavior to a panel. This is the most common interactivity used for 2D plots.

```python
panzoom = panel.panzoom()
```

You can optionally lock one axis:

```python
panzoom = panel.panzoom(fixed='x')  # lock horizontal movement
panzoom = panel.panzoom(fixed='y')  # lock vertical movement
```

---

## Orthographic Interactivity

### `panel.ortho()`

Adds a basic orthographic projection controller. Similar to panzoom, but with fixed scale and behavior suited for 2D rendering.

```python
ortho = panel.ortho()
```

This is useful for pixel-perfect rendering or interfaces that require fixed-size panels.

!!! warning

    This controller is still experimental.

---

## Arcball Interactivity (3D)

### `panel.arcball()`

Enables 3D rotation using an arcball-style controller. The arcball rotates the model around a virtual sphere.

```python
arcball = panel.arcball()
```

You can set the initial orientation:

```python
arcball = panel.arcball(initial=(0, 0, 0))
```

Once active, you can control it via:

```python
arcball.set((x, y, z))    # set rotation angles
angles = arcball.get()             # get current rotation
arcball.reset()           # reset to initial state
```

---

## Camera Interactivity (3D)

### `panel.camera()`

Adds a full 3D camera with position, target (look-at), and up vector.

```python
camera = panel.camera()
```

You can initialize it with:

```python
camera = panel.camera(
    initial=(3, 3, 3),
    initial_lookat=(0, 0, 0),
    initial_up=(0, 0, 1)
)
```

To control it manually:

```python
camera.set(position=(...), lookat=(...), up=(...))
pos = camera.position()
```

This interactivity is suited for full 3D scenes, model navigation, and dynamic viewing.

---

## Summary

| Interactivity | Description                   | Use cases                   |
| ------------- | ----------------------------- | --------------------------- |
| `panzoom()`   | Mouse-driven 2D pan/zoom      | Line plots, scatter, 2D UI  |
| `ortho()`     | Fixed orthographic controller | Pixel-accurate 2D panels    |
| `arcball()`   | Interactive 3D rotation       | Object inspection, rotation |
| `camera()`    | Full 3D camera                | Volumes, meshes, 3D scenes  |

Each mode provides access to its state (e.g. position, rotation) and can be reset or manually adjusted.

More interactive features and fine-grained control will be added in future versions.
