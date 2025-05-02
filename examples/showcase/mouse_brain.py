"""# Mesh example

Show a 3D mesh.

"""

from pathlib import Path

import numpy as np

import datoviz as dvz


CURDIR = Path(__file__).resolve().parent.parent.parent
data = np.load(CURDIR / 'data/mesh/brain.npz')
pos = data['pos']
normal = data['normal']
color = data['color']
index = data['index']
nv, ni = pos.shape[0], index.shape[0]

light_dir = (+1, -1, -1)
light_params = (0.5, 0.5, 0.5, 16)

angles = (-2.4, -0.9, 0.1)

# -------------------------------------------------------------------------------------------------

app = dvz.App()
figure = app.figure()
panel = figure.panel()
arcball = panel.arcball(initial=angles)

visual = app.mesh(indexed=True, lighting=True)
visual.set_data(
    position=pos,
    normal=normal,
    color=color,
    index=index,
    light_dir=light_dir,
    light_params=light_params,
)
panel.add(visual)

app.run()
app.destroy()
