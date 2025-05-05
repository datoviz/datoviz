# Examples

* [Datoviz Features Examples](#datoviz-features-examples)
    * [Arcball example](#arcball-example)
    * [Axes example](#axes-example)
    * [Camera example](#camera-example)
    * [Fixed example](#fixed-example)
    * [GUI example](#gui-example)
    * [GUI panel example](#gui-panel-example)
    * [Visibility example](#visibility-example)
    * [Keyboard example](#keyboard-example)
    * [Mouse example](#mouse-example)
    * [Mesh visual example](#mesh-visual-example)
    * [Offscreen example](#offscreen-example)
    * [Panel example](#panel-example)
    * [Polygon example](#polygon-example)
    * [Shapes](#shapes)
    * [Surface example](#surface-example)
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

app = dvz.App()
# NOTE: at the moment, you must indicate gui=True if you intend to use a GUI in a figure
figure = app.figure(gui=True)
panel = figure.panel()
panel.demo_3D()

# Set initial angles for the arcball (which modifies the model matrix).
arcball = panel.arcball(initial=(-1.5, 0.0, +1.5))

# Display a little GUI widget with sliders to control the arcball angles.
app.arcball_gui(panel, arcball)

# Angles can be set and retrieved as follws:
angles = (-1.5, 0.0, +2.5)
arcball.set(angles)
angles = arcball.get()
print('Arcball angles:', angles)

app.run()
app.destroy()
```
</details>

## Axes example

Show how to use 2D axes.

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/axes.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/axes.py</code></summary>

```python
import numpy as np

import datoviz as dvz

n = 10
xmin, xmax = 1, 10
ymin, ymax = 100, 1000
x, y = np.meshgrid(np.linspace(xmin, xmax, n), np.linspace(ymin, ymax, n))
nn = x.size
color = np.random.randint(low=100, high=240, size=(nn, 4)).astype(np.uint8)
size = np.full(nn, 20)

app = dvz.App(background='white')
figure = app.figure()
panel = figure.panel()
axes = panel.axes((xmin, xmax), (ymin, ymax))

visual = app.point(
    position=axes.normalize(x, y),
    color=color,
    size=size,
)
panel.add(visual)


@app.connect(figure)
def on_mouse(ev):
    if ev.mouse_event() == 'drag':
        xlim, ylim = axes.bounds()
        print(f'x: [{xlim[0]:g}, {xlim[1]:g}] ; y: [{ylim[0]:g}, {ylim[1]:g}]')


app.run()
app.destroy()
```
</details>

## Camera example

Show how to manipulate a camera.

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/camera.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/camera.py</code></summary>

```python
import datoviz as dvz

app = dvz.App()
# NOTE: at the moment, you must indicate gui=True if you intend to use a GUI in a figure
figure = app.figure()
panel = figure.panel()
panel.demo_3D()

# Camera initial parameters (the ones used when calling camera_reset()).
eye = (0, 0, 2)
up = (0, 1, 0)
lookat = (0, 0, 0)
# Get or create the panel's 3D perspective camera.
camera = panel.camera(initial=eye, initial_up=up, initial_lookat=lookat)

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
        eye = list(camera.eye())
        eye[i] += dp
        lookat = (eye[0], eye[1], eye[2] - 1)

        # Update the camera.
        camera.set(eye=eye, lookat=lookat)


app.run()
app.destroy()
```
</details>

## Fixed example

Show how to fix a visual in the panel on one or several axes.

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/fixed.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/fixed.py</code></summary>

```python
import datoviz as dvz

app = dvz.App()
figure = app.figure()
panel = figure.panel()
visual = panel.demo_2D()
visual.fixed('y')  # or 'x', or 'z', or 'x, y'... or True for all axes

app.run()
app.destroy()
```
</details>

## GUI example

Show how to create a GUI dialog.

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/gui.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/gui.py</code></summary>

```python
import numpy as np

import datoviz as dvz
from datoviz import Out, vec2, vec3

# Dialog width.
w = 300

labels = ['col0', 'col1', 'col2', '0', '1', '2', '3', '4', '5']
rows = 2
cols = 3
selected = np.array([False, True], dtype=np.bool)

# IMPORTANT: these values need to be defined outside of the GUI callback.
checked = Out(True)
color = vec3(0.7, 0.5, 0.3)

slider = Out(25.0)  # Warning: needs to be a float as it is passed to a function expecting a float

# GUI callback function, called at every frame. This is using Dear ImGui, an immediate-mode
# GUI system. This means the GUI is recreated from scratch at every frame.


app = dvz.App()
# NOTE: at the moment, you must indicate gui=True if you intend to use a GUI in a figure
figure = app.figure(gui=True)


@app.connect(figure)
def on_gui(ev):
    # Set the size of the next GUI dialog.
    dvz.gui_pos(vec2(25, 25), vec2(0, 0))
    dvz.gui_size(vec2(w + 20, 550))

    # Start a GUI dialog, specifying a dialog title.
    dvz.gui_begin('My GUI', 0)

    # Add a button. The function returns whether the button was pressed during this frame.
    if dvz.gui_button('Button', w, 30):
        print('button clicked')

    # Create a tree, this call returns True if this node is unfolded.
    if dvz.gui_node('Item 1'):
        # Display an item in the tree.
        dvz.gui_selectable('Hello inside item 1')
        # Return True if this item was clicked.
        if dvz.gui_clicked():
            print('clicked sub item 1')
        # Go up one level.
        dvz.gui_pop()

    if dvz.gui_node('Item 2'):
        if dvz.gui_node('Item 2.1'):
            dvz.gui_selectable('Hello inside item 2')
            if dvz.gui_clicked():
                print('clicked sub item 2')
            dvz.gui_pop()
        dvz.gui_pop()

    if dvz.gui_table('table', rows, cols, labels, selected, 0):
        print('Selected rows:', np.nonzero(selected)[0])

    if dvz.gui_checkbox('Checkbox', checked):
        print('Checked status:', checked.value)

    if dvz.gui_colorpicker('Color picker', color, 0):
        print('Color:', color)

    if dvz.gui_slider('Slider', 0.0, 100.0, slider):
        print('Slider value:', slider.value)

    # End the GUI dialog.
    dvz.gui_end()


app.run()
app.destroy()
```
</details>

## GUI panel example

Show how to create a GUI panel.

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/gui_panel.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/gui_panel.py</code></summary>

```python
import datoviz as dvz

app = dvz.App()
# NOTE: at the moment, you must indicate gui=True if you intend to use a GUI in a figure
figure = app.figure(gui=True)

# Create a panel, specifying the panel offset and size (x, y, width, height, in pixels).
panel1 = figure.panel((50, 50), (300, 300))
panel1.demo_3D()

panel2 = figure.panel((400, 100), (300, 300))
panel2.demo_2D()

# We transform the static panels into GUI panels (experimental).
panel1.gui('First panel')
panel2.gui('Second panel')

app.run()
app.destroy()
```
</details>

## Visibility example

Show how to show/hide a visual.

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/hide.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/hide.py</code></summary>

```python
import datoviz as dvz
from datoviz import Out

app = dvz.App()
# NOTE: at the moment, you must indicate gui=True if you intend to use a GUI in a figure
figure = app.figure(gui=True)
panel = figure.panel()
visual = panel.demo_2D()

visible = Out(True)


@app.connect(figure)
def on_gui(ev):
    dvz.gui_begin('GUI', 0)
    if dvz.gui_checkbox('Visible?', visible):
        visual.show(visible.value)
        figure.update()
    dvz.gui_end()


app.run()
app.destroy()
```
</details>

## Keyboard example

Show how to react to keyboard events.

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/keyboard.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/keyboard.py</code></summary>

```python
import datoviz as dvz

app = dvz.App()
figure = app.figure()


@app.connect(figure)
def on_keyboard(ev):
    print(f'{ev.key_event()} key {ev.key()} ({ev.key_name()})')


app.run()
app.destroy()
```
</details>

## Mouse example

Show how to react to mouse events.

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/mouse.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/mouse.py</code></summary>

```python
import datoviz as dvz

app = dvz.App()
figure = app.figure()


@app.connect(figure)
def on_mouse(ev):
    action = ev.mouse_event()
    x, y = ev.pos()
    print(f'{action} ({x:.0f}, {y:.0f}) ', end='')

    if action in ('click', 'double_click'):
        button = ev.button_name()
        print(f'{button} button', end='')

    if action in ('drag_start', 'drag_stop', 'drag'):
        button = ev.button_name()
        xd, yd = ev.press_pos()
        print(f'{button} button pressed at ({xd:.0f}, {yd:.0f})', end='')

    if action == 'wheel':
        w = ev.wheel()
        print(f'wheel direction {w}', end='')

    print()


app.run()
app.destroy()
```
</details>

## Mesh visual example

Show the mesh visual with predefined shapes.

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/obj.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/obj.py</code></summary>

```python
from pathlib import Path

import datoviz as dvz

ROOT_DIR = Path(__file__).resolve().parent.parent.parent
file_path = ROOT_DIR / 'data/mesh/bunny.obj'

linewidth = 0.1
edgecolor = (0, 0, 0, 96)
light_params = (0.25, 0.75, 0.25, 16)

sc = dvz.ShapeCollection()
sc.add_obj(file_path, contour='full')

app = dvz.App()
figure = app.figure()
panel = figure.panel()
arcball = panel.arcball(initial=(0.35, 0, 0))
camera = panel.camera(initial=(0, 0, 3))

visual = app.mesh_shape(
    sc, lighting=True, linewidth=linewidth, edgecolor=edgecolor, light_params=light_params
)
panel.add(visual)

app.run()
app.destroy()

sc.destroy()
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

app = dvz.App(offscreen=True)
figure = app.figure()
panel = figure.panel()
panel.demo_2D()

# Save a PNG screenshot.
app.screenshot(figure, 'offscreen_python.png')

app.destroy()
```
</details>

## Panel example

Show how to create several panels.

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/panel.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/panel.py</code></summary>

```python
import datoviz as dvz

app = dvz.App()
figure = app.figure()

# Create two panels side-by-side.
panel1 = figure.panel((0, 0), (400, 600))
panel2 = figure.panel((400, 0), (400, 600))

# Add demo visuals to the panels.
visual1 = panel1.demo_2D()
visual2 = panel2.demo_3D()

# Set some margins for the first panel, which affects the panel's coordinate systems.
# [-1, +1] map to the "inner" viewport.
panel1.margins(20, 100, 20, 20)  # top, right, bottom, left

# Indicate that the first visual should be hidden inside the margins, outside of [-1, +1].
visual1.clip('outer')

app.run()
app.destroy()
```
</details>

## Polygon example

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/polygon.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/polygon.py</code></summary>

```python
import numpy as np

import datoviz as dvz


def make_polygon(n, center, radius):
    # WARNING: watch the direction (-t) otherwise the contour_joints won't work!
    t = np.linspace(0, 2 * np.pi, n + 1) - ((np.pi / 2.0) if n != 4 else np.pi / 4)
    t = t[:-1]
    x = center[0] + radius * np.cos(-t)
    y = center[1] + radius * np.sin(-t)
    return np.c_[x, y]


# Generate the shapes.
r = 0.25
w = 0.9
shapes = []
sizes = (4, 5, 6, 8)
colors = dvz.cmap(dvz.CMAP_BWR, np.linspace(0, 1, 4))
sc = dvz.ShapeCollection()
for n, x, color in zip(sizes, np.linspace(-w, w, 4), colors):
    points = make_polygon(n, (x, 0), r)
    sc.add_polygon(points, color=color, contour=True)

app = dvz.App()
figure = app.figure()
panel = figure.panel()
ortho = panel.ortho()

visual = app.mesh_shape(sc, linewidth=15, edgecolor=(255, 255, 255, 200))
panel.add(visual)

app.run()
app.destroy()
sc.destroy()
```
</details>

## Shapes

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/shapes.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/shapes.py</code></summary>

```python
import numpy as np

import datoviz as dvz

rows = 12
cols = 16
N = rows * cols
t = np.linspace(0, 1, N)

x, y = np.meshgrid(np.linspace(-1, 1, rows), np.linspace(-1, 1, cols))
z = np.zeros_like(x)

offsets = np.c_[x.flat, y.flat, z.flat]
scales = 1.0 / rows * (1 + 0.25 * np.sin(5 * 2 * np.pi * t))
colors = dvz.cmap(dvz.CMAP_HSV, np.mod(t, 1))

sc = dvz.ShapeCollection()
for offset, scale, color in zip(offsets, scales, colors):
    sc.add_cube(offset=offset, scale=scale, color=color)

app = dvz.App()
figure = app.figure()
panel = figure.panel()
arcball = panel.arcball(initial=(-1, -0.1, -0.25))

visual = app.mesh_shape(sc, lighting=True)
panel.add(visual)

app.run()
app.destroy()
sc.destroy()
```
</details>

## Surface example

Show a rotating surface in 3D.

Illustrates:

- White background
- Surface shape
- Mesh visual and surface mesh
- Arcball interactivity
- Initial arcball angles

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/surface.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/surface.py</code></summary>

```python
import numpy as np

import datoviz as dvz

HAS_CONTOUR = True

# Grid parameters.
row_count = 200
col_count = row_count
# n = row_count * col_count

# Allocate heights and colors arrays.
grid = np.meshgrid(row_count, col_count)
shape = (row_count, col_count)
heights = np.zeros(shape, dtype=np.float32)

# Create grid of coordinates
x = np.arange(col_count)
y = np.arange(row_count)
xv, yv = np.meshgrid(x, y)

# Distances.
center_x = col_count / 2
center_y = row_count / 2
d = np.sqrt((xv - center_x) ** 2 + (yv - center_y) ** 2)

# Heights.
a = 4.0 * 2 * np.pi / row_count
b = 3.0 * 2 * np.pi / col_count
c = 0.5
hmin = -0.5
hmax = +0.5
heights = np.exp(-0.0001 * d**2) * np.sin(a * xv) * np.cos(b * yv)

# Colors.
colors = dvz.cmap(dvz.CMAP_PLASMA, heights, hmin, hmax)

linewidth = 0.1
edgecolor = (0, 0, 0, 64)

# -------------------------------------------------------------------------------------------------

sc = dvz.ShapeCollection()
sc.add_surface(heights=heights, colors=colors, contour='edges')

app = dvz.App(background='white')
figure = app.figure()
panel = figure.panel()
arcball = panel.arcball(initial=(0.42339, -0.39686, -0.00554))

visual = app.mesh_shape(
    sc, lighting=True, contour=HAS_CONTOUR, linewidth=linewidth, edgecolor=edgecolor
)
panel.add(visual)

app.run()
app.destroy()
sc.destroy()
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

app = dvz.App()
figure = app.figure()
panel = figure.panel()
panel.demo_2D()


@app.timer(delay=0.0, period=1.0, max_count=0)
def on_timer(ev):
    # Every second, we show the timestamps of the last `count` frames.
    # NOTE: it is currently impossible to call dvz.app_timestamps() after the window was closed.
    # The timestamps are automatically recorded at every frame, this call fetches the last 5.
    seconds, nanoseconds = app.timestamps(figure, 5)

    # We display the values.
    print('Last 5 frames:')
    print(np.c_[seconds, nanoseconds])


app.run()
app.destroy()
```
</details>

## Video example

Show how to generate an offscreen video.

NOTE: experimental, the API will change.

![](https://raw.githubusercontent.com/datoviz/data/main/screenshots/examples/video.png)

<details>
<summary><strong>👨‍💻 Expand the code</strong> from <code>examples/video.py</code></summary>

```python
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
static void show_arcball_angles(DvzApp* app, DvzId window_id, DvzMouseEvent* ev)
{
    ANN(app);

    // We only run the callback function when mouse drag stops (button down, move, button up).
    if (ev.type != DVZ_MOUSE_EVENT_DRAG_STOP)
        return;

    // The user data is passed as last argument in dvz_app_on_mouse().
    DvzArcball* arcball = (DvzArcball*)ev->user_data;
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
    DvzShape* shape = dvz_shape();
    dvz_shape_obj(shape, path);
    if (!shape->vertex_count)
    {
        dvz_shape_destroy(shape);
        return 0;
    }

    // Set the color of every vertex (the shape comes with an already allocated color array).
    for (uint32_t i = 0; i < shape->vertex_count; i++)
    {
        // Generate colors using the "bwr" colormap, in reverse (blue -> red).
        // dvz_colormap_scale(
        //     DVZ_CMAP_COOLWARM, shape->vertex_count - 1 - i, 0, shape->vertex_count,
        //     shape->color[i]);
        // shape->color[i][0] = shape->color[i][1] = shape->color[i][2] = 128;
        shape->color[i][3] = 32;
    }

    // Create a mesh visual with basic lightingsupport.
    DvzVisual* visual = dvz_mesh_shape(batch, shape, DVZ_MESH_FLAGS_LIGHTING);

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

