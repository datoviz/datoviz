> **Execution Status**
>
> - **Status:** approved post-RC1 implementation plan
> - **Updated on:** 2026-07-16
> - **Purpose:** define backend-neutral arbitrary 2D clipping without delaying v0.4.0-rc1
> - **First slice:** retained polygon-with-holes clips lowered through stencil testing

# Arbitrary Clip Masks

## Decision Summary

Datoviz should add a retained, scene-owned `DvzClip` abstraction after RC1. The public scene API
must describe clip geometry, coordinate space, and attachment scope; it must not expose stencil
references, mask render passes, backend texture formats, or Vulkan/WebGPU objects.

The first implementation should support hard 2D clips defined by one polygon with optional holes.
It should lower them to:

```text
existing panel/plot scissor intersection
  -> triangulated clip geometry written into stencil
  -> ordinary visual draws accepted where stencil equals the clip reference
```

This work is post-RC1 and must not block the current release candidate unless a concrete
GSP/Matplotlib integration blocker promotes it. Soft coverage masks, path Boolean composition,
arbitrary image masks, and broad figure-level effects remain later extensions behind the same
backend-neutral clip concept.


## Existing State And Gap

The current scene contract supports rectangular clipping only:

1. `DvzVisualAttachDesc.clip_rect` selects `AUTO`, `PANEL`, or `PLOT`;
2. scene frame planning resolves that choice to a framebuffer rectangle;
3. DRP2 emits dynamic viewport and scissor state per draw;
4. data visuals normally use the plot scissor, while panel chrome may use the full panel;
5. volume box and plane clipping are visual-specific 3D sampling controls, not general panel
   clipping.

The lower layers contain useful but incomplete stencil foundations:

1. vklite has depth/stencil image and pipeline support plus a native stencil technique test;
2. the DRP2 specification describes stencil pipeline state and `SetStencilReference`;
3. the active DRP2 C command model exposes depth-only render attachments and depth-only pipeline
   configuration;
4. `SetStencilReference` is not present in the active C command enum, stream API, serializers,
   validators, or scene emitter;
5. scene render targets normally use `D32_SFLOAT`, not a combined depth/stencil format.

Therefore arbitrary clips require one complete scene -> FramePlan -> DRP2 -> runtime slice. A scene
setter alone would create a false public capability.


## Goals

The first slice must:

1. clip panel content and individual visual attachments to arbitrary polygonal regions;
2. support concave polygons and polygon holes through the existing F64 polygon/triangulation path;
3. intersect arbitrary clips with the existing panel or plot scissor;
4. preserve ordinary depth testing, blending, MSAA, draw order, and panel composition;
5. apply the same clip semantics to rendering, GPU query, picking, capture, and readback;
6. work through the shared DRP2 stream on native Vulkan and the declared WebGPU subset;
7. retain clip geometry and invalidate only affected frame-plan products when it changes;
8. report unsupported formats, mask limits, and backend gaps through deterministic diagnostics.


## Non-Goals For The First Slice

Do not include:

1. a public stencil API;
2. a user-editable render graph or custom mask pass;
3. grayscale, feathered, blurred, or image-based alpha masks;
4. SVG/Bezier path ingestion beyond CPU tessellation into polygon rings;
5. arbitrary Boolean clip stacks or nested clip algebra;
6. world-space solid clipping of meshes or volumes;
7. replacement of volume box/plane clipping;
8. a Vulkan-only shortcut or browser-specific scene contract;
9. CPU geometry hit testing as a substitute for clipped GPU query behavior.


## Public Semantics

### Clip Object

`DvzClip` is scene-owned retained semantic state. It is neither a visible `DvzVisual` nor a backend
render resource. Its first geometry source is a `DvzPolygon`, including its outer ring and holes.
The clip observes polygon geometry revisions and is re-realized when that polygon changes.

The first API review should start from this shape; exact names remain open until implementation:

