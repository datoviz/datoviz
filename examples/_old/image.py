"""# Image example

Show an image.

Illustrates:

- Creating a figure, panel
- Panzoom interactivity
- Loading a PNG image with pillow
- Image visual
- Creating a texture

"""

from pathlib import Path
import numpy as np
from PIL import Image

import datoviz as dvz

# Boilerplate.
app = dvz.app(0)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)


# Load a PNG image.
CURDIR = Path(__file__).parent
filepath = CURDIR / "../data/textures/image.png"
with Image.open(filepath) as f:
    image = np.array(f.convert('RGBA'), dtype=np.uint8)
    height, width = image.shape[:2]

    # Texture parameters.
    format = dvz.FORMAT_R8G8B8A8_UNORM
    filter = dvz.FILTER_LINEAR
    address_mode = dvz.SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER

    # Create a texture out of a RGB image.
    tex = dvz.texture_2D(batch, format, filter, address_mode, width, height, image, 0)


# Create a figure 1000x1000.
figure = dvz.figure(scene, 1000, 1000, 0)

# Panel spanning the entire window.
panel = dvz.panel_default(figure)

# Panzoom interactivity.
pz = dvz.panel_panzoom(panel)

# Image visual.
visual = dvz.image(batch, dvz.IMAGE_FLAGS_RESCALE)

# One image in this visual, there could be multiple images sharing the same underlying texture.
dvz.image_alloc(visual, 1)

# xyz coordinates of the top left corner.
pos = np.array([[0, 0, 0]], dtype=np.float32)
dvz.image_position(visual, 0, 1, pos, 0)

# Image size, in pixels.
size = np.array([[width, height]], dtype=np.float32)
dvz.image_size(visual, 0, 1, size, 0)

# Image anchor.
anchor = np.array([[0, 0]], dtype=np.float32)
dvz.image_anchor(visual, 0, 1, anchor, 0)

# uv coordinates of the top left corner, and bottom right corner.
texcoords = np.array([[0, 0, 1, 1]], dtype=np.float32)
dvz.image_texcoords(visual, 0, 1, texcoords, 0)


# Assign the texture to the visual.
dvz.image_texture(visual, tex, filter, address_mode)

# Add the visual.
dvz.panel_visual(panel, visual, 0)

# Run the application.
dvz.scene_run(scene, app, 0)

# Cleanup.
dvz.scene_destroy(scene)
dvz.app_destroy(app)
