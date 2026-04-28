# Scene Visual Mini-Contracts

This document defines family-level mini-contracts for the current preferred v0.4 visual families.

It refines:

1. `VISUAL_FAMILIES.md`
2. `VISUAL_CONTRACT.md`
3. `RESOURCE_MODEL.md`

It does not define the final public C API.


## Purpose

The purpose of these mini-contracts is to make each retained family concrete enough that:

1. family boundaries are explicit,
2. resource schemas are comparable,
3. variant axes are constrained,
4. planning and picking expectations are visible,
5. future implementation work does not drift back toward backend-shaped design.


## Shared Template

Each family mini-contract specifies:

1. semantic purpose,
2. scene-facing resource classes,
3. parameter schema,
4. transform model,
5. stage participation,
6. picking model,
7. variant axes,
8. fallback notes.


## `primitive`

### Semantic Purpose

`primitive` is a first-class low-level visual family for explicit primitive-driven rendering.

It exists to:

1. expose a durable minimal renderable family,
2. support experimentation and bring-up,
3. serve as a baseline pressure test for the broader visual contract.


### Resource Classes

Typical resource classes:

1. `ItemTable`
2. optional `IndexedGeometry`
3. optional `StyleBlock`

The family should stay close to primitive-oriented data rather than adopting family-specific semantic
payloads.


### Parameter Schema

Expected parameters:

1. primitive topology or equivalent mode,
2. family-wide sizing or linewidth controls when applicable,
3. minimal style controls required by the selected primitive mode.


### Transform Model

Allowed:

1. no transform,
2. panel-camera transform,
3. panel-camera plus visual-local transform.


### Stage Participation

Default:

1. render-only

Optional:

1. picking participation when the chosen primitive mode supports it.


### Picking Model

Picking should be optional and simple.

If supported, it should map directly to item identity without family-specific interpretation logic.


### Variant Axes

Allowed axes:

1. primitive mode,
2. indexed versus non-indexed path when relevant,
3. minimal style mode differences.


### Fallback Notes

`primitive` should remain intentionally constrained.

If a use case needs richer semantic behavior, the preferred direction is usually another family rather
than continuously expanding `primitive`.


## `pixel`

### Semantic Purpose

`pixel` represents a filled square pixel-like mark.

It is intentionally simpler than `point` and `marker`.

It should not carry:

1. arbitrary shape semantics,
2. rotation semantics,
3. marker-style edge treatment.


### Resource Classes

Typical resource classes:

1. `ItemTable`
2. optional `StyleBlock`

The item table is expected to contain per-item pixel placement and color data.


### Parameter Schema

Expected parameters:

1. pixel sizing policy,
2. family-wide color or opacity defaults when needed.

Not expected:

1. shape selector,
2. angle or rotation,
3. marker-edge controls.


### Transform Model

Allowed:

1. panel-camera transform,
2. panel-camera plus visual-local transform when justified

The semantics should stay pixel-oriented rather than marker-oriented.


### Stage Participation

Default:

1. render-only


### Picking Model

Picking is optional but straightforward.

If supported, the mapping should be item-based and should not introduce marker-like sub-identity.


### Variant Axes

Allowed axes should remain minimal:

1. sizing policy,
2. optional fixed-versus-scaled pixel behavior.

The family should not grow a shape system.


### Fallback Notes

Because `pixel` is already the simplest mark family, fallback pressure should usually be low.


## `point`

### Semantic Purpose

`point` represents a point-like mark family richer than `pixel` but simpler than `marker`.

It should support point semantics such as:

1. per-item size,
2. per-item color,
3. standard point interaction and picking behavior.


### Resource Classes

Typical resource classes:

1. `ItemTable`
2. optional `StyleBlock`


### Parameter Schema

Expected parameters:

1. point sizing behavior,
2. optional family-wide defaults for appearance,
3. optional scale policy.

Not expected by default:

1. marker-shape selection,
2. rotation controls,
3. marker-edge styling.


### Transform Model

Allowed:

1. panel-camera transform,
2. panel-camera plus visual-local transform.


### Stage Participation

Default:

1. render-only

Optional:

1. picking participation.


### Picking Model

Picking should be natural for this family and usually item-based.


### Variant Axes

Allowed axes:

1. size policy,
2. scaling policy,
3. optional render-quality differences that do not turn the family into `marker`.


### Fallback Notes

If a feature request introduces shape, edge, or rotation semantics, that is usually pressure toward
`marker`, not toward expanding `point`.


## `marker`

### Semantic Purpose

`marker` represents shaped point-like marks with richer visual semantics than `point`.

This family is where scene-visible concepts such as:

1. shape,
2. rotation,
3. edge treatment,
4. richer symbolic styling

belong by default.


