# Incremental C QA Handoff

Status: active incremental source audit completed through `input`, math statistics, and the bounded math box checkpoint; the remaining campaign and two-lane render coordination were approved on 2026-08-02. Updated: 2026-08-02.

This handoff records the current static-analysis, sanitizer, lifetime, bounds, and corruption-prevention pass. It is development evidence for the RC3 release-quality lane, not final exact-artifact or platform-matrix proof.

## Operating Contract

- Work one active module at a time, confirm that it is built and used before investing in it, keep each slice small, and checkpoint only after focused tests pass.
- Keep Vulkan validation layers disabled during this pass because their known defects and leaks contaminate memory-tool results. Pass `-DDVZ_USE_VALIDATION=OFF` explicitly when configuring every normal or sanitizer build and verify the resulting `CMakeCache.txt`; do not infer the setting from an old build directory.
- Use normal tests first, then ASan/UBSan/LSan, TSan for Datoviz-owned concurrency where practical, and focused `clang-tidy`/`cppcheck`. Treat tool stalls, provider incompatibilities, suppressions, and unavailable coverage as limitations rather than passes.
- Do not claim a repository-wide clean result from per-module analysis. This campaign has deliberately favored narrow, actionable findings over a noisy whole-tree report.
- Keep changes to `external/` isolated in their own commits and document local patches in the vendored file header. Do not modify other vendored sources unless a finding cannot be fixed at a Datoviz-owned boundary.
- Preserve the repository staging rules, especially for `data`, generated bindings, runtime libraries, and unrelated concurrent work.

The local Linux environment currently has `cppcheck` and `clang-tidy`. Separate `build`, `build-asan`, and `build-tsan` trees were configured with validation disabled; future agents must recheck rather than rely on those local caches. CUDA was disabled in sanitizer builds. No additional tool installation is required for the immediate next slice.

## Remaining Campaign Contract

This queue is the proposed execution contract for the autonomous remainder of the exploratory source audit. Maintainer approval of the mapping commit authorizes local inspection, focused fixes, tests, handoff updates, and checkpoint commits on `qa/rc3-source-audit`; it does not authorize pushing, publication, `data` changes, generated binary payloads, or unrelated refactors.

### Execution Protocol

1. Use one primary writer and committer. Read-only subagents may map callers, independently review a bounded slice, or triage analyzer output; they must not edit, stage, commit, or run concurrent heavy builds.
2. At each module entry, re-read applicable rules/specifications, verify the listed paths and production callers against current HEAD, inventory focused tests, and record mechanical drift. Path or test-name drift may update this queue without changing its reviewed architecture or scope.
3. Establish a normal-test baseline before editing. Inspect ownership, arithmetic, bounds, null/error paths, partial construction, teardown, re-registration/recreation, and supported concurrency before interpreting analyzer output.
4. Fix only actionable correctness, lifetime, bounds, overflow, corruption-prevention, or error-propagation defects. Add focused regression coverage; do not manufacture code changes for a clean slice.
5. Run focused normal tests, `clang-tidy`, and `cppcheck`, then ASan/UBSan/LSan with validation disabled. Use TSan only for a concrete supported Datoviz-owned concurrent path. Run Vulkan validation separately in a non-sanitized configuration when graphics ownership, commands, resources, synchronization, WSI, or presentation changes; if known layer defects prevent usable evidence, record the limitation and continue to the remaining authorized work.
6. Run `just build`, relevant test lanes, `just spec-check`, and `git diff --check` in proportion to the change. Run `just ctypes && just ctypes-check` after public header, exported API, binding policy, or generator changes; run shader ABI and visual-boundary checks after their governed inputs change.
7. Make one local checkpoint commit per coherent fix group after the required staging inspection. Keep external/vendor changes isolated, preserve unrelated work, and update this handoff with exact evidence and limitations after each module or meaningful submodule group.
8. Continue past unavailable providers, inconclusive tools, and physical-platform gaps after recording them honestly. Stop only for an architectural or public-API choice not determined by reviewed contracts, a required scope expansion, a repeated irreducible blocker, a staged stop-sign path, conflicting user work, or an action requiring external publication or unavailable authority.

### Temporary Two-Lane Coordination

If the maintainer approves [HANDOFF_RC3_RENDER_QA_ORCHESTRATION.md](HANDOFF_RC3_RENDER_QA_ORCHESTRATION.md) and the render-product architecture, that handoff temporarily refines the dependency order below while rendering contracts are changing. The original module map remains the complete campaign boundary; only the timing and final-sign-off points change.

