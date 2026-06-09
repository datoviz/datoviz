export const LIVE_EXAMPLES = [
  {
    id: "feature_basic_scene",
    label: "Basic Scene",
    scenarioId: "feature_basic_scene",
  },
  {
    id: "feature_timer_animation",
    label: "Timer Animation",
    scenarioId: "feature_timer_animation",
    animate: true,
  },
  {
    id: "feature_triangulation_polygon",
    label: "Polygon Triangulation",
    scenarioId: "feature_triangulation_polygon",
  },
  {
    id: "feature_builtin_shapes_2d",
    label: "Builtin Shapes 2D",
    scenarioId: "feature_builtin_shapes_2d",
  },
  {
    id: "feature_builtin_shapes_3d",
    label: "Builtin Shapes 3D",
    scenarioId: "feature_builtin_shapes_3d",
  },
  {
    id: "feature_isolines",
    label: "Isolines",
    scenarioId: "feature_isolines",
  },
  {
    id: "feature_animation_tracks",
    label: "Animation Tracks",
    scenarioId: "feature_animation_tracks",
    animate: true,
  },
  {
    id: "feature_obj_loading",
    label: "OBJ Loading",
    scenarioId: "feature_obj_loading",
  },
  {
    id: "feature_picking",
    label: "Picking",
    scenarioId: "feature_picking",
  },
  {
    id: "image_probe",
    label: "Image Probe",
    scenarioId: "feature_image_probe",
  },
];

export function liveExampleById(id) {
  return LIVE_EXAMPLES.find((example) => example.id === id) ?? null;
}
