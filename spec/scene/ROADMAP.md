# Scene Roadmap

Status: informative roadmap distilled from the former `agents/soon` and `agents/later` queues.

This file keeps backlog direction visible without recreating agent queues. Durable contracts remain
in the specialized spec files linked below. If a roadmap item becomes active, update the owning spec
and [../../agents/now/STATUS.md](../../agents/now/STATUS.md), not this file alone.


## Release Proof First

RC1 work should stay focused on:

1. WebGPU/WASM experimental subset and diagnostics;
2. v0.3 visible capability audit with fix/defer/external disposition;
3. public API/status labels;
4. compact example proof with validation notes;
5. retained textured mesh, raw `ctypes`, text, axes, labels, colorbars, scale bars, queries, and
   app/offscreen paths kept in validation.

High-payoff showcase order:

1. gallery proof pass for protein, LiDAR, brain, labels, textured mesh or terrain/planet,
   colorbars/legends, and capture paths;
2. vector visual pressure with a wind-field or displacement showcase;
3. raw label-id query hardening under transforms, larger fields, and request churn;
4. one explanatory layout proof combining axes, colorbar, categorical legend, scale bar, and panel
   reserves;
5. experimental splat showcase only after release-proof lanes are stable.


## Visual And Example Backlog

| Topic | Owning spec | Preserved direction |
| --- | --- | --- |
| Point, pixel, marker | [visuals/POINT.md](visuals/POINT.md), [visuals/PIXEL.md](visuals/PIXEL.md), [visuals/MARKER.md](visuals/MARKER.md) | Exact SDF marker picking, bitmap marker mode, atlas substrate, shared SDF/MSDF helpers, remaining scalar/grouped/constant attributes, `shift`, data-space sizes, backend-aware point-like lowering. |
| Segment, path, vector | [visuals/SEGMENT.md](visuals/SEGMENT.md), [visuals/PATH.md](visuals/PATH.md), [visuals/VECTOR.md](visuals/VECTOR.md) | Stroke-width-aware picking, path/subpath identity, WGSL stroke lowering, closed paths, dashing, vector heads, SVG import as authoring layer, 3D tubes/ribbons as later families. |
| Bezier and curves | [semantics/GEOMETRY_UTILITIES.md](semantics/GEOMETRY_UTILITIES.md), [visuals/PATH.md](visuals/PATH.md) | CPU tessellation helpers may feed ordinary `dvz_path()` data. Do not add a separate curve visual for the v0.4 baseline. |
| Image and labels | [visuals/IMAGE.md](visuals/IMAGE.md), [visuals/LABELS.md](visuals/LABELS.md), [integration/napari/NAPARI.md](integration/napari/NAPARI.md) | Shader-side scalar lookup, contrast/gamma/opacity, raw label-id probing under panzoom/keep-aspect and large fields, nearest integer labels, napari blend policy, N-D slicing adapter-owned first. |
| Volume | [visuals/VOLUME.md](visuals/VOLUME.md) | Napari-style volume clipping example, MIP validation, opacity/transfer/sample/clipping controls, DVR/MIP picking only after GPU payload semantics are explicit, MPR/isosurfaces/bricking/out-of-core deferred. |
| Sphere | [visuals/SPHERE.md](visuals/SPHERE.md) | Screen-radius support through shared `radius_space`, textured/equirectangular variants, analytic picking payloads, material expansion through shared material layer. |
| Splat | [visuals/SPLAT.md](visuals/SPLAT.md), [proposals/future/SPLATTING_FRAME_PLAN_REQUIREMENTS.md](proposals/future/SPLATTING_FRAME_PLAN_REQUIREMENTS.md) | v0.4 screen-space anisotropic Gaussian splats; v0.5 compute-generated inputs, projected covariance, scalable transparent splatting, and 3DGS compatibility. |
| Orientation gizmo | [proposals/active/CONTROLLER_INSPECTORS_AND_GIZMOS.md](proposals/active/CONTROLLER_INSPECTORS_AND_GIZMOS.md) | Passive axis triad attached to a panel, not transform handles; ordinary mesh geometry in an inset/overlay viewport; controller orientation only. |


## Query And Interaction Backlog

The durable query architecture is [interaction/GPU_QUERY_SYSTEM.md](interaction/GPU_QUERY_SYSTEM.md).
Preserved follow-up direction:

1. keep public interaction centered on `dvz_panel_query()` and `DvzQueryResult`;
2. keep rendered visual query authority GPU-only;
3. keep generic query code under `src/scene/query/` and visual policy in family query files;
4. use `r32uint` as the baseline query profile and `rg32uint` as the preferred 64-bit profile;
5. implement two-attachment `r32uint` fallback only end to end before selecting it automatically;
6. defer MIP and DVR/composite volume sample queries until exact GPU semantics are specified;
7. preserve no-CPU-visual-picking tests for readback failure paths;
8. add exact marker, mesh face/region, image alpha/texel, text/glyph/labels, volume ray-hit, and
   hit-policy extensions as separate focused changes.


