# DRP2 WebGPU Roadmap

Status: strategic roadmap distilled from the former agent backlog.

DRP2 should remain the shared contract for native and browser execution. WebGPU pressure is useful
only when it prevents native-only assumptions from entering DRP2 or scene semantics.


## Ground Rules

1. DRP2 command, validation, error, lifetime, and capability semantics are backend-agnostic.
2. Public DRP2 headers must not expose Vulkan, window, canvas, or browser types.
3. WGSL is the portable shader language; native runtimes may additionally accept GLSL/SPIR-V behind
   capability flags.
4. Browser support is not a fork of scene semantics.
5. Native interop remains opt-in, advanced, capability-gated, and outside portable DRP2 headers.
6. Owned and borrowed resource lifetimes must be explicit and validated in every runtime.
7. Large-data users need memory budget reporting, actionable OOM errors, deterministic headless
   readback, and profiling hooks before broad stabilization.


## Current Baseline

The native path already exercises:

```text
scene frame plan -> DRP2 stream -> vklite runtime -> canvas/stream/app
```

The experimental browser path has a pure WebGPU runner for the current subset and a narrow
scene/WASM point/panzoom bridge that emits WGSL DRP2 JSON into that runtime. The release target is a
documented subset with explicit unsupported-feature diagnostics, not native parity.


## Phase Order

1. **Contract freeze:** keep the minimal DRP2 command set versioned, schema-backed, and free of
   backend type leakage.
2. **Semantic core:** validate handles, generations, command order, resource usage, ownership,
   thread-safety rules, memory budgets, and deterministic replay before backend execution.
3. **Native runtime:** map DRP2 to Vulkan through runtime/backend code only, with deterministic
   offscreen readback and validation-layer coverage.
4. **Browser WebGPU runtime:** replay the same fixtures in browser, with matching capabilities,
   diagnostics, lifecycle errors, and resource retention behavior.
5. **WASM transport:** harden the portable scene/DRP2 WASM bridge, keep pointer ownership and
   diagnostics explicit, and broaden scene-emitted WGSL streams beyond the point slice.
6. **Renderer v1 parity:** add dynamic viewport/scissor, multiple bind groups, texture sampling,
   compute, explicit synchronization, and thread-safe submission semantics.
7. **Performance and reliability:** add decode/record/submit/update benchmarks, long-run churn
   tests, invalid-stream tests, OOM tests, and leak checks.
8. **Post-v1 memory:** add zero-copy streaming, persistent mapped buffers, sparse/virtual resources,
   placement/aliasing policies, and multi-GPU workflows only after the v1 contract is stable.


## Practical Browser Subset

The first browser subset should stay small:

1. point;
2. primitive;
3. image;
4. one basic depth-tested mesh scene;
5. explicit failures for unsupported shader formats, commands, formats, and features.

Promotion of a deferred command must update DRP2 specs, schemas, native validation, WebGPU
execution, fixtures, lifecycle rules, and capability reporting together.


## Strategic Backlog

1. capability schema for precision, formats, alignment, row pitch, memory budgets, and unsupported
   feature reporting;
2. generation-safe object registry and use-after-destroy validation;
3. mock backend tests for semantic validation;
4. native and browser fixture parity;
5. compute pass and deterministic compute/reduction fixtures;
6. public timing/counter API and benchmark baselines;
7. memory budget, OOM, eviction, and leak diagnostics;
8. native interop buffer/image export/import with explicit synchronization;
9. browser conformance replay suite;
10. versioned WASM -> JS command transport.


## Acceptance Gates

1. minimal DRP2 contract and schemas are stable;
2. semantic validation passes with a mock backend;
3. native rendering fixtures pass through Vulkan runtime;
4. browser WebGPU fixtures pass with parity diagnostics;
5. WASM scene emission can feed the browser runtime for the agreed experimental subset;
6. native/browser contract parity covers the agreed renderer v1 slice;
7. performance and reliability evidence is tracked before broad feature expansion.
