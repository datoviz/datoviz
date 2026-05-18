# Image Embedding LOD Explorer Example Specification

> **Agent Pickup**
> - **Category:** `dashboards`
> - **Implementation target:** Polished demo concept; implement in stages so the first slice can run with bounded resources.
> - **Data policy:** Public/downloaded assets require cache metadata and an offline fallback or reduced fixture.
> - **Preprocessing:** Usually required; specify source download, conversion, decimation/packing, and generated cache files.
> - **Validation:** Manual visual checklist plus bounded smoke command; add screenshot/readback validation when feasible.


## Summary

This example demonstrates an interactive image embedding explorer for Datoviz v0.4. A set of
images is embedded into a high-dimensional feature space, reduced to 2-D or 3-D coordinates, and
shown as image sprites at their projected positions.

The core rendering feature is level of detail (LOD): when the view is zoomed out, each image is
shown as a rectangle filled with its mean color; as the user zooms in, the same item progressively
uses precomputed 2x2, 4x4, 8x8, 16x16, 32x32, and optionally 64x64 thumbnails. The runtime should
only choose and draw the appropriate LOD. Expensive embedding, resizing, color conversion, packing,
and optional PCA reconstruction belong in preprocessing.

The first implementation target is at most 10,000 images. That size is large enough to pressure-test
scene image batching, texture arrays, interaction, culling, picking, and LOD selection without
requiring an out-of-core streaming system on day one.

This is an informative worked example, not a normative API proposal. It pressure-tests the scene
image visual, panzoom/camera controllers, retained sampled fields, texture-array or atlas resource
management, and item picking. Validate through the smoke/manual checklist path by checking startup,
LOD changes under zoom, nonblank rendering, culling behavior, and stable item picking.

For broader v0.5+ dashboard architecture ideas that may eventually simplify this example's LOD,
linked interaction, telemetry, and retained-resource update paths, see
`spec/scene/dashboards/DASHBOARD_RENDERING_ROADMAP.md`.


## Example Name

Suggested C example:

```text
examples/c/image_embedding_lod_glfw.c
```

Suggested preprocessing script:

```text
examples/c/prepare_image_embedding_lod.py
```

Suggested dataset bundle directory:

```text
data/image_embedding_lod/
```

Suggested gallery title:

```text
Image Embedding LOD Explorer
```


## User-Facing Behavior

The application opens a Datoviz window with one main embedding panel.

By default:

1. up to 10,000 images are displayed in a 2-D embedding;
2. images use data-space anchors and screen-space sprite sizes;
3. panzoom controls move through the embedding;
4. coarse zoom levels show colored rectangles derived from image mean color;
5. intermediate zoom levels show progressively finer thumbnails;
6. high zoom levels show recognizable 32x32 or 64x64 thumbnails;
7. hovering or clicking an item reports the image id and metadata;
8. the runtime does not resize images, recompute embeddings, or repack texture data.

An optional 3-D variant may use an arcball/camera controller and billboard sprites, but the first
version should be 2-D to keep the interaction and LOD policy easy to validate.


## Owning Specs

This example exercises:

1. `../../visuals/IMAGE.md` for textured rectangles and solid-color image items;
2. `../../pipeline/RESOURCE_MODEL.md` for sampled fields, texture resources, and retained updates;
3. `../../pipeline/FRAME_PLAN.md` for batched draw emission;
4. `../../pipeline/INVALIDATION_AND_CACHING.md` for avoiding per-frame resource churn;
5. `../../interaction/CONTROLLERS.md` for panzoom or camera control;
6. `../../interaction/PICKING.md` for item identification under the cursor;
7. `../../semantics/VISUAL_CONTRACT.md` for stable visual and item identity.


## Dataset Scale

The initial target is:

```text
n_images <= 10,000
embedding_dim_runtime = 2, optionally 3
thumbnail_lods = 1x1, 2x2, 4x4, 8x8, 16x16, 32x32
optional_lods = 64x64
runtime_image_format = RGBA8 or BGRA8, whichever the active texture path supports best
```

Approximate uncompressed RGBA8 texture memory for 10,000 square thumbnails:

| LOD | Pixels per image | Bytes per image | Bytes for 10k |
| --- | ---: | ---: | ---: |
| 1x1 | 1 | 4 | 40 KB |
| 2x2 | 4 | 16 | 160 KB |
| 4x4 | 16 | 64 | 640 KB |
| 8x8 | 64 | 256 | 2.5 MB |
| 16x16 | 256 | 1,024 | 10 MB |
| 32x32 | 1,024 | 4,096 | 40 MB |
| 64x64 | 4,096 | 16,384 | 160 MB |

The default bundle should include up to 32x32 for broad portability. 64x64 should be optional or
loaded only when the GPU budget is known to be sufficient.


## Preprocessing Goals

Preprocessing should produce render-ready arrays. Runtime should not depend on machine-learning
libraries, image-processing libraries beyond a lightweight loader, or dataset-specific feature
extractors.

The preprocessing script should:

1. load a directory or manifest of source images;
2. decode images and apply EXIF orientation when available;
3. convert to a consistent RGBA8 color space;
4. preserve aspect ratio by fitting into a square thumbnail with transparent or edge-color padding;
5. compute per-image mean color from valid pixels;
6. generate all requested LOD thumbnails using high-quality area or Lanczos resampling;
7. compute or load high-dimensional feature vectors;
8. reduce features to 2-D or 3-D coordinates;
9. normalize coordinates to a stable scene-space extent;
10. pack thumbnails into texture-array pages or atlas pages;
11. write a compact manifest and binary payloads;
12. verify that every runtime id resolves to one image, one position, one metadata row, and one
    texture layer or atlas rectangle per LOD.


## Embedding Policy

The example should support two preprocessing modes.

### Features Provided

If a feature array is provided, preprocessing uses it directly:

```text
features float32[n_images, feature_dim]
```

The script may then run PCA, UMAP, t-SNE, or a caller-selected reducer. PCA should be the default
because it is deterministic, cheap, and does not add heavy dependencies unless already available.

### Images Only

If only images are provided, the first implementation may use simple deterministic features:

1. downsample each image to 16x16 or 32x32 RGB;
2. flatten to a float32 vector;
3. standardize features;
4. run PCA to 2-D or 3-D.

This mode is not meant to be state-of-the-art semantic embedding. Its purpose is to make the example
run from a local image folder without a model download.

Future variants may accept CLIP, DINO, ResNet, or externally generated embeddings, but those model
dependencies should stay outside the default runtime path.


## Runtime Bundle Format

The first version may use a directory bundle with simple files:

```text
manifest.json
positions.f32
sizes.f32
colors.rgba8
items.u32
lod_1_page_000.rgba8
lod_2_page_000.rgba8
lod_4_page_000.rgba8
lod_8_page_000.rgba8
lod_16_page_000.rgba8
lod_32_page_000.rgba8
metadata.jsonl
```

The manifest owns shape, type, and page metadata:

```json
{
  "version": 1,
  "count": 10000,
  "embedding_dim": 2,
  "position_dtype": "float32",
  "position_shape": [10000, 3],
  "sprite_size_dtype": "float32",
  "mean_color_dtype": "rgba8",
  "lods": [
    {
      "size": 1,
      "format": "rgba8",
      "page_count": 1,
      "layers_per_page": 10000,
      "files": ["lod_1_page_000.rgba8"]
    },
    {
      "size": 32,
      "format": "rgba8",
      "page_count": 3,
      "layers_per_page": 4096,
      "files": [
        "lod_32_page_000.rgba8",
        "lod_32_page_001.rgba8",
        "lod_32_page_002.rgba8"
      ]
    }
  ],
  "metadata": "metadata.jsonl"
}
```

The exact container can evolve later into a single binary file. The first requirement is that the
runtime loader can validate sizes without guessing.


## Texture Packing

Preferred first implementation: one texture array per `(LOD, page)`.

Each image instance stores:

```text
image_id   uint32
position   float32[3]
size       float32[2] or scalar base size
mean_color rgba8
page_id    uint32 per LOD, or derived from image_id
layer_id   uint32 per LOD, or derived from image_id
```

