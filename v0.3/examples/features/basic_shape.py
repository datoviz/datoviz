"""
# Basic visual from a ShapeCollection

This example demonstrates how to create a **Basic** visual from a `ShapeCollection` in Datoviz.

Compared to the **Mesh** visual, the **Basic** visual is much lighter and more efficient, but it does
not support lighting, texturing, or wireframes. It only allows a single color per vertex.

---
tags:
  - shape
  - arcball
  - basic
in_gallery: true
make_screenshot: true
---

"""

import numpy as np

import datoviz as dvz

rng = np.random.default_rng(seed=3141)

# Load the bunny mesh.
sc = dvz.ShapeCollection()
file_path = dvz.download_data('mesh/bunny.obj')
sc.add_obj(file_path)

# Initialize the scene.
app = dvz.App()
figure = app.figure()
panel = figure.panel(background=True)
arcball = panel.arcball(initial=(0.35, 0, 0))
camera = panel.camera(initial=(0, 0, 3))

# Random colors.
nv = sc.vertex_count()
color = rng.integers(low=0, high=255, size=(nv, 4), dtype=np.uint8)
color[:, 3] = 255  # Set alpha channel to fully opaque.

# Create a basic visual from the ShapeCollection.
visual = app.basic(shape=sc, color=color, depth_test=True)
panel.add(visual)

app.run()
app.destroy()
sc.destroy()