### Resource Classes

Typical resource classes:

1. `ItemTable`
2. `StyleBlock`


### Parameter Schema

Expected parameters:

1. edge color and width,
2. shape-related defaults,
3. scaling controls,
4. optional family-wide rotation or style defaults.


### Transform Model

Allowed:

1. panel-camera transform,
2. panel-camera plus visual-local transform.


### Stage Participation

Default:

1. render-only

Optional:

1. picking participation.


### Picking Model

Picking should usually be item-based, with no ambiguity about which marker instance was hit.


### Variant Axes

Allowed axes:

1. marker shape mode,
2. edge mode,
3. scaling mode,
4. optional quality differences.


### Fallback Notes

If a runtime cannot support a richer marker path, the preferred fallback is usually a simpler marker
variant or a downgrade toward `point`, provided the loss is reported clearly when visible to users.


## `segment`

### Semantic Purpose

`segment` represents endpoint-defined linear primitives.

It is distinct from `path` because the primary semantic unit is an independent segment, not a grouped
ordered sequence.


### Resource Classes

Typical resource classes:

1. `ItemTable`
2. `StyleBlock`

Each item is expected to carry endpoint-oriented data.


### Parameter Schema

Expected parameters:

1. linewidth policy,
2. cap controls,
3. optional transform-related style settings.


### Transform Model

Allowed:

1. panel-camera transform,
2. panel-camera plus visual-local transform.


### Stage Participation

Default:

1. render-only

Optional:

1. picking participation.


### Picking Model

Picking should usually resolve to the segment item identity.


### Variant Axes

Allowed axes:

1. cap style,
2. linewidth mode,
3. optional anti-alias or quality path.


### Fallback Notes

If joins, topology, or grouped ordering become central, that is pressure toward `path`, not toward
expanding `segment`.


## `path`

### Semantic Purpose

`path` represents grouped ordered sequences.

This family owns scene semantics such as:

1. path topology,
2. joins,
3. caps,
4. grouped path identity,
5. stacked or wiggle-like path modes.


### Resource Classes

Typical resource classes:

1. `GroupedItemTable`
2. `StyleBlock`


### Parameter Schema

Expected parameters:

1. join mode,
2. cap mode,
3. linewidth policy,
4. topology mode,
5. path-layout or stacking controls when relevant.


### Transform Model

Allowed:

1. panel-camera transform,
2. panel-camera plus visual-local transform.


### Stage Participation

Default:

1. render-only

Optional:

1. picking participation,
2. compute-assisted variants later if clearly justified.


### Picking Model

Picking should be able to resolve at least:

1. path-group identity,
2. optionally finer-grained item identity when supported.


### Variant Axes

Allowed axes:

1. open versus closed,
2. join and cap policy,
3. stacked or wiggle-like path mode,
4. optional quality or anti-aliasing path.


### Fallback Notes

Wiggle-like behavior should remain path-scoped unless it later proves to require a materially
different family contract.


## `glyph`

### Semantic Purpose

`glyph` represents grouped text- or symbol-like instances driven by layout and atlas-backed sampling.

It is the family for:

1. text-like placement,
2. symbol placement with layout semantics,
3. anchored, shifted, or grouped label-like rendering.


### Resource Classes

Typical resource classes:

1. `GroupedItemTable`
2. `SampledField`
3. `StyleBlock`


### Parameter Schema

Expected parameters:

1. glyph sizing controls,
2. anchor controls,
3. background or antialias controls,
4. layout-related defaults.


### Transform Model

Allowed:

1. panel-camera transform,
2. panel-camera plus visual-local transform,
3. layout-aware transforms derived from group placement.


### Stage Participation

Default:

1. render-only

Optional:

1. picking participation.


### Picking Model

Picking should be able to resolve:

1. glyph-group identity,
2. optionally glyph-instance identity when needed.


### Variant Axes

Allowed axes:

1. text-like versus symbol-like modes,
2. atlas or layout mode,
3. optional quality or antialias mode.


### Fallback Notes

`monoglyph` should not return as a separate family.
Any useful simplification should be a `glyph` variant or implementation path.


## `image`

### Semantic Purpose

`image` represents placed sampled images.

This family owns semantics such as:

1. image placement,
2. anchor and scaling behavior,
3. color interpretation modes,
4. border and fill behavior.

Volume slice display belongs to the `volume` family (`render_mode = slice`), not to `image`.
`image` handles flat 2D rasters only.


### Resource Classes

Typical resource classes:

1. `SampledField`
2. `StyleBlock`

Optional:

1. `DerivedField` when the image mode depends on planned extraction or intermediate preparation.


### Parameter Schema

Expected parameters:

1. anchor controls,
2. size or scaling policy,
3. border controls,
4. colormap or fill controls.