```c
typedef struct DvzClip DvzClip;

DvzClip* dvz_clip(DvzScene* scene, uint32_t flags);
void dvz_clip_destroy(DvzClip* clip);

DvzResult dvz_clip_set_polygon(DvzClip* clip, DvzPolygon* polygon);
DvzResult dvz_clip_set_space(DvzClip* clip, DvzVisualCoordSpace space);
DvzResult dvz_clip_set_transform(DvzClip* clip, mat4 transform);
```

The referenced polygon must belong to the same scene. Destruction or invalidation of the polygon
must deactivate the clip with a diagnostic rather than leave a dangling reference. The scene frame
snapshot owns any realized geometry needed by an emitted frame.

### Coordinate Spaces

Reuse the established visual coordinate meanings:

| Space | Clip behavior |
| --- | --- |
| `DATA` | geometry follows the panel data transform and controller |
| `VIEW` | geometry uses resolved metric view coordinates |
| `PANEL` | normalized, panel-relative geometry remains fixed during navigation |
| `PANEL_PIXEL` | panel-local logical-pixel geometry remains screen locked |

An arbitrary clip is a 2D framebuffer coverage constraint after coordinate transformation. A
`DATA` clip over a 3D panel does not become a world-space cutting volume.

### Attachment And Panel Scope

The attachment descriptor is the authoritative per-visual location because one visual may be
attached to different panels with different clip behavior:

```c
DvzVisualAttachDesc attach = dvz_visual_attach_desc();
attach.clip_rect = DVZ_VISUAL_CLIP_PLOT; /* coarse rectangular intersection */
attach.clip = clip;                      /* optional arbitrary intersection */
dvz_panel_add_visual(panel, visual, &attach);
```

Appending a `DvzClip*` field is compatible with the descriptor's `struct_size` growth policy, but
the public API review must confirm generated-binding behavior before the field lands.

The common panel-content operation should also have an explicit convenience API:

```c
DvzResult dvz_panel_set_content_clip(DvzPanel* panel, DvzClip* clip);
DvzResult dvz_panel_clear_content_clip(DvzPanel* panel);
```

Panel content clipping applies by default to data visuals and plot-grid roles. It does not clip
axes, labels, legends, colorbars, borders, backgrounds, readouts, or other panel chrome. An explicit
attachment clip overrides the panel content clip for that attachment. The API review must include a
clear per-attachment opt-out from an inherited panel clip; do not overload a null pointer with both
"inherit" and "disable" meanings.

A full-figure panel can express the initial figure-shaped use case. A separate
`dvz_figure_set_content_clip()` should be added only after its interaction with multiple panels,
panel chrome, and final composition is specified.

### Composition Rules

The effective first-slice coverage is:

```text
render area
  intersect selected PANEL/PLOT scissor
  intersect zero or one effective arbitrary clip
```

No union, difference, XOR, nested intersection stack, or inverted clip is implicit. Those
operations may later be represented by precomputed polygon geometry or an explicit clip-composition
API.


## Frame-Plan And Scene Lowering

`DvzClip` should lower to normalized clip facts carried by the frame plan, not trigger family
checks in generic draw emission. The normalized facts should include:

1. stable clip identity and revision;
2. resolved coordinate/transform policy;
3. triangulated position/index resources;
4. panel-local scissor bounds for coarse rejection;
5. effective panel/attachment scope;
6. hard-mask realization mode;
7. a pass-local stencil reference assigned during lowering.

The emitter should collect unique effective clips for a panel pass, assign nonzero 8-bit stencil
references, emit their mask draws, and associate each clipped draw packet with the matching
reference. Reference zero means outside every mask. Unclipped draws use a stencil-disabled pipeline
variant.

The first implementation may impose a bounded number of unique clips per render pass. It must
validate the limit and emit a deterministic diagnostic rather than wrap or alias stencil values.
The implementation should cache mask geometry and pipeline objects across frames while rebuilding
pass-local references and transformed bounds when panel or clip revisions require it.


## DRP2 Work

Before scene clipping is exposed, reconcile the active C implementation with the existing DRP2
stencil specification.

