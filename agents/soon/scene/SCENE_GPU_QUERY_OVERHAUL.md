# Scene GPU Query Overhaul

> **Execution Status**
> - **Status:** `IN PROGRESS - FOUNDATION LANDED`
> - **Updated on:** `2026-05-27`
> - **Purpose:** give the next agent team an immediately actionable plan for replacing scene
>   pick/probe with a GPU-only query system.

Start here when asked to implement or continue the pick/probe/probe-request overhaul.

Durable architecture lives in
[`../../../spec/scene/interaction/GPU_QUERY_SYSTEM.md`](../../../spec/scene/interaction/GPU_QUERY_SYSTEM.md).
This file is execution guidance: stages, subagent lanes, expected edits, and validation.


## One-Sentence Goal

Replace the current public pick/probe request paths with one GPU-only panel query system, split query
code into generic orchestration and visual-family implementations, remove CPU visual fallback paths,
and harden DRP2/FramePlan enough to carry portable integer query payloads.


## Read First

1. [`../../../spec/scene/interaction/GPU_QUERY_SYSTEM.md`](../../../spec/scene/interaction/GPU_QUERY_SYSTEM.md)
2. [`../../../spec/scene/interaction/PANEL_QUERY.md`](../../../spec/scene/interaction/PANEL_QUERY.md)
3. [`../../../spec/scene/interaction/PICKING.md`](../../../spec/scene/interaction/PICKING.md)
4. [`../../../spec/drp2/CAPABILITIES.md`](../../../spec/drp2/CAPABILITIES.md)
5. [`../../../agents/later/SCENE_PICK_PROBE_REQUEST_PATH_REFACTOR.md`](../../../agents/later/SCENE_PICK_PROBE_REQUEST_PATH_REFACTOR.md)

The older pick/probe refactor note is useful audit context, but the GPU query spec supersedes it for
new design decisions.


## Current Code Hotspots

1. `src/scene/request_execute.c`: main file to split and eventually delete or shrink to a migration
   wrapper. It currently mixes queue execution, family dispatch, geometry expansion, readback, labels,
   volume, and result decode.
2. `src/scene/query/queue.c`: transitional public query bridge that still queues old pick/probe
   requests and converts the old results into `DvzQueryResult`.
3. `src/scene/probe_plan.c`: image probe plan builder still used by the bridge path, with old
   one-sample readback assumptions.
4. `src/scene/render_pass.c`: scene-to-DRP2 converter now forwards `DvzFramePlanCopyDesc` metadata
   for supported DRP2 fields, but runtime copy selection is still tied to the current color target.
5. `include/datoviz/scene/frame_plan.h`: copy metadata exists; multi-output query integration and
   full DRP2 origin/depth/attachment execution remain.
6. `include/datoviz/scene/types.h`: query request/result/capability fields exist; capability-driven
   profile selection is not implemented.
7. `include/datoviz/scene/interaction.h`: public query API exists, while pick/probe remain public
   transitional APIs.
8. `src/drp2/serialization.c` and `spec/drp2/schema/common/GPUTextureFormat.json`: already contain
   `r32uint` and `rg32uint`, useful for query profiles.
9. `spec/drp2/fixtures/positive/pressure_picking_readback.json`: existing `r32uint` readback fixture
   that should be expanded into runtime and scene-facing query tests.


## Landed Foundation - 2026-05-27

Committed implementation slices:

1. `87c91a15a Add scene query migration guardrails`
   - source guard for generic query purity,
   - CMake inclusion for `src/scene/query/` and `src/scene/visuals/*/`,
   - migration marker in `request_execute.c`.
2. `108fda50d Add scene query capability surface`
   - public query profiles, statuses, value kinds, request/result structs,
   - query/readback capability fields and app-side Vulkan population.
3. `566b13f45 Widen frame plan query readback metadata`
   - `DvzFramePlanCopyDesc`,
   - `dvz_frame_plan_copy_ex()`,
   - fixture/runtime metadata propagation and tests.
