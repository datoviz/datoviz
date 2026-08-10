# Datoviz v0.4 Documentation Status

Status: RC3 implementation inventory and rewritten course previews complete; maintainer review, exact release artifacts, and publication decisions remain. Updated: 2026-08-08.

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
- PR #132 is triaged read-only; most topics are superseded, and focused successor PR #136 implements the original author's final shader-tool embedding feedback with a clean, fully green hosted matrix at `f3b47dbf5`.
- Rewritten course chapters 1-3, their canonical programs, source synchronization, and installed source-prefix smoke are implemented.
- Rewritten course previews are generated from the canonical programs with real stdout, exact flat-color validation, deterministic fixed-time animation, and no `data` dependency.

## Remaining RC3 Documentation Work

1. Obtain maintainer review of the four-page visual pilot before broad rollout.
2. Review the rewritten course voice, pacing, API profile, ownership explanations, package-first instructions, and generated previews.
3. Approve exact animation/card publication candidates if they should replace canonical website assets.
4. Obtain contributor confirmation on focused successor PR #136, merge the clean and fully green branch with explicit approval, then close PR #132 as superseded with separately approved public text.
5. Keep branch-specific links unchanged until the branch cutover and reconcile them atomically afterward.
6. Draft exact RC3 release notes, validation evidence, and release-specific known issues only when artifact scope is fixed.
7. Review exact outreach drafts before any dataset-author or public GitHub communication.

## PR #132 Disposition

Per-image present semaphore, Canvas shader compilation, Kvazaar/PThreads4W, and `DVZ_LOG_LEVEL` work are superseded by integrated implementation. Vulkan fallback changes require a current focused reproducer. Focused successor PR #136 carries the pre-existing GLFW-target reuse, opt-in macOS Vulkan-environment sanitization, and `_time_utils.h` guard/comment; the broader developer-preset overhaul remains deferred.

Current maintainer action: obtain contributor confirmation on PR #136. Keep PR #132 open until PR #136 merges, then close PR #132 as superseded after approval of the exact public action and text.

### PR #136 Finalization Handoff

PR [#136](https://github.com/datoviz/datoviz/pull/136) targets `v0.4-dev` from `fix/pr132-embedding-portability`. The exact validated head is `f3b47dbf54a6d6142151ef2ed5761b920fe65781`; hosted run [31270720274](https://github.com/datoviz/datoviz/actions/runs/31270720274) passes Linux tests, macOS tests, and the Windows build. GitHub reported the PR `CLEAN` and `MERGEABLE` after that run. The final CI-only fix replaced an unreliable filesystem path-hiding test with the namespaced hermetic setting `DVZ_GLSLC_AUTO_DISCOVERY=OFF`.

The remaining sequence is:

1. Check PR #136 for @chittti's downstream FetchContent confirmation, new review comments, a changed head, or newly required checks. Do not assume the recorded state is still current.
2. If confirmation is present, verify that the head still descends from `f3b47dbf5`, every required check is green, the PR remains clean and mergeable, and no unresolved feedback remains.
3. Obtain explicit maintainer approval for the exact merge action. Prefer a normal merge that preserves the six logical commits and their authorship; do not squash or rebase unless the maintainer explicitly chooses that history policy.
4. Merge through the authenticated `gh` CLI so the action appears under the maintainer's identity, then verify that `v0.4-dev` contains the merged PR head and that GitHub records PR #136 as merged.
5. Prepare the exact PR #132 supersession comment and close action, obtain explicit approval for both, publish through `gh`, and verify the resulting state. Do not close PR #132 before PR #136 merges.
6. Update `agents/now/STATUS.md`, `agents/now/RELEASE.md`, and this file on the active branch to record the merged SHA and resolved embedding lane; run the documentation checks and `git diff --check` before committing.
7. After merge verification, obtain approval for any remote branch deletion, delete `fix/pr132-embedding-portability` if approved, and remove the temporary local `agent/pr136-refresh` worktree/branch only when they are no longer needed. Never include `data` or unrelated main-worktree changes in cleanup commits.

Publication authority remains narrow: the current record does not authorize merging, closing PR #132, posting new comments, deleting the remote branch, or changing GitHub state. Each external action requires the approval defined in `AGENTS.md`; use the GitHub connector for read-only inspection and authenticated `gh` for approved mutations.

## RC4 Documentation Gate

Complete course chapters 4-15 and the epilogue through an interactive textured and lit generated mesh. Generate a preview for every chapter, freeze the tutorial-facing API and advanced/unstable compatibility profile, validate every chapter against exact installed artifacts on supported hosted platforms, and resolve or record reader feedback.

The required course uses generated geometry and a procedural texture. Suzanne and committed binary course assets are optional polish and must not become release blockers.

## Final Documentation Gate

Publish final feature status, known issues, platform limitations, installation/build guidance, Python and WebGPU scope, release notes, release-pinned course, final media, and announcement assets. Add exact Zenodo DOI/date metadata after archiving and submit or explicitly defer JOSS.

## Validation

Documentation-only changes require `git diff --check`, `just docs-build-check`, `just docs-status-check`, and inspection of `git status --short`. Generated references, examples, screenshots, animations, or inventories additionally require their focused generator and checker. Never hard-wrap Markdown prose.
