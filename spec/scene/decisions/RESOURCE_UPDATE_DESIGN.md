> **Execution Status**
> - **Status:** `SCENE SPEC DECISION RECORD`
> - **Updated on:** `2026-05-09`
> - **Purpose:** define the intended v0.4 retained update contract for scene geometry, textures,
>   glyph atlases, and other mutable resources.

# Resource Update Design

This note narrows the larger scene resource model into the active update contract needed now for
mesh, text, image, and picking-related retained resources.


## Objective

Keep scene resources retained and backend-agnostic while making partial mutation first-class for:

1. vertex and index buffers,
2. flat per-item tables,
3. grouped item tables,
4. texture regions,
5. glyph atlas growth and patching,
6. small parameter blocks where full replacement remains acceptable.


## Existing Grounding In The Repo

The current branch already has real retained dirty-range behavior:

1. public visual subrange updates in
   [include/datoviz/scene.h](/home/cyrille/GIT/Viz/datoviz/include/datoviz/scene.h)
2. dirty-range tracking in
   [src/scene/_scene.h](/home/cyrille/GIT/Viz/datoviz/src/scene/_scene.h)
3. upload emission from dirty state in
   [src/scene/scene.c](/home/cyrille/GIT/Viz/datoviz/src/scene/scene.c)
4. broader resource-model context in
   [spec/scene/pipeline/RESOURCE_MODEL.md](/home/cyrille/GIT/Viz/datoviz/spec/scene/pipeline/RESOURCE_MODEL.md)

This note defines what should remain stable as the system grows beyond point and image visuals.


## Core Recommendation

Subrange and region updates are Phase-1 requirements, not deferred optimizations.

Recommended first-class update families:

1. full replacement,
2. contiguous buffer subrange update,
3. grouped-span update built on top of contiguous subranges,
4. texture region update,
5. atlas append/patch update.

The public contract should be resource-oriented, not Vulkan-oriented.


## Why This Matters Now

This is already needed for active or near-active work:

1. point/path partial updates,
2. mesh vertex/index edits,
3. image tile or subimage uploads,
4. glyph atlas population and growth,
5. pick/highlight-related derived updates,
6. dynamic annotations and overlays.

If the API only models full replacement, every one of these systems will end up with its own
private mutation path.


## Update Targets

The update contract should distinguish resource classes explicitly.

Recommended classes:

1. flat buffer-like resources
2. indexed geometry resources
3. grouped item resources
4. texture resources
5. small parameter/uniform resources

Each class needs different update semantics even if the backend ultimately records buffer or texture
writes.


## Flat Buffer Updates

Flat per-item or per-vertex tables should support:

1. full replace,
2. contiguous item-range update,
3. dirty-range merging across multiple updates before the next emit.

The current `dvz_visual_set_data_range()` behavior is already the right active precedent.

Recommendation:

1. keep item-based API at the scene boundary,
2. convert to byte offsets internally,
3. merge overlapping or adjacent dirty spans before frame-plan emission.


## Indexed Geometry Updates

Mesh and other indexed resources need more than a single generic attribute update.

Required first-pass operations:

1. full geometry replacement,
2. vertex subrange update,
3. index subrange update.

Recommended rule:

1. geometry updates target the explicit mesh resource,
2. visual styling state stays on the visual,
3. face ordering must remain stable across partial updates unless the resource is replaced
   explicitly.


## Grouped Item Updates

Grouped resources such as text runs or paths should support span-aware mutation.

Recommended semantics:

1. update one or more contiguous item spans,
2. preserve stable span identity for picking and layout,
3. permit internal flattening into one or more merged dirty writes where possible.

The public API should not force callers to think in bytes, but it should also avoid pretending every
group update is independent if the implementation stores one flat item table underneath.


## Texture Region Updates

Texture region updates are required now.

Recommended baseline operations:

1. full replace,
2. 2D region upload `(x, y, width, height)`,
3. format-preserving subimage updates only,
4. row-stride validation at the scene boundary.

Initial scope can stay conservative:

1. base mip only,
2. one layer for ordinary 2D textures,
3. explicit later extension for mip/layer updates if needed.

This is enough for image visuals and the first text atlas path.


## Glyph Atlas Updates

Text makes the texture-update contract non-optional.

