# Scene Resource Model

Status: normative v0.4 scene pipeline spec.

This document defines producer-side scene resources: scene-owned logical data objects consumed by
visuals, planning, DRP2 emission, readback, and diagnostics. It does not define backend allocation
policy, backend handles, synchronization objects, or the final public C API.


## Purpose

Scene resources provide:

1. stable logical identity for visual dependencies;
2. explicit ownership, roles, dirty ranges, and revisions;
3. deterministic upload, copy, readback, and `FramePlan` construction;
4. a boundary between CPU-owned scene state and backend-owned execution objects.


## Core Rules

1. Scene-authored resources are authoritative by default.
2. GPU-produced data is frame-local unless a spec explicitly declares a persistent derived cache or
   the result is read back/promoted into CPU-owned scene state.
3. Resource identity is scene-level and independent from DRP2 object ids.
4. Visuals reference resources declaratively; resources do not expose Vulkan, Metal, WebGPU,
   swapchain, command-buffer, or synchronization internals.
5. One logical scene resource may lower to zero, one, or many DRP2 resources at emission time.


## Taxonomy

### Ownership Classes

| Class | Scope |
|---|---|
| `SceneShared` | reusable across visuals, panels, or frames |
| `VisualOwned` | owned conceptually by one visual but still visible to planning/diagnostics |
| `PanelLocal` | scoped to one panel or target configuration |
| `TransientDerived` | planning/runtime intermediate, usually one `FramePlan` only |
| `ReadbackSink` | producer-visible destination for execution results |

### Logical Kinds

| Kind | Covers |
|---|---|
| `BufferResource` | structured arrays, indices, item records, grouped sequences, storage data |
| `Texture2DResource` | 2D sampled/writable image-like data, atlases, visible intermediates |
| `Texture3DResource` | volumetric sampled data and subvolume updates |
| `SamplerLikeResource` | filtering/addressing policy stored inside a parameter block, not standalone |
| `ParameterBlockResource` | small structured style, material, mode, transform, lighting, or dispatch data |
| `ReadbackResource` | typed post-execution payloads for picking, export, tests, or tooling |
| `DerivedResource` | scene/planning-generated data, explicit in planning and diagnostics |

### Scene-Facing Classes

| Class | Meaning | Typical consumers |
|---|---|---|
| `ItemTable` | flat per-item records; each row is an independent logical item | `pixel`, `point`, `marker`, `segment` |
| `GroupedItemTable` | item storage plus structural spans | `path`, `glyph`, wiggle-like path variants |
| `IndexedGeometry` | vertices, optional indices, and geometry-side payloads | `mesh`, geometry-backed `sphere` |
| `SampledField` | authoritative regular sampled field | `image`, `volume`, glyph atlases, probes |
| `ParameterBlockResource` | visual/family parameter data after style/default resolution | all families |
| `DerivedField` | scene/planning-generated non-authoritative data | picking targets, composition, geometry |
| `ReadbackTarget` | producer-visible execution results | picking, image export, compute captures |

Future semantic resources such as graphs, unstructured grids, sparse/bricked fields, tracks,
ensembles, and molecular structures should lower through these classes before creating new resource
classes. See the future roadmap proposals under `../proposals/future/`.


## Required Class Semantics

### `ItemTable`

`ItemTable` is the default class for mark-like flat records. It supports row-level and contiguous
range updates; updates to one row do not require group/span interpretation.

### `GroupedItemTable`

`GroupedItemTable` stores flat items plus **spans**: contiguous structural ranges such as one path
or one string. Spans are distinct from semantic groups used by `PER_GROUP` attributes in
[ATTRIBUTE_SOURCES.md](ATTRIBUTE_SOURCES.md):

| Term | Meaning |
|---|---|
| span | structural range in one grouped resource (`span_sizes`) |
| group | semantic population/region/channel id (`group_id`) |

Grouped resources must support item-level dirtiness, span-level dirtiness, optional per-span
metadata, and batched rendering without losing stable span identity.

### `IndexedGeometry`

`IndexedGeometry` carries vertices, optional indices, and optional geometry channels such as normals
or contour metadata. Mesh collections may be represented either as one visual per small number of
semantic meshes or as grouped geometry for large collections. In both forms, semantic ids such as
`region_id` must survive for picking, selection, visibility, opacity, and style updates.

