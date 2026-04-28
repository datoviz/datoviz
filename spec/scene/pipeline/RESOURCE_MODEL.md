# Scene Resource Model

This document defines the producer-side resource model for the future scene layer.

It specifies how scene-owned data should be represented before planning and DRP2 emission.

It does not define backend allocation policy or the final public C API.


## Purpose

The scene resource model exists to:

1. give visuals stable logical data dependencies,
2. make dirty tracking and subrange uploads explicit,
3. support incremental `FramePlan` construction,
4. keep CPU-owned scene data separate from backend-owned execution objects.


## Position

Scene resources sit between:

1. user-facing scene and visual state,
2. `FramePlan` planning,
3. DRP2 resource creation, write, copy, and readback commands.

They are scene-owned logical objects that may materialize as DRP2 buffers, textures, or readback
targets, but they are not themselves backend objects.


## Core Rule

A scene resource describes logical data ownership, usage, and dirtiness.

A scene resource does not expose:

1. Vulkan, Metal, WebGPU, or swapchain handles,
2. backend allocation objects,
3. backend synchronization primitives,
4. command-buffer recording internals.


## Requirements

The first resource model must support:

1. stable logical identity,
2. CPU ownership by default,
3. explicit usage roles,
4. dirty tracking at whole-resource and subrange granularity,
5. sharing across multiple visuals and panels,
6. deterministic upload, copy, and readback planning,
7. derived transient resources created during planning when needed.


## Authority Rule

Scene-authored resources should remain authoritative by default.

The first scene slice should distinguish clearly between:

1. CPU-authored source resources that are authoritative across frames,
2. CPU-cached derived resources that may be reused across frames,
3. frame-local transient derived resources that exist only to serve one `FramePlan`,
4. GPU-produced results that do not become authoritative scene state unless explicitly promoted.

The important consequence is:

1. compute output is frame-local by default,
2. persistence across frames must be explicit,
3. GPU-produced data must not silently replace scene-owned authored state.


## Why This Needs To Be Explicit

The local `v0.3/` scene and visual code already implies several distinct data shapes and binding
patterns:

1. structured per-item geometry payloads for markers, meshes, segments, arrows, and some sphere
   representations,
2. grouped sequences for paths and text,
3. sampled textures for images and texture-backed meshes,
4. 3D sampled textures for volume rendering,
5. per-visual parameter blocks distinct from per-item payloads,
6. explicit texture/sampler slots distinct from buffer-like data streams.

At the `DvzVisual` level in `v0.3`, this appears as a recurring split between:

1. attribute-like data streams,
2. optional index data,
3. parameter blocks,
4. texture bindings,
5. optional grouping metadata.

The future scene layer should preserve those capabilities without baking them prematurely into
backend-shaped visual internals.


## v0.3 Vocabulary Mapping

The scene spec should use terminology that is recognizable from the local `v0.3` visual stack.

At the broadest level:

1. `Visual` remains the right top-level term,
2. `Resource` is the right term for scene-owned data consumed by visuals,
3. common v0.3 concepts such as attribute streams, indices, params, textures, and groups are useful
   conceptual inputs for v0.4 planning.

The scene spec should not copy the `v0.3` API mechanically, but it should stay close enough that the
relationship is obvious.


## Resource Identity

Every scene resource needs a stable logical identity within the owning scene.

That identity should support:

1. visual declarations in `VISUAL_CONTRACT.md`,
2. dependency tracking in `FRAME_PLAN.md`,
3. scene diagnostics,
4. picking and readback result routing when applicable,
5. incremental updates across frames.

Resource identity is scene-level and should remain independent from any DRP2 object id assignment.


## Resource Ownership Classes

The first scene slice should distinguish these ownership classes:

1. `SceneShared`: reusable across multiple visuals or panels,
2. `VisualOwned`: conceptually owned by one visual but still represented as an explicit scene
   resource,
3. `PanelLocal`: scoped to one panel or target configuration,
4. `TransientDerived`: created during planning for intermediate stages,
5. `ReadbackSink`: producer-visible destination for execution results.

Even `VisualOwned` resources should still be visible to planning and diagnostics as first-class
scene resources.

