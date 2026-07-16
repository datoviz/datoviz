# Datoviz, GSP, and VisPy2

Datoviz v0.4 is the rendering engine. GSP/VisPy2 owns the high-level scientific plotting direction.
This division prevents the engine documentation from promising automatic plotting behavior that a
different layer must define.

**Audience:** users choosing a Python abstraction and contributors deciding which repository owns a
feature. **Prerequisite:** none. You will learn when direct Datoviz is appropriate and which plotting
expectations belong to the external, work-in-progress GSP/VisPy2 layer.


## Choose by abstraction level

| Need | Layer |
| --- | --- |
| Explicit scenes, visual families, typed arrays, controllers, capture, or native embedding | Datoviz |
| C/C++ engine integration or lower-level Python access to the same engine | Datoviz |
| Backend validation, WebGPU portability experiments, or runtime contribution | Datoviz Advanced |
| Plot objects, automatic scales/layout, notebook-first ergonomics, or a rich Python object model | GSP/VisPy2, external work in progress |

Datoviz's normal Python entry point is `import datoviz as dvz`: generated `dvz_*` calls with
documented NumPy adaptation. `datoviz.raw` is the exact pointer/count form of that same binding, not
a high-level plotting API. Datoviz v0.4 does not restore the old Python plotting surface or invent
engine methods such as `scatter()` and `imshow()`.


## Boundary for examples and documentation

A Datoviz example should show the complete engine workflow: scene, figure, panel, visual, explicit
attribute arrays, panel attachment, and window or capture target. A future GSP/VisPy2 example may
turn a higher-level plot specification into that engine state, but its automatic scale, chart, and
object semantics remain owned by the higher layer.

Use the [Project status](../reference/project-status.md) for the current `external/GSP` label and
[Choose your layer](../start/choose-your-layer.md) for user routing. Do not infer availability from
the intended architecture.
