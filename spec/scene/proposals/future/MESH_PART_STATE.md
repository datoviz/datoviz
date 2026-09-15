> **Execution Status**
> - **Status:** `FUTURE MESH PROPOSAL`
> - **Updated on:** `2026-09-15`
> - **Purpose:** define independently stateful indexed mesh parts for an early v0.5 candidate without introducing a generic grouping or programmable-visual framework.
> - **Baseline reviewed:** `a61553d734b4b778a570d0c9bb092d9d6ab0f636`

# Mesh Parts With GPU-Resident Per-Part State

## Decision

Add one mesh-specific capability: an indexed triangle-list mesh may define a fixed set of **parts**, where each part is a contiguous span of the mesh index buffer and carries small retained state.

A part has:

- an immutable indexed geometry span;
- a mesh-local affine transform;
- a color multiplier;
- an opacity multiplier;
- visibility;
- `item_state` bits for hover, selection, and future interaction state;
- a stable compact part index used as the mesh's `DVZ_SCENE_TARGET_ITEM` identity.

The initial portable lowering should use:

1. one mesh visual;
2. one shared vertex/index geometry;
3. one GPU-resident per-part state buffer with a CPU retained-state shadow;
4. one direct indexed draw per visible part;
5. `instance_count = 1` and `first_instance = part_index`;
6. the part-state buffer consumed as instance-rate vertex input.

This lowering reuses instancing hardware without exposing parts as instances. An instance repeats the complete geometry; a part selects a distinct index span from one geometry.

Do not begin with a generic group framework, multi-draw indirect, a custom shader API, a programmable vertex-transform hook, or per-vertex deformation. Those remain possible later generalizations or lowerings behind the same public semantics.

## Motivation And Boundary

The immediate pressure is an Allen mouse brain atlas represented by one large indexed triangle mesh containing roughly 1,000 anatomical components. Geometry is effectively immutable, while each component may independently change transform, visibility, color, opacity, hover/selection state, and picking identity. An explode control translates components away from the whole-brain centroid without rewriting geometry.

The same primitive applies to segmented anatomy, molecular assemblies, geological units, CAD or multibody models, finite-element regions, and other meshes partitioned by semantic region rather than material.

The renderer owns indexed mesh parts only. Atlas ontology, centroids, explode directions, labels, and hierarchy remain application state. In particular, do not add `dvz_mesh_explode()`.

This proposal extends the existing [`mesh` family](../../visuals/MESH.md), [`item-state system`](../../interaction/ITEM_STATE_SYSTEM.md), [`picking contract`](../../interaction/PICKING.md), [`transform pipeline`](../../pipeline/TRANSFORM_PIPELINE.md), [`transparency semantics`](../../semantics/TRANSPARENCY.md), and [`FramePlan`](../../pipeline/FRAME_PLAN.md). Those canonical specifications retain authority over their shared behavior.

## Vocabulary

| Term | Meaning |
| --- | --- |
| mesh geometry | Existing retained vertex attributes and index buffer. |
| part layout | Immutable mapping from compact part indices to index-buffer spans. |
| part state | Small mutable retained state associated with each part. |
| part index | Zero-based compact index in `[0, part_count)`. |
| stable application identity | Optional existing target link key associated with the part item. |

A part is not an independently owned mesh, visual, material, instance, or scene node. Prefer **part** over **region**, which is domain-specific, and **submesh**, which commonly implies independent geometry or material ownership.

## Geometry Contract

The first slice supports indexed triangle-list meshes only. Each part is represented by an index range expressed in index elements rather than bytes:

```c
typedef struct DvzMeshPartRange
{
    uint32_t first_index;
    uint32_t index_count;
} DvzMeshPartRange;
```

The initial invariants are:

1. `part_count > 0` while multipart mode is active.
2. Every part has `index_count > 0`.
3. `first_index` and `index_count` are multiples of three.
4. Ranges are sorted by `first_index`, disjoint, and form a complete partition of the active index buffer.
5. `base_vertex == 0` for the first implementation.
6. Part indices remain stable until `dvz_mesh_set_parts()` replaces or clears the layout.

The complete-partition rule makes face-to-part mapping, bounds, and diagnostics deterministic. Importers may reorder the index buffer once to make each region contiguous; vertex data need not be reordered.

Parts may share vertices. Parts own index spans, not vertices, so one source vertex may receive different part transforms when referenced by triangles in different spans. This is why a per-vertex part ID is not the baseline representation.

## Ownership And Retained State

`dvz_mesh_set_parts()` copies the supplied ranges. The mesh visual owns:

