> **Execution Status**
> - **Status:** `ACTIVE SELECTION / HIGHLIGHT DESIGN NOTE`
> - **Updated on:** `2026-05-09`
> - **Purpose:** define the intended v0.4 scene-level model for selection, hover, highlight, and
>   linked identity styling across visual families.

# Selection and Highlight Design

This note narrows older broad selection material into the active v0.4 contract that should align
with the current scene, picking, transparency, and annotation work.


## Objective

Support coherent scene-level interaction state for:

1. hover,
2. single and multi-selection,
3. linked highlighting across several visuals,
4. item-level and object-level emphasis,
5. selection-driven styling that remains compatible with transparency and text overlays.


## Existing Grounding In The Repo

Useful existing context:

1. broad future selection note:
   [spec/scene/interaction/SELECTION.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/interaction/SELECTION.md)
2. active picking note:
   [agents/now/PICKING_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/PICKING_DESIGN.md)
3. active transparency note:
   [agents/now/TRANSPARENCY_WBOIT_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/TRANSPARENCY_WBOIT_DESIGN.md)
4. active UI mutation note:
   [agents/now/UI_BACKEND_INTEGRATION.md](/home/cyrille/GIT/Viz/datoviz/agents/now/UI_BACKEND_INTEGRATION.md)

This note records the tighter active recommendation set.


## Core Recommendation

Selection and highlight should be scene-owned semantic state, not visual-private styling hacks.

Recommended split:

1. picking resolves logical identities,
2. scene-owned interaction state decides hover/selection membership,
3. visuals consume derived highlight state,
4. external UI and controllers mutate the same scene-owned selection objects.


## Identity Granularity

Selection must preserve the same logical granularity as picking.

Recommended rule:

1. object-level visuals may expose object-level selection only,
2. point, marker, segment, repeated primitive, and similar retained-item visuals expose item-level
   selection,
3. meshes expose face-level selection first when face-level picking is enabled,
4. images and slices may expose pixel or sample selection only when that identity is stable and
   useful.

Do not coarsen a precise pick result into object-only selection unless the caller requests that.


## Hover Versus Selection

Hover and persistent selection should be separate state channels.

Recommended model:

1. one transient hover target or hover hit list,
2. one persistent selection set,
3. separate styling policy for hover and selection,
4. explicit precedence when an item is both hovered and selected.

Recommended precedence:

1. selected + hovered
2. selected
3. hovered
4. unselected


## Linked Selection

Linked highlighting across visuals is a first-class requirement.

Recommended direction:

1. one logical selection object may be attached to several visuals,
2. each visual resolves the shared logical identity into its own item domain,
3. one user action can therefore highlight a point cloud, a mesh region, and a label together when
   they reference the same semantic entity.

This is important for scientific exploration workflows and for external UI inspectors.


## Selection Shape

The active contract should allow more than one selected identity.

Required baseline:

1. empty selection,
2. one selected identity,
3. multi-selection set,
4. replace, additive, subtractive, and toggle update modes.

Lasso and box selection can remain follow-on interaction modes, but the data model should already
support multi-selection.


## Highlight Styling Model

Highlight should be described semantically and applied per family through each visual’s rendering
path.

Recommended style dimensions to reserve now:

1. alpha emphasis or de-emphasis,
2. color override or tint,
3. size or line-width emphasis where meaningful,
4. edge or outline emphasis for meshes and regions,
5. label or annotation attachment later.

Do not freeze one universal GPU-mask implementation detail into the scene-level API.


## Transparency Interaction

Selection highlighting must remain compatible with transparent visuals.

Recommended rule:

1. transparent visuals still participate in hover and selection,
2. highlight semantics are resolved before transparent compositing,
3. selection state does not bypass the transparency mode or force a separate visual family,
4. if a highlight effect is unavailable for one family or mode, that limitation must be explicit.


## External UI Interaction

External UI should be able to inspect and mutate selection state directly.

Recommended behavior:

1. scene exposes stable getters/setters for selection objects,
2. property panels can read current hover or selection payloads,
3. list/tree widgets can drive programmatic selection changes,
4. scene invalidation flows through the normal retained update path.


## Readout And Annotation Relationship

Selection is related to, but distinct from, probe/readout semantics.

Recommended split:

1. selection records which identities are active,
2. readout/probe systems derive values, coordinates, or labels from those identities,
3. annotations may mirror selection state, but annotation objects do not own selection state.


## Capability And Failure Model

Selection should degrade only through explicit capability or family limits.

Recommended rules:

1. a visual declares whether it supports scene-driven highlight styling,
2. item-level selection is invalid if the visual cannot maintain stable item identity,
3. unsupported highlight effects should become explicit diagnostics or documented family limits,
4. the selection object itself should remain backend-agnostic and valid even if one attached visual
   has narrower styling support.


## Initial Public API Direction

The exact names can still move, but the conceptual API should support:

1. scene-owned selection object creation,
2. attaching a selection object to one or more visuals,
3. programmatic set/clear/toggle operations on logical identities,
4. hover-state routing from picking results,
5. family-level highlight style configuration.


## Immediate Scope Recommendation

The narrowest useful active implementation target is:

1. scene-owned hover state,
2. scene-owned multi-selection set,
3. item-level selection for point and repeated primitive families,
4. face-level selection for meshes,
5. visual-driven highlight styling through existing retained update paths.


## Explicit Non-Goals For The First Slice

1. freezing lasso/box implementation details now,
2. multi-layer named selection stacks,
3. family-private ad hoc highlight systems,
4. backend-native selection handles in the scene API.
