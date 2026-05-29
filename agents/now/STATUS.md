# Datoviz v0.4 Status

> **Execution Status**
> - **Status:** `ACTIVE STATUS GATEBOARD`
> - **Updated on:** `2026-05-29`
> - **Purpose:** keep the current v0.4 implementation status concise, identify RC1 blockers,
>   and point to the durable spec or completed record for each lane.

This file is the branch-level status dashboard. It should stay short enough to scan before
starting work. Durable behavior belongs in `spec/`; completed implementation history belongs in
`agents/done/`; long-horizon or RC2 polish belongs in `agents/later/`.

Start release sequencing from
[`RELEASE.md`](RELEASE.md). Use [`DOCUMENTATION.md`](DOCUMENTATION.md) for API/docs inventory,
public documentation deliverables, and RC documentation gates. Use [`START.md`](START.md) for the
current dispatch context.


## Current Pickup

**Next critical-path item:** RC1 release proof: WebGPU/WASM disposition, v0.3 visible parity,
public API/status cleanup, and compact example validation.

The native scene path now has enough first-slice coverage for rendered text, linear 2D axes/ticks,
continuous colorbars, label annotations, scale bars, and retained textured mesh. Remaining work in
those lanes is RC proof or polish unless a release example exposes a concrete blocking gap.

Feature-freeze blockers:

| Lane | Status | Next proof |
| --- | --- | --- |
| Retained textured mesh | `Done first slice / RC proof` | Keep `examples/c/visuals/textured_mesh.c`, `test_scene_mesh_visual_binds_texture_field`, and `test_scene_textured_mesh_emits_texture_pipeline` in validation; add/promote `fixture_mesh_textured.c` or terrain/planet capture for release proof. |
| WebGPU/WASM experimental path | `Partial / blocker` | Supported subset, unsupported-feature diagnostics, and preflight/browser smoke. |
| Raw `ctypes` API | `Done for RC1` | Keep `just bindings` in RC validation; it covers generation, ABI layout checks, raw examples, render smoke, and editable/wheel install smokes. |
| v0.3 visible parity audit | `Missing / blocker` | Visible capability table with fix/defer/GSP disposition. |
| Public API/status cleanup | `Missing / blocker` | Supported, experimental, advanced/unstable, deferred, and external/GSP labels. |
| Release example proof | `Partial / blocker` | Compact native + WebGPU proof set with validation notes. |

Primary references:

1. [`RELEASE.md`](RELEASE.md)
2. [`DOCUMENTATION.md`](DOCUMENTATION.md)
3. [`../done/PINNED_READOUT_OVERLAY_CARD_IMPLEMENTATION.md`](../done/PINNED_READOUT_OVERLAY_CARD_IMPLEMENTATION.md)
4. [`../../spec/scene/examples/PLANNING.md`](../../spec/scene/examples/PLANNING.md)
5. [`../../spec/scene/api/API_SURFACE.md`](../../spec/scene/api/API_SURFACE.md)
6. [`../../spec/scene/validation/DEFERRED_TRACKER.md`](../../spec/scene/validation/DEFERRED_TRACKER.md)
7. [`../soon/scene/SCENE_SHINY_DEMO_NEXT_STEPS.md`](../soon/scene/SCENE_SHINY_DEMO_NEXT_STEPS.md)


## Feature Status