## Techniques, Effects, And Materials

Durable contracts live in:

1. [implementation/GRAPH_TECHNIQUES.md](implementation/GRAPH_TECHNIQUES.md)
2. [implementation/TRANSPARENCY_MSAA.md](implementation/TRANSPARENCY_MSAA.md)
3. [implementation/OCCLUSION_EFFECTS.md](implementation/OCCLUSION_EFFECTS.md)
4. [proposals/active/DEPTH_OF_FIELD_POSTPROCESS.md](proposals/active/DEPTH_OF_FIELD_POSTPROCESS.md)
5. [proposals/promoted/SCREEN_SPACE_EFFECTS_DESIGN.md](proposals/promoted/SCREEN_SPACE_EFFECTS_DESIGN.md)
6. [proposals/active/MATERIAL_LIGHTING_API.md](proposals/active/MATERIAL_LIGHTING_API.md)

Preserved direction:

1. all techniques remain panel-local, default-off retained state expanded into FramePlan graph
   resources and passes;
2. DoF is showcase-oriented postprocess, not a scientific default;
3. outline, edge enhancement, and bloom need explicit identity, composition, export, and scissor
   policy before public API;
4. SSAO quality work should improve depth reconstruction, kernels, rotation, blur, and tuning
   without replacing the graph path;
5. MSAA hardening should cover sample-count serialization, vklite resolve lowering, sphere
   alpha-to-coverage, and comparison examples;
6. non-volume consumers of volume occlusion should start with primitive or unlit mesh and verify
   graph reads, bind layout, shader variant, and disabled/enabled pixel behavior;
7. material polish should use explicit retained fields and capability resolution, not one-off
   visual-family switches.


## Runtime, WebGPU, WASM, And Packaging

Durable browser/runtime direction lives in [integration/WEBGPU_WASM.md](integration/WEBGPU_WASM.md)
and [../drp2/WEBGPU_ROADMAP.md](../drp2/WEBGPU_ROADMAP.md).

Preserved direction:

1. pure WebGPU fixture runner stays in RC validation;
2. WASM work starts from a portable scene/DRP2/WGSL target with native runtime pieces excluded;
3. browser input must become scene/controller calls that emit DRP2 updates;
4. JSON is a debug transport, not the final hot path;
5. component packaging work should add CI profiles, out-of-tree consumer smokes, and component-owned
   header install sets before tightening package boundaries.


## Text And Layout Backlog

Durable contracts live in [semantics/TEXT.md](semantics/TEXT.md) and
[implementation/TEXT_SHAPING_ATLAS.md](implementation/TEXT_SHAPING_ATLAS.md).

Preserved direction:

1. keep `DvzText*` as the semantic public text surface and `dvz_glyph()` as lower-level rendering;
2. wire axes, colorbars, legends, and readouts to semantic text;
3. finish data/world placement, depth policy, DPI and scissor validation;
4. keep deterministic bitmap text available for tests and diagnostics;
5. make FreeType/HarfBuzz optional but preferred for production quality;
6. move atlas requests from codepoints to `(font face, glyph id)`;
7. add diagnostics for missing glyphs, fallback renderer, atlas capacity, and malformed UTF-8;
8. keep Slug-style vector text and equation/MicroTeX support optional.


## Post-v0.4 Structural Work

Do not let these delay `v0.4.0` unless they fix a release blocker:

1. split remaining large scene emission, JSON export, constructor/allocation, dirty-state, and
   teardown helpers;
2. keep request/query execution generic and move visual-family policy behind operations tables;
3. move DRP2 trace fingerprint normalization into DRP2 diagnostics;
4. add out-of-tree package-consumer tests before tightening installed headers;
5. migrate only low-risk single-frame graphics tests into shared fixtures;
6. add resource-capacity accounting, in-process thread workers, and reporting polish only after
   process sharding remains stable;
7. avoid mixing broad mechanical refactors with new visual families or API expansion.


## Long-Horizon Rendering

Future unified ray rendering should remain compatible with the active scene -> FramePlan -> DRP2
boundary. The first practical path should be compute raymarching, not hardware ray tracing.

Low-regret preparation:

1. keep `DvzSampledField` common across images, slices, labels, and volumes;
2. make scale/colormap realization a shared transfer resource rather than visual-private CPU-expanded
   texture data;
3. keep query results spatial and typed, including world/data coordinates and sampled values;
4. leave room for panel-level renderer or technique selection;
5. keep frame-plan outputs explicit: color, depth or hit distance, ids, values, and auxiliary
   request products;
6. keep hardware ray tracing APIs out of public scene contracts until compute raymarching validates
   the abstraction.
