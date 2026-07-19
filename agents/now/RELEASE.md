# Datoviz v0.4 Release Plan

Status: active release roadmap. Updated: 2026-07-19.

This is the short route from the current branch to `v0.4.0`. Use [STATUS.md](STATUS.md) for current
blockers and [DOCUMENTATION.md](DOCUMENTATION.md) for public documentation gates.

Durable release policy lives in [../../spec/release/](../../spec/release/):
[READINESS.md](../../spec/release/READINESS.md),
[RC_PROCESS.md](../../spec/release/RC_PROCESS.md),
[PHYSICAL_VALIDATION.md](../../spec/release/PHYSICAL_VALIDATION.md),
[COMMUNICATION.md](../../spec/release/COMMUNICATION.md), and
[GALLERY_OUTREACH.md](../../spec/release/GALLERY_OUTREACH.md).

Maintainer execution docs live in
[../../docs/contributors/release-process.md](../../docs/contributors/release-process.md),
[../../docs/contributors/release-physical-validation.md](../../docs/contributors/release-physical-validation.md),
[../../docs/contributors/release-flight-checklist.md](../../docs/contributors/release-flight-checklist.md),
and [../../docs/contributors/release-wheels.md](../../docs/contributors/release-wheels.md).
Public status docs live in [../../docs/reference/project-status.md](../../docs/reference/project-status.md)
and [../../docs/reference/feature-status.md](../../docs/reference/feature-status.md).

Distribution packaging preflight lives in
[DISTRIBUTION_RELEASE_CHECKLIST.md](DISTRIBUTION_RELEASE_CHECKLIST.md). Run it locally before
dispatching wheel CI, submitting conda-forge staged-recipes, or publishing the vcpkg overlay.


## Scope

Required before feature freeze:

1. Native scene/app path covers the declared v0.4 visual and interaction subset.
2. Retained textured mesh has deterministic example or fixture proof.
3. WebGPU/WASM has an honest experimental RC subset broad enough to host most non-desktop scene
   examples live on the website, including core visuals, animation/frame callbacks,
   compute-to-render particles, and a narrow request/query/readback slice.
4. Python binding generation and smoke tests work for the intended public C surface.
5. v0.3 visible capability gaps are fixed, active experimental, explicitly deferred, or
   external/GSP-owned.
6. Core examples compile and exercise the release feature set.
7. Minimal compute+graphics interop has an experimental C-first proof, explicit DRP2
   synchronization, and a gallery-oriented particle example.
8. Low-level Qt/PyQt hosted rendering works through a source-built Qt bridge, is documented as
   source-build-only in RC1, and does not add Qt as a dependency of `libdatoviz`. Packaged-provider
   work is an RC3 deliverable after the narrow RC2 packaging hotfix.

Not required for v0.4:

1. v0.3 source or ABI compatibility.
2. High-level object-oriented Python plotting wrappers.
3. Publication-quality PDF/SVG/vector export.
4. Full WebGPU parity with native Vulkan, full query parity across every visual family, browser
   equivalents for native desktop runtime examples, and advanced WebGPU technique parity.
5. Complex text shaping, TeX/math layout, collision solving, dashboards, general custom shader
   APIs, CUDA interop beyond an optional native advanced example, CuPy/Python interop,
   LOD/out-of-core policies, or full application APIs.

External ownership:

1. GSP/VisPy2 owns high-level OO Python and plotting APIs.
2. GSP/Matplotlib owns publication-oriented vector export.
3. Datoviz v0.4 owns the C engine, native scene/app path, raw/generated low-level Python binding
   surface, raster capture, experimental WebGPU/WASM path, and experimental compute+graphics
   proof.


## Release Sequence

### 0. Git History Cleanup

Status: deferred beyond v0.4 by maintainer decision on 2026-07-17. Do not rewrite RC or final
release refs. The preserved preparation record is [HISTORY_CLEANUP.md](HISTORY_CLEANUP.md).

The v0.4 development branch briefly carried large generated/runtime payloads and copied legacy
trees. The current source tree has removed those payloads, but the Git object database only shrinks
after a coordinated history rewrite or deletion of every ref that still keeps those objects alive.

Policy if cleanup is reconsidered in a future maintenance window:

1. Do not rewrite v0.4 RC or final release refs.
2. From `v0.4.0-rc1` onward, treat public release refs as stable and do not rewrite history except
   for emergencies.