`TransientDerived` should be the default class for compute-produced or planning-produced intermediate
results unless a spec-level contract explicitly declares a reusable persistent cache.


## Resource Kinds

The first scene resource model should support at least these kinds:

1. `BufferResource`
2. `Texture2DResource`
3. `Texture3DResource`
4. `SamplerLikeResource`
5. `ParameterBlockResource`
6. `ReadbackResource`
7. `DerivedResource`

These are logical scene categories.
They should not be read as a final DRP2 object taxonomy.


## Scene-Facing Resource Classes

In addition to abstract kinds, the scene spec should use resource classes that match how visual
families actually think about data.

The first useful scene-facing classes are:

1. `ItemTable`
2. `GroupedItemTable`
3. `IndexedGeometry`
4. `SampledField`
5. `StyleBlock`
6. `DerivedField`
7. `ReadbackTarget`

These classes are not replacements for lower-level kinds.
They are the scene-level vocabulary that should sit above them.


## `ItemTable`

`ItemTable` is the default class for flat per-item records.

It should cover families such as:

1. `pixel`
2. `point`
3. `marker`
4. `segment`

Typical item fields may include position, size, color, angle, width, endpoint data, and similar
per-item properties.

Conceptually:

1. each row is one logical item,
2. items are independent by default,
3. updating one item does not require understanding neighboring rows.

This is the natural class for mark-like or instance-like families where batching is still possible but
group semantics are not intrinsic to the family contract.


## `GroupedItemTable`

`GroupedItemTable` is the scene-level class for grouped sequence or layout-driven records.

It should cover families such as:

1. `path`
2. `glyph`
3. path-derived specializations such as wiggle-like modes

It combines:

1. underlying item storage,
2. span boundaries — contiguous ranges of items that form one logical sub-object,
3. optional per-span metadata,
4. span-aware dirtiness.

**Terminology note**: the contiguous boundary units stored here are called **spans** in the public
API to distinguish them from attribute-level groups (`PER_GROUP` source).
A span is one polyline in `path`, one string in `glyph`.
A group is a population-level attribute bucket (neuron population, brain region) used with the
`PER_GROUP` attribute source.
The two concepts are orthogonal and must not be conflated.

Conceptually:

1. the underlying storage may still be one flat batch-friendly array,
2. but the scene model also records how rows are partitioned into higher-level logical spans,
3. each span is an independent semantic object even if many spans are rendered in one GPU batch.

Examples:

1. many independent paths rendered together,
2. many text labels rendered together,
3. many wiggle traces rendered together.

This class is important because performance and semantics pull in different directions:

1. GPU efficiency often prefers one shared batched buffer,
2. scene semantics still need to know which rows belong to which logical object.

`GroupedItemTable` exists to preserve both properties at once.

Without it, the scene layer would be forced into one of two bad choices:

1. split every logical path or label into separate resources and lose batching opportunities,
2. collapse everything into one anonymous flat table and lose semantic group identity.

The grouped-table concept is therefore not only acceptable conceptually, it is one of the key scene
abstractions for families such as `path` and `glyph`.


## `IndexedGeometry`

`IndexedGeometry` is the class for geometry defined by vertices plus optional indices and related
geometry-side payloads.

It should primarily cover:

1. `mesh`
2. `sphere` when represented as actual geometry rather than an impostor-like mode

This class may include:

1. vertex records,
2. optional index records,
3. optional auxiliary geometry channels such as normals or contour metadata.


### Small-N Versus Large-N Mesh Collections

The scene spec should distinguish clearly between:

1. semantic object granularity,
2. visual object granularity,
3. batched geometry storage.

These are related, but they should not be forced to be identical.

For scientific mesh collections such as anatomical regions, cortical parcels, or segmented
assemblies, the preferred guidance is:

1. when the number of independently manipulated meshes is small, using one visual per mesh is often
   the clearest scene representation,
2. when the number of meshes becomes large, one grouped mesh visual or a small number of grouped
   mesh visuals is usually the better realization,
3. in both cases, the scene should preserve stable semantic identity for each logical region or
   submesh.

The practical rule is therefore:

1. small-N favors simplicity of one visual per logical mesh,
2. large-N favors grouped geometry plus per-region style or state tables,
3. semantic identity such as `region_id` must survive either realization strategy.

