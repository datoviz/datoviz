# Release Readiness

Datoviz v0.4 can be an aggressive architecture release with future API breaks still possible. It
should not ship as a loose development snapshot. A public release needs a coherent surface,
repeatable validation, supportable packaging, honest documentation, and useful examples.


## Freeze Policy

Feature freeze and API freeze are separate.

Feature freeze means the declared v0.4 capability set is present or explicitly classified. Required
before feature freeze:

1. native scene/app path covers the declared visual and interaction subset;
2. retained textured mesh has deterministic example or fixture proof;
3. WebGPU/WASM has an honest experimental subset with unsupported-feature diagnostics;
4. raw `ctypes` generation and smoke tests work for the intended public C surface;
5. v0.3 visible capability gaps are fixed, explicitly deferred, or external/GSP-owned;
6. core examples compile and exercise the release feature set.

API freeze means names, include paths, ownership, lifetimes, callbacks, and common workflows have
been reviewed as a coherent v0.4 surface. Required before API freeze:

1. public headers are inventoried and classified;
2. ownership, destroy order, callback lifetimes, polling, and readback lifetimes are documented;
3. lower-level APIs are marked supported, experimental, or advanced/unstable;
4. recoverable error/status behavior is distinguished from assertion-only invariants;
5. accidental public symbols are removed, renamed, or explicitly documented.

Patch releases may fix defects, but should avoid unnecessary source breakage after `v0.4.0`.


## Release Readiness Audits

Run these as bounded release tasks, not broad refactor branches:

1. public API inventory, docstrings, include structure, object ownership, callbacks, and
   recoverable error/status policy;
2. implementation safety audit for allocation, partial cleanup, Vulkan ownership, command-buffer
   lifetimes, sizes/counts/strides/downcasts, and long live-loop resource churn;
3. test-suite inventory separating release regressions, GPU/Vulkan validation, examples, docs,
   package install, and optional long-running checks;
4. public Markdown audit for obsolete v0.3 claims, broken links, mixed page types, and missing
   status labels;
5. package, license, asset, data, generated-media, and third-party-notice review before final
   artifacts.

Record actionable findings in the owning spec, docs gate, or issue. Do not let broad cleanup delay
`v0.4.0` unless it blocks the declared release surface.


## Validation Matrix

Each release candidate should record which of these ran and what remains excluded or known-broken:

1. `git diff --check`;
2. clean build from documented commands;
3. focused and full `just test` runs;
4. `just spec-check`;
5. example smoke tests;
6. docs build and link checks;
7. gallery smoke and selected capture generation;
8. static-analysis triage where practical;
9. ASan/UBSan or equivalent memory/undefined-behavior checks where practical;
10. Vulkan validation-layer smoke for graphics lifetimes, render targets, command buffers,
    swapchains, or synchronization;
11. source archive and wheel build/install smokes on supported platforms.


## Release-Blocking Checklists

Code:

1. public headers reviewed;
2. public symbols inventoried;
3. ownership and destroy rules reviewed;
4. error paths and partial initialization reviewed;
5. static-analysis findings triaged;
6. memory/UB checks run where practical;
7. Vulkan validation smoke passed for graphics paths;
8. long-running live-loop smoke passed for selected examples.

Documentation and examples:

1. feature/status table published;
2. known issues and unsupported variants published;
3. C API reference generated or outlined;
4. raw `ctypes` and WebGPU/WASM scope documented;
5. every public visual or feature has a minimal example or explicit status;
6. gallery media is generated from current code;
7. data licenses, dependencies, and attribution are listed.
8. citation metadata is release-ready: `CITATION.cff` is current, the public citation page explains
   version-specific and project-level citation, and the final Zenodo DOI/date placeholders are
   resolved before `v0.4.0`.

Packaging and assets:

1. source archive builds;
2. wheels build, install, and smoke-test;
3. dynamic dependencies are documented;
4. optional features fail gracefully when dependencies are unavailable;
5. every shipped asset has a known license;
6. generated media and public datasets have provenance;
7. installed CMake/pkg-config consumers compile against public headers without private include
   paths, including transitive dependencies exposed by those headers;
8. checksums/signing policy is decided.
9. the GitHub release is archived with Zenodo for final `v0.4.0`, and the release notes record both
   the version DOI and the concept DOI.

Scholarly citation:

1. a JOSS draft exists in `paper/` before RC1;
2. the draft has the required statement of need, state of the field, software design, research
   impact, AI usage disclosure, acknowledgements, and references sections;
3. JOSS submission status is recorded in release notes, but acceptance is not a blocker for
   publishing `v0.4.0`.

Current macOS arm64 evidence, recorded 2026-06-18: vendored package install, system-auto package
install, strict Homebrew-style source install, install-prefix audit, installed CMake consumer,
installed pkg-config consumer, host-native wheel build/inspect/check, and wheel CMake consumer
passed locally. Remaining packaging proof that cannot be completed on this Mac: hosted macOS 15
wheel repair/tag validation and Windows wheel/runtime consumer validation.
