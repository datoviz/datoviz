"""# Basic visual example

Show a colored triangle using a basic visual.

Illustrates:

- Creating a figure, panel
- Basic visual
- Vertex color interpolation

"""

import numpy as np
import datoviz as dvz

# Boilerplate.
app = dvz.app(0)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)

# Create a figure 800x600.
figure = dvz.figure(scene, 800, 600, 0)

# Panel spanning the entire window.
panel = dvz.panel_default(figure)

# Basic visual.
visual = dvz.basic(batch, dvz.PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0)

# Visual data allocation.
dvz.basic_alloc(visual, 3)

# Positions.
pos = np.array([
    [-1, -1, 0],
    [0, 1, 0],
    [+1, -1, 0],
]).astype(np.float32)
dvz.basic_position(visual, 0, 3, pos, 0)

# Colors.
color = np.array(
    [[255, 0, 0, 255],
     [0, 255, 0, 255],
     [0, 0, 255, 255],
     ]).astype(np.uint8)
dvz.basic_color(visual, 0, 3, color, 0)

# Add the visual.
dvz.panel_visual(panel, visual, 0)

# Run the application.
dvz.scene_run(scene, app, 0)

# Cleanup.
dvz.scene_destroy(scene)
dvz.app_destroy(app)
