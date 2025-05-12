# Quick start

This page presents a simple quick-start Python script that displays a 2D scatter plot with axes and mouse-based pan and zoom interactivity.

You need a Python installation with NumPy, and you must install Datoviz using `pip install datoviz` before running the script.


![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/quickstart.png)

```python

# Import necessary libraries
import numpy as np  # Import NumPy to generate the data

import datoviz as dvz  # Import Datoviz

# Number of points in the scatter plot.
n = 1000

# Generate random x and y coordinates for the points.
x = np.random.normal(scale=0.2, size=n)
y = np.random.normal(scale=0.2, size=n)

# Generate random colors for each point.
# Colors are represented as RGBA values (Red, Green, Blue, Alpha).
# The values are integers between 0 and 255 for RGB, and 255 for alpha (full opacity).
color = np.random.randint(size=(n, 4), low=100, high=240, dtype=np.uint8)
color[:, 3] = 255

# Generate random sizes for each point, in pixels.
size = np.random.uniform(low=10, high=30, size=n)

# Create a Datoviz application with a white background.
app = dvz.App(background='white')

# Create a 800x600 px figure.
figure = app.figure(800, 600)

# Add a panel to the figure, like a subplot. By default, it spans the entire window.
# A panel also acts as a container for visual elements.
panel = figure.panel()

# Define the axes limits for the scatter plot.
xmin, xmax = -1, +1  # x-axis range
ymin, ymax = -1, +1  # y-axis range

# Add 2D axes to the panel with the specified limits. This automatically adds pan-zoom
# interactivity with the mouse.
axes = panel.axes((xmin, xmax), (ymin, ymax))

# Create a Point visual (scatter plot) using the generated data.
# The `axes.normalize()` function maps the x and y coordinates to the normalized range [-1, 1],
# which is the range expected for the position of all visuals in Datoviz.
# The Datoviz coordinate system is the normalized device coordinate system [-1, +1] on all
# three axes.
visual = app.point(
    position=axes.normalize(x, y),  # normalized 3D positions of the points (with z=0 by default)
    color=color,  # colors of the points
    size=size,  # pixel sizes of the points
)

# Add our visual to the panel.
panel.add(visual)

# Run the application to display the visualization. Start the internal Datoviz event loop
# (using glfw by default for now).
app.run()

# Destroy the application after closing the visualization window.
app.destroy()

```
