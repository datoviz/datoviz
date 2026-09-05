# Agent Instruction Follow-up

Status: updated 2026-09-05. These instruction improvements are not release gates and do not authorize publication or work outside the repository.

## Completed

- Shared Claude/Codex entry points, conditional task routing, routine implementation autonomy, corrected module guidance, and separated gallery procedures landed in `4754e066d`.
- Personal setup cleanup landed in `codex-workbench` commit `5ce103a`: the generic task-executor and placeholder skills were removed, task-handoff guidance now follows the target repository, and duplicated workflow policy was simplified. This addresses the identified personal-skill conflicts; fresh-session behavior has not yet been evaluated.
- [STATUS.md](STATUS.md) now prioritizes current gates and evidence links. Detailed local pre-freeze results were retained in [RELEASE.md](RELEASE.md#pre-freeze-local-evidence-recorded-2026-08-31); the compression does not revalidate release readiness or remote state.

## Recommended Order

1. Add a small instruction checker for local links, root and scoped Claude imports, and Markdown prose wrapping. Provide a local command and CI integration. Check structural properties rather than exact wording; account for code fences, tables, and external links.
2. Evaluate fresh Codex and Claude sessions using representative C-fix, public-header, documentation, and release-question tasks. Check rule discovery, reading volume, validation selection, unnecessary pauses, and publication boundaries. Use isolated checkouts for mutation-based trials and record observations rather than assuming the cleanup improves behavior.
3. Add focused binding/API or gallery validation skills only if those trials reveal recurring procedural gaps. Reuse canonical repository procedures, give each explicit triggers and completion criteria, and share procedures across Codex and Claude discovery paths. Retain unconditional safety and approval rules in `AGENTS.md`.

## Completion

Instruction edits require link, import, content, formatting, and `git diff --check` validation. Checker or skill changes also need focused behavior validation. Follow existing publication and protected-path rules. Remove this note when the selected work is complete and useful lasting guidance has moved to its canonical location.
