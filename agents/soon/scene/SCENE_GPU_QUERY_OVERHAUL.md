# Scene GPU Query Overhaul

> **Execution Status**
> - **Status:** `IN PROGRESS - LABELS GPU QUERY RENDERED`
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

1. `src/scene/query/queue.c`: native query queue ownership and figure-level processing entry point.
2. `src/scene/query/execute.c`: generic native query orchestration across visual-family ops.
3. `src/scene/query/executor.c`: retained DRP2 query executor lifecycle.
4. `src/scene/image_query_plan.c`: synthetic image sample query frame-plan construction.
4. `src/scene/render_pass.c`: scene-to-DRP2 converter now forwards `DvzFramePlanCopyDesc` metadata
   for supported DRP2 fields, but runtime copy selection is still tied to the current color target.
5. `include/datoviz/scene/frame_plan.h`: copy metadata exists; multi-output query integration and
   full DRP2 origin/depth/attachment execution remain.
6. `include/datoviz/scene/types.h`: query request/result/capability fields exist.
7. `include/datoviz/scene/interaction.h`: public query API is the only panel query request API.
8. `src/drp2/serialization.c` and `spec/drp2/schema/common/GPUTextureFormat.json`: already contain
   `r32uint` and `rg32uint`, useful for query profiles.
9. `spec/drp2/fixtures/positive/pressure_picking_readback.json`: existing `r32uint` readback fixture
   that should be expanded into runtime and scene-facing query tests.


## Landed Foundation - 2026-05-27

Committed implementation slices:

1. `87c91a15a Add scene query migration guardrails`
   - source guard for generic query purity,
   - CMake inclusion for `src/scene/query/` and `src/scene/visuals/*/`,
   - initial migration marker, later removed with the legacy request executor.
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
13. `e9c3befe4 scene: stop query pick adapter fallback`
    - native query no longer falls back through public pick queues.
14. `78f09486e scene: remove unused query pick adapter`
    - removed the unused native-query-to-pick adapter surface.
15. `6d53c33de scene: stop native query probe adapter fallback`
    - native query no longer falls back through public probe queues.
16. `5c66418da scene: trim unused query ops fields`
    - simplified the family operation contract around the callbacks actually used by native query.
17. `efac8cbab scene: add native image sample queries`
    - image pixel/sample value queries now return native query results.
18. `2273c35c4 scene: cover image sample query readback failure`
    - forced image readback failures now produce query failures instead of CPU results.
19. `bdf3e9c68 scene: fill native query framebuffer position`
    - query results carry framebuffer coordinates.
20. `a24b87155 scene: expose native query pending executor`
    - legacy shims can reuse the native query executor directly.
21. `eb0bdd118 scene: honor panel order in native queries`
    - native query candidate traversal follows panel visual order.
22. `fdc1b761d scene: route legacy picks through native query`
    - old public pick requests are now query shims.
23. `eecd91546 scene: remove legacy pick executor bridge`
    - removed the old pick plan/resolve code from `request_execute.c`.
24. `6dfa2bf93 scene: route labels probes through native query`
    - old labels probe requests are now query shims over family-owned labels query execution.
25. `25b65c244 scene: route image probes through native query`
    - old image probe requests are now query shims over family-owned image query execution.
26. `12542e336 examples: use scene query for pick hover`
    - `pick_hover` now queues item queries and applies selection from `DvzQueryResult`.
27. `d10429dbc examples: use scene query for image probe`
    - `image_probe` now queues pixel queries and builds pinned readouts from query results.
28. `c936aca30 examples: use scene query for labels showcase`
    - labels showcase hover/click selection now uses segment queries.
29. `bf9fb4149 examples: use scene query for rich card probe`
    - rich-card overlay demo now consumes pixel query results.
30. `aa098676d examples: use scene query in scheduler lab`
    - scheduler lab now uses item and pixel queries instead of pick/probe requests.
31. `c61026aa4 scene: make selection readouts query native`
    - selection cards and pinned readouts now retain/use `DvzQueryResult` directly.
32. `scene: remove legacy pick probe request API`
    - removed public pick/probe functions and result types,
    - deleted `request_execute.c` and `request_queue.c`,
    - moved retained query executor lifecycle under `src/scene/query/executor.c`,
    - app rendering now processes native query queues directly.
