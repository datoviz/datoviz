"""# Image example

Show an image.

"""

from pathlib import Path
import numpy as np
from PIL import Image

import datoviz as dvz
from datoviz import A_

# Boilerplate.
app = dvz.app(0)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)

# Create a figure 1000x1000.
figure = dvz.figure(scene, 1000, 1000, 0)

# Panel spanning the entire window.
panel = dvz.panel_default(figure)

# Panzoom interactivity.
pz = dvz.panel_panzoom(panel)

# Image visual.
visual = dvz.image(batch, 0)

# One image in this visual, there could be multiple images sharing the same underlying texture.
dvz.image_alloc(visual, 1)

# xy coordinates of the upper left corner, and lower right corner
pos = np.array([[-1, +1], [+1, -1]], dtype=np.float32)
dvz.image_position(visual, 0, 1,  pos, 0)

# uv coordinates of the upper left corner, and lower right corner
texcoords = np.array([[0, 0], [1, 1]], dtype=np.float32)
dvz.image_texcoords(visual, 0, 1, texcoords, 0)

# Load a PNG image.
CURDIR = Path(__file__).parent
filepath = CURDIR / "../data/textures/image.png"
with Image.open(filepath) as f:
    image = np.array(f.convert('RGBA'), dtype=np.uint8)
height, width = image.shape[:2]

# Texture parameters.
format = dvz.DvzFormat.DVZ_FORMAT_R8G8B8A8_UNORM
address_mode = dvz.DvzSamplerAddressMode.DVZ_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
filter = dvz.DvzFilter.DVZ_FILTER_LINEAR

# Create a texture out of a RGB image.
tex = dvz.tex_image(batch, format, width, height, A_(image))

# Assign the texture to the visual.
dvz.image_texture(visual, tex, filter, address_mode)

# Add the visual.
dvz.panel_visual(panel, visual)

# Run the application.
dvz.scene_run(scene, app, 0)

# Cleanup.
dvz.scene_destroy(scene)
dvz.app_destroy(app)
