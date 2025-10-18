"""
manual_test_fullscreen.py

1: Start in window mode.
    Switch to fullscreen, then back to window mode.

2: Start in fullscreen mode.
    Switch to window, then back to fullscreen mode.

"""

import time
import numpy as np
import datoviz as dvz
import imageio.v3 as iio


def load_image():
    filepath = dvz.download_data('textures/image.png')
    arr = iio.imread(filepath)
    h, w, _ = arr.shape
    return np.dstack((arr, np.full((h, w), 255))).astype(np.uint8)


image = load_image()
image_size = image.shape[:2]
texcoords = np.array([[0, 0, 1, 1]], dtype=np.float32)


def show_image(fullscreen: bool = False, resize: bool = False) -> None:
    app = dvz.App()
    figure = app.figure(fullscreen=fullscreen)
    panel = figure.panel(background=True)

    # Add content to panel.
    visual = app.image(
        size=[image_size],  # image size in pixels.
        texcoords=texcoords,
    )
    texture = app.texture_2D(image=image, interpolation='linear')  # by default, no interpolation
    visual.set_texture(texture)
    panel.add(visual)

    # Execute case for each timer call
    case = 0
    print(f'case: {case}, fullscreen: {fullscreen}')

    @app.timer(period=2, delay=2)
    def on_timer(ev):
        nonlocal case, fullscreen
        case += 1
        if case < 3:
            fullscreen = not fullscreen
            print(f'Case: {case}, fullscreen: {fullscreen}')

            figure.set_fullscreen(fullscreen)
        else:
            app.stop()

    app.run()
    app.destroy()


def show_visual(fullscreen: bool = False, resize: bool = False) -> None:
    app = dvz.App()
    figure = app.figure(fullscreen=fullscreen)

    panel = figure.panel()
    panel.demo_3D()

    # Execute case for each timer call
    case = 0
    print(f'case: {case}, fullscreen: {fullscreen}')

    @app.timer(period=2, delay=2)
    def on_timer(ev):
        nonlocal case, fullscreen
        case += 1
        if case < 3:
            fullscreen = not fullscreen
            print(f'Case: {case}, fullscreen: {fullscreen}')

            figure.set_fullscreen(fullscreen)
        else:
            app.stop()

    app.run()
    app.destroy()

    time.sleep(2)
    print()


if __name__ == '__main__':
    show_image(fullscreen=False)
    show_image(fullscreen=True)

    show_visual(fullscreen=False)
    show_visual(fullscreen=True)