| Module or slice | Before render landing | Final sign-off after landing |
| --- | --- | --- |
| `math` | Complete normally. | Repeat only if math sources, public/shared math headers, build flags, or direct consumer assumptions change. |
| `window` host/wrap/headless/GLFW core | Audit ownership, callbacks, configuration, metrics, failure rollback, and local lifecycle. | Repeat affected tests if borrow/ownership, surface replacement, extensions, resize/frame-request, presentation callbacks, or shared headers change. Withhold canvas/presentation integration sign-off until landing. |
| `stream` registry/core state machine | Audit registration, growth, start/update/submit/stop transactionality, and local ownership. | Always repeat canvas/video attachment integration; repeat core if public/shared contracts, payload lifetime, callback tables, state ordering, or error semantics change. |
| `video` encoder core/mux/providers | Audit provider-independent configuration, encoder ownership, muxing, provider cleanup, and isolated provider paths. | Always repeat `video_sink.c` plus canvas/stream integration; repeat core/provider slices if frame descriptors, formats, stride/timestamp, public/shared headers, provider configuration, or file/error contracts change. |
| `canvas` | Defer. | Complete the planned module audit after render landing. |
| `vk` | Defer. | Audit affected loader/device/queue/memory/interop/capability/format/sample/resolve ownership surfaces after landing. Complete the original module boundary unless the landing manifest mechanically proves an isolated untouched slice. |
| `vklite` | Defer. | Audit every changed command/sync/resource/descriptor/pipeline/rendering/surface/swapchain/technique slice plus direct runtime consumers. |
| DRP2 wire/serialization/recording | Read-only inspection is safe, but do not record final sign-off while the protocol and shared headers may change. | Repeat changed packet/schema/fixture/semantic slices; unchanged pure slices may retain prior evidence only when the landing manifest proves no public/shared/header/build dependency. |
| DRP2 backend/objects/pipeline/pass/transfer/runtime | Defer. | Audit all affected native execution and parity slices after vklite and render contracts stabilize. |
| Scene isolated retained CPU state | Inspect only when the render landing manifest is expected to exclude its public contracts, shared headers, frame artifacts, query formats, and shaders. My preference is to defer final scene sign-off. | Repeat direct tests for every changed or dependency-invalidated slice. |
| `frame_plan`, `render_contract`, `scene_emit`, scene `runtime`, `techniques`, governed shaders, shader registry, and affected visual foundation/capability/pipeline/lowering/material paths | Defer. | Complete these source-audit slices after legacy rendering paths are removed. Include analytic sphere, mesh/primitive, surface-record, AO, EDL, transparency, MSAA, panel extent, capability, and shader ABI boundaries. |
| Scene query/interaction/text/annotation and unrelated visuals | No final work is required solely because rendering changes. | Repeat only when changed directly or invalidated through public/shared headers, frame-artifact identity, query readback formats, common shaders, build configuration, or direct caller contracts. |
| `app` | Defer. | Audit affected configuration/lifecycle/scheduling/presentation/trace/replay/recovery integration after the rendering and QA branches converge. |

This policy is contract-based, not directory-based. A textually unchanged module slice is invalidated by a public/shared header, lifecycle, frame-state, resource identity, shader ABI, fixture, build-configuration, or direct caller contract change on which its conclusion depended.

The render lane must supply a landing manifest with its merge base/head, changed public/shared headers, source paths, generated inputs, shader/build/fixture changes, ownership and lifecycle deltas, protocol/schema deltas, test configurations, and explicit unaffected claims. Apply these mechanical rules:

1. An isolated implementation change reruns the slice's static checks, focused normal/sanitizer tests, and direct boundary consumer tests.
2. A public or shared-header change reruns every recorded slice that includes or relies on it.
3. A DRP2 command/schema/semantic change reruns changed C tests, fixtures, `just drp2-fixtures`, WebGPU preflight, and scene emission for the command class.
4. A shader/technique/resource-contract change reruns shader ABI/source guards, relevant vklite/DRP2 runtime, scene technique/emission tests, validated offscreen proof, and the affected visual conformance cases.
5. A window/swapchain/canvas frame-lifecycle change reruns window/canvas/stream/video integration and bounded live presentation; offscreen evidence alone is insufficient.
6. A public API/header, binding policy, or binding-generator change runs `just ctypes` and `just ctypes-check` before Python, GSP, or packaging validation.

During the parallel period, use separate worktrees and build trees, one writer/committer per lane under one primary integrator, and explicit writer/build/sanitizer/GPU/shader/binding/DRP2/integration locks. Do not run concurrent heavy builds, sanitizers, shaderc first-use, GPU/WSI, validation, CUDA/NVENC, binding generation, DRP2 fixture generation, or packaging. A QA finding in a render-leased path becomes a cross-lane record rather than an overlapping edit.

### Dependency Order

| Order | Module | Planned checkpoints | Principal focused runner/gate |
| --- | --- | --- | --- |
| 1 | `math` | Numeric helpers; animation/PRNG; parallel execution | `dvztest_math`, `just test math` |
| 2 | `window` | Host/config/lifecycle; backend wrapping/headless; GLFW callbacks and native surface boundary | `dvztest_canvas`, `just test window` |
| 3 | `canvas` | Core frame lifecycle; stream/sink integration; offscreen/live-image; window/swapchain presentation | `dvztest_canvas`, render lanes, bounded live canvas |
| 4 | `stream` | Sink registry; start/update/submit/stop transactionality; video attachment | `dvztest_canvas`, `just test stream` |
| 5 | `video` | Core/backend selection; MP4 muxing; kvazaar; NVENC; video sink integration | `dvztest_canvas`, `just test video` where providers exist |
| 6 | `vk` | Loader/instance/GPU discovery; queues/device; memory/interop; GPU context | `dvztest_vk`, `just test vk` |
| 7 | `vklite` | Commands/sync; buffers/images; descriptors/pipelines; rendering; surface/swapchain; techniques | `dvztest_vk`, full `vklite` module, runtime/render lanes |
| 8 | `drp2` | Streams/transport; recording; semantic transactionality; native objects/pipelines; transfers/passes/borrowed frames; parity | `dvztest_drp2`, DRP2 fixtures, runtime-vklite, WebGPU preflight |
| 9 | `scene` | Core/domain/contracts/query/interaction/visual families/annotation/emission/runtime/techniques | `dvztest_scene`, scene/DRP2/runtime/render lanes, source guards |
| 10 | `app` | Configuration/lifecycle; scheduling/presentation; trace/record/replay; runtime recovery | unified `app` filters, scene app-offscreen groups, integration/live loops |