| Lane | Disposition | Evidence | Next |
| --- | --- | --- | --- |
| App frame scheduling | `Done` | [`../done/APP_FRAME_SCHEDULING_REFACTOR.md`](../done/APP_FRAME_SCHEDULING_REFACTOR.md) | Keep scheduler validation in release smoke only. |
| Text | `Closed first slice / RC polish` | `examples/c/visuals/text.c`, scene text tests, app/offscreen text smokes | Data/world placement, DPI/clipping, fallback diagnostics, and public API wording. |
| 2D axes and ticks | `Closed first slice / RC proof` | `src/scene/tests/axis.c`, `examples/c/techniques/scatter_axes.c` | Screenshot/offscreen proof, formatter/clipping polish, shared reserve behavior. |
| Continuous colorbars and categorical legends | `Closed first slices` | `src/scene/tests/fields.c`, `test_app_offscreen_colorbar_has_visible_ramp_and_labels`, `examples/c/visuals/colorbar.c`, `examples/c/showcase/labels.c` | Shared layout, richer legend composition, and interactive scale/range editing remain follow-up. |
| Label annotations and readouts | `Readout, selection, overlay-card, and FreeType rich text-block slices landed` | annotation/text realization tests, `test_scene_selection_card_realizes_pick_metadata`, `test_scene_overlay_card_public_api`, `test_scene_overlay_card_rich_text_public_api`, `test_app_offscreen_text_block_raster_has_nonblank_pixels`, `examples/c/techniques/image_probe.c`, `examples/c/techniques/overlay_card.c`, `examples/c/techniques/overlay_rich_card.c`, `examples/c/techniques/rich_text_block.c`, [`../done/PINNED_READOUT_OVERLAY_CARD_IMPLEMENTATION.md`](../done/PINNED_READOUT_OVERLAY_CARD_IMPLEMENTATION.md) | Keep richer text-block polish, DPI cache keys, wrapping, HarfBuzz shaping, richer font-style resolution, and broader annotation rich-text integration as follow-up. |
| Scale bars | `Closed first slice / validation` | [`../done/SCENE_SCALEBAR_RENDERING_SLICE.md`](../done/SCENE_SCALEBAR_RENDERING_SLICE.md), [`../done/SCENE_SCALEBAR_3D_REFERENCE_SLICE.md`](../done/SCENE_SCALEBAR_3D_REFERENCE_SLICE.md), [`../done/SCENE_SCALEBAR_UPDATE_PERF_REFACTOR.md`](../done/SCENE_SCALEBAR_UPDATE_PERF_REFACTOR.md) | Keep fixture/example smoke and churn trace in release validation. |
| Grid layout and linked panels | `Partial / RC proof` | grid/panel tests, `examples/c/techniques/linked_panels.c` | Prove release examples resize and link predictably; defer richer dashboard layout. |
| Visual families | `Mostly first-slice active` | point, pixel, marker, primitive, mesh including retained textured mesh, path/segment, image, labels, volume, sphere, polygon/composite examples/tests | Fill release-example gaps and mark unsupported variants explicitly. |
| Pick, probe, selection | `Broader first slices landed` | point/pixel/marker/sphere/stroke/primitive/image/mesh/volume item-pick tests, image/probe tests, label segment-probe tests, selection-mask tests, `examples/c/techniques/pick_hover.c` | Richer payloads, linked-panel probe state, exact marker/path semantics, mesh face/region identity, volume ray hits, and text picking remain follow-up. |
| WebGPU/WASM | `Browser proof done / WASM emission pending` | `examples/webgpu/`, `examples/webgpu/COMPAT.md`, `tools/webgpu_fixture_preflight.py`, `tools/webgpu_runner_smoke.mjs`, DRP2 WGSL point/primitive/image fixtures; `just webgpu-fixture-preflight` passes `39/39`, `just webgpu-runner-smoke` passes `37 + 2 + 81` plus repeated runtime frames, browser dashboard passed fixture compatibility `120/120` and retained runtime stress `4/4` on 2026-05-29 after `292e82899` | Keep WASM scene-emission and browser capability diagnostics as the remaining experimental-path blockers. |
| Raw `ctypes` | `Done for RC1` | `tools/bindings/extract_api.py`, `tools/bindings/generate_ctypes.py`, `tools/bindings/generate_ctypes_abi.py`, `tools/bindings/ctypes_package_smoke.py`, `testing/test_ctypes_raw_smoke.py`, `examples/python/raw/`, `just bindings` | Broaden ABI/pointer policy only when richer raw examples require it. |
| Runtime hardening | `Ongoing` | DRP2/vklite/app tests and completed lifetime records | Fix concrete lifetime, resize, descriptor, repeated-frame, or churn bugs as examples expose them. |
| Scene source split | `Ongoing / structural` | [`../../spec/scene/implementation/SCENE_CODE_SPLIT_ROADMAP.md`](../../spec/scene/implementation/SCENE_CODE_SPLIT_ROADMAP.md), [`../../spec/scene/implementation/SCENE_ARCHITECTURE_COMPLETION_PLAN.md`](../../spec/scene/implementation/SCENE_ARCHITECTURE_COMPLETION_PLAN.md) | The old scene `plan/` bucket is gone; FramePlan, scene emission, render contracts, runtime render emission, core scene, visual descriptor-kind, colormap, domain-buffer, query-policy, upload-support, panel-helper, helper-declaration boundary, shared query-helper, query render-metadata guard, standard query item-decode, standard item-target eligibility, query native-target policy, FramePlan/render-contract metadata enforcement, and field dirty propagation slices are split. Latest focused validation includes `scene/query` (`40/40`), `scene/frame-plan` (`55/55`), `scene-graph` (`158/158`), focused field-update filters, and `app-offscreen` (`76/76`). Next pickup is remaining query-family ownership, deeper typed retained fallback cleanup, annotation/domain cleanup, then coarse standalone scene layer feasibility; only move remaining upload payload builders after confirming they are still mixed into scene emission. |
| API inventory and docs | `Blocker for RC1` | [`DOCUMENTATION.md`](DOCUMENTATION.md), public headers under `include/datoviz/` | Produce public surface/status table and known-gap notes. |


## Shiny Demo Follow-Up

These lanes are not more important than the WebGPU/WASM and raw `ctypes` blockers, but they are the
best next choices when the task is to improve examples or add a visible capability:

