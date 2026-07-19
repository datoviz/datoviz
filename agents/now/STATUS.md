# Datoviz v0.4 Status

Status: active v0.4 release candidate. Updated: 2026-07-18.

Keep this file short. Durable behavior belongs in `spec/`; completed history belongs in git
history, not in agent archives.


## Current Pickup

The Release logging and implicit-validation blocker documented in
[HANDOFF_RC1_RELEASE_SILENCE.md](HANDOFF_RC1_RELEASE_SILENCE.md) is closed. Replacement `Wheels`
run `29644925786` built commit `ea06c5cdf0e7a267341b5834419d7854959399dd`; all 29 build and
installed-wheel jobs passed across the six target OS/architecture lanes and Python 3.10 through
3.14. Hosted conformance run `29645577693` passed all six lanes and its aggregate report gate.

The macOS terminal-IPython hosted window close hang is resolved by `9c1e60912`. Keep close/reopen
in physical-machine RC validation; use
[HANDOFF_IPYTHON_RUN_CLOSE_HANG.md](HANDOFF_IPYTHON_RUN_CLOSE_HANG.md) as the completed record.

Linux staging packages shaderc under the exact CMake runtime basename `libshaderc_shared.so.1`,
resolving the clean-host failure exposed by #460. Active workflows use Node 24-native GitHub action
majors; the third-party `ilammy/msvc-dev-cmd@v1` has no Node 24 release yet.

Physical MacBook M3 validation used the exact arm64 wheel from run `29644925786`, SHA-256
`21c1f68e852d92c7a8134867c5f5455442a37f73dfb57b8b40752fce871a26e2`. Both unattended
profiles, deterministic captures, C/Python decoded-pixel parity, installed examples, shaderc, and
the CMake consumer passed; the maintainer approved all seven attended interaction scenarios.
Physical evidence intake run `29645582130` accepted the bundle. The synced seven-machine report at
`build/physical-evidence/report/index.html` passes every gate with no missing machines.

The RC1 source bundle/checksums and release notes are closed. All six wheels are on TestPyPI and
PyPI. TestPyPI verification run `29652477816` and PyPI verification run `29666589331` matched every
indexed file byte-for-byte to canonical Wheels run `29644925786` and passed all six clean
installed-package smokes plus their aggregate gates. The tag and GitHub draft exist. Next critical
path: update the public install guidance and draft evidence, then complete separately approved
GitHub release and documentation publication.

Completed runtime cleanup to keep in validation: DRP2 render-pass begin commands now carry explicit
render area, viewport, and scissor rectangles, scene emission initializes full targets before panel
passes, and mixed plain/MSAA panels use an explicit-region resolve path so panel resolves do not
clobber earlier framebuffer contents.

RC1 execution order:

1. Keep the v0.4 Git history cleanup deferred; do not rewrite RC or final release refs.
2. Keep the now-green wheel matrix in release evidence and inspect downloaded artifacts before
   upload.
3. Prepare the RC1 source bundle, release notes, tag, and publication rehearsal.
4. Install, inspect, and smoke-test the built wheels, including native dependencies and the CMake
   consumer check.
5. Keep the v0.3 visible parity audit and public API/status disposition table reconciled through
   RC1: specifically re-check `docs/reference/feature-status.md`,
   `docs/reference/project-status.md`, `docs/reference/v03-visible-parity.md`, generated C API
   docs, Python binding docs/policy, and the GSP/VisPy2 boundary language for contradictions.
6. Polish the WebGPU/WASM story: supported live routes, experimental scope, diagnostics, and
   non-parity boundaries.
7. Proofread the public docs, gallery pages, generated matrices, screenshots, and example metadata.
8. Publish the RC1 tag and artifacts only after final validation and release notes are recorded.
9. Keep the narrow retained visual item-range slice in validation: the point-first
   `dvz_visual_set_item_range()` / clear/get API is active, with broader attribute views, scalar
   GPU mappings, modifiers, compaction, sorting, indirect draw, and additional visual families
   deferred unless a concrete RC blocker appears.

Pre-RC repository hygiene decision: Git history cleanup is deferred beyond v0.4. Do not rewrite
RC or final release refs; any future cleanup requires a separate coordinated plan.

Release decision: explicit linear `f16`/`f32` scientific image export/readback is deferred beyond
RC1. The v0.4.0 capture contract is sRGB RGBA8 screenshot/export pixels.

