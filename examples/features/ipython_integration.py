"""
# IPython integration

Interactive scatter plot you can tweak live from an IPython session.

Run with:

```
ipython -i examples/features/ipython_integration.py
```

Then, from IPython, call:

```
jitter_points()
```

which will update the points directly in the window.

Available handles at the prompt:
- `pts` (2D positions in data space)
- `points` (the Datoviz visual)
- `jitter_points(scale=0.05)` to perturb positions
- `recolor()` to randomize colors
- `app` if you need to `app.stop()` manually

---
tags:
  - ipython
  - interactive
  - point
in_gallery: false
make_screenshot: false
---

"""

from __future__ import annotations

import numpy as np

import datoviz as dvz


def _in_ipython() -> bool:
    try:
        from IPython import get_ipython
    except Exception:
        return False
    return get_ipython() is not None


rng = np.random.default_rng(seed=3141)

# Data.
n = 500
pts = rng.random((n, 2))
color = rng.integers(low=120, high=240, size=(n, 4), dtype=np.uint8)
color[:, 3] = 255
size = np.full(n, 12.0)

# Scene.
app = dvz.App(background='white')
figure = app.figure(800, 600)
panel = figure.panel()
axes = panel.axes((0, 1), (0, 1))

points = app.point(
    position=axes.normalize(pts[:, 0], pts[:, 1]),
    color=color,
    size=size,
)
panel.add(points)


def jitter_points(scale: float = 0.05) -> None:
    """
    Add Gaussian noise to point positions (clamped to [0, 1]) and update the visual.
    """
    global pts
    pts = np.clip(pts + rng.normal(0.0, scale, size=pts.shape), 0.0, 1.0)
    points.set_position(axes.normalize(pts[:, 0], pts[:, 1]))


def recolor() -> None:
    """
    Randomize point colors and update the visual.
    """
    color[:] = rng.integers(low=80, high=240, size=color.shape, dtype=np.uint8)
    color[:, 3] = 255
    points.set_color(color)


if _in_ipython():
    # Hand over control to IPython's GUI loop; window stays live while you use the prompt.
    app.enable_ipython()
else:
    app.run()
    app.destroy()