| Priority | Lane | Current read | Next action |
| ---: | --- | --- | --- |
| 1 | Gallery proof pass | Protein, LiDAR, brain, labels, textured mesh, and WebGPU subset examples form the proof set. | Run/capture/tune defaults, promote or add the textured terrain/planet proof, and fix concrete rough edges before RC1. |
| 2 | Vector/arrow visual | Missing as a semantic visual; wind-field examples can use primitives only as a temporary bridge. | Start from [`../soon/scene/SCENE_VECTOR_VISUALS_PLAN.md`](../soon/scene/SCENE_VECTOR_VISUALS_PLAN.md), then pressure it with a wind-field showcase. |
| 3 | Label probe hardening | Raw `dvz_labels()` integer probe and sparse signed/unsigned label-volume lookup are implemented. | Broaden transform, larger-field, request-churn, and readback-efficiency coverage. |
| 4 | Explanatory layout proof | Reserve/layout infrastructure exists for axes, colorbars, legends, and scale bars. | Add one composed example and focused validation for predictable adornment composition. |
| 5 | Splat visual | Not implemented and intentionally a new visual family; acceptable as v0.4 experimental showcase scope if it lands cleanly. | Add retained splat fixture and dense capture only after release-proof lanes stay on track; full Gaussian-splat pipelines remain later. |

The longer rationale is in
[`../soon/scene/SCENE_SHINY_DEMO_NEXT_STEPS.md`](../soon/scene/SCENE_SHINY_DEMO_NEXT_STEPS.md).


## Parallel Lanes

Good parallel work now:

1. **RC1 release closure:** feature/status table, v0.3 visible parity audit,
   WebGPU/WASM subset smoke, and compact example proof list.
2. **Example proof:** C examples and fixture/gateway smokes that use already-implemented features,
   including retained textured mesh.
3. **Runtime hardening:** focused scene -> DRP2 -> vklite/canvas/app lifetime or churn fixes with
   narrow tests.
4. **WebGPU parity:** `examples/webgpu`, DRP2 fixtures/preflight, runner smoke, browser dashboard
   proof, WGSL emission, and diagnostics.
5. **API/docs inventory:** work from [`DOCUMENTATION.md`](DOCUMENTATION.md) that classifies actual
   v0.4 behavior.
6. **Scene source split:** staged cleanup from
   [`../../spec/scene/implementation/SCENE_CODE_SPLIT_ROADMAP.md`](../../spec/scene/implementation/SCENE_CODE_SPLIT_ROADMAP.md),
   continuing with remaining query-family ownership, normal typed-metadata enforcement outside
   query, annotation/domain helpers, standalone scene layer feasibility, and focused tests.
   FramePlan internals, scene emission, render contracts, runtime render emission, core scene
   ownership, first colormap/domain-buffer slices, query policy, upload-support helpers, panel
   helpers, helper-declaration boundary pass, shared query helpers, query render-metadata guard,
   standard query item decoding, standard item-target eligibility, query native-target policy,
   FramePlan/render-contract metadata enforcement, and field dirty propagation are complete through
   2026-05-29.
7. **RC2 polish:** text placement/DPI, axes formatter/clipping, shared layout, richer legends,
   richer readouts, and broader pick/probe payloads.
8. **Pinned readout/card lane:** completed private C card shell, rendered pinned image readouts,
   selected-item metadata cards, public overlay card API, rich overlay card API, example proof,
   non-overlay rich text-block proof, and FreeType-backed private rich text-block rasterization in
   [`../done/PINNED_READOUT_OVERLAY_CARD_IMPLEMENTATION.md`](../done/PINNED_READOUT_OVERLAY_CARD_IMPLEMENTATION.md).
9. **Shiny demo follow-up:** gallery proof first, then vector/arrow visual, label GPU probing,
   explanatory layout proof, and optional experimental splats as recorded in
   [`../soon/scene/SCENE_SHINY_DEMO_NEXT_STEPS.md`](../soon/scene/SCENE_SHINY_DEMO_NEXT_STEPS.md).

Avoid parallel edits that touch the same write scope:

1. scene visual emission and shader bind layouts,
2. DRP2 schema/fixture changes and WebGPU/DVZR serialization for the same command,
3. app/runtime frame-loop changes and request-processing behavior,
4. shared test registration blocks,
5. public header renames while feature or binding agents are active.


## Validation Defaults

For documentation-only passes:

1. `git diff --check`
2. inspect `git status --short`

For scene/DRP2/runtime code changes:

1. `just build`
2. the narrowest relevant `just test <filter>`
3. `just spec-check` for DRP2 schema, fixture, or portable-command changes
4. Vulkan validation or bounded GLFW/offscreen smoke when graphics lifetimes, command buffers,
   render targets, swapchains, or synchronization are touched


## Tracker Maintenance

Keep this file concise:

1. one row per lane,
2. one disposition per row,
3. one evidence pointer and one next action,
4. no implementation diary,
5. no reusable agent prompts.

When a lane lands, move or rewrite the final record under `agents/done/`, remove the active plan
from `agents/now/` or `agents/soon/`, and update README/index links in the same cleanup.
