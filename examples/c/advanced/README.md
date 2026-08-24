# C Advanced Examples

Low-level runtime, DRP2, and host-integration examples live here. These examples are useful when
embedding Datoviz, debugging the runtime path, or bypassing the scene layer deliberately.

- `raw_triangle_vklite.c`: raw vklite draw commands recorded into a Datoviz canvas.
- `raw_triangle_drp2.c`: hand-written DRP2 command stream executed by the native vklite runtime.
- `external_surface_glfw.c`: host-owned GLFW window and Vulkan surface rendered by a Datoviz view.
- `gui_implot.cpp`: opt-in ImPlot v1.0 charts beside an embedded Datoviz 3D GUI viewport; configure with `-DDVZ_BUILD_IMPLOT_EXAMPLE=ON`.

Common scene workflows such as offscreen capture and video export stay in `features/`.
