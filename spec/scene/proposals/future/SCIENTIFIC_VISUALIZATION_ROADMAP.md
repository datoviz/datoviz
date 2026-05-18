# Scientific Visualization Roadmap

> **Status:** exploratory roadmap for future v0.5+ scene capabilities.
> **Scope:** scientific data models and visual families not already fully covered by the active
> v0.4 scene implementation.
> **Purpose:** preserve future design directions without duplicating existing v0.4 specs.


## Summary

Datoviz v0.4 already has strong coverage for retained scene objects, DRP2 lowering, regular
sampled fields, images, volumes, meshes, points, paths, particles, picking, selection, dashboards,
napari integration, WebGPU planning, and compute-to-render pressure tests.

This document collects broader scientific visualization directions that remain only partially
covered or not covered by the current spec corpus. It is an index and orientation note, not a
frozen API proposal.

The main missing layer is not another low-level draw primitive. It is a set of richer semantic
resources that can lower to existing visuals and future compute/render pipelines:

- graph/network resources;
- unstructured grids and cell meshes;
- vector and tensor field interpretations;
- categorical label volumes and sparse voxel fields;
- time-aware tracks and trajectories;
- ensemble and uncertainty resources;
- molecular and structural-biology resources;
- out-of-core and progressive residency primitives.


## Existing Coverage To Reuse

Do not duplicate these as unrelated new systems.

### Active Or Planned Visual Building Blocks

- `point`, `pixel`, `marker`, `segment`, `path`, `image`, `mesh`, `sphere`, and `volume` are covered
  by the per-family docs in [`../../visuals/`](../../visuals/).
- Text, labels, colorbars, annotations, scales, picking, and selection already have semantic and
  proposal documents.
- Regular 2D/3D fields are covered by
  [`SAMPLED_FIELD_API_DESIGN.md`](../promoted/SAMPLED_FIELD_API_DESIGN.md),
  [`../../pipeline/RESOURCE_MODEL.md`](../../pipeline/RESOURCE_MODEL.md),
  [`../../visuals/IMAGE.md`](../../visuals/IMAGE.md), and
  [`../../visuals/VOLUME.md`](../../visuals/VOLUME.md).
- Particle and compute-to-render dataflow are already represented by
  [`PARTICLE_SYSTEM_DESIGN.md`](../active/PARTICLE_SYSTEM_DESIGN.md),
  [`SPLATTING_FRAME_PLAN_REQUIREMENTS.md`](../future/SPLATTING_FRAME_PLAN_REQUIREMENTS.md), and
  [`../../semantics/NONLINEAR_TRANSFORMS.md`](../../semantics/NONLINEAR_TRANSFORMS.md).

### Existing Example Pressure Tests

Several future directions already have good example coverage:

- vector fields: wind, CFD, tokamak, napari vectors;
- trajectories: animal migration, flight paths, tractography, HEP events, napari tracks;
- molecular rendering: protein arcball viewer;
- dense labels: napari label segmentation and categorical scales;
- dashboards: DAQ, market, DICOM, and image embedding LOD.

Future proposals should link to these examples rather than restating them.


## Roadmap Matrix

| Direction | Current coverage | Missing layer | Preferred next document |
|---|---|---|---|
| Graph/network | points, segments, paths | graph topology, layout, node/edge identity | [`GRAPH_NETWORK_DESIGN.md`](../future/GRAPH_NETWORK_DESIGN.md) |
| Unstructured grid / cell mesh | surface `mesh`, FEM surface example | tetra/hex/wedge cells, cell fields, cuts | [`UNSTRUCTURED_GRID_DESIGN.md`](../future/UNSTRUCTURED_GRID_DESIGN.md) |
| Vector field | sampled vector data, markers, paths, examples | field-level sampling, streamlines, probes | [`FIELD_VISUALIZATION_ROADMAP.md`](../future/FIELD_VISUALIZATION_ROADMAP.md) |
| Tensor field | weak adjacent coverage in splats/fields | tensor rank, eigen glyphs, invariants | [`FIELD_VISUALIZATION_ROADMAP.md`](../future/FIELD_VISUALIZATION_ROADMAP.md) |
| Categorical label volume | categorical scales, 2D labels, deferred volume note | 3D label volume semantics and picking | [`FIELD_VISUALIZATION_ROADMAP.md`](../future/FIELD_VISUALIZATION_ROADMAP.md) |
| Sparse voxel/grid | dense volume and resource update docs | sparse/bricked/page-resident fields | [`FIELD_VISUALIZATION_ROADMAP.md`](../future/FIELD_VISUALIZATION_ROADMAP.md) |
| Tracks/trajectories | `path`, particles, many examples | first-class time/identity track resource | [`DOMAIN_RESOURCE_ROADMAP.md`](../future/DOMAIN_RESOURCE_ROADMAP.md) |
| Ensemble/uncertainty | errorbar, boxplot, splat covariance | ensemble axis, probability/interval resources | [`DOMAIN_RESOURCE_ROADMAP.md`](../future/DOMAIN_RESOURCE_ROADMAP.md) |
| Molecular/structural biology | protein example, sphere/segment/mesh/volume | molecule semantic object and ids | [`DOMAIN_RESOURCE_ROADMAP.md`](../future/DOMAIN_RESOURCE_ROADMAP.md) |
| Out-of-core/progressive | dirty updates, LOD examples | page residency, partial-valid rendering | [`OUT_OF_CORE_PROGRESSIVE_DESIGN.md`](../future/OUT_OF_CORE_PROGRESSIVE_DESIGN.md) |


