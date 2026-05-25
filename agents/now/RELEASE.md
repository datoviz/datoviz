# Datoviz v0.4 Release Readiness Plan

> **Execution Status**
> - **Status:** `PLANNING GUIDE`
> - **Updated on:** `2026-05-16`
> - **Scope:** post-feature-completion release work for the public Datoviz v0.4 release
> - **Non-scope:** implementation planning for visual families, text, axes, GUI widgets, WebGPU,
>   video, and other feature tracks already covered by focused implementation plans.


## Purpose

This document describes the work needed after the v0.4 feature set is mostly implemented, before
Datoviz v0.4 should be released publicly.

The key distinction is intentional:

1. **Implementation completion** decides what v0.4 can do.
2. **Release readiness** decides whether v0.4 is coherent, documented, testable, supportable, and
   worth announcing.

Datoviz v0.4 can still be an aggressive architectural release with future API breaks allowed
between v0.4 and v0.5. It should not, however, be a loose development snapshot. The release should
have a clear public API story, a reproducible build and packaging story, complete enough
documentation, a strong gallery, and a credible quality audit.


## Release Principles

1. **Separate feature freeze from API freeze.**
   Feature freeze means the core v0.4 capability set is present. API freeze means names,
   ownership, include paths, object lifetimes, and common workflows have been reviewed and made
   internally consistent. The first can happen before the second.

2. **Prefer explicit release candidates.**
   v0.4 should likely go through several release candidates. Each release candidate should have a
   written scope, known issues, validation matrix, generated artifacts, and migration notes.

3. **Make AI agents useful but constrained.**
   Agents can do broad audits, repetitive documentation rewrites, test cleanup, example generation,
   and static-analysis triage. They should work from small task records, leave diffs reviewable,
   and avoid mixing feature work with release-quality work in the same branch slice.

4. **Treat documentation and examples as product surface.**
   The public release is not just the C library. The website, gallery, API docs, Python bindings,
   examples, release notes, and packaging experience are part of what users evaluate.

5. **Keep the scene layer as the primary public narrative.**
   The main documentation should lead with retained scene usage. Lower layers (`drp2`, `vklite`,
   `canvas`, `stream`, `video`) should be documented as advanced and integration-oriented surfaces,
   especially for users who already have their own engine or want only capture/stream/sink pieces.

6. **Use a task-oriented documentation taxonomy.**
   Public documentation should follow a Diataxis-style split between tutorials, how-to guides,
   reference, and explanation. Each page should have one primary job: teach a first experience,
   solve a specific task, state technical facts, or explain architecture and tradeoffs.

7. **Keep WebGPU/WASM in v0.4 as experimental portability work.**
   The v0.4 release should include the browser/WebGPU/WASM lane even if it supports only a narrow
   subset of native scene features. The release promise is an experimental portability path over
   scene-emitted WGSL DRP2 streams, not full feature parity with the Vulkan runtime.

8. **Keep the high-level OO plotting layer outside Datoviz.**
   Datoviz v0.4 remains a C implementation plus low-level/generated binding surface. The
   object-oriented plotting API is expected to live in the external GSP/VisPy2 stack, either
   alongside v0.4 or shortly after it.

9. **Treat vector export as a GSP/backend concern, not a Datoviz v0.4 feature.**
   Publication-oriented PDF/SVG/vector output should be produced by a GSP Matplotlib backend.
   Datoviz is the interactive GPU/raster backend for the GSP protocol. Datoviz-native image
   capture remains in scope; Datoviz-native structural vector export is not a v0.4 requirement.


## Feature-Scope Terms

In this plan, "MVP" means the first release-quality slice that prevents obvious v0.3 regressions
and unblocks core examples. It does not mean a prototype without tests, diagnostics, or repeated
update behavior.

### Rendered Text MVP

Basic rendered text already exists in the C examples and scene visual path. The v0.4 MVP is the
first release-quality, tested, and integrated slice of that capability, not the first proof that
glyphs can appear on screen.

The v0.4 text slice should include:

1. visible single-line UTF-8 text attached to panels,
2. a built-in fallback font and one user-supplied font path or font-byte path if practical,
3. run-level size and color,
4. screen-space placement for labels, axes, legends, colorbars, HUD/readouts, and annotations,
5. data-space anchoring for simple point/image annotations,
6. panel clipping, DPI-aware sizing, offscreen capture, and GLFW rendering,
7. retained updates for string, style, placement, and destroy.

The v0.4 text slice may defer:

