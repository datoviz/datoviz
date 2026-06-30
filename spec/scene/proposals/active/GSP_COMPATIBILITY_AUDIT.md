# Datoviz v0.4 GSP Compatibility Audit

Status: active audit for `gsp-compat-prep`.


## Executive Summary

Datoviz v0.4 is close to a viable GSP backend target. The scene/app path, retained visual model,
FramePlan artifact emission, raw ctypes generation, offscreen capture, dense visual updates,
sampled fields, scene buffers, and unified panel query API already match the GSP direction.

The initial Datoviz-side blockers before an RC1-compatible GSP adapter were narrow and API-shaped:

1. expose a public live runtime/view capability query;
2. expose or explicitly document stable object identity and protocol-id mapping rules;
3. harden/document query support per visual family and query target;
4. extend raw ctypes policy/smokes to include the GSP-critical scene/query/capability path;
5. publish a visual-family mapping table usable by adapter authors without private source reads.

Most transform, resource, offscreen, and first-slice visual mapping needs are already satisfiable by
public C APIs. GSP should own high-level Python semantics, protocol object models, vector export,
virtual data sources, and adapter-side mapping tables. Datoviz should not embed GSP or expose
Vulkan/runtime internals.


## Implementation Checkpoint

Status after commits through `DVZ-GSP-005: document visual family adapter mapping` on
`gsp-compat-prep`:

| Task | Checkpoint result |
|---|---|
| `DVZ-GSP-001` | `dvz_view_capabilities()` exposes a public live view/runtime capability snapshot. |
| `DVZ-GSP-002` | Scene-local `DvzId` getters exist for scene, figure, panel, visual, and retained scene objects needed by query/adapter maps. GSP protocol ids remain adapter-owned. |
| `DVZ-GSP-003` | Query results now include parent scene and figure ids in addition to panel/visual ids. |
| `DVZ-GSP-004` | Raw ctypes policy and smoke coverage include GSP-critical capability/query structs and the no-runtime panel query path. |
| `DVZ-GSP-005` | `spec/scene/visuals/GSP_MAPPING.md` documents the adapter-facing family/resource/query mapping. |
| `DVZ-GSP-006` | Public C app smoke covers image background, point overlay, offscreen render, PNG capture, point query, and image-only query. |

The narrow Datoviz-side RC1 GSP-prep checkpoints are complete. Structured diagnostic subject
records can stay post-RC1 unless the first adapter needs them.


## Already Ready

