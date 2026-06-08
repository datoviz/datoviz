# WASM/WebGPU Debuggability Refactor Plan

Execution Status:

- Status: proposed architecture hardening plan
- Updated on: 2026-06-07
- Purpose: make the WASM/WebGPU scene path easier to implement, validate, and debug
- Scope: scene-to-FramePlan emission, DRP2 packet transport, browser WebGPU replay, diagnostics,
  and validation workflow


## Problem

The WASM/WebGPU path currently works as an end-to-end integration slice, but failures are too often
diagnosed at the browser runtime boundary after many layers have already transformed the data.

The hard path is:

```text
portable scenario or scene API
  -> retained scene state
  -> FramePlan
  -> runtime emitter caches
  -> DRP2 command stream
  -> split setup/update/frame binary packets
  -> JavaScript packet decoder
  -> WebGPU runtime object creation
  -> async browser frame/readback result
```

This is the right architectural direction, but the current contracts between stages are too soft.
String keys, borrowed pointers, cached object ids, shader sources, packet payloads, and browser GPU
objects can become invalid or incomplete without failing at the layer that introduced the problem.

When that happens, the observed error is downstream, for example a WebGPU pipeline creation failure,
instead of an early scene or DRP2 diagnostic that names the invalid descriptor.


## Diagnosis

The issue is not that WASM, WebGPU, or Emscripten are intrinsically unsuitable. They make debugging
less forgiving, but most long debugging sessions come from missing intermediate invariants.

Current failure amplifiers:

1. Scene runtime emission mixes planning, descriptor resolution, object-id allocation, cache reuse,
   and DRP2 command construction.
2. Some runtime identities are plain strings with implicit validity rules.
3. Empty or malformed keys can enter persistent maps and affect later emissions.
4. Shader descriptor validity is not checked in one canonical place before object ids are allocated.
5. DRP2 packet encoding is mostly mechanical, but packet-level conformance tests do not yet cover
   all browser-promoted scene features.
6. JavaScript receives packet bytes after multiple C-side transformations, so browser errors often
   lack the original scene/FramePlan context.
7. Browser smoke tests are valuable, but they are currently too late in the validation chain to be
   the first precise failure detector.


## Known WASM Failure Mode: Stack Overflow Masquerading As Data Corruption

One concrete 2026-06 debugging failure looked like a query/shader/DRP2 bug but was primarily an
Emscripten stack-size problem.

Observed symptoms:

1. the query FramePlan was valid immediately before runtime emission: six nodes and one render node;
2. `dvz_frame_plan_emitter_emit_drp2()` received the valid plan;
3. after DRP2 stream setup/handshake, `plan->count` was observed as zero in the optimized WASM
   build;
4. query setup packets included render-pipeline commands, but decoded shader modules appeared empty
   downstream;
5. temporary sentinel returns changed the observed behavior because they changed control flow and
   stack/layout, not because they isolated the logical bug.

Root cause and fix:

1. Emscripten's default stack is small for C code with large automatic arrays. A stack overflow can
   corrupt global or heap memory and then fail far away from the function that overflowed.
2. The direct query offender was `_dvz_scene_query_drop_superseded_results()`, which allocated
   `DvzQueuedQueryResult kept[DVZ_SCENE_MAX_QUERY_RESULTS]` on the stack. With
   `DVZ_SCENE_MAX_QUERY_RESULTS == 128` and `sizeof(DvzQueuedQueryResult) == 608`, that one local
   array used 77,824 bytes.
3. Moving that scratch queue to heap storage reduced the function's measured stack frame from
   77,824 bytes to 608 bytes.
4. The WASM scene target now uses an explicit `-sSTACK_SIZE=1048576`. This is intentionally larger
   than Emscripten's default but much smaller than the temporary 16 MiB value used to prove the
   failure mode.

Workflow rule:

1. If valid C state becomes impossible after an unrelated WASM call, do not keep narrowing with
   sentinel returns first.
2. Build a native repro for the C boundary and run ASan/UBSan.
3. Build Emscripten variants with `-fsanitize=address`, `-sASSERTIONS=2`, `-sSAFE_HEAP=1`, and
   `-sSTACK_OVERFLOW_CHECK=2` where applicable. ASan and SAFE_HEAP cannot be combined.
