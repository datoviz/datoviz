# Example Selection By Capability

Use `docs/examples/capabilities.json` when a user asks for a visual, feature, data kind, or domain
tag and the best starting example is not obvious by filename.


## Lookup Order

Search capability groups in this order:

1. `visuals` for requests such as point, marker, image, mesh, text, labels, sphere, or volume;
2. `features` for layout, controllers, picking, updates, colorbars, scale bars, lighting, or
   blending;
3. `composites` for semantic objects such as polygon and graph;
4. `tags` for workflows, domains, techniques, interaction modes, or real-data labels;
5. `data_kinds` when the user specifies synthetic, simulated, prepared, or real data.

Each capability entry lists examples with `id`, `title`, `category`, `status`, `agent_copy_safe`,
`source`, and `page`. After finding candidates, open `docs/examples/examples.json` for build,
smoke, validation, dataset, and summary details.


## Selection Rules

Prefer candidates in this order:

1. `agent_copy_safe: true`;
2. `status: supported`;
3. atomic categories: `visual`, `feature`, then `composite`;
4. showcase examples only for composed workflows, real-data stories, or polished demos.

When no copy-safe candidate exists, say so and use the example as reference material instead of
copying it directly.


## Common Routes

| Request | Capability path |
| --- | --- |
| Scatter or point cloud basics | `visuals.point`, then `tags.dense-point-cloud` if needed |
| Image with color scale | `visuals.image`, `features.colorbar`, `features.colormap-scale` |
| Axes and labels | `features.axes-2d`, `features.axis-labels` |
| Pan and zoom | `features.controller-panzoom`, `tags.panzoom` |
| 3D mesh interaction | `visuals.mesh`, `features.controller-arcball`, `features.mesh-texture` |
| Picking or probing | `features.pick-point`, `features.pick-hover`, `features.image-probe` |
| Real or prepared datasets | `data_kinds.real`, `data_kinds.prepared`, then inspect `dataset` |
| Scientific showcase | `tags.scientific`, then prefer the matching data kind |


## Guardrails

Do not treat tag coverage as API support by itself. Confirm support with the example's `status`,
documentation page, and validation command.

Do not turn a showcase into a minimal template unless the user explicitly needs that workflow.
Showcases may include dataset loading, GUI controls, capture setup, or experimental runtime paths
that are inappropriate for small generated snippets.
