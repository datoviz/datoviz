> **Execution Status**
> - **Status:** `STAGED ROADMAP / OPTIONAL RC4 FOUNDATIONS AND v0.5+ FEATURES`
> - **Updated on:** `2026-08-04`
> - **Purpose:** define reusable sampling, logical-view, incremental-update, streaming, and structured-surface capabilities motivated by rolling sampled data without introducing a waterfall-specific resource or runtime path.
> - **Release boundary:** RC3 remains feature-frozen; sampler-addressing and multi-region-update foundations may land as non-blocking RC4 work before the API freeze; logical field views, asynchronous streaming extensions, and GPU-displaced structured surfaces remain post-v0.4 work.

# Sampled-Field Views And Incremental Updates

## Summary

Rolling spectrograms, periodic simulations, scrolling acquisition images, texture atlases, tiled domains, and structured height fields share the same underlying requirements. Datoviz should address those requirements through orthogonal sampled-field capabilities rather than a public ring-buffer or waterfall abstraction.

The target composition is:

```text
SampledField storage
    + binding-local sampling policy
    + binding-local logical field view
    + sparse incremental update regions
    + optional asynchronous producer handoff
```

A rolling waterfall is one pressure test: new FFT rows update physical field storage while the bound visual advances a logical origin. The same resource may be bound through another visual with a different origin, window, sampler, or presentation.

## Decisions

1. Do not add `DvzRingBuffer`, `DvzRingTexture`, `DvzWaterfall`, or another use-case-specific public resource.
2. Keep sampled-field storage ordinary and stable; circularity belongs to logical-to-physical addressing, not resource identity.
3. Keep sampling policy separate from logical field-view geometry.
4. Make logical field views binding-local so several visuals may present different windows over one field.
5. Track sparse field dirtiness as a bounded region set rather than one bounding box.
6. Preserve the existing scene -> FramePlan -> DRP2 -> vklite runtime path.
7. Treat a structured surface as mesh convenience or a mesh shader variant, not a separate visual family.
8. Prefer a GPU field-displaced structured-grid mesh for rapidly changing height fields after the v0.4 final release.
9. Keep producer synchronization and ownership explicit; background producers must cross the documented render-thread handoff rather than mutate scene state directly.

## Pressure Tests

The design should serve at least these cases:

- a rolling amplitude-frequency-time waterfall;
- an oscilloscope, DAQ, or telemetry history;
- a camera or decoder updating image strips or tiles;
- a periodic PDE, fluid, wave, or cellular simulation domain;
- a texture or glyph atlas receiving disjoint patches;
- repeated or mirrored material textures;
- tiled maps and logical crops over larger resident fields;
- two panels viewing different windows or origins of one field;
- a terrain, digital elevation model, or structured simulation surface driven by a scalar field;
- a 3D volume receiving sparse subvolume updates.

## Current Baseline And Gaps

The v0.4 scene already owns regular 2D and 3D `DvzSampledField` resources, supports full and regional CPU updates, binds fields to image, labels, mesh-texture, and volume consumers, and emits regional texture uploads through FramePlan and DRP2.

The remaining gaps relevant to this roadmap are:

1. `DvzFieldSamplingDesc` defines repeat and mirror-repeat address modes, but the scene accepts clamp-to-edge only.
2. DRP2 documents sampler address modes, while the concrete in-process implementation currently carries filtering and hardcodes clamp-to-edge in the Vulkan backend.
3. sampled-field dirty state retains one rectangular region, so disjoint updates may inflate into a much larger upload;
4. regional upload preparation may repack data that already has a usable row pitch;
5. changing texture coordinates can approximate a moving view for rendering, but it does not define a shared logical mapping for probing, picking, export, or multiple consumers;
6. the current textured-mesh slice samples RGBA data for fragment color and does not displace structured geometry from a scalar field.

## Milestone 1: RC3 Specification Only

RC3 does not take new runtime implementation from this roadmap. Its only permitted work is to preserve this decision, keep public limitations honest, and define fixtures or measurements that future work can use.

RC3 must not gain a new resource class, shader variant, public streaming API, or alternate renderer path for this use case.