The order follows the standalone QA handoff and keeps smaller CPU-oriented modules ahead of provider-heavy foundations. During the approved two-lane campaign, use the temporary ordering above: `math`, window core, stream core, and video core/provider slices may precede the render landing; canvas and the affected Vulkan/DRP2/scene/app foundation follow it. A finding may pull a downstream caller into focused validation, but implementation changes stay in the owning module checkpoint unless the reviewed contract proves the foundation itself is wrong.

### `math`

Boundary: `src/math/{vec,box,stats,anim,parallel}.c`, `src/math/prng.cpp`, `include/datoviz/math.h`, `include/datoviz/math/**`, and `src/math/tests/**`; active consumers span geometry, controllers, scene transforms/layout/animation, and rendering helpers.

1. Audit `vec.c`, `box.c`, and `stats.c` for empty inputs, invalid counts, count/stride arithmetic, non-finite values, normalization/division by zero, degenerate boxes, stable reductions, allocation failure, and aliasing; use `test_math.c`, `test_box.c`, `test_stats.c`, and `test_color.c` where shared inline math is exercised.
2. Audit `anim.c` and `prng.cpp` for malformed tracks, zero/negative/non-finite durations, endpoint and extrapolation behavior, keyframe ordering/count bounds, state initialization, deterministic sequences, integer/range bias, and overflow; use `test_anim.c` and `test_prng.c`.
3. Audit `parallel.c` for zero work, worker-count and partition arithmetic, callback/user-data lifetime, thread creation failure, join/cleanup, error propagation, and nested/re-entrant use; use `test_parallel_thread_config` and `test_stats_parallel`, adding TSan only if the inspected contract exposes real concurrent Datoviz state.

Checkpoint numeric helpers separately from stateful/parallel fixes. Complete with focused static analysis, normal and ASan/UBSan/LSan math tests, applicable TSan, `just build`, `just spec-check`, bindings when public headers change, and `git diff --check`.

### `window`

Boundary: `src/window/{window_host,backend_wrap,backend_headless,backend_glfw}.c`, `window_internal.h`, `include/datoviz/window.h`, `include/datoviz/window/**`, and `src/window/tests/**`; callers are canvas, app, GUI/Qt adapters, examples, and test fixtures.

1. Audit `window_host.c` for host/window ownership, backend-slot and window-array growth, configuration ABI validation, required-extension copies, fallback and partial creation, router/gesture lifetime, resize/scale caching, frame requests, wait hooks, surface replacement, and reverse-order teardown.
2. Audit `backend_wrap.c` and `backend_headless.c` for borrowed versus owned native handles/surfaces, attach/detach/replacement, null and repeated lifecycle calls, metrics and extension queries, callback/user-data lifetime, and backend failure rollback.
3. Audit `backend_glfw.c` for GLFW global initialization/termination, native window/surface ownership, callback installation and re-entry, key/button normalization, logical/framebuffer/content-scale conversion, minimized/closed windows, required extensions, and teardown after partial Vulkan-surface creation.

Use `test_window.c`, `test_window_wrap.c`, `dvztest_canvas --module window`, related input tests, focused ASan/UBSan/LSan with validation disabled, and GLFW/Vulkan validation plus physical presentation only as separate evidence. Keep the earlier teardown fix from `1b96056e4` in scope without claiming it completed this module.

### `canvas`

Boundary: `src/canvas/{canvas,canvas_stream,offscreen_sink,live_image_sink,window_surface,swapchain_sink}.c`, internal headers, `include/datoviz/canvas.h`, `include/datoviz/canvas/**`, and `src/canvas/tests/**`; callers are stream, video, DRP2/app presentation, Qt hosting, and live examples.

1. Audit `canvas.c` for GPU/window/config ownership, frame-slot count and state transitions, begin/submit/end balance, borrowed frame command buffers/images/semaphores, resize/recreate, wait/close, deferred work, callback lifetime, partial construction, and teardown with active frames.
2. Audit `canvas_stream.c` for stream attachment/detachment, sink negotiation, frame handoff, update/restart/error propagation, close ordering, and prevention of duplicate ownership across canvas and stream.
3. Audit `offscreen_sink.c` and `live_image_sink.c` for image/host-buffer byte arithmetic, readback/upload ranges, callback payload lifetime, consumer backpressure, partial failure, repeated start/stop, and thread-visible state where supported.
4. Audit `window_surface.c` and `swapchain_sink.c` for borrowed window/surface contracts, swapchain image/view/command ownership, acquire/present indices, frame-slot versus image-count separation, out-of-date/suboptimal/minimized paths, resize rollback, and exact semaphore/fence lifecycle.

