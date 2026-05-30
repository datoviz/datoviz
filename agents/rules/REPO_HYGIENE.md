# Repository Hygiene Rules

Repo hygiene covers branch policy, commits, generated files, documentation placement, and agent
handoffs.


## Branch Policy

Datoviz v0.4-dev is not constrained by v0.3 API or ABI compatibility. Prefer architecture,
correctness, maintainability, and testability even when that breaks old APIs.

Keep prose short and decision-oriented. When refactoring, update useful comments that become stale;
remove only comments that are wrong, redundant, or tied to deleted code.


## Commit Safety

Do not commit `data` submodule changes or generated/runtime binary payloads without explicit user
approval for those exact paths in the current turn.

Stop-sign paths:

1. `M data`
2. `? data`
3. staged `data` gitlink updates
4. `libs/vulkan/`
5. `*.dylib`, `*.so`, `*.dll`, `*.npy`, `*.npz`, `.DS_Store`

Before every commit:

```sh
git diff --check
git status --short
git diff --cached --stat
```

Verify the staged set excludes stop-sign paths, unrelated user changes, generated payloads, vendored
runtime libraries, and large binary assets.

For long multi-stage tasks, make logical checkpoint commits after relevant validation. Do not leave a
large approved plan as tens of unstaged modified files unless a blocker prevents safe commits.


## Documentation Placement

Use this routing:

1. Durable v0.4 design, semantics, architecture, and API contracts: `spec/`.
2. Public v0.4 user documentation: `docs/`.
3. Current release status and agent dispatch: `agents/now/`.
4. Repo-wide agent rules: `agents/rules/`.

The `docs/` tree may be aggressively rebuilt in place for v0.4 public documentation. Do not put
private implementation plans, scratch notes, or agent diaries there.

Keep `agents/` small. Prefer updating the current status or durable spec over adding another plan.
Delete obsolete agent notes once their useful facts are captured in code, tests, `spec/`, or git
history.


## Vendored Code

Treat vendored code as read-only by default. Do not modify `external/` unless the task explicitly
requires it. Prefer fixing Datoviz-owned code in `src/`, `include/`, `testing/`, or build wiring.


## Orientation

Start from [../../AGENTS.md](../../AGENTS.md), then use [../now/START.md](../now/START.md) and the
nearest rule/spec file for the touched area.
