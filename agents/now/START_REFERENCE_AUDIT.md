# Get Started And Reference Audit

Status: audit complete; remediation pending. Updated: 2026-07-12.

Scope: `docs/index.md`, all seven pages under `docs/start/`, and all 44 Markdown pages under
`docs/reference/`, including authored reference prose, 15 visual-family pages, and generated C API
output. Generated API defects must be fixed in public headers, metadata, or generators rather than
patched in generated module pages.

This file is the acceptance checklist for the pre-RC remediation. Remove it after every item is
closed and durable validation lives in tooling, public docs, or release checks.


## Audit Summary

The overall information architecture, v0.4 layer model, GSP/VisPy2 boundary, visual-family
contracts, generated C API classification, Qt status, citation posture, and broad WebGPU/compute
classification are sound. The highest-risk gap is the first-render story: the homepage, Quickstart
fixtures, gallery scenario, screenshot, and live WebGPU route describe one scatter example but do
not currently share one executable visual contract.

Mechanical evidence gathered during the audit:

- `just docs-api-check` passes classification for 1,548 functions;
- `just docs-build-check` passes and generates the expected gallery derivatives;
- authored Python fences parse;
- `examples/docs/quickstart.py` passes `py_compile`;
- `examples/docs/quickstart.c` passes syntax-only compilation against public headers;
- visual-family required attributes match current examples and public contracts;
- local authored links resolve after excluding generated media derivatives and data-URI images;
- `git diff --check` passes.


## A. First-Render Correctness

- [ ] Make `examples/docs/quickstart.py` and `examples/docs/quickstart.c` produce the documented
  canonical scatter result, or give them a separate screenshot and remove the equivalence claim.
- [ ] Reconcile seed, positions, random colors, alpha, diameter range, background, filled style,
  depth mode, blending, figure size, and title with `examples/c/start/scatter.c` and the published
  screenshot/live route.
- [ ] Harden the complete C fixture with pointer/result checks, one cleanup path, and useful failure
  diagnostics.
- [ ] Harden the complete Python fixture with handle/result checks and guaranteed scene cleanup
  after blocking or test-session completion.
- [ ] Make `docs/start/quickstart.md` describe exactly the fixture, screenshot, and live result it
  actually presents.
- [ ] Reduce the homepage scatter block to a checked canonical excerpt/include or otherwise prevent
  it from becoming a third independently maintained implementation.
- [ ] Add validation that resolves MkDocs Snippets includes and directly checks the included Python
  and C fixture files.
- [ ] Validate screenshot/code correspondence with deterministic capture or an explicit canonical
  scenario contract.


## B. Authored API And Lifecycle Correctness

- [ ] Replace nonexistent `DVZ_DRP2_PACKET_KIND_FRAME` in `docs/reference/c-api/index.md` with the
  current public `DVZ_DRP2_PACKET_FRAME` constant.
- [ ] Harden the offscreen example in `docs/reference/python-direct-engine.md`: check app/view
  handles, compare against `DVZ_CANVAS_FRAME_READY`, and guarantee app-before-scene cleanup.
- [ ] Reconcile `docs/reference/python-direct-engine.md` terminal-IPython close/reopen claims with
  the active macOS hosted-close investigation and `docs/how-to/use-ipython.md` warning.
- [ ] Label `docs/start/first-c-program.md` code as a call-sequence excerpt with assumed arrays and
  objects, and route readers to the checked complete fixture.
- [ ] Keep generated C API pages generated; continue using `just docs-api-check` as their coverage
  and routing proof.


## C. Release And Status Freshness

- [ ] Remove or generate the stale hard-coded WebGPU example counts in
  `docs/reference/webgpu-subset.md`; link the manifest-derived matrix as the source of truth.
- [ ] Update `docs/reference/compute-graphics.md` to record the native Vulkan proof already captured
  in `agents/now/STATUS.md`, while retaining environment-dependent skip guidance.
- [ ] Keep status vocabulary definitions in `project-status.md`, broad classifications in
  `feature-status.md`, and manifest-derived example/backend facts in generated outputs.
- [ ] Add drift checks for any status facts that remain intentionally duplicated.
- [ ] Reconcile exact package versions, commands, tags, release-note links, and channel availability
  at the RC cut rather than inventing unpublished artifacts.


## D. Installation And Platform Guidance

- [ ] Clone the active branch directly with submodules, or run `git submodule update --init
  --recursive` after switching branches.
- [ ] Give every “use the exact release command” instruction a clickable authority; before RC
  publication, state clearly that no installable RC command is published and route to source build.
- [ ] Distinguish required normal-build shader tools from optional tests/tooling dependencies;
  explain the `glslc`, `glslangValidator`, shaderc, runtime GLSL, and precompiled-shader paths
  accurately.
- [ ] Explain when normal builds use vendored dependencies and when Ubuntu system-auto packaging
  additionally needs GLFW, cglm, mimalloc, and related development packages.