Required protocol work:

1. extend the active render-pass command model from depth-only state to a depth/stencil attachment
   contract with explicit format and independent depth/stencil load, store, clear, and access
   fields;
2. add stencil front/back compare and operations plus read/write masks to render-pipeline state;
3. add `DVZ_DRP2_COMMAND_SET_STENCIL_REFERENCE` and
   `dvz_drp2_stream_set_stencil_reference()`;
4. update command storage, schema alignment, JSON/DVZR serialization, recording, replay,
   validation, diagnostics, fingerprints, and capability declarations;
5. validate pipeline/pass format compatibility and forbid stencil commands outside render passes;
6. support combined depth/stencil formats without regressing existing depth-only passes;
7. add positive and negative portable fixtures before scene emission depends on the feature.

The DRP2 specification remains backend-neutral. Vulkan stencil enums and WebGPU object details must
not become public DRP2 or scene semantics.


## Runtime Lowering

### Native Vulkan

The native path should:

1. allocate or reuse a combined depth/stencil attachment such as `D32_SFLOAT_S8_UINT` when a pass
   requires arbitrary clipping and depth;
2. use a mask pipeline with color writes disabled, depth test/write disabled, stencil compare
   `ALWAYS`, and pass operation `REPLACE`;
3. draw triangulated clip geometry with its assigned stencil reference;
4. use ordinary visual pipeline variants with stencil compare `EQUAL`, stencil writes disabled,
   and unchanged depth/blend state;
5. clear stencil deterministically before mask realization without erasing color already rendered
   for other panels;
6. preserve attachment sample-count compatibility under MSAA.

The existing vklite stencil technique test is proof of mechanism, not proof of the scene/DRP2
contract.

### WebGPU

The WebGPU path should implement the same command stream with a supported depth/stencil format,
stencil load/store operations, pipeline stencil state, and `setStencilReference()`. If the selected
surface/runtime profile cannot provide the required format or sample count, capability resolution
must reject or deactivate the clip with an explicit diagnostic.

No WGSL fragment-mask branch should be required for the hard-stencil slice.


## Antialiasing And Soft Masks

Single-sample stencil clipping has a hard binary edge. When MSAA is enabled, clip geometry and the
stencil attachment use the panel sample count so edge sample coverage provides the first
antialiasing path. The clip contract must not promise feathering in the first slice.

A later soft-mask realization may rasterize coverage into an `R8_UNORM` texture and sample it from
visual fragment shaders. That extension requires explicit decisions about mask resolution,
filtering, premultiplication, derivatives, composition, query thresholds, and the cost of adding a
mask binding or shader variant across visual families. It should remain an internal realization of
`DvzClip`, not a separate public clipping model.


## Queries, Picking, And Readback

Rendered queries must reproduce the effective clip used by the color pass. In particular:

1. clipped fragments write neither color nor query identity;
2. query passes reuse the same clip geometry, transformed bounds, scissor, and stencil semantics;
3. clip-only changes invalidate query planning as well as color rendering;
4. unsupported exact clipped queries return an unsupported diagnostic;
5. CPU point-in-polygon testing must not silently replace the GPU result because it can diverge at
   transformed, multisampled, discarded, or depth-tested boundaries.

Capture and readback need no separate clip operation once the rendered target is correct.


## Capability And Fallback Policy

The capability resolver should distinguish:

1. combined depth/stencil attachment support;
2. required attachment sample counts;
3. stencil pipeline and dynamic reference support;
4. render-target/pass compatibility;
5. clipped query support;
6. optional future sampled coverage-mask support.

Fallback order for the first slice:

1. use stencil with the requested MSAA sample count;
2. if policy permits, reduce to a supported sample count while preserving the hard clip;
3. otherwise deactivate or reject the arbitrary clip with a deterministic diagnostic.

Falling back to the polygon bounding-box scissor changes visible semantics and must never happen
silently.


## Post-RC1 Implementation Sequence

### Phase 1: Contract And DRP2 Reconciliation

