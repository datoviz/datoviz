# Post-v0.4 Refactor Roadmap

> **Execution Status**
> - **Status:** `POST-V0.4 ROADMAP`
> - **Updated on:** `2026-05-26`
> - **Purpose:** keep useful refactors visible without making them v0.4 release blockers.

This file lists refactoring work that should wait until after the public `v0.4.0` release unless a
specific item becomes necessary to fix a release-blocking bug. The v0.4 release branch should focus
on supported-surface honesty, WebGPU/WASM experimental proof, raw `ctypes`, release examples,
runtime hardening, and live validation of landed retained scale-bar update-performance behavior.


## Not v0.4 Release Blockers

These tasks are useful, but they should not delay `v0.4.0` by themselves.

1. Split `dvz_scene_json()` into focused append helpers.
2. Extract broad scene constructor/slot-allocation helpers for fields, buffers, visuals, and owned
   image-field creation.
3. Perform a large `src/scene/scene.c` file split or translation-unit reshuffle.
4. Migrate more low-risk graphics tests into shared app/offscreen or shared GPU fixtures.
5. Add test-runner resource-capacity accounting, in-process thread workers, and reporting polish.
6. Tighten component-owned header install sets after package-consumer smoke tests are stable.
7. Move app DRP2 trace fingerprint/snapshot normalization into a shared DRP2 diagnostics layer for
   the render-conformance framework.
8. Clean up graph technique builders when the change only reduces local clutter and does not fix
   stream output, descriptor lifetime, pass ordering, or a release example.
9. Replace repeated shader ABI switch tables with descriptor tables when there is no active
   shader/cache-key bug.
10. Build shared atlas/resource substrates for bitmap markers, marker/text SDF/MSDF sharing, or
    future text-layout infrastructure.
11. Extract or table-drive the visual-family pick/probe operations currently concentrated in
    `src/scene/request_execute.c`, while preserving the no-CPU-visual-picking contract.


## First Post-v0.4 Refactor Batch

The most valuable first batch after release is structural scene cleanup with focused tests:

1. finish the lower-risk pieces from
   [SCENE_C_REFACTOR_NOTES_2026-05-11.md](SCENE_C_REFACTOR_NOTES_2026-05-11.md) that were not
   needed for v0.4, plus the request-path-specific follow-up in
   [SCENE_PICK_PROBE_REQUEST_PATH_REFACTOR.md](SCENE_PICK_PROBE_REQUEST_PATH_REFACTOR.md);
2. move DRP2 stream fingerprint/snapshot normalization out of app tracing and into a DRP2-owned
   diagnostics layer;
3. add out-of-tree package-consumer smoke tests for component targets before tightening installed
   header ownership;
4. migrate only stable, single-frame graphics tests into shared fixtures and keep failure-path,
   resize, pick/probe, GLFW, video, and device-lost tests isolated until audited.


## Guardrails

1. Do not mix broad mechanical refactors with new visual families or API expansion.
2. Keep behavior-preserving scene refactors covered by `just build`, `just test scene`, and the
   narrow focused test for the touched subsystem.
3. Keep package/install refactors covered by out-of-tree consumer smokes before changing installed
   header ownership.
4. Keep runner scheduling changes generic in `testing/`; Datoviz-specific GPU, GLFW, video, and
   environment behavior should stay in module tests or runner adapters.
5. Do not combine request-path structure cleanup with new pick targets or new visual-family picking
   semantics unless the feature is required to fix a release-blocking defect.