Use `test_canvas.c`, `test_canvas_glfw.c`, `dvztest_canvas --module canvas`, render-smoke/conformance lanes, `dvz_live_canvas --frames 300`, and present-path checks when presentation changes. Separate CPU sanitizer evidence from validation-layer and physical WSI evidence; commit core/frame fixes separately from sink/presentation fixes.

### `stream`

Boundary: `src/stream/{sink_registry,stream}.c`, `include/datoviz/stream.h`, `include/datoviz/stream/**`, and `src/stream/tests/**`; callers are canvas, video, app, and capture/export examples.

1. Audit `sink_registry.c` for registration/name bounds, duplicate or missing sinks, requested-versus-auto selection, callback tables, user-data lifetime, registry growth/allocation failure, and teardown.
2. Audit `stream.c` for state-machine validity, attach/detach ownership, start rollback, update/restart failure, first-error propagation, frame payload lifetime, submit ordering, stop/destroy idempotence, and video attachment cleanup.

Use the five focused stream regressions through `dvztest_canvas --module stream`, add failure-injection tests for concrete gaps, run normal and ASan/UBSan/LSan CPU paths, and add TSan only for a documented cross-thread sink contract. Validate canvas/video consumers without absorbing their fixes into the stream checkpoint.

### `video`

Boundary: `src/video/**`, `include/datoviz/video.h`, `include/datoviz/video/**`, and `src/video/tests/**`; `external/minimp4.h` remains vendored and its existing Datoviz patch is isolated at `e090875ac`.

1. Audit `encoder_core.c`, `encoder_backend.c`, `encoder_backend_stub.c`, internal headers, and file helpers for configuration validation, dimension/format/rate arithmetic, backend selection, frame ownership, partial initialization, flush/finalize/destroy, provider-missing diagnostics, and repeated failure cleanup.
2. Audit `encoder_mux_mp4.c` for timestamp/sample-size/count arithmetic, monotonicity, file I/O failure, codec-header ownership, mux finalization after partial writes, and malformed backend output; do not broaden vendored `minimp4` changes without an unavoidable boundary finding.
3. Audit `encoder_backend_kvazaar.c` and its tests for plane/stride/chroma sizing, thread/queue ownership, delayed frames, packet cleanup, flush, provider configuration, and error propagation.
4. Audit `encoder_backend_nvenc.c` and its tests for CUDA/Vulkan external-memory and semaphore ownership, device UUID selection, mapped-resource lifetime, timeline values, surface counts, failure unwind, and Windows/Linux provider differences.
5. Audit `video_sink.c` and canvas/stream integration for attach/start/submit/stop transactionality, backpressure, frame-copy lifetime, restart, and destruction while encoding.

Run provider-independent contract tests under normal and ASan/UBSan/LSan first, then kvazaar/NVENC cases only when compiled and available. Treat non-instrumented codec/CUDA/provider code, absent Windows/MoltenVK proof, and hardware codec availability as explicit limitations rather than skips promoted to passes.

### `vk`

Boundary: `src/vk/*.c`, private headers, `include/datoviz/vk.h`, `include/datoviz/vk/**`, and `src/vk/tests/**`; VMA compiled from `external/vk_mem_alloc.cpp` is vendored and read-only unless a defect cannot be contained at the Datoviz boundary.

1. Audit `instance.c`, `gpu.c`, `_loader.h`, instance/GPU private headers, and validation helpers for one-time loader state, runtime paths, extension/layer arrays, two-call enumeration races, GPU indices, callback lifetime, partial construction, and probe-storage ownership.
2. Audit `queues.c` and `device.c` for family/queue counts, role selection/fallback, duplicate requests, optional video/present queues, feature/extension storage, command/descriptor-pool ownership, failed build, wait/destroy ordering, and rebuild.
3. Audit `memory.c` and public memory/interop headers for `VkDeviceSize` conversions, offset-plus-size overflow, map/flush/invalidate ranges, VMA results, partial buffer/image creation, imported versus owned memory, external FD/Win32/semaphore handle transfer, and CUDA assumptions.
4. Audit `gpu_ctx.c` for copied versus borrowed configuration, selected-device identity, instance/device/allocator composition, interop configuration, error forwarding, accessors, and reverse-order idempotent teardown.

Use queue CPU tests and `dvztest_vk --module vk` before GPU sanitizer runs, then separate validation-layer evidence. Isolate public interop/API fixes and refresh bindings. Do not claim sanitizer coverage for Vulkan loader, driver, VMA, CUDA, Win32, or MoltenVK internals.

### `vklite`

Boundary: `src/vklite/*.c`, private headers, `include/datoviz/vklite.h`, `include/datoviz/vklite/**`, `src/vklite/tests/**`, and test shaders/fixtures; callers are canvas, DRP2 native execution, video interop, scene GPU tests, and raw-vklite examples.

