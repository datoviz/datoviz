# What Is Datoviz?

Datoviz is a visualization engine for scientific data. It is useful when you need to draw large or
interactive scenes directly: point clouds, images, meshes, volumes, annotations, linked panels, or
offscreen screenshots.

Datoviz fits into a larger Python visualization direction around GSP/VisPy2. The intended split is:
GSP/VisPy2 provides high-level scientific plotting, while Datoviz provides the lower-level rendering
engine that can draw and update large scenes. GSP/VisPy2 is still work in progress, so Datoviz v0.4
also exposes direct Python access through one generated `ctypes` binding. Use
`import datoviz as dvz` for the normal call form with documented NumPy array adaptation, and use
`datoviz.raw` only when you need the exact C-shaped pointers, counts, bytes, or callbacks.

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
GSP/VisPy2 when that layer is available. Until then, use Datoviz directly when you are comfortable
building scenes from visuals and data arrays.


## Where To Start

Begin with the [Quickstart](quickstart.md) if you want to run a first example. Use
[Choose your layer](choose-your-layer.md) if you are deciding between Python, C, browser examples,
or lower-level integration paths.