4. Add `-fstack-usage` and `-Wframe-larger-than=<threshold>` to rank large automatic stack frames.
5. Prefer removing large automatic arrays or moving scratch storage to heap/owned state before
   raising `STACK_SIZE`. Raising stack size is a mitigation, not a root-cause explanation.
6. Keep a narrow Node packet probe for the promoted WASM feature so the browser/WebGPU runtime is
   not the first detector.

Current measured stack offenders after the query cleanup include unrelated annotation/render/text
paths. If future WASM work trips stack checks, inspect those frames before assuming query, shader, or
packet corruption.


## Goal

Make the browser path boring to debug.

A broken scene, visual descriptor, shader descriptor, resource descriptor, packet, or WebGPU command
should fail at the earliest responsible boundary with a deterministic diagnostic and a narrow test
that reproduces the failure without a full browser session when possible.

The target debugging ladder is:

```text
scene invariant failure
  -> FramePlan validation failure
  -> resolved runtime descriptor validation failure
  -> DRP2 stream semantic failure
  -> DRP2 packet conformance failure
  -> WebGPU replay failure
  -> browser smoke failure
```

Browser smoke should confirm integration. It should not be the normal way to discover malformed
shader modules, empty object keys, invalid bind group layouts, or bad packet payload spans.


## Non-Goals

1. Do not port native Vulkan/vklite/canvas/window/app internals to WASM.
2. Do not make JavaScript understand Datoviz scene visual families.
3. Do not replace DRP2 as the browser runtime contract.
4. Do not add browser-only scene semantics to work around missing C-side validation.
5. Do not broaden browser feature scope while the validation ladder is still weak.
6. Do not require source-map/browser debugger workflows for routine scene emission bugs.


## Design Principles

1. Validate before lowering.
2. Make invalid states unrepresentable where practical.
3. Keep DRP2 emission mechanical.
4. Preserve shared scene semantics between native and browser paths.
5. Use typed descriptors internally instead of ad hoc strings and pointer bundles.
6. Attach enough provenance to diagnostics to identify the scene object, visual family, pass role,
   packet phase, and command index.
7. Promote a feature only after it has at least one pre-browser validation test and one browser
   integration test.


## Target Architecture

The browser path should keep the existing high-level route:

```text
C/WASM scene state
  -> FramePlan
  -> DRP2 command stream
  -> split DRP2 packets
  -> WebGPU runtime
```

The refactor inserts explicit validation and typed resolved descriptors between FramePlan and DRP2:

```text
FramePlan render/readback nodes
  -> resolved runtime descriptors
  -> descriptor validation
  -> DRP2 command stream
  -> stream semantic validation
  -> packet encode/decode conformance
  -> WebGPU replay
```


## Resolved Runtime Descriptors

Add a small internal descriptor layer owned by scene runtime emission.

Initial descriptor families:

1. `DvzSceneResolvedShader`
   - stable vertex key;
   - stable fragment key;
   - shader format;
   - vertex stage;
   - fragment stage;
   - vertex source pointer and byte size;
   - fragment source pointer and byte size;
   - optional built-in identity metadata;
   - pass role and visual family provenance.

2. `DvzSceneResolvedPipeline`
   - stable pipeline key;
   - shader descriptor reference or shader ids after allocation;
   - vertex layout;
   - bind group layout requirements;
   - color/depth targets;
   - blend, raster, multisampling, and topology state;
   - pass role and visual family provenance.

3. `DvzSceneResolvedResource`
   - stable key;
   - resource kind and role;
   - size, format, usage, sample count, extent, and stride where applicable;
   - ownership and lifetime class;
   - FramePlan node provenance.

4. `DvzSceneResolvedReadback`
   - source resource;
   - byte size;
   - row pitch and alignment requirements;
   - decode profile;
   - result routing metadata.

These descriptors are not public API. They are an internal contract between scene FramePlan
preparation and DRP2 command emission.


## Key Policy

Persistent runtime keys must be non-empty, bounded, and typed by role.

Rules:

1. `_obj_id()` must not accept empty keys for persistent objects.
2. `_resource_id()` and related persistent resource helpers must reject empty keys unless the caller
   uses a separate explicit anonymous/transient API.
3. Shader, pipeline, bind-group-layout, bind-group, sampler, buffer, texture, and readback keys
   should have role-specific prefixes or typed wrappers.
