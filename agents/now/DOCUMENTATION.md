# Datoviz v0.4 Documentation Status

Status: RC3 implementation inventory, rewritten course previews, and automated course prose review complete; maintainer review, exact release artifacts, and publication decisions remain. Local prose and PR state updated: 2026-09-17.

Use [RELEASE.md](RELEASE.md) for sequencing, [STATUS.md](STATUS.md) for repo-wide blockers, and `spec/docs/` plus `spec/release/` for durable policy. This file is the sole active RC3 documentation inventory.

## Completed RC3 Documentation Work

- The public v0.4 structure, navigation, status vocabulary, Known limitations page, and preserved `/v0.3/` route are implemented.
- Generated C reference covers all 1,597 exported functions across 13 pages with drift checking.
- Python guidance covers all 18 NumPy-adapted calls, three generated helpers, exact `datoviz.raw` use, ownership, callbacks, and the GSP/VisPy2 boundary.
- All 12 real or prepared dataset showcases pass attribution and provenance checks.
- Gallery tooling enforces canonical `1280x720` animation media, bounded FPS/CRF fallback, encoded-output validation, isolated deterministic parallelism, and capture-worker limits.
- All 38 animation frame caches were regenerated under the prepared-data-aware key, all 38 animation candidates are current, and all 29 MP4 card candidates plus posters pass their budgets; publication remains an exact-approval action.
- The nine invalidated still-cache records were regenerated and verified against the canonical images: eight are byte-identical and `features_panel_mixed_2d_3d` is pixel-equivalent with maximum channel delta 4 across 0.0911% of components.
- The designated Linux host produced two byte-identical 104-image screenshot runs; the approved 54 changed images were promoted through `data` commit `d72c72c` and parent gitlink commit `264517633` with machine-readable evidence.
- The four-page visual-system pilot passes strict build and software-rendered desktop/mobile inspection.
- PR #157 merged the refreshed embedding and platform-portability work at `fccf83061b05b7373f5caa1ba40bdecfc3578306` with green Linux, macOS, Windows, agent-instruction, and submodule-reachability checks. PR #136 is merged and superseded by #157; original PR #132 is closed as superseded.
- Rewritten course chapters 1-3, their canonical programs, source synchronization, and installed source-prefix smoke are implemented.
- Rewritten course previews are generated from the canonical programs with real stdout, exact flat-color validation, deterministic fixed-time animation, and no `data` dependency.
- The 2026-09-08 automated course prose review covers the overview and chapters 1-3. Setup now gives corrected Windows Developer PowerShell, Release-configuration source-install, package DLL `PATH`, and configuration-specific executable guidance; maintainer review of voice and pacing remains required.

The two bounded Fractal prose batches (`c4adddfd8` and `6d4affab6`) clarify twelve authored guide/reference pages, preserving code blocks, link targets, and technical qualifications. Strict documentation builds and status checks pass; [campaign evidence](FRACTAL_CODE_HARDENING.md) records their scope. Automated course prose review is complete; maintainer voice and pacing review and broad visual rollout remain pending.

## Remaining RC3 Documentation Work

1. Obtain maintainer review of the four-page visual pilot before broad rollout.
2. Review the rewritten course voice, pacing, API profile, ownership explanations, package-first instructions, and generated previews.
3. Approve exact animation/card publication candidates if they should replace canonical website assets.
4. Review the local RC3 release-notes draft; finalize its exact identity, validation evidence, and release-specific known issues only when artifact scope is fixed.
5. Review exact outreach drafts before any dataset-author or public GitHub communication.

## PR #132 Disposition

Per-image present semaphore, Kvazaar/PThreads4W, and `DVZ_LOG_LEVEL` work were integrated independently. The current vendored Vulkan-header fallback does not have a reproduced defect. The broad developer-preset overhaul remains deferred rather than entering RC3.

The contributor reviewed PR #136 and requested namespaced shader-tool settings and accurate no-compiler behavior. PR #157 incorporated those follow-ups together with parent-owned GLFW reuse, the guarded Windows lean-header policy, opt-in macOS Vulkan-environment sanitization, removal of the obsolete Canvas swizzle-shader pipeline, deterministic compiler-discovery controls, FetchContent and no-compiler regressions, and Windows CI hardening. It merged on 2026-09-17 at `fccf83061b05b7373f5caa1ba40bdecfc3578306` after all hosted checks passed. PR #136 is merged and its final comment records #157 as the refreshed successor. PR #132 was closed as superseded on 2026-09-17 after the approved final comment was published.

## RC4 Documentation Gate

Complete course chapters 4-16 and the epilogue through an interactive textured and lit generated mesh. Generate a preview for every chapter, freeze the tutorial-facing API and advanced/unstable compatibility profile, validate every chapter against exact installed artifacts on supported hosted platforms, and resolve or record reader feedback.

The required course uses generated geometry and a procedural texture. Suzanne and committed binary course assets are optional polish and must not become release blockers.

## Final Documentation Gate

Publish final feature status, known issues, platform limitations, installation/build guidance, Python and WebGPU scope, release notes, release-pinned course, final media, and announcement assets. Add exact Zenodo DOI/date metadata after archiving and submit or explicitly defer JOSS.

## Validation

Follow [documentation validation](../rules/DOCUMENTATION.md) for public documentation changes. The release and maintainer-review gates above apply when work affects release scope or readiness.
