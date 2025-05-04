"""
# Offscreen example

Show how to render an offscreen image.

NOTE: the API for this feature may change in an upcoming version.

"""

import datoviz as dvz

app = dvz.App(offscreen=True)
figure = app.figure()
panel = figure.panel()
panel.demo_2D()

# Save a PNG screenshot.
app.screenshot(figure, 'offscreen_python.png')

app.destroy()
