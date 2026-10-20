# Interactive Mesh Performance Handoff

Status: active non-blocking v0.4-final measurement work and preserved early-v0.5 implementation sequence. Updated: 2026-10-18.

This handoff records execution evidence and task order for large semantically partitioned meshes. Durable query behavior lives in [GPU_QUERY_SYSTEM.md](../../spec/scene/interaction/GPU_QUERY_SYSTEM.md), asynchronous completion in [ASYNC_QUERY_EXECUTION.md](../../spec/scene/proposals/future/ASYNC_QUERY_EXECUTION.md), mesh-part state in [MESH_PART_STATE.md](../../spec/scene/proposals/future/MESH_PART_STATE.md), and embedded presentation in [GUI_VIEWPORT_PRESENTATION.md](../../spec/architecture/GUI_VIEWPORT_PRESENTATION.md).

## Opt-in Nonblocking GUI Presentation — October 18 2026

Scientific candidate `2644ca68e` adds `DVZ_GUI_VIEWPORT_FLAGS_NONBLOCKING` to the native Dear ImGui viewport configuration. Strict same-frame presentation remains the default. The implementation uses three lazy canvas-owned targets, checked source and host timeline ownership, a display pin, resize request generations, immutable per-slot descriptors, bounded admission, and last-compatible-completed-image promotion. Source demand, resize and pending completion drive host scheduling; a visible idle viewport no longer requests frames indefinitely. Worker posting wakes the native event wait without traversing mutable GUI registries. A source has one viewport consumer; advanced borrowed sources opt in before their first submission. The consumer dependency baseline and pin remain unchanged.

Overlap required underlying runtime safety fixes: borrowed uploads are recorded into their owner command buffer, shared resources retire only after the canvas proves completion, logical slot scopes stay bounded across resize, abandoned-submit recovery drains earlier MAIN work only on the error path, and queries execute after a successful source submission before the user frame callback. Failed GPU submission rebuilds consumed retained upload state before retry. Runtime reset invalidates realized derived caches without creating permanent dirty state on unrelated visual families. Independent high-effort Sol 6.1 review found no remaining architecture blocker. A negative control removing unsent-frame recovery fails the actual rendered-pixel assertion; restoring recovery passes with synchronization validation.

The final Linux Debug candidate passes all 1,314 native tests with synchronization and submit-time validation explicitly enabled, with no failures or skips. Coverage includes source/host gates, bounded three-slot admission, obsolete resize completion, submit/recovery failures, displayed-image retention, idle and worker wakeup, two-source progress, resize/hide/show, callback destruction, borrowed upload/copy/compute ordering, retirement allocation failure, same-frame query/callback order and three-scope resize resource stability. Generated bindings and ABI checks pass. The Python/tooling suite passes 494 tests with one skip; specification/source guards, shaderc smoke, 30-frame presentation checks, Quickstart, generated documentation, strict documentation build and status checks pass. The paired IBL candidate suite passes 1,105 tests with one optional registered-asset skip. A separate final 120-frame real-data nonblocking rotation run explicitly enables the Vulkan validation layer, synchronization validation and submit-time checking; model change and callback checks pass with no reported validation errors. These Linux checks do not replace macOS interaction acceptance or exact-head remote CI.

Matched desktop review-2 rotation uses D070/full 50 um slices, six major context surfaces, 2240×1600 host, 1534×1385 source, UI scale 1.5, CPython 3.11 and the same Debug library on RTX 5090/driver 595.99.02. Fresh-process strict/nonblocking/nonblocking/strict order, three 240-frame trials per process, gives six samples per mode. Actual arcball model changes and callback health are checked. Validation is disabled only for timing and trace measurements. Median host time is 4.583 ms strict versus 1.557 ms nonblocking (66.0% lower); median completed source throughput is 218 versus 321 images/s, and nonblocking promotion throughput is approximately 317 images/s. Nonblocking submits approximately 120–121 source images per 240 host iterations, coalescing input while reusing completed images. This is host and source progress evidence, not display refresh rate or input-to-photon latency.

Separate three-trial memory checks remain stable: strict GPU process memory is 232 MiB and nonblocking is 300 MiB; process RSS stays approximately 710–711 MiB in both modes. The extra 68 MiB is a measured bounded cost of buffered targets and scoped intermediates at this resolution, not a universal budget. Canvas and scene tests establish bounded resource counts across slot reuse and resize.

