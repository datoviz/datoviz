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


## `composite_graph`

Minimal target: one deterministic clustered graph with user-provided 2D layout, marker nodes,
Bezier path edges, stable node/edge ids, per-item styles, and explicit control points on selected
inter-cluster edges.

This should prove the public graph semantic object and graph composite without promising layout
algorithms. Layout is uploaded by user code. Edge-mode comparisons belong in tests or lab examples,
not in the public composite gallery.