Pre-RC architecture refactor status: complete. The mechanical API tiering, backend-neutral render
types, DRP2 metadata authority, WASM bridge split, and panel render planner checkpoint commits have
landed on `v0.4-dev`; optional scene internal-state splitting is deferred. Do not restart the old
architecture plan from agent notes.

Blockers:

| Lane | Status | Next proof |
| --- | --- | --- |
| C/C++ distribution preflight | Replacement Wheels run `29644925786` passed all 29 jobs at `ea06c5cdf`; hosted conformance run `29645577693` passed all six lanes; physical M3 intake `29645582130` records both unattended profiles and all seven attended checks as passing. The merged seven-machine report passes every gate. TestPyPI verification run `29652477816` and PyPI verification run `29666589331` matched and smoke-tested all six indexed wheels. | Switch public installation guidance to PyPI, preserve the verified bytes, and retain both package-index reports with the release record. |
| Windows wheel proof | The July 2026 pass fixes Win32 `min`/`max`, configured wheel paths, exported C11 requirements, duplicate MSVC pthread implementations, and the ARM64 shaderc omission. Windows uses static shaderc. Hosted run `29624999442` built and inspected AMD64/ARM64 wheels, confirmed architecture, and passed installed shader-resource, shaderc, render, and Python smokes on both architectures. Physical Windows AMD64 validation passed end to end; this is provisional because the corrected Release matrix changes the artifact checksum. | Repeat or confirm physical AMD64 proof against the exact replacement artifact. |
| WebGPU/WASM experimental path | WebGPU fixture runner works; the generic WASM scene ABI and split DRP2 packet path now register 90 scenarios and expose 86 browser-live gallery routes. Point-cloud public route metadata references a hashed 500k-point bundle; native capture and deterministic WASM packet proof pass, while its local filtered browser route reaches the known external headless instance-loss skip. SVG Tiger has headed browser proof and an approved committed prepared-data bundle attributed to Nicolas P. Rougier's Glumpy example gallery. | Confirm point-cloud redistribution authorization and manually verify its public website route, then continue generic volume and rendering techniques. |
| Compute+graphics experimental path | DRP2 `ResourceBarrier`, FramePlan scene compute lowering, WebGPU fixture parity, and the C `gpu_particle_smoke` showcase are active. CPU command-generation proof passed on 2026-06-04. Native Vulkan compute+graphics proof passed on 2026-06-17: `test_frame_plan_emitter_runtime_compute_two_frames_glsl_executes`, `test_vklite_compute_1`, `test_technique_compute_graphics`, and `examples/c/showcases/gpu_particle_smoke.c --png` with artifact `build/release-evidence/gpu_particle_smoke.png`. | Keep the slice classified as experimental in the feature/status table: native proof exists, but this is a narrow scene-compute/DRP2 interop path, not a general compute framework. |
| Qt/PyQt hosted path | Native Qt hosting and `datoviz_qtbridge` are implemented and locally proven from source. On 2026-06-18, `DVZ_CMAKE_ARGS="-DDVZ_ENABLE_QT_BRIDGE=ON" just build`, `./build/examples/qt/hosted_qt_smoke 120`, `./build/examples/qt/qt_hosting --smoke-ms 1000`, `DATOVIZ_QTBRIDGE_LIBRARY=build/qtbridge/libdatoviz_qtbridge.so uv run --isolated --with PyQt6 python -m datoviz.qt`, and the hosted PyQt smoke passed after updating the example to pass `DvzColor` to the raw background-color API. The isolated probe reported bridge Qt 6.11.1 and PyQt Qt 6.11.0; the system PyQt6 package in this shell lacks `QVulkanInstance` and is not a valid PyQt hosting proof. Canonical RC1 wheels include `datoviz.qt` and the `datoviz[qt]` extra but not the native bridge, so Qt hosting is source-build-only in RC1. | Deliver and validate a packaged provider for RC2, preferably conda-first. Keep diagnostics explicit for a missing bridge, unsupported PyQt/PySide bindings, and Qt runtime mismatches; retain base-wheel checks with `--qt-probe optional`. |
| v0.3 visible parity audit | Public table landed in `docs/reference/v03-visible-parity.md`, classifying visible v0.3-era capabilities as fixed, experimental, deferred, or external/GSP without preserving old APIs. | Keep it reconciled with feature/status docs, installed headers, generated C reference, and known gaps before RC1. |
| Public API/status cleanup | Completed and merged to `v0.4-dev` by the July 2026 pre-RC campaign. The cleanup removed unused legacy/internal public APIs, collapsed transitional aliases, normalized naming and argument ordering, tightened ownership/constness, made stable app-facing names backend-neutral, classified DRP2/vklite protocol escape hatches as advanced/unstable, kept `datoviz.raw` exact while fixing known ownership traps, and converted stable fallible mutators to `DvzResult` where appropriate. | Keep generated C reference, Python binding docs/policy, public status docs, and examples reconciled before RC1. |
| Release example proof | Partial for the full RC, but the 2026-06-09 `EXAMPLES_NOTES.md` ledger is closed: source/gallery polish, `showcases/surface_grid`, `features/bounds_overlay`, runtime/readability fixes, scenario-helper audit, comment metadata audit, and builtin-shapes parity audit are resolved with native smoke or explicit audit evidence. | Continue broader release proof outside `EXAMPLES_NOTES.md`: visible parity table, API disposition, and any additional focused native evidence where the environment supports Vulkan. |

