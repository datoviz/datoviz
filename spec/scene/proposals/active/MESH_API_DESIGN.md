> **Execution Status**
> - **Status:** `PARTIALLY IMPLEMENTED SCENE SPEC PROPOSAL`
> - **Updated on:** `2026-05-20`
> - **Purpose:** preserve remaining mesh API/resource ownership decisions after the first indexed
>   mesh and instancing slices landed.

# Mesh API Design

## Decision Addressed

The active `mesh` visual must keep geometry sharing, visual styling, instancing, picking, and
runtime updates separate.

The remaining proposal question is the public API shape around explicit scene-owned mesh resources
and instance data, not whether the mesh family exists.


## Short Summary

The first retained mesh path is implemented through `dvz_mesh()`, retained vertex attributes,
optional scene-owned index buffers, lit material state, depth-tested rendering, and
`instance_transform` instanced draws. Future API work should refine that path into an explicit
resource model rather than returning to a v0.3-style batch/request layer.


## Chosen Direction

| Layer | Owner | Contract |
|---|---|---|
| `DvzGeometry` | Caller/geom module | CPU-side generation, import, preprocessing, and staging. |
| Mesh resource | Scene | Uploaded vertex/index data, format availability, dirty ranges, stable primitive ordering, sharing. |
| Mesh visual | Scene | Material, transform, visibility, alpha/render mode, picking policy, panel attachment. |
| Mesh instance table | Visual or visual-owned resource | Per-instance model transform, stable instance id, and later narrow overrides. |
| Texture/sampler resources | Scene | Referenced by material/visual bindings, not baked into mesh resource identity by default. |

Core choices:

1. mesh visuals should reference scene mesh resources instead of privately owning geometry by
   default;
2. resource updates target mesh resources, while material and transform updates target visuals;
3. one mesh visual may render zero, one, or many instances of the same resource;
4. material state belongs to the visual, so the same geometry can appear with different materials;
5. face-level picking depends on stable resource primitive ordering and instance-aware result
   routing;
6. transparency mode belongs to the visual/material side and lowers through the shared render
   contract.


## Canonical Migration Links

The authoritative mesh rules now live in:

1. [Visual Family: mesh](../../visuals/MESH.md) for implemented attributes, parameters, resource
   expectations, instancing, and mesh semantics;
2. [Mesh Shading Design](MESH_SHADING_DESIGN.md) and
   [Material Lighting API](MATERIAL_LIGHTING_API.md) for remaining lighting/material staging;
3. [Geometry Utilities](../../semantics/GEOMETRY_UTILITIES.md) for CPU geometry generation and
   preprocessing;
4. [Transform Pipeline](../../pipeline/TRANSFORM_PIPELINE.md) for local/object, model, view,
   projection, and upload-space rules;
5. [Transparency](../../semantics/TRANSPARENCY.md) and
   [Render Contract Resolver](RENDER_CONTRACT_RESOLVER.md) for alpha/depth/pass behavior;
6. [Picking](../../interaction/PICKING.md) and [Selection](../../interaction/SELECTION.md) for
   interaction payloads.

Do not restate vertex attribute tables, generic scene lifecycle rules, or transparency/depth
policy in this proposal.


## Remaining Unresolved Points

1. Final public C names for mesh resource creation, upload, replacement, partial updates, and
   visual binding.
2. Whether instance tables are visual-owned data or separate scene resources.
3. Exact mesh picking payload for object, instance, face, and optional hit position.
4. Texture/material binding API once textured mesh variants are enabled.
5. Serialization shape for shared mesh resources and visual references.
6. Capability diagnostics for unsupported instancing, transparency modes, or material variants.
7. Narrow first public slice: resource upload from `DvzGeometry`, vertex/index subrange updates,
   one-instance and multi-instance draws, classic-lit material state, opaque mode first, and
   face-picking-ready ordering.
