"""
# Axes example

Show how to use 2D axes.

"""

import numpy as np

import datoviz as dvz

n = 10
xmin, xmax = 1, 10
ymin, ymax = 100, 1000
x, y = np.meshgrid(np.linspace(xmin, xmax, n), np.linspace(ymin, ymax, n))
nn = x.size
color = np.random.randint(low=100, high=240, size=(nn, 4)).astype(np.uint8)
size = np.full(nn, 20)

app = dvz.App(background='white')
figure = app.figure()
panel = figure.panel()
axes = panel.axes((xmin, xmax), (ymin, ymax))

visual = app.point(
    position=axes.normalize(x, y),
    color=color,
    size=size,
)
panel.add(visual)

app.run()
app.destroy()
