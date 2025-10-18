"""
# Topologies of the Basic visual

Show the different topology possibilities of the basic visual.

---
tags:
  - basic
  - topology
  - gui
in_gallery: false
make_screenshot: false
---

"""

import sys

import numpy as np

import datoviz as dvz

OPTIONS = dvz.TOPOLOGY_OPTIONS

N = 30
t2 = np.linspace(-1.0, +1.0, 2 * N)
y1 = -0.1 + 0.25 * np.sin(2 * 2 * np.pi * t2[0::2])
y2 = +0.1 + 0.25 * np.sin(2 * 2 * np.pi * t2[1::2])

y = np.c_[y1, y2].ravel()

position = np.c_[t2, y, np.zeros(2 * N)]
group = np.repeat([0, 1], N)
color = dvz.cmap('hsv', t2, vmin=-1, vmax=+1)

app = dvz.App()
figure = app.figure(gui=True)
panel = figure.panel()
panzoom = panel.panzoom()
visual = None


def set_primitive(primitive):
    global visual
    if visual:
        panel.remove(visual)
    visual = app.basic(primitive, position=position, color=color, group=group, size=5)
    panel.add(visual)


set_primitive(sys.argv[1] if len(sys.argv) >= 2 else 'point_list')

selected = dvz.Out(0)


@app.connect(figure)
def on_gui(ev):
    dvz.gui_pos(dvz.vec2(10, 10), dvz.vec2(0, 0))
    dvz.gui_size(dvz.vec2(300, 80))
    dvz.gui_begin('Primitive topology', 0)
    if dvz.gui_dropdown('Topology', len(OPTIONS), list(OPTIONS), selected, 0):
        primitive = OPTIONS[selected.value]
        set_primitive(primitive)
    dvz.gui_end()


app.run()
app.destroy()