| GSP need | Current Datoviz surface | Evidence inspected | Notes |
|---|---|---|---|
| Scene/figure/panel/app/view creation | `dvz_scene()`, `dvz_figure()`, `dvz_panel()`, `dvz_app()`, `dvz_view_offscreen()`, `dvz_view_glfw()` | `include/datoviz/scene.h`, `include/datoviz/app.h`, `src/app/app.c` | Public C flow matches `scene -> figure -> panel -> view -> render`. |
| Scene-to-runtime boundary | `dvz_figure_emit_frame()`, `DvzSceneFrameArtifact`, `dvz_scene_frame_artifact_stream()`, app-owned `DvzView` execution | `spec/scene/core/RUNTIME_BOUNDARY.md`, `include/datoviz/scene.h`, `src/app/app.c` | Runtime consumes artifact-owned DRP2 streams; scene does not expose backend handles. |
| Default/static capability snapshot | `DvzCapabilitySnapshot`, `dvz_capability_snapshot()`, `dvz_scene_set_capabilities()` | `include/datoviz/scene/types.h`, `include/datoviz/scene/frame_plan.h`, `src/scene/frame_plan/capabilities.c` | Useful for tests and planning, but not a live runtime query. |
| Query/readback capability fields | Snapshot includes readback, integer format, render-target format, profile, and row-pitch fields | `include/datoviz/scene/types.h`, `src/app/app.c`, `src/scene/query/policy.c` | More complete than the original GSP prep baseline. |
| Dense visual uploads | `dvz_visual_set_data()`, `dvz_visual_set_data_many()` | `include/datoviz/scene.h`, `src/scene/visuals/attr_data.c`, examples under `examples/c/visuals/` | Payloads are copied before return; suitable for ctypes pointers. |
| Partial visual updates | `dvz_visual_set_data_range()` | `include/datoviz/scene.h`, `src/scene/visuals/attr_data.c`, `examples/c/features/update_visual_data.c` | Direct local update path exists without JSON/base64. |
| Scene buffers | `dvz_scene_buffer()`, `dvz_scene_buffer_set_data()`, `dvz_visual_set_buffer()`, `dvz_visual_set_attr_buffer()` | `include/datoviz/scene.h`, `src/scene/domain/buffer.c`, `src/scene/visuals/visual_bindings.c` | Supports copied CPU buffers and live/external resource labels when no CPU data is present. |
| Compute buffer binding | `dvz_scene_compute()`, `dvz_scene_compute_set_buffer()`, `dvz_figure_add_compute()` | `include/datoviz/scene.h`, `src/scene/domain/compute.c` | Experimental but already public and FramePlan-backed. |
| Sampled fields and region updates | `DvzSampledField`, `dvz_sampled_field_set_data()`, `dvz_sampled_field_update_region()`, `dvz_visual_set_field()` | `include/datoviz/scene/field.h`, `src/scene/domain/field*.c` | Covers image/labels/volume texture resource path. |
| Point visual mapping | `dvz_point()`, dense `position`, `color`, `diameter_px`, optional `item_state` | `include/datoviz/scene.h`, `src/scene/visuals/point/`, `examples/c/visuals/point.c` | First GSP point slice is straightforward. |
| Image visual mapping | `dvz_image()`, `dvz_visual_set_field()`, `dvz_visual_set_texture()`, sampled-field path | `include/datoviz/scene.h`, `include/datoviz/scene/field.h`, `src/scene/visuals/image/` | Retained sampled-field path is preferred over transitional wrappers. |
| Mesh/primitive mapping | `dvz_mesh()`, `dvz_primitive()`, `dvz_mesh_set_geometry()`, index buffers, textured mesh field binding | `include/datoviz/scene.h`, `src/scene/visuals/mesh/`, `src/scene/visuals/primitive/` | GSP can map basic triangle/primitive data through public C. |
| Volume first slice | `dvz_volume()`, 3D sampled field, slice/MIP/composite modes, GPU slice sample query | `include/datoviz/scene.h`, `src/scene/visuals/volume/`, `spec/scene/interaction/GPU_QUERY_SYSTEM.md` | Slice queries are active; DVR/MIP semantic query remains deferred. |
| Text/glyph first slice | `dvz_text`, `dvz_glyph()`, text lowering to glyph visual | `include/datoviz/scene/text.h`, `include/datoviz/scene.h`, `src/scene/text/`, `src/scene/visuals/glyph/` | Rendering exists; query identity is not yet GSP-ready. |
| Panel DATA/view mapping | `dvz_panel_set_domain()`, `dvz_panel_set_view2d()`, `dvz_panel_visible_domain()`, `DvzVisualAttachDesc.coord_space` | `include/datoviz/scene.h`, `include/datoviz/scene/types.h`, `src/scene/core/panel_view.c` | Matches staged transform model for first GSP 2D demos. |
| Controller mapping | `dvz_view_panzoom()`, `dvz_view_arcball()`, `dvz_view_turntable()`, `dvz_view_fly()` | `include/datoviz/app.h`, `src/scene/core/controllers.c`, controller headers | Public app helpers cover expected interaction families. |
| Visual model transform | `DvzVisualTransformDesc`, `dvz_visual_set_transform_desc()`, `dvz_visual_set_transform()` | `include/datoviz/scene.h`, `include/datoviz/scene/types.h` | Matrix/model transforms are available; nonlinear transforms remain GSP/producer-side. |
| Unified panel query API | `DvzQueryRequest`, `DvzQueryResult`, `dvz_panel_query()`, `dvz_scene_poll_query()`, `dvz_figure_process_queries()`, `dvz_panel_query_now()` | `include/datoviz/scene/interaction.h`, `include/datoviz/scene/types.h`, `src/scene/query/` | Public pick/probe split is gone. |
| Query status vocabulary | `DvzQueryStatus` distinguishes hit, miss, outside, stale, unsupported, GPU, readback, decode failures | `include/datoviz/scene/enums.h` | Matches GSP readiness requirement. |
| Broad GPU query implementation | point, pixel, marker, sphere, segment/path/stroke, primitive, mesh, image, labels, volume query files | `src/scene/query/`, `src/scene/visuals/*/query.c`, `src/scene/tests/query.c` | Several families are active; payload completeness varies. |
| Offscreen raster capture | `dvz_view_offscreen()`, `dvz_view_render_once()`, `dvz_view_capture_png()` | `include/datoviz/app.h`, `src/app/app.c`, `examples/c/runtime/offscreen_capture.c` | PNG capture is sRGB RGBA8, matching current release policy. |
| Raw ctypes generation | `datoviz.raw`, `tools/bindings/ctypes_smoke.py`, `tools/bindings/ctypes_render_smoke.py` | `spec/bindings/README.md`, `spec/bindings/ctypes.yml`, `testing/test_ctypes_raw_smoke.py` | Exact raw layer exists; GSP-specific smoke coverage should be added. |
| WASM-friendly scene API policy | Opaque handles, POD descriptors/results, fixed-width ids/counts, backend-neutral scene headers | `spec/scene/api/WASM_PORTABILITY.md`, public scene headers | GSP work should follow the existing policy. |