3. Keep `datoviz/datoviz` as the active v0.4+ repository.
4. Preserve exact old history outside the active repo only if needed, preferably in a separate
   `datoviz-legacy` mirror or GitHub release archives.
5. Expect existing direct-git users to reclone after the rewrite; repairing old clones is possible
   but should not be the primary instruction.

Safe execution sequence:

1. Announce a temporary push freeze.
2. Create and verify a legacy backup of the current refs.
3. Run `git filter-repo` in a fresh mirror clone first, removing the agreed heavy paths.
4. Inspect rewritten refs, sizes, tags, and a fresh clone smoke test.
5. Force-push cleaned active refs with `--force-with-lease` only after explicit maintainer
   approval.
6. Publish the reclone instructions in the README/release notes/pinned issue before cutting RC1.

Existing clone migration note:

```sh
mv datoviz datoviz-pre-v0.4-history-cleanup
git clone --recursive https://github.com/datoviz/datoviz.git
cd datoviz
```

For uncommitted local work:

```sh
git diff > /tmp/datoviz-local.patch
# reclone, then:
git apply /tmp/datoviz-local.patch
```

For committed local work, prefer exporting patches before recloning:

```sh
git format-patch origin/main..HEAD -o /tmp/datoviz-patches
# reclone, then:
git am /tmp/datoviz-patches/*.patch
```

Do not run the destructive rewrite from an automation session without an explicit final maintainer
confirmation for the exact refs to rewrite and force-push.

Branch routing is separate from history cleanup. During the narrow RC2 hotfix, make `v0.4-dev` the
GitHub default without changing either branch tip. After RC2, preserve the old v0.3 `main` as
`v0.3-maintenance`, rename `v0.4-dev` to `main`, and update branch-specific automation and links.
That cutover must preserve commits and tags; it is not permission to merge the incompatible lines,
rewrite history, or force-update release refs.

### 1. Feature-Freeze Candidate

Exit criteria:

1. Feature/status docs exist and use `supported`, `experimental`, `advanced/unstable`, `deferred`,
   and `external/GSP`; final RC work reconciles them with the visible parity audit and known gaps.
2. No feature-freeze blocker in [STATUS.md](STATUS.md) is unclassified.
3. The aggressive public API/ABI consistency campaign has landed on `v0.4-dev`; use
   [HANDOFF_PUBLIC_API_PRE_RC_AUDIT.md](HANDOFF_PUBLIC_API_PRE_RC_AUDIT.md) as the completed
   record when reconciling generated references, binding docs, examples, and status pages.
4. Retained textured mesh remains in validation with deterministic proof.
5. Text, axes, ticks, colorbars, annotations, scale bars, and retained visuals are represented in
   examples or tests.
6. Qt/PyQt hosting remains classified as implemented but source-build-only in RC1, with the absence
   of a packaged bridge, the limits of `datoviz[qt]`, bridge diagnostics, and wheel checks kept
   explicit.

Suggested validation:

```sh
git diff --check
just build
just test scene
just spec-check
```

### 2. v0.3 Visible Parity Audit

Audit visible capabilities, not old APIs: retained scene workflow, offscreen/GLFW app, screenshot
capture, frame callbacks, multi-panel figures, text, axes, colorbars, panzoom, arcball/fly/turntable,
point, pixel, marker, primitive, segment/path, image, mesh/textured mesh, sphere, and volume.

Exit criteria: every visible gap is fixed, experimental, deferred, or external/GSP, and the table is
linked from release docs. The current public table is
[../../docs/reference/v03-visible-parity.md](../../docs/reference/v03-visible-parity.md).

### 3. WebGPU/WASM Experimental Slice

Ship an honest browser/backend subset with broad live-example coverage, not native parity.

Exit criteria:

1. Portable scenario host exists for native and browser runners.
2. Example manifest marks each public example as `webgpu-live`, `webgpu-planned`,
   `webgpu-deferred`, or `native-only`.
3. Most non-desktop scene examples have live browser gallery routes or explicit `webgpu-planned`
   gaps.
4. DRP2/WebGPU runner and WGSL emission work for the declared subset.
5. Compute-to-render particle showcase runs in browser WebGPU at a documented particle budget.
6. Point/marker picking plus one probe/readback example works through async browser readback.
7. Unsupported commands, visual families, query targets, shader variants, native-only runtime
   features, and capability failures have diagnostics.