1. paragraph layout and wrapping,
2. bidirectional and complex-script shaping,
3. rich text with mixed style runs,
4. full TeX/math layout inside Datoviz,
5. color emoji,
6. glyph-level picking or substring selection,
7. public glyph-atlas manipulation APIs,
8. collision avoidance and label-placement solving.

### 2D Axes, Colorbar, Legend, And Annotation MVP

The v0.4 explanatory-object slice should include:

1. 2D x/y axes for panzoom panels,
2. deterministic linear numeric ticks with "nice" spacing,
3. tick labels, axis labels, and units,
4. axis lines, tick marks, and optional grid lines,
5. axes that update correctly after pan/zoom and resize,
6. one continuous colorbar per panel or visual scale,
7. vertical and horizontal colorbar orientation,
8. colorbar title, units, deterministic tick labels, and a visible ramp,
9. simple panel-attached text labels and pinned readout annotations,
10. explicit diagnostics when a requested explanatory object is unsupported.

The v0.4 explanatory-object slice may defer:

1. log, datetime, categorical, geographic, and nonlinear axes,
2. multiple coordinated axes on the same side,
3. automatic label collision avoidance,
4. interactive axis or colorbar range editing,
5. categorical legends beyond a small documented first slice,
6. grouped legends, multi-scale legends, and symbol/line-style legend composition,
7. callout leader lines, dimension annotations, scale bars, and measurement tools,
8. vector/PDF/SVG preservation of text and axes inside Datoviz.


## v0.3 Regression Checklist

v0.4 does not need source or ABI compatibility with v0.3. It should still avoid losing the visible
capabilities users already associate with Datoviz unless a capability is explicitly moved to
GSP/VisPy2 or deferred with a release note.

### Avoid Regressing In v0.4

1. Core retained-scene workflow: create a figure, create panels, attach visuals, run offscreen or
   GLFW, resize, capture images, and drive frame callbacks.
2. Active visual families at a user-visible level: point, pixel, marker, primitive/basic, segment,
   path, image, mesh, sphere, volume, and text/glyph.
3. Full and partial data updates for visuals, including repeated updates across live frames.
4. Texture-backed image and volume workflows, including sampled fields and colormap-like scalar
   interpretation where supported.
5. 2D axes, ticks, labels, scale/range handling, colormaps, and continuous colorbars.
6. Interaction controllers: panzoom, arcball, camera, fly, turntable, and input routing through the
   app layer.
7. Multi-panel figures, linked-panel behavior, per-panel viewport/scissor, layout reservation, and
   stable resize behavior.
8. User-visible render controls: depth testing, transparency/alpha modes, mesh lighting/material
   controls, marker styling, segment/path width and cap/join behavior, image filtering, sphere
   modes, and volume render/slice/clipping controls.
9. GUI-backed examples where GUI state drives scene parameters, even if the GUI API itself remains
   an advanced/native example layer.
10. Offscreen screenshot/gallery capture and enough video/frame-sequence capture to support release
    examples.

### Acceptable v0.4 Deferrals

1. Exact v0.3 API compatibility, including old `DvzBatch`, `DvzViewset`, `DvzDual`, `DvzBaker`,
   `dvz_panel_default()`, `dvz_panel_visual()`, `dvz_scene_run()`, and per-visual allocation helper
   names.
2. Python OO wrappers that mirror v0.3 ergonomics.
3. Convenience/demo helpers such as demo panels, gizmos, quick horizontal grids, and old
   figure/panel update helpers when the underlying C capability exists through the new scene/app
   path.
4. Niche v0.3 visuals and variants such as `wiggle`, standalone `slice`, `monoglyph`, rounded image
   borders, full marker texture modes, and exact image anchor/permutation parity.
5. Python-side GUI wrappers, data download helpers, shape collections, and gallery data-preparation
   conveniences.
6. Full publication/static/vector export from Datoviz; this belongs to GSP/Matplotlib.

### Moved Out Of Datoviz Core

1. High-level object-oriented Python plotting APIs.
2. Declarative plotting grammar and backend-agnostic scene descriptions.
3. Publication-oriented PDF/SVG/vector export.
4. Python-first data loading, preprocessing, shape collections, and example asset orchestration,
   except for small helper scripts needed by Datoviz's own C examples.


## Proposed Milestones

### M0: Feature-Complete Development Snapshot

Goal: declare that the v0.4 feature set is mostly present, even if still rough.

Exit criteria:

1. The supported v0.4 visual families are listed explicitly.
2. Text, axes, ticks, labels, colorbars, annotations, GUI docs, video export, IPython use, WebGPU
   status, and gallery-critical examples have an owner and a completion state.
