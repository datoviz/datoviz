# RC3 Documentation Inventory

Status: implementation inventory complete; maintainer review and GPU-dependent media gates remain. Updated: 2026-07-30.

Use [DOCUMENTATION.md](DOCUMENTATION.md) for release sequencing and [STATUS.md](STATUS.md) for repo-wide blockers.

## Freeze Inventory

| Surface | State | Evidence or remaining gate |
| --- | --- | --- |
| Information architecture and navigation | ready for freeze | Strict MkDocs build passes; only blocker-level corrections should follow after pilot review. |
| Generated C reference | complete | 1,578 exported functions are classified across 13 generated pages; generated output drift checking renders and compares the full reference. |
| Python binding guidance | complete | The public inventory covers 18 NumPy-adapted policy calls, three generated helpers, exact `datoviz.raw` calls, ownership, callbacks, and the GSP/VisPy2 boundary. |
| Feature status and known limitations | complete | Public status vocabulary is mechanically checked; the consolidated Known limitations page links exact platform, browser, compatibility, and provider boundaries. |
| Dataset attribution and provenance | complete | All 12 real/prepared dataset entries provide name, source link, license, citation guidance, preprocessing, and provenance; the manifest checker enforces the record. |
| Gallery generation policy | implemented | Canonical `1280x720` animation frames, bounded FPS/CRF fallback, output validation, deterministic parallel work, and capture-worker limits are committed. |
| Published animation/card freshness | regeneration required | Existing build-local output predates the new encoder fingerprint. Regenerate and inspect after the GPU driver is restored; do not commit media without exact approval. |
| Canonical source screenshots | revalidation and review required | Isolated comparison found one byte-identical and seven tightly pixel-equivalent candidates, but their cache records are stale under the finalized fingerprint. Ninety-six materially different recaptures require visual review and exact approval before any `data` change. The current gate is `stale=8, uncached=96`. |
| Four-page visual-system pilot | awaiting maintainer review | Get Started, Core concepts, Choose your layer, and Advanced pass strict build and software-rendered desktop/mobile inspection. Do not begin broad rollout until visual density, colors, diagram language, and media choices are approved. |
| Three-chapter Vulkan tutorial pilot | implemented; feedback remains | Compiled source, external GLSL, installed-package path, captures, reference synchronization, and focused checks are present. Maintainer voice/API-profile review and hosted/physical platform proof remain separate release gates. |
| External outreach | not started | No dataset-author message or public GitHub follow-up was posted. Exact content and publication action still require approval. |

## PR #132 Triage

GitHub state inspected read-only on 2026-07-30: [PR #132](https://github.com/datoviz/datoviz/pull/132) remains open, non-draft, and non-mergeable against current `v0.4-dev`. Its July discussion already agreed to split independent changes after RC1.

| Original topic | Current disposition |
| --- | --- |
| Per-image present semaphore | Complete through merged [PR #133](https://github.com/datoviz/datoviz/pull/133), commit `08f1c3a41`, with subsequent recreation coverage. Remove from any successor to PR #132. |
| Canvas shader compiler fallback | Superseded by commit `f46ac75bd`, which makes the shared `glslc` helper authoritative across native shader products. |
| Kvazaar/PThreads4W on MSVC | Superseded by commits `e54575a85` and `891f0f723`, which unify the pthread target and derive the static-library define from target type. |
| `DVZ_LOG_LEVEL` in embedded/MSVC builds | Superseded by commits `c5038adbc` and `e5bb34631`, which apply environment configuration once on Windows and POSIX and preserve explicit runtime changes. |
| Vulkan header/package fallback | The original patch no longer fits the current dependency/package structure. Commits `d94f72dd6`, `2c8a49f3d`, and `1108d1b45` now export header requirements, publish C flags, and diagnose loader discovery; reassess only through a focused reproducer against the current build. |
| Reuse a pre-existing GLFW target | Still relevant for `add_subdirectory` consumers: the vendored branch currently calls `add_subdirectory(external/glfw)` before testing `TARGET glfw`. Review as one focused embedding change with a nested-consumer configure test. |
| Cross-platform developer presets | Not integrated. Rebase as a separate policy change that preserves current package smoke/install presets and validates CMake schema, generators, binary directories, and legacy names on hosted platforms. |
| Opt-in macOS Vulkan environment sanitization | Not integrated. Reassess separately against the current loader/SDK handoff and only after a reproducible conflicting-environment case; default behavior must remain unchanged. |
| `_time_utils.h` macro guard/comment | Still a valid narrow cleanup: guard `WIN32_LEAN_AND_MEAN` and update the stale `timeval` comment, with an MSVC compile check. Keep it separate from build-system changes. |

Recommended maintainer action: ask the author to close PR #132 as superseded and open focused successors only for the remaining items they still need. Do not post that request or close the PR without explicit approval of the exact GitHub action and text.

## Remaining RC3 Decisions

1. Review the four-page visual pilot before broad documentation rollout.
2. Reboot or repair the GPU driver, then regenerate and validate animation/card media under the new pipeline.
3. Review the 96 isolated screenshot differences and approve exact canonical replacements, if any, before touching `data`.
4. Review the three-chapter tutorial voice/API profile.
5. Decide whether to request focused successor PRs for the residual PR #132 topics.
6. Draft exact RC3 release notes and validation evidence only when the release artifact scope is fixed.