Grouped mesh realizations should therefore be allowed to combine:

1. one shared `IndexedGeometry` payload or a small number of shared geometry payloads,
2. one partition map from geometry ranges to semantic region identity,
3. one per-region style or state resource for visibility, opacity, selection, and related controls.

This avoids two bad outcomes:

1. forcing a large scene to create one visual per small mesh even when batching is clearly better,
2. forcing a batched realization to collapse into anonymous geometry with no stable semantic
   identity.

The first scene slice does not need to freeze an exact public API for grouped meshes yet.

But it should already preserve this stronger rule:

1. one logical region may be represented either by one visual or by one partition inside a grouped
   mesh visual,
2. picking, selection, visibility, and style updates must still resolve through stable scene
   identity rather than draw-call shape.


## User-Facing Resource Constructors

The only user-facing resource constructors in the v0.4 API are `dvz_texture_2d` and
`dvz_texture_3d`, which create `SampledField` resources. All other resource classes
(`ItemTable`, `GroupedItemTable`, `IndexedGeometry`, `DerivedField`, etc.) are internal
resources managed by the scene through `dvz_visual_set_data` and `dvz_visual_alloc`. The user
never constructs them directly.

The scene provides two user-visible readback surfaces: `DvzPickResult` (picking, polled via
`dvz_scene_poll_pick_result` or the `DVZ_EVENT_PICK_RESULT` callback) and the `DvzSelection`
GPU mask buffer (selection). There is no generic user-facing readback resource constructor.


## `SampledField`

`SampledField` is the scene-level class for sampled scalar, vector, color, or volumetric fields.

It should cover:

1. `image`
2. `volume`
3. glyph atlas-like resources
4. image-family slice-like modes sourced from volumetric data when needed

The important semantic point is that this class describes sampled data content, not backend texture
dimensionality alone.


## `StyleBlock`

`StyleBlock` is the class for small structured parameter data associated with a visual or family.

It should cover:

1. family-wide style controls,
2. material controls,
3. contour and edge controls,
4. transfer-function controls,
5. mode and quality selectors.


## `DerivedField`

`DerivedField` is the class for planning- or scene-generated data that is not the original authored
input.

Examples:

1. panel-local picking targets,
2. intermediate composition targets,
3. compute-derived buffers,
4. scene-derived geometry or field representations.

`DerivedField` should be interpreted together with the ownership classes above:

1. most compute-derived fields should be `TransientDerived`,
2. reusable derived caches should be declared explicitly rather than inferred,
3. a derived field should only become authoritative across frames if the scene contract says so or if
   its contents are explicitly promoted into a CPU-owned scene resource.


## `ReadbackTarget`

`ReadbackTarget` is the scene-level class for producer-visible execution results.

It should cover:

1. picking results,
2. image export results,
3. optional compute-result captures for testing or tooling.


## Persistence Of Compute-Derived Results

When a compute stage writes a derived resource, the default policy should be:

1. the output is valid for the current scene-level `FramePlan` only,
2. later frames may regenerate it from the authoritative scene inputs,
3. reuse across frames is allowed only when the scene declares a persistent derived cache,
4. readback into a CPU-owned resource is the explicit path for promotion into long-lived scene state.

This keeps compute as an implementation and planning tool without making GPU-side intermediates the
implicit source of truth.


## Resource Facets

One practical lesson from the local `v0.3` visual stack is that a resource kind alone is not enough.

The first scene model should also recognize common resource facets that recur across many visuals:

1. attribute stream,
2. index stream,
3. parameter block,
4. sampled texture input,
5. grouping descriptor,
6. derived target or readback payload.

For example:

1. point and marker visuals are mostly attribute streams plus parameter blocks,
2. mesh visuals add indices and optional texture inputs,
3. path visuals add grouped sequence interpretation,
4. glyph visuals combine instance-like records with atlas sampling,
5. image and volume visuals rely heavily on sampled texture resources plus small parameter blocks.

This separation lets the future scene model stay close to the proven v0.3 visual shapes without
copying the old API one-to-one.

These facets are intentionally close to the `v0.3` mental model:

