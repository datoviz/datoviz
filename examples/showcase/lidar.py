"""
# LIDAR point cloud

Data origin: https://lidarpayload.com/sample-data/, RESEPI-M2X-100m10ms-FOV90-On RangerPro.laz

---
tags:
  - pixel
  - arcball
in_gallery: true
make_screenshot: true
---

"""

import numpy as np

import datoviz as dvz

data = np.load(dvz.download_data('misc/lidar.npz'))
pos, color = data['pos'], data['color']
print(f'Loaded LIDAR data with {len(pos)} points.')
N = pos.shape[0]

# -------------------------------------------------------------------------------------------------

app = dvz.App()
figure = app.figure()
panel = figure.panel(background=True)
arcball = panel.arcball(initial=(0.5, 0.9, 0.1))
camera = panel.camera(initial=(0, 0, 1.5))

visual = app.pixel(position=pos, color=color, size=2, depth_test=True)
panel.add(visual)

app.run()
app.destroy()