1. Audit `commands.c` and `sync.c` for owned versus borrowed recording wrappers, begin/end/reset/submit/destroy authority, selected-command state, command-pool lifetime, barrier counts, stage/access/layout pairs, submit arrays, timeline monotonicity, fence/semaphore ownership, and failure cleanup.
2. Audit `buffers.c` and `images.c` for size/range/extent/mip/layer arithmetic, mapping and staging lifetime, external wraps/imports, view ownership, layout history, transition/copy preconditions, partial creation, and recreation.
3. Audit `sampler.c`, `shader.c`, `slots.c`, and `descriptors.c` for enum/SPIR-V validation, slot/binding/set counts, collisions/capacity, layout/group dependency lifetime, descriptor-pool semantics, allocation rollback, and recreate rules.
4. Audit `compute.c`, `graphics.c`, and `rendering.c` for shader/attachment/vertex-array bounds, format/sample/depth compatibility, pipeline-layout ownership, dynamic state/render areas, bind-before-draw/dispatch, render balance, and borrowed attachments.
5. Audit `surface.c` and `swapchain.c` for native-surface ownership, cached capability arrays, zero/minimized extents, format/mode choice, image/view counts, old-swapchain recreation, acquire/present indices, out-of-date/suboptimal paths, device rebinding, rollback, and teardown order.
6. Exercise render-to-texture, stencil, MSAA, compute-to-graphics, picking/readback, WBOIT, and SSAO through `test_techniques.c` and representative canvas/DRP2 consumers without introducing a parallel technique path.

Use the full unified `vklite` module plus the focused `dvztest_vk` subset, runtime-vklite and relevant render lanes, bounded live presentation for WSI changes, ASan/UBSan/LSan with validation disabled, and validation layers separately. TSan is limited to a real Datoviz-owned synchronization path; offscreen tests do not prove WSI.

### `drp2`

Boundary: `include/datoviz/drp2.h`, `include/datoviz/drp2/**`, `src/drp2/**`, `src/drp2/tests/**`, and executable contract mirrors in `spec/drp2/**` and their fixture/preflight tools. Preserve the authority order in `spec/drp2/AUTHORITY.md`.

1. Audit `stream.c`, `_stream.h`, and command metadata for command/label growth, fixed counts, validation phase, null/zero behavior, borrowed versus owned payload/SPIR-V lifetime, owner-lock release, identity, and destroy safety.
2. Audit `packet.c`, `packet_wire.h`, and `serialization.c` for alignment/length/count arithmetic, hostile or truncated decode, union selection, payload offsets/arena lifetime, phase splitting, JSON escaping, unsupported raw/base64 combinations, cleanup, and round-trip fidelity.
3. Audit `recording.c` for paths/files, blob sizes and references, owner-array growth, malformed/truncated JSONL, indexed fields, timestamps/frame indices, reconstructed-stream ownership, finalization failure, pacing arithmetic, and partial-load cleanup.
4. Audit `semantic.c`, `objects.c`, and semantic runtime state for exact command/lifetime state machines, typed identity and ID reuse, table growth, reference counts, encoder/pass sequencing, submit-once behavior, resource usage/ranges/formats/capabilities, precise diagnostics, and failed-stream non-commit.
5. Audit `backend.c`, `objects.c`, and `pipeline.c` for protocol-to-Vulkan conversion, resource/shader/bind/pipeline ownership, descriptor refresh after recreation, deferred destruction, tail trimming, reset, and wait/destroy.
6. Audit `transfer.c`, `pass.c`, and backend runtime execution for texture/buffer row and size arithmetic, staging cleanup, image layouts, attachment compatibility, pass cleanup, owned versus borrowed frame commands, target retirement, readback, and external timeline consumption.
7. Prove semantic/native parity with the same typed streams, fixtures, and WebGPU preflight; never compensate in scene with family names, resource names, upload order, or backend forks.

Use `dvztest_drp2` groups, DRP2 contract/runtime lanes, `just drp2-fixtures`, WebGPU preflight, `just spec-check`, CPU sanitizers first, and GPU sanitizers/validation separately. Intentional contract changes update authoritative prose, schema, fixtures, runner, C implementation, and focused tests in that order. Level 2 execution remains partly platform evidence and Level 3 output is deferred to DRP2 2.1.

### `scene`

Boundary: `src/scene/**`, `include/datoviz/scene.h`, and `include/datoviz/scene/**`; callers include app, WASM, GUI/examples, Python bindings, and DRP2 fixture/export tooling. Scene owns retained high-level state and frame artifacts, emits backend-neutral plans/DRP2, and never owns Vulkan, swapchain, borrowed frame-command, or presentation lifecycle. Read `spec/scene/AUTHORITY.md` and the specialized spec for each slice; DRP2 rules outrank scene prose at the runtime boundary.

Global invariants: IDs/resource keys remain scene-local and stable across collection growth; parent/child teardown is ordered; arithmetic is checked before allocation/copy; artifacts freeze owned immutable stream/payload snapshots; uploads never select rendering; typed descriptors/lowering carry visual policy; query freshness and scene identity are preserved; adaptation is explicit; generic code does not grow family-specific branches.

