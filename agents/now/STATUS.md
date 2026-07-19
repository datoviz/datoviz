# Datoviz v0.4 Status

Status: active v0.4 release candidate. Updated: 2026-07-19.

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
the CMake consumer passed. The seven approved attended labels covered checkout-built examples, but
the evidence schema did not represent the exact-wheel Quickstart separately and those labels had
empty observations. Physical evidence intake run `29645582130` accepted that incomplete bundle;
the resulting report must not be cited as proof of an RC1 installed-wheel native window.

Post-release testing reproduced a macOS RC1 wheel failure at `glfwCreateWindowSurface()`: Datoviz
initialized its packaged sibling Vulkan loader while GLFW independently discovered another loader.
Commits `434f14bea` and `08174f3d7` hand the active Vulkan entry point to GLFW, add installed-wheel
native-window CI, distinguish the exact-wheel Quickstart, and reject undocumented manual results.
A fresh local arm64 wheel passed the complete installed validator and its canonical Quickstart;
the maintainer explicitly confirmed resize, pan, zoom, and normal close. RC2 is now a narrow
replacement-wheel hotfix. Its proof must use new canonical artifacts; the local wheel is diagnostic.

Post-fix Test run `29693662817` passed Linux, macOS, and Windows. Linux now includes the complete
Python tooling suite after the native build, shaderc validation, Xvfb Quickstart fixtures, and the
full compiled test suite. Hosted conformance run `29693217596` passed all six lanes and its
aggregate report against the RC1 wheel campaign with the corrected validator. This proves the
validator and hosted capability model, not the future RC2 artifact bytes.

The canonical RC2 campaign is complete. Wheels run `29695746332` passed all 29 jobs and produced
the six `0.4.0rc2` wheels at artifact commit `8765535db`. Exact-artifact hosted conformance run
`29696169890` passed all six lanes and its aggregate report. Physical intake run `29697580837`
accepted the MacBook M3 bundle after the canonical arm64 wheel passed two unattended profiles and
all eight attended interaction scenarios. TestPyPI verification run `29698900673` matched all six
indexed files byte-for-byte to the canonical Wheels run and passed every clean-install lane plus
the aggregate report. Artifact-neutral follow-up Test run `29696633533` passed all three operating
systems. Production publication is now the critical path.

The RC1 source bundle/checksums and release notes are closed. All six wheels are on TestPyPI and
PyPI. TestPyPI verification run `29652477816` and PyPI verification run `29666589331` matched every
indexed file byte-for-byte to canonical Wheels run `29644925786` and passed all six clean
installed-package smokes plus their aggregate gates. The tag and GitHub prerelease are public with
11 verified assets. `datoviz.org` now serves the v0.4 RC documentation, the former site is preserved
under `/v0.3/`, and the four initially missing gallery video/poster pairs are public and verified.
Post-RC1 branch cleanup is complete. Next critical path: ship the narrow RC2 replacement-wheel
hotfix. The former RC2 documentation/gallery and packaged-provider scope moves to RC3.

Completed runtime cleanup to keep in validation: DRP2 render-pass begin commands now carry explicit
render area, viewport, and scissor rectangles, scene emission initializes full targets before panel
passes, and mixed plain/MSAA panels use an explicit-region resolve path so panel resolves do not
clobber earlier framebuffer contents.

RC2 hotfix execution order:

1. Keep the v0.4 Git history cleanup deferred; do not rewrite RC or final release refs.
2. After the reviewed fixes reach `v0.4-dev`, make it the GitHub default branch so the repository
   landing page, clone default, PR base, website, and public RC agree. Keep the old v0.3 `main`
   unchanged until the post-RC2 naming cutover.
3. Keep RC2 limited to the packaged Vulkan/GLFW loader fix, its automated regression gates,
   corrected Quickstart guidance, and release evidence/process corrections.
4. Keep the RC1 wheel and package-index reports immutable; build a new canonical matrix for RC2
   rather than modifying RC1 evidence.
5. Completed: six-platform hosted conformance and exact canonical arm64-wheel physical evidence
   passed; physical Linux and Windows are recorded as unavailable exclusions, never as passes.
