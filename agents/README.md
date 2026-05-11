# Agents Index

This directory contains execution guidance for automation agents.

Stable scene semantics belong in [../spec/scene](/home/cyrille/GIT/Viz/datoviz/spec/scene).
Completed implementation records belong in `agents/done/`. Long-horizon backlog belongs in
`agents/later/`.


## Current Priority

Active execution notes:

1. [now/V0_4_NEXT_STEPS.md](/home/cyrille/GIT/Viz/datoviz/agents/now/V0_4_NEXT_STEPS.md)
2. [now/DRP2_SPEC.md](/home/cyrille/GIT/Viz/datoviz/agents/now/DRP2_SPEC.md)
3. [now/SCENE_PUBLIC_API_HEADER_PLAN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/SCENE_PUBLIC_API_HEADER_PLAN.md)

The scene design addenda that used to live in `agents/now/` now live under
[../spec/scene/proposals](/home/cyrille/GIT/Viz/datoviz/spec/scene/proposals).


## Start Here

If resuming work on the branch:

1. Read [now/V0_4_NEXT_STEPS.md](/home/cyrille/GIT/Viz/datoviz/agents/now/V0_4_NEXT_STEPS.md)
   for the current practical task list.
2. Read [../spec/scene/README.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/README.md) before
   changing scene semantics, public scene API shape, frame planning, visual families, interaction,
   annotations, scales, or runtime boundaries.
3. Read [../spec/scene/api/API_SURFACE.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/api/API_SURFACE.md)
   before changing public scene API shape or adding new scene object families.
4. Read [now/SCENE_PUBLIC_API_HEADER_PLAN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/SCENE_PUBLIC_API_HEADER_PLAN.md)
   to distinguish implemented public scene APIs from drafted-but-not-yet-implemented APIs.
5. Read [now/DRP2_SPEC.md](/home/cyrille/GIT/Viz/datoviz/agents/now/DRP2_SPEC.md) if touching
   `spec/drp2/`, `src/drp2/`, or DRP2-emitting scene code.
6. Read [done/SCENE_DRP2_IMPLEMENTATION.md](/home/cyrille/GIT/Viz/datoviz/agents/done/SCENE_DRP2_IMPLEMENTATION.md)
   and [done/DRP2_SCENE_SAFETY.md](/home/cyrille/GIT/Viz/datoviz/agents/done/DRP2_SCENE_SAFETY.md)
   when touching the completed first scene -> DRP2 -> runtime slice.
7. Treat [later/DRP2_WEBGPU_ROADMAP.md](/home/cyrille/GIT/Viz/datoviz/agents/later/DRP2_WEBGPU_ROADMAP.md)
   and [later/SPLIT.md](/home/cyrille/GIT/Viz/datoviz/agents/later/SPLIT.md) as backlog.


## Directory Layout

### `now/`

Short practical next-step notes. These files should answer what to do next and where to read the
normative spec. They should not be the long-term home for scene semantics. `SCENE_PUBLIC_API_HEADER_PLAN.md` is now a
header-state note, not an instruction to draft another broad API surface.

1. [now/V0_4_NEXT_STEPS.md](/home/cyrille/GIT/Viz/datoviz/agents/now/V0_4_NEXT_STEPS.md)
2. [now/DRP2_SPEC.md](/home/cyrille/GIT/Viz/datoviz/agents/now/DRP2_SPEC.md)
3. [now/SCENE_PUBLIC_API_HEADER_PLAN.md](/home/cyrille/GIT/Viz/datoviz/agents/now/SCENE_PUBLIC_API_HEADER_PLAN.md)

### `done/`

Completed phase records. These are useful context, but they are not current execution plans.

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
13. [done/CONTROLLER_TRANSFORM_DESIGN.md](/home/cyrille/GIT/Viz/datoviz/agents/done/CONTROLLER_TRANSFORM_DESIGN.md)

### `later/`

Backlog, strategic direction, or secondary cleanup tracks.

1. [later/SPLIT.md](/home/cyrille/GIT/Viz/datoviz/agents/later/SPLIT.md)
2. [later/DRP2_WEBGPU_ROADMAP.md](/home/cyrille/GIT/Viz/datoviz/agents/later/DRP2_WEBGPU_ROADMAP.md)


## Maintenance Rules

1. Keep `agents/now/` small and practical.
2. Move stable scene semantics to specialized files under `spec/scene/`.
3. Keep active not-yet-promoted design addenda in `spec/scene/proposals/`.
4. Move completed implementation records to `agents/done/`.
5. Move speculative or long-horizon execution ideas to `agents/later/`.
6. On the `v0.4` branch, prefer architecture, correctness, and maintainability over API or ABI
   compatibility with earlier work.