## Milestone 2: Optional RC4 Sampler Completion

The first non-blocking RC4 foundation completes the existing sampler policy across scene, FramePlan metadata, DRP2, vklite/Vulkan, serialization, recording/replay, validation, and WebGPU.

Required behavior:

1. support clamp-to-edge, repeat, and mirror-repeat on every applicable axis;
2. retain clamp-to-edge as the default;
3. propagate the complete `DvzFieldSamplingDesc` instead of reducing it to a nearest/linear boolean;
4. key persistent sampler reuse by every behaviorally relevant descriptor field;
5. allow different bindings of one field to use different sampling policies;
6. preserve native and WebGPU parity or report an explicit capability failure;
7. keep mipmap and mixed-filter expansion separate unless independently specified and validated.

This slice is useful for repeated textures, tiled fields, periodic domains, and the RC4 textured-mesh course even without logical field views.

Acceptance evidence:

- image and textured-mesh fixtures sample coordinates outside `[0, 1]` under each supported address mode;
- DRP2 JSON and recording round trips preserve sampler policy;
- Vulkan validation remains clean;
- native and WebGPU reference results agree within their established image-comparison policy;
- generated bindings and public reference documentation match the implemented descriptor behavior.

## Milestone 3: Optional RC4 Sparse Region Tracking

The second non-blocking RC4 foundation replaces the single sampled-field dirty rectangle with a bounded reusable region-set abstraction.

The region set should:

1. represent 1D ranges, 2D rectangles, and 3D boxes through one internal policy where practical;
2. preserve disjoint regions when separate transfers are cheaper than their bounding box;
3. merge overlapping and adjacent regions;
4. use a documented cost heuristic that includes command count and transferred bytes;
5. cap retained fragmentation and fall back to a full-resource update when the cap or cost threshold is exceeded;
6. preserve all pending regions until every relevant visual consumer has emitted them;
7. force a correct full upload after runtime reset or resource recreation.

The existing `dvz_sampled_field_update_region()` remains sufficient as the first public entry point. A batch API should be added only when atomicity, binding overhead, or measured producer behavior demonstrates a need.

Regional upload preparation should also avoid tightly packed scratch copies when the retained source pointer and row or image pitch can be passed safely to DRP2. Copy elimination must not weaken stream payload lifetime or replay behavior.

Acceptance evidence:

- `K` full-width row changes transfer approximately `K * width * texel_size` bytes rather than a full field;
- changes spanning the physical end and beginning of one axis remain two small uploads;
- arbitrary disjoint image patches and 3D subvolumes remain correct;
- diagnostics expose emitted regions, transferred bytes, merges, and full-upload fallbacks;
- a bounded fragmentation stress test proves predictable memory and command growth.

## Milestone 4: RC4 Demonstration And v0.4 Final Freeze

If both optional foundations land before the RC4 API freeze, one deterministic scrolling-field example may demonstrate a scalar field, regional row writes, repeat addressing, and a moving texture rectangle. The example must use generic sampled-field APIs and must not introduce waterfall-specific public symbols.

The v0.4 final gate adds no new semantics from this roadmap. It validates, documents, profiles, and fixes the RC4 implementation only. Logical field views, new streaming APIs, zero-copy tiers, and GPU-displaced surfaces must not begin during the final gate.

## Milestone 5: Early v0.5 Binding-Local Field Views

The first post-v0.4 public feature is a binding-local logical view over physical sampled-field storage.

The conceptual mapping is:

```text
physical_sample = address(logical_sample + origin, storage_extent, sampling_policy)
```

The eventual descriptor should represent integer sample origins and logical extents on applicable axes. Normalized offset and scale may be derived for filtered shader sampling, but integer sample coordinates remain the semantic authority so periodic shifts are exact and stable for large histories.

Required semantics:

1. the view belongs to one visual field slot, not to the shared `DvzSampledField`;
2. changing the view updates a small parameter block and does not rewrite field data, geometry, or FramePlan topology;
3. two consumers may bind one field with different views;
4. rendering, probing, picking, query coordinates, export, and field-geometry conversion use the same logical mapping;
5. view origin and extent remain distinct from `DvzFieldGeometry`, which describes physical meaning such as origin, spacing, units, axis order, and flips;
6. sampling address policy remains distinct from the view descriptor;
7. empty or partially initialized histories have an explicit valid logical extent or missing-data policy rather than exposing unwritten samples as valid history.

