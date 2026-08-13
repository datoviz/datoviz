# Issues #139 And #140 Handoff

Status: audited and approved implementation handoff for the active `v0.4-dev` line. Updated: 2026-08-13.

This handoff covers [issue #139](https://github.com/datoviz/datoviz/issues/139) and [issue #140](https://github.com/datoviz/datoviz/issues/140). A local implementation audit and an independent `gpt-5.6-sol` high-reasoning architectural audit agreed that #140 exposes resource-lifecycle defects deeper than its reported shrink symptom and that #139 requires a distinct committed-text stream rather than key remapping. Land the checkpoints below as focused commits after their gates pass; do not publish a new geometry-resource API as part of these fixes.

## Decisions

### Issue #139: physical keys and layout-aware text

Keep `DVZ_KEY_*` as physical key identities named for their standard-US positions. Do not remap them through the current layout and do not add AZERTY-specific aliases.

Add a separate public committed UTF-8 stream for layout-aware commands and text. Use `DvzInputTextEvent`, `DvzInputTextCallback`, `DVZ_INPUT_EVENT_TEXT`, `dvz_input_subscribe_text()`, `dvz_input_emit_text()`, and `dvz_view_emit_text()`. Do not add `dvz_text_input_emit()` or `dvz_text_emit()`; `dvz_text_*` remains the scene text-rendering namespace. The normative contract is [../../spec/scene/interaction/KEYBOARD_INPUT.md](../../spec/scene/interaction/KEYBOARD_INPUT.md).

`DvzInputTextEvent` carries one borrowed UTF-8 commit span, byte size, modifier snapshot, and user data. The implementation must cover the input router, union event stream, native GLFW character routing, GUI capture without duplicates, hosted injection, ordinary Qt committed-key text, generated bindings, focused examples or fixtures, and documentation. Do not fold a general shortcut-remapping system, composition/preedit UI, full IME, or text editor into this issue.

### Issue #140: coherent mesh geometry replacement

A successful `dvz_mesh_set_geometry()` call replaces the complete retained mesh payload. Vertex/index resources, logical counts, draw metadata, dirty/version state, and the next validated frame plan must describe the same new geometry whether the replacement grows, shrinks, or preserves capacity.

The caller must not recreate the panel, visual, or scene when topology dimensions change. Fix the retained resource/count invalidation path rather than weakening draw validation or clamping stale draw counts during emission.

The reported validation error is correct. The FramePlan is rebuilt, but ordinary mesh index count is reconstructed from retained allocation `byte_size` rather than the smaller logical upload extent. The same audit must cover vertex and instance counts derived from capacity.

Scene-buffer array slots are reusable while persistent resource keys currently derive from slot indices. This aliases unrelated semantic lifetimes. Fix #140 on a scene-resource lifecycle foundation: immutable semantic identity, separate logical extent and capacity, explicit owner-driven retirement, transactional retirement emission, backend-safe deferred physical destruction, and registry reclamation only after successful emission. Migrate scene buffers completely; design the internal record for later resource kinds without migrating all graph, texture, sampler, or pipeline resources in this issue.

Mesh replacement must also become atomic. Reuse stable visual-owned buffers, prepare all facet/index changes before commit, implement indexed-to-nonindexed transitions, and leave the prior geometry intact on failure. Keep the retained geometry transaction internal until public partial-update, sharing, optional-facet, and lifetime semantics are deliberately designed.

## Checkpoint 0: Contract And Reproduction

1. Read [../../spec/scene/pipeline/RESOURCE_MODEL.md](../../spec/scene/pipeline/RESOURCE_MODEL.md), [../../spec/scene/pipeline/INVALIDATION_AND_CACHING.md](../../spec/scene/pipeline/INVALIDATION_AND_CACHING.md), [../../spec/scene/pipeline/FRAME_PLAN.md](../../spec/scene/pipeline/FRAME_PLAN.md), [../../spec/scene/semantics/VISUAL_CONTRACT.md](../../spec/scene/semantics/VISUAL_CONTRACT.md), and [../../spec/scene/interaction/KEYBOARD_INPUT.md](../../spec/scene/interaction/KEYBOARD_INPUT.md).
2. Reproduce #140 without weakening the runtime validator. Prove that retained capacity exceeds logical index extent after a large-to-small replacement and identify every semantic count still derived from `byte_size / stride`.
3. Add failing tests for large-to-small-to-large indexed replacement, indexed-to-nonindexed-to-indexed replacement, instance-count shrink, destroy/recreate with buffer-slot reuse, and failed replacement rollback.

Commit boundary: regression tests and any necessary diagnostic-only support, with the old behavior demonstrably failing.

## Checkpoint 1: Scene-Buffer Lifecycle Foundation

1. Add stable semantic identity to `DvzSceneBuffer`; never derive identity from `_scene_buffer_index()` or a reusable array slot.
2. Represent logical byte/item extent independently from allocation capacity. Full and zero-length replacements update logical extent even when capacity is retained.
3. Introduce explicit live/retired scene-resource state with ownership and descriptor/content/lifecycle revisions sufficient for deterministic planning.
4. Propagate retirement through FramePlan and the transactionally cloned emitter. A failed plan or conversion leaves retirement pending.
5. Preserve one runtime object id across compatible capacity growth and use existing DRP2/vklite recreation and dependent-bind-group refresh behavior.
6. Emit DRP2 destruction for committed retirement and rely on runtime deferred destruction for in-flight or borrowed backend objects.
7. Reclaim registry entries only after retirement commits. Prove repeated create/destroy does not grow scene-emitter state without bound.
8. Keep readable resource keys as derived/debug identities for now; do not replace the entire key system or migrate unrelated resource kinds.

Starting points: `src/scene/core/_scene.h`, `src/scene/domain/buffer.c`, `src/scene/core/resource_key.c`, `src/scene/runtime/state.c`, `src/scene/runtime/upload.c`, scene FramePlan emission, and existing DRP2 recreate/destroy tests.

Commit boundary: scene-buffer identity, extent, lifecycle, retirement, and focused runtime tests all green.

## Checkpoint 2: Semantic Counts And Atomic Mesh Replacement

1. Make visual descriptor finalization obtain index, vertex, and instance counts from logical extent or explicit semantic metadata, never capacity.
2. Retain compatible visual-owned attribute/index buffers rather than destroying and allocating a new scene buffer for every full replacement.
3. Stage geometry conversion, facet validation, allocations, index presence, bindings, and counts before committing the visual.
4. Explicitly unbind the prior index facet when the new geometry is nonindexed.
5. Leave the complete prior renderable state intact on validation, allocation, conversion, or binding failure.
6. Do not promise arbitrary omitted-facet removal through the current `DvzGeometry`; its standard constructor materializes its standard facets. Treat a future facet-presence descriptor as separate API work.

Starting points: `src/scene/domain/mesh_geometry.c`, visual data/binding code, `src/scene/visuals/desc.c`, `src/scene/scene_emit/metadata.c`, `src/scene/scene_emit/upload_support.c`, and `src/scene/runtime/draw_packet.c`.

Commit boundary: Philippe's shrink reproduction, all transition/rollback cases, repeated FramePlans, and validation-enabled runtime emission pass.

## Checkpoint 3: Committed UTF-8 Input

1. Append `DVZ_INPUT_EVENT_TEXT` without renumbering existing kinds and add the public event/callback types and direct router subscription.
2. Make `dvz_input_emit_text()` synchronously dispatch a validated borrowed UTF-8 commit and union event.
3. Add `dvz_view_emit_text()` for hosted adapters; do not add another public convenience emitter.
4. Route GLFW character input into the router while preserving the distinct physical key callback and modifier state.
5. Feed GUI text input before application routing and suppress application delivery only while GUI keyboard/text capture is active.
6. Route ordinary Qt committed-key text as UTF-8 on press/repeat and nothing on release. Use tested native scan-code mapping for physical keys and emit `DVZ_KEY_UNKNOWN` where physical identity is unavailable; do not substitute `QKeyEvent::key()` layout meaning.
7. Keep logical accelerators, composition/preedit, and full IME outside this checkpoint.
8. Refresh generated bindings and update examples/course input proof so positional controls use physical keys and character-labelled commands use committed text.

Starting points: `include/datoviz/input/router.h`, `src/input/input_router.c`, `src/window/backend_glfw.c`, `src/gui/gui.cpp`, `include/datoviz/app.h`, `src/app/app.c`, the Qt adapter, bindings, and input tests.

Commit boundary: public input API, native/hosted routing, bindings, tests, and documentation agree.

## Validation Matrix

For resource lifecycle and #140:

- large-to-small-to-large indexed replacement with retained capacity;
- indexed-to-nonindexed-to-indexed transitions;
- vertex, index, and instance logical-count shrink;
- zero logical extent;
- destroy/recreate in one scene slot with different size, stride, and usage;
- same-runtime-id growth recreation and dependent binding refresh;
- failed replacement preserves prior visual state;
- repeated FramePlan emission and repeated create/destroy without state-table growth;
- deferred physical destruction under borrowed/in-flight command-buffer conditions.

For #139:

- physical identity remains independent from layout-generated text;
- press/repeat and no release text;
- valid ASCII, non-ASCII, and multiple-scalar UTF-8 commits plus malformed-input rejection;
- modifier snapshots and synchronous borrowed-span lifetime;
- direct and union subscription mutation/unsubscribe safety;
- GUI capture without duplicate delivery;
- hosted and Qt UTF-8 conversion plus unknown physical-key fallback;
- controller held state remains physical.

## Validation Gates

For each checkpoint, run the narrowest affected tests while iterating. Before finalizing #140, run focused geometry/mesh, scene-buffer, FramePlan, DRP2 recreation/destruction, runtime-vklite, and validation-enabled scene tests, followed by `just build`, `just spec-check`, and `git diff --check`.

Before finalizing #139, run focused input/router tests, available GLFW and Qt adapter checks, `just ctypes`, `just ctypes-check`, `just build`, `just spec-check`, the chapter-5 input smoke when present, and `git diff --check`.

Before each commit, inspect `git status --short` and `git diff --cached --stat`; exclude the `data` submodule, generated/runtime binaries, paper artifacts, and unrelated user changes.

## Completion And GitHub Follow-Up

Each issue is complete only when its regression tests fail on the old behavior, pass with the fix, and the relevant contract and generated surfaces agree with the implementation. #140 additionally requires stable resource-table size under repeated replacement/destruction and strict draw validation. After validated commits are pushed, prepare concise GitHub comments naming behavior, tests, and commits, then obtain explicit approval before publishing or closing either issue.
