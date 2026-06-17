# Datoviz v0.4 Status

Status: active RC preparation. Updated: 2026-06-17.

Keep this file short. Durable behavior belongs in `spec/`; completed history belongs in git
history, not in agent archives.


## Current Pickup

Next critical path: RC1 release proof.

Pre-RC repository hygiene note: if the v0.4 Git history cleanup remains desired, do it before
`v0.4.0-rc1` and before treating release refs as stable. The agreed process, user migration note,
and force-push guardrails are recorded in [RELEASE.md](RELEASE.md#0-pre-rc-git-history-cleanup).

Color management is closed for the v0.4 core rendering/screenshot path: sampled-field color roles,
role propagation through FramePlan/DRP2, shader-side semantic color linearization, final sRGB
screenshot/readback behavior, and focused GPU/offscreen fixtures are landed. Use
[`../../spec/scene/implementation/COLOR_MANAGEMENT_IMPLEMENTATION_PLAN.md`](../../spec/scene/implementation/COLOR_MANAGEMENT_IMPLEMENTATION_PLAN.md)
as the audit checklist before changing sampled-field texture roles, shader color linearization,
render-target color formats, or screenshot/readback color encoding.

Release decision: explicit linear `f16`/`f32` scientific image export/readback is deferred beyond
RC1. The v0.4.0 capture contract is sRGB RGBA8 screenshot/export pixels.

Blockers:

| Lane | Status | Next proof |
| --- | --- | --- |
| Windows wheel proof | Wheel workflow fixes are on `v0.4-dev`: Windows matrix/tool setup, vcpkg-provided `glslangValidator` resolution, vendored static Kvazaar `KVZ_STATIC_LIB`, and vcpkg binary archive caching. GitHub Actions confirmed Linux and macOS arm64 wheel paths during probe runs; Windows remains the active unknown because hosted runners are slow and logs arrive late. | Prefer local Windows iteration before more GHA polling: warm `C:/vcpkg`, set `VCPKG_ROOT=C:/vcpkg`, `VCPKG_BINARY_SOURCES=clear;files,C:/vcpkg-binary-cache,readwrite`, `DVZ_CMAKE_ARGS=-DDVZ_ENABLE_SHADERC=ON`, then run `just build`, `just ctypes`, `python tools/release_wheels/stage_wheel.py --clean`, `python tools/release_wheels/build_wheel.py --dist-dir wheelhouse --platform-tag win_amd64`, `just wheel-inspect --wheel wheelhouse/*.whl --native-deps`, and `python tools/release_wheels/check_wheel.py --wheel wheelhouse/*.whl --cmake-consumer --qt-probe optional`. If local AMD64 is green, push `v0.4-dev` and dispatch full wheel CI for Windows ARM64/macOS x86_64 confirmation. |
| WebGPU/WASM experimental path | WebGPU fixture runner works; generic WASM scene ABI emits split DRP2 packets for buffer-backed point/pixel positions, basic marker, segment/path with cap/join controls, primitive, RGBA8 image, basic retained 2D axes/ticks/grid labels, basic signed 2D labels, low-level atlas glyph, semantic bitmap text, basic/textured/material mesh, basic sphere, panzoom, 3D arcball/fly/turntable/orbit controller examples, sampled-field/image color-scale routes, panel background, synthetic composed-showcase routes, retained data update/visibility routes, basic depth-test route, alpha blending, material/lighting routes, textured planets, protein, and portable C scenario/frame-callback proof for `feature_timer_animation`. Native scenario runner has requirements, portable event/post-frame hooks, and query/readback-shaped scenario proofs. Browser-live routes now cover 68 examples, including standalone point, pixel, marker, primitive, segment, path, image, mesh, sphere, text, glyph, labels, panel/annotation/layout, image/color-scale, controller, polygon composite, linked-panels axes, scale-bar measurement, surface-grid, U.S. state choropleth, textured planets, protein, retained update/visibility, depth-test, alpha-blending, and material/lighting routes. The frame artifact refactor is complete: scene emission returns artifact-owned stream snapshots, WASM/WebGPU consumes artifact packet spans, JSON is debug/fixture-only, and retained scene mutation no longer depends on raw emitted stream lifetime. | Follow the good-enough RC plan in `spec/scene/integration/WASM_WEBGPU_PARITY_PLAN.md`: next handle remaining non-data composed routes or continue visible parity/API disposition. |
| Compute+graphics experimental path | DRP2 `ResourceBarrier`, FramePlan scene compute lowering, WebGPU fixture parity, and the C `gpu_particle_smoke` showcase are active. CPU command-generation proof passed on 2026-06-04. Native Vulkan compute+graphics proof passed on 2026-06-17: `test_frame_plan_emitter_runtime_compute_two_frames_glsl_executes`, `test_vklite_compute_1`, `test_technique_compute_graphics`, and `examples/c/showcases/gpu_particle_smoke.c --png` with artifact `build/release-evidence/gpu_particle_smoke.png`. | Keep the slice classified as experimental in the feature/status table: native proof exists, but this is a narrow scene-compute/DRP2 interop path, not a general compute framework. |
| Qt/PyQt hosted path | Native Qt hosting and the optional `datoviz_qtbridge` provider are active. Native Qt smokes passed on 2026-06-17: `hosted_qt_smoke 120` and `hosted_qt_widgets --smoke-ms 1000`. PyQt bridge probe and `examples/python/qt/hosted_pyqt.py --smoke-ms 1000` passed with an isolated PyQt6 wheel and `DATOVIZ_QTBRIDGE_LIBRARY=build/qtbridge/libdatoviz_qtbridge.so`; the system PyQt6 package in this shell lacks `QVulkanInstance` and is not a valid PyQt hosting proof. | Classify as `supported, optional provider` once the feature/status table lands; keep packaging/install diagnostics explicit for missing bridge, unsupported PyQt/PySide bindings, and Qt runtime mismatches. |
| v0.3 visible parity audit | Missing. | Table each visible capability as fixed, deferred, or external/GSP. |
| Public API/status cleanup | Missing. | Mark public surfaces as supported, experimental, advanced/unstable, deferred, or external/GSP. |
| Release example proof | Partial for the full RC, but the 2026-06-09 `EXAMPLES_NOTES.md` ledger is closed: source/gallery polish, `showcases/surface_grid`, `features/bounds_overlay`, runtime/readability fixes, scenario-helper audit, comment metadata audit, and builtin-shapes parity audit are resolved with native smoke or explicit audit evidence. | Continue broader release proof outside `EXAMPLES_NOTES.md`: visible parity table, API disposition, and any additional focused native evidence where the environment supports Vulkan. |

June 10 examples cleanup is closed in git history: shared interaction fixes, 3D context polish,
technique-panel polish, reviewed visual polish, and graph replacement landed with focused tests and
native smoke evidence.

Known follow-up: `gui/viewport_resize_hidden_smoke` currently fails. Recheck it during the next GUI
or runtime resize pass before treating hidden viewport resize coverage as clean.

Closed first slices that should stay in validation: frame artifact scene emission, raw `ctypes`,
retained textured mesh, retained DATA-coordinate visual attachments, color management, text, 2D
axes/ticks, colorbars, labels/readouts, scale bars, app/offscreen rendering, broad item/sample query
paths, scene visual-boundary checks, WebGPU fixture runner, and WASM frame artifact packet scene
smoke.

New Python binding direction: keep `datoviz.raw` as the exact generated `ctypes` layer, and make
top-level `import datoviz as dvz` the planned array-aware facade that preserves `dvz_*` names while
accepting NumPy arrays for policy-declared data arguments. Source of truth:
[../../spec/bindings/ARRAY_FACADE.md](../../spec/bindings/ARRAY_FACADE.md).


## Active Lanes

1. **Release closure:** feature/status table, visible parity audit, API disposition, known gaps.
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
7. [SHADER_TRANSFORM_FUTURE_COMPAT.md](SHADER_TRANSFORM_FUTURE_COMPAT.md)
8. [../../spec/scene/integration/OPTIONAL_PROVIDERS.md](../../spec/scene/integration/OPTIONAL_PROVIDERS.md)
9. [../../spec/scene/integration/QT_HOST_BRIDGE.md](../../spec/scene/integration/QT_HOST_BRIDGE.md)
10. [../../spec/bindings/ARRAY_FACADE.md](../../spec/bindings/ARRAY_FACADE.md)
11. [../../spec/scene/integration/WASM_DEBUGGABILITY_REFACTOR_PLAN.md](../../spec/scene/integration/WASM_DEBUGGABILITY_REFACTOR_PLAN.md)
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
