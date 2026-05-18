# Graph And Network Design

> **Status:** exploratory future scene-resource proposal.
> **Primary gap:** Datoviz has points, segments, and paths, but no semantic graph resource with
> node/edge identity, layout state, or graph-specific picking.


## Summary

Graphs should be modeled as semantic resources with one or more render views, not as one monolithic
visual. A graph combines topology, node and edge attributes, optional layout state, and interaction
identity. Rendering can lower to existing or future visuals: markers or spheres for nodes, segments
or paths for edges, glyphs for labels, and overlays for selection or communities.


## Simple Examples

- brain connectivity graph: anatomical regions as nodes, weighted edges as connectivity;
- protein interaction network;
- cell lineage tree;
- dependency graph;
- road or transport network;
- knowledge graph or causal graph;
- simulation mesh adjacency graph;
- real-time network topology dashboard.


## Core Data Model

Candidate scene resource:

```text
DvzGraph
  node_count
  edge_count
  edge_src[edge_count]       uint32
  edge_dst[edge_count]       uint32
  node_position[node_count]  vec2 or vec3
  node_id[node_count]        stable semantic ids, optional
  edge_id[edge_count]        stable semantic ids, optional
  node attributes            color, size, group, label, pinned, weight, visibility
  edge attributes            color, width, weight, direction, label, visibility
```

The graph resource should own topology and stable semantic ids. A graph view should own rendering
style, selected layout, label budget, and panel-specific interaction state.


## Views

Recommended first render views:

| View | Lowering |
|---|---|
| nodes | `point`, `marker`, or `sphere` |
| straight edges | `segment` |
| bent or bundled edges | `path` |
| arrowheads | marker/primitive instances near edge endpoints |
| labels | `glyph` |
| communities | translucent hulls, meshes, or image-like overlays |
| selection | highlight overlays using node/edge ids |

This split keeps graph visualization composable and avoids making every graph style a separate
visual family.


## Layout

Layout should be separable from rendering.

Candidate layout modes:

1. external/precomputed layout uploaded from Python or application code;
2. CPU incremental layout for small graphs;
3. GPU force-directed layout using compute;
4. hierarchical, radial, circular, or tree layout supplied by application code;
5. graph embedding or community layout imported from external tools.

GPU layout should update a persistent `node_position` buffer. Render views should consume that
buffer directly without CPU readback.

One-frame GPU force layout shape:

```text
Compute: clear/accumulate node forces
Compute: edge attraction from edge_src/edge_dst
Compute: repulsion or grid/Barnes-Hut approximation
Compute: integrate position and velocity
Render:  edges read node_position
Render:  nodes read node_position
Render:  labels and selection overlays
```


## Efficient Edge Rendering

For dynamic layouts, avoid CPU-expanding edge endpoint buffers every frame.

Preferred shader-driven edge path:

```text
vertex_index -> edge_id = vertex_index / 2
side         -> vertex_index % 2
node_id      = side == 0 ? edge_src[edge_id] : edge_dst[edge_id]
position     = node_position[node_id]
```

This needs render shaders that can read graph topology and node-position buffers as storage or
vertex-accessible resources. If the active backend cannot support this path, a CPU-expanded segment
buffer remains a correctness fallback.


## Picking And Selection

Graph picking should resolve semantic graph ids, not only visual item ids.

Recommended payloads:

```text
node pick:
  graph id
  node index
  semantic node id
  position
  attributes or link key

edge pick:
  graph id
  edge index
  semantic edge id
  src node id
  dst node id
  nearest point on edge
  edge parameter t in [0, 1]
```

Selections should support:

- node selection;
- edge selection;
- selected node plus incident-edge highlight;
- selected edge plus endpoint-node highlight;
- group/community selection;
- linked selection across graph and non-graph panels.


## Compute And DRP2 Requirements

Useful future DRP2/frame-plan capabilities:

- persistent storage buffers for node positions, velocities, forces, and topology;
- compute passes with read/write buffer declarations;
- compute-to-render barriers for buffers read as vertex/storage inputs;
- direct and indirect draw support;
- optional GPU reductions for layout energy or max displacement;
- optional GPU sorting/binning for large-graph LOD.

These overlap with the generic compute requirements in
[`SPLATTING_FRAME_PLAN_REQUIREMENTS.md`](SPLATTING_FRAME_PLAN_REQUIREMENTS.md) and
[`PARTICLE_SYSTEM_DESIGN.md`](PARTICLE_SYSTEM_DESIGN.md).


## WebGPU Considerations

The baseline graph view should not require native multi-draw indirect. WebGPU-friendly paths are:

- nodes as one instanced or direct draw;
- edges as one direct draw with shader endpoint lookup;
- compute layout with storage buffers;
- CPU or precomputed layout fallback when compute is unavailable.


## Example Plan

Add a future example such as:

```text
spec/scene/examples/compute/GRAPH_FORCE_LAYOUT.md
```

Suggested features:

- synthetic presets: tree, grid, random geometric, scale-free, community graph;
- CPU/precomputed layout path;
- optional GPU force-directed layout;
- pause/resume layout;
- pin and drag nodes;
- color by community or degree;
- node/edge picking;
- FPS, layout iterations, energy, and max-displacement telemetry.


## Open Questions

- Should `DvzGraph` be a public C scene object or a higher-level Python/GSP helper first?
- Should node/edge attributes use generic item tables or graph-specific setters?
- Should graph layout algorithms ship with Datoviz or remain user-provided compute shaders?
- How should edge bundling and curved edges share infrastructure with `path`?
- Which graph sizes should the first implementation target?
