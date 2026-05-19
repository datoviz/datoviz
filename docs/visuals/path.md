# Path Visual

The **Path** visual renders ordered polylines: sequences of connected vertices that form one or
more open paths. In v0.4-dev it is the stroke/line family for contours, traces, trajectories,
field lines, tracks, and line-based annotations.

Path is distinct from future tube/ribbon rendering. A path may contain 3D positions, but its
thickness is a screen-space stroke. Radius-bearing 3D curve surfaces belong to the future
`tube` visual contract.

!!! note

    This page describes the v0.4-dev path direction. The Python examples shown below may still use
    legacy naming until the public bindings and gallery are regenerated from the v0.4 C API.


<figure markdown="span">
![Path visual](https://raw.githubusercontent.com/datoviz/data/main/gallery/visuals/path.png)
</figure>


---

## Overview

- Renders connected polylines from vertex sequences.
- Supports dense per-vertex color.
- Supports optional dense per-point `stroke_width` in screen pixels.
- Supports explicit open subpath lengths in the stroked path lane.
- Uses primitive line-strip rendering when `stroke_width` is absent.
- Lowers to segment-style screen-space strokes when `stroke_width` is present.

!!! warning

    Path-native joins, closed subpaths, path-specific caps, dashes, path picking, and WGSL lowering
    are not implemented yet. Stroked paths currently lower to independent segment-style strokes
    with butt caps.

---

## When to use

Use the path visual when:

- You want to draw 2D or 3D trajectories, contours, tracks, or field lines.
- You need continuous polylines with per-vertex color.
- You want optional screen-space stroke width.
- You want to batch multiple independent open paths in one visual.

Use a future `tube` visual, or a precomputed `mesh` fallback today, when the curve should be a 3D
object with physical radius, surface normals, lighting, and SSAO/G-buffer participation.

---

## Properties

### Per-item

| Attribute | Type | Description |
|---|---|---|
| `position` | `(N, 3) float32` | Vertex positions in visual space |
| `color` | `(N, 4) uint8` | RGBA color per vertex |
| `stroke_width` | `(N,) float32` | Optional screen-space stroke width in pixels |

### Per-visual (uniform)

| Parameter | Type | Description |
|---|---|---|
| subpath lengths | `uint32[M]` | Optional open subpath vertex counts via `dvz_path_set_subpaths()` |

Path cap and join parameters are target-contract features, not active v0.4-dev behavior.


## Planned caps and joins

The target path contract includes endpoint caps and corner joins. These are planned features:

| Cap Name       | Image |
|----------------|------|
| `round`        | ![cap_round](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/segment_round.png)    |
| `triangle_in`  | ![cap_triangle_in](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/segment_triangle_in.png)    |
| `triangle_out` | ![cap_triangle_out](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/segment_triangle_out.png)    |
| `square`       | ![cap_square](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/segment_square.png)    |
| `butt`         | ![cap_butt](https://raw.githubusercontent.com/datoviz/data/main/screenshots/guide/segment_butt.png)    |


## Join styles

| Join Name |
|---|
| `miter` |
| `bevel` |
| `round` |


---

## Grouping paths

Each visual can include multiple independent open paths. In the v0.4 C API, the packed `position`
array is accompanied by explicit subpath lengths:

```c
dvz_path_set_subpaths(path, subpath_count, lengths);
```

When unset, all points belong to one open path. Current thin line-strip rendering does not yet use
subpath lengths; the stroked path lane does.

---

## Large-scale paths

For very large paths, use the primitive line-strip lane when possible. Use `stroke_width` only when
screen-space thickness is needed, because the current stroked lane derives segment-style resources
from the source path.

---

## Example

```python
--8<-- "cleaned/visuals/path.py"
```

---

## Summary

The path visual is ideal for rendering styled, continuous line sequences.

* yes: dense positions and colors
* yes: optional per-point `stroke_width`
* yes: multiple open subpaths in the stroked lane
* deferred: path-native caps, joins, closed paths, dashes, picking, WGSL parity
* separate future family: 3D tubes and ribbons

See also:

* [**Segment**](segment.md) for unconnected lines
* [**Basic**](basic.md) for large 1-pixel polylines
