"""# GUI panels example

Show several dockable panels.

Illustrates:

- Creating dockable panels
- Volume visual

"""

import gzip
from pathlib import Path
import numpy as np
import datoviz as dvz
from datoviz import vec2, vec3, vec4


# -------------------------------------------------------------------------------------------------
# Functions
# -------------------------------------------------------------------------------------------------

def load_mouse_volume():
    CURDIR = Path(__file__).parent
    filepath = (CURDIR / "../data/volumes/allen_mouse_brain_rgba.npy.gz").resolve()
    with gzip.open(filepath, 'rb') as f:
        volume_data = np.load(f)
    shape = volume_data.shape
    MOUSE_D, MOUSE_H, MOUSE_W = shape[:3]
    format = dvz.FORMAT_R8G8B8A8_UNORM
    texture = dvz.texture_3D(
        batch, format, dvz.FILTER_LINEAR,
        dvz.SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, MOUSE_W, MOUSE_H, MOUSE_D, volume_data, 0)
    return texture, MOUSE_D, MOUSE_H, MOUSE_W


def create_volume(texture, MOUSE_D, MOUSE_H, MOUSE_W):
    volume = dvz.volume(batch, dvz.VOLUME_FLAGS_RGBA)

    scaling = 1.0 / MOUSE_D
    dvz.volume_texture(volume, texture)

    x, y, z = MOUSE_W * scaling, MOUSE_H * scaling, 1
    dvz.volume_bounds(volume, vec2(-x, +x), vec2(-y, +y), vec2(-z, +z))

    dvz.volume_transfer(volume, vec4(1, 0, 0, 0))
    return volume


# -------------------------------------------------------------------------------------------------
# Main script
# -------------------------------------------------------------------------------------------------

app = dvz.app(0)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)
figure = dvz.figure(scene, 800, 600, dvz.CANVAS_FLAGS_IMGUI)


# Volume panel
# -------------------------------------------------------------------------------------------------

panel1 = dvz.panel(figure, 100, 100, 300, 200)
arcball = dvz.panel_arcball(panel1)
volume = create_volume(*load_mouse_volume())
dvz.panel_visual(panel1, volume, 0)
dvz.panel_gui(panel1, "Panel 1", 0)

# Initial view
dvz.arcball_initial(arcball, vec3(-2.25, 0.65, 1.5))
camera = dvz.panel_camera(panel1, 0)
dvz.camera_initial(camera, vec3(0, 0, 1.5), vec3(), vec3(0, 1, 0))
dvz.panel_update(panel1)


# Scatter panel
# -------------------------------------------------------------------------------------------------

panel2 = dvz.panel(figure, 200, 350, 300, 200)
pz = dvz.panel_panzoom(panel2)
visual = dvz.point(batch, 0)

n = 10_000
dvz.point_alloc(visual, n)

pos = np.random.normal(size=(n, 3), scale=.2).astype(np.float32)
color = np.random.uniform(size=(n, 4), low=50, high=240).astype(np.uint8)
size = np.random.uniform(size=(n,), low=10, high=30).astype(np.float32)

dvz.point_position(visual, 0, n, pos, 0)
dvz.point_color(visual, 0, n, color, 0)
dvz.point_size(visual, 0, n, size, 0)
dvz.panel_visual(panel2, visual, 0)
dvz.panel_gui(panel2, "Panel 2", 0)


# Run and cleanup
# -------------------------------------------------------------------------------------------------

dvz.scene_run(scene, app, 0)
dvz.scene_destroy(scene)
dvz.app_destroy(app)
