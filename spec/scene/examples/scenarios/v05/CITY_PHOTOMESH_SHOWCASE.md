# City Photomesh Showcase

> **Scenario ID:** `showcases_city_photomesh`
> **Example status:** post-RC3, non-blocking v0.5 showcase design
> **Target:** native C showcase and deterministic preparation pipeline
> **Data:** source-gated public download, prepared bundle outside the parent repository
> **Validation:** preparation checks, native smoke, deterministic screenshot, and manual visual review

## Decision

Design a native Datoviz showcase for a textured photogrammetric city crop, beginning with Helsinki only after source access, license, attribution, redistribution, and exact download facts have been recorded by an inspection step. The design is not an RC3 or RC4 release commitment and must not delay either release.

The intended showcase has two reproducible profiles generated from one source crop: `1m` targets one million aggregate triangles for normal interactive use, while `10m` targets ten million for a local high-quality demonstration. They use the same local origin, coordinate convention, crop, stable tile boundaries, camera presets, and provenance metadata so that later per-tile LOD remains possible.

Raw downloads, intermediate outputs, and prepared assets are not parent-repository content. Follow [the shared example-data policy](../../../../../tools/data/DATA_POLICY.md): use `.cache/datoviz/examples/city_photomesh/` during investigation and preparation; promote only deliberately reviewed, LFS-managed data-submodule artifacts with complete provenance. No code, data-submodule pointer, generated binary, or gallery-media work is implied by this design.

## Candidate Source And License Gate