3. Unsupported or experimental features are explicitly marked as such.
4. WebGPU/WASM is present as an experimental v0.4 lane with its supported DRP2 command subset,
   scene visual subset, browser runtime status, and known gaps documented.
5. The feature implementation plans in `agents/now/` have either been completed, deferred, or
   reduced to known release-blocking issues.
6. A first pass of examples exercises every intended public feature at least once.


### M1: API Shape Review

Goal: decide what the v0.4 public surface is, before polishing docs around it.

Work:

1. Audit every public header under `include/datoviz/`.
2. Classify symbols as public, internal-leaking, experimental, deprecated, or accidental.
3. Confirm include structure: umbrella header, module aggregators, subheaders, and private
   dependencies such as `_macros.h`.
4. Review object ownership and destroy rules across public APIs.
5. Review naming consistency for constructors, destroyers, setters, update functions, callbacks,
   request/result APIs, and enum names.
6. Review error/status behavior: assertion-only invariants vs recoverable runtime failures.
7. Decide whether lower-level APIs are officially supported in v0.4 or documented as advanced
   unstable surfaces.
8. Review the public API against
   [`../../spec/api/PUBLIC_API_CONVENTIONS.md`](../../spec/api/PUBLIC_API_CONVENTIONS.md),
   including struct-versus-setter choices, composite object naming, and WASM/binding ergonomics.
9. Decide and document the v0.4 Python API policy: Datoviz ships raw/generated `ctypes`
   bindings only, the v0.3-style Python OO API is out of scope, GSP/VisPy2 owns the future
   object-oriented and plotting APIs, and Matplotlib is the publication/vector-export backend.
10. Decide and document that Datoviz-native vector export is not part of the v0.4 backend promise;
   Datoviz should provide raster image capture while GSP/Matplotlib owns publication-oriented
   vector output.
11. Add or normalize Doxygen-style docstrings for public and module-level functions.
12. Generate a public API inventory and compare it with the intended v0.4 narrative.

Exit criteria:

1. A generated or scripted API inventory exists.
2. Public headers have consistent doc comments.
3. Accidental public symbols are removed, renamed, or explicitly documented.
4. Public constructors, descriptors, style setters, composite objects, and binding-facing APIs have
   been checked against `spec/api/PUBLIC_API_CONVENTIONS.md`.
5. The v0.4 compatibility policy is written: API breaks allowed in v0.5, but v0.4 patch releases
   should avoid unnecessary source breakage.


### M2: Code Quality and Safety Audit

Goal: harden the implementation after the feature rush.

Work:

1. Review module boundaries and remove duplicated cross-module helpers.
2. Inspect allocation, ownership, and destroy paths.
3. Check command-buffer, frame-target, swapchain, borrowed-handle, and Vulkan object lifetimes.
4. Check arithmetic around sizes, counts, strides, texture layouts, row pitches, and downcasts.
5. Review runtime state for file-scope mutable variables and test-control leakage.
6. Normalize use of `dvz_calloc`, `dvz_free`, `dvz_memcpy`, `dvz_memset`, `dvz_fprintf`, and
   compatibility wrappers.
7. Audit error paths and partial-initialization cleanup.
8. Review long-running live paths for transient resource accumulation.
9. Run static analysis and triage actionable results.
10. Run memory and undefined-behavior tools where practical.

Recommended validation:

1. `git diff --check`
2. `just build`
3. `just test`
4. Focused `just test <module>` loops for every touched module.
5. `clang-tidy -p build` on changed C/C++ files.
6. `scan-build` or Clang Static Analyzer for broader C changes.
7. `cppcheck --enable=warning,style,performance,portability` as a secondary pass.
8. ASan/UBSan builds for CPU-heavy paths.
9. Vulkan validation-layer smoke tests for graphics paths.
10. Representative live-loop smoke tests such as bounded GLFW examples and video/export loops.

Exit criteria:

1. Static-analysis findings are either fixed or recorded with rationale.
2. Memory/lifetime risks have focused tests or issue records.
3. Validation logs are captured for the release candidate.
4. Known release-blocking quality issues are tracked in one place.


### M3: Test Suite Review

Goal: make the test suite a maintainable release asset rather than an accumulation of development
checks.

Work:

1. Inventory tests by module, behavior, and risk area.
2. Separate true regression tests from exploratory or temporary tests.
3. Remove duplicated fixture setup where shared helpers would make tests clearer.
4. Ensure each active module has meaningful tests for normal paths, error paths, lifetime,
   repeated updates, and destroy behavior.
