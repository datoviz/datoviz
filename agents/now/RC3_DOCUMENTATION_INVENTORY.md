# RC3 Documentation Inventory

Status: implementation inventory, GPU-dependent media generation, and rewritten course previews complete; maintainer review, exact artifacts, and publication decisions remain. Updated: 2026-08-01.

Use [DOCUMENTATION.md](DOCUMENTATION.md) for documentation sequencing and [STATUS.md](STATUS.md) for repo-wide blockers.

## Freeze Inventory

| Surface | State | Remaining gate |
| --- | --- | --- |
| Information architecture and navigation | Ready for freeze | Maintainer review of the four-page visual pilot; branch-link reconciliation after cutover. |
| Generated C reference | Complete | Re-run at the exact release commit and after any public API change. |
| Python binding guidance | Complete | Re-run generated and authored checks at the exact release commit. |
| Feature status and known limitations | Complete | Add exact RC3 artifact issues when release scope freezes. |
| Dataset attribution and provenance | Complete | Review exact outreach drafts before external communication. |
| Gallery generation policy | Implemented | Preserve the canonical policy and focused checks. |
| Animation/card candidates | Build-local candidates current | Maintainer approval of exact publication bytes if promotion is desired. |
| Canonical screenshots | Reviewed Linux baseline promoted | Future replacements require the same repeatability, review, provenance, and exact-approval boundary. |
| Four-page visual-system pilot | Awaiting maintainer review | Approve visual density, colors, diagrams, and media choices before broad rollout. |
| Rewritten Vulkan course chapters 1-3 | Implemented with generated previews | Review voice, pacing, and previews; prove the first official package newer than RC2 and supported hosted platforms. |
| PR #132 | Triaged read-only | Decide whether to close as superseded and request focused successors for remaining topics. |
| External outreach | Not started | Approve exact content and publication action before contact. |

## PR #132 Disposition

Per-image present semaphore, Canvas shader compilation, Kvazaar/PThreads4W, and `DVZ_LOG_LEVEL` work are superseded by merged implementation. Vulkan fallback changes require a current focused reproducer. Reusing a pre-existing GLFW target, developer presets, opt-in macOS Vulkan-environment sanitization, and the `_time_utils.h` guard/comment remain possible focused successors.

Recommended maintainer action: ask the author to close PR #132 as superseded and open focused successors only for remaining needs. Do not publish that request without approval of the exact GitHub action and text.

## Remaining Decisions

1. Review the visual pilot.
2. Review rewritten course chapters 1-3 and their replacement previews.
3. Approve exact animation/card publication bytes if desired.
4. Decide PR #132 successor requests.
5. Draft RC3 notes and evidence after artifact scope freezes.