1. attribute stream is the scene-side successor to per-visual vertex attribute arrays,
2. index stream is the successor to optional indexed visuals,
3. parameter block is the successor to visual params slots,
4. sampled texture input is the successor to texture-plus-sampler visual bindings,
5. grouping descriptor is the successor to grouped path and glyph data.

The relation between classes and facets is:

1. a class describes the scene meaning of the resource,
2. a facet describes how a visual consumes or organizes it.


## BufferResource

`BufferResource` is the default resource kind for structured array-like scene data.

It should cover:

1. vertex-like payloads,
2. index-like payloads,
3. per-item structured records,
4. grouped sequence data,
5. storage-style compute input or output,
6. staging-compatible upload sources when visible at scene level.

Examples from the local `v0.3` visual families include:

1. marker records with position, color, size, shape, and angle,
2. mesh vertex and index data,
3. segment and arrow endpoint data,
4. path point sequences,
5. glyph instance records,
6. point and pixel records.


## Texture2DResource

`Texture2DResource` covers sampled or writable 2D image-like data.

It should support:

1. RGBA image data,
2. single-channel image data used with a colormap path,
3. atlas-like textures such as glyph atlases,
4. optional mesh-associated textures,
5. offscreen export or intermediate composition targets when they are scene-visible.


## Texture3DResource

`Texture3DResource` covers sampled volumetric data.

It should support:

1. volume rendering inputs,
2. volume slicing inputs,
3. capability-gated volume variants,
4. deterministic subvolume updates when the future scene API needs them.


## SamplerLikeResource

The scene layer may need a logical sampling descriptor even if the final runtime materialization is
still deferred.

This resource kind should cover:

1. filtering mode,
2. addressing mode,
3. compare mode when relevant later,
4. sampling policy shared across visuals.

Sampler resources are not first-class scene resources.
A texture/sampler binding slot is a field inside a `ParameterBlockResource`.
A sampler has no meaningful scene-level lifecycle independent of the parameter block that declares it.


## ParameterBlockResource

Some visual inputs are neither bulk geometry nor sampled images.

`ParameterBlockResource` covers small structured data blocks such as:

1. visual parameter structs,
2. material parameters,
3. transform blocks,
4. lighting parameters,
5. panel-derived common blocks,
6. compute dispatch parameter blocks when applicable.

Examples suggested by the local `v0.3` visuals include:

1. marker edge settings,
2. mesh lighting and shading mode parameters,
3. path linewidth and cap settings,
4. image colormap parameters,
5. volume transfer and permutation parameters,
6. glyph size and background parameters.


## ReadbackResource

`ReadbackResource` is the producer-visible destination for data retrieved after execution.

It should support:

1. single-pixel picking results,
2. offscreen image capture,
3. optional compute-result retrieval for tests or tooling.

The logical readback destination should be scene-visible and typed enough that post-frame processing
does not depend on backend mapping APIs.


## DerivedResource

Some resources should be produced by planning rather than authored directly by the user.

Examples:

1. transient picking targets,
2. transient composition targets,
3. intermediate buffers used by compute-assisted visuals,
4. visual-local derived geometry buffers created from higher-level scene data,
5. panel-local target attachments.

Derived resources are internal planning artifacts by default.
They are not scene-visible unless the user explicitly declares a named readback resource for them.
The common case — axis tick geometry, glyph quads, path tessellation — has no reason to be
user-visible; exposing it would leak implementation detail and create unnecessary API surface.
When the user explicitly requests readback of derived data (for testing or export), a named
`ReadbackResource` is the opt-in mechanism.

Derived resources should still be represented explicitly in planning and diagnostics.


## Content Shapes

In addition to kind, each resource should declare its logical content shape.

The first useful content-shape categories are:

1. scalar or small fixed struct,
2. flat array of fixed-size records,
3. grouped array with span boundaries,
4. dense 2D texel grid,
5. dense 3D texel grid,
6. opaque derived payload.

This distinction matters because a path-like grouped array should not be treated like a flat marker
array, and a volume texture should not be treated like a 2D image.


## Grouped Resources

Grouped resources are necessary for path-like, text-like, and other segmented data.

The resource model should support:

