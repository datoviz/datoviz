"""# Spheres example

Show fake 3D spheres and static text with manual camera control.

Illustrates:

- Adding multiple visuals to a panel
- Sphere visual
- Glyph (text) visual
- Dynamic and static visual (visual opting out of the global panel transform)
- Keyboard event callbacks
- Manual camera control

"""

import numpy as np
import datoviz as dvz
from datoviz import vec2, vec3, vec4, S_


# -------------------------------------------------------------------------------------------------
# 1. Creating the scene
# -------------------------------------------------------------------------------------------------

# Boilerplate.
app = dvz.app(0)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)

# Create a figure.
figure = dvz.figure(scene, 1000, 1000, 0)

# Panel spanning the entire window.
panel = dvz.panel_default(figure)

# 3D camera.
camera = dvz.panel_camera(panel, 0)


# -------------------------------------------------------------------------------------------------
# 2. Text
# -------------------------------------------------------------------------------------------------

# Show a static glyph.
glyph = dvz.glyph(batch, 0)

# First, we load the default font (Roboto) with a given font size, and we load the pre-generated
# glyph atlas.
# NOTE: generating custom atlases dynamically with arbitrary TTF fonts (using the msdfgen library)
# is possible but undocumented yet.
font_size = 32
af = dvz.atlas_font(font_size)
dvz.glyph_atlas(glyph, af.atlas)

# Glyph text.
text = "Press the arrow keys!"

# We specify the number of glyphs.
n = len(text)
dvz.glyph_alloc(glyph, n)

# When displaying a single string, all glyph share the exact same position in 3D space, BUT
# each glyph has a fixed pixel offset due to its relative position within the string (see below).
# Here, the string will be displayed at (1, 1, 0) (we will not use the panel camera transform).
pos = np.c_[np.ones(n), np.ones(n), np.zeros(n)].astype(np.float32)
dvz.glyph_position(glyph, 0, n, pos, 0)

# We can assign a different color per glyph.
color = np.full((n, 4), 255, dtype=np.uint8)
dvz.glyph_color(glyph, 0, n, color, 0)

# We specify the ASCII string (we could also specify unicode uint32 codepoints with glyph_unicode)
# NOTE: we need to use S_() to pass a Python string to this ctypes-wrapped C function expecting
# a const char*.
dvz.glyph_ascii(glyph, S_(text))

# Now we compute the glyph shifts (called "xywh") using our font.
xywh = dvz.font_ascii(af.font, S_(text))
# We also define a global relative anchor point, in pixels (xy), for the string.
#  By default, the anchor is (0, 0) which represents the lower left corner of the string. The
# anchor position is the string position defined above (1, 1, 0).
anchor = vec2(-.5 * font_size * len(text), -2 * font_size)
dvz.glyph_xywh(glyph, 0, n, xywh, anchor, 0)


# -------------------------------------------------------------------------------------------------
# 3. Spheres
# -------------------------------------------------------------------------------------------------

# Now we define a fake sphere visual, similar to markers, but with a fake 3D effect to simulate
# spheres whereas they are really 2D bitmap sprites in a 3D world.
# See https://paroj.github.io/gltut/Illumination/Tutorial%2013.html
visual = dvz.sphere(batch, 0)

# Sphere data allocation (100 000 spheres).
n = 100_000
dvz.sphere_alloc(visual, n)

# Sphere random positions.
pos = np.random.uniform(size=(n, 3), low=-1, high=+1).astype(np.float32)
pos *= np.array([100, 1, 100])
dvz.sphere_position(visual, 0, n, pos, 0)

# Sphere random colors.
color = np.random.uniform(size=(n, 4), low=50, high=200).astype(np.uint8)
color[:, 3] = 255
dvz.sphere_color(visual, 0, n, color, 0)

# Sphere sizes in pixels.
size = np.random.uniform(size=(n,), low=50, high=100).astype(np.float32)
dvz.sphere_size(visual, 0, n, size, 0)

# Light position.
dvz.sphere_light_pos(visual, vec3(-5, +5, +100))

# Light parameters.
dvz.sphere_light_params(visual, vec4(.4, .8, 2, 32))


# -------------------------------------------------------------------------------------------------
# 4. Panel composition
# -------------------------------------------------------------------------------------------------

# We add the sphere visual.
dvz.panel_visual(panel, visual, 0)

# We add the glyph visual and we opt out of the panel transform (3D movable camera).
dvz.panel_visual(panel, glyph, dvz.VIEW_FLAGS_STATIC)


# -------------------------------------------------------------------------------------------------
# 5. Manual camera control
# -------------------------------------------------------------------------------------------------

# Custom camera manipulation with the keyboard.
# NOTE: a similar interaction pattern will be soon provided as a builtin option in Datoviz
# (similar to the existing panzoom and arcball).

# Initial camera position.
eye = vec3(0, 0, 4)

# Camera movement offset.
d = .2


# Keyboard event callback function.
@dvz.keyboard
def on_keyboard(app, window_id, ev):
    global eye
    # Keyboard events are PRESS, RELEASE, and REPEAT.
    if ev.type != dvz.KEYBOARD_EVENT_RELEASE:
        # Move the camera position depending on the pressed keys.
        if ev.key == dvz.KEY_UP:
            eye[2] -= d
        elif ev.key == dvz.KEY_DOWN:
            eye[2] += d
        elif ev.key == dvz.KEY_LEFT:
            eye[0] -= d
        elif ev.key == dvz.KEY_RIGHT:
            eye[0] += d

        # Update the camera position.
        dvz.camera_position(camera, eye)

        # Update the lookat position (just forward looking).
        lookat = vec3(*eye)
        lookat[2] -= 1
        dvz.camera_lookat(camera, lookat)

        # Important: we must update the panel after the panel transformation parameters
        # have changed.
        dvz.panel_update(panel)


# We register the keyboard callback function.
dvz.app_onkeyboard(app, on_keyboard, None)


# -------------------------------------------------------------------------------------------------
# 6. Run and cleanup
# -------------------------------------------------------------------------------------------------

# Run the application.
dvz.scene_run(scene, app, 0)

# Cleanup.
dvz.atlas_destroy(af.atlas)
dvz.font_destroy(af.font)
dvz.scene_destroy(scene)
dvz.app_destroy(app)
