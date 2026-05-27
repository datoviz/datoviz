# Datoviz v0.4 Status

> **Execution Status**
> - **Status:** `ACTIVE STATUS GATEBOARD`
> - **Updated on:** `2026-05-27`
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

**Next critical-path item:** retained textured mesh plus RC1 release proof.

The native scene path now has enough first-slice coverage for rendered text, linear 2D axes/ticks,
continuous colorbars, label annotations, and scale bars. Remaining work in those lanes is RC proof
or polish unless a release example exposes a concrete blocking gap. The gallery strategy now treats
true retained textured mesh as required for v0.4, so it is the main new feature-freeze blocker before
the terrain/planet showcase can be counted.

Feature-freeze blockers:

| Lane | Status | Next proof |
| --- | --- | --- |
| Retained textured mesh | `Missing / blocker` | UV attribute upload, mesh-bound sampled texture, `color_mode = texture` shader/pipeline variant, lighting/material integration, sampler defaults, retained replacement/update path, `fixture_mesh_textured.c`, and terrain/planet C showcase. |
| WebGPU/WASM experimental path | `Partial / blocker` | Supported subset, unsupported-feature diagnostics, and preflight/browser smoke. |
| Raw `ctypes` API | `Partial / blocker` | Regenerate/load smoke and public header scope note. |
| v0.3 visible parity audit | `Missing / blocker` | Visible capability table with fix/defer/GSP disposition. |
| Public API/status cleanup | `Missing / blocker` | Supported, experimental, advanced/unstable, deferred, and external/GSP labels. |
| Release example proof | `Partial / blocker` | Compact native + WebGPU proof set with validation notes. |

Primary references:

1. [`RELEASE.md`](RELEASE.md)
2. [`DOCUMENTATION.md`](DOCUMENTATION.md)
3. [`../done/PINNED_READOUT_OVERLAY_CARD_IMPLEMENTATION.md`](../done/PINNED_READOUT_OVERLAY_CARD_IMPLEMENTATION.md)
4. [`../../spec/scene/examples/EXAMPLE_RELEASE_STAGING.md`](../../spec/scene/examples/EXAMPLE_RELEASE_STAGING.md)
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
| Visual families | `Mostly first-slice active; retained textured mesh missing` | point, pixel, marker, primitive, mesh, path/segment, image, labels, volume, sphere, polygon/composite examples/tests | Land mesh texture binding/shader path, fill release-example gaps, and mark unsupported variants explicitly. |
| Pick, probe, selection | `Broader first slices landed` | point/pixel/marker/sphere/stroke/primitive/image/mesh/volume item-pick tests, image/probe tests, label segment-probe tests, selection-mask tests, `examples/c/techniques/pick_hover.c` | Richer payloads, linked-panel probe state, exact marker/path semantics, mesh face/region identity, volume ray hits, and text picking remain follow-up. |
| WebGPU/WASM | `Partial / blocker` | `examples/webgpu/`, `tools/webgpu_fixture_preflight.py`, DRP2 WGSL fixtures | Document and smoke the experimental subset. |
| Raw `ctypes` | `Partial / blocker` | `tools/build_ctypes.py`, `datoviz/_ctypes.py`, `just ctypes` | Regenerate, load, and document low-level binding scope. |
| Runtime hardening | `Ongoing` | DRP2/vklite/app tests and completed lifetime records | Fix concrete lifetime, resize, descriptor, repeated-frame, or churn bugs as examples expose them. |
| API inventory and docs | `Blocker for RC1` | [`DOCUMENTATION.md`](DOCUMENTATION.md), public headers under `include/datoviz/` | Produce public surface/status table and known-gap notes. |


## Shiny Demo Follow-Up

These lanes are not more important than the WebGPU/WASM and raw `ctypes` blockers, but they are the
best next choices when the task is to improve examples or add a visible capability:

| Priority | Lane | Current read | Next action |
| ---: | --- | --- | --- |
| 1 | Retained textured mesh | Required for the v0.4 terrain/planet showcase; current mesh path does not yet bind textures as mesh material input. | Implement UV/texture binding and the mesh texture shader variant, then prove it with fixture and terrain/planet capture. |
| 2 | Vector/arrow visual | Missing as a semantic visual; wind-field examples can use primitives only as a temporary bridge. | Start from [`../soon/scene/SCENE_VECTOR_VISUALS_PLAN.md`](../soon/scene/SCENE_VECTOR_VISUALS_PLAN.md), then pressure it with a wind-field showcase. |
| 3 | Raw label-id GPU probe | First `dvz_labels()` raw integer probe slice is implemented for 2D labels fields and no longer depends on hidden RGBA image masks. | Broaden sparse/high-id pressure coverage and optimize the request readback path beyond full-texture copies. |
| 4 | Explanatory layout proof | Reserve/layout infrastructure exists for axes, colorbars, legends, and scale bars. | Add one composed example and focused validation for predictable adornment composition. |
| 5 | Gallery proof pass | Protein, LiDAR, brain, labels, and the upcoming textured terrain/planet showcase form the proof set. | Run/capture/tune defaults and fix concrete rough edges before RC1. |
| 6 | Splat visual | Not implemented and intentionally a new visual family. | Keep after release proof unless maximum visual novelty is the goal. |

The longer rationale is in
[`../soon/scene/SCENE_SHINY_DEMO_NEXT_STEPS.md`](../soon/scene/SCENE_SHINY_DEMO_NEXT_STEPS.md).


## Parallel Lanes

Good parallel work now:

1. **RC1 release closure:** feature/status table, v0.3 visible parity audit, raw `ctypes` smoke,
   WebGPU/WASM subset smoke, compact example proof list.
2. **Textured mesh:** UV upload, mesh texture resource binding, shader/pipeline variant, sampler
   defaults, material/lighting integration, and deterministic terrain/planet proof.
3. **Example proof:** C examples and fixture/gateway smokes that use already-implemented features.
4. **Runtime hardening:** focused scene -> DRP2 -> vklite/canvas/app lifetime or churn fixes with
   narrow tests.
5. **WebGPU parity:** `examples/webgpu`, DRP2 fixtures/preflight, WGSL emission, and diagnostics.
6. **API/docs inventory:** work from [`DOCUMENTATION.md`](DOCUMENTATION.md) that classifies actual
   v0.4 behavior.
7. **RC2 polish:** text placement/DPI, axes formatter/clipping, shared layout, richer legends,
   richer readouts, and broader pick/probe payloads.
8. **Pinned readout/card lane:** completed private C card shell, rendered pinned image readouts,
   selected-item metadata cards, public overlay card API, rich overlay card API, example proof,
   non-overlay rich text-block proof, and FreeType-backed private rich text-block rasterization in
   [`../done/PINNED_READOUT_OVERLAY_CARD_IMPLEMENTATION.md`](../done/PINNED_READOUT_OVERLAY_CARD_IMPLEMENTATION.md).
9. **Shiny demo follow-up:** retained textured mesh first, then vector/arrow visual, label GPU
   probing, explanatory layout proof, gallery proof, and splats as recorded in
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