1. a flat storage region for the underlying items,
2. a span descriptor that identifies span starts and lengths,
3. optional per-span metadata,
4. subrange dirtiness at both item and span levels.

This avoids forcing each grouped visual to invent a private data encoding.

It also preserves the ability to batch multiple independent logical groups into one GPU-oriented data
upload and one render contribution where that is beneficial.


## Resource Roles

Each resource must declare one or more logical roles for the current frame.

The first role set should include:

1. geometry input,
2. index input,
3. sampled image input,
4. sampled volume input,
5. uniform or parameter input,
6. storage input,
7. storage output,
8. render target,
9. picking target,
10. readback destination,
11. transient intermediate.

Roles are frame-dependent and should be derivable into `FramePlan` read/write sets.


## Mutability Classes

Each resource should declare its expected mutability:

1. immutable asset,
2. infrequently updated parameter block,
3. per-frame dynamic stream,
4. transient per-plan derived resource,
5. readback-only sink.

This is scene planning information, not backend memory-placement policy.

The attribute-level mutability hints from `ATTRIBUTE_SOURCES.md` map onto these resource-level
classes as follows:

| `ATTRIBUTE_SOURCES.md` hint | `DVZ_MUTABILITY_*` enum | Resource-level class |
|---|---|---|
| `static` | `DVZ_MUTABILITY_STATIC` | immutable asset |
| `dynamic` | `DVZ_MUTABILITY_DYNAMIC` | infrequently updated parameter block |
| `streaming` | `DVZ_MUTABILITY_STREAMING` | per-frame dynamic stream |

These are the same three hint levels; the enum names above are the C API spellings used in
`dvz_visual_set_mutability`.


## Dirty Tracking

Dirty tracking must be explicit and structured.

The first resource model should support:

1. whole-resource dirty state,
2. byte or element subrange dirty state for flat buffers,
3. region dirty state for textures,
4. group-aware dirty state for grouped resources,
5. revision counters for deterministic planning and caching,
6. independent tracking of content changes versus metadata changes.

Examples:

1. updating only a subset of marker positions,
2. updating one path in a multi-path resource,
3. updating a single image tile or subrectangle,
4. updating a parameter block without touching geometry,
5. updating glyph instances without regenerating the atlas.

For grouped resources, the model should ideally support both:

1. item-level dirtiness within a group,
2. group-level dirtiness when a whole logical path, label, or trace changes.


## Logical Views

One resource may need multiple logical views at the scene level even if the final DRP2 object shape
is still under review.

Examples:

1. the same image data used both as a render input and export source,
2. a volume texture sampled by different stage variants,
3. a shared parameter block read by multiple visuals,
4. one grouped resource consumed by both render and picking variants.

The scene model should therefore separate:

1. underlying resource identity,
2. role-specific usage in a given visual or plan node.


## Sharing Rules

Resources may be shared:

1. across multiple visuals,
2. across multiple panels,
3. across multiple stages in one frame,
4. across multiple frames when persistent.

The sharing model should make this explicit so planning can avoid redundant uploads and emit correct
dependency structure.


## Relationship To Visuals

Visuals should reference resources declaratively.

A visual should be able to say:

1. which resources are mandatory,
2. which are optional for a higher-quality path,
3. which are read-only versus writable,
4. which resource roles apply in render, compute, picking, or export stages,
5. which changes to a resource invalidate only part of the visual plan.

This follows the local `v0.3` reality where different visuals used different mixtures of attribute
streams, optional indices, parameter blocks, textures, and grouped data.


## Relationship To Panels

Panels should not duplicate scene resources unnecessarily.

Instead, the model should allow:

1. shared source resources reused across panels,
2. panel-local derived resources such as picking targets or viewport-dependent parameter blocks,
3. panel-local visibility or transform interpretation without copying underlying data.


## Relationship To FramePlan

`FramePlan` should consume the resource model directly.

At minimum:

1. dirty resources generate `UploadNode` work,
2. writable resources determine `ComputeNode` and `RenderNode` dependencies,
3. target-like resources determine render and readback nodes,
4. transient derived resources are created at planning time,
5. readback resources determine post-frame interpretation paths.


## Relationship To DRP2

Scene resources map onto DRP2-visible resources only at emission time.

