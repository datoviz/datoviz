# Datoviz v0.4 Documentation Status

Status: RC3 implementation inventory and rewritten course previews complete; maintainer review, exact release artifacts, and publication decisions remain. Updated: 2026-08-01.

Use [RELEASE.md](RELEASE.md) for sequencing, [STATUS.md](STATUS.md) for repo-wide blockers, and `spec/docs/` plus `spec/release/` for durable policy. This file is the sole active RC3 documentation inventory.

## Completed RC3 Documentation Work

- The public v0.4 structure, navigation, status vocabulary, Known limitations page, and preserved `/v0.3/` route are implemented.
- Generated C reference covers all 1,578 exported functions across 13 pages with drift checking.
- Python guidance covers all 18 NumPy-adapted calls, three generated helpers, exact `datoviz.raw` use, ownership, callbacks, and the GSP/VisPy2 boundary.
- All 12 real or prepared dataset showcases pass attribution and provenance checks.
- Gallery tooling enforces canonical `1280x720` animation media, bounded FPS/CRF fallback, encoded-output validation, isolated deterministic parallelism, and capture-worker limits.
- All 38 animation candidates are current and within budget; publication remains an exact-approval action.
- The designated Linux host produced two byte-identical 104-image screenshot runs; the approved 54 changed images were promoted through `data` commit `d72c72c` and parent gitlink commit `264517633` with machine-readable evidence.
- The four-page visual-system pilot passes strict build and software-rendered desktop/mobile inspection.
- PR #132 is triaged read-only; most topics are superseded, focused successor PR #136 is open, and the original author's feedback is pending.
- Rewritten course chapters 1-3, their canonical programs, source synchronization, and installed source-prefix smoke are implemented.
- Rewritten course previews are generated from the canonical programs with real stdout, exact flat-color validation, deterministic fixed-time animation, and no `data` dependency.

## Remaining RC3 Documentation Work

1. Obtain maintainer review of the four-page visual pilot before broad rollout.
2. Review the rewritten course voice, pacing, API profile, ownership explanations, package-first instructions, and generated previews.
3. Approve exact animation/card publication candidates if they should replace canonical website assets.
4. Await the original author's feedback on focused successor PR #136, then resolve PR #136 and close PR #132 as superseded when appropriate.
5. Draft exact RC3 release notes, validation evidence, and release-specific known issues only when artifact scope is fixed.
6. Review exact outreach drafts before any dataset-author or public GitHub communication.

## PR #132 Disposition

Per-image present semaphore, Canvas shader compilation, Kvazaar/PThreads4W, and `DVZ_LOG_LEVEL` work are superseded by integrated implementation. Vulkan fallback changes require a current focused reproducer. Focused successor PR #136 carries the pre-existing GLFW-target reuse, opt-in macOS Vulkan-environment sanitization, and `_time_utils.h` guard/comment; the broader developer-preset overhaul remains deferred.

Current maintainer action: await the original author's feedback on PR #136. Keep PR #132 open until that feedback is resolved and PR #136 reaches a disposition, then close PR #132 as superseded when appropriate. Do not publish any GitHub action or text without approval of the exact action and content.

## RC4 Documentation Gate

Complete course chapters 4-15 and the epilogue through an interactive textured and lit generated mesh. Generate a preview for every chapter, freeze the tutorial-facing API and advanced/unstable compatibility profile, validate every chapter against exact installed artifacts on supported hosted platforms, and resolve or record reader feedback.

The required course uses generated geometry and a procedural texture. Suzanne and committed binary course assets are optional polish and must not become release blockers.

## Final Documentation Gate

Publish final feature status, known issues, platform limitations, installation/build guidance, Python and WebGPU scope, release notes, release-pinned course, final media, and announcement assets. Add exact Zenodo DOI/date metadata after archiving and submit or explicitly defer JOSS.

## Validation

Documentation-only changes require `git diff --check`, `just docs-build-check`, `just docs-status-check`, and inspection of `git status --short`. Generated references, examples, screenshots, animations, or inventories additionally require their focused generator and checker. Never hard-wrap Markdown prose.
