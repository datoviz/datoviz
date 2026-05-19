# Scene MSAA Plan

> **Execution Status**
> - **Status:** `IMPLEMENTED THROUGH PANEL MSAA; SPHERE POLICY FOLLOW-UP OPEN`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track remaining multisample antialiasing work in the scene -> FramePlan -> DRP2 ->
>   vklite path without making it sphere-specific.


## Durable Contract

Use the MSAA implementation contract:
[../../../spec/scene/implementation/TRANSPARENCY_MSAA.md](../../../spec/scene/implementation/TRANSPARENCY_MSAA.md).

The generic graph-technique rules are in
[../../../spec/scene/implementation/GRAPH_TECHNIQUES.md](../../../spec/scene/implementation/GRAPH_TECHNIQUES.md).

This file tracks pickup order, validation, and remaining policy work.


## Current Baseline

The active lane has landed:

1. DRP2 multisample protocol fields;
2. scene panel MSAA lowering;
3. named MSAA resolve metadata in scene graph lowering.

MSAA should remain a general render-target and pipeline feature. It should not be implemented as a
sphere-only special case.


## Remaining Work

1. Harden sample-count serialization and semantic validation where fixture coverage is still thin.
2. Keep vklite multisampled graph texture and resolve lowering covered by focused tests.
3. Add or confirm alpha-to-coverage capability in visual pipeline descriptors.
4. Enable alpha-to-coverage for opaque sphere impostors when panel MSAA is active.
5. Add a GLFW sphere comparison example control for sample count and, later, alpha-to-coverage.
6. Decide when to expose a public `DvzMsaaDesc` / `dvz_panel_set_msaa()` API if it is not already
   stable enough.


## Implementation Order

Recommended remaining commits:

1. Add missing sample-count/resolve fixture coverage.
2. Add alpha-to-coverage capability in visual pipeline descriptors.
3. Enable alpha-to-coverage for opaque sphere impostors when panel MSAA is active.
4. Add or update the sphere comparison example control.
5. Broaden scene tests only after the narrow sphere and pipeline checks are stable.


## Validation

Focused validation:

```text
cmake --build build --target dvztest_drp2 dvztest_scene -j 8
./build/testing/dvztest_drp2 test_drp2
./build/testing/dvztest_scene test_scene_visual_pass_capabilities
./build/testing/dvztest_scene test_scene_sphere_emit_glsl_executes
./build/examples/c/hello_sphere_ssao_glfw 2
git diff --check
```

Broader validation before public API changes:

```text
just test drp2
just test scene
just spec-check
```
