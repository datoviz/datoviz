# Point

Screen-space circular point sprites for dense 2D or 3D point clouds.

Status: supported.
Backends: native; WebGPU live (`point`, `panzoom`).
Primitive: instanced screen-space quads.

## Preview And Links

[![Point](../../assets/gallery/v0.4/visuals/point_2d.webp)](../../examples/gallery/visuals/point_2d.md)

- Example: [Point](../../examples/gallery/visuals/point_2d.md)
- How-to: [Choose a visual family](../../how-to/choose-a-visual-family.md), [add visuals to a panel](../../how-to/add-a-visual.md)
- Related: [Pixel](pixel.md), [Marker](marker.md), [Sphere](sphere.md), [Splat](splat.md)

## Use When

Use point visuals for many circular marks whose size is expressed in pixels and whose centers live
in panel coordinates. They are the baseline choice for scatter plots and point-cloud overlays.

## Avoid When

Use [Marker](marker.md) when each item needs a symbolic shape, [Pixel](pixel.md) for square
screen-aligned cells, or [Sphere](sphere.md) for true 3D radius and lighting.

## Data Model

Create with `dvz_point(scene, flags)`. Upload one item per point, then attach the visual to a
panel. The canonical example uses scalar color values by changing the `color` attribute format and
binding a scale.

## Attributes

| Kind | Attributes |
| --- | --- |
| Required | `position` (`vec3` center), `color` (RGBA8 or configured scalar), `diameter_px` (`float`, pixels) |
| Optional | `item_state` for retained hover/selection styling; edge styling through `dvz_point_set_style()` (`edge_color`, `stroke_width_px`, filled/stroke/outline aspect); alpha mode; depth test; transform; visual-wide scale bindings |

## Picking And Probing

Point visuals expose retained dense attributes for CPU-side bounds and v0.4 picking. Pick
results identify the source item, not the generated quad vertices.

## Backend Notes

Native and WebGPU paths are active. The WebGPU gallery route is live for the RC browser subset.
Depth testing is a visual option; the example disables it for a 2D panzoom panel.

## Canonical Example

| Field | Value |
| --- | --- |
| Source | `examples/c/visuals/point.c` |
| Gallery | [Point](../../examples/gallery/visuals/point_2d.md) |
| Build | `just example-c visuals/point` |
| Smoke | `./build/examples/c/visuals/point --png` |
| Validation | `smoke+screenshot` |

## See Also

[Choose a visual family](../../how-to/choose-a-visual-family.md),
[use panzoom](../../how-to/use-panzoom.md), [pick items](../../how-to/pick-items.md),
[Pixel](pixel.md), [Marker](marker.md), [Sphere](sphere.md).