4. `1bd36cc71 Add panel query API bridge`
   - public panel query, poll, process, and blocking test helper,
   - old pick/probe to query-result conversion.
5. `6c4789d00 Add scene query family registry`
   - generic registry,
   - family operation tables for point, pixel, marker, sphere, segment, path, primitive, mesh,
     image, labels, volume, text, and glyph.
6. `5b371244c Let selection and readouts consume query results`
   - selection and pinned readout adapters for `DvzQueryResult`.
7. `2b11d7324 Add native scene query queues`
   - native pending/result queues,
   - query freshness/coalescing,
   - query processing no longer delegates wholesale to pick/probe requests.
8. `d741cf662 Route native queries through request executor adapter`
   - native query processing uses the retained request executor adapter for existing GPU-backed
     pick/probe plans without leaving public pick/probe results queued.
9. `fc2c89ed3 Support rg32uint DRP2 texture layout sizing`
   - DRP2 C format byte sizing now accepts `VK_FORMAT_R32G32_UINT`.
10. `cfb309c05 Cover pixel visual through native query API`
    - pixel query coverage through `dvz_panel_query()` / `dvz_figure_process_queries()`.
11. `d2fc260c8 Cover native query identity and pixel values`
    - point identity and image pixel-value query coverage.
12. `590a6881a Keep volume queries from CPU probe fallback`
    - native query avoids the old CPU volume slice probe path and returns explicit unsupported for
      volume sample until a GPU family path lands.

Recorded validation after these commits:

1. `just build`
2. `just test frame_plan`
3. `just test pick-probe`
4. `just test query`
5. `just test scene` with `463/463`
6. `git diff --check`
7. `python testing/test_scene_query_source_guard.py`


## Decisions Already Chosen

Use these defaults unless the user explicitly changes them:

1. Public direction: one `dvz_panel_query()` style API and one `DvzQueryResult`.
2. Compatibility: breaking v0.4-dev public API is allowed.
3. Rendered visual query authority: GPU only.
4. CPU allowance: queueing, freshness, opaque id mapping, label formatting, and selection/link
   mutation after a valid GPU result.
5. CPU prohibition: no CPU hit testing, CPU visual geometry reconstruction, or CPU retained-data
   sampling as query fallback.
6. Baseline query profile: `r32uint`.
7. Preferred 64-bit profile: `rg32uint`.
8. Fallback profile: two `r32uint` attachments if `rg32uint` is absent.
9. Avoid `rgb*` query targets.
10. First transparency support: opaque/depth-tested frontmost query only.
11. First volume support: slice query only.
12. Deferred volume defaults: MIP returns max-intensity sample; DVR/composite returns first
    opacity-threshold crossing with configurable threshold.
13. Source layout: generic code under `src/scene/query/`; visual policy under
    `src/scene/visuals/<family>/query.c`.


## Stage 0 - Guardrails And Mechanical Prep

Owner: coordinator or one small worker.

Status: landed in `87c91a15a`.

Tasks:

1. Add or update source checks that can later forbid visual-family internals in `src/scene/query/`.
2. Add a temporary TODO marker in `request_execute.c` pointing to the new plan if that file remains
   during migration.
3. Confirm `git status --short` is clean or identify unrelated user changes before editing.
4. Keep docs and tests updated at every stage.

Validation:

1. `git diff --check`
2. no build required for pure guardrail docs/checklist changes unless scripts are added.


## Stage 1 - DRP2 And Capability Foundation

Suggested subagent: DRP2/capability agent.

Status: partially landed. Scene capability fields and app population exist; DRP2/WebGPU integer
format and fixture parity still need work. `rg32uint` byte sizing is now present in DRP2 C
validation.

Primary ownership:

1. `include/datoviz/drp2/*`
2. `src/drp2/*`
3. `spec/drp2/*`
4. `include/datoviz/scene/types.h`
5. capability population points in `src/app/` and scene test helpers

