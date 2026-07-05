# What Is Datoviz?

Datoviz is a visualization engine for scientific data. It is useful when you need to draw large or
interactive scenes directly: point clouds, images, meshes, volumes, annotations, linked panels, or
offscreen screenshots.

The main idea is simple: build a scene from reusable visual objects, attach your data arrays to
those visuals, then show the scene in a window or render it to an image.


## What You Build

Most Datoviz examples use the same pieces:

| Piece | What it means |
| --- | --- |
| Scene | The whole visualization. |
| Figure | The image area, with a pixel size such as 800 by 600. |
| Panel | A drawing area inside the figure. One figure can contain one panel or many panels. |
| Visual | A renderable collection such as points, line segments, an image, a mesh, or text. |
| Data arrays | NumPy arrays or C arrays attached to visual attributes such as position, color, or size. |
| View | A window or offscreen target where the figure is rendered. |

This structure is lower-level than a plotting function, but it gives you explicit control over what
is drawn and how the scene is updated.


## When Datoviz Fits

Use Datoviz directly when you want to:

- display many points, images, meshes, volumes, or labels interactively;
- update visual data over time without rebuilding the whole scene;
- render native desktop windows or offscreen screenshots;
- embed Datoviz in a C/C++ application;
- write Python examples that pass NumPy arrays to the current v0.4 API.

For high-level scientific plotting functions such as `scatter()`, `plot()`, or `imshow()`, use
VisPy2/GSP when that layer is available. Datoviz v0.4 is the lower-level rendering engine that such
interfaces can build on.


## Where To Start

Begin with the [Quickstart](quickstart.md) if you want to run a first example. Use
[Choose your layer](choose-your-layer.md) if you are deciding between Python, C, browser examples,
or lower-level integration paths.
