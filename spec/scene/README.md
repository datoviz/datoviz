# Scene Spec

This directory defines the scene layer as a consumer of DRP2, not as a backend runtime.

The scene layer should remain pure high-level logic:

1. build and own user-facing visualization state;
2. derive rendering work from that state;
3. emit DRP2 through a runtime-facing contract;
4. stay independent from Vulkan, swapchain, and windowing internals.


## Status

- Status: active specification with multiple implementation slices in `src/scene`.
- Implementation priority: prove the declared v0.4 surface for RC1, especially release examples,
  WebGPU/WASM experimental scope, raw bindings, API/status labeling, and v0.3 visible parity.
- Primary constraint: do not let scene design leak backend details into its public API.

Current source implementation is intentionally smaller than this spec. It includes scene, figure,
panel, retained visual, capability, diagnostic, frame-plan, DRP2 emission, controller,
sampled-field, scale/colormap, colorbar, annotation, scale-bar, query, selection, graph-technique,
app/offscreen, and GLFW paths. Public headers also declare broader interaction, readout, selection,
material, technique, and visual-family behavior that is not fully rendered or semantically complete
yet. Treat broader sections of this spec as design pressure and direction, not as a claim that all
families and interactions are already implemented.


## Start Here

1. [AUTHORITY.md](AUTHORITY.md): DRP2 boundary, normative invariants, status vocabulary, and
   source-of-truth order.
2. [READING_ORDER.md](READING_ORDER.md): recommended reading sequence and topic index.
3. [core/README.md](core/README.md): foundational ownership, object model, runtime boundary, and
   use cases.
4. [api/README.md](api/README.md): public API profile, public header surface, and implementation
   bridge.
5. [semantics/README.md](semantics/README.md): user-visible scene semantics and cross-family
   behavior.
6. [pipeline/README.md](pipeline/README.md): resource, transform, invalidation, frame-plan, and
   lifecycle contracts.
7. [implementation/FRAME_ARTIFACT_REFACTOR_PLAN.md](implementation/FRAME_ARTIFACT_REFACTOR_PLAN.md):
   active scene emission artifact refactor plan.


## Directory Layout

The scene spec is split by kind of authority:

1. [core](core/README.md): foundational ownership, object model, runtime boundary, and use cases.
2. [api](api/README.md): public API profile, public header surface, and implementation bridge.
3. [semantics](semantics/README.md): user-visible scene semantics and cross-family behavior.
4. [pipeline](pipeline/README.md): resource, transform, invalidation, frame-plan, and lifecycle
   contracts.
5. [interaction](interaction/README.md): controllers, picking, selection, callbacks, and animation.
6. [visuals](visuals/README.md): per-family data contracts.
7. [validation](validation/README.md): validation, adaptation, diagnostics, and deferred items.
8. [integration](integration/README.md): host UI, threading, high-DPI, and custom visuals.
9. [export](export/README.md): image export semantics and the current vector-export scope decision.
10. [dashboards](dashboards/README.md): v0.5+ dashboard and dense multi-panel pressure notes.
11. [slices](slices/README.md): implementation-ready work packets for mature spec areas.
12. [headers](headers/README.md): implementation-facing draft C header sketches.
13. [implementation](implementation/README.md): concise notes for active implementation wiring.
14. [proposals](proposals/README.md): active, promoted, future, and historical proposal notes.
15. [decisions](decisions/README.md): historical ADR-style decision records.
16. [composites](composites/README.md): semantic objects that lower to coordinated visuals.
17. [examples](examples/README.md): worked examples and API-shape pressure tests.
18. [ROADMAP.md](ROADMAP.md): compact backlog distilled from former agent queues.


## Guiding Principles

1. Keep pushing scene semantics and producer contracts.
2. Avoid freezing backend-shaped details too early.
3. Let DRP2 and runtime work continue underneath without leaking upward.
4. Keep semantic/domain coordinates authoritative in F64; visual render attributes are lowered to
   GPU-facing F32 unless their family contract says otherwise.

Deferred items by milestone are tracked in
[validation/DEFERRED_TRACKER.md](validation/DEFERRED_TRACKER.md).
