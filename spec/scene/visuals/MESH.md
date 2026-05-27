# Visual Family: `mesh`

## Status

Normative target contract for the `mesh` visual family. Shared attribute and behavioral definitions
live in `SHARED_ATTRIBUTES.md`; general visual rules live in `../semantics/VISUAL_FAMILIES.md`,
`../semantics/VISUAL_FAMILY_RULES.md`, `../pipeline/ATTRIBUTE_SOURCES.md`, and
`../semantics/VISUAL_CONTRACT.md`.

Status on 2026-05-17: the active v0.4 runtime implements retained mesh construction, triangle-list
rendering through scene -> FramePlan -> DRP2 -> vklite, `position`, optional `color`, optional
`normal`, scene-owned `"index"` buffers, `instance_transform`, depth-tested rendering, arcball/
camera transforms, WBOIT/depth-peeling participation, SSAO G-buffer participation when normals are
present, and offscreen/app execution coverage.

Update on 2026-05-24: CPU-side `geom` helpers can derive unique mesh edges and unstitched scalar
contour segments. The first live overlay example lowers those derived outputs to `segment` visuals;
this does not make edge overlay or isolines native mesh runtime features yet.

Update on 2026-05-27: retained textured mesh is required for the v0.4 release example set, not a
later workaroundable feature. The minimum slice is `texcoords`, a 2D sampled-field or texture
resource bound by mesh visuals, a `color_mode = texture` shader/pipeline variant, texture sampling
combined with the active lighting/material path, retained texture replacement or layer switching,
sampler defaults, deterministic tests, and one C terrain or planet-surface example. Baked vertex
colors are useful fallbacks for other examples, but they do not satisfy the textured-mesh release
requirement.

Scalar colormap mode, automatic normal generation, edge overlay, isolines, shape-builder
integration, and face/region picking remain target capabilities unless marked above as implemented.

## Purpose

`mesh` renders indexed triangle surface geometry with optional lighting, texture mapping, edge
overlay, isolines, and instancing. Each mesh item is one vertex; triangles are defined by a flat
index buffer with three indices per triangle.

Typical uses: brain surfaces, terrain, anatomical models, procedural geometry, height fields,
isosurfaces, and polyhedral shapes.

Volumetric cell meshes for FEM/CFD/finite-volume data are a distinct future model; see
[`../proposals/future/UNSTRUCTURED_GRID_DESIGN.md`](../proposals/future/UNSTRUCTURED_GRID_DESIGN.md).

## Geometry Resource

The visual references a scene-owned mesh geometry resource containing:

- vertex attributes;
- optional index buffer;
- dirty ranges for partial vertex/index updates;
- stable resource identity for sharing across visuals and panels.

The visual owns material, transform, visibility, picking policy, stage participation, and instance
state. `instance_transform` renders the same vertex/index payload once per transform using an
instanced draw. Convenience APIs may upload geometry during construction, but semantically they
create or replace a scene mesh resource.

## Surface Convenience Contract

A surface plot is a structured-grid mesh convenience, not a separate visual family. `geom` or a
surface helper may generate a `DvzGeometry` payload from row/column counts, heights, optional
scalar/color data, origin, and grid basis vectors. The scene then uploads or updates an ordinary
mesh resource.

Structured-grid provenance may be retained for height-only updates, normal recomputation, row/
column edge overlays, contours, scalar isolines, and future LOD. Rendering still uses indexed
triangles.

## Per-Vertex Attributes

| Attribute | Type | Sources | Required | Notes |
|---|---|---|---|---|
| `position` | `vec3` | `PER_ITEM` | yes | visual-space vertex position |
| `color` | shared color | `CONSTANT`, `PER_ITEM` | mode-dependent | used for `color_mode = rgba` or `scalar`; ignored for texture mode |
| `normal` | `vec3` unit normal | `CONSTANT`, `PER_ITEM` | no | used for Phong; auto-compute flat normals when absent |
| `texcoords` | `vec2` in `[0, 1]` | `PER_ITEM` | texture mode only | UV coordinates for texture sampling |
| `isoline_value` | `float32` | `PER_ITEM` | isolines only | scalar used for contour placement |