4. Anonymous keys are allowed only for transient objects whose lifetime is one emitted stream or
   one command encoder.
5. If an empty key reaches runtime emission, emit a diagnostic before any object id is allocated.

The main objective is to prevent an invalid key from poisoning a persistent id map and causing a
later cache hit to skip required setup commands.


## Shader Policy

Shader resolution should have one canonical C-side path for normal render passes and query/picking
passes.

Rules:

1. Resolve shader key, stage, format, source pointer, and source size before object-id allocation.
2. In WGSL mode, a shader source must be non-null and non-empty.
3. In GLSL/SPIR-V mode, the chosen source or embedded resource must be non-null and non-empty.
4. Query shader overrides must replace or derive keys and sources through the same resolver used by
   normal visual shaders.
5. Shader descriptors must carry visual family and pass role provenance into diagnostics.
6. DRP2 `CreateShaderModule` should receive an already-valid resolved shader descriptor.

Validation errors should look like:

```text
scene runtime shader validation failed:
  visual=point pass=query format=wgsl stage=fragment key=_fs_pointg_query_u32
  reason=missing WGSL source
```


## Packet Policy

DRP2 packets should remain a transport format, not the first validation boundary.

Required packet-level checks:

1. Encode/decode round trip for every browser-promoted command shape.
2. Packet phase conformance: setup commands only in setup packets, writes in update packets, draw
   commands in frame packets.
3. Payload span validation for shader, buffer, and texture payloads.
4. Packet fixture tests for scene-generated streams, not only hand-authored DRP2 fixtures.
5. A narrow Node probe per promoted WASM feature that decodes setup/update/frame packets and asserts
   the expected command skeleton before the WebGPU runtime runs.

For shader modules, packet tests should assert:

1. non-empty stage;
2. non-empty format;
3. non-empty payload;
4. payload size matches decoded source byte length plus terminator for text shaders;
5. render pipelines reference shader module ids that appear in the same or retained setup state.


## Diagnostics

Diagnostics should cross the same boundaries as data.

Add or standardize diagnostics at these layers:

1. Scene API and portable scenario requirements.
2. FramePlan graph validation.
3. Runtime descriptor validation.
4. DRP2 stream semantic validation.
5. Packet encode/decode validation.
6. WebGPU replay validation.
7. Browser scenario/smoke harness reporting.

Each diagnostic should include:

1. layer;
2. feature or scenario id when available;
3. visual family or resource role when available;
4. pass role or packet phase when available;
5. command type and command index when available;
6. stable object/resource key when available;
7. short reason.

Avoid temporary `fprintf(stderr, ...)` debugging as the normal workflow. Temporary traces are still
acceptable during local investigation, but they should be removed before validation and replaced by
structured diagnostics when the information is generally useful.


## WASM ABI Debug Surface

Keep the public browser ABI small, but expose enough debug metadata for tooling.

Candidate debug-only ABI functions:

```c
uint32_t dvz_wasm_api_last_emit_stage(uint32_t scene);
uint32_t dvz_wasm_api_last_packet_command_count(uint32_t scene, uint32_t kind);
uint32_t dvz_wasm_api_last_drp2_validation_count(uint32_t scene);
uint32_t dvz_wasm_api_last_drp2_validation(uint32_t scene, uint32_t index);
```

These should be optional diagnostics, not browser runtime dependencies.

Rules:

1. Browser replay must still depend only on DRP2 packets and capability negotiation.
2. Debug metadata must be borrowed with the same lifetime rules as diagnostics.
3. Debug metadata must not expose native pointers or internal struct layouts.
4. Debug metadata should be compiled into the WASM scene target while the browser path is
   experimental.


## JavaScript Runtime Hardening

The WebGPU runtime should reject malformed DRP2 input deterministically before calling WebGPU when
possible.

Required checks:

1. Unknown command id.
2. Unsupported command in the active browser subset.
3. Missing object id references.
4. Shader module with empty stage or source.
5. Pipeline referencing missing shader modules or bind group layouts.
6. Bind group entry referencing missing resources.
7. Buffer binding offset/size alignment errors.
8. Texture copy row-pitch errors.
9. Readback size and mapping errors.

