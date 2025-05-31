"""
# Triangle splatting showcase example

Show triangle splatting using the **Basic** visual, data obtained from the 2025 paper
*Triangle Splatting for Real-Time Radiance Field Rendering* by Held et al., 2025.

Use the fly camera controller to navigate in the 3D scene:

- Left mouse drag: Look around (yaw/pitch)
- Right mouse drag: Move the camera left/right and up/down
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
in_gallery: true
make_screenshot: true
---

"""

import numpy as np

import datoviz as dvz

data = np.load(dvz.download_data('misc/garden.npz'))
position = data['position']
color = data['color']

# Data transformation
position[:, 0] *= -1
position[:, 1] *= -1
position[:, 1] += 0.5
position[:, 2] -= 1
angle = -0.5
rot = np.array(
    [
        [1, 0, 0],
        [0, np.cos(angle), -np.sin(angle)],
        [0, np.sin(angle), np.cos(angle)],
    ]
)
position = position @ rot

app = dvz.App()
figure = app.figure()
panel = figure.panel()
arcball = panel.fly()

visual = app.basic(
    'triangle_list',
    position=position,
    color=color,
    depth_test=True,
)
panel.add(visual)

app.run()
app.destroy()
