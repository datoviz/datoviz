# Agent Instructions

[../AGENTS.md](../AGENTS.md) is the shared entry point. Root `CLAUDE.md` imports it for Claude Code. Read only the rules and task context selected by its routing table.

## Layout

- `rules/`: repository hygiene, C conventions, code and documentation validation, graphics ownership, and scene/DRP2 rules.
- [now/START.md](now/START.md): task and release dispatch.
- [now/STATUS.md](now/STATUS.md): current release blockers and evidence.
- [now/RELEASE.md](now/RELEASE.md): release sequencing.
- [rules/DOCUMENTATION.md](rules/DOCUMENTATION.md): public documentation validation.
- [now/DOCUMENTATION.md](now/DOCUMENTATION.md): documentation release inventory and review gates.

Read `STATUS.md` when work affects release scope or readiness; read `RELEASE.md` for release planning and packaging. Neither is mandatory startup context for ordinary implementation tasks.

## Maintenance

Keep always-needed constraints in root `AGENTS.md`, specialized rules in `rules/`, durable contracts in `spec/`, and public workflows in `docs/contributors/`. Link to one authoritative source instead of copying procedures or module inventories.

Keep `now/` limited to active, blocked, or protective handoffs. Preserve unresolved decisions and exact validation boundaries. Once a handoff is obsolete, retain any useful current fact in status or a durable spec and remove the handoff; Git history is the archive. Do not add `done/`, `soon/`, or `later/` queues.

Keep instruction content independent of model names and personal tool installations. Introduce repository skills only for recurring task procedures, with a shared source and explicit discovery for each supported agent; do not move unconditional safety rules into optional skills.

When changing instructions, run `just agent-instructions-check` and `git diff --check`, and review for conflicting rules. The checker covers repository `AGENTS.md`/`CLAUDE.md` files, this index, and `rules/*.md`; it checks local link target existence, shared imports, and prose wrapping. It does not validate URL availability, heading anchors, or instruction meaning. Check other Markdown files explicitly with `python3 tools/check_agent_instructions.py <path>`. Keep one paragraph or list item per source line.
