# Image Embedding LOD Explorer

> **Example status:** informative pressure test
> **Target:** C example plus preprocessing script
> **Data:** PD12M prepared cache with synthetic fallback fixture
> **Validation:** smoke, manual zoom/pick checklist, bundle validation

See [../POLICIES.md](../POLICIES.md) for cache/download rules and API caveats.


## Summary

Render up to 10,000 sampled PD12M images at 2-D embedding coordinates with progressive thumbnail
LOD. Zoomed-out views show mean-color rectangles; zoomed-in views switch through 2x2, 4x4, 8x8,
16x16, 32x32, and optional 64x64 thumbnails. Runtime chooses LOD, culls, batches, draws, and picks;
embedding reduction, image decoding/resizing, color conversion, packing, and optional PCA
reconstruction happen in preprocessing.


## User-Visible Result

- One panzoom embedding panel opens with all images visible.
- Items are screen-space sprites anchored in normalized embedding coordinates.
- Zoom progressively reveals thumbnail detail without recreating GPU textures or pipelines.
- Hover/click reports stable `image_id`, caption, source row/URL, license, and current LOD.
- Optional later variants add 3-D embedding billboards and a selected-image detail panel.

Suggested files:

```text
examples/c/image_embedding_lod_glfw.c
examples/c/prepare_image_embedding_lod.py
data/image_embedding_lod/
```


## Feature Pressure Points

- Batched image/rectangle visuals with texture-array or atlas pages.
- Retained sampled fields, texture resources, per-batch instance buffers, and stable item identity.
- Panzoom-driven screen-size LOD selection, culling, hysteresis, and repartitioning.
- Picking that survives batch/LOD changes.
- Resource reuse: no per-frame texture, sampler, bind layout, pipeline, or shader creation.
- Dashboard-scale scene pressure without introducing an out-of-core streaming system.

Relevant specs: [../../visuals/IMAGE.md](../../visuals/IMAGE.md),
[../../pipeline/RESOURCE_MODEL.md](../../pipeline/RESOURCE_MODEL.md),
[../../pipeline/INVALIDATION_AND_CACHING.md](../../pipeline/INVALIDATION_AND_CACHING.md),
[../../interaction/CONTROLLERS.md](../../interaction/CONTROLLERS.md),
[../../interaction/PICKING.md](../../interaction/PICKING.md), and
[../../semantics/VISUAL_CONTRACT.md](../../semantics/VISUAL_CONTRACT.md).


## Required Data And Resources

Default public source: [Spawning/PD12M](https://huggingface.co/datasets/Spawning/PD12M). The
prepared cache must record `dataset_id`, optional revision, sample seed, source row/shard id, image
URL, license, caption, and embedding model (`CLIP ViT-L/14` for provided vectors).

Initial scale:

```text
n_images <= 10000
runtime_embedding_dim = 2, optional 3
thumbnail_lods = 1x1, 2x2, 4x4, 8x8, 16x16, 32x32
optional_lods = 64x64
runtime_image_format = RGBA8 or BGRA8
```

Approximate RGBA8 memory for 10k square thumbnails: 1x1 = 40 KB, 2x2 = 160 KB, 4x4 = 640 KB,
8x8 = 2.5 MB, 16x16 = 10 MB, 32x32 = 40 MB, 64x64 = 160 MB. Default bundles should include up to
32x32; 64x64 is optional.


## Preprocessing And Bundle Format

The preprocessing script should generate render-ready arrays:

- deterministic PD12M sample manifest,
- RGBA8 thumbnails with EXIF orientation handled and aspect-preserving padding,
- mean colors,
- provided CLIP embeddings or deterministic development features,
- PCA/UMAP/t-SNE reduced coordinates with seed/provenance,
- texture-array pages or atlas pages per LOD,
- validated manifest and metadata rows.

First bundle layout:

```text
manifest.json
positions.f32
sizes.f32
colors.rgba8
items.u32
embeddings.f32              # optional after preprocessing
lod_1_page_000.rgba8
lod_2_page_000.rgba8
lod_4_page_000.rgba8
lod_8_page_000.rgba8
lod_16_page_000.rgba8
lod_32_page_000.rgba8
metadata.jsonl
```

The manifest owns count, shapes, dtypes, LOD page sizes, texture format, layer capacity, reduction
method/seed, source dataset, and metadata file. The runtime loader must validate sizes without
guessing. A tiny synthetic fixture may be used for CI and offline smoke tests.


## Texture Packing And LOD

Preferred first implementation: one texture array per `(LOD, page)`. Each item stores stable id,
position, sprite size, mean color, and either derived or explicit page/layer ids. Derive ids when
all pages share capacity:

```text
page_id = image_id / layers_per_page
layer_id = image_id % layers_per_page
```

Use atlas pages with padded UV rectangles only if backend texture-array limits require it.

LOD is selected from projected sprite size:

| Projected size | Display |
| ---: | --- |
| `< 2 px` | mean-color rectangle |
| `2-4 px` | 2x2 |
| `4-8 px` | 4x4 |
| `8-16 px` | 8x8 |
| `16-32 px` | 16x16 |
| `32-64 px` | 32x32 |
| `>= 64 px` | 64x64 when available, else 32x32 |

CPU culling and LOD assignment over 10,000 items is acceptable. Add 15 percent hysteresis once LOD
switching is interactive. Draw visible items in stable order unless the dataset defines a score or
`z` order.


## Minimal Implementation Target

1. Load a validated prepared bundle or deterministic fixture.
2. Normalize the robust 99th percentile embedding extent to about `[-1, 1]` while preserving aspect.
3. Render mean-color rectangles through the scene path with panzoom and CPU hover identification.
4. Add 2x2 through 32x32 texture-array pages and partition visible items by `(LOD, page)`.
5. Update only retained instance buffers or visual attributes when partitions change.
6. Keep base sprite size around 24 px, with 1 px minimum and 96 px maximum.


## Validation

Acceptance criteria:

- bundle with <=10,000 images loads and validates all payload sizes;
- initial view is nonblank and shows all images as mean-color/coarse sprites;
- zoom reveals progressive LODs through 32x32;
- panzoom remains interactive with all items visible;
- hover/click returns stable `image_id` after LOD changes;
- no per-frame texture/pipeline creation;
- smoke/manual notes cover load, zoom, hover, and teardown.

Track per-frame DRP2 command count, active texture pages/bind groups, buffer writes, CPU culling/LOD
time, and GPU sprite draw time when profiling.


## Optional Extensions

- 3-D embedding view with billboard sprites and arcball/camera control.
- Optional 64x64 detail pages.
- Atlas fallback for limited texture-array layers.
- PCA reconstruction comparison mode using precomputed thumbnail payloads.
- Out-of-core paging for datasets larger than 10,000 images.
