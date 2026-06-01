# Public Struct ABI Audit

Status: v0.4 pre-freeze audit. Updated: 2026-06-01.

This audit applies the rule from `PUBLIC_API_CONVENTIONS.md`: growable public descriptors and
configs passed by pointer into public entry points should start with `uint32_t struct_size` and
`uint32_t flags`, and should have a canonical initializer.


## Already Covered

These public caller-authored structs already have the ABI prologue and initializer coverage:

| Struct | Public initializer |
| --- | --- |
| `DvzAppConfig` | `dvz_app_config()` |
| `DvzAppCaptureConfig` | `dvz_app_capture_config()` |
| `DvzAppResources` | `dvz_app_resources()` |
| `DvzCanvasConfig` | `dvz_canvas_config()` |
| `DvzWindowConfig` | `dvz_window_config()` |
| `DvzInstanceConfig` | `dvz_instance_config()` |
| `DvzDeviceConfig` | `dvz_device_config()` |
| `DvzGpuCtxConfig` | `dvz_gpu_ctx_config()` |
| `DvzStreamConfig` | `dvz_stream_config()` |
| `DvzVideoEncoderConfig` | `dvz_video_encoder_config()` |
| `DvzVideoSinkConfig` | `dvz_video_sink_config()` |
| `DvzDrp2RuntimeConfig` | `dvz_drp2_runtime_vklite_config()` |
| `DvzFramePlanEmitConfig` | `dvz_frame_plan_emit_config()` |
| `DvzFramePlanCopyDesc` | `dvz_frame_plan_copy_desc()` |
| `DvzPanelBackgroundDesc` | `dvz_panel_background_desc()` |
| `DvzSceneBufferDesc` | `dvz_scene_buffer_desc()` |
| `DvzSceneComputeDesc` | `dvz_scene_compute_desc()` |
| `DvzVisualAttachDesc` | `dvz_visual_attach_desc()` |
| `DvzQueryRequest` | `dvz_query_request()` |
| `DvzSampledFieldDesc` | `dvz_sampled_field_desc()` |


## Should Add Before API Freeze

These structs are public, caller-authored, passed by pointer to public create/configure functions,
and likely to grow or gain flags after v0.4:

| Area | Structs | Public consumers |
| --- | --- | --- |
| Controllers/camera | `DvzCameraDesc`, `DvzPanzoomDesc`, `DvzArcballDesc`, `DvzFlyDesc`, `DvzTurntableDesc`, `DvzOrbitCameraDesc` | `dvz_camera_create()`, `dvz_panel_set_camera()`, `dvz_panzoom()`, `dvz_arcball()`, `dvz_fly()`, `dvz_turntable()`, `dvz_orbit_camera()`, `dvz_view_*()` controller helpers |
| Scene techniques | `DvzEdlDesc`, `DvzMsaaDesc`, `DvzSsaoDesc`, `DvzVolumeOcclusionDesc`, `DvzSceneOcclusionDesc` | `dvz_panel_set_edl()`, `dvz_panel_set_msaa()`, `dvz_panel_set_ssao()`, `dvz_panel_set_volume_occlusion()`, `dvz_panel_set_scene_occlusion()` |
| Visual styles/materials | `DvzMaterialDesc`, `DvzDepthCueDesc`, `DvzPointStyleDesc`, `DvzVectorStyle`, `DvzMarkerStyle` | `dvz_visual_set_material()`, `dvz_visual_set_depth_cue()`, `dvz_point_set_style()`, `dvz_vector_set_style()`, `dvz_marker_set_style()` |
| Interaction | `DvzSelectionDesc`, `DvzHoverDesc`, `DvzItemInteractionDesc` | `dvz_selection()`, `dvz_hover()`, `dvz_item_interaction()` |
| Scales and guides | `DvzScaleDesc`, `DvzColormapDesc`, `DvzColorbarDesc`, `DvzLegendDesc`, `DvzFormatDesc` | `dvz_scale()`, `dvz_colormap()`, `dvz_colorbar()`, `dvz_legend()`, format setters |
| Text and annotations | `DvzFontDesc`, `DvzTextStyle`, `DvzTextPlacement`, `DvzAnnotationDesc`, `DvzLabelDesc`, `DvzScaleBarDesc` | `dvz_font()`, text/annotation/label/scalebar creation and setters |
| Overlay | `DvzOverlayCardStyle`, `DvzOverlayCardDesc`, `DvzOverlayRichTextDesc` | `dvz_overlay_card()`, `dvz_overlay_card_set_style()`, `dvz_overlay_card_set_rich_text()` |
| GUI | `DvzGuiConfig`, `DvzGuiViewportConfig` | `dvz_view_gui()`, `dvz_gui_viewport()`, `dvz_gui_viewport_from_view()` |
| Advanced runtime interop | `DvzDrp2ExternalBufferDesc`, `DvzInteropBufferExportConfig` | `dvz_drp2_runtime_register_external_buffer()`, `dvz_interop_buffer_export()` |
| Geometry utilities | `DvzGeometryCubeDesc`, `DvzGeometryPlaneDesc`, `DvzGeometrySphereDesc`, `DvzGeometrySurfaceGridDesc`, `DvzPolygonDesc`, `DvzTriangulationDesc` | geometry constructors and triangulation helpers |
| Animation | `DvzAnimPhaseDesc` | `dvz_anim_phase()` |

