"""
# Fly camera controller

Show how to manipulate a fly camera controller.

- Left mouse drag: Look around (yaw/pitch)
- Right mouse drag: Move the camera left/right and up/down
- Arrow keys: Move in view direction (up/down) or strafe (left/right)

---
tags:
  - fly
in_gallery: true
make_screenshot: false
---

"""

import datoviz as dvz

app = dvz.App()
# NOTE: at the moment, you must indicate gui=True if you intend to use a GUI in a figure
figure = app.figure()
panel = figure.panel(background=True)

# Set a fly camera controller.
fly = panel.fly()

# Add a mesh.
file_path = dvz.download_data('mesh/bunny.obj')
sc = dvz.ShapeCollection()
sc.add_obj(file_path, contour='full')
visual = app.mesh(sc, contour=True, lighting=True)
panel.add(visual)

app.run()
app.destroy()