## Needs Datoviz Change

Baseline gaps from the initial audit. Items addressed by the implementation checkpoint above are no
longer open RC1 blockers.

| Gap | Impact on GSP | Recommended Datoviz task | Before RC1? |
|---|---|---|---|
| No public live runtime/view capability query | Adapter cannot query actual native/offscreen/WebGPU constraints without private runtime or duplicated heuristics | `DVZ-GSP-001-capability-query.md` | Yes |
| Stable public IDs are returned by queries but not exposed for created scene objects | Adapter cannot reliably map a created `DvzVisual*`/`DvzPanel*` to query result ids using public C only | `DVZ-GSP-002-stable-identity.md` | Yes |
| No public user/protocol ID attachment policy | Adapter must maintain side maps; query/diagnostic round-trip to GSP ids is unclear | `DVZ-GSP-002-stable-identity.md` plus consultation | Decide before RC1, implementation may be minimal |
| `DvzDiagnosticReport` is string-only | Diagnostics are retrievable but not structured by subject kind/id/severity | `DVZ-GSP-002-stable-identity.md` or follow-up gap | Not required for first GSP demo, but should be classified |
| Ctypes smoke does not exercise capability/query/GSP visual path | Raw binding exists, but GSP-critical structs/functions may regress unnoticed | `DVZ-GSP-004-ctypes-readiness.md` | Yes |
| Visual-family mapping is spread across headers/source/tests | Adapter authors must inspect internals to determine supported attributes/query payloads | `DVZ-GSP-005-visual-family-mapping.md` | Yes |
| Query contract is broad but per-family payload completeness is uneven | GSP could overpromise mesh face, text/glyph, labels, volume, or image payloads | `DVZ-GSP-003-query-contract.md` and `DVZ-GSP-005-visual-family-mapping.md` | Yes for documented first slice |
| Runtime capability profile selection lacks public live caps path | Query profile tests exist, but real runtime selection is hidden in app draw | `DVZ-GSP-001-capability-query.md` and `DVZ-GSP-003-query-contract.md` | Yes |


## Can Be Handled In GSP Adapter

| GSP need | Adapter-owned approach | Datoviz dependency |
|---|---|---|
| High-level Python object model | Maintain GSP scene/session/layer objects and translate to Datoviz public C calls | Public handles and stable ids |
| Protocol ID registry | Keep GSP ids in adapter maps unless Datoviz adds explicit user ids | Public Datoviz object id getters or stable mapping hooks |
| NumPy dtype/shape validation | Validate arrays before calling raw ctypes or Datoviz top-level facade | `dvz_visual_set_data*`, sampled fields, scene buffers |
| Nonlinear transforms/projections | Apply unsupported nonlinear or projection transforms in GSP/producer code before upload for v0.4; ordinary 2D panel mapping should upload DATA coordinates and attach with `DVZ_COORD_DATA` | Datoviz DATA/VIEW/PANEL coordinates and matrix transforms |
| Vector export | Route to Matplotlib/reference backend for early GSP | Datoviz raster capture only |
| JSON/protocol serialization | Keep in GSP; local Datoviz path should use pointers/buffers | Public C ABI |
| Virtual/out-of-core data policy | Decide streaming/cache behavior in GSP | Datoviz region/range update APIs |
| Frontend hover throttling/latest-wins request policy | Coalesce UI events and submit query requests | `request_id`, `freshness_serial`, query stale statuses |
| WebGPU backend selection | Gate by capabilities and route unsupported features explicitly | Capability query and query profile fields |


## Unsafe Or Unclear

