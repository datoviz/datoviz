# `src/scene/scene.c` refactor notes

Date: 2026-05-11

This note records concrete cleanup opportunities in `src/scene/scene.c` after a focused review of
the current first-slice scene implementation. The file is functional, but it is carrying too many
distinct responsibilities in one translation unit and now has several obvious extraction points.


## Summary

There is clear room for refactoring. The highest-value work is structural rather than semantic:

1. Split `dvz_figure_emit_ex()` into smaller helpers.
2. Unify the duplicated pick/probe readback pipeline.
3. Centralize repeated visual/texture dirty-state resets.
4. Replace repeated constructor/setup patterns with small internal alloc/init helpers.
5. Decompose the JSON exporter into append helpers.

The aim should be to reduce local duplication and make ownership/state transitions easier to review
without changing the current vertical-slice behavior.


## Priority 1: split `dvz_figure_emit_ex()`

Hotspot: `src/scene/scene.c:1595`

`dvz_figure_emit_ex()` currently mixes:

- figure-id resolution
- frame-plan allocation
- dirty attribute upload emission
- index-buffer upload emission
- image-texture upload preparation
- panel render-node assembly
- nullable-argument normalization
- DRP2 emission
- post-emit dirty-flag clearing

That is too much control flow for one function, and some logic is already duplicated elsewhere.

Recommended helper boundaries:

- `_scene_figure_id(const DvzFigure*, char* out, uint32_t size)`
- `_scene_emit_visual_uploads(DvzFigure*, DvzFramePlan*)`
- `_scene_emit_panel_render(DvzFigure*, uint32_t panel_index, DvzFramePlan*, const char* figure_id)`
- `_scene_emit_defaults(...)` for nullable caps/report/cfg normalization
- `_scene_commit_emit_success(DvzFigure*)` for dirty-flag clearing

Benefit:

- easier to test emission stages independently
- easier to extend with new visual families without growing a single 300-line function
- lower risk when changing post-emit state transitions


## Priority 2: stop duplicating panel ordering and panel MVP setup

Hotspots:

- `src/scene/scene.c:440`
- `src/scene/scene.c:1758`
- `src/scene/scene.c:1783`
- `src/scene/scene.c:2036`

There is already a reusable `_scene_panel_visual_order()` helper, but `dvz_figure_emit_ex()` still
reimplements the same stable insertion sort inline at `1758-1779`. Panel MVP setup is also repeated
between main emit and pick handling.

Recommended extraction:

- reuse `_scene_panel_visual_order()` inside `dvz_figure_emit_ex()`
- add `_scene_panel_apply_mvp(const DvzPanel*, DvzMVP* out)`

Benefit:

- one sort implementation instead of two
- one panel-transform construction path instead of multiple near-identical sequences
- smaller surface for controller-mode regressions


## Priority 3: unify the ad-hoc pick/probe execution pipeline

Hotspots:

- `src/scene/scene.c:1968`
- `src/scene/scene.c:2109`

`_scene_process_point_pick_request()` and `_scene_process_image_probe_request()` share the same
overall workflow:

1. build a miss result
2. resolve panel id
3. convert request coordinates to panel NDC
4. traverse visuals in reverse z-order
5. build a one-off frame plan
6. emit DRP2 with a temporary emitter
7. execute runtime readback
8. destroy temporary objects
9. convert readback bytes into a scene result

The visual-family-specific parts are relatively small compared with the repeated plumbing.

Recommended extraction:

- `_scene_request_panel_id(...)`
- `_scene_execute_readback_plan(...)`
- one small family-specific callback/helper per request kind to populate uploads and decode result

Benefit:

- one place for temporary emitter/stream/report lifecycle
- easier to add triangle/image/mesh picking later without duplicating execution boilerplate
- lower risk of divergence between pick and probe behavior


## Priority 4: centralize visual texture dirty-state reset

Hotspots:

- `src/scene/scene.c:667`
- `src/scene/scene.c:712`
- `src/scene/scene.c:778`
- `src/scene/scene.c:3709`
- `src/scene/scene.c:3886`
- `src/scene/scene.c:4614`
- `src/scene/scene.c:1862`

The following state reset pattern appears in several places:

- `visual->texture.dirty = ...`
- `visual->texture.field_dirty = ...`
- `visual->texture.field_dirty_full = ...`
- `dvz_memset(&visual->texture.field_dirty_region, ...)`

Right now the same state machine is spread across field mutation, scale mutation, field release,
field destruction, and post-emit cleanup.

