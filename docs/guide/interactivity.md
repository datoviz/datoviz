# Interactivity

Datoviz supports basic 2D and 3D interactivity on panels, including:

- Mouse-based pan and zoom
- 3D arcball rotation
- Fly camera
- Orthographic projection control

These modes are attached to a panel and control the panel's internal transformation matrices.

!!! warning

    Interactivity features are still limited and evolving. Some functionality available in the C API is not yet exposed in Python.

---

## Pan and Zoom Interactivity

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

Adds a basic orthographic projection controller. Similar to panzoom, but with fixed scale and behavior suited for 2D rendering.

```python
ortho = panel.ortho()
```

This is useful for pixel-perfect rendering or interfaces that require fixed-size panels.

!!! warning

    This controller is still experimental.

---

## Arcball Interactivity (3D)

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
angles = arcball.get()    # get current rotation
arcball.reset()           # reset to initial state
```

When using an arcball, you can add a 3D gizmo with:

```python
panel.gizmo()
```

<figure markdown="span">
![3D gizmo](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/gizmo.png){ width="300" }
</figure>

!!! warning

    This feature is still experimental. A known issue is that the gizmo may be obscured by other visuals in the scene if they are rendered in front of it. This will be fixed in version 0.4.

---

## Fly Camera Interactivity (3D)

The fly camera allows you to navigate through a 3D scene as if you were flying. It provides controls for moving forward, backward, left, right, and up/down, as well as rotating the view using the mouse or keyboard.

```python
fly = panel.fly()
```

Features:

- **Mouse Controls**: Drag the left mouse button to rotate the view (yaw/pitch). Drag the right mouse button to roll or move sideways/upwards. Use the mouse wheel to move forward/backward.
- **Keyboard Controls**: Use arrow keys to move forward, backward, left, or right.
- **Reset**: Double-click to reset the camera to its initial position and orientation.

!!! warning

    This controller is still experimental and may need further testing.

---

## Camera Interactivity (3D)

Adds a 3D camera with position, target (look-at), and up vector.

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

!!! note

    The camera is automatically added when using an arcball.

To control it manually:

```python
camera.set(position=(...), lookat=(...), up=(...))
pos = camera.position()
```

---

## Summary

| Interactivity | Description                   | Use cases                   |
| ------------- | ----------------------------- | --------------------------- |
| `panzoom()`   | Mouse-driven 2D pan/zoom      | Line plots, scatter, 2D UI  |
| `ortho()`     | Fixed orthographic controller | Pixel-accurate 2D panels    |
| `arcball()`   | Interactive 3D rotation       | Object inspection, rotation |
| `fly()`       | Fly camera                    | Navigation in a 3D scene    |
| `camera()`    | Custom 3D camera control      | Navigation in a 3D scene    |

Each mode provides access to its state (e.g. position, rotation) and can be reset or manually adjusted.

More interactive features and fine-grained control will be added in future versions.