### `SampledField`

`SampledField` is the authoritative scene resource for regular sampled grids. It declares
dimension, resolution, sample format, channel/component type, semantic hints, and optional physical
metadata such as origin, spacing, axes, and units. Visuals borrow fields and layer interpretation on
top; runtime textures, CPU colorized caches, and derived slice resources are execution artifacts.

The primary user-facing constructor is `dvz_sampled_field`. Texture convenience calls such as
`dvz_texture_2d` and `dvz_texture_3d` are transitional wrappers that create sampled fields. See
[`../proposals/promoted/SAMPLED_FIELD_API_DESIGN.md`](../proposals/promoted/SAMPLED_FIELD_API_DESIGN.md).

### `DerivedField` And Compute Persistence

Derived fields are not authoritative unless explicitly declared persistent or promoted by readback.
Compute output is valid for the current `FramePlan` by default; reuse across frames requires a named
persistent derived cache.

### `ReadbackTarget`

The scene exposes readback through typed surfaces such as `DvzPickResult` and `DvzSelection`, not a
generic public readback resource constructor.


## Facets, Shapes, Roles

| Concept | Required values |
|---|---|
| facets | attribute stream, index stream, parameter block, sampled field input, grouping descriptor, derived/readback payload |
| content shapes | scalar/struct, flat records, grouped spans, dense 2D/3D grids, opaque derived payload |
| roles | geometry/index input, sampled input, parameter input, storage read/write, render/picking/readback target, transient intermediate |

Roles are frame-dependent and feed `FramePlan` read/write sets.


## Mutability And Ownership

Resource mutability is planning information, not backend memory-placement policy.

| Attribute hint | C enum | Resource-level class |
|---|---|---|
| `static` | `DVZ_MUTABILITY_STATIC` | immutable asset |
| `dynamic` | `DVZ_MUTABILITY_DYNAMIC` | infrequently updated parameter block or copied data |
| `streaming` | `DVZ_MUTABILITY_STREAMING` | per-frame dynamic stream |

The default write model is `dynamic`: scene copies data on write and the caller may free the source
buffer after the call. `static` does not imply pointer borrowing; borrowed/zero-copy data requires a
separate explicit lifetime contract. `streaming` uses scene-owned staging/mapped memory and should
not require a source-buffer copy each frame. A `static` resource must not be rewritten after initial
upload without redeclaring it as `dynamic` or `streaming`.


## Dirty Tracking

Resources must track:

1. whole-resource dirty state;
2. byte/element subranges for flat buffers;
3. regions for textures;
4. item and span dirtiness for grouped resources;
5. independent content versus metadata changes;
6. revision counters for deterministic planning and caching.

Invalidation scope names and cache policy live in
[INVALIDATION_AND_CACHING.md](INVALIDATION_AND_CACHING.md).


## Precision

Position data may be written as F64 and is stored/normalized in F64 when F64 source precision is
present. F32 source data is accepted without promotion. F32 downcast happens at `UploadNode` time
after Stage A normalization to `VisualSpace`; non-position attributes use their natural precision.
See [TRANSFORM_PIPELINE.md](TRANSFORM_PIPELINE.md).


## Allocation And Visual Sizing

Resource allocation is separate from visual creation. A visual may be created empty, receive data
later, and resize as counts change. Optional preallocation such as `dvz_visual_alloc(visual, n)` is a
performance hint, not a correctness requirement. Tying item count to visual construction is not the
preferred v0.4 model.


## Relationships

| Neighbor | Contract |
|---|---|
| visuals | declare resources, roles, variants, and invalidation impact; see `../semantics/VISUAL_CONTRACT.md` |
| panels | share source resources and create panel-local derived resources only when needed |
| `FramePlan` | consumes dirty resources, roles, transient derived resources, and readback targets |
| DRP2 | receives emitted backend-agnostic resources chosen by lowering; scene resources remain richer |
| diagnostics | report missing resources, wrong kind/shape, invalid ranges, unsupported formats, illegal shared writes, and unavailable readback |


## Minimum Coverage

The model is acceptable only if it cleanly represents marker-like structured arrays, mesh vertex and
index data with optional sampled fields, path/glyph grouped sequences, 2D image fields, 3D volume
fields, panel-local picking/export targets, shared parameter blocks, and persistent semantic
identity for grouped mesh regions.
