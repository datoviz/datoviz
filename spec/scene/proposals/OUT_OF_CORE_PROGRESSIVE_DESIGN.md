# Out-Of-Core And Progressive Rendering Design

> **Status:** exploratory future resource-management proposal.
> **Primary gap:** Datoviz has dirty updates and LOD examples, but no explicit page/chunk
> residency model for very large datasets.


## Summary

Many scientific datasets are too large to keep fully resident on the GPU. Datoviz should eventually
support progressive, page-resident scene resources while keeping the public visual semantics stable.

This proposal is about runtime-facing primitives, not file formats. Python or application layers can
own zarr, OME-Zarr, VTK, HDF5, cloud stores, and dataset-specific scheduling. Datoviz C should own
the retained resource identity, resident GPU payloads, partial uploads, telemetry, and rendering
behavior when only part of a resource is available.


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


## Rendering Behavior

Progressive resources should support:

- coarse placeholder while fine data loads;
- missing-chunk styling or debug overlay;
- per-frame upload budget;
- graceful refinement without recreating visuals;
- stable picking when possible;
- explicit "not resident" pick/probe result when data is unavailable;
- no unbounded transient resource creation.


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
[`SPLATTING_FRAME_PLAN_REQUIREMENTS.md`](SPLATTING_FRAME_PLAN_REQUIREMENTS.md).


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
