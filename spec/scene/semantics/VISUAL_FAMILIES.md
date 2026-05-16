# Scene Visual Families

This document defines the preferred visual-family vocabulary for the future scene layer.

It is not a frozen public API.

Its goal is narrower:

1. keep the useful broad `v0.3` idea of visuals,
2. clean up the family taxonomy for v0.4,
3. separate family identity from rendering variants,
4. avoid backend-shaped naming in the scene spec.


## Core Rule

A visual family is a scene-level semantic category of renderable.

A family should be separate only when it materially changes at least one of:

1. scene interaction semantics,
2. resource schema,
3. stage participation,
4. picking semantics,
5. capability or fallback behavior.

If a difference is only about shading, pipeline details, or implementation strategy, it should usually
be modeled as a variant rather than a distinct family.


## Relationship To `v0.3`

The local `v0.3` subtree provides the right broad vocabulary baseline:

1. `primitive`
2. `pixel`
3. `point`
4. `marker`
5. `segment`
6. `path`
7. `glyph`
8. `monoglyph`
9. `image`
10. `wiggle`
11. `mesh`
12. `sphere`
13. `volume`
14. `slice`

The future scene spec should remain recognizable to users familiar with those names.

However:

1. `v0.3` is informative, not definitive,
2. v0.4 may merge, remove, rename, or re-scope families when it improves architecture,
3. compatibility with the `v0.3` public API is not a requirement.


## Keep As First-Class Families

The following families should remain first-class scene concepts unless later evidence shows they are
artificial:

1. `primitive`
2. `pixel`
3. `point`
4. `marker`
5. `segment`
6. `path`
7. `glyph`
8. `image`
9. `mesh`
10. `sphere`
11. `volume`
12. `errorbar`
13. `boxplot`


## Active Implementation Status

This table is an implementation snapshot, not the complete family contract. "Native rendering"
means the family lowers through the active scene -> FramePlan -> DRP2 -> vklite/canvas path.

| Family | Public constructor/API | Retained state | Native rendering | GPU request/readback | Remaining gaps |
|---|---|---|---|---|---|
| `pixel` | `dvz_pixel()` | position/color/size, depth-cue state | emission and pipeline-selection coverage; no separate app readback smoke recorded | none specific | broader pixel-specific picking and styling are not active |
| `primitive` | `dvz_primitive()` | topology, position/color, optional normals/index buffers, material/depth/alpha state | point/line/triangle primitives, indexed draws, depth, WBOIT/depth-peel participation | none specific | remains a low-level escape hatch, not a replacement for richer families |
| `point` | `dvz_point()` | position/color/size, external position buffers, depth-cue/alpha state | native point path with retained updates and multi-panel coverage | active point pick readback | richer pick payloads, selection highlight rendering, and marker shapes are separate work |
| `path` | `dvz_path()` | position/color line-strip state | first slice as primitive line strip | none specific | widths, caps, joins, tapered lines, grouping, and path picking remain deferred |
| `image` | `dvz_image()`, `dvz_visual_set_field()`, texture convenience wrappers | 2D `SampledField`, colormap scale binding, partial updates | textured quad path with scalar colormap lowering and retained texture updates | active basic image probe readback | richer probe payloads, labels/categorical fields, per-item rectangles, anchors, and tinting remain deferred |
| `mesh` | `dvz_mesh()` | position, optional color/normal/index buffers, material/depth/alpha state | indexed triangle mesh path with depth, Phong/material, WBOIT/depth-peel, EDL/SSAO/G-buffer participation where eligible | none specific | broader mesh/object picking, geometry-resource public shape, and full PBR remain deferred |
| `sphere` | `dvz_sphere()`, `dvz_sphere_*()` setters | impostor mode, center/color/radius, material/depth state | analytic sphere impostor path, including raycast mode and SSAO/G-buffer coverage | none specific | texture variants, per-item material/PBR, and sphere picking are not active |
| `volume` | `dvz_volume()`, volume setters, `dvz_visual_set_field()` | 3D `SampledField`, render mode, slice, bounds, clipping, sampling, opacity, scale binding | box-proxy volume renderer with slice, MIP, and composite paths | none specific | richer transfer functions, isosurfaces, MPR, DVR/MIP picking, and volume probe payloads remain deferred |
| `marker` | none installed | none | no | no | spec only |
| `segment` | none installed | none | no | no | spec only |
| `glyph` | no visual constructor; retained `DvzText`/`DvzAnnotation` bookkeeping exists | text/font/annotation objects retain semantic state | no rendered glyph path | no | font atlas, glyph shaping/rendering, glyph/text picking, and rendered labels remain deferred |
| `errorbar` | none installed | none | no | no | spec only |
| `boxplot` | none installed | none | no | no | spec only |