33. `scene: remove dead query probe compatibility flag`
    - removed the private `DVZ_SCENE_QUERY_FLAG_COMPAT_PROBE` path left behind after public probe
      deletion,
    - query eligibility now always uses native visual capabilities.
34. `scene: rename query scratch internals`
    - renamed the remaining `DvzSceneProbePlan` scratch container and `probe_plan.c` file to query
      terminology,
    - renamed internal scratch buffers from `pick_*`/`probe_*` to `query_*`.
35. `scene: rename visual query capabilities`
    - replaced the public `dvz_visual_set_pick_capabilities()` API with
      `dvz_visual_set_query_capabilities()`,
    - renamed `DVZ_PICK_CAPABILITY_*` and visual `pick_capabilities` internals to query capability
      terminology.
36. `scene: remove CPU point pick fallback`
    - removed the unused CPU point-picking helper and `DVZ_PICK_TRACE` hook,
    - renamed the remaining panel-coordinate helper to query terminology.
37. `scene: rename native query tests`
    - renamed the old `pick_probe.c` scene test wrapper and app steady-state tests to query names,
    - kept a temporary historical validation alias until the query naming cleanup.
38. `scene: render labels queries on GPU`
    - added signed/unsigned labels query shaders that write encoded `r32uint` label ids,
    - labels query now builds a rendered one-pixel FramePlan readback instead of a direct retained
      field copy/readback,
    - CPU still maps the GPU-returned id to category label text after decode.
39. `scene: harden labels query failures`
    - labels query eligibility now rejects visuals without an integer label field,
    - tests cover missing labels field and forced labels readback failure without CPU fallback.
40. `scene: drop dead query texel fields`
    - removed unused `DvzSceneQueryPlan` texel-coordinate fields left behind after direct CPU
      labels sampling was deleted.
41. `scene: add native volume slice sample query`
    - scalar slice-mode volume sample query now renders a GPU `r32uint` payload and decodes it as a
      scalar query value,
    - MIP/composite, integer texture, and RGBA volume sample policies remain unsupported rather than
      falling back to CPU.
42. `scene: harden volume sample query tests`
    - tests cover deferred MIP policy rejection, unsupported integer volume sample format rejection,
      and forced GPU readback failure without CPU fallback.
43. `scene: add rg32 volume sample query profile`
    - requested `DVZ_QUERY_PROFILE_U64_RG32` volume slice sample queries render an `rg32uint`
      payload containing scalar value and packed GPU-computed UVW,
    - non-volume `rg32uint` query profiles remain explicitly unsupported.
44. `scene: decode volume sample voxel id`
    - requested `rg32uint` volume slice sample queries now derive `raw_id`, `resolved_id`,
      `voxel_id`, and `texel_id` from the GPU-computed UVW using retained axis order and flips.
45. `scene: drop packed rgba query profile`
    - removed the unimplemented packed RGBA query profile and capability flag so query execution
      fails explicitly when `r32uint`, `rg32uint`, or two-attachment `r32uint` profiles are absent.
46. `scene: cover unsupported query profiles`
    - added CPU query tests for missing profile support and family-level requested-profile rejection.
47. `scene: cover labels query format rejection`
    - added CPU query coverage for labels visuals with unsupported non-integer label field formats.
48. `drp2: cover rg32uint query readback`
    - added an `rg32uint` DRP2 fixture that copies a 1x1 query payload with an 8-byte row pitch,
    - added a vklite runtime test that renders two `u32` words to `rg32uint` and verifies the
      downloaded 8-byte payload.
49. `scene: defer automatic 2xr32 query selection`
    - stopped default profile selection from choosing `DVZ_QUERY_PROFILE_U64_2XR32` until true
      two-attachment query execution/readback exists,
    - added CPU query coverage for the "only 2xr32 is advertised" case.
50. `669eec431 scene: rename query hit policy API`
    - renamed the public hit-policy type, constants, setter, and interaction field to query
      terminology.
51. `4d95a4e4e scene: rename query readback fixture ids`
    - renamed old test-only `request.pick.*`, `buf.pick.*`, and `target.*.picking` resource ids to
      query ids while leaving the low-level picking render-pass/shader mode untouched.
