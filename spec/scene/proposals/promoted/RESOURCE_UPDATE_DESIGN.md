> **Execution Status**
> - **Status:** `MOSTLY PROMOTED`
> - **Updated on:** `2026-05-20`
> - **Purpose:** preserve retained-update rationale and remaining API naming notes after promotion
>   into the resource and invalidation specs.

# Resource Update Design

This is a promoted proposal record. Active retained-resource and dirty-tracking rules live in the
pipeline specs.


## Decision Addressed

Partial mutation is a first-class scene requirement, not a backend optimization. The original
proposal covered full replacement, contiguous buffer subranges, grouped spans, texture regions,
atlas append/patch updates, and simple full replacement for small parameter blocks.


## Canonical Specs

Active rules moved to:

1. [`../../pipeline/RESOURCE_MODEL.md`](../../pipeline/RESOURCE_MODEL.md) for resource classes,
   ownership, mutability, resource facets, and dirty tracking.
2. [`../../pipeline/INVALIDATION_AND_CACHING.md`](../../pipeline/INVALIDATION_AND_CACHING.md) for
   invalidation scopes, incremental update rules, grouped updates, and redraw/rebuild policy.
3. [`../../pipeline/FRAME_PLAN.md`](../../pipeline/FRAME_PLAN.md) for explicit upload/readback work.
4. [`../../validation/VALIDATION.md`](../../validation/VALIDATION.md) and
   [`../../validation/DIAGNOSTICS.md`](../../validation/DIAGNOSTICS.md) for validation and
   diagnostic reporting.

Current implementation grounding includes public visual subrange updates in [`../../../../include/datoviz/scene.h`](../../../../include/datoviz/scene.h), retained visual state in [`../../../../src/scene/core/_scene.h`](../../../../src/scene/core/_scene.h), and upload emission in [`../../../../src/scene/scene_emit/uploads.c`](../../../../src/scene/scene_emit/uploads.c).


## Rationale To Preserve

The shared update contract prevents every visual family from inventing a private mutation path for:

1. point/path partial updates,
2. mesh vertex and index edits,
3. image tile or subimage uploads,
4. glyph atlas population and growth,
5. selection/highlight-derived updates,
6. dynamic annotations and overlays.

The public contract should remain resource-oriented and item/region-oriented, not Vulkan-oriented
or byte-offset-first.


## API Naming Sketch

Exact names can still move, but keep these shapes as public-surface candidates:

1. `*_replace(...)`
2. `*_update_range(...)`
3. `*_update_vertices(...)`
4. `*_update_indices(...)`
5. `*_update_region(...)`

The point is a shared retained-mutation vocabulary across visual families.


## Remaining Backlog

1. Add mesh resource vertex/index subrange update coverage.
2. Add image and atlas texture-region update coverage.
3. Keep point/path-like dirty-range merging covered by focused tests.
4. Prove second-frame no-upload behavior after clean frames.
5. Keep pick-resolution tables consistent after subrange updates and explicit full replacements.
6. Keep serialization based on authoritative full resource state; dirty ranges are runtime/editor
   state and should not serialize as durable scene content.


## Non-Goals Still Valid

1. Arbitrary sparse multi-rectangle texture patch APIs in the first slice.
2. Backend-specific staging-buffer controls in public scene APIs.
3. Automatic diffing of arbitrary large resources.
4. Exposing byte-offset mutation as the primary user-facing scene API.
