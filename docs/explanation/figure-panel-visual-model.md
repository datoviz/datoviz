# Scene Building Blocks

Most Datoviz programs use the same small set of building blocks. You do not need to know graphics
terms before using them.


## Scene

A scene is the whole visualization. It owns the figures, panels, visuals, controllers, and data that
belong together.

Create one scene for one visualization workflow. Destroy it after any window, offscreen render, or
capture using it has finished.


## Figure

A figure is the output image area. It has a pixel size, such as 800 by 600.

A figure can be shown in a window, rendered offscreen, or embedded in a supported host application.


## Panels

A panel is a drawing region inside a figure. A simple figure has one full-size panel. A multi-panel
figure can place several panels next to each other.

Panels are where visuals are attached. They also hold view settings such as 2D pan and zoom or a 3D
camera.


## Visuals

A visual is one kind of thing to draw. Examples are points, markers, line segments, paths, images,
meshes, text, labels, and spheres.

A visual usually contains many items. For example, a point cloud should normally be one point visual
with many point positions, not one visual per point.

Use separate visuals when the data needs a different visual type, a different panel, a different
style, or a different update pattern.


## Data Arrays

Data arrays provide the values that visuals draw. Common arrays include positions, colors, sizes,
image pixels, mesh vertices, and text strings.

After you attach arrays to a visual, attach the visual to a panel. Uploading data alone does not make
the visual appear.


## Views

A view is where a figure is rendered. It can be an interactive window, an offscreen target for image
capture, or a supported embedded host.


## Adornments

Adornments are visual context around your data: axes, ticks, labels, colorbars, scale bars, legends,
and readouts. They are part of the scene, not a separate drawing system.


## Typical Order

Most examples follow this order:

1. create a scene;
2. create a figure;
3. create one or more panels;
4. create visuals;
5. set visual data arrays;
6. attach visuals to panels;
7. open a window or capture an image.

See also:

- [Coordinate systems](coordinate-systems.md)
- [Create a scene](../how-to/create-a-scene.md)
- [Profile rendering performance](../how-to/profile-performance.md)