WebGPU errors should still be captured, but most invalid Datoviz streams should fail in DRP2/WebGPU
preflight before reaching asynchronous WebGPU validation.


## Validation Ladder

Every WASM/WebGPU feature should have progressively broader validation:

1. Native C unit or integration test for FramePlan/runtime descriptor validity.
2. DRP2 stream semantic test.
3. Packet encode/decode test.
4. Node packet probe using the WASM scene module.
5. WebGPU runtime fixture or smoke test.
6. Browser-visible scenario smoke when user interaction or presentation is involved.

For query/readback features, add:

1. query plan validation;
2. query setup packet skeleton validation;
3. query frame packet skeleton validation;
4. synthetic readback decode test;
5. browser readback smoke.


## Implementation Phases

### Phase 1: Guardrails

1. Reject empty persistent object and resource keys.
2. Add shader descriptor validation before object-id allocation.
3. Add packet-level assertions for non-empty browser shader modules.
4. Add a Node packet probe for every currently promoted portable scenario.
5. Remove diagnostic-only source changes before final validation in each checkpoint.

Expected outcome: common malformed-stream bugs fail before WebGPU pipeline creation.


### Phase 2: Descriptor Extraction

1. Introduce `DvzSceneResolvedShader`.
2. Move normal render shader resolution and query shader overrides into one resolver.
3. Introduce `DvzSceneResolvedPipeline`.
4. Validate pipeline descriptors before DRP2 emission.
5. Keep DRP2 emission as a translation from resolved descriptors to commands.

Expected outcome: shader/pipeline bugs become local C-side descriptor failures.


### Phase 3: Resource And Readback Contracts

1. Introduce `DvzSceneResolvedResource`.
2. Validate resource keys, size, usage, format, extent, sample count, and lifetime.
3. Introduce `DvzSceneResolvedReadback`.
4. Validate row pitch, byte size, alignment, and decode profile before emitting copy/readback
   commands.
5. Add synthetic readback decode tests independent of browser GPU execution.

Expected outcome: readback and query bugs become resource/readback descriptor failures.


### Phase 4: Tooling

1. Add a reusable Node packet-inspection CLI for WASM scene scenarios.
2. Print compact command summaries by phase.
3. Add expectation files for promoted browser scenarios.
4. Include diagnostics and command indices in browser smoke failures.
5. Keep packet JSON export as a debug artifact, not the hot path.

Expected outcome: agents and humans can inspect the browser-bound stream without ad hoc scripts.


### Phase 5: Promotion Gates

Update the WASM/WebGPU promotion rule so a feature is not considered browser-supported until:

1. resolved descriptor validation exists for the feature;
2. packet skeleton validation exists;
3. WebGPU runtime preflight covers the command shapes;
4. browser smoke evidence exists for the user-visible behavior;
5. public docs identify unsupported variants and diagnostics.


## Suggested File Boundaries

The exact names may change during implementation, but the module boundaries should be explicit:

```text
src/scene/runtime/resolve_shader.c
src/scene/runtime/resolve_pipeline.c
src/scene/runtime/resolve_resource.c
src/scene/runtime/validate_resolved.c
src/scene/runtime/emit_drp2.c
src/wasm/scene_api.c
tools/wasm_packet_probe.mjs
web/drp2/preflight.js
```

Avoid growing `scene_api.c`, render emission files, or visual registry files with unrelated
debugging logic. Put reusable validation and inspection code in named modules.


## Acceptance Criteria

This refactor is successful when:

1. A missing WGSL source fails in C before packet encoding.
2. An empty persistent object/resource key fails in C before id allocation.
3. A malformed packet fails in packet decode/preflight before WebGPU object creation.
4. Browser smoke failures include the scenario id, packet phase, command index, and reason.
5. The common debugging path uses reusable probes and structured diagnostics, not temporary
   `fprintf()` traces.
6. A new browser feature can be promoted by adding descriptor validation, packet expectations, and
   smoke coverage without inventing a new debugging workflow.


## Near-Term Recommendation

For the current v0.4 browser work, do Phase 1 before broadening query/readback scope. Then do the
smallest useful part of Phase 2: centralize resolved shader validation for render and query passes.

That gives the most immediate return because shader/key/cache failures are currently the easiest
class of bugs to push too far downstream into packet decoding or WebGPU pipeline creation.
