"""
# Fly camera controller

Show how to manipulate a fly camera controller.

- Left mouse drag: Look around (yaw/pitch)
- Right mouse drag: Orbit around a dynamic center (in front of the camera)
- Middle mouse drag: Move the camera left/right and up/down
- Arrow keys: Move in view direction (up/down) or strafe (left/right)

---
tags:
  - fly
in_gallery: true
make_screenshot: true
---

"""

import datoviz as dvz

app = dvz.App()
figure = app.figure()
panel = figure.panel(background=True)

# Set a fly camera controller.
fly = panel.fly(initial=(0, 0.5, 4), initial_lookat=(0, 0, 0))

# Add a horizontal grid.
grid = panel.horizontal_grid(elevation=-0.62)

# Add a mesh.
file_path = dvz.download_data('mesh/bunny.obj')
sc = dvz.ShapeCollection()
sc.add_obj(file_path, contour='full')
visual = app.mesh(sc, contour=True, lighting=True)
panel.add(visual)

app.run()
app.destroy()