1. Audit `core/**` for scene/figure/panel/grid ownership, stale IDs, partial construction, layout/resize snapshots, callbacks/subscriptions, frame demand, artifact freeze/destruction, counters, and re-entry; use `scene_graph`, `dpi`, `frame_demand`, and helper tests.
2. Audit `domain/**` for buffers, fields/textures, compute, graphs/composites, meshes/polygons, sample profiles, byte/count/format/range arithmetic, dirty merging, copy ownership, and malformed descriptors; use `fields`, `sample_profile`, and graph tests.
3. Audit `frame_plan/**` and `render_contract/**` for growth, indices/IDs, cycles/topological order, producer/read rules, attachments/usages, packet spans, draw bounds, JSON, typed metadata, and complete failure diagnostics; use frame-plan tests, DRP2 contract lanes, and fixtures.
4. Audit `query/**` and family query helpers for coalescing/freshness, queue/executor/pending lifetime, cancellation, result identity/coordinates, scratch/readback sizes/formats, polling, recovery, and unsupported policy; use `query.c`, the query source guard, then GPU query cases separately.
5. Audit `interaction/**` plus `core/controllers.c` for subscriptions, handles/indices, re-entry, viewport/DPI conversion, controller rebinding, timer/phase/time bounds, catch-up, tracks, continuity, and frame-demand notifications; use interaction, animation, controller, DPI, and interaction-graph tests.
6. Audit the visual foundation files directly under `visuals/` and `visuals/registry/**` for descriptor ABI, registry completeness, attribute count/format/stride/source/mutability, range updates, dirty state, bindings, bounds over empty/non-finite data, materials/pipelines, and rollback; enforce architecture and visual-boundary guards.
7. Audit `visuals/{point,pixel,marker,segment,vector,primitive,splat}/**` for schemas, item/instance counts, queries, SVG/raster allocation, endpoint/topology ranges, generated quads, bounds, and GLSL/WGSL parity.
8. Audit `visuals/{path,stroke,mesh}/**` with mesh/polygon domain helpers for subdivision, joins/caps, derived caches, vertex/index/instance agreement, transforms, shared/external indices, textured bindings, degeneracy, and generated-size overflow.
9. Audit `visuals/{image,volume,labels,glyph,text}/**` for texture/field dimensions, layers/formats/regions, generated caches, slice/composite indices, label widths, query profiles, strings/glyphs/atlases, bindings, and payload freezing.
10. Audit `annotation/**`, `text/**`, `color/colorizer.c`, and `plot/{bars,band}.c` for empty/degenerate/log domains, ticks, strings/glyph/font/atlas lifetime, bitmap/layout/DPI bounds, legend/colorbar aggregation, colormap/palette ranges, and plot count agreement.
11. Audit `scene_emit/**` for the canonical artifact path, panel order and viewports, normalized policy, identity/versioning, exact upload ranges/ownership, rollback, dirty/hidden transitions, duplicate uploads, metadata completeness, and recreation.
12. Audit `runtime/**` for object maps, persistent/scoped resources, recreation and bind refresh, attachments/passes, offsets, shaders, draw/index bounds, phase splitting, and cleanup after failed lowering without acquiring presentation ownership.
13. Audit `techniques/**`, runtime technique targets, shader registry, and governed shader sources for attachment/sample/extent compatibility, aliasing/scoped IDs, load/store/resolve, layers/passes, fallbacks, mixed-technique rejection, recreation, and native/WebGPU parity.
14. Run the scene integration gate: focused static analysis, CPU scene and DRP2 lanes under normal and ASan/UBSan/LSan, runtime-vklite, separate validated Vulkan offscreen/query/technique cases, architecture/query/visual guards, `just build`, `just spec-check`, shader ABI checks, bindings when needed, and WebGPU/WASM smoke only when browser-facing artifacts change.

Use separate commits for coherent core/domain/contract/query/interaction/visual-family/emission/runtime/technique findings. Stop rather than silently redesign the frame-artifact boundary, generic visual policy, query architecture, or scene/DRP2 ownership. Exclude generated shader/font/atlas payloads, `text_default_msdf_atlas.inc`, gallery media, and `data` without exact approval.

### `app`

Boundary: `src/app/{app,status,trace}.c`, internal headers, `include/datoviz/{app,app_interop}.h`, `src/app/tests/test_app.c`, and app-facing integration in `src/scene/tests/app.c`; app orchestrates scene artifacts, DRP2/vklite, canvas/window/input, optional GUI, capture, record/replay, and external surfaces without duplicating scene planning or stealing borrowed resources.

1. Audit configuration, environment parsing, resource injection, owned/borrowed GPU/runtime contracts, view growth, partial creation, figure/panel links, close/reap, and destruction order; use CPU config/ABI/resource tests and GPU integration separately.
2. Audit scheduling, poll/wait/deadline arithmetic, pending-work/render gates, resize/input, posted/request/frame/GUI callbacks, offscreen/window/external surfaces, capture, close during callback, wakes, queues, scale/coordinates, and borrowed frame release; use scheduler, frame-demand, DPI, app-offscreen, and live-window tests.
3. Audit `trace.c`, `status.c`, and record/replay paths for growth/failure recovery, labels/string bounds, fingerprint semantics, transient-ID normalization, terminal truncation, paths/files, stream/payload lifetime, corrupt replay, target rewriting, pacing/speed, and teardown; use the focused trace/status cases and add malformed-input tests for concrete gaps.
4. Audit runtime failure/recovery in `app.c` for artifact acquisition, validation/execution, scopes, query completion, reset/deferred recovery, descriptor refresh, resize/recreation, callback suppression, stale runtime maps, exactly-once borrowed-frame release, recursion, shared-resource coherence, replay isolation, and retained dirty state.
5. Run focused static analysis, normal and ASan/UBSan/LSan CPU app tests, TSan only for a proven cross-thread post/wake queue, separate validated offscreen/live presentation and recovery, a bounded live loop when ownership changes, relevant scene/runtime lanes, build/spec/binding checks, and `git diff --check`.