Recommended batching:

1. controllers/camera;
2. scene techniques and visual styles;
3. interaction, scales, text, annotations, and overlay;
4. GUI and advanced runtime interop;
5. geometry utilities.


## No Prologue Needed

These public structs are fixed data records, event payloads, output/result structs, simple by-value
placement/data views, or internal container/runtime records. They do not need the ABI prologue unless
they later become pointer-passed growable descriptors:

| Category | Examples |
| --- | --- |
| Event payloads | `DvzInputResizeEvent`, `DvzInputScaleEvent`, `DvzKeyboardEvent`, `DvzPointerEvent`, `DvzPointerWheelEvent`, `DvzPointerDragEvent` |
| Result/output structs | `DvzGpuInfo`, `DvzDrp2ValidationResult`, `DvzQueryResult`, `DvzHoverState`, `DvzFramePlanPacketResult`, `DvzInteropBufferExport` |
| Geometry/data records | `DvzBounds`, `DvzRect`, `DvzPanelDesc`, `DvzGridCell`, `DvzPanelReserve`, `DvzDataDomain`, `DvzPlacement`, `DvzFieldGeometry`, `DvzFieldRegion`, `DvzFieldDataView`, `DvzVisualDataView`, `DvzVisualDataUpdate` |
| Category/color records | `DvzScaleCategory`, `DvzColormapStop`, `DvzVolumeAlphaStop`, `DvzColor`, `DvzColorf`, `DvzTime` |
| Internal/container records exposed for low-level use | `DvzObject`, `DvzContainer`, `DvzContainerIterator`, `DvzList`, `DvzQueue`, `DvzQueues`, `DvzBarriers`, `DvzSubmit` |


## Ambiguous

These structs are public and pointer-passed, but their role is closer to callback/backend glue or
fixed platform interop than a growable user descriptor. Keep them out of the first ABI-prologue
conversion unless the owning module wants them in the stable v0.4 surface:

| Struct | Reason |
| --- | --- |
| `DvzWindowExternalSurfaceInfo` | Platform handle bundle for host integration and FFI helpers; likely needs a separate ownership/lifetime review. |
| `DvzWindowBackendProcs`, `DvzWindowBackend`, `DvzWindowGlfwInputCallbacks` | Backend registration glue, not ordinary end-user descriptors. |
| `DvzDrp2RecordingInfo`, `DvzDrp2RecordedFrame`, `DvzDrp2RawFallback` | Recording metadata/result records; decide with DVZR stability. |
| `DvzDrp2BindGroupLayoutEntry`, `DvzDrp2BindGroupEntry`, `DvzDrp2ColorAttachment`, `DvzDrp2ColorTarget` | DRP2 command data records; protocol versioning may be preferable to per-struct ABI prologues. |
| `DvzImageBlit`, `DvzImageCopy`, `DvzSwapchainConfig` | Low-level vklite records currently used by value or internal paths; revisit only if promoted as stable public setup descriptors. |
