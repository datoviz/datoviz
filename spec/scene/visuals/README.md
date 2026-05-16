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


## Active Implementation Status

This table tracks the installed v0.4 scene slice at a high level. The per-family specs below are
broader than the current implementation.

| Family | Spec | Public constructor/API | Retained state | Native rendering | GPU request/readback | Remaining gaps |
|---|---|---|---|---|---|---|
| `pixel` | [PIXEL.md](PIXEL.md) | `dvz_pixel()` | position/color/size, depth-cue state | emission and pipeline-selection coverage; no separate app readback smoke recorded | no family-specific path | pixel picking/styling beyond the first point-like slice |
| `primitive` | [PRIMITIVE.md](PRIMITIVE.md) | `dvz_primitive()` | topology, position/color, optional normal/index, material/depth/alpha | point/line/triangle primitives and indexed draws | no family-specific path | intentionally narrow low-level family |
| `point` | [POINT.md](POINT.md) | `dvz_point()` | position/color/size, external position buffers, depth-cue/alpha | native retained point rendering | point pick readback | richer payloads and rendered selection highlights |
| `marker` | [MARKER.md](MARKER.md) | none installed | none | no | no | spec only |
| `segment` | [SEGMENT.md](SEGMENT.md) | none installed | none | no | no | spec only |
| `path` | [PATH.md](PATH.md) | `dvz_path()` | line-strip position/color | primitive-backed line strip | no | widths, caps, joins, grouping, tapered lines, picking |
| `glyph` | [GLYPH.md](GLYPH.md) | no visual constructor; `DvzText`/`DvzAnnotation` bookkeeping exists | retained semantic text/annotation objects only | no rendered glyph path | no | atlas/shaping/rendering, labels, glyph/text picking |
| `image` | [IMAGE.md](IMAGE.md) | `dvz_image()`, field binding, texture wrappers | 2D `SampledField`, scale/colormap binding, partial updates | textured quad path | basic image probe readback | richer probe payloads, labels/categorical fields, rectangles/anchors/tinting |
| `mesh` | [MESH.md](MESH.md) | `dvz_mesh()` | vertex attributes, optional indices/normals, material/depth/alpha | indexed triangle mesh, depth, Phong/material, WBOIT/depth-peel, EDL/SSAO/G-buffer eligibility | no mesh picking path | broader mesh/object picking, geometry-resource API, full PBR |
| `sphere` | [SPHERE.md](SPHERE.md) | `dvz_sphere()`, typed sphere setters | impostor mode, center/color/radius, material/depth | analytic impostor sphere, including raycast and SSAO/G-buffer coverage | no sphere picking path | texture variants, per-item material/PBR, sphere picking |
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
