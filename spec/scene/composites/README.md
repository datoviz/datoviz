# Scene Composites

Status: v0.4 implementation plan. Updated: 2026-06-02.

Composites are retained semantic scene objects that lower to coordinated built-in visuals. They are
not visual families and must not introduce parallel renderers, presentation paths, or Vulkan
ownership. The runtime path remains:

```text
semantic object -> composite view -> built-in visual roles -> scene frame plan -> DRP2
```


## Purpose

Use composites when a user-facing object has semantic identity that is larger than one visual:

1. polygons: fill, stroke, region ids, ring topology, and optional selection;
2. graphs: topology, node/edge ids, layout state, edge routing, labels, and selection;
3. future semantic objects that lower to several normal visuals.

Do not add a new visual family unless the renderable itself has a distinct resource schema, pass
participation, picking contract, or fallback behavior. Polygon and graph are semantic composites.


## Scientific Plotting Boundary

Scientific plotting support should be split between reusable Datoviz scene building blocks and the
future GSP/VisPy2 plotting layer. Datoviz owns rendering-native guide annotations, spans,
bars/intervals, bands/ribbons, and trace collections. GSP/VisPy2 owns statistical transforms,
high-level plotting grammar, Python data adaptation, and domain-specific recipes.

The durable boundary is recorded in
[`SCIENTIFIC_PLOTTING_BOUNDARY.md`](SCIENTIFIC_PLOTTING_BOUNDARY.md). Use that note before adding
histogram, `hline`/`vline`, error-band, or stacked-trace APIs.

The first-slice API and implementation order are tracked in
[`SCIENTIFIC_PLOTTING_IMPLEMENTATION.md`](SCIENTIFIC_PLOTTING_IMPLEMENTATION.md).


## Composite Model

`DvzComposite` should stay a narrow render-view bridge:

```text
DvzComposite
  scene
  type
  source semantic object
  source version seen
  role table
    role name
    generated visual
    z offset
    dirty flag
```

The role table must be generic. Polygon-specific fields such as `fill_dirty` and `stroke_dirty`
should be replaced by per-role dirty flags before graph support grows.

Public advanced access stays role-based:

```c
DvzVisual* dvz_composite_visual(DvzComposite* composite, const char* role);
```

Ordinary users attach the composite:

```c
dvz_panel_add_composite(panel, composite, NULL);
```


## Polygon Plan

Current baseline:

1. `DvzPolygonDesc` is the borrowed CPU input for one outer ring plus holes.
2. `dvz_triangulate_polygon()` produces ordinary `DvzGeometry`.
3. `DvzPolygon` and `DvzPolygons` retain semantic ring/style state.
4. `dvz_polygon_composite()` and `dvz_polygons_composite()` lower to `"fill"` and `"stroke"`.
5. Fill currently uses `mesh`; stroke currently uses `path`.

Implemented v0.4 polish:

1. add stable ids: `dvz_polygons_set_region_id()`, `dvz_polygons_set_region_ids()`, and
   `dvz_polygon_id()`;
2. add region visibility and bulk visibility helpers for polygon sets;
3. add bulk setters for fill colors, stroke colors, and stroke widths;
4. expose polygon stroke style helpers for caps, joins, and miter limit without requiring manual
   role-visual access;
5. preserve semantic region identity through generated visuals and query/link-key paths;
6. cache per-region triangulation and stroke spans when needed for large polygon sets;
7. keep fill/stroke lowering selectable in the future, but default to `mesh` fill plus `path`
   stroke for v0.4.

The first public composite example should be `examples/c/composites/polygon.c`: a few deterministic
regions, one hole, visible stroke joins, per-region colors, and role access only for an optional
advanced styling note.


## Bezier And Curve Plan

Curves are CPU-side geometry utilities, not a separate visual family. The first helper should
tessellate cubic Bezier controls into Datoviz-owned F64 polyline points plus optional subpath
lengths:

```c
int dvz_tessellate_bezier_cubic(
    const dvec3* controls, uint32_t curve_count, const DvzBezierTessellationDesc* desc,
    dvec3** out_points, uint32_t* out_point_count, uint32_t** out_lengths);
```

