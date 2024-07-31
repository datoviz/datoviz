# User guide

This user guide targets the Python bindings which closely follow the Datoviz C API. The ctypes bindings are auto-generated from the C function signatures.

To use a Datoviz C function in Python, you typically just need to replace the `dvz_` (functions), `DVZ_` (enumerations), or `Dvz` (structures) prefix with `dvz.` after importing Datoviz with `import datoviz as dvz`.

This user guide is still a work in progress. You're strongly advised to look at the [examples](examples.md) (in the `examples/` subfolder in the repository) and the auto-generated [API reference](api.md) (in `docs/api.md`).

Creating a GPU-based interactive visualization script with Datoviz in Python typically involves the following steps:

1. Creating an `app` and a `scene`.
2. Creating one or several `figures` (window).
3. Creating one or several `panels` (subplots) in each figure, defined by their offset and size in pixels.
4. Creating `visuals` of predefined types.
5. Setting the visual data (position, size, color, groups...).
6. Optionally, setting up event callbacks (mouse, keyboard, timers...).
7. Optionally, creating GUIs.
8. Running the application.
9. Close and destroy the `scene` and `app`.

GPU knowledge is not required when using this interface.
The lower-level GPU-based layers are not yet exposed in the `datoviz.h` public header file.
Contact us if you would be interested in using them in your application.


## App and scene

The `app` handles the window, user events, event loop.

The `scene` handles the panels, visuals, and data.

A visualization script is typically organized as follows:

```python
# This imports the binary libdatoviz shared library.
import datoviz as dvz

# Create an application. The argument is reserved to optional flags, like dvz.APP_FLAGS_OFFSCREEN
# to run an offscreen application (without window, saving a figure to a PNG file).
# See the offscreen.py example.
app = dvz.app(0)

# Retrieve the app's batch, that contains a stream of Datoviz Intermediate Protocol that will be
# processed at the next frame by the app's event loop. It's used to create visuals.
batch = dvz.app_batch(app)

# Create a scene, which handles the plotting objects (figures, panels, visuals, data, callbacks).
scene = dvz.scene(batch)

# ... your code here ...

# Run the application. The last argument is the number of frames (0 = infinite loop).
dvz.scene_run(scene, app, 0)

# App and scene clean up, memory freeing, etc.
dvz.scene_destroy(scene)
dvz.app_destroy(app)
```

Internally, the `scene` API generates a stream of Datoviz Intermediate Protocol (DIP) requests and sends them to the Datoviz Vulkan renderer (handled by the `app`).
The DIP closely resembles the WebGPU specification.
This decoupled architecture ensures that, in the future, the `scene` API can be implemented on top of other non-Vulkan DIP renderers (including a future Javascript-based one).

While the architecture has been designed with multithreading in mind (allowing data compute and transfers without blocking the event loop), we have mostly focused on single-threaded applications so far.
Multithreading functionality will be provided and documented later.


## Figures

A `Figure` is a window on which to draw visuals.
It is created as follows:

```python
# Create a figure with size 800 x 600 and no optional flags.
figure = dvz.figure(scene, 800, 600, 0)
```


## Panels

A `Panel` is a rectangular portion of a `Figure` on which to render visuals.

You can create a default panel spanning the entire figure as follows:

```python
panel = dvz.panel_default(figure)
```

Create an arbitrary panel as follows:

```python
# x, y is the offset of the top-left panel corner.
# w, h is the size in pixels of the panel.
panel = dvz.panel(figure, x, y, w, h)
```


## Visuals

The `Visual` is the most important object type in Datoviz.
It represents a visual collection of similar elements, like a set of points, markers, segments, glyphs (text), paths, images, meshes...
The notion of collection is crucial for high-performance rendering with GPUs.
Visual elements of the same type should be grouped together in the same `Visual` to ensure good performance.

The main limitation with grouping elements together is that they currently share the same transform (i.e. they share the same coordinate system).

Datoviz provides a predefined set of common visuals:

* **Raw basic visuals** (fast but low quality): pixels, squares, aliased thin lines (line strip, line list), triangles (triangle list, triangle strip);
 * **0D visuals**: `pixel`, `point` (disc), `marker`, `glyph` (string characters rendered on the GPU with multichannel signed distance fields);
 * **1D visuals**: `segment`, `path`;
 * **2D visuals**: `image`;
 * **3D visuals**: `mesh`, `sphere` (2D sprites with "fake" 3D rendering, aka impostors), `volume` (basic GPU raymarching algorithm at the moment), `slice` (volume image slices).

