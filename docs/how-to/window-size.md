# Choose Window Size

Choose the size rule that matches what you need: an exact screenshot, a normal desktop window, a
reference-sized UI, or an approximate physical size on a display.

Datoviz separates the size you request from the size actually used by the render target. This
matters on high-DPI displays, in offscreen screenshots, and when screen-space visual styling such as
text size, marker diameter, or line width must stay predictable.


## Choose A Size Policy

| Need | Use | Why |
| --- | --- | --- |
| Exact screenshot or test image size | `dvz_view_size_desc_framebuffer_px(width, height)` | Requests device pixels directly. |
| Ordinary desktop window size | `dvz_view_size_desc_host_logical_px(width, height)` | Uses the logical units expected by the operating system or host toolkit. |
| Stable canvas styling across displays | `dvz_view_size_desc_reference_px(width, height, reference_dpi)` | Keeps the scene's reference pixel size explicit. |
| Approximate physical display size | `dvz_view_size_desc_physical_mm(width_mm, height_mm, reference_dpi)` | Converts millimeters to reference pixels when display metrics are available. |

For documentation images and automated comparisons, prefer framebuffer pixels. For ordinary desktop
applications, logical pixels or reference pixels are usually easier to reason about.


## Minimal Example

```c
DvzViewDesc desc = dvz_view_desc(DVZ_VIEW_WINDOW);
DvzViewSizeDesc size = dvz_view_size_desc_reference_px(1280.0, 720.0, 96.0);

desc.size_policy = size.policy;
desc.size_width = size.width;
desc.size_height = size.height;
desc.size_reference_dpi = size.reference_dpi;
desc.title = "reference-sized view";

DvzView* view = dvz_view(app, figure, &desc);
DvzResolvedViewSize resolved = dvz_view_resolved_size(view);
```

Inspect `resolved.framebuffer_per_canvas_px_x` and
`resolved.framebuffer_per_canvas_px_y` when a screen-space mark, label, or line width looks too
large or too small.


## Size Spaces

| Space | Meaning |
| --- | --- |
| Canvas/reference pixels | Units used by scene layout and screen-space visual sizes such as marker diameters, text sizes, and stroke widths. |
| Host logical pixels | Units accepted by the operating system or host window toolkit. |
| Framebuffer pixels | Device pixels used by the GPU render target and image captures. |
| Physical units | Millimeters or inches on a live display; always approximate for windows. |

High-DPI systems often use more than one framebuffer pixel per logical or reference pixel. That is
why a window can be 800 by 600 logical pixels while its framebuffer is larger.


## Try The Example

From a source checkout:

```sh
./build/examples/c/features/view_size_policies --policy reference --frames 1
./build/examples/c/features/view_size_policies --policy pixel --frames 1
./build/examples/c/features/view_size_policies --policy physical --frames 1
```


## Important Details

`user_scale` is separate from view size. It scales screen-space styling inside an already resolved
canvas; it does not change the requested window, framebuffer, or physical size. See
`examples/c/features/user_scale.c`.

Use offscreen rendering when the output must have a precise framebuffer size. Native windows may be
resized by the window manager or scaled by the display.


## See Also

- [Render offscreen](render-offscreen.md)
- [Save screenshots](screenshots.md)
- [Create a window](create-a-window.md)
