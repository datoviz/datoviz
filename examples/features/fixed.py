"""# Fixed example

Show how to fix a visual in the panel on one or several axes.

"""

import datoviz as dvz


app = dvz.App()
figure = app.figure()
panel = figure.panel()
visual = panel.demo_2D()
visual.fixed('y')  # or 'x', or 'z', or 'x, y'... or True for all axes

app.run()
app.destroy()