52. `91a9fb9d2 scene: make query tests canonical`
    - removed the old `pick-probe` test-group alias,
    - made `scene/query` the canonical focused group,
    - tightened query-core guardrails against reintroducing pick/probe names.
50. `scene: add sparse lookup for label volumes`
    - signed/unsigned label-volume slice and composite rendering now use a sparse GPU categorical
      lookup buffer for large or signed semantic ids,
    - label-volume sample queries now return raw `r32uint` label bits, preserving `UINT32_MAX` and
      signed `-1`,
    - tests cover sparse unsigned rendering, signed lookup upload/binding, high unsigned label
      query, and signed negative label query.

Recorded validation after these commits:

1. `just build`
2. `just spec-check` with `124/124` DRP2 fixtures
3. `direnv exec . just test test_drp2_runtime_vklite_draws_rg32uint_readback`
4. `just test test_scene_query_does_not_auto_select_2xr32_profile`
5. `direnv exec . just test query` with `40/40`
6. `direnv exec . just test scene` with `478/478`
7. `git diff --check`


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
2. Remove temporary migration markers once the legacy request executor is gone.
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
   - `rg32uint` render target write plus readback. Landed.
   - two `r32uint` attachments plus readback, still deferred.
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

Status: mostly landed for the current GPU-backed families. Public structs/API, native query queues,
result polling, registry, source guard, split execution/readback/result helpers, family-owned build
and decode callbacks, and legacy pick/probe shim inversion exist. Volume slice remains intentionally
outside native query until a GPU family path lands.

Primary ownership:

1. `src/scene/query/`
2. private query headers
3. native query queue/result files
4. public/private interaction request structs
5. result queues and freshness tests

Tasks:

1. Keep native query execution split across `queue.c`, `execute.c`, `readback.c`, and `result.c`.
2. Keep family-owned plan builders and decoders as the execution boundary.
3. Use the visual-family query registry to route execution instead of old pick/probe dispatch.
4. Continue hardening generic execution that:
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

1. Keep item-id behavior in visual-family query implementations.
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

Status: image sample/item query and labels segment query are native. Labels now renders a GPU
`r32uint` label-id payload; remaining work is hardening unsupported/failure reporting and avoiding
temporary CPU-expanded geometry where the renderer already owns a GPU cache.

Primary ownership:

1. `src/scene/visuals/image/query.c`
2. `src/scene/visuals/labels/query.c`
3. image/labels shaders as needed
4. image/labels examples and tests

Tasks:

1. Convert image probe/pick into one image query path.
2. Return texel/data coordinate and displayed RGBA from GPU query.
3. Remove example-side coordinate flipping and manual pick/probe composition.
4. Convert labels probe to GPU-returned integer label id. Landed.
5. Remove CPU retained-field sampling from labels query. Landed for hit/value payloads.
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

Status: mesh identity query is native. Volume item query uses proxy geometry identity, and scalar
slice-mode sample query is now GPU-backed through rendered `r32uint` and requested `rg32uint`
payloads. Label-volume slice sample query returns raw signed/unsigned label ids through the
baseline 4-byte `r32uint` payload. The `rg32uint` profile adds GPU-computed UVW and derives
voxel/sample ids from that coordinate. Displayed RGBA and non-slice policies remain deferred.

Primary ownership:

1. `src/scene/visuals/mesh/query.c`
2. `src/scene/visuals/volume/query.c`
3. mesh/volume query shaders and metadata buffers
4. mesh/volume tests

Tasks:

1. Mesh query returns visual, instance, face/primitive, optional group/region, optional barycentric or
   hit position.
2. Do not rely solely on backend primitive id; provide explicit GPU metadata where needed.
3. Volume slice query returns UVW, voxel/sample id, and sampled value from GPU. Current `rg32uint`
   support returns scalar plus GPU UVW and derives the id after readback; current label-volume
   `r32uint` support returns raw signed/unsigned category ids.
4. Remove CPU volume ray/box and CPU sampled-field query path.
5. Return unsupported for DVR/MIP/composite until exact GPU semantics land.
6. Record MIP and DVR policy tests as skipped/deferred only if the test framework supports that
   clearly; otherwise keep them as TODO comments in the implementation plan.