Tasks:

1. Introduce a logical Datoviz/DRP2 texture format vocabulary or wrapper so query design is not
   architecturally tied to raw `VkFormat`.
2. Complete scene/runtime capability usage for supported render-target formats, supported texture
   formats, query profiles, min texture-copy row-pitch alignment, and max readback size.
3. Add DRP2 fixtures for:
   - `r32uint` render target write plus readback,
   - `rg32uint` render target write plus readback,
   - two `r32uint` attachments plus readback,
   - negative unsupported-format capability.
4. Add WGSL preflight where possible for integer render-target outputs.
5. Keep Vulkan-specific mapping inside Vulkan/DRP2 implementation code.

Validation:

1. `just spec-check`
2. focused `just test drp2`
3. any WebGPU preflight target already used by the repo if touched.


## Stage 2 - FramePlan Query Readback

Suggested subagent: scene frame-plan agent.

Status: partially landed in `566b13f45`. Metadata is represented and emitted; multiple query
outputs and full runtime origin/depth/attachment execution remain.

Primary ownership:

1. `include/datoviz/scene/frame_plan.h`
2. `src/scene/frame_plan.c`
3. `src/scene/frame_plan_emit.c`
4. `src/scene/render_pass.c`
5. `src/scene/frame_plan_fixture.c`
6. `src/scene/tests/frame_plan*.c`

Tasks:

1. Extend DRP2/runtime execution so every `DvzFramePlanCopyDesc` field is honored, including source
   origin, depth, attachment index, row pitch, destination offset, and request/readback id.
2. Remove remaining hardcoded `1x1`, `4`, `rows_per_image=1` assumptions from old probe/query paths.
3. Support multiple query output attachments/readbacks in one query pass.
4. Keep existing simple copies working during migration.
5. Add fixture-mode coverage for non-4-byte query payloads.

Validation:

1. `just build`
2. focused `just test frame_plan`
3. `just test scene` if frame-plan tests are not separately filterable.


## Stage 3 - Query Core

Suggested subagent: scene query core agent.

Status: partially landed. Public structs/API, native query queues, result polling, registry, source
guard, and a retained request-executor adapter exist. Family-owned native execution, readback
decode, and old-path replacement remain.

Primary ownership:

1. `src/scene/query/`
2. private query headers
3. `src/scene/request_queue.c` or successor queue files
4. public/private interaction request structs
5. result queues and freshness tests

Tasks:

1. Split native query execution out of `src/scene/query/queue.c` into `execute.c`, `readback.c`, and
   result helpers as needed.
2. Replace the request-executor adapter with family-owned plan builders and decoders.
3. Use the visual-family query registry to route execution instead of old pick/probe dispatch.
4. Complete generic execution that:
   - resolves panel coordinates,
   - orders panel visuals,
   - checks generic visibility and controller state,
   - asks registered family ops for eligibility,
   - invokes family plan builders,
   - executes DRP2 readback,
   - calls family decode/readout,
   - finalizes public result status.
5. Ensure generic query core contains no visual-family internals.
6. Invert the migration direction: old pick/probe shims should call query once native query
   execution exists.

Validation:

1. source lint for generic query purity,
2. focused query queue/freshness tests,
3. stale request tests,
4. no-capable/unsupported/failure status tests.


## Stage 4 - Simple Visual Families

Suggested subagent: simple visual query agent.

Primary ownership:

1. `src/scene/visuals/point/query.c`
2. `src/scene/visuals/pixel/query.c`
3. `src/scene/visuals/marker/query.c`
4. `src/scene/visuals/sphere/query.c`
5. `src/scene/visuals/segment/query.c`
6. `src/scene/visuals/path/query.c`
7. `src/scene/visuals/primitive/query.c`

Tasks:

1. Move existing item-id behavior out of `request_execute.c`.
2. Switch from RGB24 item-id encoding to selected query profile payloads.
3. Avoid query-time CPU geometry generation as the long-term path. Where the first migration must
   reuse existing CPU-expanded caches, mark it temporary and move toward rendering GPU caches.
