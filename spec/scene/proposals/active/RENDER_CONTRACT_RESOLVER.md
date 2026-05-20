> **Execution Status**
> - **Status:** `ACTIVE IMPLEMENTATION PLAN`
> - **Updated on:** `2026-05-20`
> - **Purpose:** track the active scene render-contract resolver refactor that prevents
>   transparency, occlusion, depth, and DRP2 lowering regressions.

# Scene Render Contract Resolver

## Decision Addressed

Scene visual requirements should resolve into passive draw and pass contracts before FramePlan
graph construction and DRP2 lowering.

The active question is how much remaining alpha, depth, blend, attachment, and bind-group policy
can move into that resolver before lower layers stop re-inferring the same facts.


## Short Summary

The repository already has an initial internal contract layer:

1. `src/scene/render_contract.h` and `src/scene/render_contract.c` define passive draw,
   attachment, and pass contracts;
2. `src/scene/scene.c` validates FramePlan contracts after emission;
3. `src/scene/scene_emit.c` queries draw contracts for source-over transparent depth attachment
   needs;
4. `src/scene/technique.c` emits graph-backed WBOIT, depth peeling, source-over blending, volume
   occlusion, scene occlusion, G-buffer, EDL, SSAO, and MSAA paths;
5. `src/scene/frame_plan_runtime.c` matches repeated graph passes by `work_label` ordinal;
6. `src/scene/tests/scene_graph.c` contains the first contract-focused technique fixtures.

New transparency, occlusion, depth, and screen-space-effect work should extend this contract path
instead of adding another inference lane.


## Chosen Direction

| Topic | Direction |
|---|---|
| Architecture | `retained visuals -> draw contracts -> named techniques -> pass contracts -> FramePlan -> DRP2 -> runtime`. |
| Authority | Lowering may assert resolved facts and fill mechanical DRP2 details; it must not silently choose different alpha, depth, blend, attachment, or bind-group policy. |
| Source-over alpha | `DVZ_ALPHA_BLENDED` means ordinary source-over blending; it may depth-test when requested but does not write normal depth. |
| Transparent depth writes | If transparent geometry must create depth, use an explicit prepass, occlusion pass, WBOIT, depth peeling, or another named technique. |
| Technique style | Keep named built-in techniques, but make their reads, writes, formats, samples, and draw requirements visible as generic contracts. |
| Capability fallback | Adaptation must be explicit and diagnostic; missing WBOIT/depth-peeling/sampling/blend/attachment support must not silently change semantics. |


## Required Invariants

1. A pass attachment set is the union of its draw and technique requirements.
2. Every emitted pipeline matches the active pass color attachments, formats, depth presence,
   depth format, and sample count.
3. A depth-testing draw is emitted only into a pass with a depth attachment.
4. A sampled resource has a producer-before-consumer graph edge or is explicitly borrowed.
5. Occlusion-sampling draws get the required bind layout and bind group.
6. Technique ordering is derived from resource dependencies except where an algorithm requires a
   fixed named order.
7. Contract validation runs before runtime execution for scene emission tests.


## Canonical Migration Links

Stable rules should move into:

1. [Frame Plan](../../pipeline/FRAME_PLAN.md) for graph/pass lifecycle and validation;
2. [Frame Lifecycle](../../pipeline/FRAME_LIFECYCLE.md) for when validation, emission, and runtime
   execution occur;
3. [Visual Contract](../../semantics/VISUAL_CONTRACT.md) for visual draw requirements;
4. [Transparency](../../semantics/TRANSPARENCY.md) for alpha-mode semantics;
5. [Effects](../../semantics/EFFECTS.md) and
   [Occlusion Effects](../../implementation/OCCLUSION_EFFECTS.md) for named effect behavior;
6. [Transparency And MSAA](../../implementation/TRANSPARENCY_MSAA.md) for implementation-facing
   WBOIT/MSAA details.

This proposal remains the active implementation checklist until duplicated lower-layer inference is
removed.


## Remaining Work

1. Add a resolver-matrix helper so mixed visual cases assert contracts without repeated graph
   scans.
2. Add explicit blend and pipeline policy fields to `DvzSceneDrawContract`.
3. Validate emitted DRP2 pipeline depth/blend state against the resolved contract.
4. Move WBOIT and depth-peeling attachment/blend details into named pass-contract builders.
5. Add offscreen readback tests for dense blended points, volume plus source-over mesh, volume plus
   WBOIT mesh, and volume plus depth-peel mesh.
6. Delete remaining duplicated lower-layer decisions once the contract owns the facts.
7. Decide whether draw contracts are stored in FramePlan metadata or only used during emission.
8. Decide how much fallback belongs in this resolver versus a separate capability-adaptation pass.


## Acceptance Criteria

The refactor is done when adding a visual or technique requires declaring draw/pass requirements in
one place, lower layers only assert resolved render facts, synthetic fixtures catch pass/pipeline
mismatches before runtime execution, and readback tests catch occlusion/transparency ordering
regressions.