Recommended atlas behavior:

1. append new glyphs into the current atlas when space allows,
2. upload only the affected region,
3. grow or rebuild atlas explicitly when required,
4. mark dependent glyph-run resources dirty when UVs move because of atlas growth or rebuild.

This is one reason the generic resource update contract must acknowledge both texture-region writes
and cross-resource dirty propagation.


## Parameter Blocks

Small parameter/state blocks do not need complicated partial mutation at first.

Recommendation:

1. full replacement is acceptable for small uniform-style blocks,
2. if a block becomes large or heavily patched, promote it to a richer buffer resource instead of
   forcing every parameter object through subrange APIs.

This keeps the API simple where it can be simple.


## Dirty Tracking Model

Dirty tracking should remain scene-owned retained state.

Recommended tracked scopes:

1. whole resource dirty,
2. contiguous item span dirty for flat/grouped buffers,
3. contiguous index span dirty,
4. one or more dirty texture rectangles,
5. derived-resource invalidation when upstream changes require regeneration.

The scene should be free to coalesce dirty regions before planning.


## Merging Policy

The scene should merge dirty updates conservatively but intentionally.

Recommended rule:

1. merge overlapping spans,
2. merge near-adjacent spans when that clearly reduces plan noise and the wasted upload is small,
3. keep very distant updates separate if the frame-plan/runtime can benefit from it,
4. allow family-specific policy later if one resource class needs tighter control.

The public API should not expose this policy in backend terms.


## Mutation Safety

Retained mutation must respect frame ownership and borrowed-stream safety rules.

Recommended active rule:

1. resource mutation is rejected while a borrowed emitted stream is still live,
2. accepted mutation updates retained CPU-side state and dirty ranges only,
3. runtime upload work happens on the next valid emit.

This matches the current scene behavior and should stay consistent as explicit resource types are
added.


## Validation Requirements

Update APIs need strict validation because partial writes are easy to misuse.

Required checks:

1. resource exists and is mutable,
2. update range fits the current resource extent,
3. item size / format matches resource schema,
4. overflow-safe byte math,
5. texture region fits dimensions,
6. row-stride assumptions are explicit and validated.

Invalid updates should fail before planning rather than leaving inconsistent dirty state behind.


## Frame-Plan Contract

The frame plan should make resource updates explicit.

Recommended effect:

1. dirty resource scopes become upload/write nodes,
2. clean resources produce no upload work,
3. region/subrange granularity should be visible enough in tests to catch accidental full-resource
   regressions.

This is important for both performance and deterministic testing.


## Relationship To Picking

Picking depends on stable item or face ordering under partial mutation.

Recommended rule:

1. subrange updates preserve logical ordering,
2. full replacement may change ordering and must be treated as such,
3. scene-owned pick-resolution tables are rebuilt or patched consistently with the resource update.

This matters for point/scatter/path item picking and mesh face picking.


## Relationship To Serialization

The retained update model should not make serialization incoherent.

Recommended direction:

1. scene serialization records authoritative full resource state,
2. dirty ranges are runtime/editor state and do not need to serialize as durable scene content,
3. shared resources remain shared after load.


## Initial Public API Direction

The exact function names can still move, but the conceptual API should support:

1. replace resource contents,
2. update resource buffer subrange by item/index span,
3. update resource texture region,
4. query or validate current resource extents where needed.

Examples of the intended shape:

1. `*_replace(...)`
2. `*_update_range(...)`
3. `*_update_vertices(...)`
4. `*_update_indices(...)`
5. `*_update_region(...)`

The point is not the exact spelling. The point is making retained partial mutation a deliberate,
shared contract across visual families.


## Immediate Scope Recommendation

The first implementation work should explicitly cover:

1. mesh resource vertex/index subrange updates,
2. image and atlas texture-region updates,
3. continued dirty-range merging for point/path-like resources,
4. focused scene tests that prove second-frame no-upload behavior and partial-update emission.


## Explicit Non-Goals For The First Slice

1. arbitrary sparse multi-rectangle texture patch APIs with every layout variant,
2. backend-specific staging-buffer controls in the public API,
3. automatic diffing of arbitrary large resources,
4. exposing byte-offset mutation as the primary user-facing scene API.