1. finalize the `DvzClip` ownership, inheritance, opt-out, and coordinate-space API;
2. update the specialized scene transform, frame-plan, query, and API specs;
3. reconcile DRP2 command/schema terminology for depth/stencil attachments;
4. implement and validate the DRP2 stencil state and reference command end to end;
5. add native and WebGPU fixture coverage without exposing scene clipping yet.

Exit proof: one portable DRP2 fixture draws ordinary geometry through a polygonal stencil mask on
both supported runtimes, with negative validation for invalid format/pass/state combinations.

### Phase 2: Retained Polygon Clip

1. add scene-owned `DvzClip` storage, lifetime, IDs, revisions, and diagnostics;
2. bind one `DvzPolygon` source and reuse the existing F64 triangulation path;
3. add attachment and panel-content APIs;
4. lower clips into frame-plan resources, mask draws, references, and clipped draw packets;
5. add pipeline cache keys for mask-write and stencil-test variants;
6. preserve existing rectangular scissor routing as a coarse intersection.

Exit proof: a deterministic two-panel example shows oversized points, paths, images, and mesh
content clipped by a concave polygon with a hole while axes and labels remain visible.

### Phase 3: Queries, Churn, And Public Readiness

1. apply the same clip plan to GPU query/picking passes;
2. test polygon updates, controller changes, resize, repeated frames, and destruction order;
3. verify MSAA and depth/blend interactions;
4. expose capability and diagnostic results through the public scene/runtime surface;
5. refresh generated C documentation and Python bindings;
6. add a focused C example and Python smoke using the same public contract.

Exit proof: rendering and query results agree inside, outside, on a hole, and near a transformed
clip boundary on native Vulkan and the declared WebGPU subset.

### Phase 4: Optional Extensions

Only after the hard polygon slice is stable, evaluate:

1. tessellated path and SVG authoring adapters;
2. inverted and composed clip operations;
3. soft/feathered coverage masks;
4. image-derived masks;
5. figure-composition clipping;
6. caching/atlas strategies for many small masks.


## Validation Matrix

Required implementation coverage:

1. polygon: convex, concave, holes, invalid rings, empty result, and geometry mutation;
2. spaces: DATA under pan/zoom, VIEW, PANEL under resize, and PANEL_PIXEL under device scale;
3. scopes: attachment override, panel content inheritance, explicit opt-out, and unmasked chrome;
4. visuals: point/marker, path/segment, image, mesh, text/glyph where supported, and overlapping
   depth-tested content;
5. rendering: single sample, supported MSAA counts, alpha blending, adjacent panels, offscreen,
   app presentation, capture, and repeated frames;
6. protocol: serialization round trip, recording/replay, invalid command order, wrong attachment
   format, stencil-reference bounds, and mask-count exhaustion;
7. query: inside, outside, hole, boundary, transformed clip, and unsupported-backend behavior;
8. runtime: resize, target recreation, clip destruction, polygon destruction, and no per-frame
   object accumulation;
9. cross-backend: native Vulkan artifact plus WebGPU fixture/browser proof for the declared slice.

For implementation changes, run the narrow relevant tests while iterating, then the repository
checks required for public headers, bindings, shaders, DRP2, scene, and Vulkan validation. Public
API/header changes require `just ctypes`, `just ctypes-check`, and generated-reference review;
shader changes require `just shader-abi-check`.


## Acceptance Criteria

The hard-clip feature is ready to classify as supported only when:

1. the public API contains no backend stencil terminology;
2. one clip can be retained, mutated, attached, cleared, and destroyed safely;
3. polygon holes and concavity render correctly;
4. axes/chrome scope is deterministic and documented;
5. color and query passes agree;
6. adjacent panels do not bleed or clear one another;
7. native and declared WebGPU behavior share the same DRP2 stream semantics;
8. unsupported capability combinations produce stable diagnostics;
9. generated C/Python bindings and public examples are validated;
10. runtime traces show no unbounded per-frame resource or pipeline churn.
