# Scene Invalidation And Caching

Status: normative v0.4 scene pipeline spec.

This document defines logical dirty scopes and cache layers for scene state. It keeps authored scene
state, normalized resources, panel-local state, axis layout, `FramePlan`, and DRP2 emission from
being rebuilt as one monolithic artifact.


## Purpose

The scene layer should:

1. invalidate the smallest correct scope;
2. keep panel interaction responsive without unnecessary reupload;
3. reuse normalized resources across panels;
4. support grouped-resource and axis incremental updates;
5. distinguish redraw scheduling from dependency invalidation.


## Core Rule

Data-space changes invalidate normalization-derived resources; panel-navigation changes usually
invalidate only panel-local transforms. `FramePlan` is rebuilt only when plan topology, routing,
stage participation, or target usage changes.


## Dirty Scopes

| Scope | Meaning | Typical consequences |
|---|---|---|
| `SceneStructureDirty` | object set or panel membership changed | revalidate, rebuild plan, maybe materialize runtime objects |
| `VisualPropsDirty` | semantic visual property changed | style update, variant reselection, draw contribution refresh |
| `ResourceDataDirty` | source/derived content changed | upload, possible normalization, possible count/topology update |
| `ResourceLifecycleDirty` | resource created, recreated incompatibly, or retired | plan resource declaration or retirement emission |
| `NormalizationDirty` | Stage A data-to-visual output is stale | recompute/upload visual-space resources |
| `PanelTransformDirty` | panzoom, camera, viewport, or framing changed | recompute panel transforms and redraw |
| `AxisLayoutDirty` | tick/label coverage or density is stale | regenerate/upload affected axis resources |
| `CapabilityDirty` | capability/adaptation result changed | variant reselection, diagnostics, possible plan/routing rebuild |
| `FramePlanDirty` | cached plan shape is invalid | rebuild node topology, dependencies, target usage |
| `UploadDirty` | dirty resources need execution-facing writes | emit upload work; not a user mutation itself |
| `ReadbackRoutingDirty` | result-to-scene mapping changed | refresh picking/readback payload routing |

These are logical scopes, not required public enum names.


## Cache Layers

| Cache | Contents | Authoritative? | Invalidated by |
|---|---|---|---|
| source data | item/grouped tables, sampled fields, parameter blocks | yes | explicit writes or metadata edits |
| normalized resources | `VisualSpace` coordinates, geometry, family-ready payloads | derived | `NormalizationDirty` |
| panel derived state | panzoom matrices, view/projection, viewport parameters | derived | `PanelTransformDirty` |
| axis layout | visible/covered domains, tick step, labels, tick geometry | derived | `AxisLayoutDirty` |
| `FramePlan` | node topology, dependency order, draw grouping, routing | derived | `FramePlanDirty` |

Capability changes that do not alter topology should not invalidate more than necessary; adaptation
changes that affect stage participation, target usage, or routing do invalidate the plan cache.


## Change Matrix

| Change | Usually invalidates | May invalidate | Should not usually invalidate |
|---|---|---|---|
| source data write | resource, normalization, upload | plan if counts/topology/stages change | panel transform state |
| resource retirement | resource lifecycle, plan | binding/readback routing | unrelated live resource contents |
| style or parameter change | visual props, small upload | plan if variant/stages/targets change | source data |
| panel pan/zoom | `PanelTransformDirty` | `AxisLayoutDirty` | source uploads, normalization |
| 3D camera move | `PanelTransformDirty` | view-dependent `FramePlanDirty`, 3D axis layout | normalized resources |
| visual add/remove | `SceneStructureDirty`, `FramePlanDirty` | `ReadbackRoutingDirty` | unrelated resources |
| picking toggle | visual props, plan, readback routing | new picking target/upload work | authored data |
| capability/adaptation change | capability | visual props, plan, routing, fallback-derived resources | authored source data |
| offscreen export request | plan | readback routing | normalization unless export needs distinct representation |


## Incremental Resource Updates

| Resource class | Incremental unit |
|---|---|
| `ItemTable` | item row, contiguous row range, or full replacement |
| `GroupedItemTable` | item range and structural span; whole-span relayout is a valid dirty unit |
| texture/sampled field | subrectangle/subvolume or full replacement |
| parameter block | field/block update |

The model should not require whole-resource upload for small subrange changes when the runtime has a
better path. Grouped resources must preserve span identity for path/glyph semantics, picking, and
selection even when several spans are batched into one upload/draw.


## Axes

Axes use two update levels:

1. cheap live movement through current panel transforms;
2. occasional semantic tick/label regeneration.

An axis may cache a `covered_data_domain` larger than the current `visible_data_domain`. If the
visible domain remains inside coverage and tick density/readability invariants still hold, only
`PanelTransformDirty` should apply. When coverage, density, scale, formatter, or layout invariants
fail, raise `AxisLayoutDirty` and upload only affected derived resources. See
[`../semantics/AXES.md`](../semantics/AXES.md).


## FramePlan Rebuild Policy

Rebuild `FramePlan` when render/compute node structure, dependency ordering, target usage, panel
routing, family variant plan shape, picking paths, export paths, or readback routing changes.

Do not rebuild it solely for current panel transforms, resource subrange uploads, or style values
that stay within the same variant and node topology.


## Resource Lifecycle Propagation

Resource creation, incompatible recreation, and retirement are structural changes even when no visible draw changes. A retired resource remains pending until a successfully emitted plan records its destruction; failed planning, validation, or conversion must not clear that pending state.

Frame-local absence is not retirement. Hidden panels, disabled visual contribution, capability fallback, and temporary nonparticipation may omit resource use from one frame while the semantic resource remains live. Retirement must originate from explicit owner lifecycle state.

Content revision, descriptor revision, logical extent revision, and lifecycle revision are distinct concepts even if one implementation stores them compactly. Capacity-only changes must not masquerade as logical count changes, while logical extent changes must invalidate every draw, query, picking, and export count derived from that resource.


## Redraw Versus Rebuild

Redraw is scheduling. Invalidation is dependency state. Upload is an execution consequence.
`FramePlan` rebuild is a planning consequence. A redraw request must resolve only the dirty caches
needed for the next frame.


## Diagnostics

Diagnostics and tests should expose:

1. dirty resources and ranges;
2. panels with only transform dirtiness;
3. axes with layout dirtiness and why;
4. whether the `FramePlan` cache was reused or rebuilt;
5. uploads emitted and their source dirty scopes.
6. resources created, recreated, retired, and awaiting committed destruction.


## Implementation Direction

Prefer explicit per-object dirty flags or version counters, dependency-driven recomputation, cached
normalized resources shared across panels, threshold-driven axis regeneration, and `FramePlan` reuse
for stable topology.