5. Review DRP2 fixture tests for clarity, determinism, and coverage of the public protocol subset.
6. Review scene tests for public API coverage rather than only internal implementation shape.
7. Add smoke tests for examples that are expected to be part of the release narrative.
8. Add generated-doc/example tests where possible so documentation code does not rot.
9. Define CI categories: fast CPU, GPU/Vulkan, examples, docs, wheels, and optional long-running
   validation.

Exit criteria:

1. `just test` is meaningful and expected to pass on supported developer platforms.
2. GPU-dependent tests are clearly separated and documented.
3. Example smoke tests exist for release-critical examples.
4. Tests for intentionally unsupported behavior are explicit and not confused with failures.


### M4: Repository Markdown Audit

Goal: make all committed Markdown consistent with the actual v0.4 state.

Work:

1. Audit root docs: `README.md`, `BUILD.md`, `ARCHITECTURE.md`, `CONTRIBUTING.md`,
   `MAINTAINERS.md`, `CHANGELOG.md`, `AUDIT.md`, `ADVICE.md`, and `CLAUDE.md`.
2. Audit the legacy `docs/` tree only as migration input for future public documentation; do not
   add new v0.4 architecture records there.
3. Audit `spec/` for DRP2 and any scene specs that become normative.
4. Audit `examples/*/README.md`.
5. Decide what stays as public documentation, what becomes developer architecture notes, and what
   moves to historical task records.
6. Classify each public-facing page as tutorial, how-to guide, reference, or explanation. Split or
   move pages that mix incompatible reader goals.
7. Remove or clearly mark v0.3-era claims that no longer match v0.4.
8. Normalize terminology: scene, visual, panel, figure, controller, frame plan, DRP2 stream,
   runtime, canvas, stream sink, offscreen, WebGPU, video, GUI.
9. Normalize warnings and feature-state labels: stable, experimental, advanced, backend-specific,
   planned, deferred.
10. Check all Markdown links, image references, and code fences.

Exit criteria:

1. No main public doc page describes obsolete v0.3 APIs as the current release.
2. Every experimental feature has a visible status note.
3. Markdown link checks pass or documented external exceptions are recorded.
4. The repo has a clear split between user docs, specs, architecture notes, agent plans, and task
   records.
5. User-facing pages have an explicit documentation type, and pages with mixed goals are split or
   intentionally justified.


### M5: Documentation System Decision

Goal: choose the website toolchain before writing the final documentation at scale.

Current situation:

1. The repository has `mkdocs.yml`, `tools/mkdocs_hooks.py`, and an existing `docs/` tree.
2. Material for MkDocs is no longer a low-risk default. The official changelog says the project is
   in maintenance mode and will provide critical bug fixes and security updates for at least 12
   months, while its MkDocs 2.0 post explains incompatibility risks around the MkDocs rewrite:
   <https://squidfunk.github.io/mkdocs-material/changelog/> and
   <https://squidfunk.github.io/mkdocs-material/blog/2026/02/18/mkdocs-2.0/>.
3. The most direct successor is **Zensical**, built by the creators of Material for MkDocs. Its own
   documentation describes it as a modern static site generator, currently alpha, compatible with
   Material for MkDocs, and intended as a complete replacement for the MkDocs + Material stack:
   <https://zensical.org/about/> and <https://zensical.org/compatibility/>.
4. Zensical already documents preliminary `mkdocstrings` support, which matters for Python API
   pages, but the API-reference story is still explicitly evolving:
   <https://zensical.org/docs/setup/extensions/mkdocstrings/>.
5. A third-party continuity fork, **MaterialX** / `mkdocs-materialx`, is also visible. It is based
   on `mkdocs-material` and positions itself as ongoing maintenance for the familiar stack:
   <https://pypi.org/project/mkdocs-materialx/>. Treat it as a risk-mitigation candidate rather
   than the primary successor until its governance, compatibility, and long-term maintenance are
   reviewed.

Decision track:

1. Evaluate staying on MkDocs/Material temporarily for v0.4.
2. Evaluate Zensical as the primary successor candidate, especially its compatibility with current
   Material-style Markdown, plugin support, search, custom templates, gallery generation, and API
   documentation roadmap.
3. Evaluate MaterialX / `mkdocs-materialx` as a conservative fork/bridge option if Zensical is not
   ready but MkDocs/Material maintenance risk becomes unacceptable.
4. Evaluate Sphinx plus Breathe/Doxygen for the C API reference and MyST Markdown for narrative
   docs.
