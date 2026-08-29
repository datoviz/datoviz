# Repository Hygiene Rules

Repo hygiene covers branch policy, commits, generated files, documentation placement, and agent
handoffs.


## Branch Policy

Datoviz v0.4 is not constrained by v0.3 API or ABI compatibility. Prefer architecture,
correctness, maintainability, and testability even when that breaks old APIs.

Keep prose short and decision-oriented. When refactoring, update useful comments that become stale;
remove only comments that are wrong, redundant, or tied to deleted code.


## Commit Safety

Do not stage, commit, or push `data` submodule changes or generated/runtime binary payloads
without explicit user approval for those exact paths in the current turn. Unstaged or untracked
`data` working-tree state may be ignored only if it remains unstaged and uncommitted.

Before committing or pushing a `data` gitlink, run `python3 tools/check_submodule_reachability.py data`. The exact gitlink commit must already be reachable from the advertised remote branch configured in `.gitmodules`. Publish in this order: commit the `data` change, push its branch, pass the reachability check from the parent repository, then commit and push the parent gitlink. Never rely on a local submodule object cache as proof of remote availability.

Stop-sign paths:

1. staged `data` gitlink updates
2. `libs/vulkan/`
3. `*.dylib`, `*.so`, `*.dll`, `*.npy`, `*.npz`, `.DS_Store`

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

Never hard-wrap Markdown prose, including list items. Keep each paragraph or list item on one source line; use line breaks only where Markdown structure requires them, such as headings, blank paragraph separators, separate list or table rows, blockquotes, and fenced code blocks. This applies to public documentation, release notes, specifications, agent instructions, and GitHub text prepared from repository Markdown.

Keep `agents/` small. Prefer updating the current status or durable spec over adding another plan.
Delete obsolete agent notes once their useful facts are captured in code, tests, `spec/`, or git
history.


## Documentation Images

Small, intentional public documentation images may be committed under `docs/images/`. Prefer SVG for diagrams and WebP for raster images, and do not commit a raw capture when an optimized derivative is sufficient.

Committed raster documentation images should normally be no larger than 200 KB each. Larger exact assets require the user's explicit approval in the current turn. Generated gallery media remains in its existing generation and publication pipeline rather than being copied into `docs/images/`.

Externally authored images require a recorded source, author attribution, license or permission basis, and a clear ownership boundary. Store community-project media under `docs/images/community/<project>/` and keep it separate from validated Datoviz gallery assets.


## Vendored Code

Treat vendored code as read-only by default. Do not modify `external/` unless the task explicitly
requires it. Prefer fixing Datoviz-owned code in `src/`, `include/`, `testing/`, or build wiring.


## Orientation

Start from [../../AGENTS.md](../../AGENTS.md), then use [../now/START.md](../now/START.md) and the
nearest rule/spec file for the touched area.