The first candidate is the City of Helsinki photogrammetric 3D mesh, whose official entry point is [Helsinki 3D](https://www.hel.fi/en/decision-making/information-on-helsinki/maps-and-geospatial-data/helsinki-3d). The source is expected to be textured OBJ/MTL data in ETRS-GK25 horizontal coordinates with N2000 heights, but neither the archive name, coverage, source LODs, accuracy, access mechanism, nor license terms are implementation facts until the inspection report records the exact official source.

Before preparation, record the source URL, retrieval date, archive SHA-256, publisher, applicable license version, required attribution, permission for prepared-asset and generated-media redistribution, coordinate reference system, height datum, and any access restrictions. If these facts do not permit the intended distribution, mark the bundle `external-required`, retain the design, and do not commit downloaded or derived artifacts. A documented synthetic fallback is acceptable only for runtime/renderer development; it must not be presented as Helsinki.

The relevant ingestion context is Rantanen et al., [*Open Geospatial Data Integration in Game Engine for Urban Digital Twin Applications*](https://doi.org/10.3390/ijgi12080310), ISPRS International Journal of Geo-Information 12(8), 310 (2023). It informs inspection questions only; it is not authority for the selected archive or its license.

Google Photorealistic 3D Tiles and Cesium Native are deferred. Do not use them for an offline prepared bundle unless a separate legal and technical review proves that the exact proposed acquisition, derivative, storage, and redistribution paths are allowed.

## Showcase Scope

Select a compact central-Helsinki crop approximately 0.5–1.0 km across, after inspection, with mixed building heights, streets/courtyards, vegetation or other photogrammetric detail, recognizable context, and enough open space to make fly navigation legible. The chosen bounds and local origin must be part of the manifest, not an undocumented constant in an example.

| Profile | Aggregate triangle target | Initial texture policy | Purpose |
| --- | ---: | --- | --- |
| `1m` | 1,000,000 within ±1% | Per-tile atlases normally limited to 1K–2K | Default interactive example, development, and smoke capture. |
| `10m` | 10,000,000 within ±1% | Per-tile atlases normally limited to 2K–4K | Local high-quality showcase and fidelity comparison. |

Texture allocation is ultimately constrained by a total decoded texture-memory budget, not a uniform atlas size. Record that budget and actual encoded and decoded usage in each profile manifest. Dense landmark tiles may warrant more texture resolution than low-information roof, water, or peripheral tiles.

## Representation

The default MVP representation is one mesh visual per spatial tile and texture-atlas partition, never one visual per building. The current mesh path binds a geometry set and one sampled field named `texture`; a city OBJ source normally has many groups, materials, and images. Tiling therefore avoids an oversized city-wide atlas and creates the natural unit for later culling, LOD replacement, and residency.

Target 8–32 spatial tiles, with one atlas and one visual per tile in the normal case and only a few extra partitions where atlas limits require them. Prefer fewer than 64 visuals and sampled fields. A 1M spike may use one visual only when preprocessing proves that one indexed mesh and one atlas fit backend limits with acceptable atlas utilization, quality, and memory. It must not become the assumed architecture for the full crop.

The prepared bundle replaces runtime OBJ loading. The existing OBJ loader is useful for small assets but does not provide the multi-material texture assignments or indexed, bounded-memory representation required here. The relevant current foundations are `examples/c/features/mesh_texture.c`, `examples/c/showcases/terrain_relief.c`, `tools/data/prepare_terrain_relief.py`, `examples/c/features/controller_fly.c`, `docs/how-to/3d-navigation.md`, `examples/c/example_tuner.*`, and `src/geom/obj_loader.c`.

## Preparation Contract

The pipeline is intentionally two-phase:

```text
official source archive -> inspection manifest -> deterministic prepared tile bundle -> native showcase
```

Create `tools/data/inspect_city_photomesh.py` first. It must download or reuse a source only in the local cache and write `.cache/datoviz/examples/city_photomesh/inspection.json`. The report must include the source identity and checksums, archive size, OBJ/MTL/texture inventory, coordinate bounds, group/material relationships, source triangle and index counts, texture dimensions/formats, native LOD structure when present, estimated geometry and texture memory, candidate crop bounds, and the source-license gate result.

Only after inspection confirms a usable source may `tools/data/prepare_city_photomesh.py` be introduced. It owns cache paths, downloads, checksums, manifests, and conversion orchestration; bounded-memory mesh processing may use a dedicated native utility where that is materially safer than Python. The initial command shape is:

```sh
uv run tools/data/prepare_city_photomesh.py inspect
uv run tools/data/prepare_city_photomesh.py prepare --profile 1m --target-triangles 1000000
uv run tools/data/prepare_city_photomesh.py prepare --profile 10m --target-triangles 10000000
```

For a fixed source archive and pinned tool versions, preparation must be deterministic. It must parse OBJ/MTL/material references, triangulate, deduplicate composite OBJ vertices by position/UV/normal indices, crop, rebase coordinates, partition into stable tiles, preserve UV seams and material boundaries, generate atlases, allocate and reconcile global triangle budgets, simplify while protecting tile borders and meaningful disconnected components, optimize ordering, encode textures, and validate outputs. Tiles must either split cross-boundary triangles or apply one documented deterministic ownership rule.

Use a local Y-up rendering system to preserve `float32` precision. Unless inspection demonstrates a source-specific constraint, map `X = easting - origin_easting`, `Y = height - origin_height`, and `Z = -(northing - origin_northing)`. Store the source coordinate system, height datum, origin, complete transform, source crop bounds, and local tile bounds in the manifest.

Allocate initial tile budgets proportionally to each tile's source triangle count, then apply explicit minimums, optional complexity/projected-area weighting, and exact global reconciliation. The manifest must contain source and output triangle counts for every tile, as well as the rationale and parameters for any non-proportional adjustment.

## Bundle Format And Provenance

Use a versioned `meshbin` format rather than runtime OBJ parsing. Start with `float32` positions and normals, `float32` UVs, and `uint32` indices; packed attributes require a later precision and quality validation. Each header must identify magic/version, vertex/index counts, attribute encodings, local bounds, tile and profile IDs, material or texture reference, geometric-error estimate, and array byte offsets.

The local prepared layout is:

```text
.cache/datoviz/examples/city_photomesh/prepared/
  manifest.json
  attribution.txt
  viewpoints.json
  1m/tile_000.meshbin, tile_000.jpg, ...
  10m/tile_000.meshbin, tile_000.jpg, ...
```

The actual tile count follows inspection. Every bundle manifest must include source metadata, legal and attribution facts, source/archive/output checksums, preparation command and tool versions, coordinate metadata, crop and tile bounds, profile and tile triangle/vertex counts, texture dimensions and encoded/decoded sizes, total expected GPU memory, camera presets, and generated-media redistribution status.

## Native Showcase

`examples/c/showcases/city_photomesh.c` is introduced only after a validated prepared bundle exists. It locates a complete selected profile in the configured cache or promoted data bundle, preloads all tiles for the MVP, creates one mesh visual per tile/atlas partition, binds texture fields, enables opaque depth-tested rendering, and fails clearly with the exact preparation command when data is unavailable. It must never silently replace missing city data with an in-memory synthetic scene.

Default navigation is turntable, with fly navigation and deterministic named viewpoints. The first GUI may expose profile, tile-boundary/statistics overlays, navigation mode, field of view, fly speed, look sensitivity, viewpoint/reset, texture mode, restrained lighting controls, SSAO, depth cue/fog, MSAA, visual count, aggregate triangles, decoded texture memory, and frame time. Profile changes may recreate tile visuals; seamless asynchronous switching is deferred.

Photogrammetric textures contain baked illumination. Default to unlit texture rendering or deliberately restrained diffuse-plus-ambient lighting, and compare both modes before presenting lighting as a showcase improvement. Evaluate mipmaps and anisotropic filtering if aerial or oblique views shimmer; full PBR, normal maps, and image-based lighting are not prerequisites.

## Deferred Tiled LOD

Once both fixed profiles pass their independent validation, a later design slice may select `1m` or `10m` content per tile by projected size, frustum-cull invisible tiles, enforce a resident-memory budget, load/decode in workers, and create/replace/destroy visuals on the render thread. Mixed-LOD boundaries must avoid visible cracks and disruptive transitions. Do not start this slice before the fixed-profile bundle establishes its tile identity, ownership, and memory facts.

## Staged Deliverables And Acceptance

The first spike is source inspection plus one small selected tile or crop converted to an indexed mesh with one or a few atlased textures. It proves UV orientation, local coordinates, texture quality, measured geometry/texture memory, and the one-visual-versus-tiled decision. It does not download, commit, or redistribute a city dataset before the source-license gate passes.

The MVP produces deterministic 1M and 10M bundles, the C showcase, turntable/fly navigation, controls appropriate to the current public API, named screenshot viewpoints, and complete provenance/attribution. It accepts only when aggregate triangle counts are within ±1% of the requested profile; texture references, indices, positions, and tile bounds validate; source and output checksums exist; tile seams are not obvious from the default overview; depth behavior is stable; the 1M profile is responsive on a stated representative GPU; the 10M profile has a recorded fidelity advantage; and the named viewpoint yields a deterministic screenshot.

## Open Questions

- Which exact archive and crop best satisfy the visual and legal constraints?
- Does the source already provide a useful spatial or adaptive LOD hierarchy?
- How many material images exist in the selected crop, and can the 1M spike use one efficient atlas?
- Which texture-memory budgets produce a meaningful 1M/10M difference?
- Does simplification preserve seams and protected borders without custom constraints?
- Are mipmaps sufficient for oblique views, or is anisotropic filtering required?
- Should prepared textures use JPEG, PNG, KTX2/Basis, or another encoding after quality and packaging review?
