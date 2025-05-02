"""# LIDAR point cloud

Data origin: https://lidarpayload.com/sample-data/, RESEPI-M2X-100m10ms-FOV90-On RangerPro.laz

"""

from pathlib import Path

import numpy as np

import datoviz as dvz


CURDIR = Path(__file__).resolve().parent.parent.parent
file_path = CURDIR / 'data/misc/lidar.npz'
data = np.load(CURDIR / 'data/misc/lidar.npz')
pos, color = data['pos'], data['color']
print(f"Loaded LIDAR data with {len(pos)} points.")
N = pos.shape[0]

app = dvz.App()
figure = app.figure()
panel = figure.panel()
arcball = panel.arcball(initial=(.5, .9, .1))
camera = panel.camera(initial=(0, 0, 1.5))

visual = app.pixel(position=pos, color=color, size=2, depth_test=True)
panel.add(visual)

app.run()
app.destroy()