When `color_mode = scalar` and `lighting = phong`, the colormap-derived color feeds the diffuse
term. `lighting = flat` ignores normals.

## Index Buffer

| Property | Value |
|---|---|
| Type | flat `uint32` array |
| Topology | triangle list only |
| Count rule | multiple of 3 |
| Mutability | `dynamic` |

Invalid indices are validation errors. Replacing the index buffer updates all visuals referencing
the resource on the next validated frame plan.

## Visual Parameters

| Parameter | Type | Default | Applies to | Notes |
|---|---|---|---|---|
| `texture` | 2D `SampledField`, RGBA `u8` | none | `color_mode = texture` | v0.4 required capability; mesh texture slot, not image-only setter |
| `colormap` | `Scale` kind `color` | none | `color_mode = scalar` | maps per-vertex scalar to color |
| `backface_culling` | `bool` | `true` | all | set `false` for open/thin surfaces |
| lighting parameters | shared | shared defaults | `lighting = phong` | see `SHARED_ATTRIBUTES.md` |
| `edgecolor` | `rgba_u8` | transparent | edge overlay | transparent disables edges |
| `linewidth` | `float32` pixels | `1.0` | edge overlay | used when edges enabled |
| `isoline_count` | `uint32` | `0` | isolines | requires `isoline_value` |
| `isoline_range` | `vec2` | colormap domain or data range | isolines | explicit value overrides derived range |
| `isoline_color` | `rgba_u8` or color `Scale` | black | isolines | uniform color or mapped by level |

## Edge Overlay

Mesh wireframe is planned, not part of the active first-slice runtime. The public edge overlay
path should be derived geometry:

1. derive a unique edge list from triangle indices;
2. preserve stable edge ids when the mesh resource is stable;
3. render edges as camera-facing ribbons or segment impostors in an overlay pass;
4. use analytic fragment coverage for antialiasing;
5. depth-test against the mesh depth buffer with bias when needed;
6. expose color, width, opacity, boundary-only mode, and feature-edge mode as mesh parameters.

Vulkan polygon line mode and permanent barycentric wireframe shaders are diagnostic or fallback
tools, not the default public path.

The first implementation path should reuse `segment` or a segment-like derived overlay for
wireframe rendering. Joins are not required for general mesh wireframes because mesh vertices do not
define ordered polylines; boundary loops and feature-edge chains may lower to `path` later when an
ordered chain is available.

## Isolines

`isoline_count > 0` draws evenly spaced levels within `isoline_range`. The range defaults to the
attached colormap scale domain when `color_mode = scalar`; otherwise it is derived from
`isoline_value` data.

Uniform isoline color uses one `rgba_u8`; mapped isoline color uses a separate color `Scale` keyed
by level value. The face-color colormap and isoline-color scale are independent.

Prefer a dedicated mesh-isoline or derived-overlay path once scalar surface fields are retained in
the mesh resource. Do not add isoline payloads to the baseline vertex layout unless the selected
shader variant needs them.

CPU-extracted contour segments should lower to `segment` first. Once contour stitching exists,
connected open and closed contour lines should lower to `path` spans so joins, caps, and per-level
styling follow the path contract. A shader-only isoline variant remains useful for dense visual
overlays, but it should be treated separately from exportable or pickable contour geometry.

## Shape Builders

Shape builders generate vertex/index data for common geometries and upload them to a mesh resource:
`square`, `disc`, `sphere`, `cube`, `cylinder`, `cone`, `torus`, `surface`, and Platonic solids.
They are CPU-side conveniences, not a separate visual family. Builders may support per-shape
transforms and merging multiple shapes into one mesh resource.

## Defaults And Missing Values