5. Evaluate Docusaurus if the website needs richer gallery interactivity and React components.
6. Evaluate a smaller custom static generator only if gallery automation or API reference needs
   cannot be met by maintained tools.

Decision criteria:

1. Maintenance outlook and community health.
2. Good Markdown authoring experience.
3. Full C API reference integration from headers.
4. Python API reference integration from generated bindings.
5. Gallery generation with screenshots, animated GIFs, videos, per-example pages, source code, and
   downloadable assets.
6. Search, versioned docs, redirects, link checking, and CI deployment.
7. Low friction for AI agents to edit and regenerate pages.

Exit criteria:

1. A documentation generator is selected.
2. The old `mkdocs.yml` path is either retained with a time-boxed migration note or replaced.
3. Site build runs in CI.
4. The docs source structure is stable enough for agent-assisted content generation.
5. If Zensical is chosen, the migration notes identify unsupported Material/MkDocs features,
   required config changes, plugin replacements, and remaining API-reference gaps.


### M6: Documentation Content

Goal: produce documentation that teaches the v0.4 architecture and public API honestly.

Recommended structure:

1. **Home:** what Datoviz is, what v0.4 supports, screenshots/videos, install links, and clear
   status.
2. **Tutorials:** first successful experiences for a beginner, such as first C scene, first Python
   scene, first image, first interactive app, and first offscreen capture. These should be guided,
   linear, and short; they should not try to become API reference or architecture essays.
3. **How-to guides:** narrow task recipes for users who know what they want to do, such as adding a
   visual, configuring controllers, using scales/colormaps, picking/probing, recording/replaying
   DVZR, exporting video, embedding GUI controls, running offscreen, and trying WebGPU if supported.
4. **Reference:** generated C API, Python raw bindings, visual capability tables, environment
   variables, feature/status tables, command-line options, example metadata, supported platforms,
   and lower-layer API facts. Reference pages should be terse, complete, and easy to scan.
5. **Explanation:** conceptual and architectural pages covering figure/panel/visual concepts,
   scene vs DRP2/vklite/canvas responsibilities, frame plans, ownership and borrowed handles,
   rendering techniques, performance model, backend limits, and release compatibility policy.
6. **Examples and gallery:** one page per example, generated from runnable source and captured
   media. Gallery pages can link to tutorial/how-to/reference/explanation pages, but should not
   replace them.
7. **Developer docs:** build, tests, architecture, contributing, release process, agent usage, and
   historical task records. Developer docs should stay visibly separate from user documentation.

Content requirements:

1. Every documented feature must have at least one runnable example.
2. Every public visual family must have one feature example and one gallery-quality example unless
   explicitly marked experimental.
3. Every public header should contribute to the generated C reference.
4. Every public function should have a useful one-sentence summary and parameter documentation.
5. Pages should include limitations where they matter instead of implying completeness.
6. Every public documentation page should declare or imply one primary type: tutorial, how-to,
   reference, or explanation.
7. Tutorials should optimize for learning and confidence, not feature exhaustiveness.
8. How-to guides should answer concrete tasks and link out to reference instead of duplicating it.
9. Reference should state behavior, inputs, outputs, constraints, defaults, and support level
   without narrative detours.
10. Explanation should cover why the system works the way it does, not step-by-step instructions.


### M7: Python Binding Readiness

Goal: ship raw Python bindings that are useful, generated, and honest about scope.

Assumptions:

1. v0.4 should keep raw bindings close to the C API.
2. ctypes remains attractive because v0.3 already used header parsing and generation scripts.
3. Higher-level OO bindings may belong in `gsp` / VisPy 2 rather than directly in the Datoviz
   Python package.

Work:

1. Audit existing Python package layout and generation scripts.
2. Decide the generated binding boundary: which headers, which symbols, which feature flags.
3. Update the header parser for v0.4 public headers.
4. Generate `ctypes` signatures, enums, structs, callbacks, constants, ownership wrappers, and
   docstrings where possible.
5. Add Python smoke tests for loading the shared library, creating/destroying core objects, simple
   scene rendering, image capture, and selected callbacks.
6. Add packaging tests for wheels and source distributions.
7. Document raw binding limitations and the intended role of future OO bindings.

Exit criteria:

1. Generated raw bindings load on supported platforms.
2. Python examples in docs are executable or clearly marked pseudocode/deferred.
3. Wheel CI is aligned with the v0.4 C library and dynamic dependency strategy.


### M8: Gallery and Example System

Goal: make the gallery the strongest proof that v0.4 is real.

Gallery categories:

1. **Feature examples:** one small example per feature, visual family, controller, interaction
   mode, export mode, and advanced layer.
