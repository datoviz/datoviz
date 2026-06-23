# Visual Family Implementation Status

Status: informative implementation snapshot for the active v0.4 scene slice.

This is the canonical per-family status matrix. The per-family specs define data contracts and
broader intended behavior; this file records what is active today.


## Active Implementation Status

| Family | Spec | Public constructor/API | Retained state | Native rendering | GPU request/readback | Remaining gaps |
|---|---|---|---|---|---|---|
| `pixel` | [PIXEL.md](PIXEL.md) | `dvz_pixel()` | position/color/pixel_size_px, depth-cue state | square pixel marks, GLSL native points, WGSL instanced quads | square GPU picking | scalar color mode, shift, data-space pixel size |
| `primitive` | [PRIMITIVE.md](PRIMITIVE.md) | `dvz_primitive()` | topology, position/color, optional normal/index, material/depth/alpha | point/line/triangle primitives and indexed draws | item-level primitive picking | intentionally narrow low-level family |
| `point` | [POINT.md](POINT.md) | `dvz_point()` | position/color/diameter_px, external buffers, style/depth-cue/alpha | circular AA points, GLSL native points, WGSL instanced quads | circular GPU picking | scalar color/diameter_px modes, shift, data-space diameter_px, richer selection |
| `marker` | [MARKER.md](MARKER.md) | `dvz_marker()` | position/color/diameter_px/angle/shape, style | code-SDF marker sprites in GLSL and WGSL instanced quads | bounding-box GPU picking | exact SDF picking, bitmap/SDF atlas, scalar modes |
| `segment` | [SEGMENT.md](SEGMENT.md) | `dvz_segment()` | endpoint positions/color/stroke_width_px/caps | analytic screen-space stroke quads in GLSL | stroke GPU picking | dashes, arrows, gradients, grouped sources, WGSL parity |
| `path` | [PATH.md](PATH.md) | `dvz_path()` | line-strip plus optional subpaths/stroke_width_px/caps/joins | primitive line-strip or path-native stroked lowering | stroke GPU picking over lowered edges | analytic curve tessellation helpers, closed-path API, dashes, path/span identity picking, WGSL parity |
| `vector` | [VECTOR.md](VECTOR.md) | `dvz_vector()` | position/vector/color/stroke_width_px, style, subpaths | cap-based straight and curved stroke lowering | vector item picking through stroke lowerings | independent head dimensions, scalar styling, WGSL parity |
| `glyph` | [GLYPH.md](GLYPH.md) | `dvz_glyph()` low-level plus semantic `dvz_text()` lowering | text/font/annotation state lowers to glyph visuals | atlas-backed bitmap/SDF/MSDF-capable glyph path | no | data/world placement, HarfBuzz shaping, diagnostics, glyph/text picking |
| `image` | [IMAGE.md](IMAGE.md) | `dvz_image()`, field binding, texture wrappers | multi-item position/extent/anchor/tex_rect/tint over 2D `SampledField`, scale/colormap binding, partial updates | textured rectangle path | image item picking and pixel probe readback | richer probe payloads and tiled/LOD policy |
| `labels` | [LABELS.md](LABELS.md) | `dvz_labels()`, field + categorical scale binding | integer 2D `SampledField`, categorical scale, opacity/background/selected/hidden/boundary/fallback style | integer texture fetch with GLSL and WGSL variants | raw 2D label-id probe readback | 3D label slices, optimized sparse/high-id probe pressure tests |
| `mesh` | [MESH.md](MESH.md) | `dvz_mesh()` | vertex attributes, optional indices/normals, instance attributes, material/depth/alpha | indexed triangle mesh, optional instancing, depth, Phong/material, WBOIT/depth-peel, EDL/SSAO/G-buffer eligibility | item-level mesh picking | face/region picking, geometry-resource API, full PBR |
| `sphere` | [SPHERE.md](SPHERE.md) | `dvz_sphere()` | position/color/radius, impostor mode, material/depth | analytic impostor sphere, including raycast and SSAO/G-buffer coverage | sphere item picking | texture variants and per-item material/PBR |
| `splat` | [SPLAT.md](SPLAT.md) | `dvz_splat()` | position/color/sigma/angle | screen-space Gaussian billboards | no | opacity attribute, request/readback, scalable splat tiers |
| `volume` | [VOLUME.md](VOLUME.md) | `dvz_volume()`, volume setters, field binding | 3D `SampledField`, mode/slice/bounds/clipping/sampling/opacity/scale | box-proxy slice, MIP, and composite rendering | volume proxy item picking and slice probe/readout | isosurfaces, MPR, DVR/MIP ray-hit picking, categorical label volumes, and WebGPU parity |
| `errorbar` | [ERRORBAR.md](ERRORBAR.md) | none installed | none | no | no | spec only |
| `boxplot` | [BOXPLOT.md](BOXPLOT.md) | none installed | none | no | no | spec only |


## Future Or Spec-Only Families

| Family | Spec | Status |
|---|---|---|
| `tube` | [TUBE.md](TUBE.md) | future/spec-only radius-bearing 3D curve-surface family |