| Field | Default | Missing-value policy | `DvzStyle` override |
|---|---|---|---|
| `position` | required | NaN/Inf invalidates affected primitive or fails strict validation | no |
| `index` | optional for non-indexed draws | invalid index is validation error | no |
| `color`, `normal`, `texcoords` | mode-dependent defaults | scalar NaN uses scale missing color | yes |
| edge overlay fields | disabled unless enabled | invalid width/color falls back only when configured | yes |
| lighting parameters | shared defaults | NaN falls back to family default | yes |

## Variant Axes

| Axis | Values | Default |
|---|---|---|
| `color_mode` | `rgba`, `scalar`, `texture` | `rgba` |
| `lighting` | `phong`, `flat` | `phong` |

Variants are set at visual creation time. `color_mode = texture` requires `texcoords` and a
texture resource. `color_mode = scalar` requires a color scale and scalar values.

## Transform, Stages, And Picking

Transform model and stage participation are standard; see `SHARED_ATTRIBUTES.md`.

Picking returns face identity, not vertex identity. For indexed geometry, face id is the index
buffer triplet index. For non-indexed triangles, face id is `vertex_index / 3`.

Status on 2026-05-26: the active GPU-backed request path includes item-level mesh picking through
the rendered mesh target. Mesh face/region identity remains deferred.

## Related Families

| Situation | Preferred family |
|---|---|
| Per-sphere attributes | `sphere` |
| 2D filled polygons without depth | `primitive` with `triangle_list` |
| Volume rendering | `volume` |
| Point cloud | `point` or `pixel` |

## Required Cases

1. brain surface with vertex-colored regions;
2. terrain with satellite texture; this is the v0.4 retained textured-mesh proof case;
3. activity-colored cortical surface using scalar colormap;
4. flat-shaded polyhedral mesh;
5. wireframe/edge overlay;
6. isoline contours on a height field;
7. procedural sphere from shape builder;
8. deferred data upload into an initially empty visual.

## v0.3 Correspondence

| v0.3 | v0.4 |
|---|---|
| `dvz_mesh_position` | `position` `PER_ITEM` |
| `dvz_mesh_color` | `color` `PER_ITEM` |
| `dvz_mesh_texcoords` | `texcoords` `PER_ITEM` |
| `dvz_mesh_normal` | `normal` `PER_ITEM`, optional with auto-compute fallback |
| `dvz_mesh_isoline` | `isoline_value` `PER_ITEM` |
| `dvz_mesh_left/right/contour` | hidden edge-rendering implementation detail |
| `dvz_mesh_texture` | `texture` resource |
| `dvz_mesh_index` | dynamic `index` buffer |
| `dvz_mesh_light_pos/color` | `light_pos`, `light_color` |
| `dvz_mesh_material_params` | `ambient`, `diffuse`, `specular` |
| `dvz_mesh_shine` | `shininess` |
| `dvz_mesh_emit` | `emissive` |
| `dvz_mesh_edgecolor/linewidth` | `edgecolor`, `linewidth` |
| `dvz_mesh_density` | `isoline_count` |
| `dvz_mesh_alloc` | automatic visual resize |
| `dvz_mesh_shape` / `dvz_mesh_reshape` | shape builder integration |

The target v0.4 contract adds variant axes, retained mesh texture binding,
`backface_culling`, `isoline_range`, auto normals, scale-backed colormaps, `CONSTANT` color, empty
visual support, dynamic index updates, and hidden edge-distance payloads.

## PBR Forward Compatibility

The v0.4 material model is Blinn-Phong (`ambient`, `diffuse`, `specular`, `shininess`,
`emissive`). The parameter block reserves:

| Reserved field | Future role | v0.4 value |
|---|---|---|
| `metallic` | PBR metallic factor | zero, ignored |
| `roughness` | PBR roughness factor | zero, ignored |

A future `normal_map` texture slot may share `texcoords` with the diffuse texture. See
`../semantics/LIGHTING.md` for the PBR/ray tracing upgrade path.
