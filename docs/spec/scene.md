# Scene API specification

## Definitions

### World

A **World** is a 2D or 3D coordinate system in which the data is defined.

### Camera

A 2D or perspective **Camera**, always associated to a **Panel**, is defined by:

* perspective parameters
* further nonlinear transforms?
* 2D/3D position
* orientation (up, right vectors)
* an initial aspect ratio
* an aspect ratio policy, defining how the field of views changes when the panel is resized
* an interactivity type, specifying how the camera changes in response to interact events

### Spinner?

A **Spinner** is a rotation matrix applied to the data, used for example by the arcball (implemented in the Model matrix).

### Visual

A **Visual** is any graphical element in a **World**.

### Box

A **Box** is a 2D or 3D bounding box around a **Visual**.

### Canvas

A **Canvas** is a physical window with a given size in framebuffer pixels and screen pixels, on which to draw one or several **Panels**.

### Panel

A **Panel** is a rectangular surface with a given position (within a canvas), size (in framebuffer/screen pixels), and associated to a **Camera**. The **Camera** renders to the **Panel**.

### Link

A **Link** associates some characterictics of two cameras together (absolute or relative position, orientation, etc).

### Scene

A **Scene** is a full specification of the world(s), cameras, canvases, panels, links, and camera-panel bindings in a given application.

### Focus

At any given time, a **Panel** may have the **Focus** (when clicking in it, for example)

### Interact event

The **Scene** may receive **Interact events**, containing information about which **Panel** has the focus, the mouse gestures, position relative to the focused panel, the key pressed, etc.

### Scene event

The **Scene** emits **Scene events** whenever something changes in the scene.

### Emitter?

An **Emitter** reacts to **Scene events** and emits **Rendering requests**, to be sent to the **Renderer**.

### Visual request

The **Scene** may raise **Visual requests**, specifying that at a given time, in a given **World**, a given **Visual** is asking for data within a given **Bounding box** and with given **Camera** properties.

This is how the user specifies the data of every **Visual**.

### Renderer

A **Renderer** is a component receiving **Rendering requests**, and capable of generating on-demand bitmaps of the **Canvases** and **Panels**.

### Rendering request

A **Rendering request** is a request to be sent to the renderer, specifying what to render, how, when, and where.


## API

TODO
