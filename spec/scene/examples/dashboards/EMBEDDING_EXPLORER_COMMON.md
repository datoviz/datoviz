# Embedding Explorer Common Design

> **Status:** Informative shared design note.
> **Scope:** shared data, preprocessing, interaction, and runtime policies for embedding explorer
> dashboard examples.
> **Consumers:** `IMAGE_EMBEDDING_LOD.md` and `SEMANTIC_EMBEDDING_ATLAS.md`.


## Summary

Embedding explorer examples visualize a collection of items with high-dimensional vectors. The
preprocessing step reduces those vectors to 2-D or 3-D for display. The runtime renders the reduced
positions, metadata, categories, labels, thumbnails, or cards without owning heavyweight machine
learning dependencies.

The examples should teach the same idea across different modalities:

1. images cluster by visual or image-caption embedding similarity;
2. entities or documents cluster by semantic text similarity;
3. selecting an item reveals metadata and nearest neighbors;
4. optional query embeddings make typed text behave like another point in the same vector space.


## Shared Item Model

Every embedding explorer bundle should make the distinction between vector-space data, display data,
and metadata explicit:

```text
item_id          uint32 or uint64 stable id
embedding        optional float32[n_items, embedding_dim]
position         float32[n_items, 3] reduced 2-D or 3-D display position
category         optional uint32 cluster/category id
color            optional rgba8[n_items]
importance       optional float32[n_items] label or draw priority
visual_payload   thumbnails, labels, glyph ids, icons, or card metadata
metadata         JSONL or indexed strings
```

The reduced position is never treated as the source of semantic truth. Search, nearest-neighbor
lists, and query results should use the N-dimensional embedding when the bundle stores it. The
2-D/3-D position is only the projection used for rendering and navigation.


## Preprocessing Boundary

Preprocessing is responsible for expensive, unstable, or optional dependencies:

1. downloading and caching public data;
2. sampling with a recorded seed;
3. decoding images or extracting text fields;
4. loading or generating N-dimensional embeddings;
5. normalizing embeddings for cosine or dot-product search;
6. reducing embeddings to 2-D and optionally 3-D;
7. clustering or assigning categories;
8. generating colors, label priorities, and representative cluster labels;
9. packing thumbnails, string tables, metadata, and binary arrays;
10. writing a manifest with schema version, shapes, dtypes, sources, and reduction parameters.

The C runtime should be able to load a finished bundle without importing Python libraries, calling
network APIs, running embedding models, resizing images, or recomputing dimensionality reduction.


## Runtime Boundary

The C example should focus on visualization and interaction:

1. load the manifest and validate every array size before upload or use;
2. render points, labels, thumbnails, or cards through the scene -> DRP2 -> runtime path;
3. pan, zoom, pick, select, and highlight items;
4. jump to an item or query result;
5. show nearest-neighbor lists when vectors are present;
6. keep all GPU resources retained across frames unless the loaded bundle changes.

For bundles up to roughly 100k items, brute-force CPU nearest-neighbor search over normalized
float32 vectors is acceptable for development and demonstration. Larger bundles should use a
precomputed approximate-nearest-neighbor index generated during preprocessing.


## Query Embeddings

Typed semantic query has two levels.

Title or metadata search is a string operation. It does not require an embedding model and should be
available in the C runtime for both image and semantic examples.

Semantic query embeds arbitrary user text with the same model family used by the source vectors.
The recommended first implementation keeps that model outside Datoviz:

```text
C app query text
  -> optional Python sidecar or offline query cache
  -> query vector float32[embedding_dim]
  -> C nearest-neighbor search
  -> highlighted ids and viewport jump
```

The first checked-in example should ship a small set of precomputed query vectors so the behavior is
testable without a live Python process. A Python sidecar can be added as optional development
infrastructure. Datoviz should not bundle a transformer inference runtime for the first embedding
explorer slice.


## Bundle Families

The image and semantic examples should share naming conventions where possible:

```text
manifest.json
positions_2d.f32
positions_3d.f32, optional
embeddings.f32, optional
colors.rgba8
items.u32 or items.u64
categories.u32, optional
metadata.jsonl
strings.bin and strings.idx, optional compact text storage
query_presets.jsonl, optional
```

Image bundles add thumbnail LOD pages. Semantic bundles add label tables, cluster labels, and
possibly page summaries or lead-image references.


## Visual And Interaction Pattern

The shared user experience should be consistent:

1. zoomed out: dense points, mean colors, or density-like marks;
2. mid zoom: cluster labels, important item labels, or coarse thumbnails;
3. close zoom: readable item labels, thumbnails, or selected-neighborhood highlights;
4. selection: a compact card with item metadata and nearest neighbors;
5. search: jump to title/metadata match or semantic-query result;
6. color modes: cluster/category, source, language, year, score, or selected-neighborhood distance.

These examples should pressure-test text rendering, picking, screen-space overlays, large retained
buffers, and optional image LOD without requiring a general GUI framework.