Vulkan/NVTX diagnostics confirm the intended wait removal: strict rotation records 480 timeline waits totaling 527.889 ms over 240 host iterations; nonblocking records no timeline waits and no queue-idle calls. Each app run has one final device-idle drain. The first overlapping run lazily creates one additional slot and its intermediates; this is bounded startup work, not per-frame allocation. A second 240-frame rotation range has no Vulkan resource creation, allocation or destruction calls, no timeline waits and no queue-idle calls. GPU intervals overlap CPU execution and must not be added to host wait times. Consumer receipts stay ignored under `build/review2-rotation/`; native receipts stay under the candidate build tree. Measured library SHA256: `b6a6bb2742fd60ab12c91853cea5695c4f5754896dd29d0784808489418aa46e`.

Keep this policy opt-in pending manual review of image freshness, scientific selection/query expectations and macOS behavior. Exact-head native publication and the remote Linux/macOS/Windows CI workflow still require the repository's explicit final-content publication approval. Panel-level reuse remains a separate possible optimization; this work does not cache unchanged slice panels independently of the 3D source canvas.

## Scientific Candidate Integration And Remaining Cost — October 18 2026

Scientific branch `fix/arcball-viewport-orbit` integrates main through merge `84f65c0ff`; `eb0b6794a` adds an exclusive CPU regression combining dirty indexed state with a later failed scalar LUT allocation. Failed emission preserves consumed bank identity/revision and volume realization; retry emits the sparse bank update and corrected LUT; the next unchanged emit emits neither. An independent high-effort Sol 6.1 audit found no integration blocker. Existing candidate navigation, indexed-state and fragment-coverage APIs are retained.

The merged candidate builds, passes generated-binding freshness/policy/ABI checks, 113 runtime-vklite checks with explicit synchronization/submit-time validation, 644 scene CPU checks, 5 indexed-state checks, 4 fragment-clip checks, 17 WBOIT checks and the scalar-transfer check. The new combined retry test passes separately. Against the explicitly paired candidate facade/library, the IBL consumer passes 1,102 tests with one optional registered-asset test skipped. The consumer dependency pin and configured baseline remain unchanged.

Fresh 240-frame Vulkan/NVTX rotation capture on the real X11 Debug candidate measures 4.460 ms/frame with performance-only validation disabling. Source completion accounts for 488.995 ms across 240 waits (2.037 ms/frame), while another 240 timeline waits total only 0.149 ms. GPU workload union is 1.938 ms/frame and overlaps the host waits. No queue-idle calls or transient buffer/command-buffer allocations occur in the range; one device-idle call totals 0.136 ms. The previous per-frame broad waits and LUT staging churn are absent.

Separate two-run, 240-frame visibility diagnostics keep the panels and input trajectory: normal 4.113/4.327 ms, hidden slices 3.555/3.536 ms, hidden context meshes 2.291/2.241 ms, hidden volume 3.729/3.902 ms. These sequential ablations are indicative, not additive or controlled ABBA comparisons. Python callbacks consume 101 ms across a separate 180-frame profiled run, including replay overhead. The remaining opportunity is rendering/presentation: investigate unchanged-panel render reuse and the existing nonblocking viewport generation/fairness design while preserving explicit strict same-frame presentation. Do not infer that hiding scientifically meaningful context is an acceptable product change. The current whole-source viewport cannot independently retain slice panels when 3D rotation dirties its canvas.

Queue-role investigation confirms a separate metadata issue: family/count requests do not populate role capability/is-set metadata, and repeated same-family requests can reuse queue indices. All current production consumers explicitly request MAIN. Repairing metadata should have isolated CPU and guarded allocated-handle tests; it must not silently enable asynchronous or cross-queue consumers. Their semaphore/barrier/ownership contracts require separate design and validation.

Consumer receipts remain ignored under `build/review2-rotation/`; candidate native receipts remain under its ignored build tree. Measured library SHA256: `19d52000bdc68c4721c28668f476c00faec94c7141b707e24965224e3ea5a04b`. Manual interaction and macOS acceptance remain deferred. Native main publication and scientific-branch publication remain pending the repository's explicit approval requirement.

## Queue-Scoped Immediate Submission — October 18 2026