8. Scene semantics are shared; there is no WebGPU-only scene contract.
9. Browser runtime traffic uses frame artifact packet spans; DRP2 JSON remains debug/fixture-only.
10. `webgpu-live` gallery examples reuse the same canonical C example or portable C scenario as the
    native route; browser JavaScript remains host glue and does not reimplement example behavior.

### 4. Compute+Graphics Experimental Slice

Ship one real GPU compute-to-render path without widening v0.4 into a general compute framework.

Exit criteria:

1. Minimal DRP2 synchronization semantics are active, schema-backed, fixture-covered, and mapped by
   native vklite execution.
2. A portable compute-to-render command stream covers storage-buffer write followed by render-time
   vertex or instance consumption.
3. Native validation proves compute, synchronization, graphics consumption, and readback or captured
   visual evidence.
4. WebGPU accepts the portable subset or emits explicit unsupported-feature diagnostics.
5. The C `gpu_particle_smoke` gallery proof remains available with a release artifact target.
6. Optional CUDA SDK interop remains native-only, capability-gated, and advanced/unstable.

### 5. Qt/PyQt Hosted Path

RC1 availability: the implementation and local source-build proof are complete, but canonical RC1
wheels do not contain `datoviz_qtbridge`. The `datoviz[qt]` extra supplies PyQt6 only. Ordinary
wheel users therefore do not have a ready-to-use Qt hosted path in RC1.

Exit criteria:

1. `datoviz_qtbridge` builds only when Qt development headers and libraries are available.
2. `libdatoviz` has no Qt link dependency.
3. `datoviz.qt` dynamically loads the bridge when PyQt hosting is requested.
4. PyQt6 hosting no longer calls missing Python bindings for
   `QVulkanInstance::setVkInstance()` or `QVulkanInstance::vkInstance()` directly.
5. Unsupported PyQt/PySide bindings, missing bridge libraries, and Qt runtime mismatches fail with
   clear diagnostics.
6. The native Qt smoke and Python PyQt hosted example are proven locally or recorded as blocked by
   environment constraints.

### 6. Raw `ctypes` API Candidate

Exit criteria:

1. Generated bindings load the intended installed C API.
2. ABI/layout smoke checks pass.
3. Raw examples cover the supported low-level Python path.
4. The docs state that high-level Python plotting is GSP/VisPy2 scope and distinguish the planned
   top-level NumPy-adapted call form from exact `datoviz.raw` access.
5. C API docs continue to state that `dvz_figure_emit()` and `dvz_figure_emit_ex()` are gone and
   that `DvzSceneFrameArtifact` is the scene emission product.

### 7. RC1

Exit criteria:

1. RC1 tag and notes exist.
2. Build/test/spec validation is recorded.
3. Feature table, visible parity table, known gaps, Python binding scope, and WebGPU/WASM scope are
   published or linked.
4. Release examples are documented enough for early testers.
5. Required RC note fields from
   [../../spec/release/RC_PROCESS.md](../../spec/release/RC_PROCESS.md) are present.
6. `CITATION.cff`, [citation docs](../../docs/reference/citation.md), and the JOSS draft are
   present and do not claim a final Zenodo DOI before it exists.

Current packaging gate:

1. Canonical Wheels run `29644925786` passed all 29 jobs at exact artifact commit `ea06c5cdf`,
   covering Linux x86_64/aarch64, macOS 15 arm64/Intel, Windows AMD64/ARM64, Python 3.10 through
   3.14 installed-wheel smokes, and the non-blocking Linux prerelease smoke.
2. Hosted conformance run `29645577693` passed all six lanes. Physical intake run `29645582130`
   accepted seven attended labels for checkout-built examples, but the schema did not separately
   record the exact-wheel Quickstart and allowed empty observations. Post-release testing therefore
   supersedes the claim that this bundle proved an RC1 installed native window.
3. The RC1 source bundle, validation pack, checksums, release report, and notes are recorded.
4. TestPyPI package-index verification run `29652477816` matched all six indexed wheel filenames,
   byte counts, and SHA-256 values to run `29644925786`; all clean installed-package smokes and the
   aggregate report passed. PyPI verification run `29666589331` then matched the same six filenames,
   byte counts, and SHA-256 values and passed every clean installed-package smoke plus the aggregate
   report. Public installation guidance may now use PyPI.

### 8. RC2 Hotfix

Exit criteria:

1. The macOS packaged Vulkan loader is handed to GLFW before initialization.
2. Clean installed-wheel native-window smokes run on Linux Xvfb/Lavapipe; hosted macOS arm64
   requires offscreen MoltenVK rendering because hosted runners do not reliably expose an
   interactive desktop session.