4. Keep item/group/link-key result mapping.
5. Make marker exactness explicit if the first pass remains proxy-like.

Validation:

1. point/pixel/marker/sphere item query tests,
2. segment/path item or group query tests,
3. primitive item/primitive id query tests,
4. no CPU fallback tests for forced readback failure.


## Stage 5 - Image And Labels

Suggested subagent: image/labels query agent.

Primary ownership:

1. `src/scene/visuals/image/query.c`
2. `src/scene/visuals/labels/query.c`
3. image/labels shaders as needed
4. image/labels examples and tests

Tasks:

1. Convert image probe/pick into one image query path.
2. Return texel/data coordinate and displayed RGBA from GPU query.
3. Remove example-side coordinate flipping and manual pick/probe composition.
4. Convert labels probe to GPU-returned integer label id.
5. Remove CPU retained-field sampling from labels query.
6. CPU may map returned label id to category label after GPU result.
7. Add unsupported statuses for label formats or query profiles the runtime cannot support.

Validation:

1. image texel/readout query tests,
2. transparent/background image miss policy tests,
3. labels signed/unsigned high-id tests,
4. forced GPU readback failure yields failure, not CPU result,
5. examples using image/labels query no longer compose pick/probe manually.


## Stage 6 - Mesh And Volume

Suggested subagent: mesh/volume query agent.

Primary ownership:

1. `src/scene/visuals/mesh/query.c`
2. `src/scene/visuals/volume/query.c`
3. mesh/volume query shaders and metadata buffers
4. mesh/volume tests

Tasks:

1. Mesh query returns visual, instance, face/primitive, optional group/region, optional barycentric or
   hit position.
2. Do not rely solely on backend primitive id; provide explicit GPU metadata where needed.
3. Volume slice query returns UVW, voxel/sample id, and sampled value from GPU.
4. Remove CPU volume ray/box and CPU sampled-field query path.
5. Return unsupported for DVR/MIP/composite until exact GPU semantics land.
6. Record MIP and DVR policy tests as skipped/deferred only if the test framework supports that
   clearly; otherwise keep them as TODO comments in the implementation plan.

Validation:

1. mesh face/primitive query tests,
2. volume slice query tests,
3. forced readback failure tests,
4. unsupported status tests for volume render modes not yet queryable.


## Stage 7 - Public API Migration

Suggested subagent: API/tests/examples agent.

Status: partially landed. Query API, selection adapter, and pinned readout adapter exist. Examples,
tests, and old public pick/probe removal remain.

Primary ownership:

1. `include/datoviz/scene/interaction.h`
2. `include/datoviz/scene/types.h`
3. `src/scene/interaction.c`
4. examples using pick/probe
5. `src/scene/tests/pick_probe.c` and successor tests

Tasks:

1. Keep refining the public query API only as needed by native execution.
2. Convert examples from pick/probe to query.
3. Expand selection and pinned readout tests around native query results.
4. Remove or privatize public pick/probe entry points.
5. Rename tests from pick/probe when their behavior is now query-specific.
6. Keep migration notes in specs, not legacy `docs/`.

Validation:

1. `just build`
2. `just test scene`
3. focused example smoke for hover query, image/labels readout, and selection.


## Stage 8 - Cleanup And File Split

Suggested subagent: cleanup agent after feature agents land.

Primary ownership:

1. `src/scene/request_execute.c`
2. `src/scene/probe_plan.c`
3. broad visual files touched by query policy
4. CMake source globs/layout if subdirectories need explicit inclusion

Tasks:

1. Delete obsolete pick/probe execution code.
2. Split remaining long files where useful.
3. Move query-specific shaders into clear names.
4. Ensure CMake includes new subdirectories.
5. Keep comments, updating rather than deleting them.
6. Remove stale active plans or rewrite completed record under `agents/done/` once the overhaul lands.

