# Coordinate Systems

Datoviz keeps scientific coordinates and backend coordinates separate. User-facing scene state is
expressed in semantic spaces; frame planning lowers that state to GPU-facing transforms and
attributes.

## Spaces

| Space | Meaning | Typical owner |
| --- | --- | --- |
| Data | User scientific or application coordinates. | User data, panel domains, scales, query readouts. |
| Panel/domain | Visible data interval or fitted domain for one panel. | Panel view resolver and controllers. |
| Visual-local | Coordinates local to one visual or resource, such as image quad placement or mesh vertices. | Visual family contract. |
| World | 3D scene coordinates before camera/view projection. | 3D visual state and camera setup. |
| View | Camera-relative coordinates after view transform. | Camera/controller evaluation. |
| Clip/NDC | GPU-normalized space after projection and perspective divide. | Frame planning and shaders. |
| Framebuffer | Pixel coordinates in the render target. | Runtime input, picking/query readback, screenshots. |
| Texture/sample | Normalized UV/UVW or integer texel/voxel/sample indices. | Image, labels, volume, glyph atlas, sampled fields. |


## Pixel And Display Spaces

Datoviz distinguishes logical display pixels from physical framebuffer pixels. Visual style sizes
use logical pixels unless a visual-family page explicitly says otherwise. Render targets, readback,
and screenshots use framebuffer pixels.

| Quantity | Space | Affected by device scale | Affected by controllers | Typical API |
| --- | --- | --- | --- | --- |
| Data positions | Data/panel/world, by visual contract | No | Yes, when attached with controller application | `position`, `position_start`, `position_end` |
| Screen-space sizes | Logical pixels | Converted by the view/runtime to framebuffer pixels | No, size stays screen-stable while centers move | `diameter_px`, `pixel_size_px`, `stroke_width_px`, text size |
| Panel rectangles and reserves | Logical figure pixels | Converted during viewport/scissor resolution | No | `dvz_panel_set_desc()`, reserves, padding |
| Offscreen view extent | Framebuffer pixels | Direct exact output for `dvz_view_offscreen()` | No | `dvz_view_offscreen(app, figure, width, height)` |
| Screenshot/RGBA capture | Framebuffer pixels | Already applied | No | `dvz_view_capture_png()`, Python `dvz_view_capture_rgba()` |
| Pointer input | Framebuffer/window pixels at the backend boundary | Backend reports scale separately where available | Routed through panel/controller transforms | input and query APIs |

For a Matplotlib/GSP-style backend, convert display quantities to Datoviz logical-pixel style
attributes before upload. Use framebuffer dimensions only when creating/capturing render targets or
when interpreting raw input/readback coordinates.

## Precision Boundary

Semantic and domain coordinates should remain authoritative in double precision where precision
matters. Visual render attributes lower to GPU-facing float data unless the visual family contract
says otherwise.

This means a plot, mesh generator, or application may retain high-precision data coordinates while
the emitted frame uploads compact GPU-facing attributes.

## Controllers

Controllers mutate transforms and visible domains, not source data:

| Controller | Coordinate effect |
| --- | --- |
| Panzoom | Changes the visible 2D panel/domain extent. |
| Arcball/orbit/turntable/fly | Changes 3D camera or view state. |
| Linked controllers | Share explicit controller/domain state between panels. |

If no controller is bound, a panel uses its configured or fitted domain.

## Input And Queries

Pointer input starts in framebuffer coordinates. The scene routes it through the target viewport and
panel transform, then query execution resolves a rendered contribution, sampled field value, or
miss.

Do not compute rendered visual hits from CPU geometry as a fallback. Query semantics must follow the
same transform, viewport, depth, clipping, and shader behavior used by rendering.

## Nonlinear And Geographic Data

Scene-managed nonlinear/geographic projections are deferred in v0.4. The supported pattern is:

1. project nonlinear/geographic data on the CPU;
2. upload ordinary Cartesian positions or sampled fields;
3. keep units/provenance in application metadata, labels, legends, or annotations.

## See Also

- [Use coordinate systems](../how-to/coordinate-systems.md)
- [Controllers](controllers.md)
- [Queries](queries.md)
- [Project status](project-status.md)