The output lowers to ordinary `dvz_path()` data. Future helpers may add quadratic Bezier,
Catmull-Rom, and B-spline. Graph curved edges, annotation arrows, and smooth path examples should
all consume this shared utility.


## Graph Plan

Graphs should become v0.4 API as semantic resources with render views, not monolithic visuals.

```text
DvzGraph       = topology, node/edge attributes, stable ids, layout buffers
DvzGraphView   = panel/render style, label budget, edge mode, selection overlays
DvzGraphLayout = optional producer of node positions
```

Minimal public graph data:

```text
node_count
edge_count
edge_src[edge_count]
edge_dst[edge_count]
node_position[node_count]
node_id[node_count] optional
edge_id[edge_count] optional
node color/size/visibility
edge color/width/visibility
```

Recommended first API shape:

```c
DvzGraph* graph = dvz_graph(scene, 0);
dvz_graph_set_node_count(graph, node_count);
dvz_graph_node_positions(graph, 0, node_count, positions);
dvz_graph_set_edge_count(graph, edge_count);
dvz_graph_edges(graph, 0, edge_count, endpoints); // packed source,target pairs
dvz_graph_node_ids(graph, 0, node_count, node_ids);
dvz_graph_edge_ids(graph, 0, edge_count, edge_ids);

DvzGraphEdgeStyle edge_style = dvz_graph_edge_style();
edge_style.mode = DVZ_GRAPH_EDGE_MODE_SEGMENT;
dvz_graph_set_edge_style(graph, &edge_style);
DvzComposite* composite = dvz_graph_composite(graph, 0);
```

Initial role lowering:

| Role | Lowering |
| --- | --- |
| `"nodes"` | `marker` by default, optional `point`, `pixel`, or `sphere` |
| `"edges"` | `segment` by default, optional `primitive` or `path` |
| `"labels"` | semantic text/glyph when label data and budget are enabled |
| `"selection"` | generated highlight visuals when selection policy lands |

Edge mode policy:

1. `primitive` line list: fastest, aliased, best for large static previews;
2. `segment`: high-quality straight screen-space strokes;
3. `path`: routed, bundled, or Bezier-curved edges with span-to-edge-id mapping.

Layout policy:

1. `USER`: supplied positions, required first;
2. `CPU_STATIC`: Datoviz reference layout for small deterministic examples;
3. `CPU_DYNAMIC`: incremental layout, optional and bounded;
4. `GPU_COMPUTE`: compute writes the same node-position buffer consumed by rendering;
5. `EXTERNAL_PROVIDER`: plugin/provider writes positions or routed edge geometry.

Dynamic graph rendering should eventually avoid CPU-expanded edge endpoints every frame. The future
fast path is shader endpoint lookup from `edge_src`, `edge_dst`, and `node_position` buffers. Until
that lands, CPU-expanded segment/path data is the correctness path.


## Plugin Boundary

Graph layout plugins should be data/layout producers, not renderers. They may provide:

1. CPU layout callbacks that write node positions;
2. prepared external layouts imported by user code;
3. DRP2-compatible compute descriptors that write node-position buffers;
4. optional routed-edge path geometry.

Plugins must not submit Vulkan commands, own Datoviz command buffers, or bypass scene -> DRP2.
Advanced algorithms such as Graphviz-style layout, ForceAtlas2, cuGraph, tree layout, community
detection, and edge bundling should remain external until concrete examples prove a narrow core
need.


## Example Commitments

Add a public composite lane:

```text
examples/c/composites/
```

v0.4 targets:

1. `composite_polygon`: `examples/c/composites/polygon.c`;
2. `composite_graph`: `examples/c/composites/graph.c`.

Keep compute-driven graph layout in `examples/c/lab/` until the compute-to-render buffer contract is
release-proven.


## Implementation Order

1. Generalize `DvzComposite` internals to role-based dirty flags.
2. Polish polygon ids, bulk style setters, visibility, stroke style, semantic mapping, and tests.
3. Add the polygon composite example and manifest/example-planning entries.
4. Add Bezier tessellation helpers and tests.
5. Add `DvzGraph`, `DvzGraphEdgeStyle`, graph composite roles, and user-layout rendering.
6. Add graph examples for static layout and edge modes.
7. Add semantic graph picking/selection only after node/edge id mapping is stable.