6. Completed: TestPyPI byte-identity and clean-install verification passed for all six wheels.
7. After RC2, preserve old `main` as `v0.3-maintenance`, rename `v0.4-dev` to `main`, and update
   branch-specific workflows, links, badges, and clone instructions without rewriting history.
8. Move Qt provider packaging, gallery/data attribution, generated C reference completion,
   documentation/gallery freeze, PR #132 triage, and candidate features to RC3.

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
| C/C++ distribution preflight | Canonical RC2 Wheels run `29695746332`, hosted conformance `29696169890`, physical M3 intake `29697580837`, and TestPyPI verification `29698900673` all passed against the exact replacement artifacts. | Publish only the validated bytes; preserve RC1 artifacts and reports unchanged. |
| Windows wheel proof | Canonical RC2 AMD64 and ARM64 wheels passed build, inspection, shaderc, CMake-consumer, installed-package, and TestPyPI byte-identity checks. No physical Windows machine was available for the replacement campaign. | Record physical Windows as unavailable, not passed; restore exact-artifact physical Windows proof for RC3 or final. |
| WebGPU/WASM experimental path | WebGPU fixture runner works; the generic WASM scene ABI and split DRP2 packet path now register 90 scenarios and expose 89 browser-live gallery routes (point cloud is registered but no longer exposed). Point cloud is delisted from the public browser gallery (`webgpu-deferred`, 2026-07-19): its source RESEPI LiDAR dataset is third-party and all-rights-reserved, so it cannot be redistributed as a public web data bundle; native capture and the deterministic local WASM packet proof still pass, and the localhost-only dev route is retained. SVG Tiger has headed browser proof and an approved committed prepared-data bundle attributed to Nicolas P. Rougier's Glumpy example gallery. | Point-cloud redistribution disposition resolved (unlicensed → delisted); continue generic volume and rendering techniques. |
| Compute+graphics experimental path | DRP2 `ResourceBarrier`, FramePlan scene compute lowering, WebGPU fixture parity, and the C `gpu_particle_smoke` showcase are active. CPU command-generation proof passed on 2026-06-04. Native Vulkan compute+graphics proof passed on 2026-06-17: `test_frame_plan_emitter_runtime_compute_two_frames_glsl_executes`, `test_vklite_compute_1`, `test_technique_compute_graphics`, and `examples/c/showcases/gpu_particle_smoke.c --png` with artifact `build/release-evidence/gpu_particle_smoke.png`. | Keep the slice classified as experimental in the feature/status table: native proof exists, but this is a narrow scene-compute/DRP2 interop path, not a general compute framework. |
| Qt/PyQt hosted path | Native Qt hosting and `datoviz_qtbridge` are implemented and locally proven from source. On 2026-06-18, `DVZ_CMAKE_ARGS="-DDVZ_ENABLE_QT_BRIDGE=ON" just build`, `./build/examples/qt/hosted_qt_smoke 120`, `./build/examples/qt/qt_hosting --smoke-ms 1000`, `DATOVIZ_QTBRIDGE_LIBRARY=build/qtbridge/libdatoviz_qtbridge.so uv run --isolated --with PyQt6 python -m datoviz.qt`, and the hosted PyQt smoke passed after updating the example to pass `DvzColor` to the raw background-color API. The isolated probe reported bridge Qt 6.11.1 and PyQt Qt 6.11.0; the system PyQt6 package in this shell lacks `QVulkanInstance` and is not a valid PyQt hosting proof. Canonical RC2 wheels include `datoviz.qt` and the `datoviz[qt]` extra but not the native bridge, so Qt hosting remains source-build-only. | Deliver and validate a packaged provider for RC3, preferably conda-first. Keep diagnostics explicit for a missing bridge, unsupported PyQt/PySide bindings, and Qt runtime mismatches; retain base-wheel checks with `--qt-probe optional`. |
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

1. **RC2 hotfix:** packaged macOS Vulkan/GLFW loader repair, installed native-window regression
   gates, corrected public guidance, and new canonical evidence without changing RC1 artifacts.
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
   packaged provider for RC3, preferably conda-first, without changing the base-wheel contract.
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