Immediate vklite submission no longer idles the entire device before submitting. It still waits for its selected queue afterward, preserving synchronous command-buffer and staging-buffer lifetimes. The generic helper requires caller-supplied memory barriers and explicit cross-queue synchronization; its public signature is unchanged.

Same-queue submission order alone was insufficient: a prior borrowed canvas frame can still read a buffer that an immediate DRP2 copy overwrites. Owned DRP2 command buffers now record conservative ALL_COMMANDS memory dependencies at begin and end, covering prior readers/writers and publishing writes to later device consumers on MAIN. All owned begin paths and the explicit timeline handoff end path use these helpers. Borrowed recording ownership is unchanged; host readback and cross-queue handoffs retain their separate dependencies.

Synchronization validation and submit-time checking were explicitly enabled. An intentionally unsubmitted pair of unbarriered fills verifies the active validator reports WRITE_AFTER_WRITE. An actual borrowed-read / immediate-copy / borrowed-read regression passes with known old/new bytes and no validation errors. Temporarily removing the owned barriers makes this regression report WRITE_AFTER_READ; restoring them passes. Selected-queue completion and independence from an allocated unrelated queue both pass. The complete runtime-vklite lane passes 113 tests with no failures or skips. Binding generation and freshness/policy/ABI checks pass; no public API or binding output change remains. A high-effort Sol 6.1 implementation/audit and independent GUI-owner review found no remaining blockers.

The independent-queue test uncovered a separate pre-existing metadata defect: device queue allocation retains family/index/handle but does not initialize role capability flags, so optional role lookup can fail despite allocated queues. The regression enumerates actual allocated handles; production queue-role behavior is unchanged. Repairing that metadata needs separate caller and cross-queue validation rather than silently enabling new queue choices in this patch.

## Scoped Strict GUI Viewport Completion — October 18 2026

The strict GUI viewport resolver now waits for the exact source canvas timeline value instead of idling the device. Offscreen submissions already signal that owned timeline at ALL_COMMANDS. The helper checks wait results and device loss, and the resolver also covers a source submitted earlier without new dirty work. Existing same-MAIN-queue image transitions provide device write/read dependencies; a host timeline wait proves completion rather than replacing those barriers. Resize, descriptor/image retirement and teardown retain their existing waits.

This is a synchronous scoped improvement, not the full nonblocking generation-promotion and fair-batching target in GUI_VIEWPORT_PRESENTATION. The source remains current for strict same-frame presentation. A Vulkan regression proves source completion returns while later MAIN work remains blocked, then checks reuse and resize. GUI resize/hidden, forwarded arcball and multiple-viewport input-router checks pass with synchronization validation explicitly enabled. The high-effort architecture auditor and independent DRP2 reviewer found no remaining blockers.

## Matched Consumer Validation And Timings — October 18 2026

The full offscreen canvas subset passes 12 tests, including resize, failed sink submission, device-loss state, monotonic completion and source-scoped completion. Three focused GUI checks pass: resize/hidden, forwarded arcball and multiple viewport input routers. Together with the 113-test runtime lane and deliberate synchronization-validation activation/ablation controls, these provide exercised-path Vulkan evidence rather than a claim of universal or cross-platform correctness. Existing unrelated shader-helper declaration warnings remain in unchanged scene runtime tests; no new warnings were introduced in the changed translation units.

The real X11 review-2 replay now starts through the standard work-area-fitting path; earlier forced-unfitted startup stalls are excluded. The matched comparison uses an isolated scientific consumer based on `af1ac065f` with the prior LUT optimization in both builds and the new queue/GUI synchronization changes only in the after build. Both use Debug, Python 3.11.15, NVIDIA RTX 5090/595.99.02, D070/full 50 um slices/six context surfaces, 2240×1600 host dimensions, UI scale 1.5 and identical 1614×1385 source-viewport dimensions. An ABBA order uses two fresh processes per state, three 240-frame replays per process; every replay verifies actual arcball model changes and successful callbacks. These are bounded host-frame measurements, not input-to-photon latency or portable FPS claims.