The translation should remain free to choose:

1. how logical ids are assigned,
2. whether one logical scene resource maps to one or multiple DRP2 objects,
3. when deferred object-creation details such as views or samplers are materialized,
4. how transient derived resources are represented in the active DRP2 surface.

The scene resource model should therefore remain richer than the currently frozen DRP2 command set
where needed, while still avoiding backend leakage.


## Diagnostics

The resource model should allow clear producer-side diagnostics such as:

1. missing required resource,
2. wrong resource kind for a visual variant,
3. incompatible content shape for a requested use,
4. invalid dirty range,
5. unsupported format or dimension,
6. illegal shared write pattern across stages,
7. readback requested from a resource without a readback-capable path.


## Minimum Resource Families To Support

The first resource model is acceptable only if it can cleanly represent:

1. marker-like structured arrays,
2. mesh-like vertex and index data with optional texture input,
3. path-like grouped sequences,
4. text-like grouped glyph instance data plus atlas sampling,
5. image-like 2D texture input,
6. volume-like 3D texture input,
7. panel-local picking and export targets,
8. shared parameter blocks reused across visuals or panels.


## Source Data Precision

The scene resource model accepts position data as **F64 (double-precision float)** natively.

This is intentional: the dominant source of position data in scientific Python workflows is
NumPy, where the default float type is `float64`.
Silently downcasting at ingestion time would introduce precision loss before normalization,
defeating the CPU precision policy described in `TRANSFORM_PIPELINE.md`.

The model therefore follows these rules:

1. `BufferResource` accepts F64 position arrays at write time.
2. The scene stores source position data in F64 internally and performs all normalization
   (Stage A of the transform pipeline) in F64.
3. F32 downcast happens at `UploadNode` time in `FramePlan`, after normalization to
   `VisualSpace`.
4. F32 source data is also accepted and is not promoted to F64.
5. Non-position attributes (color, size, scalar values) use their natural precision and
   are not required to be F64.

The public write API should therefore accept both `float32` and `float64` arrays for position
attributes, with the declared type stored alongside the data so the normalization stage knows
what arithmetic to use.


## Data Ownership And Memory Model

When the user writes data to a scene resource, the scene must decide whether to copy the buffer
or borrow the pointer.

Copying is safe but expensive for large data (volume textures, million-point arrays, brain atlas
meshes).
Borrowing is zero-copy but requires a lifetime contract between the user and the scene.

The preferred model ties ownership semantics to the mutability hint declared on the attribute or
resource (see `ATTRIBUTE_SOURCES.md`):

1. `static` — the user declares the buffer will remain valid for the lifetime of the scene.
   The scene borrows the pointer and does not copy.
   The user must not free or mutate the buffer while the scene is alive.
   This is the zero-copy path for large immutable data.

2. `dynamic` — the scene copies the data on write.
   The user retains ownership of the source buffer and may free it after the write call returns.
   This is the safe default for moderate-sized data that changes occasionally.

3. `streaming` — the scene owns a staging buffer and exposes a mapped write pointer.
   The user writes directly into the scene-managed staging memory each frame.
   No source buffer is required; no copy occurs after the initial staging allocation.

The default when no hint is given is `dynamic` (copy on write).

This model means large static resources (full volume textures, static atlas meshes) incur no
redundant copy if the user explicitly declares them `static`.
The scene trusts the declared lifetime.

These ownership semantics should be enforced at the API level: a `static` resource must not be
written to after initial upload without re-declaring it as `dynamic` or `streaming`.


## Allocation And Visual Sizing

Resource allocation should be separate from visual creation.

A visual is created without a committed item count:

```text
visual = dvz_point(scene, flags)   // no size yet
```

Data is written when available:

```text
dvz_visual_write(visual, DVZ_ATTR_POSITION, 0, n, xyz)  // establishes size on first write
```

An optional explicit pre-allocation hint is available for performance when the count is known
upfront:

```text
dvz_visual_alloc(visual, n)        // hints GPU buffer pre-allocation; not required
```

Resizing is allowed: writing with a different total count triggers reallocation.
This supports streaming workloads where item count changes frame to frame.

Tying item count to construction (as in v0.3) is not the preferred v0.4 model.