The visuals are implemented on the GPU using advanced antialiasing techniques in the shaders.

More visuals will be implemented later, as will as the possibility of creating custom visuals via user-provided shaders.

To create a visual, use this:

```python
# Create a `point` visual with no optional flags.
visual = dvz.point(batch, 0)
```

## Visual data

Once a visual is created, specify its data using the provided visual-specific functions.

The most common types of visual properties are the point positions and colors, but each visual comes with specific data properties (size, shape, groups, etc.).
Refer to the C API reference for more details.

### Terminology

We use the following terminology:

* **item**: a single visual element, like a particular point or marker, or a single image in an `image` visual (remember that each visual is a collection of elements, so the `image` visual represents a set of one or multiple images)
* **group**: a consecutive group of items that share common properties. This is mostly used in the `path` visual for now, where a group refers to a single path, while an item refers to a point within a path. A `path` visual therefore contains a set of points (items) each organized into a single or multiple disjoint paths (groups).
* **vertex**: a 3D point sent to the GPU (this is transparent to the user). For example, a single image is defined by two triangles and six vertices. Datoviz handles the triangulation automatically and transparently, so you typically don't need to be aware of vertices.

A particular visual represents a collection of `n` items indexed from `0` to `n-1`.


### Python ctypes bindings

C visual data functions expect a pointer to arrays of a given type, for example typically an array of `vec3` (three float32 numbers) for positions, or an array of `cvec4` (four `char`, ie four rgba uint8 bytes).

Python ctypes bindings are auto-generated and expect a NumPy array whenever a C visual data function expects a pointer to an array of values.
At the moment, the ctypes bindings check the dtype, the shape, and the C-contiguity of the provided arrays.


### Position

The `position` property specifies the 3D positions of the visual points.
Some visuals require the point position in a particular format.
For example, the segment positions are specified by the 3D coordinates of the initial and terminal sections of each segment.
The image positions are currently defined by 2D coordinates of the upper left and lower right corners (this might change in the future depending on user feedback).

To set the positions of a visual, for example the `point` visual, use this:

```python
# Define a (N, 3) NumPy array of float32 values (one row = one point).
# Note that the C function dvz_point_position() expects a vec3 array.
pos = np.random.normal(size=(n, 3), scale=.25).astype(np.float32)

# Set the positions of `n` items starting with item #0.
# The last argument represents the optional data transfer flags (typically 0).
dvz.point_position(visual, 0, n, pos, 0)
```

The coordinate system is as follows:

* **x**: left to right (-1..1)
* **y**: bottom to top (-1..1)
* **z**: front to back (0..1)

The positions must be provided in a normalized coordinate system (normalized device coordinates, or NDC, in computer graphics terminology).
Your data is typically not in this range so you'll need to normalize it manually to the [-1..1] interval before passing it to Datoviz.

Datoviz v0.2 does not yet provide builtin axes nor data normalization features, but that will come in v0.3.

### Color

Colors are passed as rgba 4*uint8 values.
Use opacity values < 255 in the last component (`a` for alpha) to define transparent elements.


### Textures

Textures are used by the `image` (2D textures), `mesh` (2D textures) and `volumes` (3D textures) visuals.
See the examples for more details.

For example, here is how to create a 2D texture and pass it to an `image` visual:

```python
# Assuming rgba is a 3D NumPy array (height, width, 4).
height, width = rgba.shape[:2]

# Texture parameters.
format = dvz.FORMAT_R8G8B8A8_UNORM  # The Vulkan format corresponds to 4xuint8 values.
address_mode = dvz.SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER  # Texture address modeL.
filter = dvz.FILTER_LINEAR  # Linear filtering, use dvz.FILTER_NEAREST to disable.

# Create a texture out of a RGB image.
# NOTE: since dvz_tex_image() accepts any type of pointer, we need to manually convert the NumPy
# array to a void* pointer. This is done with the `A_()` function (`from datoviz import A_`).
tex = dvz.tex_image(batch, format, width, height, A_(image))

# Finally, we assign this texture to the image visual.
dvz.image_texture(visual, tex, filter, address_mode)
```