- the copied part layout;
- a CPU-side part-state shadow;
- one local AABB per part;
- state and draw-sequence dirty tracking;
- the GPU part-state resource.

Existing vertex and index resources remain ordinary mesh resources. `DvzGeometry` need not own multipart metadata in the first slice because it is commonly a temporary CPU helper destroyed after `dvz_mesh_set_geometry()`.

Replacing the part layout resets all part state. Passing `part_count == 0` clears multipart mode. Any ITEM target link-key array whose cardinality no longer matches the resulting item cardinality must be cleared or diagnosed rather than silently reused.

## Proposed Public C API

The public API should remain mesh-specific and batched:

```c
typedef enum DvzMeshPartFlags
{
    DVZ_MESH_PART_VISIBLE = 0x00000001u,
} DvzMeshPartFlags;

typedef struct DvzMeshPartStateUpdate
{
    uint32_t struct_size;
    uint32_t flags;

    uint32_t first_part;
    uint32_t part_count;

    const mat4* transforms;
    const DvzColor* tints;
    const float* opacities;
    const uint32_t* part_flags;
    const uint32_t* item_states;
} DvzMeshPartStateUpdate;

DVZ_EXPORT DvzResult dvz_mesh_set_parts(
    DvzVisual* mesh, const DvzMeshPartRange* ranges, uint32_t part_count);

DVZ_EXPORT DvzResult dvz_mesh_set_part_state(
    DvzVisual* mesh, const DvzMeshPartStateUpdate* update);

DVZ_EXPORT uint32_t dvz_mesh_part_count(const DvzVisual* mesh);
```

Every update array pointer is optional and `NULL` means unchanged. Every supplied array contains `part_count` elements beginning at `first_part`, and input is copied synchronously. The descriptor must follow the repository's normal `struct_size` and flags initialization and validation conventions.

Do not initially add one setter per property and part. Such functions would encourage thousands of FFI calls without adding capability.

### Initial State And Value Semantics

`dvz_mesh_set_parts()` initializes each part to:

| Field | Initial value |
| --- | --- |
| transform | identity |
| tint | white |
| opacity | `1` |
| part flags | `DVZ_MESH_PART_VISIBLE` |
| item state | `DVZ_ITEM_STATE_NONE` |

Transforms use one full `mat4` per part with the same matrix convention as `instance_transform`, must be finite, and represent mesh-local affine transforms. At the motivating scale, a full matrix costs little, reuses existing shader logic, supports arbitrary affine scientific transforms, and avoids committing to a translation/quaternion/scale decomposition. Visibility must not be encoded through a singular or zero-scale transform.

Tint multiplies the base mesh color, allowing exact per-part colors when the base is white while composing with vertex colors and lighting. Opacity is finite, lies in `[0, 1]`, and multiplies alpha after base color and tint. `item_states` uses existing `DVZ_ITEM_STATE_*` bits.

## Python Facade

The Python facade should accept NumPy arrays and make one raw C call per batch. An `_array_facade.py` specialization should:

1. convert inputs to exact dtypes;
2. require C-contiguous arrays;
3. infer `part_count`;
4. validate consistent leading dimensions and shapes;
5. populate one `DvzMeshPartStateUpdate`;
6. perform one ctypes transition without constructing a Python or ctypes object per part.

Representative usage:

```python
parts = np.column_stack((first_indices, index_counts)).astype(np.uint32)
dvz.dvz_mesh_set_parts(mesh, parts)

dvz.dvz_mesh_set_part_state(
    mesh,
    first_part=0,
    transforms=transforms,
    tints=tints,
    opacities=opacities,
    part_flags=flags,
)
```

An explode animation computes one transform per application-owned centroid and updates only `transforms`. Geometry and the index buffer remain untouched.

## Internal State And GPU Representation

The public API must not expose the GPU ABI. One possible internal 96-byte state is:

```c
typedef struct DvzMeshPartGpuState
{
    mat4 transform;
    vec4 color_opacity;
    uint32_t item_state;
    uint32_t flags;
    uint32_t reserved[2];
} DvzMeshPartGpuState;
```

At 1,000 parts this is approximately 94 KiB, so even a complete per-frame upload is small relative to atlas geometry. GPU-resident does not mean GPU-only: the CPU shadow remains authoritative for dirty tracking, bounds, visibility, query decoding, and validation.

