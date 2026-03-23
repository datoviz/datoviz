# Scene Spec

This directory defines the future scene layer as a consumer of DRP2, not as a backend runtime.

The scene layer should remain pure high-level logic:

1. build and own user-facing visualization state,
2. derive rendering work from that state,
3. emit DRP2 through a runtime-facing contract,
4. stay independent from Vulkan, swapchain, and windowing internals.


## Status

- Status: planning only
- Implementation priority: after the DRP2 contract is frozen enough to avoid churn
- Primary constraint: do not let scene design leak backend details into its public API


## Documents

- `REQUIREMENTS.md`: what the scene layer needs from DRP2 and the runtime
- `OBJECT_MODEL.md`: minimum stable concepts
- `AXES.md`: scene-side semantic model for axes, ticks, labels, and related annotations
- `VISUAL_FAMILIES.md`: preferred v0.4 visual-family taxonomy grounded in local `v0.3` terminology
- `VISUAL_CONTRACT.md`: producer-side contract every future visual type must satisfy
- `VISUAL_MINI_CONTRACTS.md`: family-level mini-contracts for the current preferred v0.4 visuals
- `RESOURCE_MODEL.md`: scene-owned logical data model for visuals, planning, upload, and readback
- `TRANSFORM_PIPELINE.md`: explicit data-normalization and panel-transform pipeline for scene visuals
- `FRAME_PLAN_IR.md`: producer-side intermediate representation for one planned frame
- `FRAME_LIFECYCLE.md`: update/build/emit flow
- `RUNTIME_BOUNDARY.md`: allowed and forbidden dependencies on the runtime layer
- `USE_CASES.md`: pressure-test scenarios
- `examples/`: worked scene-spec examples that instantiate families, transforms, and frame plans
