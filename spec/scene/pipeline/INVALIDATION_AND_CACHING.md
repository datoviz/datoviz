# Scene Invalidation And Caching

This document defines how invalidation and caching should work in the future scene layer.

It exists to make explicit a boundary that is already implied by the current scene specs:

1. scene objects are long-lived state,
2. `FramePlan` is per-frame derived state,
3. not every scene mutation should force every derived artifact to be rebuilt,
4. panel interaction must stay responsive without unnecessary reupload.


## Purpose

The invalidation and caching model should:

1. keep scene updates deterministic,
2. avoid rebuilding normalized resources when only panel navigation changes,
3. avoid rebuilding `FramePlan` structure when only cheap panel-local inputs change,
4. allow grouped resources and axes to update incrementally,
5. make redraw and upload policy explicit.


## Core Rule

The scene layer should invalidate the smallest correct scope.

In practice:

1. data-space changes should invalidate normalization-derived resources,
2. panel-navigation changes should usually invalidate panel-local transforms only,
3. some scene changes require a fresh `FramePlan`,
4. some scene changes require only new parameter values or subrange uploads.


## Derived Layers

The current scene architecture implies several distinct layers of state:

1. authored scene state,
2. normalized scene resources,
3. panel-local derived state,
4. axis-derived semantic layout,
5. per-frame `FramePlan`,
6. emitted DRP2 command stream.

The invalidation model should let each of these layers change independently when possible.


## Main Invalidation Scopes

The first scene slice should recognize at least these invalidation scopes:

1. `SceneStructureDirty`
2. `VisualPropsDirty`
3. `ResourceDataDirty`
4. `NormalizationDirty`
5. `PanelTransformDirty`
6. `AxisLayoutDirty`
7. `CapabilityDirty`
8. `FramePlanDirty`
9. `UploadDirty`
10. `ReadbackRoutingDirty`

These are logical scopes, not necessarily final public enum names.


## Scope Meanings


### `SceneStructureDirty`

This means the set of scene objects or their panel membership changed.

Examples:

1. a visual is created or destroyed,
2. a visual is added to or removed from a panel,
3. an axis is attached or detached,
4. an offscreen target is added.

This usually forces:

1. planning revalidation,
2. `FramePlan` rebuild,
3. possible new runtime-side materialization during DRP2 emission.


### `VisualPropsDirty`

This means a visual’s semantic properties changed without necessarily changing the underlying bulk
resource payload.

Examples:

1. color mode changed,
2. picking enablement toggled,
3. line width changed,
4. image display mode changed,
5. sphere variant changed from impostor-first to mesh-backed.

This may require:

1. style-block update only,
2. variant reselection,
3. draw contribution rebuild,
4. `FramePlan` rebuild when stage participation or target usage changes.


### `ResourceDataDirty`

This means a source or derived scene resource changed.

Examples:

1. point positions updated,
2. one path group updated,
3. one image field replaced,
4. one style block rewritten,
5. one sampled field region updated.

This usually implies:

1. upload work,
2. possible normalization invalidation,
3. possible draw contribution rebuild if counts or topology changed.


### `NormalizationDirty`

This means data-to-visual normalization must be recomputed.

Examples:

1. source data changed,
2. domain bounds changed,
3. normalization policy changed,
4. one visual-family interpretation of data changed.

This should usually force:

1. recomputation of derived visual-space resources,
2. upload of those derived resources if they are cached CPU-side,
3. downstream visual contribution refresh.


### `PanelTransformDirty`

This means panel-local panzoom or camera state changed.

Examples:

1. 2D pan changed,
2. 2D zoom changed,
3. 3D camera moved,
4. viewport size changed.

This should usually force:

1. panel transform recomputation,
2. redraw scheduling,
3. possible axis-layout reevaluation,

but should usually not force:

1. source data updates,
2. normalization recomputation,
3. bulk visual resource reupload.


### `AxisLayoutDirty`

This means the semantic layout of one or more axes must be regenerated.

Examples:

1. the visible data domain moved near the edge of the covered domain,
2. zoom changed enough that tick density is no longer acceptable,
3. axis formatter or scale type changed,
4. panel size changed enough that label layout must change.

This may require:

1. rebuilding derived tick values,
2. regenerating derived geometry or glyph payload,
3. uploading only the affected axis-derived resources.


### `CapabilityDirty`

This means the capability record or the chosen adaptation outcome changed in a way that may alter the
scene-visible execution path.

Examples:

1. the runtime capability set changed,
2. a browser versus native runtime switch selected a different adaptation path,
3. picking or readback support availability changed,
4. a degraded variant became required or no longer required.

This may require:

1. variant reselection,
2. capability diagnostics refresh,
3. `FramePlan` rebuild when topology or target usage changes,
4. redraw or interaction-policy update when optional affordances are enabled or disabled.


### `FramePlanDirty`

This means the previously cached plan shape is no longer valid.

Examples:

1. a visual’s stage participation changed,
2. a target or pass requirement changed,
3. picking was enabled or disabled,
4. an offscreen export path was requested,
5. a capability fallback changed the node topology.

This usually forces:

1. rebuilding render and compute node structure,
2. rebuilding dependency ordering,
3. rebuilding logical target usage.


### `UploadDirty`

This means the scene already knows which scene-owned resources changed and only needs to emit the
corresponding upload work.

This is not a user-facing state mutation by itself.
It is the execution-facing consequence of changes such as:

1. `ResourceDataDirty`,
2. `NormalizationDirty`,
3. `AxisLayoutDirty`.


### `ReadbackRoutingDirty`

This means the mapping from execution results back to scene objects changed.

Examples:

1. pick payload layout changed,
2. a visual changed family or picking mode,
3. a readback target was replaced.

This should usually be rare.


## Cache Layers

The first scene slice should assume several distinct caches.


### 1. Source Data Cache

This is the long-lived CPU-owned authored scene data.

Examples:

1. item tables,
2. grouped item tables,
3. sampled fields,
4. style blocks.

This cache is authoritative.


### 2. Normalized Resource Cache

This holds derived visual-ready resources in `VisualSpace`.

Examples:

1. normalized point coordinates,
2. derived geometry payloads,
3. scene-derived mesh-side auxiliary channels,
4. any cached visual-family-ready representation of source data.

This cache should be invalidated by data-space and normalization-policy changes, not by panel pan or
camera motion.


### 3. Panel Derived State Cache

This holds panel-local live state.

Examples:

1. 2D panzoom matrices,
2. 3D camera view/projection state,
3. viewport-dependent panel parameters.

This cache may change every frame and should be cheap to rebuild.


### 4. Axis Layout Cache

This holds the currently prepared semantic tick layout and its derived geometry coverage.

Examples:

1. `visible_data_domain`,
2. `covered_data_domain`,
3. chosen tick step,
4. formatted labels,
5. derived tick and label payloads.

This cache should be rebuilt only when layout invariants stop being satisfied.


### 5. FramePlan Cache

This holds the most recent valid plan shape for the scene or panel.

Depending on the final implementation, this may cache:

1. node topology,
2. dependency ordering,
3. stable draw grouping,
4. panel-target routing.

This cache should be invalidated only when plan structure truly changes.

Capability changes that do not alter topology should not invalidate more than necessary, but any
adaptation change that affects stage participation, target usage, or routing should invalidate this
cache.


## Incremental Update Rules

The guiding rule is:

1. authored state changes invalidate upward only as far as required,
2. panel interaction invalidates downward only as far as required.

That means:

1. not every write forces normalization,
2. not every normalization change forces topology changes,
3. not every panel transform change forces uploads,
4. not every redraw forces `FramePlan` rebuild.


## Typical Change Matrix


### Source Data Write

Examples:

1. new point positions,
2. new path vertices,
3. new volume values.

Usually invalidates:

1. `ResourceDataDirty`
2. `NormalizationDirty`
3. `UploadDirty`

May invalidate:

1. `FramePlanDirty` if counts, topology, or stage participation changed.


### Style Or Visual Parameter Change

Examples:

1. marker edge width,
2. point size,
3. image colormap mode,
4. volume transfer settings.

Usually invalidates:

1. `VisualPropsDirty`
2. `UploadDirty` for a small style-like resource

May invalidate:

1. `FramePlanDirty` if it changes family variant, stage participation, or target usage.


### Panel Pan Or Zoom

Usually invalidates:

1. `PanelTransformDirty`

May invalidate:

1. `AxisLayoutDirty` when layout coverage or density rules are exceeded.

Should usually not invalidate:

1. `NormalizationDirty`
2. source resource uploads


### Camera Move In 3D

Usually invalidates:

1. `PanelTransformDirty`

May invalidate:

1. `FramePlanDirty` if view-dependent participation changes materially,
2. `AxisLayoutDirty` for 3D axis-like overlays if they exist.

Should usually not invalidate:

1. normalized scene resources


### Visual Added Or Removed

Usually invalidates:

1. `SceneStructureDirty`
2. `FramePlanDirty`

May also invalidate:

1. `ReadbackRoutingDirty`


### Picking Enabled Or Disabled

Usually invalidates:

1. `VisualPropsDirty`
2. `FramePlanDirty`
3. `ReadbackRoutingDirty`

It may require:

1. new picking-pass participation,
2. new pick payload encoding,


### Capability Or Adaptation Change

Usually invalidates:

1. `CapabilityDirty`

May invalidate:

1. `VisualPropsDirty` when a fallback changes active variant semantics,
2. `FramePlanDirty` when pass topology or target usage changes,
3. `ReadbackRoutingDirty` when picking or readback paths are enabled, disabled, or reshaped.

Should not invalidate:

1. source authored data by itself,
2. normalization state unless the adapted path explicitly changes normalization semantics.
3. new readback planning.


### Offscreen Export Requested

Usually invalidates:

1. `FramePlanDirty`

May also invalidate:

1. `ReadbackRoutingDirty`


## ItemTable Updates

For `ItemTable`, the normal incremental unit is the item row or a contiguous row range.

This should support:

1. one-item updates,
2. contiguous subrange updates,
3. full-table replacement.

The scene layer should not require reupload of the whole table when only a small subrange changed,
unless the implementation lacks a better path temporarily.


## GroupedItemTable Updates

For `GroupedItemTable`, the scene model should recognize both:

1. item-level dirtiness,
2. group-level dirtiness.

This matters because grouped families have two valid incremental granularities:

1. change one vertex or glyph item inside a group,
2. replace or relayout one whole group.

The scene layer should keep the grouped abstraction explicit because the batching goal and the
semantic goal differ:

1. one GPU-friendly batch may contain many groups,
2. scene semantics still need to know which group changed,
3. picking and selection may target group identity,
4. only the affected item ranges should be invalidated when possible.

This makes `GroupedItemTable` a strong fit for path- and glyph-like families rather than a temporary
hack.


## Axes: Cheap Motion Versus Semantic Regeneration

Axes should follow a two-level update model:

1. cheap live panel motion,
2. occasional semantic layout regeneration.

During continuous panning or camera motion, the existing axis-derived geometry should usually be
reused.

Semantic regeneration should happen only when one or more invariants fail, such as:

1. the visible domain approaches the edge of the covered domain,
2. tick spacing becomes too dense or too sparse,
3. label readability drops below threshold,
4. formatter or scale policy changes.

This means axis recomputation should be threshold-driven, not frame-driven.


## Covered Domain Policy For Axes

An axis should be able to cache a layout for a larger `covered_data_domain` than the current
`visible_data_domain`.

That cache allows:

1. smooth panning within the covered range,
2. fewer semantic rebuilds,
3. fewer uploads during interaction.

When the visible domain remains comfortably inside the covered domain:

1. only `PanelTransformDirty` should usually apply,
2. `AxisLayoutDirty` should remain clear.

When the visible domain approaches the coverage boundary or density thresholds fail:

1. `AxisLayoutDirty` should be raised,
2. derived axis resources should be rebuilt and uploaded.


## FramePlan Rebuild Policy

The scene layer should distinguish between:

1. `FramePlan` topology changes,
2. parameter or transform updates within a stable plan.

`FramePlan` should usually be rebuilt when:

1. render-node or compute-node structure changes,
2. target usage changes,
3. panel routing changes,
4. family variant selection changes planning shape,
5. picking or export paths are added or removed.

`FramePlan` should usually not be rebuilt when only:

1. current panel transforms changed,
2. small style values changed within the same visual variant,
3. resource subranges changed but plan topology stayed the same.


## Redraw Versus Rebuild

A redraw request should not automatically imply full rebuild of every derived artifact.

The intended chain is:

1. mutation happens,
2. the relevant invalidation scopes are marked,
3. the next frame build resolves only the necessary caches and uploads,
4. the scene redraws.

So:

1. redraw is a scheduling concept,
2. invalidation is a dependency concept,
3. upload is an execution consequence,
4. `FramePlan` rebuild is a planning consequence.


## Capability Changes

If runtime capabilities become known late or change across environments, the scene layer may need to
invalidate cached planning choices.

Examples:

1. one family variant becomes unsupported,
2. one picking path changes,
3. one volume mode falls back to another.

This should usually invalidate:

1. `VisualPropsDirty` logically,
2. `FramePlanDirty`,
3. possibly normalized derived resources when the fallback requires a different representation.


## Diagnostics

The invalidation system should be inspectable in diagnostics and tests.

Useful diagnostic questions include:

1. which resources are dirty,
2. which panels have only transform dirtiness,
3. which axes are layout-dirty,
4. whether the current `FramePlan` cache was reused or rebuilt,
5. which uploads were emitted and why.


## Recommended Implementation Direction

The scene spec should prefer:

1. explicit per-object dirty flags or version counters,
2. dependency-driven recomputation,
3. cached normalized resources reused across panels,
4. threshold-driven axis regeneration,
5. `FramePlan` reuse when only parameter values or panel transforms changed.

This is the simplest model that stays performant without pushing backend concerns upward.