| Issue | Why unsafe or unclear | Proposed handling |
|---|---|---|
| Adding `user_id` fields directly to every Datoviz object | ABI/API commitment crosses Datoviz/GSP ownership; hard to reverse after RC1 | Create consultation request; prefer minimal getters first unless Pro review recommends user ids |
| Treating one-based internal array indices as stable across destroy/reuse | Current `_scene_visual_public_id()` and `_scene_panel_public_id()` are stable for current retained slot lifetime, but not a documented protocol identity contract | Expose/document lifetime rules and tests before GSP depends on them |
| Relying on `DvzDiagnosticReport.messages` for machine routing | Strings are useful for humans but weak for GSP structured errors | Keep first GSP demo tolerant; plan structured diagnostic subject records separately |
| Query result payload fields beyond implemented families | Public struct has many fields; some remain future/deferred | Visual-family mapping must name supported fields per target/profile |
| `DVZ_QUERY_PROFILE_U64_2XR32` | Named but not implemented end to end for auto-selection | Keep disabled unless explicit support lands; document unsupported |
| Glyph/text query | Rendering path exists but object/glyph identity query remains incomplete | Do not advertise as first-slice GSP query support |
| Volume DVR/MIP query semantics | Spec says slice first; MIP/composite query should return unsupported until exact GPU policy lands | Keep GSP volume query limited to slice/sample paths |
| Live external scene buffers without CPU data | API can register labels, but adapter/runtime resource fulfillment needs exact ownership tests | Keep out of first GSP slice unless a focused compute/live-buffer demo needs it |
| WebGPU/WASM parity for query/readback | WGSL shaders and DRP2 fields exist, but WebGPU query preflight is still not full native parity | Capability-gate all browser query paths and keep unsupported explicit |


## Recommended Datoviz Task List

1. Completed: `DVZ-GSP-001-capability-query.md`.
2. Completed: `DVZ-GSP-002-stable-identity.md`.
3. Completed first contract slice: `DVZ-GSP-003-query-contract.md`.
4. Completed raw smoke slice: `DVZ-GSP-004-ctypes-readiness.md`.
5. Completed: `DVZ-GSP-005-visual-family-mapping.md`.
6. Completed: `DVZ-GSP-006-public-c-smoke.md`.


## Recommended GSP-Side Task List

1. Build the first `gsp-backend-datoviz` adapter around public C/raw ctypes only.
2. Maintain a protocol-id to Datoviz-handle/id registry in the adapter until Datoviz resolves user-id policy.
3. Implement point and image layer mapping first; defer mesh, volume, and text until the Datoviz visual-family table marks the exact payloads supported.
4. Keep NumPy/memoryview upload paths direct through `dvz_visual_set_data*`, `dvz_sampled_field_*`, and `dvz_scene_buffer_set_data()`.
5. Gate every optional visual/query/export path by the Datoviz capability snapshot.
6. Treat vector export as non-Datoviz/backend-neutral GSP responsibility.
7. Add adapter tests that compare Datoviz query results to GSP protocol object ids through the adapter mapping table.


## Files, Functions, And Types Inspected

Specs:

| File | Purpose in audit |
|---|---|
| `spec/scene/README.md` | scene scope and source-of-truth orientation |
| `spec/scene/AUTHORITY.md` | authority order and invariants |
| `spec/scene/core/RUNTIME_BOUNDARY.md` | runtime/capability/readback boundary |
| `spec/scene/pipeline/FRAME_PLAN.md` | FramePlan/resource/readback requirements |
| `spec/scene/pipeline/TRANSFORM_PIPELINE.md` | transform and coordinate semantics |
| `spec/scene/validation/ADAPTATION.md` | capability adaptation outcomes |
| `spec/scene/interaction/PANEL_QUERY.md` | public query model |
| `spec/scene/interaction/GPU_QUERY_SYSTEM.md` | detailed query implementation contract |
| `spec/scene/proposals/active/GSP_COMPATIBILITY_PREP.md` | GSP readiness target |
| `spec/drp2/CAPABILITIES.md` | backend-neutral runtime snapshot requirements |
| `spec/scene/api/API_IMPLEMENTATION_READINESS.md` | current implementation readiness summary |
| `spec/scene/api/WASM_PORTABILITY.md` | public API portability constraints |
| `spec/bindings/README.md` | raw ctypes architecture |
| `spec/bindings/ctypes.yml` | raw ctypes policy and smoke scope |