The initial GPU representation should be one instance-rate vertex buffer exposing a transform, color/opacity, and state/flags. This reuses proven instance-transform machinery, is portable across Vulkan and WebGPU, and needs no new descriptor resource class. If richer shader variants later create vertex-attribute pressure, the same public API may lower to a storage buffer indexed by instance index.

## Common Draw Lowering

A multipart mesh remains one FramePlan visual and one logical `SceneDrawPacket`. Extend the normalized draw-packet contract with an owned or explicitly lifetime-bounded indexed subdraw sequence; do not add mesh-family branches to generic render emission.

Each subdraw contains at least:

```c
typedef struct SceneIndexedSubdraw
{
    uint32_t first_index;
    uint32_t index_count;
    uint32_t first_instance;
} SceneIndexedSubdraw;
```

An ordinary mesh retains its current single-draw fast path. A multipart mesh binds pipeline and resources once, then emits one direct draw for every visible part:

```text
index_count    = part.index_count
instance_count = 1
first_index    = part.first_index
base_vertex    = 0
first_instance = stable part index
```

Visible-subdraw ordinal and stable part index are distinct when hidden parts are skipped. Validation must cover every emitted subdraw's index and instance ranges.

Subdraw emission belongs below visual and pass selection so ordinary depth, picking, surface capture, WBOIT, and depth peeling receive identical part geometry without separate family-specific loops. No pipeline, bind-group, vertex-buffer, index-buffer, or uniform change occurs between parts.

### Backend Mapping

Vulkan lowers each visible part to `vkCmdDrawIndexed(part.index_count, 1, part.first_index, 0, part_index)`. WebGPU lowers it to `drawIndexed(part.indexCount, 1, part.firstIndex, 0, partIndex)`. In both cases, nonzero `first_instance` selects the instance-rate state and provides the compact part identity.

The direct loop is the common portable baseline. Vulkan multi-draw indirect, GPU-generated commands, or WebGPU-specific command strategies are later optimizations only if measurement shows command encoding to be material.

## Transform, Color, And Normal Semantics

The transform order is:

```text
source vertex -> part transform -> visual transform -> panel camera/MVP
```

Lit variants must transform normals using the inverse-transpose of the applicable part/model linear transform, following current instanced-mesh behavior.

For base color `C`, part tint `T`, and part opacity `a`:

```text
C_part.rgb = C.rgb * T.rgb
C_part.a   = C.a * T.a * a
```

Existing selected, unselected, and hovered styles are then applied to `C_part`. Material, lighting, texture binding, and alpha mode remain visual-level. The first slice does not add per-part materials, textures, or arbitrary uniforms.

## Visibility And Dirty State

Invisible parts emit no color, depth, surface-capture, transparency, or picking draw; are excluded from visual bounds; and cannot become a new hover or selection hit. CPU state therefore filters the direct subdraw sequence even though visibility is also retained in GPU state.

| Change | Geometry upload | Part-state upload | Bounds | Draw sequence | FramePlan or pipeline |
| --- | --- | --- | --- | --- | --- |
| replace/clear part layout | no | allocate/full | local and global | rebuild | multipart variant may change |
| transform | no | changed range | global | unchanged | unchanged |
| tint or opacity | no | changed range | unchanged | unchanged | unchanged |
| item state | no | changed range | unchanged | unchanged | unchanged |
| visibility | no | changed range | global | rebuild | unchanged |
| target link key | no | no | unchanged | unchanged | unchanged |
| vertex positions | existing path | no | local and global | unchanged | existing behavior |
| index contents | existing path | no | revalidate and recompute | possibly rebuild | existing behavior |

A single coalesced dirty interval is sufficient initially. A complete update is roughly 100 KiB at the motivating scale; a hover transition should update only the old and new records.

## Item Identity And Interaction

When parts are active, `DVZ_SCENE_TARGET_ITEM` means mesh part and `resolved_id` is the compact part index. Do not add `DVZ_SCENE_TARGET_PART`.

Application-stable identity uses existing target link keys:

```c
dvz_visual_set_target_link_keys(
    mesh, DVZ_SCENE_TARGET_ITEM, channel, keys, part_count);
```

This keeps rendering identity compact while allowing an application to associate an ontology or domain ID with each part.

The mesh item cardinality becomes:

```text
part_count     when multipart
instance_count when instanced
1              otherwise
```

The first implementation rejects simultaneous multipart and ordinary instanced mode, avoiding unresolved part-by-instance identity, bounds, addressing, and picking semantics.

`DvzHover` and `DvzSelection` continue operating on semantic item indices. The mesh family translates item-state writes into part-state records; interaction code must not know about part storage. A hover transition should dirty at most the prior and new part records.

