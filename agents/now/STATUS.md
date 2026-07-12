# Datoviz v0.4 Status

Status: active RC preparation. Updated: 2026-07-07.

Keep this file short. Durable behavior belongs in `spec/`; completed history belongs in git
history, not in agent archives.


## Current Pickup

Immediate runtime troubleshooting task:
[HANDOFF_IPYTHON_RUN_CLOSE_HANG.md](HANDOFF_IPYTHON_RUN_CLOSE_HANG.md) records the active
investigation plan for the macOS terminal-IPython hosted window close hang. Handle this before
returning to broader RC stabilization if the user's next pickup is Python/IPython runtime work.

Next critical path: close RC1 release notes, source bundle/checksum, artifact inspection,
publication rehearsal, and final public status/documentation reconciliation.

Completed runtime cleanup to keep in validation: DRP2 render-pass begin commands now carry explicit
render area, viewport, and scissor rectangles, scene emission initializes full targets before panel
passes, and mixed plain/MSAA panels use an explicit-region resolve path so panel resolves do not
clobber earlier framebuffer contents.

Pre-RC1 execution order:

1. Run the v0.4 Git history cleanup if it is still desired before stable RC refs exist.
2. Keep the now-green wheel matrix in release evidence and inspect downloaded artifacts before
   upload.
3. Prepare the RC1 source bundle, release notes, tag, and publication rehearsal.
4. Install, inspect, and smoke-test the built wheels, including native dependencies and the CMake
   consumer check.
5. Keep the v0.3 visible parity audit and public API/status disposition table reconciled before
   RC1: specifically re-check `docs/reference/feature-status.md`,
   `docs/reference/project-status.md`, `docs/reference/v03-visible-parity.md`, generated C API
   docs, Python binding docs/policy, and the GSP/VisPy2 boundary language for contradictions.
6. Polish the WebGPU/WASM story: supported live routes, experimental scope, diagnostics, and
   non-parity boundaries.
7. Proofread the public docs, gallery pages, generated matrices, screenshots, and example metadata.
8. Cut RC1 only after final validation and release notes are recorded.
9. Keep the narrow retained visual item-range slice in validation: the point-first
   `dvz_visual_set_item_range()` / clear/get API is active, with broader attribute views, scalar
   GPU mappings, modifiers, compaction, sorting, indirect draw, and additional visual families
   deferred unless a concrete RC blocker appears.

Pre-RC repository hygiene note: if the v0.4 Git history cleanup remains desired, do it before
`v0.4.0-rc1` and before treating release refs as stable. The agreed process, user migration note,
and force-push guardrails are recorded in [RELEASE.md](RELEASE.md#0-pre-rc-git-history-cleanup).

Release decision: explicit linear `f16`/`f32` scientific image export/readback is deferred beyond
RC1. The v0.4.0 capture contract is sRGB RGBA8 screenshot/export pixels.

Pre-RC architecture refactor status: complete. The mechanical API tiering, backend-neutral render
types, DRP2 metadata authority, WASM bridge split, and panel render planner checkpoint commits have
landed on `v0.4-dev`; optional scene internal-state splitting is deferred. Do not restart the old
architecture plan from agent notes.

Blockers:

| Lane | Status | Next proof |
| --- | --- | --- |
| C/C++ distribution preflight | macOS vendored/system package, strict Homebrew-style source install, installed CMake/pkg-config consumers, host-native wheel CMake-consumer proof, Linux manylinux proof, and Windows AMD64 local wheel proof advanced on 2026-06-18 after `d94f72dd6`, `2c8a49f3d`, the macOS pkg-config validator fix, and `19e62968`; see [C_DISTRIBUTION.md](C_DISTRIBUTION.md) for exact evidence. Hosted wheel CI run `27975460115` passed on 2026-06-22 with Linux x86_64/aarch64, macOS 15 arm64/Intel, Windows AMD64/ARM64, Python 3.10 through 3.14 installed-wheel smokes, and Linux prerelease smoke. Local artifact inspection confirmed expected tags, CMake package files, Windows `datoviz.lib`, and architecture-correct macOS dylibs. | Prepare RC1 source bundle/checksum, release notes, and publication rehearsal; keep the wheel matrix in validation. |
| Windows wheel proof | Wheel workflow fixes are on `v0.4-dev`: Windows matrix/tool setup, MSVC environment setup, vcpkg-provided `glslangValidator` resolution, vendored static Kvazaar `KVZ_STATIC_LIB`, vcpkg binary archive caching, the Datoviz PEP 517 release-wheel backend, and the Windows payload/CMake-consumer fixes from `19e62968`. Local Windows AMD64 wheel proof passed on 2026-06-18, and hosted wheel CI run `27975460115` passed on 2026-06-22 for Windows AMD64 and ARM64 build, inspect, artifact upload, and Windows Python 3.10 through 3.14 installed-wheel smokes. Downloaded Windows artifacts include `datoviz.dll` and `datoviz.lib`. | Keep Windows in validation; no active Windows wheel blocker remains unless a rerun regresses. |
| WebGPU/WASM experimental path | WebGPU fixture runner works; the generic WASM scene ABI and split DRP2 packet path now register 82 scenarios and expose 81 browser-live gallery routes. Picking smoke asserts resolved hover/click output and supports per-route filtering. Direct browser proof passes for the ten 2026-07-11 routes plus graph composite, orientation gizmo, signed categorical label probe, 10,000-point quickstart scatter, and retained UTC datetime axis. | Obtain manual website confirmation for scatter/datetime, then continue the ordered coverage program in `spec/scene/integration/WASM_WEBGPU_PARITY_PLAN.md` with marker-symbol basics, volume, techniques, and splat/point-cloud batches. |
| Compute+graphics experimental path | DRP2 `ResourceBarrier`, FramePlan scene compute lowering, WebGPU fixture parity, and the C `gpu_particle_smoke` showcase are active. CPU command-generation proof passed on 2026-06-04. Native Vulkan compute+graphics proof passed on 2026-06-17: `test_frame_plan_emitter_runtime_compute_two_frames_glsl_executes`, `test_vklite_compute_1`, `test_technique_compute_graphics`, and `examples/c/showcases/gpu_particle_smoke.c --png` with artifact `build/release-evidence/gpu_particle_smoke.png`. | Keep the slice classified as experimental in the feature/status table: native proof exists, but this is a narrow scene-compute/DRP2 interop path, not a general compute framework. |
| Qt/PyQt hosted path | Native Qt hosting and the optional `datoviz_qtbridge` provider are active. On 2026-06-18, `DVZ_CMAKE_ARGS="-DDVZ_ENABLE_QT_BRIDGE=ON" just build`, `./build/examples/qt/hosted_qt_smoke 120`, `./build/examples/qt/qt_hosting --smoke-ms 1000`, `DATOVIZ_QTBRIDGE_LIBRARY=build/qtbridge/libdatoviz_qtbridge.so uv run --isolated --with PyQt6 python -m datoviz.qt`, and the hosted PyQt smoke passed after updating the example to pass `DvzColor` to the raw background-color API. The isolated probe reported bridge Qt 6.11.1 and PyQt Qt 6.11.0; the system PyQt6 package in this shell lacks `QVulkanInstance` and is not a valid PyQt hosting proof. Public docs classify this as `supported, optional provider`. | Keep packaging/install diagnostics explicit for missing bridge, unsupported PyQt/PySide bindings, and Qt runtime mismatches; retain optional wheel checks with `--qt-probe optional`. |
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
6. **Qt/PyQt provider:** optional Qt bridge shared library, dynamic Python loader, binding
   diagnostics, and hosted PyQt smoke proof.
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
