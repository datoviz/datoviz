# Image Picking Recovery Plan

> **Execution Status**
> - **Status:** `ACTIVE PLAN`
> - **Updated on:** `2026-05-15`
> - **Purpose:** rebuild image picking/probing step by step after the segmentation-label hover
>   regression exposed weak GPU-readback coverage.


## Problem

The current image probe path is not sufficiently proven. A focused test suite passed while the live
`examples/napari/large_labels_segmentation_glfw` segmentation-label hover path broke after removing
the CPU-side result overwrite. That means the tests did not cover the real coordinate/resource stack
used by the example.

The important lesson is that executing a GPU readback is not enough. The tests must prove that the
resolved label/color comes from the GPU bytes under the same conditions as the app path: hidden
pick-capable image visuals, non-fullscreen image quads, panel-local coordinates, retained request
execution, panzoom transforms, and `DVZ_PANZOOM_FLAGS_KEEP_ASPECT`.


## Ground Rules

1. Keep the demo usable while rebuilding the slice. If needed, temporarily restore the previous CPU
   fallback or revert the broken change before proceeding.
2. Do not remove fallback behavior until GPU-only coverage proves the runtime path under realistic
   scene/app conditions.
3. Treat image picking/probing as a retained request-executor feature, not as ad hoc per-hover scene
   reconstruction.
4. Keep each step independently testable and commit only after the narrow validation loop passes.


## Step-by-Step Plan

1. Stabilize the baseline.
   - Restore `large_labels_segmentation_glfw` hover labels to working behavior.
   - Record whether that baseline uses CPU fallback, GPU readback, or both.
   - Add a temporary trace/log hook if needed to show readback bytes and resolved result bytes.

2. Add a GPU-only 1x1 readback test.
   - Render a known RGBA texture into a 1x1 request target.
   - Assert exact downloaded bytes.
   - Make the test impossible to pass via retained CPU texture data.
   - Validate both hit and transparent-miss behavior.

3. Prove coordinate mapping without panzoom.
   - Cover a fullscreen image quad.
   - Cover a non-fullscreen image quad matching the napari demo, currently `[-0.92, +0.92]`.
   - Cover all four quadrants and at least one transparent/outside pixel.
   - Assert that the GPU bytes, not CPU recomputation, determine the result.

4. Prove controller transforms.
   - Add panzoom tests for zoom-in, pan, and combined pan+zoom.
   - Add a `DVZ_PANZOOM_FLAGS_KEEP_ASPECT` case matching the GLFW demo.
   - Verify panel-local pointer coordinates are transformed through the same MVP path used by
     normal scene rendering.

5. Prove hidden segmentation picking.
   - Use a visible image visual plus a hidden pick-capable image visual.
   - Assert `DVZ_SCENE_TARGET_SEGMENT` probes select the hidden pick-capable visual.
   - Assert decoded label ids for normal, zoomed, and panned views.
   - Include a zero-label/transparent miss case.

6. Move the live example onto the proven path.
   - Keep the hidden label texture orientation aligned with the displayed image.
   - Avoid CPU mouse-position label computation in the example.
   - Add a bounded smoke mode or test helper that queues deterministic probe requests without manual
     cursor movement.

7. Remove or demote the CPU fallback.
   - Only after the GPU-only tests cover the app-equivalent path.
   - If a fallback remains, mark it explicitly as a diagnostic or compatibility path and add tests
     that prove the normal path does not use it.


## Validation Targets

Use the narrowest useful loop at each step:

1. `cmake --build build --target dvztest_scene large_labels_segmentation_glfw -j2`
2. `just test scene_image_probe`
3. `just test scene`
4. `./build/examples/napari/large_labels_segmentation_glfw 2`
5. `git diff --check`

For changes touching request executors, Vulkan resources, readbacks, command buffers, or retained
runtime lifetime, also run `just build` and consider `clang-tidy -p build` on touched files.


## Exit Criteria

Image picking/probing is considered repaired when:

1. The live segmentation example reports correct labels at identity, zoomed, panned, and
   pan+zoomed views.
2. The same behavior is covered by automated scene tests.
3. The test path cannot pass by CPU-side recomputation of mouse position or texture coordinates.
4. Repeated hover requests reuse the retained request executor and static image-probe resources.
5. A transparent/zero-label pixel produces a miss instead of a stale label.
