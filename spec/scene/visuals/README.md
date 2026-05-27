# Scene Visual Family Specs

This directory contains per-family data contracts for the v0.4 visual families.

Each document defines:

1. per-item attribute schema (field name, type, accepted sources, mutability),
2. visual-wide parameters (not per-item),
3. color mode or other variant axes,
4. transform model,
5. stage participation and picking model,
6. fallback notes,
7. relationship to v0.3.

These documents refine `../semantics/VISUAL_FAMILY_RULES.md` and `../semantics/VISUAL_CONTRACT.md` with enough concrete
detail to implement or validate each family.


## Consistency Contract

The active v0.4 visual API should use precise attribute names rather than a family-dependent
`size` field. The canonical names for this consistency pass are:

| Family | Canonical size/shape attribute | Meaning |
|---|---|---|
| `pixel` | `pixel_size` | square side length in screen pixels |
| `point` | `diameter` | circular point diameter in screen pixels |
| `marker` | `diameter` | marker sprite box diameter in screen pixels |
| `splat` | `sigma` | Gaussian standard deviations in screen pixels |
| `sphere` | `radius` | sphere radius in world/data units unless a radius mode is added |
| `segment` | `stroke_width` | line stroke width in screen pixels |
| `path` | `stroke_width` | path stroke width in screen pixels |
| `vector` | `vector` | displacement/direction from the item anchor; shaft/head shape uses stroke attributes |
| `tube` | `radius` | future 3D curve-surface radius in world/data units |
| `image` | `extent` | image rectangle width/height |
| `mesh` | none | geometry size comes from vertex data and transforms |

Public typed setters should configure behavior, not duplicate generic visual data upload. Visual
item data should flow through `dvz_visual_set_data()` / range updates using the canonical attribute
names above. Examples include `dvz_sphere_mode()`, `dvz_visual_set_material()`, shared
fill/stroke/outline aspect setters, and stroke-style setters as behavior APIs.

Picking is GPU-backed only. A visual without a GPU pick implementation for the requested precision
must fail explicitly through `DvzPickStatus` rather than falling back to CPU hit testing or silently
returning a miss.

Mesh instancing is part of the mesh contract. A mesh visual may draw one shared geometry resource
multiple times through per-instance attributes such as `instance_transform`, `instance_color`, and
an optional authored `instance_id`. Mesh picking must preserve both instance identity and
face/triangle identity.

Image visuals are multi-item visuals. The coherent v0.4 model is many image rectangles sampling one
shared texture/field/atlas, with per-item `position`, `extent`, `anchor`, `tex_rect`, `angle`, and
`tint`. Arbitrary different texture resources per image item are deferred; use multiple image
visuals or an atlas/texture array for that case.


## Active Implementation Status

This table tracks the installed v0.4 scene slice at a high level. The per-family specs below are
broader than the current implementation.

| Family | Spec | Public constructor/API | Retained state | Native rendering | GPU request/readback | Remaining gaps |
|---|---|---|---|---|---|---|
| `pixel` | [PIXEL.md](PIXEL.md) | `dvz_pixel()` | position/color/pixel_size, depth-cue state | square pixel marks, GLSL native points, WGSL instanced quads | square GPU picking | constant/scalar/grouped sources, shift, data-space pixel size |
| `primitive` | [PRIMITIVE.md](PRIMITIVE.md) | `dvz_primitive()` | topology, position/color, optional normal/index, material/depth/alpha | point/line/triangle primitives and indexed draws | item-level primitive picking | intentionally narrow low-level family |
| `point` | [POINT.md](POINT.md) | `dvz_point()` | position/color/diameter, external buffers, style/depth-cue/alpha | circular AA points, GLSL native points, WGSL instanced quads | circular GPU picking | scalar/grouped sources, shift, data-space diameter, richer selection |
| `marker` | [MARKER.md](MARKER.md) | `dvz_marker()` | position/color/diameter/angle/shape, style | code-SDF marker sprites in GLSL | bounding-box GPU picking | exact SDF picking, bitmap/SDF atlas, WGSL parity |
| `segment` | [SEGMENT.md](SEGMENT.md) | `dvz_segment()` | endpoint positions/color/stroke_width/caps | analytic screen-space stroke quads in GLSL | stroke GPU picking | dashes, arrows, gradients, richer path identity, WGSL parity |
| `path` | [PATH.md](PATH.md) | `dvz_path()` | line-strip plus optional subpaths/stroke_width/caps/joins | primitive line-strip or path-native stroked lowering | stroke GPU picking over lowered edges | analytic curve tessellation helpers, first-class closed-path API, dashes, path/subpath identity picking, WGSL parity |
| `vector` | [VECTOR.md](VECTOR.md) | none installed | none | no | no | proposed vector/arrow contract |
| `glyph` | [GLYPH.md](GLYPH.md) | `dvz_glyph()` low-level plus semantic `dvz_text()` lowering | text/font/annotation state lowers to glyph visuals | atlas-backed bitmap/SDF/MSDF-capable glyph path | no | data/world placement, HarfBuzz shaping, diagnostics, glyph/text picking |
| `image` | [IMAGE.md](IMAGE.md) | `dvz_image()`, field binding, texture wrappers | multi-item position/extent/anchor/tex_rect/tint over 2D `SampledField`, scale/colormap binding, partial updates | textured rectangle path | image item picking and pixel probe readback | richer probe payloads and tiled/LOD policy |
| `labels` | [LABELS.md](LABELS.md) | `dvz_labels()`, field + categorical scale binding | integer 2D `SampledField`, categorical scale, opacity/background/selected/hidden/boundary/fallback style | integer texture fetch with GLSL and WGSL variants | raw 2D label-id probe readback | 3D label slices, optimized sparse/high-id probe pressure tests |
| `mesh` | [MESH.md](MESH.md) | `dvz_mesh()` | vertex attributes, optional indices/normals, instance attributes, material/depth/alpha | indexed triangle mesh, optional instancing, depth, Phong/material, WBOIT/depth-peel, EDL/SSAO/G-buffer eligibility | item-level mesh picking | face/region picking, geometry-resource API, full PBR |
| `sphere` | [SPHERE.md](SPHERE.md) | `dvz_sphere()` | position/color/radius, impostor mode, material/depth | analytic impostor sphere, including raycast and SSAO/G-buffer coverage | sphere item picking | texture variants and per-item material/PBR |
| `splat` | [SPLAT.md](SPLAT.md) | none installed | none | no | no | proposed v0.4 experimental screen-space Gaussian contract |
| `volume` | [VOLUME.md](VOLUME.md) | `dvz_volume()`, volume setters, field binding | 3D `SampledField`, mode/slice/bounds/clipping/sampling/opacity/scale | box-proxy slice, MIP, and composite rendering | volume proxy item picking and slice probe/readout | isosurfaces, MPR, DVR/MIP ray-hit picking, categorical label volumes, and WebGPU parity |
| `errorbar` | [ERRORBAR.md](ERRORBAR.md) | none installed | none | no | no | spec only |
| `boxplot` | [BOXPLOT.md](BOXPLOT.md) | none installed | none | no | no | spec only |


