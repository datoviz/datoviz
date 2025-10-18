"""
# Animation

Show how to make animations with timers.

---
tags:
  - point
  - timer
  - animation
in_gallery: true
make_screenshot: true
---

"""

import numpy as np

import datoviz as dvz

rng = np.random.default_rng(seed=3141)

n = 32
x, y = np.meshgrid(np.linspace(-1, +1, n), np.linspace(-1, +1, n))
nn = x.size
position = np.c_[x.flat, y.flat, np.zeros(nn)]
initial = position.copy()
color = rng.integers(low=100, high=240, size=(nn, 4), dtype=np.uint8)
size = np.full(nn, 20)

app = dvz.App(background='white')
figure = app.figure()
panel = figure.panel()
visual = app.point(position=position, color=color, size=size)
panel.add(visual)


# This callback function runs 60x per second.
@app.timer(period=1.0 / 60.0)
def on_timer(e):
    t = e.time()
    a = 0.025
    p = np.linspace(0.5, 2.0, nn)
    position[:, 0] = initial[:, 0] + a * np.cos(2 * np.pi * p * t)
    position[:, 1] = initial[:, 1] + a * np.sin(2 * np.pi * p * t)
    visual.set_position(position)


app.run()
app.destroy()
