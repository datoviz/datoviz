export const LIVE_EXAMPLES = [
  {
    id: "start_scatter",
    label: "Scatter Plot",
    scenarioId: "start_scatter",
  },
  {
    id: "features_datetime_axis",
    label: "Datetime Axis",
    scenarioId: "features_datetime_axis",
    animate: true,
  },
  {
    id: "features_marker_symbols",
    label: "Marker Symbols",
    scenarioId: "features_marker_symbols",
  },
  {
    id: "composites_graph",
    label: "Graph Composite",
    scenarioId: "composites_graph",
  },
  {
    id: "features_orientation_gizmo",
    label: "Orientation Gizmo",
    scenarioId: "features_orientation_gizmo",
    animate: true,
  },
  {
    id: "features_probe_labels",
    label: "Label Probe",
    scenarioId: "features_probe_labels",
  },
  {
    id: "features_basic_scene",
    label: "Basic Scene",
    scenarioId: "features_basic_scene",
  },
  {
    id: "features_timer_animation",
    label: "Timer Animation",
    scenarioId: "features_timer_animation",
    animate: true,
  },
  {
    id: "features_builtin_shapes_2d",
    label: "Builtin Shapes 2D",
    scenarioId: "features_builtin_shapes_2d",
  },
  {
    id: "features_builtin_shapes_3d",
    label: "Builtin Shapes 3D",
    scenarioId: "features_builtin_shapes_3d",
  },
  {
    id: "features_isolines",
    label: "Isolines",
    scenarioId: "features_isolines",
  },
  {
    id: "features_animation_tracks",
    label: "Animation Tracks",
    scenarioId: "features_animation_tracks",
    animate: true,
  },
  {
    id: "features_compute_buffer_animation",
    label: "Compute Buffer Animation",
    scenarioId: "features_compute_buffer_animation",
    animate: true,
  },
  {
    id: "features_obj_loading",
    label: "OBJ Loading",
    scenarioId: "features_obj_loading",
  },
  {
    id: "features_picking",
    label: "Picking",
    scenarioId: "features_picking",
  },
  {
    id: "features_selection_pixel",
    label: "Pixel Selection",
    scenarioId: "features_selection_pixel",
    animate: true,
  },
  {
    id: "features_selection_sphere",
    label: "Sphere Selection",
    scenarioId: "features_selection_sphere",
    animate: true,
  },
  {
    id: "features_selection_mesh_instances",
    label: "Mesh Instance Selection",
    scenarioId: "features_selection_mesh_instances",
    animate: true,
  },
  {
    id: "features_image_probe",
    label: "Image Probe",
    scenarioId: "features_image_probe",
  },
  {
    id: "features_colorbar",
    label: "Colorbar",
    scenarioId: "features_colorbar",
  },
  {
    id: "features_scalebar",
    label: "Scale Bar",
    scenarioId: "features_scalebar",
  },
  {
    id: "features_scalebar_units",
    label: "Scale Bar Units",
    scenarioId: "features_scalebar_units",
  },
  {
    id: "features_legend_categorical",
    label: "Categorical Legend",
    scenarioId: "features_legend_categorical",
  },
  {
    id: "features_annotation_readout",
    label: "Annotation Readout",
    scenarioId: "features_annotation_readout",
  },
  {
    id: "showcases_linked_probe_colorbar",
    label: "Linked Probe With Colorbar",
    scenarioId: "showcases_linked_probe_colorbar",
  },
  {
    id: "showcases_scientific_plotting",
    label: "Scientific Plotting Workflow",
    scenarioId: "showcases_scientific_plotting",
  },
  {
    id: "visuals_vector",
    label: "Vector",
    scenarioId: "visuals_vector",
  },
  {
    id: "showcases_wind_field",
    label: "Wind Field",
    scenarioId: "showcases_wind_field",
    animate: true,
  },
  {
    id: "showcases_gpu_particle_smoke",
    label: "GPU Particle Smoke",
    scenarioId: "showcases_gpu_particle_smoke",
    animate: true,
  },
  {
    id: "features_panel_single",
    label: "Single Panel",
    scenarioId: "features_panel_single",
  },
  {
    id: "features_panel_grid",
    label: "Panel Grid",
    scenarioId: "features_panel_grid",
  },
  {
    id: "features_panzoom",
    label: "Panzoom",
    scenarioId: "features_panzoom",
  },
  {
    id: "features_axes_2d",
    label: "Path With 2D Axes",
    scenarioId: "features_axes_2d",
  },
  {
    id: "features_axis_labels",
    label: "Axis Labels",
    scenarioId: "features_axis_labels",
  },
  {
    id: "features_sampled_field_update",
    label: "Sampled Field Update",
    scenarioId: "features_sampled_field_update",
    animate: true,
  },
  {
    id: "features_colormap_scale",
    label: "Scalar Color Scale",
    scenarioId: "features_colormap_scale",
  },
  {
    id: "features_panel_background",
    label: "Panel Background",
    scenarioId: "features_panel_background",
  },
  {
    id: "composites_polygon",
    label: "Polygon Composite",
    scenarioId: "composites_polygon",
  },
  {
    id: "showcases_panel_linked_axes",
    label: "Linked Panels With Axes",
    scenarioId: "showcases_panel_linked_axes",
  },
  {
    id: "showcases_scalebar_measurement",
    label: "Scale Bar Measurement Workflow",
    scenarioId: "showcases_scalebar_measurement",
  },
  {
    id: "showcases_surface_grid",
    label: "Surface Grid",
    scenarioId: "showcases_surface_grid",
  },
  {
    id: "showcases_point_cloud",
    label: "Point Cloud",
    scenarioId: "showcases_point_cloud",
    effectLimitations: [
      {
        effect: "edl",
        status: "unavailable",
        warning: "The desktop example enables eye-dome lighting; the WebGPU route uses direct point rendering.",
      },
    ],
    dataBundles: [
      {
        id: "point_cloud",
        url: "../../webgpu-data/examples/point_cloud/sha256-ad5b997813ca275a/manifest.json",
        virtualRoot: "data/examples/point_cloud",
        required: true,
      },
    ],
  },
  {
    id: "showcases_svg_tiger",
    label: "SVG Tiger",
    scenarioId: "showcases_svg_tiger",
    effectLimitations: [
      {
        effect: "msaa",
        status: "unavailable",
        warning: "The desktop example enables MSAA for smoother fill and outline edges; the WebGPU route is currently single-sampled.",
      },
    ],
    dataBundles: [
      {
        id: "svg_tiger",
        url: "../../webgpu-data/examples/svg_tiger/sha256-640a536d848ff2eb/manifest.json",
        virtualRoot: "data/examples/svg_tiger",
        required: true,
      },
    ],
  },
  {
    id: "showcases_terrain_relief",
    label: "McHenrys Peak Terrain Relief",
    scenarioId: "showcases_terrain_relief",
    effectLimitations: [
      {
        effect: "msaa",
        status: "limited",
        warning: "The desktop example requests 8x MSAA; the WebGPU route lowers this to the supported 4x sample count.",
      },
    ],
    dataBundles: [
      {
        id: "terrain_relief",
        url: "../../webgpu-data/examples/terrain_relief/sha256-0539b707993348c0/manifest.json",
        virtualRoot: "data/examples/terrain_relief",
        required: true,
      },
    ],
  },
  {
    id: "showcases_spherical_harmonics",
    label: "Spherical Harmonics",
    scenarioId: "showcases_spherical_harmonics",
    animate: true,
    effectLimitations: [
      {
        effect: "msaa",
        status: "unavailable",
        warning: "The desktop example enables 8x MSAA for smoother mesh silhouettes; the WebGPU route is currently single-sampled.",
      },
    ],
  },
  {
    id: "showcases_streaming_daq",
    label: "Streaming DAQ · 64 channels",
    scenarioId: "showcases_streaming_daq",
    animate: true,
  },
  {
    id: "showcases_cortical_activity",
    label: "Human Auditory Cortical Activity",
    scenarioId: "showcases_cortical_activity",
    animate: true,
    effectLimitations: [
      {
        effect: "msaa",
        status: "unavailable",
        warning: "The desktop example enables MSAA for smoother cortical mesh edges; the WebGPU route is currently single-sampled.",
      },
    ],
    dataBundles: [
      {
        id: "cortical_activity",
        url: "../../webgpu-data/examples/cortical_activity/sha256-4b8e4d31566009f3/manifest.json",
        virtualRoot: "data/examples/cortical_activity",
        required: true,
      },
    ],
  },
  {
    id: "showcases_galaxy",
    label: "Density-Wave Galaxy",
    scenarioId: "showcases_galaxy",
    animate: true,
  },
  {
    id: "showcases_choropleth",
    label: "U.S. State Choropleth",
    scenarioId: "showcases_choropleth",
  },
  {
    id: "features_update_partial",
    label: "Partial Data Update",
    scenarioId: "features_update_partial",
    animate: true,
  },
  {
    id: "features_update_visual_data",
    label: "Visual Data Update",
    scenarioId: "features_update_visual_data",
    animate: true,
  },
  {
    id: "features_visibility",
    label: "Visual Visibility",
    scenarioId: "features_visibility",
    animate: true,
  },
  {
    id: "features_technique_depth_test",
    label: "Depth Test Toggle",
    scenarioId: "features_technique_depth_test",
  },
  {
    id: "features_alpha_blending",
    label: "Alpha Blending",
    scenarioId: "features_alpha_blending",
  },
  {
    id: "features_material_mesh",
    label: "Mesh Materials",
    scenarioId: "features_material_mesh",
  },
  {
    id: "features_lighting",
    label: "Lighting",
    scenarioId: "features_lighting",
  },
  {
    id: "showcases_textured_planet",
    label: "Textured Planets and Orbital Debris",
    scenarioId: "showcases_textured_planet",
    animate: true,
    dataBundles: [
      {
        id: "orbital_debris",
        url: "../../webgpu-data/examples/orbital_debris/sha256-5ae33a50aeb8fa81/manifest.json",
        virtualRoot: "data/examples/orbital_debris",
        required: true,
      },
      {
        id: "planet_sky",
        url: "../../webgpu-data/examples/planet_sky/sha256-ca8592458acb5ae2/manifest.json",
        virtualRoot: "data/examples/planet_sky",
        required: true,
      },
    ],
  },
  {
    id: "showcases_protein",
    label: "Protein",
    scenarioId: "showcases_protein",
    effectLimitations: [
      {
        effect: "ssao",
        status: "unavailable",
        warning: "The desktop example enables SSAO; the WebGPU route currently omits this post-processing effect, so molecular creases and contacts have less ambient-occlusion shading.",
      },
    ],
  },
  {
    id: "features_panel_multi",
    label: "Multiple Panels",
    scenarioId: "features_panel_multi",
  },
  {
    id: "features_panel_linked",
    label: "Linked Panels",
    scenarioId: "features_panel_linked",
  },
  {
    id: "features_text_block",
    label: "Text Block",
    scenarioId: "features_text_block",
  },
  {
    id: "features_overlay_card",
    label: "Overlay Card",
    scenarioId: "features_overlay_card",
  },
  {
    id: "features_guide_lines",
    label: "Guide Lines",
    scenarioId: "features_guide_lines",
  },
  {
    id: "features_guide_spans",
    label: "Guide Spans",
    scenarioId: "features_guide_spans",
  },
  {
    id: "features_bars_bands",
    label: "Bars And Bands",
    scenarioId: "features_bars_bands",
  },
  {
    id: "features_controller_fly",
    label: "Fly Controller",
    scenarioId: "features_controller_fly",
  },
  {
    id: "features_controller_turntable",
    label: "Turntable Controller",
    scenarioId: "features_controller_turntable",
  },
  {
    id: "features_bezier_curve_path",
    label: "Bezier Curve Path",
    scenarioId: "features_bezier_curve_path",
  },
  {
    id: "features_path_join",
    label: "Path Join",
    scenarioId: "features_path_join",
  },
  {
    id: "features_coordinate_system",
    label: "Coordinate System",
    scenarioId: "features_coordinate_system",
  },
  {
    id: "features_visual_transform",
    label: "Visual Transform",
    scenarioId: "features_visual_transform",
  },
  {
    id: "features_panel_view2d",
    label: "Panel View 2D",
    scenarioId: "features_panel_view2d",
    animate: true,
  },
  {
    id: "features_camera_manual",
    label: "Manual Camera",
    scenarioId: "features_camera_manual",
  },
  {
    id: "features_controller_arcball",
    label: "Arcball Controller",
    scenarioId: "features_controller_arcball",
  },
  {
    id: "features_mesh_texture",
    label: "Textured Mesh",
    scenarioId: "features_mesh_texture",
  },
  {
    id: "features_reference_grid",
    label: "Reference Grid",
    scenarioId: "features_reference_grid",
  },
  {
    id: "features_bounds_overlay",
    label: "Bounds Overlay",
    scenarioId: "features_bounds_overlay",
  },
  {
    id: "visuals_point",
    label: "Point",
    scenarioId: "visuals_point",
  },
  {
    id: "visuals_pixel",
    label: "Pixel",
    scenarioId: "visuals_pixel",
  },
  {
    id: "visuals_marker",
    label: "Marker",
    scenarioId: "visuals_marker",
  },
  {
    id: "visuals_primitive",
    label: "Primitive",
    scenarioId: "visuals_primitive",
  },
  {
    id: "visuals_segment",
    label: "Segment",
    scenarioId: "visuals_segment",
  },
  {
    id: "visuals_path",
    label: "Path",
    scenarioId: "visuals_path",
  },
  {
    id: "visuals_image",
    label: "Image",
    scenarioId: "visuals_image",
  },
  {
    id: "visuals_image_rgba",
    label: "RGBA Image",
    scenarioId: "visuals_image_rgba",
  },
  {
    id: "visuals_mesh",
    label: "Mesh",
    scenarioId: "visuals_mesh",
  },
  {
    id: "visuals_sphere",
    label: "Sphere",
    scenarioId: "visuals_sphere",
    effectLimitations: [
      {
        effect: "msaa",
        status: "unavailable",
        warning: "The desktop example enables 8x MSAA with alpha-to-coverage; the WebGPU route is currently single-sampled.",
      },
    ],
  },
  {
    id: "visuals_text",
    label: "Text",
    scenarioId: "visuals_text",
  },
  {
    id: "visuals_glyph",
    label: "Glyph",
    scenarioId: "visuals_glyph",
  },
  {
    id: "visuals_labels",
    label: "Labels",
    scenarioId: "visuals_labels",
  },
];

export function liveExampleById(id) {
  return LIVE_EXAMPLES.find((example) => example.id === id) ?? null;
}
