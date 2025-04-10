"""# Volume example

Show a 3D volume.

Illustrates:

- Creating a figure, panel
- Arcball interactivity
- Loading a volume from file
- Creating a 3D texture
- Volume visual

"""

import gzip
from pathlib import Path
import numpy as np
import datoviz as dvz
from datoviz import vec2, vec3, vec4


# -------------------------------------------------------------------------------------------------
# 1. Creating the scene
# -------------------------------------------------------------------------------------------------

# Boilerplate.
app = dvz.app(0)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)

# Create a figure 800x600.
figure = dvz.figure(scene, 800, 600, 0)

# Panel spanning the entire window.
panel = dvz.panel_default(figure)

# Arcball interactivity.
arcball = dvz.panel_arcball(panel)


# -------------------------------------------------------------------------------------------------
# 2. Loading the volume and creating the 3D GPU texture
# -------------------------------------------------------------------------------------------------

# Load a volume file.
CURDIR = Path(__file__).parent
filepath = (CURDIR / "../data/volumes/allen_mouse_brain_rgba.npy.gz").resolve()
with gzip.open(filepath, 'rb') as f:
    volume_data = np.load(f)
shape = volume_data.shape

# Volume parameters.
MOUSE_D, MOUSE_H, MOUSE_W = shape[:3]
scaling = 1.0 / MOUSE_D

# Create the 3D texture.
format = dvz.FORMAT_R8G8B8A8_UNORM
tex = dvz.tex_volume(batch, format, MOUSE_W, MOUSE_H, MOUSE_D, volume_data)


# -------------------------------------------------------------------------------------------------
# 3. Volume visual
# -------------------------------------------------------------------------------------------------

# Create the volume visual.
visual = dvz.volume(batch, dvz.VOLUME_FLAGS_RGBA)

# Bind the volume texture to the visual.
volume_tex = dvz.volume_texture(
    visual, tex, dvz.FILTER_LINEAR, dvz.SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)

# Volume parameters.
x, y, z = MOUSE_W * scaling, MOUSE_H * scaling, 1
dvz.volume_bounds(visual, vec2(-x, +x), vec2(-y, +y), vec2(-z, +z))
dvz.volume_transfer(visual, vec4(1, 0, 0, 0))


# Add the visual to the panel AFTER setting the visual's data.
dvz.panel_visual(panel, visual, 0)


# -------------------------------------------------------------------------------------------------
# 4. Initial panel parameters
# -------------------------------------------------------------------------------------------------

# Initial arcball angles.
dvz.arcball_initial(arcball, vec3(-2.25, 0.65, 1.5))

# Initial camera position.
camera = dvz.panel_camera(panel, 0)
dvz.camera_initial(camera, vec3(0, 0, 1.5), vec3(), vec3(0, 1, 0))

# Update the panel after updating the arcball and camera.
dvz.panel_update(panel)


# -------------------------------------------------------------------------------------------------
# 5. Run and cleanup
# -------------------------------------------------------------------------------------------------

# Run the application.
dvz.scene_run(scene, app, 0)

# Cleanup.
dvz.scene_destroy(scene)
dvz.app_destroy(app)
