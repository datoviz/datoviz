# Scene Visual Contract

Status: normative v0.4 scene semantics spec.

This document defines the producer-side contract for `DvzVisual*`. It does not define the final
public C API or backend implementation.


## Position

A visual is a scene-owned high-level scientific renderable. It sits below `Scene`/`Panel`, above
`FramePlan` and DRP2 emission, and outside backend/runtime internals.


## Core Rules

1. A visual declares work; it does not allocate backend objects, record commands, own swapchains,
   manage synchronization, or emit DRP2 directly.
2. `DvzVisual*` is the single opaque handle type. Family identity is fixed by the typed constructor
   (`dvz_point`, `dvz_marker`, `dvz_mesh`, etc.).
3. Scene planning owns batching, draw merging, transient resources, capability fallback, and dirty
   propagation.
4. Data preparation details are implementation concerns; the scene contract is the logical resources,
   parameters, transforms, stages, and identity.


## Family Baseline

The preferred first-class families are `primitive`, `pixel`, `point`, `marker`, `segment`, `path`,
`glyph`, `image`, `mesh`, `sphere`, and `volume`. Historical `v0.3` names such as `monoglyph`,
`wiggle`, and `slice` are background vocabulary, not the preferred top-level taxonomy. Family
direction is canonical in [VISUAL_FAMILIES.md](VISUAL_FAMILIES.md).


## Visual Identity And Inputs

Every visual has stable scene-level identity for deterministic `FramePlan` inclusion, picking,
selection, diagnostics, resource sharing, and incremental updates. Backend object ids are separate.

A visual is planned as a read-only function of visual-local properties, referenced resources,
panel/camera state, capabilities, and resolved controller/animation state.


## Required Declarations

| Declaration | Required content |
|---|---|
| family identity | semantic family; variant identity is separate |
| resource schema | required/optional resources, roles, read/write access, panel-local derived resources |
| parameter schema | style, mode, transform, picking, quality/fallback controls |
| transform contract | none, panel-camera, visual-local, or composition |
| stage participation | render, compute-then-render, picking, export/offscreen variants |
| draw contribution | render node compatibility, variant, logical binds, transforms, draw params, batching eligibility |
| compute contribution | stage, reads/writes, variant, dispatch/work shape, downstream outputs |
| picking contract | support, payload identity, scene mapping, pass requirements, capability needs |
| capability adaptation | required/optional caps, deterministic fallback, diagnostics |


## Public Type And Resource Model

Visual users see item counts, group assignments, family-valid operations, and opaque handles. They do
not see subtype structs, backend buffers, draw-call batching, or transient resources. Empty visuals
are valid and render nothing; item and index counts may change through data writes. Optional
preallocation is a performance hint, not a construction requirement.

Resources are scene-owned and referenced by visuals. Reusable data shared across visuals should be
explicit scene resources rather than hidden backend-visible visual internals. Resource contracts are
canonical in [`../pipeline/RESOURCE_MODEL.md`](../pipeline/RESOURCE_MODEL.md); per-attribute source
rules are in [`../pipeline/ATTRIBUTE_SOURCES.md`](../pipeline/ATTRIBUTE_SOURCES.md).


## Resource And Parameter Schemas

| Family | Typical resource schema |
|---|---|
| `point`, `pixel`, `marker`, `segment` | `ItemTable` plus parameter block |
| `path` | `GroupedItemTable` plus path-style parameters |
| `glyph` | grouped glyph instances plus font/atlas sampled field |
| `mesh` | `IndexedGeometry`, optional indices/sampled fields, material-like parameter block |
| `image` | `SampledField` plus placement/style/colormap parameters |
| `volume` | 3D `SampledField` plus transfer/traversal parameters |

Parameter schemas are distinct from bulk payloads. Datoviz does not expose `DvzMaterial` as a
first-class API concept; lit mesh/sphere shading values are visual parameter blocks.


## Variant Identity

Each visual exposes a logical shader/rendering variant identity containing family, feature flags,
stage participation, picking variant needs, and capability fallback choices. Variant axes are
scene-visible only when they affect planning or fallback.

Examples:

| Family | Variant axes |
|---|---|
| `image` | rgba, colormap, fill, projected extraction |
| `mesh` | colored, textured, lit, contour-enabled |
| `path` | open, closed, stacked, wiggle-like |
| `glyph` | atlas/symbol mode, text layout mode |
| `volume` | direct, colormap, traversal/slice mode |


## DRP2 Binding Convention

Built-in visuals lowered to DRP2 use:

| Set/binding | Meaning |
|---|---|
| set `0` | scene/panel common data |
| set `0`, binding `0` | MVP uniform |
| set `0`, binding `1` | viewport uniform |
| set `1+` | visual-specific resources |

Visual-specific resources must not occupy the common group. Built-in visuals that consume MVP or
viewport data must bind the common group.


## Capability And Transparency

Visuals must distinguish requested semantics from realization quality. Fallback may lower quality
or disable optional modes, but it should preserve declared meaning when possible and report
diagnostics when no valid fallback exists. Transparency-sensitive workflows must distinguish opacity
or emphasis semantics from capability-gated transparency quality.


## Dirty State And Panels

Visuals expose enough structure for the scene to distinguish property changes, resource-content
changes, shader-variant changes, and simple parameter updates. A visual may appear in multiple
panels; shared visual state must be separate from panel-local transform, visibility, target, and
picking participation. Dirty-scope policy is canonical in
[`../pipeline/INVALIDATION_AND_CACHING.md`](../pipeline/INVALIDATION_AND_CACHING.md).


## Diagnostics And FramePlan

Diagnostics should mention visual identity and logical variant identity for missing resources,
wrong resource kind, unsupported capability, missing picking mapping, invalid transforms, or
incompatible panel targets.

`FramePlan` consumes visual resource declarations, stage declarations, variant identity, draw
contributions, compute contributions, and picking declarations. It should not rediscover visual
semantics from backend state.


## Minimum Worked Cases

The contract is acceptable only if it describes: a static plot visual, a dynamic dirty-range visual,
a pickable visual with payload-to-item mapping, an offscreen-export visual, and a compute-assisted
visual whose output feeds rendering.
