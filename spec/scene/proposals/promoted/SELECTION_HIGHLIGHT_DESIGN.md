> **Execution Status**
> - **Status:** `SCENE SPEC PROPOSAL`
> - **Updated on:** `2026-05-20`
> - **Purpose:** preserve remaining scene-level selection, hover, highlight, and linked-identity
>   decisions after the canonical interaction specs absorbed the durable rules.

# Selection and Highlight Design

## Decision Addressed

Selection and highlight are scene-owned semantic state, not visual-private styling hacks.

The remaining proposal-stage question is how rich the first linked-identity and highlight payloads
should be while preserving family-specific rendering freedom.


## Short Summary

Picking reports scene identities. Interaction policy resolves those identities into hover or
selection targets. Scene-owned hover/selection state then drives per-family highlight styling and
optional linked behavior across visuals.

The selection model must support object-level, item-level, face-level, instance-aware, and future
sample/region identities without forcing every visual to expose every granularity.


## Chosen Direction

| Topic | Direction |
|---|---|
| Ownership | Scene owns hover state, selection sets, link keys, and highlight policy. |
| Hover | Keep raw pick hit and resolved hover target so readouts can stay precise while highlight policy may be coarser. |
| Selection | Store the resolved target, not the pre-policy raw hit. |
| Channels | Keep hover and persistent selection as separate state channels with explicit precedence. |
| Multi-selection | Support empty, single, and multi-selection with replace/add/subtract/toggle update modes. |
| Granularity | Preserve the granularity exposed by each visual: object, instance, item, group, face, pixel/sample, or region. |
| Linking | Scene-owned link keys map semantic entities across visual-local identity domains. |
| Highlight | Describe highlight semantically; each visual family chooses the supported rendering path. |
| Transparency | Highlight is resolved before compositing and must not bypass the visual's alpha/render mode. |


## First-Slice Target

1. Scene-owned hover state.
2. Scene-owned multi-selection set.
3. Item-level selection for point and repeated primitive families.
4. Object-level and face-level selection for meshes.
5. Separate hover and persistent-selection channels.
6. Visual-driven highlight styling through retained update paths.
7. Programmatic getters/setters for external UI.


## Canonical Migration Links

The authoritative rules now live in:

1. [Selection And Linked Highlight](../../interaction/SELECTION.md) for scene-owned selection,
   linked highlighting, input modes, lasso/box direction, and readback;
2. [Picking](../../interaction/PICKING.md) for pick request identity, freshness, and payload
   interpretation;
3. [Controllers And Interaction](../../interaction/CONTROLLERS.md) for routing and controller
   mutation flow;
4. [Visual Contract](../../semantics/VISUAL_CONTRACT.md) for visual identity and style
   participation;
5. [Transparency](../../semantics/TRANSPARENCY.md) for alpha/compositing compatibility;
6. [Annotations](../../semantics/ANNOTATIONS.md) for selection-driven label/readout consumers.

Do not duplicate pick freshness, controller routing, or GPU highlight implementation details here.


## Remaining Unresolved Points

1. Final public C names and payload structs for selection objects, hover state, link channels, and
   highlight descriptors.
2. Whether first-slice link channels are named strings, typed handles, or both.
3. Whether a local identity may have only one link key per channel initially, as proposed.
4. How active link-channel provenance is stored when selection was produced by linked behavior.
5. Exact mesh object/instance/face selection payload and highlight rendering.
6. Image/slice persistent sample-selection policy and diagnostics for unstable sample identity.
7. Family capability reporting when a requested highlight effect is unsupported.
8. Box/lasso implementation details, which should remain outside the first scene-owned state
   contract.
