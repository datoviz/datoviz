# Examples

* [Datoviz Features Examples](#datoviz-features-examples)
    * [Arcball example](#arcball-example)
    * [Camera example](#camera-example)
    * [Datoviz Rendering Protocol (DRP) example](#datoviz-rendering-protocol-(drp)-example)
    * [GUI example](#gui-example)
    * [GUI panel example](#gui-panel-example)
    * [Visibility example](#visibility-example)
    * [Keyboard example](#keyboard-example)
    * [Mouse example](#mouse-example)
    * [Offscreen example](#offscreen-example)
    * [Panel example](#panel-example)
    * [Panzoom example](#panzoom-example)
    * [PyQt6 local example](#pyqt6-local-example)
    * [Timestamps example](#timestamps-example)
    * [Video example](#video-example)
* [C Examples](#c-examples)
    * [Scatter plot example](#scatter-plot-example)
    * [Datoviz Rendering Protocol example](#datoviz-rendering-protocol-example)
    * [Offscreen example](#offscreen-example)
    * [Scatter plot example](#scatter-plot-example)


# Datoviz Features Examples

## Arcball example

Show how to manipulate an arcball.

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/arcball.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/arcball.py</code></summary>

```python
import datoviz as dvz
from datoviz import vec3

app = dvz.app(0)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)
# NOTE: at the moment, you need to set this flag when creating a figure if you intend to use ImGui.
figure = dvz.figure(scene, 800, 600, dvz.CANVAS_FLAGS_IMGUI)
panel = dvz.panel_default(figure)
visual = dvz.demo_panel_3D(panel)

# Get or create an arcball interaction for a panel.
arcball = dvz.panel_arcball(panel)

# Set initial angles for the arcball (which modifies the model matrix).
dvz.arcball_initial(arcball, vec3(-1.5, 0.0, +1.5))

# NOTE: at the moment, we need to tell Datoviz that the panel transform has changed.
dvz.panel_update(panel)

# Display a little GUI widget with sliders to control the arcball angles.
dvz.arcball_gui(arcball, app, dvz.figure_id(figure), panel)

dvz.scene_run(scene, app, 0)
dvz.scene_destroy(scene)
dvz.app_destroy(app)
```
</details>

## Camera example

Show how to manipulate a camera.

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/camera.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/camera.py</code></summary>

```python
import datoviz as dvz
from datoviz import vec3

app = dvz.app(0)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)
# NOTE: at the moment, you need to set this flag when creating a figure if you intend to use ImGui.
figure = dvz.figure(scene, 800, 600, 0)
panel = dvz.panel_default(figure)
visual = dvz.demo_panel_3D(panel)

# Get or create the panel's 3D perspective camera.
camera = dvz.panel_camera(panel, 0)

# Camera initial parameters (the ones used when calling camera_reset()).
eye = vec3(0, 0, 2)
up = vec3(0, 1, 0)
lookat = vec3(0, 0, 0)
dvz.camera_initial(camera, eye, lookat, up)

# NOTE: at the moment, we need to tell Datoviz that the panel transform has changed.
dvz.panel_update(panel)

# Keyboard event callback function.


@dvz.keyboard
def on_keyboard(app, window_id, ev):
    global eye

    # Camera movement offset.
    d = .1

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

        # Optional here, this is just to show how to update the up vector of the camera.
        dvz.camera_up(camera, vec3(0, 1, 0))

        # Important: we must update the panel after the panel transformation parameters
        # have changed.
        dvz.panel_update(panel)


# We register the keyboard callback function.
dvz.app_on_keyboard(app, on_keyboard, None)

dvz.scene_run(scene, app, 0)
dvz.scene_destroy(scene)
dvz.app_destroy(app)
```
</details>

## Datoviz Rendering Protocol (DRP) example

Show a simple triangle using raw DRP requests.

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/drp.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/drp.py</code></summary>

```python
import numpy as np
import datoviz as dvz

app = dvz.app(0)
batch = dvz.app_batch(app)

# Constants.
width = 1024
height = 768

# Define the Vertex dtype
vertex_dtype = np.dtype([
    ('pos', np.float32, (3,)),  # 3D position (vec3)
    ('color', np.uint8, (4,))   # RGBA color (cvec4)
])
vertex_size = vertex_dtype.itemsize
pos_offset = vertex_dtype.fields['pos'][1]
color_offset = vertex_dtype.fields['color'][1]


# Create a canvas.
req = dvz.create_canvas(batch, width, height, dvz.DEFAULT_CLEAR_COLOR, 0)
canvas_id = req.id


# Create a custom graphics.
req = dvz.create_graphics(batch, dvz.GRAPHICS_CUSTOM, 0)
graphics_id = req.id


# Vertex shader.
vertex_glsl = """
#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 color;
layout(location = 0) out vec4 out_color;

void main()
{
    gl_Position = vec4(pos, 1.0);
    out_color = color;
}
"""

req = dvz.create_glsl(
    batch, dvz.SHADER_VERTEX, vertex_glsl)

# Assign the shader to the graphics pipe.
vertex_id = req.id
dvz.set_shader(batch, graphics_id, vertex_id)


# Fragment shader.
fragment_glsl = """
#version 450

layout(location = 0) in vec4 in_color;
layout(location = 0) out vec4 out_color;

void main()
{
    out_color = in_color;
}
"""

req = dvz.create_glsl(
    batch, dvz.SHADER_FRAGMENT, fragment_glsl)

# Assign the shader to the graphics pipe.
fragment_id = req.id
dvz.set_shader(batch, graphics_id, fragment_id)


# Primitive topology.
dvz.set_primitive(batch, graphics_id, dvz.PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)

# Polygon mode.
dvz.set_polygon(batch, graphics_id, dvz.POLYGON_MODE_FILL)


# Vertex binding.
dvz.set_vertex(
    batch, graphics_id, 0, vertex_size, dvz.VERTEX_INPUT_RATE_VERTEX)

# Vertex attrs.
dvz.set_attr(batch, graphics_id, 0, 0, dvz.FORMAT_R32G32B32_SFLOAT, pos_offset)
dvz.set_attr(batch, graphics_id, 0, 1, dvz.FORMAT_R8G8B8A8_UNORM, color_offset)


# Create the vertex buffer dat.
req = dvz.create_dat(batch, dvz.BUFFER_TYPE_VERTEX, 3 * vertex_size, 0)
dat_id = req.id

# Bind the vertex buffer dat to the graphics pipe.
req = dvz.bind_vertex(batch, graphics_id, 0, dat_id, 0)

# Upload the triangle data.
data = np.array([
    ((-1, +1, 0), (255, 0, 0, 255)),
    ((+1, +1, 0), (0, 255, 0, 255)),
    ((+0, -1, 0), (0, 0, 255, 255)),
], dtype=vertex_dtype)
req = dvz.upload_dat(batch, dat_id, 0, 3 * vertex_size, data, 0)


# Commands.
dvz.record_begin(batch, canvas_id)
dvz.record_viewport(
    batch, canvas_id, dvz.DEFAULT_VIEWPORT, dvz.DEFAULT_VIEWPORT)
dvz.record_draw(batch, canvas_id, graphics_id, 0, 3, 0, 1)
dvz.record_end(batch, canvas_id)


# Run the application.

# NOTE: disabling this example for now as the current stable version of Datoviz is NOT built with
# shaderc support, due to compatibility issues on Linux. We'll fix it later.
# dvz.app_run(app, 0)

# Cleanup.
dvz.app_destroy(app)
```
</details>

## GUI example

Show how to create a GUI dialog.

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/gui.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/gui.py</code></summary>

```python
import ctypes
import numpy as np
import datoviz as dvz
from datoviz import vec2, vec3, Out

app = dvz.app(0)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)
# NOTE: at the moment, you need to set this flag when creating a figure if you intend to use ImGui.
figure = dvz.figure(scene, 800, 600, dvz.CANVAS_FLAGS_IMGUI)

# Dialog width.
w = 300

labels = [
    "col0", "col1", "col2",
    "0",    "1",    "2",
    "3",    "4",    "5"]
rows = 2
cols = 3
selected = np.array([False, True], dtype=np.bool)

# IMPORTANT: these values need to be defined outside of the GUI callback.
checked = Out(True)
color = vec3(.7, .5, .3)

slider = Out(25.0)  # Warning: needs to be a float as it is passed to a function expecting a float

# GUI callback function, called at every frame. This is using Dear ImGui, an immediate-mode
# GUI system. This means the GUI is recreated from scratch at every frame.


@dvz.gui
def on_gui(app, fid, ev):

    # Set the size of the next GUI dialog.
    dvz.gui_pos(vec2(25, 25), vec2(0, 0))
    dvz.gui_size(vec2(w + 20, 550))

    # Start a GUI dialog, specifying a dialog title.
    dvz.gui_begin("My GUI", 0)

    # Add a button. The function returns whether the button was pressed during this frame.
    if dvz.gui_button("Button", w, 30):
        print("button clicked")

    # Create a tree, this call returns True if this node is unfolded.
    if dvz.gui_node("Item 1"):
        # Display an item in the tree.
        dvz.gui_selectable("Hello inside item 1")
        # Return True if this item was clicked.
        if dvz.gui_clicked():
            print("clicked sub item 1")
        # Go up one level.
        dvz.gui_pop()

    if dvz.gui_node("Item 2"):
        if dvz.gui_node("Item 2.1"):
            dvz.gui_selectable("Hello inside item 2")
            if dvz.gui_clicked():
                print("clicked sub item 2")
            dvz.gui_pop()
        dvz.gui_pop()

    if dvz.gui_table("table", rows, cols, labels, selected, 0):
        print("Selected rows:", np.nonzero(selected)[0])

    if dvz.gui_checkbox("Checkbox", checked):
        print("Checked status:", checked.value)

    if dvz.gui_colorpicker("Color picker", color, 0):
        print("Color:", color)

    if dvz.gui_slider("Slider", 0.0, 100.0, slider):
        print("Slider value:", slider.value)

    # End the GUI dialog.
    dvz.gui_end()


# Associate a GUI callback function with a figure.
dvz.app_gui(app, dvz.figure_id(figure), on_gui, None)

dvz.scene_run(scene, app, 0)
dvz.scene_destroy(scene)
dvz.app_destroy(app)
```
</details>

## GUI panel example

Show how to create a GUI panel.

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/gui_panel.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/gui_panel.py</code></summary>

```python
import datoviz as dvz

app = dvz.app(0)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)
# NOTE: at the moment, you need to set this flag when creating a figure if you intend to use ImGui.
figure = dvz.figure(scene, 800, 600, dvz.CANVAS_FLAGS_IMGUI)

# Create a panel, specifying the panel offset and size (x, y, width, height, in pixels).
panel1 = dvz.panel(figure, 50, 50, 300, 300)
dvz.demo_panel_3D(panel1)

# Wrap a panel in a GUI dialog.
dvz.panel_gui(panel1, "Panel 1", 0)

panel2 = dvz.panel(figure, 400, 100, 300, 300)
dvz.demo_panel_2D(panel2)
dvz.panel_gui(panel2, "Panel 2", 0)

dvz.scene_run(scene, app, 0)
dvz.scene_destroy(scene)
dvz.app_destroy(app)
```
</details>

## Visibility example

Show how to show/hide a visual.

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/hide.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/hide.py</code></summary>

```python
import ctypes
import numpy as np
import datoviz as dvz
from datoviz import vec2, vec3, Out

app = dvz.app(0)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)
# NOTE: at the moment, you need to set this flag when creating a figure if you intend to use ImGui.
figure = dvz.figure(scene, 800, 600, dvz.CANVAS_FLAGS_IMGUI)
panel = dvz.panel_default(figure)
visual = dvz.demo_panel_2D(panel)

visible = Out(True)


@dvz.gui
def on_gui(app, fid, ev):
    dvz.gui_begin("GUI", 0)
    if dvz.gui_checkbox("Visible?", visible):
        dvz.visual_show(visual, visible.value)
        dvz.figure_update(figure)
    dvz.gui_end()


dvz.app_gui(app, dvz.figure_id(figure), on_gui, None)

dvz.scene_run(scene, app, 0)
dvz.scene_destroy(scene)
dvz.app_destroy(app)
```
</details>

## Keyboard example

Show how to react to keyboard events.

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/keyboard.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/keyboard.py</code></summary>

```python
import datoviz as dvz

app = dvz.app(0)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)
figure = dvz.figure(scene, 800, 600, 0)


@dvz.keyboard
def on_keyboard(app, window_id, ev):
    action = {dvz.KEYBOARD_EVENT_RELEASE: "released",
              dvz.KEYBOARD_EVENT_PRESS: "pressed"}.get(ev.type)
    print(f"{action} key {ev.key} ({dvz.key_name(ev.key)})")


dvz.app_on_keyboard(app, on_keyboard, None)

dvz.scene_run(scene, app, 0)
dvz.scene_destroy(scene)
dvz.app_destroy(app)
```
</details>

## Mouse example

Show how to react to mouse events.

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/mouse.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/mouse.py</code></summary>

```python
import datoviz as dvz

app = dvz.app(0)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)
figure = dvz.figure(scene, 800, 600, 0)


@dvz.on_mouse
def on_mouse(app, window_id, ev):
    action = dvz.from_enum(dvz.MouseEventType, ev.type)
    x, y = ev.pos
    print(f"{action} ({x:.0f}, {y:.0f}) ", end="")

    if ev.type in (dvz.MOUSE_EVENT_CLICK, dvz.MOUSE_EVENT_DOUBLE_CLICK):
        button = ev.button
        print(f"{dvz.button_name(button)} button", end="")

    if ev.type in (dvz.MOUSE_EVENT_DRAG_START, dvz.MOUSE_EVENT_DRAG_STOP, dvz.MOUSE_EVENT_DRAG):
        button = ev.button
        xd, yd = ev.content.d.press_pos
        print(f"{dvz.button_name(button)} button pressed at ({xd:.0f}, {yd:.0f})", end="")

    if ev.type == dvz.MOUSE_EVENT_WHEEL:
        w = ev.content.w.dir[1]
        print(f"wheel direction {w}", end="")

    print()


dvz.app_on_mouse(app, on_mouse, None)

dvz.scene_run(scene, app, 0)
dvz.scene_destroy(scene)
dvz.app_destroy(app)
```
</details>

## Offscreen example

Show how to render an offscreen image.

NOTE: the API for this feature may change in an upcoming version.

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/offscreen.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/offscreen.py</code></summary>

```python
import datoviz as dvz

app = dvz.app(dvz.APP_FLAGS_OFFSCREEN)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)
figure = dvz.figure(scene, 800, 600, 0)
dvz.demo_panel_2D(dvz.panel_default(figure))

# Need to run at least one frame before capturing a screenshot.
dvz.scene_run(scene, app, 1)

# Save a PNG screenshot.
dvz.app_screenshot(app, dvz.figure_id(figure), "offscreen_python.png")

dvz.scene_destroy(scene)
dvz.app_destroy(app)
```
</details>

## Panel example

Show how to create several panels.

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/panel.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/panel.py</code></summary>

```python
import datoviz as dvz

app = dvz.app(0)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)
figure = dvz.figure(scene, 800, 600, 0)

# Create two panels side-by-side.
panel1 = dvz.panel(figure, 0, 0, 400, 600)
panel2 = dvz.panel(figure, 400, 0, 400, 600)

# Add demo visuals to the panels.
visual1 = dvz.demo_panel_2D(panel1)
visual2 = dvz.demo_panel_3D(panel2)

# Set some margins for the first panel, which affects the panel's coordinate systems.
# [-1, +1] map to the "inner" viewport.
dvz.panel_margins(panel1, 20, 100, 20, 20)  # top, right, bottom, left, like in CSS
# Indicate that the first visual should be hidden inside the margins, outside of [-1, +1].
dvz.visual_clip(visual1, dvz.VIEWPORT_CLIP_OUTER)

dvz.scene_run(scene, app, 0)
dvz.scene_destroy(scene)
dvz.app_destroy(app)
```
</details>

## Panzoom example

Show how to manipulate a panzoom.

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/panzoom.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/panzoom.py</code></summary>

```python
import datoviz as dvz
from datoviz import dvec2

app = dvz.app(0)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)
figure = dvz.figure(scene, 800, 600, 0)
panel = dvz.panel_default(figure)
panzoom = dvz.panel_panzoom(panel)
visual = dvz.demo_panel_2D(panel)

# Create a data coordinate system [0, 100] x [0, 10].
ref = dvz.ref(0)
dvz.ref_set(ref, dvz.DIM_X, 0, 100)
dvz.ref_set(ref, dvz.DIM_Y, 0, 10)

# When passed a zero vector, dvz.panzoom_xlim() returns the current xmin and xmax in data
# coordinates.
xlim = dvec2(0)
dvz.panzoom_xlim(panzoom, ref, xlim)

# When passed a non-zero vector, dvz.panzoom_xlim() sets the current xmin and xmax.
xlim[1] /= 2.0
dvz.panzoom_xlim(panzoom, ref, xlim)

# NOTE: at the moment, we need to tell Datoviz that the panel transform has changed.
dvz.panel_update(panel)

dvz.scene_run(scene, app, 0)

dvz.ref_destroy(ref)
dvz.scene_destroy(scene)
dvz.app_destroy(app)
```
</details>

## PyQt6 local example

Show how to integrate offscreen Datoviz figures into a PyQt6 application, using the Datoviz
server API which provides a fully offscreen renderer with support for multiple canvases.

NOTE: this API will change in an upcoming release.

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/pyqt_offscreen.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/pyqt_offscreen.py</code></summary>

```python
import sys

try:
    from PyQt6.QtWidgets import QApplication, QMainWindow, QSplitter
    from PyQt6.QtCore import Qt
except:
    from PyQt5.QtWidgets import QApplication, QMainWindow, QSplitter
    from PyQt5.QtCore import Qt

import datoviz as dvz
from datoviz.backends.pyqt6 import QtServer


WIDTH, HEIGHT = 800, 600


class ExampleWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Example Qt Datoviz window")

        # Create a Qt Datoviz server.
        self.qt_server = QtServer()

        # Create two figures (special Qt widgets with a Datoviz figure).
        w, h = WIDTH // 2, HEIGHT
        self.qt_figure1 = self.qt_server.create_figure(w, h)
        self.qt_figure2 = self.qt_server.create_figure(w, h)

        # Fill the figures with mock data.
        dvz.demo_panel_2D(dvz.panel(self.qt_figure1.figure, 0, 0, w, h))
        dvz.demo_panel_2D(dvz.panel(self.qt_figure2.figure, 0, 0, w, h))

        # Add the two figures in the main window.
        splitter = QSplitter(Qt.Orientation.Horizontal)
        splitter.addWidget(self.qt_figure1)
        splitter.addWidget(self.qt_figure2)
        splitter.setCollapsible(0, False)
        splitter.setCollapsible(1, False)
        self.setCentralWidget(splitter)
        self.resize(WIDTH, HEIGHT)


if __name__ == "__main__":
    app = QApplication(sys.argv)
    mw = ExampleWindow()
    mw.show()
    sys.exit(app.exec())
```
</details>

## Timestamps example

Show how to retrieve the exact timestamps of the presentation of the last frames to the screen.
This may be useful in specific use-cases (e.g. hardware synchronization in scientific experimental
setups).

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/timestamps.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/timestamps.py</code></summary>

```python
import numpy as np
import datoviz as dvz

app = dvz.app(0)
batch = dvz.app_batch(app)
scene = dvz.scene(batch)
figure = dvz.figure(scene, 800, 600, 0)
panel = dvz.panel_default(figure)
dvz.demo_panel_2D(panel)

# Frame presentation timestamps.
# Every second, we show the timestamps of the last `count` frames.
count = 5

# We prepare the arrays holding the data.
seconds = np.zeros(count, dtype=np.uint64)  # epoch, in seconds
nanoseconds = np.zeros(count, dtype=np.uint64)  # number of ns within the second


@dvz.timer
def on_timer(app, window_id, ev):
    #  The timestamps are automatically recorded at every frame, this call fetches the last
    # `count` ones.
    dvz.app_timestamps(app, dvz.figure_id(figure), count, seconds, nanoseconds)

    # We display the values.
    print(f"Last {count} frames:")
    print(np.c_[seconds, nanoseconds])


# Timer: retrieve and display the timestamps every second.
# NOTE: it is currently impossible to call dvz.app_timestamps() after the window has been closed.
dvz.app_ontimer(app, on_timer, None)
dvz.app_timer(app, 0, 1, 0)

# Run the application and cleanup.
dvz.scene_run(scene, app, 0)
dvz.scene_destroy(scene)
dvz.app_destroy(app)
```
</details>

## Video example

Show how to generate an offscreen video.

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/video.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/video.py</code></summary>

```python
from pathlib import Path
import os
import numpy as np

try:
    import tqdm
    import imageio
except ImportError as e:
    print("This example requires the tqdm and imageio dependencies. Aborting")
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
arcball = dvz.panel_arcball(panel)
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
    img = dvz.pointer_image(rgb, WIDTH, HEIGHT)
    return img


# Make the video.
fps = 60  # number of frames per second in the video
laps = 1  # number of rotations
lap_duration = 4.0  # duration of each rotation
frame_count = int(lap_duration * laps * fps)  # total number of frames to generate
# path to video file to write
output_file = Path(__file__).parent / "video.mp4"
kwargs = dict(
    fps=fps,
    format="FFMPEG",
    mode="I",
    # Quality FFMPEG presets
    codec="libx264",
    output_params=(
        "-preset slow -crf 18 -color_range 1 -colorspace bt709 "
        "-color_primaries bt709 -color_trc bt709"
    ).split(" "),
    pixelformat="yuv420p",
)
if 'DVZ_CAPTURE' not in os.environ:  # HACK: avoid recording the video with `just runexamples`
    with imageio.get_writer(output_file, **kwargs) as writer:
        for angle in tqdm.tqdm(np.linspace(0, 2 * np.pi, frame_count)[:-1]):
            writer.append_data(render(angle))

# Cleanup.
dvz.server_destroy(server)
```
</details>

# C Examples

Scatter plot example

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/brain.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/brain.py</code></summary>

```c
/*************************************************************************************************/
/*  Scatter plot example                                                                         */
/*************************************************************************************************/

/// We import the library public header.
#include <datoviz.h>


// Callback function called at every mouse event (mouse, click, drag...)
static void show_arcball_angles(DvzApp* app, DvzId window_id, DvzMouseEvent ev)
{
    ANN(app);

    // We only run the callback function when mouse drag stops (button down, move, button up).
    if (ev.type != DVZ_MOUSE_EVENT_DRAG_STOP)
        return;

    // The user data is passed as last argument in dvz_app_on_mouse().
    DvzArcball* arcball = (DvzArcball*)ev.user_data;
    ANN(arcball);

    // Get the arcball angles and display them.
    vec3 angles = {0};
    dvz_arcball_angles(arcball, angles);
    printf("Arcball angles: %.02f, %.02f, %.02f\n", angles[0], angles[1], angles[2]);
}


// Entry point.
int main(int argc, char** argv)
{
    // Create app object.
    DvzApp* app = dvz_app(0);
    DvzBatch* batch = dvz_app_batch(app);

    // Create a scene.
    DvzScene* scene = dvz_scene(batch);

    // Create a figure.
    DvzFigure* figure = dvz_figure(scene, 800, 600, DVZ_CANVAS_FLAGS_FPS);

    // Create a panel.
    DvzPanel* panel = dvz_panel_default(figure);

    // Arcball.
    DvzArcball* arcball = dvz_panel_arcball(panel);

    // Set the initial arcball angles.
    dvz_arcball_initial(arcball, (vec3){+0.6, -1.2, +3.0});
    dvz_panel_update(panel); // IMPORTANT after changing the interactivity parameters

    // File path to a .obj file.
    // This is a 3D mesh reconstruction of a mouse brain, provided by the Allen Institute.
    char path[1024] = {0};
    snprintf(path, sizeof(path), "data/mesh/brain.obj");

    // Load the obj file.
    DvzShape shape = dvz_shape_obj(path);
    if (!shape.vertex_count)
    {
        dvz_shape_destroy(&shape);
        return 0;
    }

    // Set the color of every vertex (the shape comes with an already allocated color array).
    for (uint32_t i = 0; i < shape.vertex_count; i++)
    {
        // Generate colors using the "bwr" colormap, in reverse (blue -> red).
        // dvz_colormap_scale(
        //     DVZ_CMAP_COOLWARM, shape.vertex_count - 1 - i, 0, shape.vertex_count,
        //     shape.color[i]);
        // shape.color[i][0] = shape.color[i][1] = shape.color[i][2] = 128;
        shape.color[i][3] = 32;
    }

    // Create a mesh visual with basic lightingsupport.
    DvzVisual* visual = dvz_mesh_shape(batch, &shape, DVZ_MESH_FLAGS_LIGHTING);

    // NOTE: transparent meshes require special care.
    dvz_visual_depth(visual, DVZ_DEPTH_TEST_DISABLE); // disable depth test
    dvz_visual_cull(visual, DVZ_CULL_MODE_BACK);      // cull mode
    dvz_visual_blend(visual, DVZ_BLEND_OIT);          // special, imperfect order-independent blend
    dvz_mesh_light_params(visual, 0, (vec4){.75, .1, .1, 16}); // light parameters

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(panel, visual, 0);

    // Print the arcball angles in the terminal.
    dvz_app_on_mouse(app, show_arcball_angles, arcball);

    // Run the app.
    dvz_scene_run(scene, app, 0);

    // Cleanup.
    dvz_scene_destroy(scene);
    dvz_app_destroy(app);

    return 0;
}

```
</details>

Datoviz Rendering Protocol example

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/drp.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/drp.py</code></summary>

```c
/*************************************************************************************************/
/*  Datoviz Rendering Protocol example                                                           */
/*************************************************************************************************/

// Imports.
#include <datoviz_protocol.h>
#include <stddef.h>

// Entry point.
int main(int argc, char** argv)
{
    // Create app object.
    DvzApp* app = dvz_app(0);
    DvzBatch* batch = dvz_app_batch(app);
    DvzRequest req = {0};

    // Constants.
    uint32_t width = 1024;
    uint32_t height = 768;

    // Structure holding the vertex data.
    struct Vertex
    {
        vec3 pos;
        DvzColor color;
    };


    // Create a canvas.
    req = dvz_create_canvas(batch, width, height, DVZ_DEFAULT_CLEAR_COLOR, 0);
    DvzId canvas_id = req.id;


    // Create a custom graphics.
    req = dvz_create_graphics(batch, DVZ_GRAPHICS_CUSTOM, 0);
    DvzId graphics_id = req.id;


    // Vertex shader.
    const char* vertex_glsl = //
        "#version 450\n"
        "\n"
        "layout(location = 0) in vec3 pos;\n"
        "layout(location = 1) in vec4 color;\n"
        "layout(location = 0) out vec4 out_color;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(pos, 1.0);\n"
        "    out_color = color;\n"
        "}\n";
    req = dvz_create_glsl(batch, DVZ_SHADER_VERTEX, vertex_glsl);

    // Assign the shader to the graphics pipe.
    DvzId vertex_id = req.id;
    dvz_set_shader(batch, graphics_id, vertex_id);


    // Fragment shader.
    const char* fragment_glsl = //
        "#version 450\n"
        "\n"
        "layout(location = 0) in vec4 in_color;\n"
        "layout(location = 0) out vec4 out_color;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    out_color = in_color;\n"
        "}\n";
    req = dvz_create_glsl(batch, DVZ_SHADER_FRAGMENT, fragment_glsl);

    // Assign the shader to the graphics pipe.
    DvzId fragment_id = req.id;
    dvz_set_shader(batch, graphics_id, fragment_id);


    // Primitive topology.
    dvz_set_primitive(batch, graphics_id, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    // Polygon mode.
    dvz_set_polygon(batch, graphics_id, DVZ_POLYGON_MODE_FILL);


    // Vertex binding.
    dvz_set_vertex(batch, graphics_id, 0, sizeof(struct Vertex), DVZ_VERTEX_INPUT_RATE_VERTEX);

    // Vertex attrs.
    dvz_set_attr(
        batch, graphics_id, 0, 0, DVZ_FORMAT_R32G32B32_SFLOAT, offsetof(struct Vertex, pos));
    dvz_set_attr(
        batch, graphics_id, 0, 1, DVZ_FORMAT_R8G8B8A8_UNORM, offsetof(struct Vertex, color));


    // Create the vertex buffer dat.
    req = dvz_create_dat(batch, DVZ_BUFFER_TYPE_VERTEX, 3 * sizeof(struct Vertex), 0);
    DvzId dat_id = req.id;

    // Bind the vertex buffer dat to the graphics pipe.
    req = dvz_bind_vertex(batch, graphics_id, 0, dat_id, 0);

    // Upload the triangle data.
    struct Vertex data[] = {
        {{-1, +1, 0}, {255, 0, 0, 255}},
        {{+1, +1, 0}, {0, 255, 0, 255}},
        {{+0, -1, 0}, {0, 0, 255, 255}},
    };
    req = dvz_upload_dat(batch, dat_id, 0, sizeof(data), data, 0);


    // Commands.
    dvz_record_begin(batch, canvas_id);
    dvz_record_viewport(batch, canvas_id, DVZ_DEFAULT_VIEWPORT, DVZ_DEFAULT_VIEWPORT);
    dvz_record_draw(batch, canvas_id, graphics_id, 0, 3, 0, 1);
    dvz_record_end(batch, canvas_id);


    // Run the application.
    dvz_app_run(app, 0);

    // Cleanup.
    dvz_app_destroy(app);

    return 0;
}

```
</details>

Offscreen example

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/offscreen.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/offscreen.py</code></summary>

```c
/*************************************************************************************************/
/*  Offscreen example                                                                            */
/*************************************************************************************************/

/// We import the library public header.
#include <datoviz.h>

// Entry point.
int main(int argc, char** argv)
{
    // Create app object.
    DvzApp* app = dvz_app(DVZ_APP_FLAGS_OFFSCREEN);
    DvzBatch* batch = dvz_app_batch(app);

    // Create a scene.
    DvzScene* scene = dvz_scene(batch);

    // Create a figure.
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);

    // Create a panel.
    DvzPanel* panel = dvz_panel_default(figure);

    // Panzoom.
    DvzPanzoom* pz = dvz_panel_panzoom(panel);

    // Create a visual.
    DvzVisual* visual = dvz_point(batch, 0);

    // Allocate a number of points.
    const uint32_t n = 10000;
    dvz_point_alloc(visual, n);

    // Set the point positions.
    vec3* pos = dvz_mock_pos_2D(n, 0.25);
    dvz_point_position(visual, 0, n, pos, 0);
    FREE(pos);

    // Set the point RGBA colors.
    DvzColor* color = dvz_mock_color(n, 128);
    dvz_point_color(visual, 0, n, color, 0);
    FREE(color);

    // Set the point sizes.
    float* size = dvz_mock_uniform(n, 25, 50);
    dvz_point_size(visual, 0, n, size, 0);
    FREE(size);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(panel, visual, 0);

    // Run the app.
    dvz_scene_run(scene, app, 0);

    // Screenshot.
    dvz_app_screenshot(app, dvz_figure_id(figure), "offscreen_example.png");

    // Cleanup.
    dvz_scene_destroy(scene);
    dvz_app_destroy(app);

    return 0;
}

```
</details>

Scatter plot example

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/scatter.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/scatter.py</code></summary>

```c
/*************************************************************************************************/
/*  Scatter plot example                                                                         */
/*************************************************************************************************/

/// We import the library public header.
#include <datoviz.h>

// Entry point.
int main(int argc, char** argv)
{
    // Create app object.
    DvzApp* app = dvz_app(0);
    DvzBatch* batch = dvz_app_batch(app);

    // Create a scene.
    DvzScene* scene = dvz_scene(batch);

    // Create a figure.
    DvzFigure* figure = dvz_figure(scene, 800, 600, 0);

    // Create a panel.
    DvzPanel* panel = dvz_panel_default(figure);

    // Panzoom.
    DvzPanzoom* pz = dvz_panel_panzoom(panel);

    // Create a visual.
    DvzVisual* visual = dvz_point(batch, 0);

    // Allocate a number of points.
    const uint32_t n = 10000;
    dvz_point_alloc(visual, n);

    // Set the point positions.
    vec3* pos = dvz_mock_pos_2D(n, 0.25);
    dvz_point_position(visual, 0, n, pos, 0);
    FREE(pos);

    // Set the point RGBA colors.
    DvzColor* color = dvz_mock_color(n, 128);
    dvz_point_color(visual, 0, n, color, 0);
    FREE(color);

    // Set the point sizes.
    float* size = dvz_mock_uniform(n, 25, 50);
    dvz_point_size(visual, 0, n, size, 0);
    FREE(size);

    // Add the visual to the panel AFTER setting the visual's data.
    dvz_panel_visual(panel, visual, 0);

    // Run the app.
    dvz_scene_run(scene, app, 0);

    // Cleanup.
    dvz_scene_destroy(scene);
    dvz_app_destroy(app);

    return 0;
}

```
</details>

