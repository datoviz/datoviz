# Scene MSAA Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track remaining multisample antialiasing work after panel-local MSAA and resolve
>   metadata landed.


## Current State

Durable contracts live in:

1. [`../../../spec/scene/implementation/TRANSPARENCY_MSAA.md`](../../../spec/scene/implementation/TRANSPARENCY_MSAA.md)
2. [`../../../spec/scene/implementation/GRAPH_TECHNIQUES.md`](../../../spec/scene/implementation/GRAPH_TECHNIQUES.md)

Use this file only for pickup order, validation, and remaining policy work. Do not duplicate the
MSAA graph, DRP2, vklite, or alpha-to-coverage contracts here.

The active lane has landed DRP2 multisample protocol fields, scene panel MSAA lowering, and named
MSAA resolve metadata in scene graph lowering. MSAA should remain a general render-target and
pipeline feature, not a sphere-only special case.


## Remaining MSAA Work

Recommended follow-up commits:

1. Harden sample-count serialization and semantic validation where fixture coverage is thin.
2. Keep vklite multisampled graph texture and resolve lowering covered by focused tests.
3. Add or confirm alpha-to-coverage capability in visual pipeline descriptors.
4. Enable alpha-to-coverage for opaque sphere impostors when panel MSAA is active.
5. Add a GLFW sphere comparison control for sample count and later alpha-to-coverage.
6. Decide when to expose a public `DvzMsaaDesc` or `dvz_panel_set_msaa()` API if it is not already
   stable enough.


## Validation

For docs-only changes, run:

```text
rg for old moved filenames and stale soon/spec links
git diff --check
git status --short
```

For implementation changes, use focused checks such as:

```text
cmake --build build --target dvztest_drp2 dvztest_scene -j 8
./build/testing/dvztest_drp2 test_drp2
./build/testing/dvztest_scene test_scene_visual_pass_capabilities
./build/testing/dvztest_scene test_scene_sphere_emit_glsl_executes
```

Before public API changes, run broader `just test drp2`, `just test scene`, and `just spec-check`
coverage.
