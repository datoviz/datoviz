"""# Raster plot

Show a raster plot (spikes in 2D, time on the x axis, neuron depth on the y axis) using
the Pixel visual.

"""

from pathlib import Path
import numpy as np
import datoviz as dvz


CURDIR = Path(__file__).resolve().parent.parent.parent

N = 1_000_000  # maximum number of spikes

# Load the data.
file_path = CURDIR / 'data/misc/raster_1M.npz'
data = np.load(file_path)
t = data['times'][:N]  # Spike times in seconds
d = data['depths'][:N]  # Depth in microns

# Normalization.
t_min, t_max = t.min(), t.max()
t_norm = 2 * (t - t_min) / (t_max - t_min) - 1

d_min, d_max = d.min(), d.max()
d_norm = 2 * (d - d_min) / (d_max - d_min) - 1

positions = np.c_[t_norm, d_norm, np.zeros_like(t_norm)].astype(np.float32)
num_points = positions.shape[0]

colors = np.full((num_points, 4), 0, dtype=np.uint8)
colors[:, 3] = 64

# -------------------------------------------------------------------------------------------------

app = dvz.App(background='white')
figure = app.figure()
panel = figure.panel()
panzoom = panel.panzoom()
camera = panel.camera(initial=(0, 0, 1.5))

visual = app.pixel(position=positions, color=colors)
panel.add(visual)

app.run()
app.destroy()