2. **Scientific showcase examples:** polished examples using real or realistic data from multiple
   domains.
3. **Integration examples:** offscreen rendering, video export, external engine/canvas stream use,
   IPython, WebGPU, and GUI controls.
4. **Performance examples:** large point clouds, streaming traces, image stacks, mesh scenes,
   particles, and high-FPS live loops.

Scientific showcase candidates:

1. neuroscience: atlas slice, calcium imaging, tractography, DICOM-like volume;
2. astronomy: galaxy/star field, volume or point-cloud catalog;
3. geoscience: terrain flyover, global wind, weather fields;
4. biology/chemistry: protein viewer, molecular representation;
5. finance: market microstructure, order book, time-series panels;
6. physics/simulation: particles, Gray-Scott, fluid/vector fields;
7. imaging: microscopy, segmentation labels, multichannel images;
8. graph/network: spatial graph or node-link visualization if supported by v0.4.

Automation requirements:

1. Examples should include metadata: title, category, dependencies, expected runtime, backend
   requirements, data sources, license, gallery inclusion flag, and output media type.
2. Gallery generation should run examples, capture screenshots, animated GIFs or videos where
   appropriate, and generate one page per example with code.
3. CI should have a fast gallery smoke mode and a slower full gallery generation mode.
4. Example media should be deterministic enough for regression checks where possible.
5. Data download/cache helpers should handle offline failure gracefully.

Exit criteria:

1. The public website has both feature coverage and attractive showcase coverage.
2. Gallery pages link to exact source files.
3. Key examples produce reusable images/videos for the communication plan.


### M9: Packaging, CI, and Distribution

Goal: make installation and release artifacts reproducible.

Work:

1. Define supported platforms for v0.4: Linux, macOS, Windows, GPU requirements, driver
   requirements, and optional CUDA/video/WebGPU pieces.
2. Review CMake options for modular builds. Consider whether a documented lower-level build
   profile should exist for users who only want `canvas`, `stream`, `video`, or lower-level Vulkan
   pieces without the full scene layer.
3. Review dynamic dependency handling: Vulkan loader, GLFW, shader tooling, video encoders, CUDA,
   WebGPU/WASM assets, fonts, and gallery data.
4. Review `pyproject.toml`, wheel workflows, platform tags, shared-library bundling, and runtime
   search paths.
5. Add CI jobs for release artifacts, docs, gallery smoke, examples, and package install tests.
6. Add reproducibility notes: exact build commands, environment variables, and artifact naming.
7. Add release signing/checksum policy if needed.

Exit criteria:

1. A clean checkout can build, test, generate docs, and produce release artifacts from documented
   commands.
2. Wheels and source archives are tested after installation, not just built.
3. Optional features fail gracefully when dependencies are unavailable.


### M10: Legal, License, and Asset Review

Goal: avoid release surprises around dependencies and gallery assets.

Work:

1. Audit vendored third-party libraries and their licenses.
2. Audit binary assets, shaders, fonts, images, datasets, and generated gallery media.
3. Verify that all showcase datasets can be redistributed or downloaded under acceptable terms.
4. Add attribution pages for datasets and third-party assets.
5. Check generated bindings and docs for embedded third-party content.
6. Confirm package metadata, copyright notices, and license files.

Exit criteria:

1. Every shipped asset has a known license.
2. Dataset licenses are visible from gallery pages.
3. Release artifacts include required notices.


### M11: Release Candidate Process

Goal: make the final release a controlled sequence instead of a single risky tag.

Proposed sequence:

1. **v0.4.0-rc1: API and architecture candidate**
   Public headers, core docs outline, examples smoke, and broad tests are in place. Known visual
   polish issues may remain.

2. **v0.4.0-rc2: documentation and gallery candidate**
   Website structure, generated API reference, raw Python bindings, gallery automation, and core
   showcase examples are available.

3. **v0.4.0-rc3: packaging and quality candidate**
   Static analysis, memory/lifetime audit, CI matrix, wheels, install tests, link checks, and
   release notes are close to final.

4. **v0.4.0 final**
   Only release-blocking fixes since rc3. No new features unless they fix a release-critical gap.

Each release candidate should publish:

1. exact commit and tag;
2. feature status table;
3. known issues;
4. platform validation matrix;
5. test/static-analysis summary;
6. docs/gallery build links;
7. wheel/source artifacts;
8. migration notes from v0.3 and dev snapshots;
9. feedback request targeted at users and contributors.


### M12: Communication Plan

Goal: make the public release easy to understand and attractive to the right audience.

