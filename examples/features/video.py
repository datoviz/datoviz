"""
# Video generation

Show how to generate an offscreen video.

!!! warning

    This feature is experimental, the API is likely to change in a future version.

---
tags:
  - offscreen
  - video
dependencies:
  - imageio
  - tqdm
in_gallery: true
make_screenshot: false
---

"""

import os

import numpy as np

try:
    import imageio
    import tqdm
except ImportError:
    print('This example requires the tqdm and imageio dependencies. Aborting')
    exit()

import datoviz as dvz
from datoviz import vec3

# Image size.
WIDTH, HEIGHT = 1920, 1280

# Initialize Datoviz scene.
server = dvz.server(0)
scene = dvz.scene(None)
batch = dvz.scene_batch(scene)
figure = dvz.figure(scene, WIDTH, HEIGHT, 0)
panel = dvz.panel_default(figure)
visual = dvz.demo_panel_3D(panel)
arcball = dvz.panel_arcball(panel, 0)
camera = dvz.panel_camera(panel, 0)


# Rendering function.
def render(angle):
    # Update the arcball angle.
    dvz.arcball_set(arcball, vec3(0, angle, 0))
    dvz.panel_update(panel)

    # Render the scene.
    dvz.scene_render(scene, server)

    # Get the image as a NumPy array (3*uint8 for RGB components).
    rgb = dvz.server_grab(server, dvz.figure_id(figure), 0)
    img = dvz.utils.pointer_image(rgb, WIDTH, HEIGHT)
    return img


# Make the video.
fps = 60  # number of frames per second in the video
laps = 1  # number of rotations
lap_duration = 4.0  # duration of each rotation
frame_count = int(lap_duration * laps * fps)  # total number of frames to generate
# path to video file to write
output_file = 'video.mp4'
kwargs = dict(
    fps=fps,
    format='FFMPEG',
    mode='I',
    # Quality FFMPEG presets
    codec='libx264',
    output_params=(
        '-preset slow -crf 18 -color_range 1 -colorspace bt709 '
        '-color_primaries bt709 -color_trc bt709'
    ).split(' '),
    pixelformat='yuv420p',
)
if 'DVZ_CAPTURE' not in os.environ:  # HACK: avoid recording the video with `just runexamples`
    with imageio.get_writer(output_file, **kwargs) as writer:
        for angle in tqdm.tqdm(np.linspace(0, 2 * np.pi, frame_count)[:-1]):
            writer.append_data(render(angle))

# Cleanup.
dvz.server_destroy(server)