## Rationale For Kept Families

### `primitive`

Keep `primitive` as an explicit low-level visual family.

Reason:

1. it provides a controlled low-level escape hatch,
2. it can support experimentation without forcing new family design immediately,
3. it is still useful as a baseline pressure test for the visual contract.

`primitive` should stay intentionally constrained and should not become the default answer to every new
rendering need.

It is expected to remain a long-term first-class family rather than a temporary compatibility
convenience.


### `pixel`

Keep `pixel` separate from `point`.

Reason:

1. pixel semantics are simpler,
2. pixel visuals do not need the same feature surface as point visuals,
3. keeping them separate preserves a meaningful user-facing distinction.


### `point`

Keep `point` as a distinct family from `pixel` and `marker`.

Reason:

1. point visuals usually carry richer per-item sizing and interaction semantics than pixels,
2. they are still simpler than marker visuals with shape and edge treatment,
3. they are a common scientific primitive in their own right.


### `marker`

Keep `marker` separate from `point`.

Reason:

1. shape, rotation, and edge styling materially affect the family contract,
2. marker-like visuals often have different picking and styling semantics than simple points,
3. the extra expressive surface is important enough to justify a separate family.


### `segment`

Keep `segment` as its own family.

Reason:

1. endpoint-based semantics differ from point- or path-based semantics,
2. caps, widths, and endpoint-local styling are central to the family,
3. it remains a common primitive for graphs, rulers, and overlays.


### `path`

Keep `path` as its own family.

Reason:

1. grouped sequence semantics are fundamental,
2. joins, caps, ordering, and topology matter at the scene level,
3. it is the natural home for path-derived specializations such as stacked or wiggle-like visuals.


### `glyph`

Keep `glyph` as its own family.

Reason:

1. text and symbol layout semantics are different from generic image or marker semantics,
2. atlas-backed grouped instance data is central to the family,
3. picking and anchoring behavior are distinctive enough to justify a dedicated family.

`glyph` is the renderable visual-family contract for shaped text/glyph runs. Higher-level retained
text, annotation, axis-label, legend, and colorbar objects are semantic scene objects that may lower
to `glyph` contributions. They are not separate top-level visual families unless a later spec proves
that their resource schema, picking behavior, or fallback policy materially differs from `glyph`.


### `image`

Keep `image` as its own family.

Reason:

1. sampled-field semantics are central,
2. placement, anchoring, scaling, and colormap modes are scene-visible concepts,
3. flat 2D raster, atlas, and heatmap semantics are distinct from 3D volume slicing.


### `mesh`

Keep `mesh` as its own family.

Reason:

1. indexed geometry plus optional normals, material, Phong, and depth-cue state is a distinct
   resource schema,
2. mesh semantics differ materially from point, marker, and image families,
3. mesh is likely to remain central for scientific surfaces and geometry-heavy scenes.


### `sphere`

Keep `sphere` as a first-class family.

Reason:

1. sphere semantics are important enough to deserve a stable scene-level concept,
2. users should not be forced to think of spheres only as a special case of `mesh`,
3. the family can still support multiple rendering variants without losing its identity.

The likely variants are:

1. impostor-style rendering,
2. mesh-backed rendering,
3. other sphere-specific paths later if needed.

The fact that a mesh visual can render triangulated spheres does not remove the value of a distinct
`sphere` family at the scene level.


### `volume`

Keep `volume` as its own family.

Reason:

1. volumetric field semantics differ materially from image and mesh,
2. transfer-function and traversal semantics are family-level concerns,
3. capability adaptation is likely to be more significant here than in simpler families.


### `errorbar`

Keep `errorbar` as its own family.

Reason:

1. statistical error semantics (asymmetric per-axis extents, cap treatment) differ from raw
   segment endpoint data,
2. it is a first-class scientific primitive with distinct picking and layout conventions,
3. keeping it explicit simplifies integration with axes and legends.


### `boxplot`

Keep `boxplot` as its own family.

Reason:

1. the five-value statistical summary (whisker low/high, box low/high, median) is a
   distinct semantic unit that cannot be expressed cleanly via `segment` or `primitive`,
2. the `DVZ_BOXPLOT_DIRECTIONAL` / candlestick variant requires per-item directional body
   coloring that differs fundamentally from uniform box-and-whisker semantics,
3. it is a common primitive in both scientific and financial visualization.


## Remove As First-Class Families

The following `v0.3` family should not carry forward as a first-class v0.4 family:

1. `monoglyph`

Reason:

1. it was an unsuccessful specialization,
2. it does not justify a separate long-term family identity,
3. any surviving useful behavior should be absorbed into `glyph` variants or implementation details.


## Reframe As Variants Or Subfamilies

The following `v0.3` names should not be treated as default top-level families yet:

1. `wiggle`
2. `slice`


### `wiggle`

Current direction:

1. treat as a specialized `path` mode, variant, or subfamily,
2. keep the door open to promote it later if it needs a materially different contract.

There is no `spec/scene/visuals/WIGGLE.md`. Wiggle behavior is covered by `PATH.md` under
the `path` family's variant axis.

Promotion to a full family would require evidence that it has meaningfully different:

1. grouped-data schema,
2. planning behavior,
3. interaction semantics,
4. capability or fallback rules.


### `slice`

`slice` is a render mode of the `volume` family, not a variant of `image`.

2D slicing through a 3D volume is handled by `dvz_volume` with `render_mode = slice` (see
`visuals/VOLUME.md`). The `image` family handles flat 2D rasters only; it does not model 3D
volume slicing. Slice parameters (`slice_axis`, `slice_position`) are visual-wide parameters
of `volume` in slice mode.

The important rule is that scene terminology should describe semantics, not texture dimensionality or
backend object shape.


## Family Versus Variant

The following are good candidates for variants rather than separate families:

1. `image`: rgba, colormap, fill modes (flat 2D rasters only; 3D slicing belongs to `volume`)
2. `mesh`: colored, textured, lit, contour-enhanced
3. `sphere`: impostor-style, mesh-backed, or other sphere-specific rendering paths
4. `path`: plain, stacked, wiggle-like, closed/open variants
5. `glyph`: text-like, symbol-like, atlas-backed variants
6. `volume`: direct-color, colormap, alternate traversal or transfer modes


## Family Contracts

Every retained family should eventually specify:

1. semantic purpose,
2. resource schema,
3. parameter schema,
4. transform model,
5. stage participation,
6. picking model,
7. variant axes,
8. fallback rules.

Those family-level details should refine `../semantics/VISUAL_CONTRACT.md`, not replace it.


## Immediate v0.4 Family Set

The current preferred v0.4 family set is:

1. `primitive`
2. `pixel`
3. `point`
4. `marker`
5. `segment`
6. `path`
7. `glyph`
8. `image`
9. `mesh`
10. `sphere`
11. `volume`
12. `errorbar`
13. `boxplot`

With the following provisional non-top-level concepts:

1. `wiggle` under `path`
2. `slice` under `volume` (render mode, not under `image`)
