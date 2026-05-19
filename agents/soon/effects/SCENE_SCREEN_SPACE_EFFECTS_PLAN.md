# Scene Screen-Space Effects Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track the remaining outline, screen-space edge enhancement, and bloom execution
>   order after durable semantics and graph-technique rules were split into `spec/scene`.


## Current State

Durable screen-space effect contracts live in:

1. [`../../../spec/scene/semantics/EFFECTS.md`](../../../spec/scene/semantics/EFFECTS.md)
2. [`../../../spec/scene/proposals/active/SCREEN_SPACE_EFFECTS_DESIGN.md`](../../../spec/scene/proposals/active/SCREEN_SPACE_EFFECTS_DESIGN.md)
3. [`../../../spec/scene/implementation/GRAPH_TECHNIQUES.md`](../../../spec/scene/implementation/GRAPH_TECHNIQUES.md)
4. [`../../../spec/scene/implementation/OCCLUSION_EFFECTS.md`](../../../spec/scene/implementation/OCCLUSION_EFFECTS.md)

Use this file only for pickup sequencing, validation, and unresolved implementation choices. Do not
duplicate public effect semantics, composition ordering, graph pass/resource contracts, or G-buffer
rules here.

All screen-space effects should remain default-off, panel-local retained technique settings and
should use the normal scene -> FramePlan graph -> DRP2 -> vklite/canvas route.


## Remaining Screen-Space Work

Recommended follow-up commits:

1. Add retained internal panel technique state for outline, edge enhancement, and bloom descriptors
   without changing default FramePlan output.
2. Pick the first outline identity source: object IDs, selection masks, or a deliberately narrow
   selected/hovered-object mask.
3. Add outline source resources and a source pass for eligible opaque selected/hovered targets,
   with multi-panel scissor coverage.
4. Add outline composite after base scene composition and before external UI; keep width units and
   physical/logical pixel behavior explicit.
5. Add screen-space edge enhancement by reusing G-buffer depth/normal resources where available and
   falling back to depth-only edges only by explicit policy.
6. Add opt-in bloom over resolved panel color only after the export and quantitative-image
   inclusion policy is explicit.
7. Add public typed descriptor constructors and panel setters only after API naming review.


## Open Implementation Choices

Track these while implementing, and promote stable answers back to `spec/scene`:

1. outline target bitset shape for hover, selection, explicit visual flags, and annotations;
2. object-level versus item-level identity in the first outline slice;
3. hover/selection precedence when both are active;
4. transparent visual outline policy;
5. depth and normal threshold units for edge enhancement;
6. LDR thresholding versus HDR intermediate for the first bloom path;
7. export inclusion defaults for presentation effects.


## Tests And Examples

Focused tests should cover:

1. default-off FramePlan parity;
2. opt-in graph resource and pass creation;
3. graph dependency order;
4. DRP2 runtime lowering smoke coverage;
5. multi-panel scissor correctness;
6. offscreen image differences;
7. hover/selection outline state changes.

Useful examples:

1. mesh/sphere selection outline example;
2. dense surface edge-enhancement example with SSAO comparison;
3. astronomy or fluorescence-style bloom example.


## Validation

For docs-only changes, run:

```text
rg for old moved filenames and stale soon/spec links
git diff --check
git status --short
```

For implementation changes in this lane, use:

```text
just build
just test scene
just test drp2
```

Add offscreen image-difference coverage before treating a runtime effect as complete.
