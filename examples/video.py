"""# Video example

X

Illustrates:

- X

"""

from pathlib import Path
import numpy as np

try:
    import tqdm
    import imageio
except ImportError as e:
    print("This example requires tqdm and imageio dependencies. Aborting")
    exit()

import datoviz as dvz
from datoviz import vec3


# Image size.
WIDTH, HEIGHT = 1280, 960

# Initialize Datoviz scene.
server = dvz.server(0)
scene = dvz.scene(None)
batch = dvz.scene_batch(scene)
figure = dvz.figure(scene, WIDTH, HEIGHT, 0)
panel = dvz.panel(figure, 0, 0, WIDTH, HEIGHT)
arcball = dvz.panel_arcball(panel)
camera = dvz.panel_camera(panel, 0)

# Cube with one different color per face.
colors = np.zeros((6, 4), dtype=np.uint8)
colors[:, 3] = 255
colors[[0, 1, 2], [0, 1, 2]] = 255
colors[3:, :] = 255
colors[[3, 4, 5], [0, 1, 2]] = 0
shape = dvz.shape_cube(colors)


# Mesh visual.
visual = dvz.mesh_shape(batch, shape, dvz.MESH_FLAGS_LIGHTING)
dvz.panel_visual(panel, visual, 0)

# Change the camera position.
dvz.camera_position(camera, vec3(0, 1.5, 3))


# Rendering function.
def render(angle):
    # Update the arcball angle.
    dvz.arcball_set(arcball, vec3(0, angle, 0))
    dvz.panel_update(panel)

    # Render the scene.
    dvz.scene_render(scene, server)

    # Get the image as a NumPy array (3*uint8 for RGB components).
    rgb = dvz.server_grab(server, dvz.figure_id(figure), 0)
    img = dvz.pointer_image(rgb, WIDTH, HEIGHT)
    return img


# Make the video.
fps = 30  # number of frames per second in the video
laps = 2  # number of cube rotations
lap_duration = 2.0  # duration of each rotation
frame_count = int(lap_duration * laps * fps)  # total number of frames to generate
output_file = Path(__file__).parent / "video.mp4"  # path to video file to write

with imageio.get_writer(output_file, fps=fps) as writer:
    for angle in tqdm.tqdm(np.linspace(0, 2 * np.pi, frame_count)[:-1]):
        writer.append_data(render(angle))


# Cleanup.
dvz.server_destroy(server)
