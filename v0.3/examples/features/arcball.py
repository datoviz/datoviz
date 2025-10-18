"""
# Arcball

Show how to manipulate an arcball.

---
tags:
  - arcball
in_gallery: true
make_screenshot: false
---

"""

import datoviz as dvz

app = dvz.App()
# NOTE: at the moment, you must indicate gui=True if you intend to use a GUI in a figure
figure = app.figure(gui=True)
panel = figure.panel(background=True)
panel.demo_3D()

# Set initial angles for the arcball (which modifies the model matrix).
arcball = panel.arcball(initial=(-1.5, 0.0, +1.5))

# Display a little GUI widget with sliders to control the arcball angles.
panel.arcball_gui()

# Angles can be set and retrieved as follows:
angles = (-1.5, 0.0, +2.5)
arcball.set(angles)
angles = arcball.get()
print('Arcball angles:', angles)

app.run()
app.destroy()