Public headers:

| File | Functions/types inspected |
|---|---|
| `include/datoviz/scene.h` | `dvz_scene`, `dvz_figure`, `dvz_panel`, `dvz_scene_set_capabilities`, `dvz_figure_emit_frame`, `DvzSceneFrameArtifact`, `dvz_figure_process_queries`, `dvz_visual_set_data`, `dvz_visual_set_data_many`, `dvz_visual_set_data_range`, `dvz_scene_buffer*`, `dvz_visual_set_buffer`, `dvz_visual_set_index_data`, `dvz_visual_set_attr_buffer`, visual constructors |
| `include/datoviz/scene/types.h` | `DvzCapabilitySnapshot`, `DvzDiagnosticReport`, `DvzVisualTransformDesc`, `DvzVisualAttachDesc`, `DvzQueryRequest`, `DvzQueryResult`, `DvzSelectionItem`, `DvzDataDomain`, `DvzPanelView2D` |
| `include/datoviz/scene/enums.h` | `DvzQueryStatus`, `DvzQueryProfile`, `DvzQueryValueKind`, `DvzQueryCapabilityFlag`, visual/target/coordinate enums |
| `include/datoviz/scene/interaction.h` | `dvz_visual_set_query_capabilities`, `dvz_query_request`, `dvz_panel_query`, `dvz_scene_poll_query`, `dvz_panel_query_now`, interaction/selection/hover helpers |
| `include/datoviz/scene/frame_plan.h` | `dvz_capability_snapshot`, `dvz_capability_snapshot_copy`, `DvzFramePlanCopyDesc`, diagnostic helpers |
| `include/datoviz/scene/field.h` | `DvzSampledField`, `DvzSampledFieldDesc`, `DvzFieldDataView`, `dvz_sampled_field_set_data`, `dvz_sampled_field_update_region`, `dvz_visual_set_field` |
| `include/datoviz/scene/scale.h` | scale/colormap/unit object APIs used by image/volume/readout mapping |
| `include/datoviz/app.h` | `DvzApp`, `DvzView`, `dvz_app*`, `dvz_view*`, `dvz_view_render_once`, `dvz_view_capture_png`, controller helper constructors |

Source and tests:

| File or group | Functions/behavior inspected |
|---|---|
| `src/app/app.c` | `_app_apply_runtime_caps()`, app render path capability construction, `dvz_figure_process_queries()` call after runtime execution |
| `src/scene/frame_plan/capabilities.c` | `dvz_capability_snapshot()`, ABI validation, default snapshot fields |
| `src/scene/query/` | query queue, freshness, profile selection, execution, readback, result mapping |
| `src/scene/visuals/*/query.c` | per-family query implementation layout and active families |
| `src/scene/visuals/families.c` | `_scene_visual_public_id()` |
| `src/scene/interaction/hit_test.c` | `_scene_panel_public_id()` |
| `src/scene/core/frame_trace.c` | `_scene_figure_id()` |
| `src/scene/domain/buffer.c` | scene buffer allocation/copy/resource-key behavior |
| `src/scene/domain/field*.c` | sampled field data and dirty region behavior |
| `src/scene/core/panel_view.c` | panel domain/view extent resolution |
| `src/scene/tests/query.c` | query status/profile/family coverage, point-over-image-style dual query coverage |
| `src/scene/tests/app.c` | offscreen render/query/capture behavior |
| `testing/test_ctypes_raw_smoke.py` | raw ctypes smoke entry points |
| `tools/bindings/ctypes_smoke.py` | generated raw import/layout smoke scope |
| `tools/bindings/ctypes_render_smoke.py` | offscreen raw ctypes smoke path |
| `examples/c/runtime/offscreen_capture.c` | public offscreen render/capture example |
| `examples/c/features/update_visual_data.c` | public partial visual update example |
| `examples/c/visuals/*.c` | per-family public usage examples |


## ChatGPT Pro Consultation Requests

Answered:

1. `spec/scene/proposals/active/gsp_tasks/CHATGPT_PRO_CONSULTATION_001.md`

Decision: use the hybrid RC1 path. Datoviz exposes opaque scene-local `DvzId` getters for retained
objects that can appear in query, selection, diagnostics, or adapter maps. GSP owns all protocol-id
maps. Do not add Datoviz `user_id`/protocol-id setters or user-id fields in `DvzQueryResult` for
RC1.