Closed first slices that should stay in validation: frame artifact scene emission, Python binding,
retained textured mesh, retained DATA-coordinate visual attachments, color management, text, 2D
axes/ticks, colorbars, labels/readouts, scale bars, app/offscreen rendering, broad item/sample query
paths, scene visual-boundary checks, WebGPU fixture runner, and WASM frame artifact packet scene
smoke. Before changing sampled-field texture roles, shader color linearization, render-target color
formats, or screenshot/readback encoding, use
[`../../spec/scene/implementation/COLOR_MANAGEMENT_IMPLEMENTATION_PLAN.md`](../../spec/scene/implementation/COLOR_MANAGEMENT_IMPLEMENTATION_PLAN.md)
as the color-management audit checklist.

Python binding direction: Datoviz has one generated `ctypes` binding. Use top-level
`import datoviz as dvz` for the normal `dvz_*` call form with policy-declared NumPy adaptation; use
`datoviz.raw` only for exact pointers, counts, bytes, callbacks, and ABI debugging. Source of truth:
[../../spec/bindings/ARRAY_FACADE.md](../../spec/bindings/ARRAY_FACADE.md).

GSP backend RC lane: the readiness checklist for making Datoviz a stable GSP/Matplotlib rendering
target is [../../spec/api/GSP_BACKEND_READINESS.md](../../spec/api/GSP_BACKEND_READINESS.md).
First RC slices are complete: direct Python `dvz_visual_set_data_many()` mapping facade
(`e898a7369`), `dvz_view_capture_rgba()` (`227bbd97d`), direct-engine docs/example/smoke
(`f88f5d82f`), pixel/capture semantics docs (`7b607090d`), and adjacent-panel scissor proof
(`b2a562f7f`). Remaining work is optional unless GSP integration finds a concrete blocker;
alpha-preserving PNG bytes can be deferred.


## Active Lanes

1. **Release closure:** visible parity audit, final API/status reconciliation, known gaps, release
   notes, source bundle/checksum, artifact inspection, and publication rehearsal.
2. **Example proof:** C examples and fixture smokes for the declared release surface, especially
   one short feature example per public v0.4 feature, retained textured mesh, and composed
   annotation/layout examples.
3. **WebGPU/WASM RC examples:** portable scenario host, native event/query scenario migrations,
   browser scenario event delivery, live website examples for most non-desktop scene scenarios,
   current subset diagnostics, compute particles, narrow request/query/readback,
   manifest-backed example classification, and browser/runner smoke evidence. Live gallery routes
   must reuse the same C example or portable C scenario as native validation; browser code is host
   glue only. Keep automated browser smoke representative rather than exhaustive: a basic runtime
   route, a query/readback route, a compute route, and a few targeted smoke rows for newly promoted
   capability clusters are enough. Broad `wasm-*` sampler pages are development aids, not the
   public promotion surface.
4. **Compute+graphics:** minimal DRP2 sync objects/barriers, native compute-to-render proof,
   WebGPU parity diagnostics, and the C-first particle-smoke gallery target remain active. Native
   Vulkan proof is recorded; keep RC work focused on honest experimental status and artifact
   retention rather than broadening into a general compute framework.
5. **Runtime hardening:** concrete scene -> DRP2 -> vklite/canvas/app lifetime, resize, descriptor,
   repeated-frame, or churn bugs.
6. **Qt/PyQt provider:** RC1 source-build-only bridge and diagnostics are proven; deliver a tested
   packaged provider for RC2, preferably conda-first, without changing the base-wheel contract.
7. **Docs inventory:** public header inventory, ownership notes, Python binding scope,
   `datoviz.raw` exact-call scope, WebGPU/WASM scope, known issues, and GSP/VisPy2 boundary.