## Picking

Part picking uses the same index span, transform, visibility, and `first_instance = part_index` as visible rendering. The query shader emits the mesh ITEM identity from the part/instance index, so an exploded part is picked at its rendered position rather than its original position.

First-slice query behavior is:

- `DVZ_QUERY_CAPABILITY_ITEM` is supported;
- ordinary nonmultipart FACE behavior is unchanged;
- multipart FACE queries are explicitly unsupported with a diagnostic.

Combined part-plus-face identity is a separate portable design problem and must not block part-level picking. A later query-only solution may carry static expanded triangle-corner metadata for global face and part identity while retaining part transforms in the shared state buffer.

## Bounds

When layout or indexed geometry changes, compute one mesh-local AABB per part from the vertices referenced by that part's index span. Shared vertices participate normally.

To obtain visual bounds, transform the eight corners of every visible part AABB by its part transform, union the results, then apply existing visual-transform handling. Transform or visibility changes dirty global bounds; tint, opacity, hover, and selection do not. Position or index changes invalidate local part bounds.

At 1,000 parts this requires only 8,000 transformed corners per recomputation. Existing camera-fit logic then sees exploded geometry through `dvz_visual_bounds()` without an atlas-specific API. GPU part culling is out of scope; CPU frustum rejection using these AABBs is a possible measured follow-up.

## Transparency And Render Products

Per-part opacity does not alter the visual-level alpha-mode contract. Opaque, blended, WBOIT, and depth-peeling modes continue assigning the entire visual to a pass; opacity only modifies fragment alpha within that route. Automatic subdivision into opaque and transparent part passes is deferred.

Every render product that consumes mesh geometry must observe part transforms and visibility. Surface position, normal, depth, coverage, picking, WBOIT, and depth-peeling passes should all receive the same subdraw sequence through common packet emission. Downstream GTAO and resolve/composite stages remain unaware of parts.

## Geometry Mutation

Position changes preserve the part layout but invalidate part-local and global bounds. Index-buffer replacement must revalidate every range. If the replacement is incompatible, diagnose the invalid multipart state and refuse to render until a valid layout is supplied; never silently clamp ranges.

Structural geometry and layout replacement are expected to be rare. Interactive state updates must never upload vertex or index data.

## Scope And Explicit Non-Goals

The first public slice includes:

- retained layout and CPU/GPU state;
- transform, tint, opacity, visibility, and item state;
- ITEM identity, link keys, hover, selection, and transformed bounds;
- direct indexed subdraw lowering shared by relevant render passes;
- existing visual-level alpha modes;
- Vulkan and live WebGPU;
- one batched Python facade.

The first slice excludes:

- multipart combined with ordinary mesh instancing;
- multipart FACE queries;
- textured/PBR-specific multipart variants;
- per-part materials, textures, or arbitrary uniforms;
- GPU culling or indirect draws;
- generic `PER_GROUP` attributes;
- programmable/custom shaders or vertex hooks;
- per-vertex deformation as a substitute for parts;
- application-domain atlas APIs.

Textured multipart meshes may follow after the ordinary built-in untextured mesh path is proven. Experience from mesh parts may later inform a generic grouping or programmable-visual design, but this use case alone is insufficient evidence for one.

## Alternatives Rejected For The Baseline

| Alternative | Reason not to choose it initially |
| --- | --- |
| one visual per part | Exceeds the current `DVZ_SCENE_MAX_RENDER_VISUALS == 128` scale/cap and duplicates scene metadata, resource resolution, bounds, and pipeline work. |
| per-draw uniforms | Requires rebinding or dynamic offsets; push constants do not provide a portable WebGPU solution. |
| ordinary instancing | Redraws the complete geometry for each transform. |
| one draw with per-vertex part ID | Cannot give a shared source vertex different transforms in different parts without duplication. |
| expanded or duplicated geometry | Increases geometry memory and duplicates other vertex attributes. |
| per-vertex deformation | Update cost scales with vertices and does not inherently solve identity, visibility, style, or shared vertices. |
| Vulkan multi-draw indirect | Adds command resources and synchronization before the portable semantics are proven. |
| generic grouping or programmable visuals | Forces unrelated API and shader-ABI decisions without a second demonstrated family use case. |

## Risks And Measurement Gates

The primary risk is roughly 1,000 direct indexed draws per mesh per relevant pass, especially live WebGPU command encoding. Secondary risks are shader-variant growth, vertex-attribute pressure, visibility churn, layout replacement invalidating compact identity, and deferred combined face identity.