For 10,000 images, derive `page_id` and `layer_id` from the image index when all pages use the same
layer capacity:

```text
page_id = image_id / layers_per_page
layer_id = image_id % layers_per_page
```

Texture arrays are preferred over atlases for the first version because they avoid bleeding between
neighboring thumbnails and keep LOD sampling simple. If the active backend cannot support enough
array layers, use square atlas pages with per-item UV rectangles and padding.


## LOD Selection

LOD should be selected from projected screen size in pixels. The policy should be deterministic and
cheap enough to evaluate every frame.

Recommended thresholds:

| Projected sprite size | Display |
| ---: | --- |
| `< 2 px` | solid rectangle using mean color |
| `2-4 px` | 2x2 thumbnail |
| `4-8 px` | 4x4 thumbnail |
| `8-16 px` | 8x8 thumbnail |
| `16-32 px` | 16x16 thumbnail |
| `32-64 px` | 32x32 thumbnail |
| `>= 64 px` | 64x64 thumbnail when available, otherwise 32x32 |

The LOD decision may be made on the CPU per frame for the first implementation. Items should then be
partitioned into batches by `(LOD, page)`:

```text
solid mean-color batch
lod 2, page 0
lod 2, page 1
lod 4, page 0
...
lod 32, page N
```

Hysteresis should be used once runtime LOD switching is implemented interactively, so an item does
not flicker when its screen size hovers near a threshold. A simple policy is to switch to a finer LOD
only when the projected size is 15 percent above the threshold, and to switch to a coarser LOD only
when it is 15 percent below the threshold.


## Culling and Overlap Policy

The first version should implement view-frustum or panel-rectangle culling before assigning LOD
batches. For <=10,000 images, a CPU pass over all items per frame is acceptable.

When zoomed far out, many images may overlap heavily. The first version should keep the behavior
simple:

1. draw all visible items in stable input order;
2. optionally sort by cluster, score, or `z` only when the dataset defines one;
3. avoid expensive overlap removal in the renderer;
4. leave screen-space decluttering as a future feature.

If visual density is too high, the example may expose a maximum visible item budget and deterministic
subsampling policy, but this should not be required for 10,000 items.


## Scene Setup

The 2-D scene contains:

1. one figure and one full-window panel;
2. a panzoom controller;
3. one or more image visuals for thumbnail LOD batches;
4. one rectangle or image-color visual for the mean-color batch;
5. optional hover highlight visual;
6. optional selected-image detail panel once UI support is available.

All item positions use normalized embedding coordinates. The default normalization should fit the
robust 99th percentile extent into approximately `[-1, 1]` on the longest axis while preserving
aspect ratio.

Sprite size should be defined in screen pixels for the default explorer:

```text
base_sprite_size_px = 24
min_sprite_size_px = 1
max_sprite_size_px = 96
```

The panzoom transform changes projected screen size. The LOD policy uses the current transform to
choose which thumbnail resolution to draw.


## FramePlan Shape

For a typical frame:

```text
update controller transform
compute visible items and projected sprite sizes
partition visible items by LOD and page
update per-batch instance buffers when partitions changed
render mean-color rectangles
render thumbnail batches ordered coarse to fine or stable by LOD/page
render hover/selection overlay
resolve/present
```

The example should avoid creating GPU textures, samplers, bind group layouts, pipelines, or shader
modules per frame. Per-frame changes should be limited to small instance buffers or existing retained
visual attribute updates.


## DRP2 Command Categories

The example implies:

1. buffer creation and writes for positions, sizes, item ids, and per-batch instances;
2. texture creation and writes for LOD texture arrays or atlas pages;
3. sampler creation, preferably nearest sampling for very coarse LOD and linear sampling for larger
   thumbnails;
4. bind groups for each active `(LOD, page)` texture resource;
5. render pipelines for solid-color rectangles and sampled thumbnails;
6. per-frame viewport and scissor setup;
7. draw or draw-indexed commands for instanced quads;
8. optional readback or picking command stream for hover/click item identity.


## Picking

