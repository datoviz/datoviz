# Datoviz v0.4 Status

Status: active RC preparation. Updated: 2026-06-22.

Keep this file short. Durable behavior belongs in `spec/`; completed history belongs in git
history, not in agent archives.


## Current Pickup

Next critical path: follow the Pre-RC1 execution order below, then move directly to RC1 release
notes, tag, artifacts, and publication checks.

Pre-RC1 execution order:

1. Run the v0.4 Git history cleanup if it is still desired before stable RC refs exist.
2. Finish the hosted macOS 15 wheel proof for `macosx_15_0_x86_64` and `macosx_15_0_arm64`.
3. Make the GitHub Actions wheel workflow green across the required matrix.
4. Install, inspect, and smoke-test the built wheels, including native dependencies and the CMake
   consumer check.
5. Keep the v0.3 visible parity audit and public API/status disposition table reconciled.
6. Polish the WebGPU/WASM story: supported live routes, experimental scope, diagnostics, and
   non-parity boundaries.
7. Proofread the public docs, gallery pages, generated matrices, screenshots, and example metadata.
8. Cut RC1 only after final validation and release notes are recorded.

Pre-RC repository hygiene note: if the v0.4 Git history cleanup remains desired, do it before
`v0.4.0-rc1` and before treating release refs as stable. The agreed process, user migration note,
and force-push guardrails are recorded in [RELEASE.md](RELEASE.md#0-pre-rc-git-history-cleanup).

Release decision: explicit linear `f16`/`f32` scientific image export/readback is deferred beyond
RC1. The v0.4.0 capture contract is sRGB RGBA8 screenshot/export pixels.

Blockers:

| Lane | Status | Next proof |
| --- | --- | --- |
| C/C++ distribution preflight | macOS vendored/system package, strict Homebrew-style source install, installed CMake/pkg-config consumers, host-native wheel CMake-consumer proof, Linux manylinux proof, and Windows AMD64 local wheel proof advanced on 2026-06-18 after `d94f72dd6`, `2c8a49f3d`, the macOS pkg-config validator fix, and `19e62968`; see [C_DISTRIBUTION.md](C_DISTRIBUTION.md) for exact evidence. Hosted wheel CI on 2026-06-22 confirmed Linux x86_64/aarch64 builds plus installed-wheel smokes, Windows AMD64/ARM64 builds plus installed-wheel smokes, and exposed that current macOS Vulkan/Homebrew runtime libraries require macOS 15. | Prove macOS 15 wheels on `macos-15-intel` and `macos-15`: `macosx_15_0_x86_64` and `macosx_15_0_arm64`. |
| Windows wheel proof | Wheel workflow fixes are on `v0.4-dev`: Windows matrix/tool setup, vcpkg-provided `glslangValidator` resolution, vendored static Kvazaar `KVZ_STATIC_LIB`, vcpkg binary archive caching, the Datoviz PEP 517 release-wheel backend, and the Windows payload/CMake-consumer fixes from `19e62968`. Local Windows AMD64 MinGW wheel proof passed on 2026-06-18, including native dependency inspection, installed-wheel import, `datoviz.cli`, shaderc GLSL compilation, and the installed-wheel CMake consumer. On 2026-06-22, hosted wheel CI run `27966579584` confirmed Windows AMD64 and ARM64 build, inspect, artifact upload, and Windows Python 3.10 through 3.14 installed-wheel smokes. | Keep Windows in validation; no active Windows wheel blocker remains unless a rerun regresses. |
| WebGPU/WASM experimental path | WebGPU fixture runner works; generic WASM scene ABI emits split DRP2 packets for buffer-backed point/pixel positions, basic marker, segment/path with cap/join controls, primitive, RGBA8 image, basic retained 2D axes/ticks/grid labels, basic signed 2D labels, low-level atlas glyph, semantic bitmap text, basic/textured/material mesh, basic sphere, panzoom, 3D arcball/fly/turntable/orbit controller examples, sampled-field/image color-scale routes, panel background, synthetic composed-showcase routes, retained data update/visibility routes, basic depth-test route, alpha blending, material/lighting routes, textured planets, protein, and portable C scenario/frame-callback proof for `feature_timer_animation`. Native scenario runner has requirements, portable event/post-frame hooks, and query/readback-shaped scenario proofs. Browser-live routes now cover 67 examples, including standalone point, pixel, marker, primitive, segment, path, image, mesh, sphere, text, glyph, labels, panel/annotation/layout, image/color-scale, controller, polygon composite, linked-panels axes, scale-bar measurement, surface-grid, U.S. state choropleth, textured planets, protein, retained update/visibility, depth-test, alpha-blending, and material/lighting routes. The frame artifact refactor is complete: scene emission returns artifact-owned stream snapshots, WASM/WebGPU consumes artifact packet spans, JSON is debug/fixture-only, and retained scene mutation no longer depends on raw emitted stream lifetime. | Follow the good-enough RC plan in `spec/scene/integration/WASM_WEBGPU_PARITY_PLAN.md`: next handle remaining non-data composed routes or continue visible parity/API disposition. |
| Compute+graphics experimental path | DRP2 `ResourceBarrier`, FramePlan scene compute lowering, WebGPU fixture parity, and the C `gpu_particle_smoke` showcase are active. CPU command-generation proof passed on 2026-06-04. Native Vulkan compute+graphics proof passed on 2026-06-17: `test_frame_plan_emitter_runtime_compute_two_frames_glsl_executes`, `test_vklite_compute_1`, `test_technique_compute_graphics`, and `examples/c/showcases/gpu_particle_smoke.c --png` with artifact `build/release-evidence/gpu_particle_smoke.png`. | Keep the slice classified as experimental in the feature/status table: native proof exists, but this is a narrow scene-compute/DRP2 interop path, not a general compute framework. |
| Qt/PyQt hosted path | Native Qt hosting and the optional `datoviz_qtbridge` provider are active. On 2026-06-18, `DVZ_CMAKE_ARGS="-DDVZ_ENABLE_QT_BRIDGE=ON" just build`, `./build/examples/qt/hosted_qt_smoke 120`, `./build/examples/qt/hosted_qt_widgets --smoke-ms 1000`, `DATOVIZ_QTBRIDGE_LIBRARY=build/qtbridge/libdatoviz_qtbridge.so uv run --isolated --with PyQt6 python -m datoviz.qt`, and the hosted PyQt smoke passed after updating the example to pass `DvzColor` to the raw background-color API. The isolated probe reported bridge Qt 6.11.1 and PyQt Qt 6.11.0; the system PyQt6 package in this shell lacks `QVulkanInstance` and is not a valid PyQt hosting proof. Public docs classify this as `supported, optional provider`. | Keep packaging/install diagnostics explicit for missing bridge, unsupported PyQt/PySide bindings, and Qt runtime mismatches; retain optional wheel checks with `--qt-probe optional`. |
| v0.3 visible parity audit | Public table landed in `docs/reference/v03-visible-parity.md`, classifying visible v0.3-era capabilities as fixed, experimental, deferred, or external/GSP without preserving old APIs. | Keep it reconciled with feature/status docs, installed headers, generated C reference, and known gaps before RC1. |
| Public API/status cleanup | Initial public status docs landed in `docs/reference/feature-status.md` and `docs/reference/project-status.md`, with the API implementation snapshot in `spec/scene/api/API_IMPLEMENTATION_READINESS.md`; these now link to the v0.3 visible parity table. Pre-RC1 API/ABI/FFI hardening landed on 2026-06-18: raw ctypes and generated C reference now track exported `DVZ_EXPORT` ABI, `dvz_ffi_*` is the explicit FFI helper namespace, owned string returns are policy-marked, and FramePlan packet APIs are intentionally classified advanced/unstable. The agreed RC1 posture is no broad `dvz_ffi_*` wrapper sweep, keep cglm-aligned records opaque in raw ctypes, and document advanced runtime APIs as advanced/unstable unless a specific accidental symbol is found. | No active API/ABI/FFI blocker. Keep future API work focused on specific release examples or accidental-symbol removals, not broad convenience expansion. |
| Release example proof | Partial for the full RC, but the 2026-06-09 `EXAMPLES_NOTES.md` ledger is closed: source/gallery polish, `showcases/surface_grid`, `features/bounds_overlay`, runtime/readability fixes, scenario-helper audit, comment metadata audit, and builtin-shapes parity audit are resolved with native smoke or explicit audit evidence. | Continue broader release proof outside `EXAMPLES_NOTES.md`: visible parity table, API disposition, and any additional focused native evidence where the environment supports Vulkan. |

Closed first slices that should stay in validation: frame artifact scene emission, raw `ctypes`,
retained textured mesh, retained DATA-coordinate visual attachments, color management, text, 2D
axes/ticks, colorbars, labels/readouts, scale bars, app/offscreen rendering, broad item/sample query
paths, scene visual-boundary checks, WebGPU fixture runner, and WASM frame artifact packet scene
smoke. Before changing sampled-field texture roles, shader color linearization, render-target color
formats, or screenshot/readback encoding, use
[`../../spec/scene/implementation/COLOR_MANAGEMENT_IMPLEMENTATION_PLAN.md`](../../spec/scene/implementation/COLOR_MANAGEMENT_IMPLEMENTATION_PLAN.md)
as the color-management audit checklist.

New Python binding direction: keep `datoviz.raw` as the exact generated `ctypes` layer, and make
top-level `import datoviz as dvz` the planned array-aware facade that preserves `dvz_*` names while
accepting NumPy arrays for policy-declared data arguments. Source of truth:
[../../spec/bindings/ARRAY_FACADE.md](../../spec/bindings/ARRAY_FACADE.md).


## Active Lanes

1. **Release closure:** visible parity audit, final API/status reconciliation, known gaps.
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
7. **Docs inventory:** public header inventory, ownership notes, raw `ctypes` scope, array-aware
   Python facade scope, WebGPU/WASM scope, known issues, and GSP/VisPy2 boundary.

Current runtime/WebGPU guardrail: keep texture-backed scene visuals on typed visual/draw-contract
DRP2 streams. Do not restore legacy texture-render shortcuts; WebGPU parity work should validate
capability diagnostics against the same streams used by native runtime execution. Do not promote a
browser gallery example by reimplementing its scene, visual state, animation, picking, selection,
query/probe, or data semantics in JavaScript.

Example data guardrail: examples that declare prepared, generated, or external data must not
silently synthesize an in-memory runtime fallback when the expected bundle is absent. Fail with the
exact preparation command instead. Synthetic/simulated examples remain acceptable only when that
data source is explicit in the manifest and example contract.

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
6. [../../spec/scene/implementation/SCENE_VISUAL_BOUNDARY_GUARDRAILS.md](../../spec/scene/implementation/SCENE_VISUAL_BOUNDARY_GUARDRAILS.md)
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
