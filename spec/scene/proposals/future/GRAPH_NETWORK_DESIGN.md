# Graph And Network Design

> **Status:** exploratory future scene-resource proposal.
> **Primary gap:** Datoviz has points, segments, and paths, but no semantic graph resource with
> node/edge identity, layout state, or graph-specific picking.


## Summary

Graphs should be modeled as semantic resources with one or more render views, not as one monolithic
visual. A graph combines topology, node and edge attributes, optional layout state, and interaction
identity. Rendering can lower to existing or future visuals: markers or spheres for nodes, segments
or paths for edges, glyphs for labels, and overlays for selection or communities.

Do not promote a public `DvzGraph` API immediately. The first implementation should use
example-local C structs and prepared/Python-generated data to stress the scene and DRP2 paths. If
the pattern proves reusable across several examples, promote the model first to internal scene
resources and only later to public C handles.


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

The first C stress example still needs concrete C structs to represent graph data, but those structs
should be example-local rather than public API. A minimal example-local shape is:

```c
typedef struct DvzGraphData
{
    uint32_t node_count;
    uint32_t edge_count;

    vec3* node_pos;
    uint32_t* edge_src;
    uint32_t* edge_dst;

    DvzColor* node_color;
    float* node_size;
    float* edge_weight;
    DvzColor* edge_color;
} DvzGraphData;
```

Potential promotion path:

```text
phase 1: example-local C structs + Python/prepared graph data
phase 2: internal scene/resource structs such as DvzGraphResource, DvzGraphView, DvzGraphLayout
phase 3: public DvzGraph only if repeated examples need a stable C API
```

This keeps the first implementation honest about C-side memory layout while avoiding premature API
commitment.


## Resource And View Split

Separate graph data from panel-specific rendering.

```text
DvzGraph       = topology, node/edge attributes, layout buffers, stable ids
DvzGraphView   = how one panel renders the graph
DvzGraphLayout = optional producer of node positions
```

One graph may have several views:

- a full node-link overview;
- a filtered detail view;
- a 2D layout and a 3D layout;
- an adjacency matrix view;
- a community-only summary;
- a selected-neighborhood view.

The graph resource should not own panel cameras, label budgets, hover state, or rendering style that
is specific to one view.


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

Static layout should be the first user-facing milestone. Given node positions and edge topology,
Datoviz should render the graph efficiently before dynamic layout is attempted.

Recommended milestones:

1. render nodes and straight edges from uploaded positions/topology;
2. pick nodes and highlight incident edges;
3. update node positions from CPU without rebuilding topology;
4. add optional GPU compute layout that writes the same node-position buffer;
5. add pin/drag interaction and layout telemetry.

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

Curved, routed, or bundled edges should lower to `path` rather than a graph-only renderer. The
generated path spans must preserve a mapping back to graph edge ids:

```text
graph edge id -> path span id
path span id  -> graph edge id
```

Edge routing and bundling can be CPU-generated, Python-generated, or compute-generated later. Once
the geometry exists, ordinary path rendering and path picking/selection rules should apply.


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
[`SPLATTING_FRAME_PLAN_REQUIREMENTS.md`](../future/SPLATTING_FRAME_PLAN_REQUIREMENTS.md) and
[`PARTICLE_SYSTEM_DESIGN.md`](../active/PARTICLE_SYSTEM_DESIGN.md).

Datoviz should ship at most a simple reference layout initially, not a full graph-layout library.
The important engine contract is that node-position buffers may be produced externally, updated by
CPU, or written by compute, and then consumed directly by graph render views. Advanced algorithms
such as Graphviz-style layout, ForceAtlas2 variants, community layout, or cuGraph-style processing
can remain application/Python-provided.


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
- static CPU/precomputed layout path first;
- optional GPU force-directed layout later;
- pause/resume layout;
- pin and drag nodes;
- color by community or degree;
- node/edge picking;
- FPS, layout iterations, energy, and max-displacement telemetry.

Recommended first size target:

```text
first slice:   <= 1,000 nodes, <= 5,000 edges
medium target: <= 10,000 nodes, <= 100,000 edges
future stress: >= 100,000 nodes, >= 1,000,000 edges
```

The first slice should prioritize semantic correctness, stable ids, picking, and efficient straight
edge rendering. Larger graph support will require LOD, label budgets, edge thinning, community
aggregation, culling, and possibly compute-generated visibility or draw arguments.

Useful telemetry:

- node count;
- edge count;
- draw count;
- uploaded bytes;
- layout iterations per frame;
- layout time;
- max displacement;
- visible labels;
- picked node or edge.


## Adjacency Matrix View

Node-link rendering should not be the only long-term graph representation. Dense graphs often read
better as an adjacency matrix.

An adjacency matrix view could lower to:

- `image` or `pixel` cells;
- row/column labels;
- linked selection with the node-link view;
- categorical or scalar edge weights mapped through scales.

This is not a first implementation requirement, but the graph resource/view split should not assume
that every graph view is node-link geometry.


## Open Questions

- Which example-local C struct layout best matches eventual DRP2 buffers without overfitting the
  public API?
- Which attributes are common enough to deserve graph-specific convenience setters if `DvzGraph`
  becomes public later?
- Should a built-in reference force layout be example-only, internal, or exposed as an optional
  `DvzGraphLayout` object?
- What is the smallest useful edge-picking path: CPU nearest-edge fallback, GPU ID pass, or
  both?
- How should adjacency matrix views share selection and ordering with node-link views?
