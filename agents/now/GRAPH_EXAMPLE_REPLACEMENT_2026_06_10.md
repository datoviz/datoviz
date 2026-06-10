# Graph Example Replacement Decision

Status: ready for implementation.

Scope: replace `examples/c/composites/graph.c` content only. Do not introduce or promote a new
public graph API in this batch.


## Decision

Use a deterministic small brain-connectivity graph.

The example should model cortical and subcortical regions as nodes, weighted anatomical or
functional links as edges, and three labeled communities:

1. `Visual` regions;
2. `Motor` regions;
3. `Memory` regions.

Keep all node metadata, edge metadata, styling, and helper routines local to
`examples/c/composites/graph.c`. The existing graph composite API may be used as the rendering
surface because it is already part of the repository, but this batch must not add a new public
`DvzGraph` resource/model API.


## Implementation Shape

Use fixed arrays for:

1. node labels, positions, community ids, semantic ids, and node strengths;
2. edge source/target indices, weights, and bridge flags;
3. community label placement and colors.

Render nodes and weighted Bezier edges through the existing graph composite. Add text labels for
the three communities and a small title label so the example reads as scientific content instead
of a generic social graph.


## Validation

Before committing the implementation:

```sh
git diff --check
just build
just test graph
./build/examples/c/composites/graph --png
```