An illustrative API shape may use a `DvzFieldViewDesc` passed to a field slot, but names and ABI remain unsettled until image rendering, probing, labels, volume slices, and WebGPU have been pressure-tested together.

## Milestone 6: Early v0.5 Streaming Handoff

High-rate producers need a general background-to-render-thread handoff for field regions and other resource updates. The implementation should follow the scene threading contract and should not grant background threads direct scene, DRP2, or Vulkan mutation.

Required policy:

1. make payload ownership and lifetime explicit;
2. provide bounded back-pressure rather than unbounded allocation;
3. preserve region identity while coalescing pending updates;
4. distinguish deliver-every-update acquisition from latest-ready visualization;
5. provide completion state sufficient for safe producer-buffer reuse;
6. drain updates at a documented frame boundary before invalidation and emission;
7. retain deterministic single-threaded behavior for ordinary callers.

Later measured tiers may add borrowed upload memory, mapped staging allocations, external GPU buffers, regional buffer-to-texture invalidation, and explicit timeline-semaphore synchronization. These are interoperability features, not prerequisites for the ordinary CPU regional path.

## Milestone 7: v0.5+ GPU Field-Displaced Structured Mesh

A structured surface remains a `mesh` convenience or mesh shader variant. It does not create a parallel surface visual family.

The preferred high-rate path keeps regular grid topology and planar coordinates static while a vertex shader samples a scalar `DvzSampledField` to obtain displacement. The same binding-local field view used by an image consumer supplies periodic origin and logical extent to the mesh consumer.

Target behavior:

1. one scalar field may drive both a 2D image and a 3D structured mesh without duplicate source uploads;
2. the vertex stage obtains height from the field while static grid vertices and indices remain resident;
3. normals derive from neighboring samples with explicit edge and periodic-boundary behavior;
4. scalar color mapping may reuse the height field or bind a separate scalar/color source;
5. material, depth, transparency, AO, clipping, and render-product participation remain ordinary mesh behavior;
6. bounds, framing, picking, probing, and export define whether they use retained metadata, GPU-derived reductions, readback, or conservative bounds;
7. capability adaptation provides an explicit CPU-derived mesh fallback when the selected GPU path is unavailable or unsuitable.

This path should serve terrains, digital elevation models, wave and PDE solutions, regularly sampled simulation surfaces, and amplitude-frequency-time plots. It must not encode waterfall-specific cursor semantics in the mesh pipeline.

## Promotion Gates

Sampler completion may promote into the normative sampled-field, DRP2, and backend specifications only after native and WebGPU behavior exists. Sparse region tracking may promote into resource invalidation and FramePlan specifications after diagnostics and cost behavior are validated. Binding-local views require joint rendering and query semantics before public API promotion. GPU field displacement requires mesh, bounds, picking, material, and capability-fallback specifications before implementation becomes normative.

## Related Specifications

- [`../../pipeline/RESOURCE_MODEL.md`](../../pipeline/RESOURCE_MODEL.md): sampled-field ownership and resource identity.
- [`../../pipeline/INVALIDATION_AND_CACHING.md`](../../pipeline/INVALIDATION_AND_CACHING.md): incremental update scopes and upload consequences.
- [`../../visuals/IMAGE.md`](../../visuals/IMAGE.md): 2D sampled-field presentation and probing.
- [`../../visuals/MESH.md`](../../visuals/MESH.md): structured surfaces remain mesh convenience.
- [`../../integration/THREAD_SAFETY.md`](../../integration/THREAD_SAFETY.md): background producer and render-thread ownership boundary.
- [`FIELD_VISUALIZATION_ROADMAP.md`](FIELD_VISUALIZATION_ROADMAP.md): broader regular, vector, tensor, categorical, sparse, and bricked field directions.
- [`INTEROPERABILITY_ARCHITECTURE.md`](INTEROPERABILITY_ARCHITECTURE.md): future external-data and synchronization tiers.
