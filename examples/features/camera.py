"""# Camera example

Show how to manipulate a camera.

"""

import datoviz as dvz

app = dvz.App()
# NOTE: at the moment, you must indicate gui=True if you intend to use a GUI in a figure
figure = app.figure()
panel = figure.panel()
panel.demo_3D()

# Camera initial parameters (the ones used when calling camera_reset()).
eye = [0, 0, 2]
up = [0, 1, 0]
lookat = [0, 0, 0]
# Get or create the panel's 3D perspective camera.
camera = panel.camera(initial=eye, initial_up=up, initial_lookat=lookat)

d = .1
mapping = {
    dvz.KEY_UP: (2, -d),
    dvz.KEY_DOWN: (2, +d),
    dvz.KEY_LEFT: (0, -d),
    dvz.KEY_RIGHT: (0, +d),
}


@app.on_keyboard(figure)
def on_keyboard(ev):
    global eye

    # Keyboard events are PRESS, RELEASE, and REPEAT.
    if ev.type != dvz.KEYBOARD_EVENT_RELEASE:
        # Move the camera position depending on the pressed keys.
        i, dp = mapping.get(ev.key, (0, 0))
        eye[i] += dp
        lookat = (eye[0], eye[1], eye[2] - 1)

        # Update the camera.
        camera.set(eye=eye, lookat=lookat, up=up)


app.run()
app.destroy()