Key messages to refine:

1. Datoviz v0.4 is a new modular C visualization engine for scientific visualization.
2. The main user model is retained scene rendering with GPU-backed visuals and interaction.
3. Lower layers are available for advanced users who need rendering protocol, Vulkan runtime,
   canvas/stream/sink, offscreen, or video integration.
4. Python raw bindings expose the C API; higher-level OO Python usage may evolve through VisPy 2 /
   `gsp`.
5. v0.4 prioritizes architecture and performance over long-term API lock-in; v0.5 may still break
   API where needed.

Assets:

1. One short launch video.
2. Several 10-20 second clips from gallery examples.
3. High-quality screenshots for the README, website home page, LinkedIn, X, and release notes.
4. Architecture diagram showing scene -> frame plan -> DRP2 -> vklite/canvas -> stream/video.
5. Comparison-style examples inspired by strong galleries from Matplotlib, pygfx, VisPy, napari,
   yt, vtk/pyvista, and scientific domain tools, while keeping Datoviz's own visual identity.

Channels:

1. GitHub release.
2. Project website.
3. LinkedIn post.
4. X post/thread.
5. Relevant scientific Python, visualization, and GPU communities.
6. Direct notes to likely early adopters and collaborators.

Exit criteria:

1. Release notes are written before final tagging.
2. README and website home page have current screenshots and crisp positioning.
3. Social media assets are generated from real gallery examples.
4. Known limitations are not hidden.


### M13: Scientific Dataset Outreach

Goal: turn the gallery into a release and adoption engine by building examples around recently
released scientific datasets, then contacting the scientists individually with useful visuals.

Rationale:

1. Datoviz is strongest when it shows real scientific data rather than synthetic demos.
2. Scientists who recently released interesting public data are natural early users, reviewers, and
   amplifiers if Datoviz can make their data look better or easier to explore.
3. A gallery example built from a real dataset can serve several purposes at once: documentation,
   visual regression smoke, benchmark, social media asset, and personalized outreach.

Workflow:

1. Build a target list of scientists and groups across disciplines who have recently published
   open datasets, preprints, papers, challenge data, or public repositories with clear visualization
   potential.
2. Prioritize datasets with permissive licenses, stable download links, manageable size, and strong
   visual structure: images, volumes, point clouds, meshes, trajectories, vector fields, time
   series, graphs, simulations, or geospatial rasters.
3. For each selected dataset, create a small Datoviz example that is useful to the domain expert,
   not just visually decorative.
4. Generate at least one high-quality still image and, where appropriate, a short video or animated
   GIF from the example.
5. Write a one-page gallery entry with dataset attribution, scientific context, source code,
   dependencies, and license.
6. Contact the scientist or group individually with a concise message: what was visualized, why the
   result may be useful, a link to the gallery page, the image/video, and an invitation for feedback
   or correction.
7. Ask for permission before implying endorsement, using quotes, or featuring names prominently in
   launch material.
8. Track responses, requested corrections, attribution requirements, and follow-up example ideas.

Suggested disciplines:

1. neuroscience and microscopy;
2. astronomy and cosmology;
3. climate, meteorology, oceanography, and geoscience;
4. biology, structural biology, genomics, and biophysics;
5. medical imaging;
6. fluid dynamics and computational physics;
7. materials science;
8. finance and market microstructure;
9. robotics, motion capture, and spatial trajectories;
10. network science and graph analysis.

Agent-assisted tasks:

1. Search recent open datasets by discipline and produce a candidate table with authors, links,
   license, data size, data type, visualization opportunity, and contact route.
2. Check dataset license and citation requirements before any example is committed.
3. Prototype a Datoviz scene for one dataset at a time, with generated screenshot/video output.
4. Draft a gallery page and a personalized outreach email for human review.
5. Compare visual ideas against galleries from Matplotlib, pygfx, VisPy, napari, yt, VTK/PyVista,
   and domain-specific tools.

Outreach candidate table fields:

| Field | Purpose |
| --- | --- |
| Scientist / group | Who should receive the individual note. |
| Discipline | Helps balance the release gallery. |
| Dataset / paper | Source of the data and scientific context. |
| Release date | Ensures the outreach is timely. |
| License / terms | Confirms whether the data and derived media can be used. |
| Data type / size | Determines feasibility for a v0.4 example. |
| Datoviz feature fit | Maps the dataset to visuals, interaction, video, GUI, or WebGPU. |
| Planned output | Screenshot, video, notebook, C example, Python example, or web demo. |
| Contact route | Email, lab contact form, GitHub issue, repository discussion, or social media. |
| Status | Candidate, approved, implemented, contacted, replied, declined, or published. |

