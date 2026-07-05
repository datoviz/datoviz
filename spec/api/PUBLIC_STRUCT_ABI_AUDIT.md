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
| `DvzCanvasLiveImageSinkConfig` | `dvz_canvas_live_image_sink_config()` |
| `DvzWindowConfig` | `dvz_window_config()` |
| `DvzWindowExternalSurfaceInfo` | `dvz_window_external_surface_info()` |
| `DvzInstanceConfig` | `dvz_instance_config()` |
| `DvzDeviceConfig` | `dvz_device_config()` |
| `DvzGpuCtxConfig` | `dvz_gpu_ctx_config()` |
| `DvzStreamConfig` | `dvz_stream_config()` |
| `DvzVideoEncoderConfig` | `dvz_video_encoder_config()` |
| `DvzVideoSinkConfig` | `dvz_video_sink_config()` |
| `DvzDrp2RuntimeConfig` | `dvz_drp2_runtime_vklite_config()` |
| `DvzDrp2ExternalBufferDesc` | `dvz_drp2_external_buffer_desc()` |
| `DvzDrp2RecordingInfo` | `dvz_drp2_recording_info()` |
| `DvzInteropBufferExportConfig` | `dvz_interop_buffer_export_config()` |
| `DvzFramePlanEmitConfig` | `dvz_frame_plan_emit_config()` |
| `DvzFramePlanCopyDesc` | `dvz_frame_plan_copy_desc()` |
| `DvzCapabilitySnapshot` | `dvz_capability_snapshot()` |
| `DvzVisualTransformDesc` | `dvz_visual_transform_desc()` |
| `DvzVisualShaderDesc` | `dvz_visual_shader_desc()` |
| `DvzGeometryCubeDesc` | `dvz_geometry_cube_desc()` |
| `DvzGeometryPlaneDesc` | `dvz_geometry_plane_desc()` |
| `DvzGeometrySphereDesc` | `dvz_geometry_sphere_desc()` |
| `DvzGeometrySurfaceGridDesc` | `dvz_geometry_surface_grid_desc()` |
| `DvzGeometryDiscDesc` | `dvz_geometry_disc_desc()` |
| `DvzGeometrySectorDesc` | `dvz_geometry_sector_desc()` |
| `DvzGeometryRegularPolygonDesc` | `dvz_geometry_regular_polygon_desc()` |
| `DvzGeometryStarDesc` | `dvz_geometry_star_desc()` |
| `DvzGeometryCylinderDesc` | `dvz_geometry_cylinder_desc()` |
| `DvzGeometryConeDesc` | `dvz_geometry_cone_desc()` |
| `DvzGeometryTorusDesc` | `dvz_geometry_torus_desc()` |
| `DvzGeometryArrowDesc` | `dvz_geometry_arrow_desc()` |
| `DvzGeometryObjDesc` | `dvz_geometry_obj_desc()` |
| `DvzPolygonDesc` | `dvz_polygon_desc()` |
| `DvzTriangulationDesc` | `dvz_triangulation_desc()` |
| `DvzPanelBackgroundDesc` | `dvz_panel_background_desc()` |
| `DvzPanelBorderDesc` | `dvz_panel_border_desc()` |
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
| `DvzScaleBarDesc` | `dvz_scale_bar_desc()` |
| `DvzFieldGeometry` | `dvz_field_geometry()` |
| `DvzFieldDataView` | `dvz_field_data_view()` |
| `DvzOverlayCardStyle` | `dvz_overlay_card_style()` |
| `DvzOverlayCardDesc` | `dvz_overlay_card_desc()` |
| `DvzOverlayRichTextDesc` | `dvz_overlay_rich_text_desc()` |
| `DvzGuiConfig` | `dvz_gui_config()` |
| `DvzGuiViewportConfig` | `dvz_gui_viewport_config()` |
| `DvzAnimPhaseDesc` | `dvz_anim_phase_desc()` |


## Should Add Before API Freeze

No remaining caller-authored growable descriptors are identified for the v0.4 API freeze.


## No Prologue Needed

These public structs are fixed data records, event payloads, output/result structs, simple by-value
placement/data views, or internal container/runtime records. They do not need the ABI prologue unless
they later become pointer-passed growable descriptors:

| Category | Examples |
| --- | --- |
| Event payloads | `DvzInputResizeEvent`, `DvzInputScaleEvent`, `DvzKeyboardEvent`, `DvzPointerEvent`, `DvzPointerWheelEvent`, `DvzPointerDragEvent` |
| Result/output structs | `DvzGpuInfo`, `DvzDrp2ValidationResult`, `DvzQueryResult`, `DvzHoverState`, `DvzFramePlanPacketResult`, `DvzInteropBufferExport`, `DvzVisualDataView` |
| Geometry/data records | `DvzBounds`, `DvzRect`, `DvzPanelDesc`, `DvzGridCell`, `DvzPanelReserve`, `DvzDataDomain`, `DvzPlacement`, `DvzFieldRegion` |
| Category/color records | `DvzVolumeAlphaStop`, `DvzColor`, `DvzColorf`, `DvzTime` |
| Internal/container records exposed for low-level use | `DvzObject`, `DvzContainer`, `DvzContainerIterator`, `DvzQueue`, `DvzQueues`, `DvzBarriers`, `DvzSubmit` |
| Borrowed runtime records | `DvzStreamFrame` |


## Ambiguous

These structs are public and pointer-passed, but their role is closer to batch row data or low-level
protocol records than a growable user descriptor. Keep them out of the first ABI-prologue conversion
unless the owning module wants them in the stable v0.4 surface:

| Struct | Reason |
| --- | --- |
| `DvzDrp2BindGroupLayoutEntry`, `DvzDrp2BindGroupEntry`, `DvzDrp2ColorAttachment`, `DvzDrp2ColorTarget` | DRP2 command data records; protocol versioning may be preferable to per-struct ABI prologues. |
| `DvzVisualDataUpdate`, `DvzScaleCategory`, `DvzColormapStop` | Batch row elements; adding a prologue to every element would materially affect memory layout and upload ergonomics. |
| `DvzDrp2RecordedFrame` | Recording result record rather than a caller-authored descriptor. |
| `DvzStreamSinkBackend`, `DvzWindowBackendProcs`, `DvzWindowBackend`, `DvzWindowGlfwInputCallbacks` | Callback/vtable registration records. Keep their fixed layout for now; revisit only if the backend plugin surface becomes a versioned public extension API. |
| `DvzImageBlit`, `DvzImageCopy`, `DvzSwapchainConfig` | Low-level vklite records currently used by value or internal paths; revisit only if promoted as stable public setup descriptors. |
