# Semantic Embedding Atlas Example Specification

> **Agent Pickup**
> - **Category:** `dashboards`
> - **Implementation target:** Polished demo concept; implement in stages with a bounded first
>   bundle.
> - **Data policy:** Public/downloaded assets require cache metadata and a deterministic reduced
>   fixture.
> - **Preprocessing:** Required; specify source download, sampling, vector/position extraction,
>   clustering, string packing, and generated cache files.
> - **Validation:** Manual visual checklist plus bounded smoke command; add screenshot/readback
>   validation when text rendering and picking are available.


## Summary

This example demonstrates a semantic embedding explorer for Datoviz v0.4. Wikipedia articles are
represented as embedded entities rather than raw words. Each item has a title, URL, wiki subset,
stable id, existing 2-D map coordinates, and optionally the original N-dimensional vector. The
runtime shows the articles as a large semantic atlas with colored points, cluster labels, selected
cards, search, and nearest-neighbor highlighting.

The default public dataset is
[nomic-ai/nomic-embed-v2-wikivecs](https://huggingface.co/datasets/nomic-ai/nomic-embed-v2-wikivecs),
which exposes map columns such as `x`, `y`, `title`, `subset`, `url`, and `wid`. The associated
Wikivecs paper describes a fully reproducible vectorization of multilingual Wikipedia with dense
vectors for articles across many language editions.

This is an informative worked example, not a normative API proposal. It pressure-tests point
rendering, panzoom navigation, rendered text, label LOD, picking, selection, nearest-neighbor
queries, and small screen-space cards. It complements `IMAGE_EMBEDDING_LOD.md`: the image example
teaches visual/image similarity with thumbnail LOD, while this example teaches semantic similarity
with text labels and metadata.


## Example Name

Suggested C example:

```text
examples/c/semantic_embedding_atlas_glfw.c
```

Suggested preprocessing script:

```text
examples/c/prepare_semantic_embedding_atlas.py
```

Suggested dataset bundle directory:

```text
data/semantic_embedding_atlas/
```

Suggested gallery title:

```text
Semantic Atlas: Wikipedia Embedding Explorer
```


## Owning Specs

This example exercises:

1. `EMBEDDING_EXPLORER_COMMON.md` for shared embedding-explorer bundle and query policy;
2. `../../visuals/POINT.md` or marker/pixel visuals for dense item rendering;
3. `../../semantics/TEXT.md` and `../../proposals/TEXT_DESIGN.md` for labels;
4. `../../proposals/SCREEN_SPACE_OVERLAY_LAYOUT.md` for selected-item cards;
5. `../../interaction/CONTROLLERS.md` for panzoom navigation;
6. `../../interaction/PICKING.md` and `../../interaction/SELECTION.md` for item selection;
7. `../../pipeline/FRAME_PLAN.md` and `../../pipeline/INVALIDATION_AND_CACHING.md` for retained
   buffers and stable per-frame updates.


## Dataset Scale

The first implementation target is a bounded English or multilingual sample:

```text
n_items_small_fixture <= 1,000
n_items_default <= 100,000
n_items_showcase <= 1,000,000, optional later
runtime_embedding_dim = optional, model-dependent
display_dim = 2 first, optionally 3 later
```

The first practical bundle should start with the existing `x,y` map coordinates so the example can
focus on Datoviz rendering and interaction. A later preprocessing mode may load the N-dimensional
vectors and generate local PCA, UMAP, or 3-D projections.

Full Wikivecs scale is out of scope for the first version. It requires tiled loading, string-table
paging, and approximate-nearest-neighbor search. The first version should make those future
boundaries visible in the manifest without implementing an out-of-core atlas.


## User-Facing Behavior

The application opens a Datoviz window with one main semantic map panel.

By default:

1. up to 100,000 Wikipedia articles are displayed as points in a 2-D semantic map;
2. panzoom controls move through the atlas;
3. zoomed-out views show only dense colored points and a few representative cluster labels;
4. mid-zoom views reveal important article titles;
5. close views reveal more local labels and selected-neighborhood highlights;
6. hovering or clicking an article reports the stable article id and metadata;
7. selecting an item opens a small screen-space card with title, wiki subset, URL, and nearest
   neighbors;
8. title search jumps to matching article titles without using an embedding model;
9. optional semantic query highlights articles near a typed query vector.

An optional 3-D variant may use a camera controller, but the first version should be 2-D to keep
label placement, picking, and interaction easy to validate.


## Data And Metadata

The cache should preserve the distinction between article identity, projection coordinates, and
semantic vectors:

```text
wid          stable Wikipedia id from Wikivecs
title        article title
url          article URL
subset       wiki subset/language edition
position     display position from existing x/y or local reduction
embedding    optional N-dimensional article vector
cluster_id   optional semantic cluster/category
importance   optional label priority
```

Raw English word embeddings should not be the flagship dataset. Wikipedia articles are
disambiguated entities, so labels such as `Apple Inc.`, `Apple`, `Machine learning`, and
`Transformer architecture` are clearer than ambiguous word tokens.


## Preprocessing Goals

The preprocessing script should:

1. download or read selected Wikivecs parquet/shard files;
2. sample deterministically by subset, title rank, cluster, or random seed;
3. load existing `x,y` map coordinates for the first version;
4. optionally load N-dimensional vectors for nearest-neighbor search and semantic query;
5. normalize positions to a robust scene-space extent;
6. assign colors by cluster, subset, or local density;
7. compute label priority from item importance, cluster centrality, or representative sampling;
8. pack strings into compact tables or write metadata JSONL;
9. write preset queries and expected nearest-neighbor ids for smoke validation;
10. validate that every rendered item resolves to one title, URL, subset, and stable id.


## Runtime Bundle Format

The first version may use a directory bundle with simple files:

```text
manifest.json
positions.f32
colors.rgba8
items.u64
clusters.u32
importance.f32
labels.txt
labels.idx
metadata.jsonl
embeddings.f32, optional
query_presets.jsonl, optional
```

The manifest owns shape, type, source, and projection metadata:

```json
{
  "version": 1,
  "dataset": {
    "id": "nomic-ai/nomic-embed-v2-wikivecs",
    "revision": "optional Hugging Face revision",
    "subset_filter": ["20231101.en"],
    "sample_seed": 12345
  },
  "count": 100000,
  "position_dtype": "float32",
  "position_shape": [100000, 3],
  "position_source": "wikivecs_x_y",
  "embedding": {
    "stored": true,
    "dtype": "float32",
    "shape": [100000, 768],
    "file": "embeddings.f32",
    "normalized": true
  },
  "metadata": "metadata.jsonl",
  "labels": {
    "text": "labels.txt",
    "index": "labels.idx"
  }
}
```

For the first C runtime, `embeddings.f32` may be omitted. Title search, picking, label LOD, and
selected cards only require positions and metadata.


## Query Policy

The example should support three query levels:

1. exact or fuzzy title search in C;
2. precomputed query presets loaded from the bundle;
3. optional arbitrary semantic query through a Python sidecar.

The sidecar embeds typed text with the same model family as the source vectors and returns one
float32 vector. The C runtime then performs nearest-neighbor search against the loaded vectors and
highlights the result ids. Datoviz should not bundle a transformer inference runtime for the first
slice.

For <=100,000 loaded vectors, brute-force normalized dot product is acceptable. For larger bundles,
the preprocessing script should generate an approximate-nearest-neighbor index and the runtime
should load only the index format it explicitly understands.


## Scene Setup

The 2-D scene contains:

1. one figure and one full-window panel;
2. a panzoom controller;
3. one point, pixel, or marker visual for articles;
4. optional cluster-label text visuals;
5. optional title-label text visuals with screen-space LOD;
6. optional hover and selected-neighborhood highlight visuals;
7. a selected-item screen-space card once overlay layout support exists.

The default normalization should fit the robust 99th percentile extent into approximately `[-1, 1]`
on the longest axis while preserving aspect ratio. Label decisions should be made from projected
screen density and per-item priority, not from source-file order.


## FramePlan Shape

For a typical frame:

```text
update panzoom transform
compute visible items and projected density
choose label LOD from zoom, density, and priority
update highlight buffers if selection/query changed
render dense article points
render cluster labels and local title labels
render hover/selection overlays and selected card
resolve/present
```

The example should avoid creating GPU buffers, text resources, pipelines, or descriptor state per
frame. Per-frame changes should be limited to controller state, small highlight buffers, label
visibility ranges, and overlay/card contents.


## Picking And Selection

Picking should return the stable Wikivecs article id, not a transient draw index.

Expected hover/click payload:

```text
item_id uint64
position float32[3]
title string
subset string
url string
cluster_id uint32, optional
distance float32, optional for query result mode
```

Selecting an item should:

1. highlight the selected article;
2. optionally highlight the nearest neighbors when embeddings are loaded;
3. center or pin the selected item only when the user requests it;
4. update the screen-space card without rebuilding scene resources.


## Acceptance Criteria

The first implementation is acceptable when:

1. a prepared bundle with <=100,000 articles loads successfully;
2. all required payload sizes are validated before use;
3. the initial view shows a nonblank semantic map;
4. panzoom remains interactive with all items visible;
5. label LOD changes under zoom without rebuilding text resources every frame;
6. hover or click reports a stable article id;
7. title search jumps to an article and highlights it;
8. optional query presets highlight stable nearest-neighbor results;
9. the example has a deterministic small fallback fixture;
10. the code path runs through scene -> DRP2 -> vklite/canvas, not a parallel renderer.


## Implementation Phases

### Phase 1: Static Point Atlas

1. Write the preprocessing script and bundle manifest.
2. Load Wikivecs `x,y,title,subset,url,wid` columns.
3. Render all items as colored points.
4. Add panzoom and CPU hover identification.

### Phase 2: Labels And Selection

1. Generate label priorities and cluster labels.
2. Add text label LOD for representative items.
3. Add selected-item highlight and small metadata card.
4. Add exact/fuzzy title search.

### Phase 3: Embedding-Space Search

1. Store normalized N-dimensional vectors for a small bundle.
2. Add brute-force nearest-neighbor search in C.
3. Add precomputed query presets.
4. Add optional Python sidecar for arbitrary text query embeddings.

### Phase 4: Scale And Polish

1. Add larger bundles with tiled metadata and string tables.
2. Add approximate-nearest-neighbor index support.
3. Add 3-D projection and camera navigation.
4. Add density-aware label decluttering.


## Open Questions

1. Should the first bundle use English Wikipedia only or a balanced multilingual sample?
2. Should cluster colors be generated from original vectors, 2-D map neighborhoods, or article
   metadata?
3. Which label renderer path should be used before full glyph rendering is promoted?
4. What compact string-table format should be shared with other large-label examples?
5. Should the optional query sidecar use stdin/stdout, a local HTTP endpoint, or a simple file-drop
   protocol?