## Future Visual-Family Specs

`splat` is documented as a proposed v0.4 experimental family in [SPLAT.md](SPLAT.md). The first
contract is limited to screen-space Gaussian billboards; scalable Gaussian-splat pipelines remain
future work.

`vector` is documented as a proposed v0.4 visual family in [VECTOR.md](VECTOR.md). It covers
quiver-style vector and arrow items with a single source identity, while allowing the first runtime
implementation to lower internally to shaft and head roles.

`tube` is documented as a future/spec-only family in [TUBE.md](TUBE.md). It covers radius-bearing 3D
curve surfaces such as tractography fibers, streamtubes, vessels, neurites, field lines, trajectory
tubes, and ribbons. It is intentionally separate from `path`: `path` owns screen-space stroked
polylines, while `tube` owns surface-like curve rendering with radius, depth, normals, and
mode-specific implementations such as impostor tubes, mesh tubes, and ribbons.

No `dvz_splat()`, `dvz_vector()`, or `dvz_tube()` constructor or runtime lowering is installed in
the active v0.4 slice.


## Reading Order

Read `../pipeline/ATTRIBUTE_SOURCES.md` in the parent directory before reading any family spec.
The granularity vocabulary (`CONSTANT`, `PER_ITEM`, `PER_SPAN`, `PER_GROUP`) and mutability hints
(`static`, `dynamic`, `streaming`) are used throughout.


## Active Proposal Inputs

1. [../proposals/active/MESH_API_DESIGN.md](../proposals/active/MESH_API_DESIGN.md)
2. [../proposals/active/MESH_SHADING_DESIGN.md](../proposals/active/MESH_SHADING_DESIGN.md)
3. [../proposals/active/MATERIAL_LIGHTING_API.md](../proposals/active/MATERIAL_LIGHTING_API.md)
4. [../proposals/active/VOLUME_DESIGN.md](../proposals/active/VOLUME_DESIGN.md)
5. [../implementation/VISUAL_SHADER_REFACTOR.md](../implementation/VISUAL_SHADER_REFACTOR.md)
6. [../proposals/active/LABELS_VISUAL_DESIGN.md](../proposals/active/LABELS_VISUAL_DESIGN.md)


## Future Semantic Resources

Some future scientific data models are intentionally not added to the active v0.4 family table yet.
They are better understood as semantic resources or compositions that lower to the families above:

1. graphs and networks lower to points/markers, segments/paths, glyphs, and overlays;
2. unstructured grids lower to boundary meshes, cut meshes, cell-edge segments, and glyphs;
3. vector and tensor fields lower to vectors/arrows, streamlines, glyphs, images, and volumes;
4. categorical label volumes and sparse voxel fields extend the sampled-field/volume direction;
5. tracks, ensembles, and molecular structures lower to multiple coordinated views.

The exploratory roadmap for these directions starts in
[`../proposals/future/SCIENTIFIC_VISUALIZATION_ROADMAP.md`](../proposals/future/SCIENTIFIC_VISUALIZATION_ROADMAP.md).
