# Agents Index

This directory is organized by lifecycle state rather than by topic.


## Current Priority

Active execution surface:

1. [now/VK_REFACTOR.md](/home/cyrille/GIT/Viz/datoviz/agents/now/VK_REFACTOR.md)

Current status:

1. The active low-level graphics stack is green on this machine:
   - `just build` passed on `2026-03-23`
   - `just test` passed on `2026-03-23` with `146/146` tests passing
2. The highest-value next work is finishing `vk`/`vklite` public-boundary and lifecycle cleanup.


## Start Here

If resuming work on the branch:

1. Read [REFACTOR_STATUS_2026-03-23.md](/home/cyrille/GIT/Viz/datoviz/agents/REFACTOR_STATUS_2026-03-23.md)
2. Read [now/VK_REFACTOR.md](/home/cyrille/GIT/Viz/datoviz/agents/now/VK_REFACTOR.md)
3. Optionally read [later/SPLIT.md](/home/cyrille/GIT/Viz/datoviz/agents/later/SPLIT.md) if you
   want to continue packaging/CI cleanup after the boundary work
4. When the branch is ready to start higher-level spec work, read
   [now/DRP2_SPEC.md](/home/cyrille/GIT/Viz/datoviz/agents/now/DRP2_SPEC.md)
5. Treat
   [later/DRP2_WEBGPU_ROADMAP.md](/home/cyrille/GIT/Viz/datoviz/agents/later/DRP2_WEBGPU_ROADMAP.md)
   as strategic backlog, not as the default next-task list


## Directory Layout

### `now/`

Files in this directory are actionable today and should stay short enough to drive real execution.

1. [now/VK_REFACTOR.md](/home/cyrille/GIT/Viz/datoviz/agents/now/VK_REFACTOR.md)
2. [now/DRP2_SPEC.md](/home/cyrille/GIT/Viz/datoviz/agents/now/DRP2_SPEC.md)

### `done/`

Files in this directory are completed phase records. They are useful context, but they are not
current execution plans.

1. [done/EXTERNAL.md](/home/cyrille/GIT/Viz/datoviz/agents/done/EXTERNAL.md)
2. [done/OFFSCREEN.md](/home/cyrille/GIT/Viz/datoviz/agents/done/OFFSCREEN.md)
3. [done/OWNERSHIP.md](/home/cyrille/GIT/Viz/datoviz/agents/done/OWNERSHIP.md)
4. [done/PRESENTATION.md](/home/cyrille/GIT/Viz/datoviz/agents/done/PRESENTATION.md)

### `later/`

Files in this directory are backlog, strategic direction, or secondary cleanup tracks.

1. [later/SPLIT.md](/home/cyrille/GIT/Viz/datoviz/agents/later/SPLIT.md)
2. [later/DRP2_WEBGPU_ROADMAP.md](/home/cyrille/GIT/Viz/datoviz/agents/later/DRP2_WEBGPU_ROADMAP.md)


## Maintenance Rules

1. If a document is not actionable this week, it should not live under `now/`.
2. Completed plans should move to `done/` once the code and tests agree they are complete.
3. Long-horizon architecture and speculative work should live under `later/`.
4. Keep the number of active docs under `now/` small; one primary active plan is ideal.
5. On the `v0.4` branch, backward compatibility with earlier `v0.4` code or with `v0.3` is not a priority.
6. Prefer changes that improve architecture, correctness, and long-term maintainability, even when they
   require API or ABI breakage.
