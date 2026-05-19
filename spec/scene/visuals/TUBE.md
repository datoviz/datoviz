# Visual Family: `tube`

This document defines the future contract for rendering 3D curve-derived surfaces such as tubes,
capsules, streamtubes, and ribbons.

Status on 2026-05-19: `tube` is **spec-only**. There is no installed `dvz_tube()` constructor,
public enum, shader path, picking path, or DRP2 lowering yet. Current examples should use
`path`, `segment`, `primitive`, or precomputed `mesh` fallbacks until an implementation lands.

This spec extracts generic visual-family requirements from the tractography, flow-field, tokamak,
tracks, particle-trail, and protein-ribbon notes. Domain examples should link here instead of
defining independent curve-rendering family names.


## Semantic Purpose

`tube` renders packed 3D curves with apparent or physical radius and surface-like depth behavior.
The input is curve data; the visual result is a 3D object with a visible cross-section.

The family boundary is:

| Family | Meaning |
|---|---|
| `path` | ordered vertices rendered as a screen-space stroke or raw line strip |
| `tube` | ordered vertices rendered with radius, surface depth, and optional normals/lighting |
| `mesh` | explicit user-provided indexed triangle geometry |

This distinction is not 2D versus 3D coordinates. A `path` may contain 3D positions. The
distinction is **stroke representation** versus **surface/radius representation**.


## Typical Uses

1. diffusion MRI tractography streamlines;
2. CFD, weather, plasma, and magnetic-field streamlines or pathlines;
3. vessels, neurites, skeletons, and other radius-bearing centerlines;
4. molecular bonds and cartoon-like tube/ribbon geometry;
5. particle trajectories and selected tracks when a 3D surface is desired;
6. presentation-quality trajectory rendering with depth, lighting, and occlusion.


## Item and Span Model

`tube` is a span-structured visual. Each logical span is one curve. The vertices within the span
are ordered samples along that curve.

The user-facing data model should be the same packed polyline layout used by `path`:

```text
position[N, 3]
offsets[M + 1] or subpath_lengths[M]
optional per-vertex attributes[N]
optional per-curve attributes[M]
```

Where `N` is the total vertex count and `M` is the curve count.

Picking should default to curve identity: a hit returns the logical curve/span id, not an anonymous
generated segment, triangle, or proxy primitive. Optional sub-hit payloads may later include segment
index, vertex interval, normalized curve coordinate, or physical hit position.


## Core Attributes

### `position`

| Property | Value |
|---|---|
| Type | `vec3`, `(x, y, z)` in visual space |
| Accepted sources | `PER_ITEM` only |
| Typical mutability | `dynamic` or `streaming` |


### `radius`

| Property | Value |
|---|---|
| Type | `float32` |
| Unit | visual/data-space units by default |
| Accepted sources | `CONSTANT`, `PER_ITEM`, `PER_SPAN` |
| Typical mutability | `dynamic` |

`radius` is not `stroke_width`. `stroke_width` belongs to `path` and `segment` and is measured in
screen pixels by default. `radius` describes a 3D curve surface. A tube farther from the camera
should normally appear smaller.

A later `radius_space = screen` mode may be useful for hybrid scientific overlays, but the default
semantic contract is data-space radius.


### `color`

Standard — see `SHARED_ATTRIBUTES.md`. Per-vertex color should interpolate along a curve.
Accepted sources: `CONSTANT`, `PER_ITEM`, `PER_SPAN`.

Typical color modes:

1. local direction RGB for tractography and field lines;
2. scalar colormap along the curve;
3. per-curve categorical color;
4. selected/highlight overlay color.


### Optional `tangent`

| Property | Value |
|---|---|
| Type | `vec3` |
| Accepted sources | `PER_ITEM` |
| Typical mutability | `static` or `dynamic` |

If absent, the scene may derive tangents from neighboring positions. Supplying tangents avoids
ambiguous finite differences for short segments, repeated points, or externally smoothed curves.


### Optional `curve_id`

| Property | Value |
|---|---|
| Type | `uint32` |
| Accepted sources | `PER_SPAN` |
| Typical mutability | `static` |

`curve_id` gives the application a stable semantic id independent of packed span order. It is useful
for tractography streamline ids, track ids, vessel ids, skeleton branch ids, and linked selections.


## Render Modes

`tube` is a semantic family. The mode chooses the implementation strategy without changing the
input data contract.

### `line_fallback`

Render the curves as `path` or `primitive` line strips.

This mode preserves the data path when tube rendering is unsupported, but it does not provide a
surface normal, true radius, or tube-like depth.


### `impostor_tube`

Render each curve segment through proxy geometry and reconstruct tube coverage, normal, and depth
in the fragment shader.

Common proxy forms:

1. camera-facing segment quads;
2. capsule impostors per segment;
3. uncapped cylinder impostors plus vertex sphere impostors;
4. short curve-piece proxies when adjacency data is available.

This is the preferred scalable direction for dense tractography and streamline datasets. It should
produce surface-like normals/depth without expanding every curve sample into many mesh vertices.


### `mesh_tube`

Generate explicit tube triangles from curve samples, local frames, cross-sections, and indices.

This mode is useful for moderate curve counts, offline export, high-quality captures, and workflows
that need ordinary mesh behavior. It should reuse `mesh` material, depth, transparency, G-buffer,
and SSAO infrastructure whenever possible.


### `ribbon`

Generate or impostor-render a strip along the curve instead of a circular cross-section.

Ribbon modes may be:

1. camera-facing ribbons for cheap visibility;
2. oriented ribbons using a transported frame or external normal;
3. domain-specific ribbons such as protein cartoons, when generated as explicit mesh data.

Ribbons are part of the `tube`/curve-surface design space because they share ordered-curve data,
radius/width-like styling, frame-generation concerns, and surface-like pass participation.


## Joins and Caps

Tube joins are not the same as 2D path joins.

For impostor tubes, the first robust join policy should be round joins:

```text
curve = union of segment capsules
```

or:

```text
uncapped segment cylinders + one sphere impostor per internal vertex
```

This avoids cracks at bends and keeps dense tractography/streamlines practical. It may produce
rounder, ball-like joins, which is acceptable for the first scalable implementation.

Later policies may include:

1. generated mesh round joins;
2. beveled joins;
3. miter-like joins for sparse presentation curves;
4. frame-continuous swept surfaces for smooth ribbons/tubes;
5. application-provided resampled or smoothed curves.

Caps should start with:

| Cap | Meaning |
|---|---|
| `none` | open end with no extra surface |
| `flat` | planar cut perpendicular to curve tangent |
| `round` | hemispherical or capsule-style end |


## Frame Generation

Mesh tubes and oriented ribbons need a stable local frame at each curve sample.

Preferred order:

1. use application-supplied tangent/normal/frame data when available;
2. otherwise derive tangents from positions;
3. use parallel transport frames for stable tube/ribbon orientation;
4. avoid pure Frenet frames as the default because they are unstable near zero curvature;
5. smooth or resample curves only as an explicit preprocessing or visual option.

Impostor tubes may not need a full transported frame for round capsules, but still need reliable
segment axes and screen-space bounds.


## Stage Participation

Target pass capabilities:

| Mode | Color | Depth | Normal/depth | Transparency | Picking |
|---|---|---|---|---|---|
| `line_fallback` | yes | limited | no | yes | path-like later |
| `impostor_tube` | yes | yes, corrected | yes, analytic | WBOIT/depth-peel target | curve id |
| `mesh_tube` | yes | yes | yes, mesh normals | mesh-like | curve id plus optional face/segment |
| `ribbon` | yes | yes | mode-dependent | yes | curve id |

The visual should not create a private renderer. It should route through the existing scene ->
FramePlan -> DRP2 -> runtime path and reuse material, alpha, G-buffer, SSAO, EDL, and clipping
infrastructure where those capabilities apply.


## Relationship To Domain Resources

`tube` is not a tractography, flow, track, graph, or molecule resource by itself.

Domain resources should lower into `tube`, `path`, `segment`, `marker`, `sphere`, `mesh`, or
`volume` visuals:

1. tractography streamlines may lower to `path` for preview or `tube` for shaded fibers;
2. vector fields may lower to arrows, particles, streamlines, and `tube`/`path` views;
3. tracks may lower to current-position markers plus `path`/`tube` histories;
4. molecules may lower to spheres, bond tubes/segments, ribbon meshes, and surfaces;
5. skeletons may lower to nodes plus `path` or `tube` branches.


## Implementation Order

Recommended future sequence:

1. keep current `path` and `segment` stroke work separate and finish their picking/WGSL debt;
2. add a spec-only `tube` status row and examples that use existing fallbacks;
3. implement a small `mesh_tube` builder or precomputed-mesh example as a correctness/reference
   path;
4. implement `impostor_tube` for scalable tractography/streamlines;
5. add curve-id picking and selection/highlight;
6. add transparency, depth cueing, and optional SSAO/G-buffer support;
7. add ribbons only after frame-generation rules and mode naming are stable.


## Non-Goals

1. Do not make `path` a catch-all for tubes, ribbons, SVG fills, and domain resources.
2. Do not use `stroke_width` for 3D tube radius.
3. Do not require CPU-generated triangle tubes for all tube rendering.
4. Do not make tractography, streamlines, tracks, or molecules separate renderers.
5. Do not add a parallel presentation or Vulkan path.