Keep lifecycle/resource, scheduler/presentation, trace/replay, and recovery commits separate. GUI internals are caller evidence only unless a narrow app contract fix requires them; physical GLFW/Metal/Windows and Qt/PyQt provider proof remain external platform gates.

### Final Exploratory Gate

After the last module, rerun the frozen locally available matrix from the exact campaign HEAD: tool versions and cache options, focused module totals, CPU ASan/UBSan/LSan, applicable TSan, separate Vulkan validation, runtime/render/slow lanes, bounded live loops, DRP2 fixtures, WebGPU preflight or live browser smoke where applicable, bindings, specs/source guards, examples affected by fixes, `just build`, and `git diff --check`. Record commit identity, commands, pass/fail/skip totals, timeouts, suppressions, provider/GPU/driver identity, and explicit exclusions. This remains development evidence and does not replace exact source/wheel/package, hosted-platform, or physical-platform RC3 proof.

## Completed Checkpoints

| Area | Result | Checkpoints |
| --- | --- | --- |
| Scene/window teardown | Fixed owned scene/runtime cleanup and a host-window leak found by sanitizer-guided inspection. This was a focused teardown slice, not a complete `scene` or `window` audit. | `1b96056e4` |
| Test runner | Fixed isolated-child failure propagation so crashes and sanitizer failures cannot be silently reported as successful tests; added scheduler regression coverage. | `faaa1dedf` |
| Vendored video helper | Fixed HEVC VPS accounting, lookup, and cleanup in `external/minimp4.h`. The external change is isolated and its header records the Datoviz patches that must survive a vendor refresh. | `e090875ac` |
| Module inventory | Confirmed the retired `ds` module is not linked into the active library and removed it from the active-module guidance. It was not audited as production code. | `ecb39a0b4` |
| Common logging | Replaced unsafe concurrent local-time conversion, then hardened logger synchronization and object-container access. Added concurrency and object lifetime tests. | `0a756e200`, `1071ca745` |
| File I/O | Hardened file/NPY size and shape validation, PNG/PPM cleanup and error handling, gzip failure paths, and byte-write handling. Added malformed-input and failure-path coverage. Public file-I/O headers and generated ctypes changed together and binding checks were run at those checkpoints. | `27ad849f6`, `9a1a16838`, `ec0c0e9ae` |
| Geometry | Confirmed `geom` is active and consumed by production paths. Added checked primitive count arithmetic, rejected malformed OBJ numeric records, and exercised the hashed polygon triangulation path. | `50c0b09e5`, `e112798da`, `dbe5736f7` |
| Thread wrappers | Fixed wrapper and atomic ownership/lifetime edges and added focused cleanup coverage. Datoviz-owned thread/common concurrency paths were exercised under TSan during the slice. | `189f1b955`, `1071ca745` |
| Common allocation | Added overflow-safe allocation and copy size handling with boundary tests. | `c86450dc3` |
| Runtime shader compiler | Confirmed `shader` is active through DRP2 pipeline creation and downstream vklite/scene runtime use. Hardened runtime-library path construction and result cleanup; corrected first-use concurrency coverage; added null, empty, malformed, default-entry-point, provider-missing, provider-incompatible, diagnostic ownership, and double-destroy cases. | `badf143a4` |
| Sanitizer configuration | Made validation-layer exclusion durable in Linux ASan, MSan, and TSan recipe configuration. Explicitly reset ASan and TSan caches to `ON`, rebuilt through the recipes, and verified both caches resolved to `DVZ_USE_VALIDATION:BOOL=OFF`. | `a5a3073ec` |
| Input | Confirmed the synchronous router is active through window backends, canvas, controllers, scene, and WASM. Hardened subscription growth against overflow and allocation failure, preserved live arrays after failed growth, prevented removed callbacks from running later in the same dispatch, avoided callback-ID reuse after wrap, handled constructor and dispatch allocation failure, and rejected corrupting duplicate presses and backwards click timestamps. Added focused callback lifetime, allocation-failure, and gesture regressions. | `e710ea15f` |
| Math statistics | Confirmed the active math source set and corrected handoff drift: the public umbrella is `include/datoviz/dvzmath.h`, there is no standalone `test_parallel.c`, and parallel coverage lives in `test_stats.c`. Removed four duplicate exported statistics implementations from `parallel.c`, preserving the documented zero-count `dvz_range()` no-op in the sole `stats.c` implementation, and added focused range, extrema, clipping, and equal-bound normalization coverage. | `c157b2cfb` |
| Math boxes | Added the missing documented non-null output assertion to `dvz_box_inverse()` and regression coverage for nonpositive extent passthrough, empty and strategy-specific merges, ordinary one-dimensional and polygon normalization, and ordinary inverse mappings. Degenerate source/target axes, non-finite inputs, overlapping buffers, and initialization of untouched `vec3` components remain explicit contract questions; this checkpoint does not change them. | `c0005f0f6` |