7. Extend the volume query payload beyond scalar plus UVW-derived ids to include displayed RGBA
   once multi-output query payload support is available.

Validation:

1. mesh face/primitive query tests,
2. volume slice query tests,
3. forced readback failure tests,
4. unsupported status tests for volume render modes not yet queryable.


## Stage 7 - Public API Migration

Suggested subagent: API/tests/examples agent.

Status: partially landed. Query API, family execution, selection/readout query ownership, and public
example migration exist. Old public pick/probe APIs and bridge files have been removed.

Primary ownership:

1. `include/datoviz/scene/interaction.h`
2. `include/datoviz/scene/types.h`
3. `src/scene/interaction.c`
4. examples using query
5. `src/scene/tests/query.c`

Tasks:

1. Keep refining the public query API only as needed by native execution.
2. Keep examples on query. Current `examples/c` no longer calls public pick/probe APIs.
3. Expand selection and pinned readout tests around native query results.
4. Keep tests query-named when their behavior is query-specific.
5. Keep migration notes in specs, not legacy `docs/`.

Validation:

1. `just build`
2. `just test scene`
3. focused example smoke for hover query, image/labels readout, and selection.


## Stage 8 - Cleanup And File Split

Suggested subagent: cleanup agent after feature agents land.

Primary ownership:

1. `src/scene/query/queue.c`
2. `src/scene/query/execute.c`
3. `src/scene/query/executor.c`
4. `src/scene/image_query_plan.c`
5. broad visual files touched by query policy
6. CMake source globs/layout if subdirectories need explicit inclusion

Tasks:

1. Keep only native query queues/results.
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

The next agent should start from the now-covered single-attachment integer readback path and choose
one of these narrow slices:

1. Add displayed RGBA to GPU volume slice queries only after choosing the payload shape. The current
   `rg32uint` profile is already scalar plus UVW, so displayed RGBA likely needs a wider payload,
   multi-output query support, or a separate readout pass.
2. If two-word fallback support is needed before displayed RGBA, implement true
   `DVZ_QUERY_PROFILE_U64_2XR32` execution end to end: two render attachments, two copy/readback
   records, family decode support, and DRP2/FramePlan tests. Until that lands, the query core must
   not auto-select this profile.
3. Keep MIP and DVR/composite volume sample queries explicitly unsupported until exact GPU semantics
   are specified and implemented.

Do not start with broad `pick`/`picking` renames. Some of that terminology still describes the
FramePlan render-pass role, and a broad rename is more likely to create churn than improve query
behavior.


## Risk Register

1. **Format support:** `rg32uint` appears broadly supported on Vulkan reports, but do not make it the
   only path. Keep `r32uint` baseline and two-attachment fallback.
2. **WebGPU:** avoid Vulkan-only primitive-id and raw `VkFormat` contracts.
3. **CPU fallback regression:** tests must force GPU/readback failure and verify no CPU result appears.
4. **Volume semantics:** slice is clear; MIP and DVR/composite sample-query semantics must remain
   explicit deferred policies until implemented on GPU. Label-volume composite rendering is active,
   but composite query semantics are not.
5. **Transparency semantics:** opaque/depth-tested frontmost query first; blended/WBOIT/depth-peel need
   explicit later profiles.
6. **Generic file pollution:** enforce source lint early, or visual-specific logic will creep back into
   query core.


## Done Criteria

The overhaul is complete when:

1. public scene interaction uses query request/result as the primary API,
2. old public pick/probe APIs are removed or clearly private transitional shims,
3. generic query code has no visual-family internals,
4. visual-family query code lives under `src/scene/visuals/<family>/`,
5. rendered visual queries have no CPU fallback,
6. labels, scalar volume slice, and label-volume slice sample queries no longer sample retained CPU
   data,
7. DRP2/FramePlan supports non-4-byte integer query payload readback,
8. capability failures produce explicit unsupported results,
9. examples use one panel query instead of manual pick/probe composition,
10. focused scene and DRP2 tests pass.

Current status: criteria 1, 2, 3, 4, 5, 6, 8, and 9 are substantially satisfied by the native query,
family-execution, public API removal, example-migration, labels GPU readout, scalar volume slice GPU
query, and label-volume raw-id query commits. Criterion 7 remains open for broader non-4-byte
integer readback profile coverage.
