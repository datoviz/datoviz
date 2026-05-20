# Scene Pick/Probe Payload Refactor

> **Status:** `PAYLOAD WIDENING SLICE COMPLETE`
> - **Completed on:** `2026-05-20`
> - **Scope:** widen pick/probe result metadata without changing the active DRP2 request path


## What Changed

This slice kept the existing scene -> DRP2 -> vklite request executor and made the public
pick/probe payloads more explicit.

Implemented pieces:

1. public `DvzSceneVisualFamily` values for resolved visual-family identity,
2. public `DvzProbeStatus` values matching the existing pick status shape,
3. widened `DvzPickResult` metadata for visual family, item/group/auxiliary ids, and link key,
4. widened `DvzProbeResult` metadata for status, visual family, item/group/auxiliary ids, and
   panel position,
5. internal request payload helpers for point-like pick and image/segment probe RGBA decoding,
6. explicit unsupported-target and outside-panel status reporting for pick/probe requests,
7. pick-result link keys propagated into `DvzSelectionItem`,
8. focused test coverage for status, visual-family, item/group/auxiliary id, link-key, and retained
   probe payload behavior.


## Validation

Validation run for this slice:

1. `just build`
2. `./build/testing/dvztest_scene test_scene_pick_probe_unsupported_targets`
3. `./build/testing/dvztest_scene test_scene_selection_apply_pick_and_link_keys`
4. `./build/testing/dvztest_scene test_scene_volume_slice_probe_cpu_sample`
5. `direnv exec . ./build/testing/dvztest_scene test_scene_process_pick_probe_requests`
6. `direnv exec . ./build/testing/dvztest_scene test_scene_point_pick_quadrants`
7. `direnv exec . ./build/testing/dvztest_scene test_scene_image_probe_segment_rgba_hidden_visual`
8. `direnv exec . ./build/testing/dvztest_scene pick_probe`
9. `direnv exec . ./build/testing/dvztest_scene point_pick`
10. `direnv exec . ./build/testing/dvztest_scene image_probe`
11. `direnv exec . ./build/testing/dvztest_scene marker_pick`
12. `direnv exec . ./build/testing/dvztest_scene pixel_pick`
13. `./build/testing/dvztest_scene pick_request`
14. `./build/testing/dvztest_scene probe_request`
15. `./build/testing/dvztest_scene process_requests`
16. `git diff --check`

The direct non-`direnv` GPU probe/request run skipped Vulkan initialization, as expected from the
macOS Vulkan environment guardrail; the same GPU paths passed through `direnv exec .`.


## Remaining Follow-Up

Deferred work remains explicit:

1. move beyond the 24-bit RGBA8 id payload when large item/label id ranges require it,
2. add deeper mesh/path/sphere/text/volume-DVR picking only when those slices are scoped,
3. add full highlight-mask rendering once the selection/highlight shader path is activated,
4. consider batched request execution only if profiling or real multi-panel traffic justifies it.
