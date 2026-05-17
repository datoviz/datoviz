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
| `sphere` | `radius` | sphere radius in world/data units unless a radius mode is added |
| `segment` | `stroke_width` | line stroke width in screen pixels |
| `path` | `stroke_width` | path stroke width in screen pixels |
| `image` | `extent` | image rectangle width/height |
| `mesh` | none | geometry size comes from vertex data and transforms |

Public typed setters should configure behavior, not duplicate generic visual data upload. Visual
item data should flow through `dvz_visual_set_data()` / range updates using the canonical attribute
names above. Examples include `dvz_sphere_mode()`, `dvz_visual_set_material()`, and shared
fill/stroke or stroke-style setters as behavior APIs.

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
| `primitive` | [PRIMITIVE.md](PRIMITIVE.md) | `dvz_primitive()` | topology, position/color, optional normal/index, material/depth/alpha | point/line/triangle primitives and indexed draws | no family-specific path | intentionally narrow low-level family |
| `point` | [POINT.md](POINT.md) | `dvz_point()` | position/color/diameter, external buffers, style/depth-cue/alpha | circular AA points, GLSL native points, WGSL instanced quads | circular GPU picking | scalar/grouped sources, shift, data-space diameter, richer selection |
| `marker` | [MARKER.md](MARKER.md) | `dvz_marker()` | position/color/diameter/angle/shape, style | code-SDF marker sprites in GLSL | bounding-box GPU picking | exact SDF picking, bitmap/SDF atlas, WGSL parity |
| `segment` | [SEGMENT.md](SEGMENT.md) | `dvz_segment()` | endpoint positions/color/stroke_width/caps | analytic screen-space stroke quads in GLSL | no | dashes, arrows, gradients, picking, WGSL parity |
| `path` | [PATH.md](PATH.md) | `dvz_path()` | line-strip plus optional subpaths/stroke_width | primitive line-strip or stroked segment lowering | no | joins, closed subpaths, dashes, picking, WGSL parity |
| `glyph` | [GLYPH.md](GLYPH.md) | no visual constructor; `DvzText`/`DvzAnnotation` bookkeeping exists | retained semantic text/annotation objects only | no rendered glyph path | no | atlas/shaping/rendering, labels, glyph/text picking |
| `image` | [IMAGE.md](IMAGE.md) | `dvz_image()`, field binding, texture wrappers | multi-item position/extent/anchor/tex_rect/tint over 2D `SampledField`, scale/colormap binding, partial updates | textured rectangle path | basic image probe readback | item-aware image picking, labels/categorical fields, richer probe payloads |
| `mesh` | [MESH.md](MESH.md) | `dvz_mesh()` | vertex attributes, optional indices/normals, instance attributes, material/depth/alpha | indexed triangle mesh, optional instancing, depth, Phong/material, WBOIT/depth-peel, EDL/SSAO/G-buffer eligibility | no mesh picking path | instance-aware mesh picking, geometry-resource API, full PBR |
| `sphere` | [SPHERE.md](SPHERE.md) | `dvz_sphere()` | position/color/radius, impostor mode, material/depth | analytic impostor sphere, including raycast and SSAO/G-buffer coverage | no sphere picking path | texture variants, per-item material/PBR, sphere picking |
| `volume` | [VOLUME.md](VOLUME.md) | `dvz_volume()`, volume setters, field binding | 3D `SampledField`, mode/slice/bounds/clipping/sampling/opacity/scale | box-proxy slice, MIP, and composite rendering | no volume probe/pick path | transfer functions, isosurfaces, MPR, DVR/MIP picking, richer payloads |
| `errorbar` | [ERRORBAR.md](ERRORBAR.md) | none installed | none | no | no | spec only |
| `boxplot` | [BOXPLOT.md](BOXPLOT.md) | none installed | none | no | no | spec only |


## Reading Order

Read `../pipeline/ATTRIBUTE_SOURCES.md` in the parent directory before reading any family spec.
The granularity vocabulary (`CONSTANT`, `PER_ITEM`, `PER_SPAN`, `PER_GROUP`) and mutability hints
(`static`, `dynamic`, `streaming`) are used throughout.


## Active Proposal Inputs

1. [../proposals/MESH_API_DESIGN.md](../proposals/MESH_API_DESIGN.md)
2. [../proposals/MESH_SHADING_DESIGN.md](../proposals/MESH_SHADING_DESIGN.md)
3. [../proposals/MATERIAL_LIGHTING_API.md](../proposals/MATERIAL_LIGHTING_API.md)
4. [../proposals/VOLUME_DESIGN.md](../proposals/VOLUME_DESIGN.md)
5. [../implementation/VISUAL_SHADER_REFACTOR.md](../implementation/VISUAL_SHADER_REFACTOR.md)
