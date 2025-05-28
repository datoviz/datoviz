"""
# Camera

Show how to manipulate a camera.

---
tags:
  - camera
  - keyboard
in_gallery: true
make_screenshot: false
---

"""

import datoviz as dvz

app = dvz.App()
figure = app.figure()
panel = figure.panel(background=True)
panel.demo_3D()

# Camera initial parameters (the ones used when calling camera_reset()).
position = (0, 0, 2)
up = (0, 1, 0)
lookat = (0, 0, 0)
# Get or create the panel's 3D perspective camera.
camera = panel.camera(initial=position, initial_up=up, initial_lookat=lookat)

d = 0.1
mapping = {
    'up': (2, -d),
    'down': (2, +d),
    'left': (0, -d),
    'right': (0, +d),
}


@app.connect(figure)
def on_keyboard(ev):
    # Keyboard events are PRESS, RELEASE, and REPEAT.
    if ev.key_event() != 'release':
        # Move the camera position depending on the pressed keys.
        i, dp = mapping.get(ev.key_name(), (0, 0))
        position = list(camera.position())
        position[i] += dp
        lookat = (position[0], position[1], position[2] - 1)

        # Update the camera.
        camera.set(position=position, lookat=lookat)


app.run()
app.destroy()
