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
