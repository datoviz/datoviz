# Public API Conventions

Status: normative v0.4 API design guidance.

This document defines cross-module rules for the public C API. It is intentionally short: when a
feature-specific spec needs more detail, it should link here and add only the local rules required by
that feature.


## Goals

1. Keep public names, ownership, and mutation patterns predictable across modules.
2. Keep the C API practical for generated bindings, including future WASM bindings.
3. Preserve implementation freedom by hiding backend, C++, and third-party-library types.
4. Make the v0.4 API consistency pass auditable before release.


## Naming

Public functions use `dvz_<object>_<action>()` for object-level actions.

When a public object has subroles, role/property setters use:

```text
dvz_<object>_<role>_<property>()
```

Examples:

```text
dvz_polygon_fill_color()
dvz_polygon_stroke_width()
dvz_graph_nodes_size()
dvz_graph_edges_color()
```

Backend or third-party names should not appear in high-level user APIs unless the user is selecting
an explicit backend policy such as a triangulation backend.


## Visuals, Semantic Objects, And Composites

`DvzVisual` is a low-level render leaf: point, marker, segment, path, mesh, image, volume, and
similar visual families.

Higher-level semantic objects own domain state and expose typed APIs. Examples include polygon
regions, graph topology, axes, colorbars, annotations, and orientation gizmos. They should not expose
their implementation visuals as the primary mutation surface.

`DvzComposite` is the renderable bridge between semantic objects and panel-attached visuals. A
composite is a scene-owned renderable view over one semantic object and may own or derive several
coordinated leaf visuals internally.

Typed semantic object APIs remain the normal user path:

```text
dvz_polygon_set_geometry()
dvz_polygon_fill_color()
dvz_graph_nodes_size()
dvz_graph_edges_color()
```

Composites provide the generic panel attachment path:

```text
dvz_panel_add_visual()
dvz_panel_add_composite()
```

Composites may expose generated leaf visuals by stable role names for advanced users, testing, and
integration code. The leaf visual roles are implementation-facing extension points, not a substitute
for the typed semantic API.


## Structs Versus Setters

Use descriptor structs when the data is naturally a coherent record:

1. construction-time configuration,
2. resource descriptors,
3. bulk or atomic operation descriptors,
4. result payloads,
5. records whose fields are normally authored together.

Use flat typed setters when fields are independent, frequently changed, or binding-facing:

1. style and appearance properties,
2. visibility and enable flags,
3. interaction options,
4. role-specific composite state.

Avoid nested public style structs as the primary user path. Optional convenience structs are allowed
when they reduce real C call-site noise, but they should not be the only way to set common style
state.


## Public Struct Rules

Public structs should be zero-initializable unless explicitly documented otherwise. Zero values must
either mean a documented default or a documented disabled/empty value.

Pointer-passed descriptor structs should document ownership, defaults, and which fields are ignored
or required for each mode.

Structs likely to grow after v0.4 should have a compatibility strategy before API freeze. Acceptable
strategies include reserved fields, a documented version/size field, or keeping the struct out of the
stable public surface until it settles.

Public structs must not expose Vulkan handles, DRP2 runtime object ids, command buffers, atlas
pages, C++ standard-library types, or third-party-library structs.


## Binding And WASM Constraints

Generated bindings and future WASM support are first-class API constraints.

Prefer APIs built from:

1. scalar values,
2. enums,
3. opaque handles,
4. pointer-plus-count arrays,
5. flat data records with simple field types.

Avoid primary APIs that require nested structs, callback-heavy setup, language-specific ownership
conventions, or exposing temporary C pointers whose lifetime is hard to express in bindings.


## Data APIs

Bulk data setters should use explicit pointer-plus-count signatures. Ownership must be documented as
one of:

1. copied before return,
2. borrowed until return,
3. retained by the callee until an explicit replacement or destroy operation.

Large mutable data should support range updates where practical. Style setters should not implicitly
replace bulk data, and bulk data setters should not implicitly change style policy except where a
descriptor explicitly says so.


## Examples

Preferred:

```c
dvz_polygon_stroke_width(polygon, 1.0f);
dvz_graph_nodes_color(graph, color);
dvz_triangulate_polygon(source, &desc);
```

Avoid as the primary API when it only wraps independent style properties:

```c
dvz_polygon_style(polygon, &(DvzPolygonStyle){...});
```

The optional convenience form may still exist if the flat setters remain available and documented.
