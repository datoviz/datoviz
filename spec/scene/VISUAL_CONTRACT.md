# Scene Visual Contract

This document defines the producer-side contract for a `Visual` in the future scene layer.

It does not define the final public C API.

Its purpose is to make `Visual` precise enough that:

1. scene planning can be deterministic,
2. `FramePlan` construction has stable inputs,
3. DRP2 pressure tests can be evaluated without backend leakage.


## Position

A `Visual` is a scene-owned high-level scientific renderable.

It sits:

1. below `Scene` and `Panel` ownership,
2. above `FramePlan`,
3. above DRP2 command emission,
4. outside backend and runtime internals.


## Terminology Baseline

The future scene spec should keep the broad `v0.3` idea of a visual while following the preferred
v0.4 family direction in `VISUAL_FAMILIES.md`.

The current preferred first-class family set is:

1. `basic`
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

Historical `v0.3` names such as `monoglyph`, `wiggle`, and `slice` remain informative background
vocabulary, but they should not be read here as the preferred v0.4 top-level taxonomy.


## Core Rule

A visual declares what work it needs.

A visual does not record backend commands, allocate backend objects directly, or own command-buffer
mechanics.


## Responsibilities

Every visual must be able to declare:

1. its stable logical identity,
2. its semantic family,
3. its resource schema,
4. its parameter schema,
5. its material or shader variant requirements,
6. its transform inputs,
7. its stage participation,
8. its picking behavior when relevant,
9. its capability-dependent fallback behavior.


## Non-Responsibilities

A visual must not:

1. expose Vulkan, Metal, WebGPU, or window-system handles,
2. emit DRP2 directly,
3. manage swapchain policy,
4. own backend pipeline caches,
5. own backend synchronization policy,
6. mutate unrelated scene objects during planning.


## Visual Identity

Each visual needs a stable logical identity within the owning scene.

That identity should support:

1. deterministic inclusion in `FramePlan` draw items,
2. picking and selection round-trips,
3. stable diagnostics,
4. resource sharing decisions,
5. incremental updates across frames.

Visual identity is a scene concept, not a backend object id.


## Visual Inputs

The first visual contract should model a visual as a pure function of:

1. visual-local properties,
2. referenced scene resources,
3. camera-derived state,
4. panel-derived state,
5. global capability information,
6. optional animation- or controller-derived state already resolved before planning.

By the time planning begins, these inputs should already be in a read-only state for the frame.


## Family Identity

Each visual must declare its semantic family.

The family vocabulary should follow `VISUAL_FAMILIES.md`.

The important separation is:

1. family identity captures scene-level semantic category,
2. variant identity captures a concrete rendering or capability-shaped path within that family.

Examples:

1. `marker` is a family, while a particular shape or edge-mode is a variant axis,
2. `image` is a family, while rgba versus colormap is a variant axis,
3. `path` is a family, while wiggle-like behavior may be a variant or subfamily.


## Visual Properties

Each visual should expose a logical property set that may include:

1. visibility and enable flags,
2. styling parameters such as color mode, line width, point size, or opacity,
3. transform parameters or transform references,
4. domain-specific display controls such as colormap range or glyph scaling,
5. picking enablement,
6. quality or precision mode when multiple variants are possible.

These are scene properties, not backend uniforms or bindings.


## Resource Requirements

Each visual must declare the logical resources it needs.

That declaration should cover:

1. required resource identities,
2. expected resource kinds such as vertex buffer, index buffer, uniform data, storage buffer, or
   sampled texture,
3. usage roles in the current frame,
4. whether the resource is optional or mandatory for the active variant,
5. whether the visual reads, writes, or both reads and writes the resource.

This declaration is used by planning and validation before DRP2 emission.

In `v0.3` terms, this is the conceptual successor to the recurring visual split between:

1. attribute-like streams,
2. optional indices,
3. parameter blocks,
4. texture inputs,
5. grouping metadata.


## Resource Schema