Validation:

1. `git diff --check`
2. `just build`
3. `just test scene`
4. `just test drp2` if DRP2 code still changed
5. `just spec-check`


## Parallel Subagent Map

Use subagents only when the user explicitly asks for parallel agent work. If they do, split work this
way:

1. **DRP2/capability agent**
   - Owns logical formats, capability snapshot, DRP2 fixtures, and integer render-target tests.
2. **FramePlan agent**
   - Owns query copy/readback node shape and scene-to-DRP2 conversion.
3. **Query core agent**
   - Owns `src/scene/query/`, queue/freshness/result, registry, and source lint.
4. **Simple visual agent**
   - Owns point/pixel/marker/sphere/segment/path/primitive query family ports.
5. **Image/labels agent**
   - Owns image and labels query paths and example migration for image/labels readout.
6. **Mesh/volume agent**
   - Owns mesh face/region query and volume slice GPU query.
7. **API/tests agent**
   - Owns public API migration, selection/readout adapters, examples, and broad test rename/update.

Workers are not alone in the codebase. Give each worker a disjoint write set. Do not let two workers
edit the same file unless the coordinator explicitly serializes that integration.


## Next Implementation Slice Recommendation

The best next code slice is family-owned extraction from the retained request-executor adapter:

1. Split `src/scene/query/queue.c` so queueing, execution, result conversion, and readback helpers
   are separate files.
2. Add family callbacks to `DvzSceneQueryFamilyOps` for match/eligible/build/decode/readout.
3. Move pixel, then point, out of `src/scene/request_execute.c` into
   `src/scene/visuals/pixel/query.c` and `src/scene/visuals/point/query.c`.
4. Replace RGB24 identity payloads with the `r32uint` baseline.
5. Keep image probe on the adapter only until its family-owned GPU value path is split out.
6. Keep labels and volume sample query explicitly unsupported until their GPU family paths exist.
7. Add forced readback-failure tests proving no CPU fallback result appears.
8. After family-owned query works, make old pick/probe APIs private shims or remove them.

That slice removes the main architectural ambiguity that remains: native query owns the public queue,
but the actual GPU rendering/readback code is still mostly in `request_execute.c`.


## Risk Register

1. **Format support:** `rg32uint` appears broadly supported on Vulkan reports, but do not make it the
   only path. Keep `r32uint` baseline and two-attachment fallback.
2. **WebGPU:** avoid Vulkan-only primitive-id and raw `VkFormat` contracts.
3. **CPU fallback regression:** tests must force GPU/readback failure and verify no CPU result appears.
4. **Volume semantics:** slice is clear; MIP and DVR/composite must remain explicit deferred policies
   until implemented on GPU.
5. **Transparency semantics:** opaque/depth-tested frontmost query first; blended/WBOIT/depth-peel need
   explicit later profiles.
6. **Generic file pollution:** enforce source lint early, or visual-specific logic will creep back into
   query core.
7. **Long-file churn:** `request_execute.c` is large; split by stages and keep behavior tests running.


## Done Criteria

The overhaul is complete when:

1. public scene interaction uses query request/result as the primary API,
2. old public pick/probe APIs are removed or clearly private transitional shims,
3. generic query code has no visual-family internals,
4. visual-family query code lives under `src/scene/visuals/<family>/`,
5. rendered visual queries have no CPU fallback,
6. labels and volume slice queries no longer sample retained CPU data,
7. DRP2/FramePlan supports non-4-byte integer query payload readback,
8. capability failures produce explicit unsupported results,
9. examples use one panel query instead of manual pick/probe composition,
10. focused scene and DRP2 tests pass.

Current status: criteria 1, 3, and 4 are partially satisfied by the foundation/native-queue commits.
Criterion 5 is partially satisfied for native query because volume CPU probing is blocked there, but
labels and family-owned GPU paths remain open. Criteria 2 and 6-9 remain open.
