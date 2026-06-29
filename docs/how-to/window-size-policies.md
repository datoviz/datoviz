# Window size policies

Datoviz separates four size spaces:

| Space | Meaning |
| --- | --- |
| Canvas/reference px | Units used by scene layout and screen-space visual sizes such as marker diameters, text sizes, and stroke widths. |
| Host logical px | Units accepted by the OS/window backend. |
| Framebuffer px | Device pixels used by the GPU render target and image captures. |
| Physical units | Millimeters or inches on a live display; always approximate for windows. |

Use `DvzViewSizeDesc` when you need explicit control over those spaces.

## Policies

`dvz_view_size_desc_framebuffer_px(width, height)` requests exact framebuffer pixels. Use this for deterministic screenshots, offscreen renders, and tests.

`dvz_view_size_desc_host_logical_px(width, height)` requests OS/window logical pixels directly. Use this for low-level host integration when another toolkit already owns logical sizing.

`dvz_view_size_desc_reference_px(width, height, reference_dpi)` requests a CSS-like canvas size. A `1280x720` canvas at `96` reference DPI targets the physical size of `1280/96` by `720/96` inches when live-window metrics are available.

`dvz_view_size_desc_physical_mm(width_mm, height_mm, reference_dpi)` requests a physical target directly. Datoviz derives the canvas/reference-pixel extent from the physical size and `reference_dpi`.

## Example

```c
DvzViewDesc desc = dvz_view_desc(DVZ_VIEW_GLFW);
desc.size = dvz_view_size_desc_reference_px(1280.0, 720.0, 96.0);
desc.title = "reference-sized view";

DvzView* view = dvz_view(app, figure, &desc);
DvzResolvedViewSize resolved = dvz_view_resolved_size(view);
```

Inspect `resolved.framebuffer_per_canvas_px_x` and `resolved.framebuffer_per_canvas_px_y` when debugging screen-space visual sizes.

For a runnable CLI example:

```bash
./build/examples/c/features/view_size_policies --policy reference --frames 1
./build/examples/c/features/view_size_policies --policy pixel --frames 1
./build/examples/c/features/view_size_policies --policy physical --frames 1
```

`user_scale` is separate. It scales screen-space styling inside an already resolved canvas; it does not change the requested window, framebuffer, or physical size. See `examples/c/features/user_scale.c`.