Each visual family should define a scene-level resource schema.

The schema should describe which kinds of logical resources may appear, and which are:

1. mandatory,
2. optional,
3. per-item,
4. per-group,
5. per-visual,
6. panel-local derived,
7. writable versus read-only.

Examples:

1. `point` typically needs an item table and may need a small style block,
2. `marker` typically needs an item table plus a marker-style block,
3. `path` typically needs grouped item data and path-style parameters,
4. `glyph` typically needs grouped glyph instances plus an atlas-like sampled field,
5. `mesh` typically needs geometry, optional indices, and material/light state,
6. `image` typically needs a sampled field plus placement/style parameters,
7. `volume` typically needs a volumetric sampled field plus transfer-related parameters.

The public API can choose any convenient construction surface later, but these logical schemas should
already be explicit at the spec layer.


## Parameter Schema

Each visual family should define a parameter schema distinct from bulk resource payloads.

This schema should describe:

1. family-level style controls,
2. mode selectors,
3. transform-related parameters when they are not purely scene-global,
4. picking-related controls,
5. quality or fallback selectors.

Examples:

1. point size behavior,
2. marker edge settings,
3. path join and cap settings,
4. glyph anchor or background controls,
5. image border, anchor, and colormap controls,
6. mesh material and contour controls,
7. volume transfer and traversal controls.


## Resource Ownership Boundary

The first scene slice should treat resources as scene-owned objects that visuals reference.

As a rule:

1. visuals may require resources,
2. visuals may interpret resource contents,
3. visuals should not privately own backend-visible resource allocations,
4. reusable data shared across visuals should live as explicit scene resources rather than hidden
   visual internals.


## Material And Shader Variant Identity

Each visual must expose a logical material or shader variant identity.

This identity should capture:

1. the rendering family the visual belongs to,
2. variant-affecting feature flags,
3. whether the visual participates in render, compute, or both,
4. whether picking requires a distinct variant,
5. capability-dependent fallback choices.

This identity is intentionally logical.
It must not assume the final DRP2 object-creation or runtime cache shape.


## Variant Axes

Each visual family should define its allowed variant axes explicitly.

A variant axis is a dimension along which the same family may take different planning or rendering
paths.

Examples:

1. `image`: rgba, colormap, fill, projected extraction mode
2. `mesh`: colored, textured, lit, contour-enabled
3. `path`: open, closed, stacked, wiggle-like
4. `glyph`: atlas or symbol mode, text layout mode
5. `volume`: direct, colormap, traversal mode

Variant axes should be:

1. scene-visible when they affect planning or capability fallback,
2. distinct from pure backend implementation details,
3. constrained enough that family boundaries remain meaningful.


## Transform Contract

Each visual must declare how its transform data is derived.

The first contract only needs the following categories:

1. no transform,
2. panel-camera transform,
3. visual-local transform,
4. panel-camera transform composed with a visual-local transform.

The scene layer is responsible for resolving those inputs into frame-ready state before or during
planning.


## Stage Participation

Each visual must declare which logical stages it participates in.

The first contract should support:

1. render-only visuals,
2. compute-then-render visuals,
3. picking-only auxiliary participation,
4. offscreen/export participation when different from onscreen participation.

This declaration must be strong enough for `FramePlan` to place the visual into the correct
`RenderNode` and `ComputeNode` instances.


## Draw Contribution

When a visual participates in a render stage, it must be able to contribute one or more logical draw
items.

Each draw contribution should determine:

1. which render node it can belong to,
2. which material or shader variant it requires,
3. which resources it binds logically,
4. which transform inputs it consumes,
5. which draw parameters it needs,
6. whether it is eligible for batching or coalescing with other items.

The contract should not require the visual itself to decide final low-level draw ordering.


## Compute Contribution

When a visual participates in compute, it must be able to declare:

1. the logical compute stage it belongs to,
2. its read resources,
3. its written resources,
4. the variant identity required for the compute stage,
5. the dispatch shape or an equivalent scene-resolved work description,
6. the resources or outputs consumed later by render or readback stages.


## Picking Contract

If a visual is pickable, it must declare:

1. whether picking is supported,
2. the logical payload identity space used for picking,
3. how picked payloads map back to scene-visible entities,
4. whether picking requires a dedicated pass or can reuse a render-stage contribution,
5. any capability or format requirements specific to picking.

A visual that is not pickable should say so explicitly rather than relying on absence by convention.


## Capability Adaptation

Visuals must support capability-aware planning.

At minimum, a visual should be able to express:

1. required capabilities without which it cannot be planned,
2. optional capabilities that select a higher-quality variant,
3. deterministic fallback behavior when optional capabilities are unavailable,
4. a scene-visible diagnostic when no valid fallback exists.

Examples:

1. fallback from FP64 to FP32,
2. fallback from compute-assisted to render-only,
3. fallback from multisampled to single-sampled rendering,
4. disabling a nonessential picking mode when the required format is unsupported.


## Dirty-State Interaction

Visuals should participate in incremental updates without forcing whole-scene rebuilds.

The visual contract should support:

1. detecting when only properties changed,
2. detecting when referenced resource contents changed,
3. distinguishing shader-variant changes from simple parameter changes,
4. allowing planning to rebuild only the affected draw, compute, or upload work.

Dirty tracking is a scene concern, but visuals must expose enough structure for it to be effective.


## Panel Relationship

A visual may appear in one or more panels.

The contract should therefore distinguish:

1. visual-global state shared across panels,
2. panel-local derived state such as camera-relative transforms,
3. panel-local enablement or visibility filters,
4. panel-local picking and target participation.

This prevents panel-specific planning from mutating the shared definition of the visual.


## Diagnostics

The visual contract should allow scene planning to report useful failures such as:

1. missing required resource,
2. incompatible resource kind for the selected variant,
3. unsupported capability for the requested quality mode,
4. missing picking payload mapping,
5. invalid transform configuration,
6. incompatible panel target requirements.

These diagnostics should mention the visual identity and logical variant identity, not backend objects.


## Relationship To FramePlan

The visual contract feeds `FramePlan` directly.

At minimum:

1. visual resource declarations feed plan resource references,
2. visual stage declarations feed node placement,
3. visual variant identity feeds logical material or shader selection,
4. visual draw contribution feeds `RenderNode` draw items,
5. visual compute contribution feeds `ComputeNode` work items,
6. visual picking declarations feed picking render and readback paths.


## Relationship To Future Public API

The future public scene API may expose concrete visual types such as points, lines, meshes, images,
volumes, or glyph-based visuals.

The current preferred direction from the scene spec is:

1. keep `basic`,
2. keep `pixel` distinct from `point`,
3. remove `monoglyph` as a first-class family,
4. treat `wiggle` as path-related unless later evidence promotes it,
5. treat `slice` as an `image`-family mode rather than a top-level family,
6. keep `sphere` as a first-class family with multiple possible rendering variants.

This document does not freeze final public naming, but it should be read as aligned with the current
preferred taxonomy above rather than as reopening older family options by default.

For now, the only stable requirement is that every future public visual type must satisfy the contract
defined here.


## Minimum Worked Cases A Visual Must Support

The first visual contract is acceptable only if it can describe:

1. one static plot visual in one panel,
2. one dynamic visual with dirty-range uploads across frames,
3. one pickable visual with payload-to-item mapping,
4. one offscreen-export visual,
5. one compute-assisted visual that produces data consumed by rendering.


## Deferred Questions

The following topics should remain open until DRP2 and runtime details are more stable:

1. the final public type hierarchy of visuals,
2. whether material identity becomes a first-class public scene object,
3. the exact shape of scene-owned descriptor or binding declarations,
4. how much batching policy is visible above planning,
5. whether some visuals may generate transient internal resources as part of planning.