With validation explicitly disabled for a separate performance experiment, mean frame time changes from 4.449 to 4.177 ms, a 6.1% reduction. With synchronization and submit-time validation explicitly enabled, it changes from 6.567 to 6.758 ms, 2.9% slower. Keep these modes separate; do not claim a validation-enabled speedup or infer a Release-build result. No Vulkan hazards appear in the corrected validation-enabled replay logs. The before library SHA256 is `c5e0b8acfcce37df4ecbb029f1361e1faa31070d70c6a23cdf557819dc49a`; after is `4fca13c1655fff9eb3c27b069826af69f6f196fca66eedf9c28919183ebd472f`. Ignored consumer receipts and comparison JSON remain under `build/review2-rotation/` in the consumer checkout; native test and ablation receipts are under `build/sync-validation/`.

The IBL dependency pin and configured baseline remain unchanged. The scientific candidate integration is recorded above; baseline promotion and manual interaction acceptance remain separate decisions. The full nonblocking GUI generation/fairness architecture and the pre-existing queue-role metadata defect remain future work.

## Unchanged Volume Transfer Uploads — October 17 2026

Volume transfer-texture preparation now follows the existing texture-dirty and volume-realized-version contract. Unchanged redraws skip both LUT rebuilding and upload emission. Alpha-stop changes, shared scale/colormap changes, field/scale binding and runtime-emitter reset still force refresh. This deliberately retains conservative invalidation for unrelated volume settings rather than introducing another cache or generation system. Scene figures share one emitter/resource namespace; new visuals and reset runtime boundaries retain initial uploads.

Transfer preparation/upload failures now propagate through internal upload helpers and abort figure emission before dirty/realized state commits. This closes a retry hazard exposed by suppressing unchanged uploads: logging a failed LUT update alone could otherwise leave an older resource while marking the new state clean. No public API, bindings, shader ABI, synchronization policy or application dependency pin changes.

Validation: the scene-semantic lane passes 596 tests; the focused volume suite passes 23 tests including GPU volume queries. Regressions cover unchanged preparation/upload suppression, alpha updates, shared colormap changes, scale rebinding, categorical palette colors and width changes, new volumes, shared figures, allocation failure, failed capability conversion, valid retries and runtime reset. A high-effort Sol 6.1 read-only architecture/implementation audit found no remaining blockers. The optional ASan scene build is unavailable because unchanged runtime-test code lacks declarations for shader-pass helpers under its Clang configuration; no sanitizer pass is claimed.

A visible scalar-volume offscreen capture and a three-frame preview sequence pass with Vulkan validation enabled; the captured volume was inspected. Real-desktop before/after timing remains unavailable: an isolated scientific-consumer pre-change build stalls in X11 window initialization before measured frames. Those attempts are excluded, and no native frame-time improvement is claimed. The consumer checkout, baseline pin and existing scientific candidate remain unchanged; the isolated validation checkout is ignored.

A separate synchronization investigation found that the synchronous command helper's production DRP2 caller submits to the main queue and already waits for that queue before destroying staging/command resources. The candidate next change is to remove its pre-submit device-wide idle while retaining post-submit queue completion, with a regression proving an independent queue can remain blocked while the selected queue completes. Audit cross-queue callers and preserve explicit semaphore/timeline dependencies before implementation. The strict GUI source-to-parent wait is a separate dependency problem and remains unchanged.

## Consumer Evidence

An IBL D070 atlas workload measured 486,674 vertices, 966,645 triangles, and 1,140 mesh components at 900 x 720 on an NVIDIA RTX 5090 using Datoviz `18b4840ff`. Five randomized fresh processes used 30 warm-up and 120 measured frames.

| Scenario | Median frame/run time | Isolated phase | Median RSS |
| --- | ---: | ---: | ---: |
| Direct populated surface | 4.14 ms | — | approximately 363 MiB |
| Complete GUI without pointer query | 7.34 ms | GUI viewport 5.12 ms | — |
| One pointer-driven FACE query per frame | 11.30 ms | Query 9.87 ms | approximately 535 MiB |
| Four pointer events per frame | 11.93 ms | Query 10.42 ms | — |

The burst workload emitted 480 pointer events but executed 120 queries, confirming latest-position coalescing within each frame. The remaining hover cost is one synchronous query per rendered pointer frame, not an event backlog. Full retained-tree construction costs less than one millisecond per frame, and a complete static 2D atlas view measures about 0.28 ms per frame; neither is a priority optimization target.

The GUI result was collected after `18b4840ff`, which prevents duplicate scheduler and GUI rendering of embedded sources. The remaining populated-viewport cost includes the source render and the device-wide wait in the strict resolution path.

## RC3 And v0.4-Final Work

These changes are useful but non-blocking and may land only as focused commits with passing validation:

1. Split query timing into build, FramePlan/DRP2 emission and validation, backend execution, download/wait, and decode/readout without adding synchronization for measurement.
2. Record first-query versus steady-state behavior, derived query vertices, upload bytes, retained resource bytes, submissions, completions, failures, superseded requests, and coalesced requests.
3. Retain deterministic synthetic large-mesh ITEM/FACE and GUI/tree/embedded-viewport benchmark workloads with machine-readable output.
4. Evaluate a mesh-only one-pixel physical query attachment while preserving logical source-pixel semantics and exact results.
5. Investigate indexed exact-face query geometry only after counters establish the current allocation and upload shape.

Ordinary CI asserts deterministic lifecycle, command, upload, allocation, freshness, and result-shape invariants rather than wall-clock thresholds. Before/after timing comparisons use fixed dimensions, warm-up, fresh processes, matched builds, matched backend/validation settings, randomized run order, and recorded hardware and commit identities.

The retained synthetic mesh-query route is:

```sh
just example-c lab/mesh_query_bench
./build/examples/c/lab/mesh_query_bench --target face --rows 697 --cols 699 --warmup 4 --queries 20
```

It reports machine-readable cold and steady phases, per-phase CPU timings, derived query vertex count, first-use upload bytes, retained query resource bytes, completed hits, and failures. Run both `item` and `face` targets when changing mesh query lowering. The 697 x 699 default produces 487,203 retained vertices and 971,616 triangles, close to the measured atlas shape without depending on consumer data.

The retained GUI/tree/viewport route is:

```sh
just example-c lab/gui_viewport_bench --profile combined --warmup 16 --frames 120
```

Run fresh `empty`, `tree`, `viewport`, and `combined` processes in randomized order. Tree profiles use 1,140 expanded synthetic hierarchy rows by default. Viewport profiles use the same atlas-scale surface grid and apply a tiny visual transform every frame so the owned source view exercises live rendering rather than the static-source cache. The benchmark enables `DVZ_APP_FRAME_TIMING` for the measured run and also emits one compact `gui_viewport_bench` summary.

## RC3 Evaluation Evidence

An isolated mesh-only 1 x 1 physical query attachment was evaluated on the 697 x 699 synthetic FACE workload in a matched debug build on the RTX 5090. Three fresh 120-query runs measured a median 11.27 ms/query with the existing panel-sized attachment and 11.46 ms/query with the 1 x 1 attachment; median backend time remained approximately 6.9 ms/query. The experiment was not retained because it produced no representative improvement. This result indicates that expanded geometry processing and synchronous execution dominate attachment fill for this workload.

The same synthetic GUI runs measured 0.34 ms/frame for the empty GUI, 0.32 ms/frame for the 1,140-row expanded tree, 4.57 ms/frame for the live atlas-scale viewport, and 4.65 ms/frame for the combined tree plus live viewport. These are single-run debug-build diagnostics rather than release thresholds, but they reproduce the consumer finding: retained tree traversal is not the priority, while live embedded presentation remains material.

## Stop And Retention Rules

- Do not retain the one-pixel target if edge, depth, transform, panel-offset, DPI, native, or WebGPU query semantics differ, or if representative end-to-end improvement is negligible.
- Do not retain indexed exact-face lowering if it exposes backend primitive identity, loses supported backend behavior, changes public results, or fails to reduce representative retained/uploaded bytes materially.
- Do not add an atlas-specific API, mesh-region alias, CPU visual-picking fallback, or second per-part state mechanism.
- Do not make asynchronous query execution, mesh parts, or GUI viewport pipelining an RC3 release gate.
- Keep pointer-rate policy, ontology lookup, and domain face-to-region mapping application-owned.

## Post-v0.4 Sequence