Recommended extraction:

- `_scene_visual_texture_mark_clean(DvzVisual*)`
- `_scene_visual_texture_mark_full_dirty(DvzVisual*, const DvzSampledFieldDesc*)`
- `_scene_visual_texture_mark_region_dirty(DvzVisual*, const DvzSampledFieldDesc*, DvzFieldRegion)`

Benefit:

- cleaner ownership/lifetime reasoning
- fewer opportunities for one path to forget one flag
- easier future transition if field/texture state moves out of `DvzVisual`


## Priority 5: remove repeated visual/buffer/field allocation patterns

Hotspots:

- `src/scene/scene.c:3633`
- `src/scene/scene.c:3940`
- `src/scene/scene.c:4380`
- `src/scene/scene.c:4493`
- `src/scene/scene.c:4554`

The file contains several families of near-identical allocation/setup code:

- slot scan + zero-init + count update for sampled fields
- slot scan + zero-init + count update for scene buffers
- slot allocation + common initialization for each visual constructor
- owned image-field creation in `dvz_visual_set_texture()` and `dvz_visual_set_texture_f32()`

Recommended extraction:

- `_scene_alloc_field_slot(DvzScene*)`
- `_scene_alloc_buffer_slot(DvzScene*)`
- `_scene_alloc_visual(DvzScene*, DvzVisualType, uint32_t flags)`
- `_scene_ensure_owned_image_field(DvzVisual*, DvzFieldFormat, DvzFieldSemantic, uint32_t w, uint32_t h)`

Benefit:

- fewer copy/paste initialization paths
- less risk of newly added visual families missing common defaults
- easier to audit zero-initialization and counter maintenance


## Priority 6: simplify destruction paths

Hotspots:

- `src/scene/scene.c:1486`
- `src/scene/scene.c:4153`

`dvz_scene_destroy()` and `dvz_visual_destroy()` both manually free overlapping visual-owned state.
They are not identical, which makes future maintenance harder. For example, scene destroy performs
manual binding resets and upload-buffer cleanup inline, while visual destroy delegates some of that
to the binding-release helpers.

Recommended extraction:

- `_scene_visual_reset(DvzVisual*, bool release_owned_resources)`
- `_scene_field_reset(DvzSampledField*)`
- `_scene_buffer_reset(DvzSceneBuffer*)`

Benefit:

- one teardown definition per object family
- easier idempotence review
- less chance of scene destroy and per-object destroy drifting apart


## Priority 7: decompose JSON export

Hotspot: `src/scene/scene.c:4754`

`dvz_scene_json()` is a monolithic serializer that knows the layout of fields, buffers, figures,
panels, visuals, attributes, and bindings. It is readable today, but it will become a maintenance
problem as scene objects grow.

Recommended extraction:

- `_scene_json_append_field(...)`
- `_scene_json_append_buffer(...)`
- `_scene_json_append_panel(...)`
- `_scene_json_append_visual(...)`

Benefit:

- smaller serialization units
- easier targeted updates when JSON shape changes
- lower chance of comma/ordering mistakes in future edits


## Correctness-adjacent cleanup worth doing during refactor

Hotspot: `src/scene/scene.c:2188`

In `_scene_process_image_probe_request()`, the `shifted` position buffer currently appears to be a
no-op:

- `target_ndc` is set to `request_ndc`
- `delta` therefore becomes zero
- the loop that subtracts `delta` from every position changes nothing

That means the heap allocation/copy at `2188-2199` is currently dead work unless the intended probe
logic was only partially implemented. Before refactoring this path, decide which of these is true:

1. the image probe should mirror point-pick recentering, in which case the target NDC constant is
   missing
2. the image probe should sample in place, in which case the temporary shifted copy can be removed

This is small, but it is the kind of ambiguity that gets harder to spot when the file stays large.


## Suggested execution order

1. Extract the shared emit helpers around `dvz_figure_emit_ex()`.
2. Reuse `_scene_panel_visual_order()` and add a shared panel-MVP helper.
3. Introduce one shared readback-execution helper for pick/probe requests.
4. Centralize texture dirty-state mutation helpers.
5. Fold visual/field/buffer constructors into small slot allocators.
6. Split JSON serialization only after the lifecycle code is cleaner.


## Non-goals

- Do not move this work into inactive modules.
- Do not broaden the scene public API just to support the refactor.
- Do not change the current scene -> DRP2 -> runtime ownership boundary while performing mechanical
  cleanup.
