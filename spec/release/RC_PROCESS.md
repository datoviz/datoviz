# Release Candidate Process

Use explicit release candidates. Each RC should have a written scope, known issues, validation matrix, generated artifacts, migration/status notes, and feedback request.


## RC1: API And Architecture Candidate

Exit criteria:

1. RC1 tag and notes exist;
2. build/test/spec validation is recorded;
3. feature table, visible parity table, known gaps, Python binding scope, and WebGPU/WASM scope are published or linked;
4. release examples are documented enough for early testers;
5. public headers, ownership rules, and lower-layer support labels have been reviewed.

Success criteria:

1. a small number of serious external testers, roughly 5-10, try the release candidate;
2. installation feedback arrives from Linux, macOS, and Windows;
3. at least several release examples run outside the main development machine;
4. reports distinguish build/install failures, rendering or driver bugs, example issues, API feedback, documentation gaps, and platform/GPU details;
5. public discussion does not confuse Datoviz v0.4 RC1 with VisPy 2.0 or with the final v0.4 release.


## RC2: Packaged Native-Window Hotfix

Exit criteria:

1. the macOS packaged Vulkan-loader/GLFW mismatch is fixed;
2. installed-wheel native windows have automated Linux/Xvfb regression proof, while hosted macOS retains offscreen MoltenVK render proof without claiming an interactive desktop session;
3. the exact canonical macOS wheel has explicit attended Quickstart evidence on the MacBook M3;
4. all six replacement wheels pass the normal RC artifact and conformance gates;
5. public guidance discloses the RC1 limitation until RC2 is available;
6. RC2 notes record physical Linux and Windows as unavailable exclusions, not passes; and
7. unrelated documentation, gallery, provider, and feature work remains deferred.


## RC3: Documentation, Packaging, Quality, And Tutorial API Candidate

Exit criteria:

1. documentation and gallery structure are mostly final;
2. generated C reference, captured artifacts, RC feedback triage, attribution, and gallery review are complete;
3. Qt/PyQt hosting has a tested packaged `datoviz_qtbridge` provider, preferably conda-first, without adding Qt to the base wheel;
4. the tutorial-enabling Canvas, vklite, shader-file, depth-attachment, OBJ-UV, and controller contracts are implemented from executable chapter spikes, documented, binding-validated where applicable, and proven through installed CMake consumers;
5. native scene, Canvas, test, and example shaders share one `glslc` build helper and named target-profile policy, normal builds no longer require `glslangValidator`, release products contain precompiled SPIR-V, and generated SPIR-V passes release/CI validation;
6. runtime shaderc is a focused thread-safe capability outside DRP2 pipeline code with a typed public availability, diagnostic, file, profile, and ownership contract; official packages guarantee its provider while disabled source builds preserve precompiled-SPIR-V rendering;
7. rewritten course chapters 1-3 are published with standalone installed-consumer, deterministic offscreen, bounded live GLFW, source-synchronization, and generated-preview proof;
8. packaging, licenses, generated artifacts, release notes, and the non-tutorial documentation surface are final candidates;
9. source archives and wheels build, install, compile external shaders, and pass installed smoke tests on supported platforms;
10. static-analysis, memory/UB, Vulkan validation, long-running loop, docs link, gallery smoke, and example smoke results are either clean or recorded as known issues;
11. checksums/signing policy and required third-party notices are decided;
12. RC3 feedback explicitly asks whether setup, first-window latency, C guidance, ownership, frame-command explanation, runtime diagnostics, resize behavior, and the tutorial API profile are suitable before chapters 4-15 are implemented.


## RC4: Tutorial And Installed Developer Experience Candidate

Exit criteria:

1. the tutorial is complete through an interactive textured and lit generated mesh and preserves the result-first, one-concept-at-a-time teaching contract;
2. tutorial-facing public APIs and ownership contracts are frozen except for recorded blockers;
3. generated geometry, the procedural asymmetric texture, their deterministic generation path, package/install behavior, and runtime discovery pass the course asset gate without a committed binary prerequisite;
4. every chapter builds through `find_package(datoviz CONFIG REQUIRED)` against exact installed packages, runtime-compiles its external shaders through the packaged shaderc path and frozen typed API, and has deterministic offscreen proof;
5. bounded GLFW resize, depth-recreation, arcball-input, repeated-frame, shutdown, and Vulkan-validation smokes pass on supported hosted platforms, with physical exclusions recorded rather than inferred;
6. tutorial source snippets, complete examples, screenshots, exercises, API links, compatibility labels, and navigation pass the strict documentation gates;
7. RC4 release notes ask for final installation, tutorial-completion, GPU/driver, interaction, asset-discovery, and conceptual feedback;
8. only recorded release or tutorial blockers remain before final v0.4.0.


## Final v0.4.0

Exit criteria:

1. `v0.4.0` is tagged and published with reproducible artifacts;
2. documentation and release notes are public, including the release-pinned modern GPU graphics tutorial;
3. launch screenshots, short clips, README/website assets, and announcement text are generated from current gallery examples and the final tutorial result where appropriate;
4. RC4 tutorial and installed-consumer feedback is resolved or recorded as a known limitation;
5. direct feedback channels are open for early users, especially scientists whose public datasets are used in showcase examples;
6. the active queue resets for v0.4 patch work and v0.5 planning.


## Required RC Notes

Every RC note should include:

1. exact commit and tag;
2. feature status table;
3. known issues;
4. platform validation matrix;
5. test/static-analysis summary;
6. docs/gallery build links;
7. wheel/source artifacts;
8. migration/status notes from v0.3 and development snapshots;
9. feedback request targeted at users and contributors.


## Feedback Triage

Each RC should have issue labels or project fields that separate:

1. installation and packaging failures;
2. rendering, driver, and platform-specific bugs;
3. example and gallery failures;
4. API and ownership/lifetime feedback;
5. documentation issues;
6. final-release blockers.

After each RC feedback window, summarize findings before the next candidate. RC1 feedback produced a narrow RC2 packaged native-window hotfix, moving its former planned scope to RC3. RC3 now proves the tutorial-facing API and rewritten chapters 1-3 in addition to the documentation, packaging, and quality surface; RC4 completes chapters 4-15 and freezes the tutorial and installed developer experience before final v0.4.0.