Exit criteria:

1. A reviewed outreach candidate table exists before the final release.
2. At least several gallery examples use real public datasets with correct attribution.
3. The launch asset set includes images/videos from these real-data examples.
4. Individual outreach messages are drafted and reviewed before they are sent.
5. Any scientist feedback that affects correctness or attribution is handled before public launch.


## AI Agent Operating Model

AI agents should be used heavily, but with a release-engineering discipline.

Recommended pattern:

1. Create a small task record for each release-readiness slice.
2. Give the agent a bounded scope: module, doc directory, test family, example group, or analysis
   tool output.
3. Require the agent to state what it changed, what it validated, and what remains uncertain.
4. Prefer many narrow PRs over one broad release-polish branch.
5. Keep generated rewrites reviewable. For docs, ask agents to preserve factual claims and update
   status rather than invent unsupported behavior.
6. For documentation tasks, require the agent to state the intended page type: tutorial, how-to,
   reference, or explanation. Do not let one generated page mix guided lessons, recipes, API facts,
   and architecture rationale unless the mix is explicitly intentional.
7. Use agents for first-pass audits, then have a human decide API names, public promises, release
   messaging, and deprecation policy.
8. For static analysis, ask agents to triage findings into true defects, false positives, tool
   limitations, and follow-up tickets.
9. For gallery work, ask agents to produce runnable examples plus metadata and generated media,
   then review visual quality manually.

Good agent task examples:

1. "Audit `include/datoviz/scene*.h` for naming and docstring consistency; propose changes before
   editing."
2. "Run `clang-tidy` on touched `src/drp2` files and fix only clear ownership or bounds issues."
3. "Rewrite `docs/guide/interactivity.md` to match current v0.4 scene APIs; do not document
   unimplemented Python features."
4. "Create gallery metadata for all examples under `examples/c/`; do not change example code."
5. "Generate a table of public symbols grouped by module and mark undocumented functions."
6. "Smoke-test all gallery examples in offscreen mode and record failures with logs."

Tasks that should remain human-led:

1. final API naming decisions;
2. release scope cuts;
3. compatibility policy;
4. claims in launch messaging;
5. visual taste and example selection;
6. licensing decisions for datasets and assets.


## Release-Blocking Checklists

### Code

1. Public headers reviewed.
2. Public symbols inventoried.
3. Ownership and destroy rules reviewed.
4. Error paths and partial initialization reviewed.
5. Static analysis run and triaged.
6. Memory/UB checks run where practical.
7. Vulkan validation smoke passed for graphics paths.
8. Long-running live-loop smoke passed for selected examples.


### Tests

1. `just build` passes from clean checkout.
2. `just test` passes or known platform exclusions are documented.
3. Narrow module tests pass for active modules.
4. Example smoke tests pass.
5. Docs code snippets are tested where practical.
6. CI matrix matches supported release platforms.


### Documentation

1. Documentation generator selected.
2. Website builds in CI.
3. Public Markdown audited for v0.4 status.
4. C API reference generated.
5. Python raw binding reference generated or documented.
6. Scene guide complete enough for first users.
7. Lower-layer docs explain DRP2, vklite, canvas, stream, offscreen, and video.
8. GUI, IPython, WebGPU, and video docs state exact support levels.
9. Link checks pass.


### Examples and Gallery

1. One example per public feature.
2. One page per gallery example with code and media.
3. Scientific showcase examples cover several domains.
4. Data licenses and dependencies are listed.
5. Gallery assets generated from current code.
6. README and launch assets use current gallery outputs.


### Packaging

1. Source archive builds.
2. Wheels build and install.
3. Installed package smoke tests pass.
4. Dynamic dependencies are documented.
5. Optional feature behavior is clear.
6. Checksums/signing policy decided.


### Communication

1. Release notes written.
2. Migration notes written.
3. Known issues written.
4. Website home page updated.
5. Social media images/videos prepared.
6. Feedback channels identified.


## Immediate Next Actions

1. Link this plan from `agents/README.md` so future agents see release readiness as a separate
   track from implementation.
2. Create a living v0.4 feature/status table once feature completion is closer.
3. Start the public-header API inventory early, before docs and bindings depend on unstable names.
4. Decide whether the documentation system migration should happen before rc1 or between rc1 and
   rc2.
5. Build a gallery metadata format before producing many examples, so agents can generate pages and
   media consistently.
6. Add a release-candidate template that records validation matrix, known issues, artifacts, and
   communication assets.