## Design Principle: Semantic Resources Lower To Existing Visuals

Most future scientific data models should not start as monolithic visual families.

Prefer:

```text
semantic resource -> derived render resources -> existing/future visual families
```

Examples:

- a graph lowers to node markers, edge segments/paths, labels, and selection overlays;
- an unstructured grid lowers to boundary meshes, cell-edge segments, cut meshes, and vector glyphs;
- a molecule lowers to atom spheres, bond segments, ribbon paths/meshes, surfaces, labels, and
  density volumes;
- a track table lowers to paths, current-position markers, fading trail segments, and event markers.

This keeps the public scene model meaningful while reusing the rendering and interaction machinery
already being built for v0.4.


## Design Principle: Python Owns Domain File Formats First

Many of these domains have large, fast-moving file ecosystems:

- graph: NetworkX, igraph, graph-tool, GraphML, GEXF, Matrix Market;
- grids: VTK/VTU, Exodus, XDMF, CGNS, meshio-supported formats;
- volumes: zarr, OME-Zarr, NIfTI, DICOM, TIFF pyramids;
- molecules: PDB, mmCIF, MMTF, MD trajectories;
- fields: xarray, netCDF, HDF5, zarr;
- ensembles: xarray/dask stacks and simulation-specific outputs.

Datoviz C should not try to own all of these readers. The likely split is:

- Python/GSP/application layer: file I/O, preprocessing, domain metadata, chunk selection, layout
  policy, solver-specific conversion;
- Datoviz C: retained resources, semantic ids, partial updates, GPU buffers/textures, interaction,
  picking payloads, render planning, telemetry, and efficient visual lowering.


## Design Principle: Compute Is An Implementation Capability, Not A Family

Compute is essential for future performance, but it should usually be a technique inside a resource
or visual path.

Good compute-backed uses:

- graph layout;
- streamline integration;
- tensor eigen decomposition for glyphs;
- particle and trajectory history updates;
- field decimation and envelope generation;
- isosurface or cut-surface extraction;
- sparse voxel brick compaction;
- out-of-core visibility and page selection;
- ensemble reductions such as mean, variance, percentile, and min/max.

The semantic API should remain stable when a CPU, precomputed, or WebGPU fallback is selected.


## Picking And Selection Direction

The active picking model already supports generic targets such as object, item, face, pixel, sample,
strip, segment, triangle, text, and annotation. Future scientific resources need richer semantic
identity mapping on top of that.

Likely future target vocabulary:

| Target | Example |
|---|---|
| `node` | graph node, skeleton joint |
| `edge` | graph edge, bond edge |
| `cell` / `element` | tetrahedron, hexahedron, FEM element |
| `voxel` | dense or sparse volume cell |
| `label` | segmentation region id |
| `track` | trajectory id |
| `atom` | molecular atom |
| `residue` / `chain` | molecular hierarchy |
| `member` | ensemble member |

This should be documented as semantic identity, not backend id encoding. Existing result structs can
evolve to carry resource ids, parent ids, link keys, voxel indices, barycentric coordinates, UVW,
physical coordinates, and tensor/vector metadata as needed.


## Example Strategy

For each future resource direction, prefer one concrete example that pressure-tests the core model:

| Direction | First useful example |
|---|---|
| Graph/network | force-directed graph with CPU/GPU layout switch |
| Unstructured grid | tetra/hex cell-mesh viewer with boundary, cut plane, and cell scalar |
| Vector field | CFD or wind field with arrows, streamlines, and probes |
| Tensor field | diffusion or stress tensor glyph viewer |
| Label volume | 3D atlas/segmentation volume with label table and picking |
| Sparse voxel/grid | sparse occupancy or AMR volume with chunk debug overlay |
| Tracks | generic time-aware trajectory layer fixture |
| Ensemble | ensemble trajectories or scalar field mean/variance viewer |
| Molecular | molecular dynamics trajectory or cryo-EM density plus protein fit |
| Out-of-core | progressive point cloud or chunked volume streaming example |


## Candidate Implementation Order

1. Add semantic roadmap/proposal docs for graph, unstructured grid, fields, domain resources, and
   out-of-core behavior.
2. Update existing visual/resource/picking docs with short cross-references and future target
   vocabulary.
3. Build example-first prototypes for graph and unstructured grid, because they expose genuinely
   missing semantic resource models.
4. Extend `SampledField`/`volume` planning for label volumes and sparse/bricked fields.
5. Extract only proven generic pieces into public API proposals.


## Open Questions

- Which future directions should become public C resources versus Python/GSP-level composites?
- Should graph, molecule, track, and ensemble resources be scene-owned objects or app-layer helper
  objects that emit ordinary visuals?
- How much semantic target vocabulary should be reserved in public enums before implementation?
- Should vector/tensor fields be represented as typed `SampledField` roles or new resource classes?
- Should sparse fields be a separate resource type or a capability tier of `SampledField`?
- How much out-of-core scheduling belongs in Datoviz C versus Python?
- Which compute algorithms should Datoviz provide directly, and which should remain user shaders?
