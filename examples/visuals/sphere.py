"""# Sphere visual example

Show the sphere visual.

"""

import numpy as np
import datoviz as dvz
from datoviz import vec3, vec4


def generate_ndc_grid(n):
    lin = np.linspace(-1, 1, n)
    x, y, z = np.meshgrid(lin, lin, lin, indexing='ij')
    positions = np.stack([x, y, z], axis=-1).reshape(-1, 3)

    # Normalize each coordinate to [0, 1] for radius/color mapping
    x_norm = (x + 1) / 2
    y_norm = (y + 1) / 2
    z_norm = (z + 1) / 2

    # Radius increases linearly in all directions (can be tuned)
    radius = 10 + 5 * np.exp(1 * (x_norm + y_norm + z_norm))
    # radius = radius.max() + radius.min() - radius
    radius = radius.flatten()

    r = x_norm.flatten()
    g = y_norm.flatten()
    b = z_norm.flatten()
    a = np.ones_like(r)
    rgb = np.stack([r[::-1], g[::-1], b, a], axis=1)
    rgb = (255 * rgb).astype(np.uint8)

    return radius.size, positions, rgb, radius


N, position, color, size = generate_ndc_grid(8)

app = dvz.App()
figure = app.figure()
panel = figure.panel()
arcball = panel.arcball()

visual = app.sphere(
    position=position, color=color, size=size,
    light_pos=(-5, +5, +100), light_params=(.4, .8, 2, 32))
panel.add(visual)

# dvz.arcball_initial(arcball, vec3(.6, .1, 1.5))
# dvz.panel_update(panel)

app.run()
app.destroy()