Phase 0 must prove a hard-coded internal fixture before public API promotion:

1. contiguous indexed ranges and shared vertices render correctly on Vulkan and WebGPU;
2. `first_instance` selects the intended state and identity on both backends;
3. no geometry is duplicated;
4. transformed picking and render products agree;
5. command-recording overhead at approximately 1,000 parts is quantified.

Measure synthetic partitioned meshes, a shared-boundary-vertex fixture, and a representative atlas across part counts `1`, `32`, `128`, `1,000`, and `4,096`, with triangle counts spanning approximately `100k`, `1M`, and `5M`.

Scenarios should cover static rendering, all-part explode updates, two-record hover changes, 10% visibility churn, WBOIT opacity animation, and repeated picking during explode. Record Python preparation and call count, C validation/copy time, dirty and upload bytes, FramePlan/DRP2 encoding, emitted draw count, backend command-recording time, GPU and total frame time, bounds time, query latency, and CPU/GPU memory.

Compare the ordinary one-draw mesh, direct multipart draws, a limited independent-visual case, and only then optional duplicated-geometry or Vulkan-indirect prototypes. If command generation or encoding exceeds roughly 25% of a 16.7 ms frame budget at about 1,000 parts in a deliberately GPU-light synthetic case, prototype a faster lowering behind the same public API. This is an optimization trigger, not an API guarantee.

## Validation Plan

### API And Retained State

Test valid partitions; clearing; unsorted, overlapping, gapped, unaligned, out-of-bounds, and empty ranges; invalid update ranges; invalid opacity; nonfinite transforms; and rejected multipart-plus-instancing. Verify defaults, full and partial updates, dirty-range coalescing, one/two-record interaction changes, and no geometry dirtiness from state-only updates.

### Lowering And Shaders

For `P` parts, verify one FramePlan visual, one logical packet, one subdraw per visible part, exact source index ranges, stable part indices in `first_instance`, complete resource-range validation, and no per-part pipeline or bind-group changes.

Compile and validate required GLSL and WGSL variants for unlit and lit mesh, item state, picking, ordinary depth, supported alpha modes, depth peeling, and surface capture.

### Rendering, Picking, And Bounds

Use a deterministic three-part mesh with shared vertices, distinct transforms and tints, one hidden part, one partially transparent part, and selection state. Validate assembled and exploded rendering, selection, WBOIT, surface products, and camera fit.

Verify part identity before and after displacement, absence of hits at the original displaced location, no hidden-part hit, preserved link keys, and highlights following transformed geometry.

Verify bounds for identity, translation, rotation, nonuniform scale, hidden-part exclusion, visual-transform composition, and all-parts-hidden behavior.

### Python And Regression

Verify dtype conversion, contiguity and shape checks, count inference, one raw C invocation for a 1,000-part update, and no Python loop over parts.

Keep existing ordinary mesh, indexed mesh, instancing, instance selection, face picking, bounds, WBOIT, GTAO/surface-product, and WebGPU behavior unchanged. Multipart mode must be opt-in and impose no work on ordinary meshes.

## Acceptance Criteria

The minimal capability is complete when:

1. One indexed mesh supports at least 1,000 parts without creating 1,000 visuals or duplicating geometry.
2. Parts may share vertices and independently support transform, tint, opacity, visibility, item state, and ITEM identity.
3. Explode and interaction animations upload no vertex or index data; a complete 1,000-part state update is approximately 100 KiB or less.
4. Picking, depth, transparency, and surface products follow transformed visible geometry, while hidden parts neither render nor hit.
5. Link keys provide stable application identity and `dvz_visual_bounds()` encloses all transformed visible parts.
6. The C and Python APIs are batched, with no public object or FFI call per part.
7. One scene visual and fixed pipeline/resource bindings serve all part subdraws through common draw-packet lowering.
8. Vulkan and WebGPU expose the same public behavior, while ordinary meshes retain their current fast path.
9. Invalid ranges, multipart-plus-instancing, and unsupported multipart FACE queries produce explicit diagnostics rather than silent degradation.

## Promotion Path

After Phase 0 evidence, promote semantics into the canonical mesh, item-state, picking, bounds/transform, transparency, FramePlan, and public API specifications, then create an implementation-ready slice. Keep this proposal as a short rationale and unresolved-performance record once those rules become normative.

Later work may add an atlas consumer outside the renderer core, textured support, multipart FACE identity, CPU part-frustum culling, indirect or cached command lowering, storage-buffer state, or a generic grouped-data abstraction when measurements and additional use cases justify them.
