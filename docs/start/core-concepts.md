# Core concepts

Datoviz uses a small set of objects to turn scientific arrays into an interactive view or an image. The same object model appears in Python, C, examples, and the API reference.

## The object model

<div class="dvz-object-diagram" role="group" aria-label="A scene contains a figure; the figure contains one or more panels; a panel contains visuals and a controller; each visual receives named data arrays">
  <div class="dvz-object-diagram__scene">
    <span class="dvz-object-diagram__label">Scene</span>
    <div class="dvz-object-diagram__figure">
      <span class="dvz-object-diagram__label">Figure · complete output</span>
      <div class="dvz-object-diagram__panels">
        <div class="dvz-object-diagram__panel">
          <span class="dvz-object-diagram__label">Panel · drawing region</span>
          <span class="dvz-object-diagram__controller">Controller · panzoom or 3D navigation</span>
          <div class="dvz-object-diagram__visual">
            <span class="dvz-object-diagram__label">Visual · one renderable collection</span>
            <div class="dvz-object-diagram__data">Named data arrays · position · color · size · pixels</div>
          </div>
        </div>
        <div class="dvz-object-diagram__panel dvz-object-diagram__panel--secondary">
          <span class="dvz-object-diagram__label">Another panel</span>
          <span>Optional second view</span>
        </div>
      </div>
    </div>
  </div>
</div>

| Object | Think of it as | Typical choice |
| --- | --- | --- |
| **Scene** | The objects and data that describe a visualization. The scene remains available after a frame has been drawn. | Usually one scene for a program or independent visualization. |
| **Figure** | The complete output image, with a width and height in pixels. | One window or captured image usually presents one figure. |
| **Panel** | A drawing region inside a figure. | Use one full panel first; add a grid only for multiple views. |
| **Visual** | A collection rendered in one way: points, paths, an image, a mesh, text, or another family. | Group related items of the same family into one visual. |
| **Data arrays** | Values assigned to exact visual attribute names. | Arrays commonly describe positions, colors, sizes, pixels, or indices. |
| **Controller** | Navigation state bound to a panel. | Panzoom for 2D; arcball, fly, or turntable for 3D. |
| **View or capture** | Where the figure is rendered. | Choose a native window, offscreen capture, or supported embedded host. |

A visual normally represents many related items. For example, use one point visual with 10,000 positions, colors, and diameters—not 10,000 one-point visuals. Separate visuals when the data needs a different visual family, panel, style, coordinate treatment, or update schedule.

!!! important "Batch items into visuals"
    Datoviz is designed to draw many items through a small number of visuals. Put related points in one point visual, related line segments in one line visual, and so on. Creating one visual per item defeats this batching model and can make a scene much slower.


## The complete workflow

Most programs follow the same sequence:

1. Create a scene, figure, and at least one panel.
2. Create a visual for the data representation you need.
3. Assign arrays to the visual's named attributes.
4. Add the visual to a panel.
5. Add a controller or adornments such as axes when needed.
6. Open a window or create an offscreen output.
7. Run the application or capture the frame.

Uploading data does not place a visual in the figure. The visual becomes part of the view only after you attach it to a panel.


## Data has a contract

Every visual family defines which attributes it accepts and what each array means. Before uploading
an array, check four properties:

- the exact attribute name, such as `"position"` or `"diameter_px"`;
- the data type, such as `float32` positions or `uint8` RGBA colors;
- the shape and item count;
- the coordinate space or unit, such as data coordinates or screen pixels.

The [visual-family reference](../reference/visual-families/index.md) describes these contracts. The
[visual attributes reference](../reference/visual-attributes.md) explains full writes, range
updates, copied data, and external-buffer variants.


## Retained state and updates

The scene and its objects remain available after the first frame. This behavior is called a **retained scene model**. Update the arrays, camera, controller, visibility, or style that changed; you do not normally rebuild the whole scene.

Ordinary `dvz_visual_set_data()` writes copy the supplied array before returning. Advanced borrowed
or external-buffer APIs have separate lifetime and synchronization rules. See
[Objects and lifetimes](../reference/objects-and-lifetimes.md) before using those paths.


## The two language paths

Python and C build the same retained objects and use the same visual attribute names. They differ
mainly in array adaptation and session management:

| Concern | Python | C |
| --- | --- | --- |
| Ordinary import/header | `import datoviz as dvz` | public headers such as `datoviz/scene.h` and `datoviz/app.h` |
| Arrays | documented calls accept compatible NumPy arrays | pass typed C arrays plus item counts |
| Window session | `dvz.run(scene, figure)` manages the common blocking session | create `DvzApp` and `DvzView`, run the app, then destroy objects |
| Exact pointer/count access | use `datoviz.raw` only when required | already explicit in the C API |

Start from a complete program before copying smaller API fragments. The
[Quickstart](quickstart.md) contains complete Python and C programs; How-To pages label excerpts
that assume existing objects or data.


## Coordinates and interaction

Positions may be expressed in data, panel, world, or screen-related spaces depending on the visual
and attribute. A panel's camera or controller transforms the data view; screen-sized attributes
such as point diameter remain measured in pixels.

Start with [panzoom](../how-to/use-panzoom.md) for 2D data or a
[3D controller](../how-to/3d-navigation.md) for spatial scenes. Use the
[coordinate-system guide](../how-to/coordinate-systems.md) before mixing overlays, labels, images,
and 3D geometry.