3. The exact canonical macOS arm64 wheel passes the Quickstart outside the checkout, with explicit
   resize, pan, zoom, and close observations.
4. All six canonical wheels pass the normal build, inspection, installed-package, rendering,
   consumer, and hosted conformance gates.
5. RC1 artifacts and evidence remain immutable; RC2 uses new checksums and evidence.
6. Public installation and release guidance disclose the RC1 limitation until RC2 replaces it.

RC2 physical-matrix exception: only the MacBook M3 is available for fresh exact-artifact physical
validation. Physical Linux and Windows are explicit unavailable exclusions and must not be inferred
from hosted results. This narrowing is accepted for the macOS-focused replacement hotfix only.
Linux retains full native tests plus installed-wheel Xvfb/Lavapipe window proof; Windows retains
fresh AMD64/ARM64 build, packaging, installation, shaderc, and consumer proof. The RC2 notes must
disclose the reduced physical matrix. Fresh exact-artifact physical Linux and Windows proof returns
as an RC3 or final-release requirement.

Deferred to RC3, not RC2 hotfix work:

1. Reassess the post-RC1
   [GSP Texture2D mesh integration plan](../../spec/scene/integration/GSP_TEXTURE2D_MESH_PLAN.md).
   Implement it only if RC1 feedback and stabilization leave room for the public field-slot
   sampling API, deterministic nearest/clamp/no-mipmap fixtures, conversion-free linear RGBA,
   unlit multiplication, and native/WebGPU validation. Otherwise defer it without blocking RC3.
2. Reassess the
   [multi-light Klein bottle slice](../../spec/scene/slices/MULTI_LIGHT_KLEIN_BOTTLE_SLICE.md).
   Implement it only if RC1 feedback and stabilization leave room for the scene-owned light API,
   panel-local fixed-capacity light sets, two-sided lighting, shared RGB example preset, generated
   checkerboard Klein-bottle showcase, and native/WebGPU validation. Otherwise defer it without
   blocking RC3.

Candidate release infrastructure, not an RC2 hotfix blocker:

1. Begin the post-RC1
   [website deployment and versioning migration](../../spec/docs/WEBSITE_DEPLOYMENT.md): preserve
   the guarded local publisher as a fallback, add hosted preview/artifact validation, and introduce
   manual production promotion of exact documentation bytes. Complete it before v0.4.0 final when
   practical; do not delay RC3 solely for this infrastructure work.

### 9. RC3 Documentation, Packaging, And Quality Candidate

Exit criteria:

1. The former RC2 documentation/gallery scope is complete: structure, generated C reference,
   captured artifacts, feedback triage, attribution, and outreach review.
2. Qt/PyQt hosting has a tested packaged `datoviz_qtbridge` provider, preferably conda-first,
   without adding Qt to the base wheel.
3. Only blocker fixes remain after those planned deliverables.
4. Packaging, licenses, generated artifacts, release notes, and docs are final candidates.
5. Packaging and quality checks from
   [../../spec/release/READINESS.md](../../spec/release/READINESS.md) are clean or recorded as
   known issues.

### 10. Final `v0.4.0`

Exit criteria:

1. `v0.4.0` is tagged and published with reproducible artifacts.
2. Documentation and release notes are public.
3. Launch screenshots, short clips, README/website assets, and announcement text are generated from
   current gallery examples.
4. Direct feedback channels are open for early users, especially scientists whose public datasets
   are used in showcase examples.
5. GitHub-Zenodo archiving is enabled for `datoviz/datoviz`.
6. The final `v0.4.0` GitHub release has a Zenodo version DOI and concept DOI.
7. `CITATION.cff`, [citation docs](../../docs/reference/citation.md), final release notes, and
   announcement text include the exact version DOI and release date.
8. If `.zenodo.json` is added later for grants, communities, or related identifiers, it must be
   kept consistent with `CITATION.cff` because Zenodo uses `.zenodo.json` preferentially.
9. The JOSS draft is submitted or explicitly deferred; JOSS acceptance is tracked separately from
   the software release.
10. The active queue resets for v0.4 patch work and v0.5 planning.


## Post-Release Refactor Queue

Do not let these delay `v0.4.0` unless needed to fix a release blocker:

1. lower-risk `src/scene/scene.c` structural cleanup;
2. shader/visual registry cleanup that does not change public behavior;
3. broader runtime/backend modularity work;
4. long-horizon WebGPU parity and advanced visual work.
