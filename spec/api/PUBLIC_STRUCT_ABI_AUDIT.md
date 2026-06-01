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
| `DvzCameraDesc` | `dvz_camera_desc()` |
| `DvzPanzoomDesc` | `dvz_panzoom_desc()` |
| `DvzArcballDesc` | `dvz_arcball_desc()` |
| `DvzFlyDesc` | `dvz_fly_desc()` |
| `DvzTurntableDesc` | `dvz_turntable_desc()` |
| `DvzOrbitCameraDesc` | `dvz_orbit_camera_desc()` |
| `DvzEdlDesc` | `dvz_edl_desc()` |
| `DvzMsaaDesc` | `dvz_msaa_desc()` |
| `DvzSsaoDesc` | `dvz_ssao_desc()` |
| `DvzVolumeOcclusionDesc` | `dvz_volume_occlusion_desc()` |
| `DvzSceneOcclusionDesc` | `dvz_scene_occlusion_desc()` |
| `DvzMaterialDesc` | `dvz_material_desc()` |
| `DvzDepthCueDesc` | `dvz_depth_cue_desc()` |
| `DvzPointStyleDesc` | `dvz_point_style_desc()` |
| `DvzVectorStyle` | `dvz_vector_style()` |
| `DvzMarkerStyle` | `dvz_marker_style()` |
| `DvzSelectionDesc` | `dvz_selection_desc()` |
| `DvzHoverDesc` | `dvz_hover_desc()` |
| `DvzItemStateVisualStyle` | `dvz_item_state_visual_style()` |
| `DvzSelectionVisualStyle` | `dvz_selection_visual_style()` |
| `DvzItemInteractionDesc` | `dvz_item_interaction_desc()` |
| `DvzFormatDesc` | `dvz_format_desc()` |
| `DvzScaleDesc` | `dvz_scale_desc()` |
| `DvzColormapDesc` | `dvz_colormap_desc()` |
| `DvzColorbarDesc` | `dvz_colorbar_desc()` |
| `DvzLegendDesc` | `dvz_legend_desc()` |
| `DvzAxisTickPolicy` | `dvz_axis_tick_policy()` |
| `DvzAxisStyle` | `dvz_axis_style()` |
| `DvzFontDesc` | `dvz_font_desc()` |
| `DvzFontDefaults` | `dvz_font_defaults()` |
| `DvzTextStyle` | `dvz_text_style()` |
| `DvzTextPlacement` | `dvz_text_placement()` |
| `DvzAnnotationDesc` | `dvz_annotation_desc()` |
| `DvzLabelDesc` | `dvz_label_desc()` |
| `DvzScaleBarDesc` | `dvz_scalebar_desc()` |
| `DvzFieldGeometry` | `dvz_field_geometry()` |
| `DvzFieldDataView` | `dvz_field_data_view()` |
| `DvzOverlayCardStyle` | `dvz_overlay_card_style()` |
| `DvzOverlayCardDesc` | `dvz_overlay_card_desc()` |
| `DvzOverlayRichTextDesc` | `dvz_overlay_rich_text_desc()` |
| `DvzGuiConfig` | `dvz_gui_config()` |
| `DvzGuiViewportConfig` | `dvz_gui_viewport_config()` |


## Should Add Before API Freeze

These structs are public, caller-authored, passed by pointer to public create/configure functions,
and likely to grow or gain flags after v0.4:

| Area | Structs | Public consumers |
| --- | --- | --- |
| Scene techniques | `DvzCapabilitySnapshot` | capability-aware setup paths |
| Visual styles/materials | `DvzVisualDataView` | data-view setters |
| Canvas/stream | `DvzCanvasLiveImageSinkConfig`, `DvzStreamFrame`, `DvzStreamSinkBackend` | `dvz_canvas_configure_live_image_sink()`, stream frame and sink backend APIs |
| Host integration | `DvzWindowExternalSurfaceInfo`, `DvzWindowBackendProcs`, `DvzWindowBackend`, `DvzWindowGlfwInputCallbacks` | external surface, custom backend, and GLFW callback registration APIs |
| Advanced runtime interop | `DvzDrp2ExternalBufferDesc`, `DvzDrp2RecordingInfo`, `DvzInteropBufferExportConfig` | `dvz_drp2_runtime_register_external_buffer()`, DRP2 recording APIs, `dvz_interop_buffer_export()` |
| Geometry utilities | `DvzGeometryCubeDesc`, `DvzGeometryPlaneDesc`, `DvzGeometrySphereDesc`, `DvzGeometrySurfaceGridDesc`, `DvzPolygonDesc`, `DvzTriangulationDesc` | geometry constructors and triangulation helpers |
| Animation | `DvzAnimPhaseDesc` | `dvz_anim_phase()` |

Recommended batching:

1. scene techniques and visual styles;
2. text, annotations, and overlay;
3. GUI and advanced runtime interop;
4. geometry utilities.


## No Prologue Needed

These public structs are fixed data records, event payloads, output/result structs, simple by-value
placement/data views, or internal container/runtime records. They do not need the ABI prologue unless
they later become pointer-passed growable descriptors:

| Category | Examples |
| --- | --- |
| Event payloads | `DvzInputResizeEvent`, `DvzInputScaleEvent`, `DvzKeyboardEvent`, `DvzPointerEvent`, `DvzPointerWheelEvent`, `DvzPointerDragEvent` |
| Result/output structs | `DvzGpuInfo`, `DvzDrp2ValidationResult`, `DvzQueryResult`, `DvzHoverState`, `DvzFramePlanPacketResult`, `DvzInteropBufferExport` |
| Geometry/data records | `DvzBounds`, `DvzRect`, `DvzPanelDesc`, `DvzGridCell`, `DvzPanelReserve`, `DvzDataDomain`, `DvzPlacement`, `DvzFieldRegion` |
| Category/color records | `DvzVolumeAlphaStop`, `DvzColor`, `DvzColorf`, `DvzTime` |
| Internal/container records exposed for low-level use | `DvzObject`, `DvzContainer`, `DvzContainerIterator`, `DvzList`, `DvzQueue`, `DvzQueues`, `DvzBarriers`, `DvzSubmit` |


## Ambiguous

These structs are public and pointer-passed, but their role is closer to batch row data or low-level
protocol records than a growable user descriptor. Keep them out of the first ABI-prologue conversion
unless the owning module wants them in the stable v0.4 surface:

| Struct | Reason |
| --- | --- |
| `DvzDrp2BindGroupLayoutEntry`, `DvzDrp2BindGroupEntry`, `DvzDrp2ColorAttachment`, `DvzDrp2ColorTarget` | DRP2 command data records; protocol versioning may be preferable to per-struct ABI prologues. |
| `DvzVisualDataUpdate`, `DvzScaleCategory`, `DvzColormapStop` | Batch row elements; adding a prologue to every element would materially affect memory layout and upload ergonomics. |
| `DvzDrp2RecordedFrame`, `DvzDrp2RawFallback` | Recording result/fallback records rather than caller-authored descriptors. |
| `DvzImageBlit`, `DvzImageCopy`, `DvzSwapchainConfig` | Low-level vklite records currently used by value or internal paths; revisit only if promoted as stable public setup descriptors. |