Focused normal tests, relevant sanitizer runs, per-file static analysis, `just build`, `just spec-check`, and `git diff --check` were used throughout the checkpoints where applicable. There is no retained machine-readable campaign report, so final RC3 evidence must rerun the frozen matrix from exact release artifacts.

## Latest Confirmed State

At `c0005f0f6` on the Linux NVIDIA RTX 5090 host:

- Normal `just build` and `just test math` passed; math selected 15/15 tests.
- The focused math lane passed 15/15 under ASan/UBSan/LSan with `DVZ_USE_VALIDATION:BOOL=OFF`; the runtime emitted the host's existing sanitizer interceptor warnings without a sanitizer finding.
- Focused `clang-tidy` on `stats.c` and `parallel.c` emitted no user-code diagnostic. Focused `cppcheck` emitted only its configuration-count notice.
- `just spec-check` passed through the main worktree's existing `.venv`, including 125/125 DRP2 fixtures, WebGPU fixtures/preflight, scheduler tests, and scene source guards. The first invocation with system Python was an environment failure because `/usr/bin/python3` lacks `pytest`, not a repository failure.
- `git diff --check` passed before the implementation checkpoint.
- The focused normal box selection passed 7/7 including its direct scene query consumer, while the exact ASan/UBSan/LSan `math/box` group passed 6/6 with validation disabled and no sanitizer finding. The broad `just atest box` selector was stopped after its six box cases passed because it also selected and stalled in an unrelated GPU scene-query case.
- Focused `clang-tidy` on `box.c` and `test_box.c` emitted no user-code diagnostic, and focused `cppcheck` emitted no diagnostic.

Previous input checkpoint evidence at `e710ea15f`:

- Normal `just build` passed, and `just test input` passed 19/19 selected input and related scene/GUI tests with validation disabled.
- The focused input binary passed 10/10 tests under ASan/UBSan/LSan without a report.
- Focused `clang-tidy` on all three input implementation files produced no emitted diagnostic. Focused `cppcheck` produced only its non-actionable callback-const suggestion and configuration-count notice.
- `just ctypes` and `just ctypes-check` passed after public input-header documentation changed. The refresh also captured a pre-existing checked-in binding freshness gap for recently added external-handle APIs.
- `just spec-check` passed, including 125/125 DRP2 fixtures, WebGPU fixtures and preflight, binding/source policy, scheduler, query, architecture, and visual-boundary guards.
- Normal, ASan, and TSan caches all contained `DVZ_USE_VALIDATION:BOOL=OFF`; the ASan and TSan values were explicitly reset to `ON` before the hardened recipes resolved them back to `OFF`.
- `git diff --check` passed before both implementation commits.

## Known Limitations

- TSan shader smoke and downstream ASan DRP2 shader execution stall inside the dynamically loaded, non-instrumented shaderc provider. Bounded runs were terminated without a sanitizer or race report. These runs are inconclusive, not passes, and the first-use concurrency test must not be weakened to avoid the provider issue.
- Vulkan validation has intentionally not been exercised in this campaign. It remains a separate RC3 gate after known validation-layer defects and leaks are dispositioned or suppressed with evidence.
- No full-tree `just analyze`, complete `just test`, MSan, Valgrind, long-loop, installed-package, source-archive, wheel, hosted-platform, or physical-platform campaign has been claimed here.
- Input TSan was not run because the public router contract is synchronous and the production caller audit found no supported Datoviz-owned concurrent access path. This is a deliberate non-applicable disposition, not evidence for concurrent router use.
- `scene` and `window` received a teardown fix only; they still need full module audits later. The Vulkan-facing foundation (`window`, `canvas`, `stream`, `video`, `vk`, `vklite`, `drp2`, `scene`, and `app`) should be approached only after the smaller CPU-oriented modules because ownership and provider noise require more careful matrices.

## Approved Restart Sequence

1. The maintainer approved the remaining campaign contract, render-product proposal, and two-lane orchestration handoff without edits on 2026-08-02.
2. Begin with the `math` numeric-helper checkpoint, apply the module-entry drift check, and proceed through only the pre-landing slices in Temporary Two-Lane Coordination while the separate render lane runs.
3. After the render branch lands, consume its cumulative landing manifest, rerun only the mechanically invalidated completed slices, and execute every deferred module/slice in dependency order. Do not infer that unchanged directories retain valid conclusions when shared contracts moved.
4. Keep this document current after each checkpoint: condense completed queue text into evidence when detail is no longer useful, preserve limitations, and leave the next exact restart point unambiguous.
5. At campaign completion, provide the maintainer a consolidated report of commits, findings, clean audited areas, tests and tools, limitations, remaining risks, integration options, and final worktree status.

For final RC3 release evidence, convert this exploratory sequence into a frozen matrix with exact commit/artifact identity, commands, tool versions, configurations, pass/fail/skip totals, timeouts, suppressions, provider versions, GPU/driver identity, and explicit exclusions.