### Transform Model

Allowed:

1. panel-camera transform,
2. panel-camera plus visual-local transform,
3. image placement in a 3D scene when the family mode requires it.


### Stage Participation

Default:

1. render-only

Optional:

1. picking participation for modes where it is meaningful,
2. offscreen/export participation.


### Picking Model

Picking should be optional and mode-dependent.

It should at least be able to identify the image instance, and may optionally resolve finer logical
sub-identity later if a mode requires it.


### Variant Axes

Allowed axes:

1. rgba versus colormap versus fill mode,
2. placement and scaling policy,
3. optional border or rounded-corner mode.


### Fallback Notes

`image` is already the simpler 2D raster family.
If image placement or colormap mode is unavailable, the preferred fallback is a display
simplification rather than collapsing into another family.
Volume slice display has its own fallback path within the `volume` family.


## `mesh`

### Semantic Purpose

`mesh` represents indexed geometry with optional material, contour, normal, and sampled-field support.

It is the primary family for geometry-heavy scientific surfaces and triangle-based renderables.


### Resource Classes

Typical resource classes:

1. `IndexedGeometry`
2. `StyleBlock`

Optional:

1. `SampledField`


### Parameter Schema

Expected parameters:

1. material controls,
2. lighting controls,
3. contour controls,
4. optional mesh-style quality settings.


### Transform Model

Allowed:

1. panel-camera transform,
2. panel-camera plus visual-local transform.


### Stage Participation

Default:

1. render-only

Optional:

1. picking participation,
2. compute-assisted variants later if justified.


### Picking Model

Picking should usually be supported at least at the mesh-instance level.

When one mesh visual represents several stable logical parts, the preferred contract should also
support semantic group or region identity rather than only primitive identity.

Examples:

1. one anatomical region inside an atlas mesh collection,
2. one labeled parcel inside a cortical surface,
3. one submesh partition inside a scientific assembly.

Primitive or face-level payload may remain optional, but it should refine semantic identity rather
than replace it when the scene model already has stable logical parts.


### Variant Axes

Allowed axes:

1. colored versus textured,
2. lit versus unlit,
3. contour-enhanced mode,
4. optional compute-assisted preparation mode.


### Fallback Notes

If a richer mesh variant is unavailable, fallback should stay within the mesh family whenever the core
geometry semantics remain intact.


## `sphere`

### Semantic Purpose

`sphere` represents sphere semantics as a first-class family.

It should remain distinct from both `point` and `mesh`, even when some implementations borrow from
those families.


### Resource Classes

Impostor-first default:

1. `ItemTable`
2. `StyleBlock`

Optional mesh-backed variant:

1. `IndexedGeometry`
2. `StyleBlock`


### Parameter Schema

Expected parameters:

1. radius or scale policy,
2. family-wide appearance controls,
3. optional lighting or shading controls,
4. optional quality selectors between impostor and mesh-backed variants.


### Transform Model

Allowed:

1. panel-camera transform,
2. panel-camera plus visual-local transform.


### Stage Participation

Default:

1. render-only

Optional:

1. picking participation.


### Picking Model

Picking should be natural for this family and should usually identify sphere instances directly.


### Variant Axes

Allowed axes:

1. impostor-first rendering path,
2. mesh-backed rendering path,
3. optional quality or lighting differences inside those paths.


### Fallback Notes

The default conceptual path should be impostor-first.

If a runtime cannot support a preferred sphere path, fallback may remain inside the sphere family by
switching variants rather than collapsing into `point` or `mesh` semantically.


## `volume`

### Semantic Purpose

`volume` represents volumetric field rendering.

It is distinct from `image` because the core semantic unit is volumetric content rather than a placed
sampled image.


### Resource Classes

Typical resource classes:

1. `SampledField`
2. `StyleBlock`

Optional:

1. `DerivedField`


### Parameter Schema

Expected parameters:

1. transfer-function controls,
2. traversal or compositing controls,
3. permutation or orientation controls,
4. volume-domain placement controls.


### Transform Model

Allowed:

1. panel-camera transform,
2. panel-camera plus visual-local transform.


### Stage Participation

Default:

1. render-only

Optional:

1. offscreen/export participation,
2. compute-assisted variants later if justified.


### Picking Model

Picking is optional and likely more constrained than for simpler families.

If supported, the contract should clearly state what identity is being picked: volume instance,
sampled projection, or some later-defined sub-identity.


### Variant Axes

Allowed axes:

1. direct versus colormap mode,
2. traversal or compositing mode,
3. optional quality mode.


### Fallback Notes

If a runtime cannot support a richer volume path, fallback should remain semantic and explicit rather
than silently degrading into an unrelated family.
