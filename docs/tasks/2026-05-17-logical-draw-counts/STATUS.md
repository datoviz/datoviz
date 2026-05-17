# Logical draw counts vs retained buffer capacity

Date: 2026-05-17
Status: plan recorded; first segment slice committed

## Context

Axis/grid rendering exposed a generic retained-visual bug: when a visual's logical item count shrinks,
its retained GPU buffers may keep larger capacity, but draw commands must use the current logical count,
not the retained buffer byte size. Otherwise stale geometry can remain visible.

Recent related commits:

- `8f12f7a8 Use logical segment draw counts`
  - Adds `vertex_count`, `index_count`, and `instance_count` to `DvzFramePlanVisualMeta`.
  - Wires logical counts for `DVZ_VISUAL_TYPE_SEGMENT`.
  - Adds an axis regression where indexed draw count shrinks after tick/segment count shrinks.

## Architectural rule

Separate these concepts everywhere in typed scene rendering:

- **resource capacity**: how much GPU memory is allocated/retained for reuse.
- **logical draw count**: how many vertices, indices, or instances are valid for this frame.

Typed scene paths should treat nonzero metadata counts as authoritative. Deriving counts from resource
byte size should remain only a fallback for raw/legacy FramePlan paths or unconverted code.

## Current state

- Segment visuals now carry logical `vertex_count` / `index_count` through FramePlan metadata.
- The typed visual lowering path uses `meta->index_count` for segments when present.
- Other visual families still need preventive audit/wiring.

## Known unrelated dirty files at time of note

These files were already dirty and are not part of this task record:

- `examples/c/allen_mouse_brain_slice_glfw.c`
- `src/scene/frame_plan_runtime.c`
- `src/scene/tests/fields.c`
