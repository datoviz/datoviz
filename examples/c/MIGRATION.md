# C Example Reset Migration

The v0.4 C example tree has been reset so public examples are separated from older smoke-test-style
source material.

## Public examples kept active

| Scenario ID | Source | Role |
| --- | --- | --- |
| `point_2d` | `visuals/point.c` | Minimal visual proof. |
| `visual_pixel` | `visuals/pixel.c` | Minimal visual proof. |
| `visual_marker` | `visuals/marker.c` | Minimal visual proof. |
| `visual_primitive` | `visuals/primitive.c` | Minimal visual proof. |
| `visual_segment` | `visuals/segment.c` | Minimal visual proof. |
| `visual_vector` | `visuals/vector.c` | Minimal visual proof. |
| `visual_path` | `visuals/path.c` | Minimal visual proof. |
| `visual_image` | `visuals/image.c` | Minimal visual proof. |
| `visual_mesh` | `visuals/mesh.c` | Minimal visual proof. |
| `sphere_impostor` | `visuals/sphere.c` | Minimal visual proof. |
| `visual_text` | `visuals/text.c` | Minimal visual proof. |
| `visual_labels` | `visuals/labels.c` | Minimal visual proof. |
| `volume` | `visuals/volume.c` | Minimal visual proof. |
| `visual_glyph` | `visuals/glyph.c` | Experimental visual proof. |
| `visual_splat` | `visuals/splat.c` | Experimental visual proof. |
| `feature_scene_basic` | `features/scene_basic.c` | Feature proof. |
| `feature_panel_single` | `features/panel_single.c` | Feature proof. |
| `feature_panel_grid` | `features/panel_grid.c` | Feature proof. |
| `feature_panel_multi` | `features/panel_multi.c` | Feature proof. |
| `feature_panel_linked` | `features/panel_linked.c` | Feature proof. |
| `path_axes_2d` | `features/axes_2d.c` | Feature proof. |
| `feature_axis_labels` | `features/axis_labels.c` | Feature proof. |
| `feature_sampled_field_2d` | `features/sampled_field_2d.c` | Feature proof. |
| `feature_sampled_field_3d` | `features/sampled_field_3d.c` | Feature proof. |
| `feature_text_block` | `features/text_block.c` | Feature proof. |
| `feature_overlay_card` | `features/overlay_card.c` | Feature proof. |
| `feature_controller_arcball` | `features/controller_arcball.c` | Feature proof. |
| `feature_controller_fly` | `features/controller_fly.c` | Feature proof. |
| `feature_controller_turntable` | `features/controller_turntable.c` | Feature proof. |
| `feature_mesh_texture` | `features/mesh_texture.c` | Feature proof. |
| `feature_material_mesh` | `features/material_mesh.c` | Feature proof. |
| `feature_lighting` | `features/lighting.c` | Feature proof. |
| `feature_datetime_axis` | `features/datetime_axis.c` | Experimental feature proof. |
| `linked_panels_axes_panzoom` | `workflows/panel_linked_axes.c` | Workflow proof. |
| `scale_bar` | `features/scalebar.c` | Minimal feature proof. |
| `scalebar_units` | `features/scalebar_units.c` | Unit-conversion feature proof. |
| `scalebar_measurement_workflow` | `workflows/scalebar_measurement.c` | Workflow proof. |
| `colorbar` | `features/colorbar.c` | Feature proof. |
| `colormap_scale` | `features/colormap_scale.c` | Feature proof. |
| `annotation_readout` | `features/annotation_readout.c` | Feature proof. |
| `image_probe` | `features/image_probe.c` | Feature proof. |
| `feature_update_visual_data` | `features/update_visual_data.c` | Feature proof. |
| `update_partial` | `features/update_partial.c` | Feature proof. |
| `feature_visibility` | `features/visibility.c` | Feature proof. |
| `depth_test` | `features/depth_test.c` | Feature proof. |
| `alpha_blending` | `features/alpha_blending.c` | Feature proof. |
| `panel_background` | `features/panel_background.c` | Feature proof. |
| `controller_panzoom` | `features/panzoom_attachment.c` | Feature proof. |
| `feature_pick_point` | `features/pick_point.c` | Feature proof. |
| `feature_pick_hover` | `features/pick_hover.c` | Feature proof. |
| `marker_picking` | `features/pick_marker.c` | Feature proof. |
| `feature_probe_labels` | `features/probe_labels.c` | Feature proof. |
| `feature_selection` | `features/selection.c` | Feature proof. |
| `feature_timer_animation` | `features/timer_animation.c` | Feature proof. |
| `feature_marker_symbols` | `features/marker_symbols.c` | Feature proof. |
| `feature_legend_categorical` | `features/legend_categorical.c` | Experimental feature proof. |
| `feature_video_export` | `features/video_export.c` | Experimental feature proof. |
| `linked_panels_probe_colorbar` | `workflows/linked_probe_colorbar.c` | Workflow proof. |
| `composite_polygon` | `composites/polygon.c` | Composite proof. |
| `composite_graph` | `composites/graph.c` | Composite proof. |
| `us_state_choropleth` | `scientific/choropleth.c` | Real Census data example with provenance. |
| `protein_arcball_viewer` | `scientific/protein.c` | Real RCSB PDB data example with provenance. |
| `showcase_gpu_particle_smoke` | `showcases/gpu_particle_smoke.c` | Experimental compute showcase. |
| `showcase_wind_field` | `showcases/wind_field.c` | Gallery/showcase seed. |
| `brain_volume_mesh` | `showcases/brain_volume_mesh.c` | Gallery/showcase seed. |
| `point_cloud` | `showcases/point_cloud.c` | Real RESEPI point-cloud showcase seed. |
| `textured_terrain_or_planet` | `showcases/textured_planet.c` | Gallery/showcase seed. |

## Lab examples kept buildable

| Source | Previous source |
| --- | --- |
| `lab/axis_lattice_smoke.c` | `tools/axis_lattice_smoke.c` |
| `lab/point_stress.c` | `stress/point_stress.c` |
| `lab/protein_viewer.c` | unchanged |
| `lab/record_dvzr.c` | `tools/record_dvzr.c` |
| `lab/replay_dvzr.c` | `tools/replay_dvzr.c` |
| `lab/scalebar_2d_3d.c` | unchanged |
| `lab/text_msdf_diagnostics.c` | `tools/text_msdf_diagnostics.c` |
| `lab/text_msdf_lab.c` | `tools/text_msdf_lab.c` |

## Legacy archive

| Legacy folder | Contents |
| --- | --- |
| `legacy/visuals/` | Former one-visual smoke examples. |
| `legacy/techniques/` | Former technique/prototype examples. |
| `legacy/showcase/` | Former singular showcase examples and prepare scripts, except promoted public examples. |
| `legacy/tools/` | Low-level/export/debug tool examples no longer in the active tree. |
| `legacy/regression/` | Former regression-style smoke example. |

Legacy examples are not built by default. Promote by copying or moving into `features/`, `visuals/`,
or `showcases/` only after polishing and manifest updates.
