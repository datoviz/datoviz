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
- `FRAME_LIFECYCLE.md`: update/build/emit flow
- `USE_CASES.md`: pressure-test scenarios
- `prototypes/`: non-authoritative API sketches
