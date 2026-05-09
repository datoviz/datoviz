# High-DPI And Device Pixel Ratio

This document defines how the scene layer handles high-DPI displays and device pixel ratio.


## Coordinate Model

The scene layer operates exclusively in **logical pixels**.

Logical pixels follow the CSS pixel model:
one logical pixel is one CSS pixel, independent of display density.

The scene never sees or computes in physical pixels.
Physical resolution is the runtime's concern.


## Device Pixel Ratio

The device pixel ratio (`dpi_scale`) is the ratio of physical pixels to logical pixels
on the current display.

Typical values:

| Display type | `dpi_scale` |
|---|---|
| Standard (96 dpi) | `1.0` |
| Retina / HiDPI (192 dpi) | `2.0` |
| Fractional DPI | `1.25`, `1.5`, `1.75`, … |

The scene reads `dpi_scale` from the window surface at startup and whenever the window
moves to a display with a different pixel density.

The scene exposes the current value as a read-only property:

```text
float dpi_scale = dvz_scene_dpi_scale(scene)
```


## What The Runtime Handles

The runtime (not the scene) is responsible for:

1. allocating render targets at `dpi_scale × logical_size` physical pixels,
2. uploading font atlases and rasterized glyphs at the correct physical resolution,
3. ensuring that swapchain images and offscreen targets match the physical extent.

The scene submits a `FramePlan` in logical coordinates.
The runtime applies `dpi_scale` when translating logical extents to physical extents.


## What The Scene Handles

The scene is responsible for:

1. **pixel-unit quantities** — sizes declared in logical pixels (marker radius, line width,
   tick label font size) are internally scaled by `dpi_scale` before being passed to upload
   nodes, so that they appear the same physical size on all displays,
2. **font rendering** — `DvzFont` rasterizes glyphs at `dpi_scale × requested_pt_size`
   physical pixels to preserve crispness; the logical size exposed to the user is
   `requested_pt_size`,
3. **picking** — pick coordinates arrive from the runtime in logical pixels; the scene
   does not need to un-scale them,
4. **input events** — pointer coordinates from the runtime are already in logical pixels.


## DPI Change At Runtime

When a window is moved to a display with a different pixel density, the runtime fires a
DPI-change event.

The scene responds by:

1. updating the cached `dpi_scale`,
2. marking all pixel-unit quantities dirty,
3. scheduling a font atlas rebuild at the new physical resolution if the scale changed,
4. triggering a redraw.

The application is notified via a scene-level diagnostic or callback if it needs to respond
(e.g., to update manually-specified pixel sizes).


## Interaction With Render Scale

Render scale (`dvz_figure_set_render_scale`, see `export/IMAGE_EXPORT.md`) and `dpi_scale` are
independent and stack multiplicatively.

The runtime allocates render targets at:

```
physical_size = dpi_scale × render_scale × logical_size
```

The scene remains unaware of both scale factors.
It emits `FramePlan` nodes in logical coordinates throughout.


## Rule Summary

1. scene coordinates are always logical pixels,
2. `dpi_scale` is read from the window surface and owned by the runtime,
3. the scene applies `dpi_scale` only to pixel-unit quantities before upload,
4. text rasterization uses `dpi_scale` to stay crisp,
5. input coordinates arrive in logical pixels — no un-scaling needed in scene code,
6. render scale and `dpi_scale` stack; the runtime applies both.


## Relationship To Other Documents

| Document | Relationship |
|---|---|
| `export/IMAGE_EXPORT.md` | render scale stacks with `dpi_scale` |
| `semantics/GEOMETRY_UTILITIES.md` | `DvzFont` rasterizes at physical resolution |
| `pipeline/FRAME_LIFECYCLE.md` | DPI-change event triggers redraw and dirty propagation |
| `pipeline/INVALIDATION_AND_CACHING.md` | pixel-unit dirty scope on DPI change |
