# Agents Index

This directory is organized by lifecycle state rather than by topic.


## Current Priority

Active execution surface:

1. [now/V0_4_NEXT_STEPS.md](/home/cyrille/GIT/Viz/datoviz/agents/now/V0_4_NEXT_STEPS.md)
2. [now/DRP2_SPEC.md](/home/cyrille/GIT/Viz/datoviz/agents/now/DRP2_SPEC.md)

Current status:

1. The low-level graphics stack cleanup has moved from active plan to completed context. The useful
   phase records live under `agents/done/`.
2. `drp2` and `scene` are active default-build modules. The first scene -> DRP2 -> vklite/canvas
   vertical slice exists, with focused tests and basic C examples.
3. The highest-value next work is to make that vertical slice less toy-like without broadening the
   architecture prematurely: more visual families, viewport/depth support, background API
   extensions, better examples, runtime/frame-target hardening, and focused validation.
4. The DRP2 spec/fixture lane remains active and should stay aligned with implementation changes.


## Start Here

If resuming work on the branch:

1. Read [now/V0_4_NEXT_STEPS.md](/home/cyrille/GIT/Viz/datoviz/agents/now/V0_4_NEXT_STEPS.md)
2. Read [now/DRP2_SPEC.md](/home/cyrille/GIT/Viz/datoviz/agents/now/DRP2_SPEC.md) if touching
   `spec/drp2/`, `src/drp2/`, or DRP2-emitting scene code.
3. Read [done/SCENE_DRP2_IMPLEMENTATION.md](/home/cyrille/GIT/Viz/datoviz/agents/done/SCENE_DRP2_IMPLEMENTATION.md)
   for the completed first vertical slice.
4. Read [done/DRP2_SCENE_SAFETY.md](/home/cyrille/GIT/Viz/datoviz/agents/done/DRP2_SCENE_SAFETY.md)
   before changing runtime/frame-target lifetime, borrowed canvas frames, object tables, or failure paths.
5. Read [done/VK_REFACTOR.md](/home/cyrille/GIT/Viz/datoviz/agents/done/VK_REFACTOR.md) and
   [done/LOW_LEVEL_CONSISTENCY.md](/home/cyrille/GIT/Viz/datoviz/agents/done/LOW_LEVEL_CONSISTENCY.md)
   only when a task touches low-level ownership or naming contracts.
6. Treat [later/DRP2_WEBGPU_ROADMAP.md](/home/cyrille/GIT/Viz/datoviz/agents/later/DRP2_WEBGPU_ROADMAP.md)
   and [later/SPLIT.md](/home/cyrille/GIT/Viz/datoviz/agents/later/SPLIT.md) as backlog, not the default
   next-task list.


## Directory Layout

### `now/`

Files in this directory are actionable today and should stay short enough to drive real execution.

1. [now/V0_4_NEXT_STEPS.md](/home/cyrille/GIT/Viz/datoviz/agents/now/V0_4_NEXT_STEPS.md)
2. [now/DRP2_SPEC.md](/home/cyrille/GIT/Viz/datoviz/agents/now/DRP2_SPEC.md)

### `done/`

Files in this directory are completed phase records. They are useful context, but they are not
current execution plans.

1. [done/EXTERNAL.md](/home/cyrille/GIT/Viz/datoviz/agents/done/EXTERNAL.md)
2. [done/BOOTSTRAP_GPU_CTX_MIGRATION.md](/home/cyrille/GIT/Viz/datoviz/agents/done/BOOTSTRAP_GPU_CTX_MIGRATION.md)
3. [done/OFFSCREEN.md](/home/cyrille/GIT/Viz/datoviz/agents/done/OFFSCREEN.md)
4. [done/OWNERSHIP.md](/home/cyrille/GIT/Viz/datoviz/agents/done/OWNERSHIP.md)
5. [done/PRESENTATION.md](/home/cyrille/GIT/Viz/datoviz/agents/done/PRESENTATION.md)
6. [done/PROTO_FIXTURE_MIGRATION.md](/home/cyrille/GIT/Viz/datoviz/agents/done/PROTO_FIXTURE_MIGRATION.md)
7. [done/VK_REFACTOR.md](/home/cyrille/GIT/Viz/datoviz/agents/done/VK_REFACTOR.md)
8. [done/LOW_LEVEL_CONSISTENCY.md](/home/cyrille/GIT/Viz/datoviz/agents/done/LOW_LEVEL_CONSISTENCY.md)
9. [done/SCENE_DRP2_SPEC_DECISIONS.md](/home/cyrille/GIT/Viz/datoviz/agents/done/SCENE_DRP2_SPEC_DECISIONS.md)
10. [done/SCENE_DRP2_IMPLEMENTATION.md](/home/cyrille/GIT/Viz/datoviz/agents/done/SCENE_DRP2_IMPLEMENTATION.md)
11. [done/DRP2_SCENE_SAFETY.md](/home/cyrille/GIT/Viz/datoviz/agents/done/DRP2_SCENE_SAFETY.md)
12. [done/RENDER_PASS_BATCHING.md](/home/cyrille/GIT/Viz/datoviz/agents/done/RENDER_PASS_BATCHING.md)

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
