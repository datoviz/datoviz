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
3. grouped primitive families such as strip-based lines or triangles may expose both parent-group
   selection and sub-primitive selection when both identities are meaningful,
4. meshes should support both object-level selection and face-level selection,
5. images and slices should support both probe-only interaction and optional persistent
   pixel/sample selection through explicit API policy,
6. images and slices may expose pixel or sample selection only when that identity is stable and
   useful.

Do not coarsen a precise pick result into object-only selection unless the caller requests that.
Likewise, do not forbid object-level selection when a family also supports a finer face/item mode.


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
3. linked selection may therefore map one semantic entity onto object-level, face-level, or
   item-level manifestations in different visuals,
4. one user action can therefore highlight a point cloud, a mesh region, and a label together when
   they reference the same semantic entity.

This is important for scientific exploration workflows and for external UI inspectors.


## Scene-Owned Link Keys

Linked selection needs more than ad hoc app-side glue.

Recommended model:

1. the scene owns stable semantic link keys,
2. a visual may provide a per-object, per-face, per-item, or per-sample mapping to those keys,
3. selection and hover operate on link keys when linked behavior is requested,
4. each attached visual resolves the selected link key back into its own local identity domain.

This means one semantic entity can be represented simultaneously as:

1. one point item,
2. one mesh face set or one mesh object,
3. one label or annotation,
4. one image region or slice sample later.

Recommended properties of the link-key model:

1. keys are scene-owned and backend-agnostic,
2. keys are stable across retained updates unless the semantic object is explicitly replaced,
3. in the first slice, each local identity may have zero or one link key per active channel,
4. many local identities across one or more visuals may share the same link key,
5. this supports one-to-many, many-to-one, and many-to-many linked selection through shared keys
   without requiring one local identity to belong to several keys at once,
6. applications may author the key tables directly when the data model is domain-specific.

Recommended future extension:

1. if several semantic grouping dimensions are needed, add multiple named link channels,
2. each channel still keeps the simpler rule of zero or one link key per local identity,
3. do not start with an unstructured many-keys-per-item model.

Recommended first-slice interaction rule:

1. linked selection operates on one active link channel at a time,
2. applications may switch the active channel through explicit policy or UI,
3. do not combine several active link channels in one linked-selection action initially.

This should be a first-class scene concept because otherwise linked selection becomes duplicated
application policy instead of shared visualization semantics.


## Selection Shape

The active contract should allow more than one selected identity.

Required baseline:

1. empty selection,
2. one selected identity,
3. multi-selection set,
4. replace, additive, subtractive, and toggle update modes.

Lasso and box selection can remain follow-on interaction modes, but the data model should already
support multi-selection.


## Selection Resolution Rule

Selection state should store the post-policy resolved identity, not the raw low-level hit.

Recommended rule:

1. picking may first report the most specific hit identity available,
2. interaction policy then resolves that hit into the intended selection target,
3. the selection object stores that resolved target as authoritative state,
4. downstream highlight, linked selection, and UI inspection operate on the resolved selection
   target rather than the pre-policy hit.

Example:

1. a mesh face is hit,
2. object-selection mode is active,
3. the interaction resolves the face hit to the mesh object,
4. the selection stores the mesh object identity rather than the face identity.

If face-selection mode is active instead, the same hit stores the face identity.


## Selection Policy And Input Mapping

Selection semantics should be configurable at the API level.

Recommended direction:

1. applications should be able to map input gestures and modifiers to selection actions,
2. that mapping should also be able to choose the active identity granularity when a visual exposes
   more than one meaningful mode,
3. mesh interactions should be able to switch between object-level and face-level selection,
4. image and slice interactions should be able to choose probe-only behavior or persistent
   pixel/sample selection.


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
4. both object-level and face-level selection for meshes,
5. separate hover and persistent-selection channels,
6. visual-driven highlight styling through existing retained update paths.


## Explicit Non-Goals For The First Slice

1. freezing lasso/box implementation details now,
2. multi-layer named selection stacks,
3. family-private ad hoc highlight systems,
4. backend-native selection handles in the scene API.