Current runtime/WebGPU guardrail: keep texture-backed scene visuals on typed visual/draw-contract
DRP2 streams. Do not restore legacy texture-render shortcuts; WebGPU parity work should validate
capability diagnostics against the same streams used by native runtime execution. Do not promote a
browser gallery example by reimplementing its scene, visual state, animation, picking, selection,
query/probe, or data semantics in JavaScript.

WebGPU browser resize/recovery guardrail: the C/WASM scene is durable user state; browser WebGPU
runtime objects and presentation textures are disposable. Browser resize and runtime recovery must
reset the WebGPU runtime plus retained scene emitter, replay setup packets, and preserve
camera/controller/scenario state. Do not recreate the C scenario for transient browser resize or
runtime errors.

Marker triangle status: the GSP visual-QA finding is resolved in Datoviz. Triangle markers use
centered screen-space bounding-box semantics, bbox-normalized triangle vertices, and
counter-clockwise rendered y-up angles; keep marker docs, examples, native shaders, and WebGPU
paths consistent if marker shape or angle semantics change.

Example data guardrail: examples that declare prepared, generated, or external data must not
silently synthesize an in-memory runtime fallback when the expected bundle is absent. Fail with the
exact preparation command instead. Synthetic/simulated examples remain acceptable only when that
data source is explicit in the manifest and example contract.

Lab example guardrail: `examples/c/lab/` examples are deprecated for now and are not part of the
release/public example proof. Do not spend audit, smoke, or API-migration effort there unless the
user explicitly asks for lab-example work in the current turn.

WebGPU example promotion handoff: prefer one commit per example or evidence checkpoint. The current
pattern is: export/reuse the canonical C scenario, register it in `src/wasm/scene_api.c` and
`src/wasm/CMakeLists.txt` only if not already registered, add or verify
`examples/webgpu/live_examples.js`, update `examples/c/MANIFEST.yaml` and generated docs when a
status changes, add targeted stream-shape assertions in `tools/wasm_scene_smoke.mjs`, add a live
route check in `tools/webgpu_browser_smoke.mjs` only when the route exercises a new capability or
protects a release blocker, record the local result in `examples/webgpu/COMPAT.md`, then validate
with the narrow native example, `node --check`, `python3 tools/check_example_manifests.py`,
`just wasm-scene-smoke`, `just webgpu-browser-smoke`, and `git diff --check`. Headless browser runs
may exit successfully while skipping live routes with the known external WebGPU instance-loss
diagnostic before scene rendering; record that honestly.

When adding visible capability work, prefer gallery-proof improvements first, then vector visual
polish, label query hardening, explanatory layout proof, and optional experimental splats only if
release-proof lanes remain on track.


## References

1. [RELEASE.md](RELEASE.md)
2. [DOCUMENTATION.md](DOCUMENTATION.md)
3. [../../spec/scene/examples/PLANNING.md](../../spec/scene/examples/PLANNING.md)
4. [../../spec/scene/api/API_SURFACE.md](../../spec/scene/api/API_SURFACE.md)
5. [../../spec/scene/validation/DEFERRED_TRACKER.md](../../spec/scene/validation/DEFERRED_TRACKER.md)
6. [../../spec/scene/visuals/BOUNDARY_CONTRACT.md](../../spec/scene/visuals/BOUNDARY_CONTRACT.md)
7. [../../spec/scene/integration/OPTIONAL_PROVIDERS.md](../../spec/scene/integration/OPTIONAL_PROVIDERS.md)
8. [../../spec/scene/integration/QT_HOST_BRIDGE.md](../../spec/scene/integration/QT_HOST_BRIDGE.md)
9. [../../spec/bindings/ARRAY_FACADE.md](../../spec/bindings/ARRAY_FACADE.md)
10. [../../docs/reference/feature-status.md](../../docs/reference/feature-status.md)
11. [../../docs/reference/project-status.md](../../docs/reference/project-status.md)
12. [../../spec/scene/integration/WASM_DEBUGGABILITY_REFACTOR_PLAN.md](../../spec/scene/integration/WASM_DEBUGGABILITY_REFACTOR_PLAN.md)
    - includes the 2026-06 WASM stack-overflow failure mode and the required
      ASan/SAFE_HEAP/stack-usage workflow.


## Validation Defaults

Documentation-only:

```sh
git diff --check
git status --short
```

Scene/DRP2/runtime code:

```sh
just build
just test <filter>
just spec-check
```

Add Vulkan validation or bounded GLFW/offscreen smoke for graphics lifetimes, command buffers,
render targets, swapchains, or synchronization.
