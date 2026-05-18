> **Execution Status**
> - **Status:** `FUTURE RESOURCE-MANAGEMENT PROPOSAL`
> - **Updated on:** `2026-05-18`
> - **Purpose:** preserve exploratory direction for page/chunk residency, partial-valid rendering,
>   and progressive upload behavior for very large datasets.
> - **Primary gap:** Datoviz has dirty updates and LOD examples, but no explicit page/chunk
>   residency model for very large datasets.

# Out-Of-Core And Progressive Rendering Design


## Summary

Many scientific datasets are too large to keep fully resident on the GPU. Datoviz should eventually
support progressive, page-resident scene resources while keeping the public visual semantics stable.

This proposal is about runtime-facing primitives, not file formats. Python or application layers can
own zarr, OME-Zarr, VTK, HDF5, cloud stores, and dataset-specific scheduling. Datoviz C should own
the retained resource identity, resident GPU payloads, partial uploads, telemetry, and rendering
behavior when only part of a resource is available.

The first out-of-core implementations may be entirely Python/GSP-level. Datoviz C should grow
residency primitives only when repeated examples need stable logical identity, GPU memory accounting,
partial-valid rendering, upload telemetry, or backend-portable chunk upload paths. The C engine does
not need to know about data that is merely on disk, in a cloud store, or in a Python cache. It only
needs to know about logical scene resources and the subset of pages currently resident, missing, or
being updated in GPU-visible form.


## Simple Examples

- billion-point point cloud;
- tiled microscopy or whole-slide image;
- chunked 3D volume;
- sparse voxel/AMR field;
- large image embedding with thumbnails;
- massive tractography dataset;
- large unstructured simulation mesh partitioned into blocks;
- time-varying field with cached visible frames.


## Core Concepts

The preferred responsibility split is:

```text
Python / application:
  file formats, chunk priority, async loading, cache, decompression, data conversion

Datoviz C / runtime:
  stable scene resource identity, resident GPU slots, uploads, telemetry, partial-valid rendering
```

This keeps high-level data policy flexible while preventing GPU residency and upload details from
being reinvented in every large-data example.


### Logical Resource

The user-facing object remains stable:

```text
large point cloud
chunked volume
tiled image
partitioned mesh
track collection
```

### Page Or Chunk

The resource is divided into independently loadable units:

```text
page id
lod level
bounding box or screen tile
byte range or external key
resident state
dirty state
last used frame
priority
```

### Residency

Residency tracks which chunks have GPU payloads now.

```text
missing -> requested -> loading -> resident -> evictable -> evicted
```

The renderer must be able to draw a partial-valid scene: coarse LOD, stale data, or missing chunks
should be explicit states, not accidental failures.

For an early prototype, Python may manage residency by creating or updating ordinary visuals for the
currently visible chunks. That is acceptable and likely preferable for first experiments. C-side
residency becomes useful when that approach causes resource churn, duplicated slot-management code,
unclear picking behavior, or backend-specific upload details leaking into applications.


## Candidate C Primitives

Useful Datoviz-side primitives:

- page/chunk registry;
- resident GPU resource table;
- partial buffer and texture uploads;
- texture-region and buffer-range dirty tracking;
- memory budget and eviction policy hooks;
- upload telemetry;
- progressive frame status;
- stable semantic ids across LOD/page replacement.

These should build on the existing resource update and invalidation model, not replace it.

These primitives should be optional and narrow. They should not imply that C owns the global dataset
or loading policy. A minimal runtime object could track:

```text
logical resource id
resident page/chunk ids
GPU slot, buffer range, texture layer, or brick-pool mapping
dirty/upload state
approximate GPU byte size
validity or missing-data state
```

Python can then provide explicit commands such as:

```text
upload chunk (lod=2, i=12, j=5) into logical resource R
evict chunk K from logical resource R
mark chunk K dirty
query resident/missing state for diagnostics
```


## Python/Application Responsibilities

Keep high-level policy out of C initially:

- file format readers;
- cloud/object-store access;
- dask/zarr/xarray orchestration;
- chunk priority policy;
- cache directory management;
- preprocessing and compression;
- dataset-specific metadata.

The application can tell Datoviz which chunks are now available and which resident chunks should be
updated or evicted.

Python may also remain the only out-of-core layer indefinitely for applications that do not need
shared runtime residency semantics. The main reason to promote a primitive into C is repeated need
across examples or bindings, not the mere existence of large data.


## Rendering Behavior

Progressive resources should support:

- coarse placeholder while fine data loads;
- missing-chunk styling or debug overlay;
- per-frame upload budget;
- graceful refinement without recreating visuals;
- stable picking when possible;
- explicit "not resident" pick/probe result when data is unavailable;
- no unbounded transient resource creation.

The distinction between "not resident" and "empty/missing value" matters. If a user probes a region
whose data page is not resident, the result should be different from a valid resident page whose
sample value is zero, transparent, masked, or empty. This is one of the few semantic facts the
runtime may need to expose even though it does not know how to load the missing page.


## Data Models

### Tiled Images

```text
tile_id
lod_level
x, y, width, height
texture layer or atlas slot
valid rectangle
```

### Chunked Volumes

```text
brick_id
lod_level
ijk origin
brick dimensions
value range/minmax
resident 3D texture region or brick pool slot
```

### Point Clouds

```text
chunk_id
lod_level
bounds
point count
attribute ranges
resident vertex buffer range
```

### Unstructured Grids

```text
partition_id
bounds
point/cell ranges
boundary/cut derived resources
field residency
```


## Compute Opportunities

- visibility and LOD selection;
- point/voxel compaction;
- min/max and occupancy summaries;
- page-level reductions;
- GPU-generated indirect draw arguments;
- on-GPU decompression or unpacking where portable;
- derived coarse representations.

These require the same compute-to-render dataflow discussed in
[`SPLATTING_FRAME_PLAN_REQUIREMENTS.md`](../future/SPLATTING_FRAME_PLAN_REQUIREMENTS.md).


## Telemetry

Out-of-core examples should report:

- resident chunks/pages;
- requested/loading chunks;
- uploaded bytes this frame;
- evicted bytes;
- GPU memory budget and usage estimate;
- visible LOD distribution;
- missing visible chunks;
- time to first coarse image and time to refined image.


## Example Plans

Useful future examples:

- progressive out-of-core point cloud;
- chunked volume streaming viewer;
- tiled whole-slide image viewer;
- sparse voxel/AMR field viewer;
- partitioned unstructured-grid viewer.


## Open Questions

- Should page/chunk residency be represented in scene `FramePlan` nodes or app-layer state?
- How much memory-budget policy should Datoviz C own?
- How should progressive picking behave when the best-resolution page is missing?
- Should resource ids remain stable across LOD replacement, or should LOD be a separate semantic id?
- What is the minimum C API that helps Python/GSP users without turning Datoviz into a data server?
- Which examples demonstrate enough duplicated Python-side residency logic to justify moving a
  primitive into C?
