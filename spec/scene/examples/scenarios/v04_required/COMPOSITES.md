# v0.4 Required Composite Examples

Composite examples demonstrate semantic objects that lower to several coordinated visuals. They
belong in `examples/c/composites/`, not in `examples/c/visuals/`, unless a future API promotes a
true visual family.


## `composite_polygon`

Minimal target: one semantic polygon and one polygon set, with visible fill/stroke roles, one hole,
per-region colors, stroke width/join controls, and deterministic capture.

This should teach ring data, holes, region styling, and `dvz_panel_add_composite()`. Keep
geographic datasets, polygon booleans, and PSLG/constrained triangulation out of the minimal
example.


## `composite_graph_static`

Minimal target: a small deterministic graph with user-provided 2D layout, marker nodes, segment
edges, stable node/edge ids, and optional selected-node styling.

This should prove the public graph semantic object and graph composite without promising layout
algorithms. Layout is uploaded by user code.


## `composite_graph_edge_modes`

Pressure target: the same graph rendered in multiple panels using raw primitive lines, high-quality
segments, and path/Bezier curved edges.

This is the main API pressure test for edge-mode selection, Bezier tessellation, and edge-id mapping.
It may remain a lab or experimental example if Bezier/path edge support is not release-ready.