- [ ] Make native Windows wheel installation the primary packaged path once RC artifacts exist;
  present native source builds and WSL2 as separate development alternatives.
- [ ] Reconcile vcpkg/Conda/source-bundle rows with the actual RC publication decision.
- [ ] Link platform support and known limitations from the installation success path.
- [ ] Shorten the primary Python install journey by routing detailed source-build/platform material
  to the appropriate build/reference pages.


## E. Onboarding Journey And Information Architecture

- [ ] Route general How-To links from the homepage, Get Started overview, and AI workflow to the
  How-To overview rather than directly to `create-a-scene.md`.
- [ ] Add one concrete next-step link per layer in `choose-your-layer.md`: Python Quickstart, First C
  Program/C integration, WebGPU examples/subset, advanced runtime reference, and GSP status.
- [ ] Resolve the hidden `what-is-datoviz.md` page by merging its useful model into the Get Started
  overview, exposing it intentionally, or removing it.
- [ ] Expose `first-c-program.md` as the C call to action from layer choice, or merge it into the
  primary journey.
- [ ] Add explicit Python and C first-action links to the Get Started overview.
- [ ] Decide whether the homepage visual-proof grid should implement the approved six-domain design
  or update the durable design decision to the current four-card composition.
- [ ] Avoid maintaining full duplicate scatter programs on the homepage and Quickstart.


## F. Editorial And Accessibility Polish

- [ ] Standardize Start headings and visible card labels to sentence case while preserving proper
  names and acronyms.
- [ ] Replace vague “things to draw” language in `what-is-datoviz.md` with visuals, attributes, and
  data arrays if the page is retained.
- [ ] Remove redundant wording such as “usual workflow is typically.”
- [ ] Add a no-browsing fallback to the AI workflow so users can provide page contents to assistants
  without web access.
- [ ] Clarify blocking `dvz.run()` lifecycle behavior in the Quickstart without encouraging users to
  copy C ownership patterns into Python.
- [ ] Verify homepage video previews for keyboard operation and reduced-motion behavior in the
  rendered site.


## G. Reference Structure And Coverage

- [ ] Preserve the authored/generated boundary: generated signatures and symbol facts come from
  public headers; authored pages provide routing, constraints, statuses, and choice guidance.
- [ ] Keep scene/app and normal NumPy-adapted Python paths primary in the C/Python reference indexes;
  keep DRP2, FFI, FramePlan, Vulkan/runtime, and exact raw calls clearly advanced.
- [ ] Keep every visual-family page aligned with the standard template: status, backends, use/avoid,
  required and optional data, controllers, picking/probing, backend notes, canonical example, and
  related How-To.
- [ ] Continue checking visual-family status against `examples/c/MANIFEST.yaml`; current audit found
  supported native families, experimental glyph/splat, volume planned for WebGPU, and splat deferred.
- [ ] Exclude data-URI images and generated gallery derivatives from raw filesystem link checks.


## Implementation Plan

### Batch 1 — First render and API blockers

Synchronize and harden both Quickstart fixtures, fix the DRP2 constant, reconcile screenshot/live
claims, harden Python offscreen lifecycle, and add include-aware fixture validation.

### Batch 2 — Release/install truth

Fix branch/submodule cloning, release-note authority, shader requirements, vendored versus system
dependencies, Windows wheel/source/WSL routing, package-channel claims, IPython status, compute proof,
and manifest-derived WebGPU counts.

### Batch 3 — Journey and consolidation

Route general links through the How-To overview, add layer CTAs and Python/C first actions, resolve
hidden onboarding pages, and make one canonical scatter source contract authoritative.

### Batch 4 — Reference drift prevention

Generate or mechanically validate duplicated status facts, preserve generated API boundaries, and
extend snippet validation to included fixture files and authored reference examples.

### Batch 5 — Editorial and rendered-site proof

Apply sentence case and precise terminology, improve AI offline fallback and Python lifecycle prose,
then check desktop/mobile navigation, video keyboard behavior, and reduced motion.

### Batch 6 — RC-cut reconciliation

Insert the real RC tag, version, wheel command, artifact and release-note links, supported package
channels, and citation/archive facts only after publication decisions are final.


## Final Acceptance Proof

- [ ] Run the include-aware Start/Home/Reference snippet checker.
- [ ] Run `just quickstart-check`, including Python and C fixture proof.
- [ ] Run `just ctypes-check` after binding-facing documentation changes.
- [ ] Run `just docs-api-check` after public-header, API metadata, or generated-reference changes.
- [ ] Run `just docs-build-check` and inspect the rendered onboarding path.
- [ ] Validate local links/anchors while excluding generated media and data URIs correctly.
- [ ] Capture or compare the deterministic Quickstart result against the published screenshot.
- [ ] Run fresh onboarding, technical, and status audits after remediation.
- [ ] Run `git diff --check` before every checkpoint commit.