Picking should return the stable `image_id`, not a transient batch-local index.

The first version may implement picking by CPU nearest-neighbor search in embedding space if GPU item
picking is not ready for image sprites. A GPU-backed path is preferred once item-id rendering exists
for image visuals.

Expected hover/click payload:

```text
image_id uint32
position float32[3]
filename string, optional
label string, optional
score float32, optional
lod uint32, current selected display LOD
```

Picking should continue to work after LOD changes and batch repartitioning.


## PCA Reconstruction Variant

PCA reconstruction is optional and should not block the texture-array implementation.

A future mode may store:

```text
mean_image float32[h, w, channels]
basis      float32[n_components, h, w, channels]
coeff      float32[n_images, n_components]
```

At preprocessing time, PCA reconstruction can generate the same thumbnail LOD payloads as the
default path, allowing direct visual comparison between real downsampled thumbnails and compressed
reconstructions.

Runtime shader reconstruction should be treated as a later experiment. It increases shader
complexity, descriptor pressure, validation complexity, and artifact risk. The default example
should remain texture-array based.


## Performance Targets

For the default 10,000-image, 2-D dataset:

1. initial load should be dominated by texture upload, not CPU resizing;
2. steady pan/zoom should avoid GPU resource creation;
3. LOD repartitioning should not allocate unbounded transient objects;
4. a laptop-class discrete or integrated GPU should remain interactive at 60 Hz for typical views;
5. full zoomed-out views should remain responsive even when all items are visible.

Performance diagnostics should watch:

1. per-frame DRP2 command count;
2. number of active texture pages and bind groups;
3. number of per-frame buffer writes;
4. CPU time spent in culling and LOD assignment;
5. GPU time spent drawing overlapping sprites.


## Acceptance Criteria

The first implementation is acceptable when:

1. a prepared bundle with <=10,000 images loads successfully;
2. all required LOD payload sizes are validated before upload;
3. the initial view shows all images as mean-color or coarse thumbnail sprites;
4. zooming in progressively reveals 2x2, 4x4, 8x8, 16x16, and 32x32 detail;
5. LOD changes do not recreate textures or pipelines;
6. panzoom remains interactive with all items visible;
7. hover or click reports a stable image id;
8. the example has a deterministic small fallback dataset if no external image bundle is provided;
9. the code path runs through scene -> DRP2 -> vklite/canvas, not a parallel renderer;
10. a focused smoke test or manual validation note records load, zoom, hover, and teardown behavior.


## Implementation Phases

### Phase 1: Static Bundle and Mean-Color Rectangles

1. Write the preprocessing script and bundle manifest.
2. Generate positions, mean colors, and metadata.
3. Render all images as colored rectangles through the scene path.
4. Add panzoom and CPU hover identification.

### Phase 2: Texture-Array Thumbnail LOD

1. Generate 2x2 through 32x32 texture-array pages.
2. Load and validate all LOD pages.
3. Partition visible items by screen-space LOD.
4. Render instanced thumbnail batches.
5. Add hysteresis and stable batch update behavior.

### Phase 3: Resource and Interaction Hardening

1. Add culling before LOD partitioning.
2. Add selected-item highlight and optional metadata readout.
3. Record DRP2 trace output for representative zoom changes.
4. Add focused tests for bundle validation and LOD threshold selection.

### Phase 4: Optional Extensions

1. 3-D embedding view with billboard sprites and arcball/camera control.
2. Optional 64x64 high-detail pages.
3. Optional atlas fallback for backends with limited array layers.
4. Optional PCA reconstruction comparison mode.
5. Optional out-of-core paging for datasets larger than 10,000 images.


## Open Questions

1. Should the first implementation add a dedicated multi-image sprite visual, or use the existing
   image visual with one retained visual per LOD/page batch?
2. Should LOD partitioning live in the example, a scene helper, or a reusable visual policy object?
3. What texture-array layer limit should the native vklite runtime expose through scene
   capabilities?
4. Should the bundle become a formal Datoviz asset format once a second example needs paged
   thumbnail arrays?
5. Should CPU fallback picking use nearest center, sprite rectangle containment, or both?