The RC4 lane also adds copied generic fragment coverage for ordinary triangle meshes, shared by rendering and FACE/ITEM queries, so applications can preserve exact half-open source-space boundaries without moving or cutting vertices. The IBL consumer uses private crossing-face presentation copies and the same binary assets. Warm real-atlas queries retain static geometry; the query-cache capacity is now shared between the plan and executor and checked at compile time. See [the coverage follow-up](SCIENTIFIC_3D_PROFILE_HANDOFF.md#original-space-fragment-coverage-follow-up-2026-10-15) for bounded source evidence. This does not introduce mesh parts or asynchronous execution.

The bounded indexed item-state bank is now implemented on the RC4 scientific lane: copied per-vertex u32 slots address a fixed-size scene-owned flags buffer; existing hover/selection styles apply in lit/unlit OPAQUE, BLENDED and WBOIT rendering. Sparse updates redraw without changing FACE/link identity or authored alpha coverage. It is a v0.4 capability, independent of the larger mesh-part API below. Full parts should reuse or evolve this bank machinery rather than introduce a second decoration mechanism. RC3 remains unchanged; RC5 is contingency for acceptance gaps, and final v0.4 validates the chosen capability rather than expanding it.

1. Implement bounded asynchronous query execution and later-frame completion while retaining the explicit synchronous helper.
2. Implement mesh parts in independently validated slices: normalized indexed subdraws, retained part layout/state, GPU state buffer, direct subdraw lowering, ITEM identity, batched Python facade, then native/WebGPU and transparency coverage.
3. Implement GUI viewport submitted/completed/displayed generations, bounded source-image buffering, scoped completion, fair multi-viewport scheduling, and deferred retirement before removing the steady-state device-wide wait.
4. Add combined part-plus-face identity only after ordinary part interaction no longer depends on expanded face query geometry.
5. Consider faster subdraw lowering, culling, or indirect commands only when direct multipart benchmark evidence crosses a recorded trigger.

## Validation Routes

Query code changes require focused scene query tests, `just build`, `just test scene`, `just spec-check`, native validation-layer coverage, relevant WebGPU fixture/browser checks, and `git diff --check`. Shader or shader-ABI changes additionally require `just shader-abi-check`. Public headers or bindings require `just ctypes` and `just ctypes-check` before Python validation.

GUI viewport synchronization changes require focused GUI viewport tests, idle and multi-viewport scheduling coverage, resize/hide/show/destruction smokes, native validation layers, bounded live runs, and the architecture contract's generation and lifetime cases.

## Release Records

Update `STATUS.md` only when retained work changes current candidate evidence or gates. Record exact candidate commit and artifact/platform validation in `RELEASE.md` only after the implementation is selected and validated. When this execution lane is complete, preserve durable behavior in `spec/` and remove this handoff; Git history remains the completed-work archive.


## Panel planning investigation — October 19 2026

### Label indexing follow-up — October 20 2026

DRP2 streams now retain a private open-addressed ID-to-label-slot index while preserving the authored label array and first-insertion order. Sparse 64-bit IDs, overwrites, empty labels and reverse-name lookup retain their previous behavior. Index allocation failure discards only the optional accelerator, keeps every label available through the original linear path and retries index construction on a later insertion. No public API, runtime object ownership, semantic validation or GPU command behavior changes.

An isolated native Debug microbenchmark used eight fresh processes in alternating ABBA order, with four processes per implementation and approximately 100,000 labels per size per process. Median insertion of 1,024 labels fell from 407.58 to 30.46 microseconds; updating all labels fell from 406.72 to 15.36 microseconds. At 4,096 labels, insertion fell from 7.42 to 0.196 milliseconds. This establishes improved metadata scaling, not whole-frame latency. Fresh insertion at 256 labels increased from 24.33 to 26.83 microseconds, while updating that size fell from 23.51 to 3.59 microseconds; allocation and small-table costs still require representative Release measurements. Raw benchmark sources and results remain ignored under `build/`.

Validation passes 178 DRP2 and related scene tests with zero skips and active Vulkan synchronization and submit-time validation, with no reported errors. Focused label growth and forced index-allocation-failure tests also pass Valgrind with no reported errors or definite leaks. Runtime object indexing remains deferred: its design must account for semantic-state transactional cloning and backend destroyed-slot reuse rather than adding an unmeasured cross-frame cache.

Branch `perf/panel-render-reuse` starts from the published scientific candidate `5690da0b2`. The application baseline remains unchanged. `DVZ_SCENE_PANEL_DIAGNOSTICS=1` enables instance-scoped per-panel planning/composition timings, logical render/draw/upload-node counts, observed revision/matrix fingerprints, and emitted-stream outcome. Counts describe planned work, not GPU execution; common binding writes emitted later are explicitly excluded. Unknown figure-only changes remain unattributed. The disabled path performs no diagnostic snapshotting or fingerprinting.

Effective MVP fingerprints are necessary: direct arcball/camera mutations can change rendered matrices without changing panel descriptor revisions. A two-panel CPU regression checks disabled diagnostics, real prelude uploads, view-local matrix changes and unrelated figure changes. A private-display real-data rotation confirms view changes in the 3D panel while the static slice panels report only unattributed figure changes. Resource fingerprints are diagnostic hints, not a complete cache-validity contract.

A bounded exact-input CPU composition-cache experiment passed four focused regressions, including failure/retry, refreshed dynamic draw state and maximum serialization bounds. Eight alternating fresh-process measurements gave median source-frame time 3.61 ms disabled versus 3.70 ms enabled (12 trials of 240 frames per mode). Composition resolution was only approximately 57 microseconds of the four panels' approximately 383 microseconds of planning in a separate diagnostic run. The cache was not retained. Investigate repeated same-frame validation, fixed-capacity temporary initialization/copies and graph/binding searches before expanding cross-frame state.

A separate isolated prototype routed the three disjoint slice panels through existing typed presentation passes. Without any image reuse, it changed 87,842 pixels, with maximum channel difference 194, at a source extent of 1823 by 1454. Fractional panel origins and outward-rounded intermediate allocations were mapped inconsistently. A prototype retaining fractional producer viewports and using floor-origin presentation fetching reduced this to 175 pixels with maximum difference 2. Do not port that patch generally: fullscreen AO/EDL/transparency consumers, absolute fragment-coordinate shaders and arbitrary resource extents need one explicit allocation-versus-logical-viewport mapping contract. The patch remains an ignored experiment. Additional passes, logging and separate-build configuration prevent interpreting its wall time as a cache speedup.

The [panel reuse proposal](../../spec/scene/implementation/PANEL_RENDER_REUSE.md) records isolation, typed products, execution acknowledgement, frame slots, failure recovery, shared dependencies and fallback requirements. GPU panel-image caching is not implemented or enabled. Raw benchmark reports, experiment patches and captures remain in the consumer's ignored `build/review2-headless/` tree. No new manual desktop or macOS evidence is claimed.


### Adjacent validation simplification

The composition resolver previously called the immutable technique-contract validator immediately before full snapshot validation, which calls that same contract validator again. It now relies on the existing full validator once at that boundary; graph, append, auxiliary-binding and execution-facing validation remain intact. An isolated three-visual CPU resolver benchmark, eight alternating fresh processes with 20,000 resolutions each, measured median 8.94 microseconds before versus 7.84 microseconds after, with identical resolved fingerprints. This approximately 12% reduction applies only to that resolver microbenchmark, not whole-frame latency. Snapshot size in this build is 199,912 bytes. Representative frame profiling remains the guide for larger topology, binding-search and storage work.


Validation of the resolver simplification: build and specification/source checks pass; all 1,315 native tests pass with forced Vulkan synchronization validation, and the paired consumer suite passes 1,106 tests with two existing optional skips. A separate real-data 24-frame rotation completes with active synchronization/submit-time validation and no reported errors. The complete native runner nevertheless emits eight Vulkan validation errors in lower-level fixtures; passing test assertions are not a clean validation result. One live swapchain test presents without waiting on acquire or transitioning the image. The other feature-disabled synchronization2 messages require isolated attribution. These findings are being triaged separately from CPU composition behavior.


### Lower-level validation fixtures

Serial isolation attributes all eight initial validation messages to two existing tests. `vk/memory_interop_buffer_export` enabled timeline semaphores without synchronization2, then invoked transfer helpers using barrier2/submit2. Its standalone device now probes and enables the required supported features and uses the selected test GPU consistently. `vklite/swapchain_acquire_present_cycle` acquired a live image then presented without an acquire-semaphore wait or layout transition. It now records an owned legacy barrier to present layout, submits with acquire wait and a distinct render-finished signal, and presents waiting on that signal before conventional queue-idle cleanup. This changes test setup only; it does not alter production synchronization policy or add waits to application frames.

Both formerly invalid focused cases pass with active synchronization and submit-time validation and zero validation errors. The subsequent complete Linux runner passes all 1,315 tests with no skips and no reported Vulkan validation errors. The scene and canvas modules separately pass 834 and 50 tests without validation errors. This exercised single-GPU Linux evidence does not establish macOS, Windows or multiple-GPU acceptance; remote CI remains pending publication of these local commits.