### Data sharing

Since textures are decoupled from visuals, they can readily be shared across visuals.

However, it is not yet possible to easily share other types of data between visuals.
While the underlying architecture has been designed to make this use-case possible, the user-exposed API does not yet support it.


### Shapes

The `mesh` visual can be used directly with properties such as vertices, indices, colors, normals and texture coordinates, but one can also use the `Shape` structure that encapsulates these arrays.
They can be created with functions for predefined shapes, along with affine transforms, merging, etc.


## Interactivity

Two types of interactivity patterns are currently supported:

* **Panzoom** (2D): pan with left mouse drag, zoom with the right mouse dag.
* **Arcball** (3D): rotate with left mouse drag.

More interactivity patterns will be implemented in the future.

Use this to define the interactivity pattern in a panel:

```python
pz = dvz.panel_panzoom(panel)
# or
arcball = dvz.panel_arcball(panel)
```


## Event callbacks

You can define custom event callbacks to react to the mouse and the keyboard.
You can also set up timers.

### Mouse

Define a mouse callback as follows:

```python
@dvz.mouse
def on_mouse(app, window_id, ev):
    # ev is the mouse event structure.
    # Mouse position.
    x, y = ev.pos
    print(f"Position {x:.0f},{y:.0f}")
    # Mouse event type.
    if ev.type == dvz.MOUSE_EVENT_CLICK:
        # Mouse click button.
        button = ev.content.b.button
        print(f"Clicked with button {button}")
```

The mouse event types are the following:

```
MOUSE_EVENT_RELEASE             b       DvzMouseButtonEvent
MOUSE_EVENT_PRESS               b       DvzMouseButtonEvent
MOUSE_EVENT_MOVE
MOUSE_EVENT_CLICK               c       DvzMouseClickEvent
MOUSE_EVENT_DOUBLE_CLICK        c       DvzMouseClickEvent
MOUSE_EVENT_DRAG_START          d       DvzMouseDragEvent
MOUSE_EVENT_DRAG                d       DvzMouseDragEvent
MOUSE_EVENT_DRAG_STOP           d       DvzMouseDragEvent
MOUSE_EVENT_WHEEL               w       DvzMouseWheelEvent
```

The letters are to be used after `ev.content.`, for example `ev.content.b` which is a `DvzMouseButtonEvent` structure.
See the C API reference for more details about the fields available in these structures.

The mouse buttons are:

```
DVZ_MOUSE_BUTTON_LEFT = 1
DVZ_MOUSE_BUTTON_MIDDLE = 2
DVZ_MOUSE_BUTTON_RIGHT = 3
```

Datoviz does not yet provide built-in picking functionality.
The only information provided by Datoviz in mouse event callbacks is the coordinates in pixels of the mouse cursor.


### Keyboard

Define a keyboard callback as follows:

```python
# Keyboard event callback function.

@dvz.keyboard
def on_keyboard(app, window_id, ev):

    # Key code (see the C API reference).
    key = ev.key

    # Modifier flags.
    mods = {
        'shift': ev.mods & dvz.KEY_MODIFIER_SHIFT != 0,
        'control': ev.mods & dvz.KEY_MODIFIER_CONTROL != 0,
        'alt': ev.mods & dvz.KEY_MODIFIER_ALT != 0,
        'sup': ev.mods & dvz.KEY_MODIFIER_SUPER != 0,
    }
    mods = '+'.join(key for key, val in mods.items() if val)

    # Keyboard events are PRESS, RELEASE, and REPEAT.
    type = {
        dvz.KEYBOARD_EVENT_PRESS: 'press',
        dvz.KEYBOARD_EVENT_REPEAT: 'repeat',
        dvz.KEYBOARD_EVENT_RELEASE: 'release',
    }
    type = type.get(ev.type, '')

    print(f"{type} {mods} {key}")

# We register the keyboard callback function.
dvz.app_onkeyboard(app, on_keyboard, None)
```


### Timer




### Manual 3D camera control

```python
from datoviz import vec3
camera = dvz.panel_camera(panel)
dvz.camera_position(camera, vec3(x, y, z))
dvz.camera_lookat(camera, vec3(x, y, z))
```


## Graphical user interfaces





