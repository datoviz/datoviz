# How to use the canvas C API

This example shows how to **write a standalone C app using only the canvas API**, not the scene API. We'll render a triangle without using existing graphics, visuals, panels, and so on. We'll follow these steps:

* Creating a graphics with custom shaders,
* Creating a function callback for command buffer refill,
* Creating a vertex buffer manually.

<!-- IMAGE ../images/screenshots/standalone_canvas.png -->

<!-- CODE_C examples/standalone_canvas.c -->
