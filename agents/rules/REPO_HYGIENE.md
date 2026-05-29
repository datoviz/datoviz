# Repository Hygiene Rules

These rules cover branch policy, commit safety, documentation placement, and agent plan routing.


## Branch Policy

Datoviz v0.4-dev is not constrained by v0.3 API or ABI compatibility. Prefer architecture,
correctness, maintainability, and testability even when that means breaking old APIs.

When refactoring, do not delete existing comments. Keep them and update them when needed.


## Commit Safety

Do not commit changes inside the `data` submodule, or commit large binary files anywhere in the
repository, without explicit user approval for that specific commit.

Treat these as stop signs unless the user has explicitly approved them in the current turn:

1. `M data`
2. `? data`
3. Any staged `data` gitlink update
4. Generated/runtime binary payloads such as `libs/vulkan/`, `*.dylib`, `*.so`, `*.dll`, `*.npy`,
   `*.npz`, or `.DS_Store`

Before committing, run:

```sh
git status --short
git diff --cached --stat
```

Verify the staged set excludes unapproved data, vendored runtime libraries, generated outputs, and
large binary assets.


## Documentation Placement

The current `docs/` tree is legacy v0.3 public documentation. Do not put new v0.4 design notes,
specifications, implementation plans, or architecture records there.

Use this routing:

1. Durable v0.4 source-of-truth material: `spec/` or the closest existing `spec/` subtree.
2. Scene semantics and architecture: `spec/scene/`.
3. Execution status, handoff notes, and automation plans: `agents/`.
4. Completed implementation records and historical plans: `agents/done/`.
5. Long-horizon backlog: `agents/later/`.

Completed agent plans should not remain in active queues. When work tracked under `agents/now/` or
`agents/soon/` is done, remove the active plan, move or rewrite the final implementation record
under `agents/done/`, and update README/index links.


## Vendored Code

Treat vendored code as read-only by default. Do not modify files under `external/` unless the task
explicitly asks for changes there.

Prefer fixing Datoviz-owned code in `src/`, `include/`, `testing/`, or build wiring before patching
vendored dependencies.


## Current Orientation

Start with:

1. [../now/START.md](../now/START.md)
2. [../now/RELEASE.md](../now/RELEASE.md)
3. [../README.md](../README.md)

Use `agents/README.md` as the index for active, imminent, historical, and backlog work.
