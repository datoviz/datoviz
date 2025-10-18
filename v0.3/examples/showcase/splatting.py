"""
# Triangle splatting showcase example

Show triangle splatting using the **Basic** visual, data obtained from the 2025 paper
*Triangle Splatting for Real-Time Radiance Field Rendering* by Held et al., 2025.

Use the fly camera controller to navigate in the 3D scene:

- Left mouse drag: Look around (yaw/pitch)
- Right mouse drag: Orbit around a dynamic center (in front of the camera)
- Middle mouse drag: Move the camera left/right and up/down
- Arrow keys: Move in view direction (up/down) or strafe (left/right)

## Credits

Dataset kindly provided by [Jan Held](https://www.janheld.com/).

* [Github repo](https://github.com/trianglesplatting/triangle-splatting)
* [Project page](https://trianglesplatting.github.io/)
* [Paper](https://arxiv.org/abs/2505.19175)

---
tags:
  - basic
  - splatting
  - fly
  - orbit
in_gallery: true
make_screenshot: true
---

"""

import numpy as np

import datoviz as dvz


def load_data(filepath):
    data = np.load(filepath)
    position = data['position']
    color = data['color']

    # Data transformation
    position[:, 0] *= -1
    position[:, 1] *= -1
    position[:, 0] += 0.30
    position[:, 1] += 0.5
    position[:, 2] -= 0.5
    angle = -0.5
    rot = np.array(
        [
            [1, 0, 0],
            [0, np.cos(angle), -np.sin(angle)],
            [0, np.sin(angle), np.cos(angle)],
        ]
    )
    position = position @ rot

    return position, color


# Use either interactive fly camera or orbit animation
ORBIT = True

position, color = load_data(dvz.download_data('misc/garden.npz'))

app = dvz.App()
figure = app.figure()
panel = figure.panel()
if not ORBIT:
    fly = panel.fly()
camera = panel.camera(initial=(-3, 1, 3))

visual = app.basic(
    'triangle_list',
    position=position,
    color=color,
    depth_test=True,
)
panel.add(visual)

if ORBIT:
    panel.orbit(period=20)

app.run()
app.destroy()
