# Gallery Reference Review, 2026-11-16

Status: all 69 screenshots outside the reference tolerance have an explicit disposition. Of these, 64 are consistent with intentional source changes, two polygon cases require a focused semantic correction, and three small raster residuals retain a comparison/disposition gate. This is a review record, not approval to replace canonical media.

## Evidence And Limits

The reviewed development captures were generated at runtime source `4689dc47f`; documentation-only commit `4f39d031e` preserves that runtime. The protected `data` tip is `6a9091c10e08e477af5b07496cac602aefc7c999`. Two independent runs produced 107 byte-identical PNGs and full-pixel/nonblank checks pass. The prior Linux reference provenance records source `c5fcaeb3a` on an NVIDIA RTX 5090 with driver 595.84; current checks use the RTX 5090 with driver 595.99.02. Individual canonical files also retain their later source-specific promotions, including AO, GUI widgets and histogram. The driver difference prevents treating this review as a matched-driver regression campaign.

All 69 reference/current pairs and enhanced differences were inspected in comparison sheets. The 2026-11-16 focused run passes 45/45 tests with zero failures/skips: 15 GUI tests, 11 text-atlas tests and 19 scene offscreen tests covering relevant lighting, AO, peeling, text, colormap and color-role behavior. All 16 course steps also pass reproducible in-tree rendering. Neither passing tests nor visual inspection proves every interaction or backend; exact-candidate, hosted and physical-platform gates remain separate.

Build-local evidence is under `build/gateway-docs/reference-review/`: comparison sheets, `classification.json`, `focused-tests.json`, `focused-tests.log`, diagnostic C sources/binaries and PNGs, and the course checker/smoke logs. These generated artifacts are excluded from Git. No reference image, `data` commit or parent gitlink was changed.

## Findings Requiring Action

1. **Polygon data colors inherit lighting.** `src/scene/domain/polygon_composite.c` creates its fill with `dvz_mesh(scene, 0)` and does not choose an unlit material. Mesh defaults are Phong, so `composites_polygon` and `showcases_choropleth` change data colors with scene lighting. For example, the polygon-set green authored as RGB `(92, 189, 132)` is shaded rather than reproduced. An isolated diagnostic copy of each canonical C example changes only the fill role to an unlit material and restores the authored RGB values. This is a concrete semantic concern, not evidence that the scene-owned lighting implementation itself regressed.
2. **The polygon transparency demonstration needs an explicit alpha policy.** The polygon example describes transparent fills and supplies alpha 210/220, but its fill mesh inherits opaque alpha mode. In diagnostic captures, explicit unlit/opaque produces the raw source RGB, while explicit unlit/blended gives RGB `(86, 177, 124)` for the green region and `(34, 139, 164)` for the holed polygon over background `(22, 27, 34)`, matching linear-light source-over composition. Do not approve a reference merely because the new shading resembles an older screenshot.
3. **Three small residuals need disposition, not speculative runtime changes.** Point Cloud has maximum delta 7, with 95% of changed components at delta 3 or less, but changes 30.6494% of components. The grid has maximum delta 19 with only nine components above delta 8; the volume has maximum delta 15 with 145 components above delta 8. No obvious lost geometry or new blank output was observed. Driver/rasterization is a hypothesis rather than an established cause.

Recommended fix: define unlit data-color defaults for polygon-composite fills and a deliberate alpha policy, with explicit lighting opt-in if needed; avoid changing global mesh defaults or restoring material-owned lights. Add focused pixel regressions for opaque polygon palette fidelity under changing panel lights and for fractional-alpha source-over, then verify the C/Python/WebGPU polygon and choropleth routes and regenerate their local media. Keep canonical promotion separate.

For the three residuals, compare the same source on the reference provider/driver when practical, or prepare a new provenance-bearing two-run candidate set and obtain explicit maintainer disposition. Do not loosen the tolerance globally or label an unavailable driver comparison a pass.

## Disposition Of All 69 Images

“Expected source change” means the visual difference is consistent with identifiable implementation history and the focused checks; it is not maintainer approval of the replacement image. Font/GUI changes retain readable captions in the inspected captures; the old grid-only axis example is intentionally sparse, rather than a missing data plot.

| Example | Disposition | Group |
| --- | --- | --- |
| `composites_polygon` | Hold: semantic correction | polygon-color |
| `features_alpha_blending` | Expected source change | lighting |
| `features_animation_tracks` | Expected source change | lighting |
| `features_annotation_readout` | Expected source change | font |
| `features_axes_2d` | Expected source change | font |
| `features_axis_labels` | Expected source change | font |
| `features_bars_bands` | Expected source change | font |
| `features_bounds_overlay` | Expected source change | lighting |
| `features_builtin_shapes_2d` | Expected source change | lighting |
| `features_builtin_shapes_3d` | Expected source change | lighting |
| `features_camera_manual` | Expected source change | lighting |
| `features_colorbar` | Expected source change | font |
| `features_controller_arcball` | Expected source change | lighting |
| `features_controller_fly` | Expected source change | lighting |
| `features_controller_turntable` | Expected source change | lighting |
| `features_coordinate_system` | Expected source change | lighting |
| `features_datetime_axis` | Expected source change | font |
| `features_gui_cimgui` | Expected source change | gui |
| `features_gui_controls` | Expected source change | gui |
| `features_gui_data_widgets` | Expected source change | gui |
| `features_gui_viewport` | Expected source change | gui |
| `features_guide_lines` | Expected source change | font |
| `features_guide_spans` | Expected source change | font |
| `features_image_probe` | Expected source change | font |
| `features_isolines` | Expected source change | lighting |
| `features_legend_categorical` | Expected source change | font |
| `features_lighting` | Expected source change | lighting |
| `features_marker_symbols` | Expected source change | font |
| `features_material_mesh` | Expected source change | lighting |
| `features_mesh_texture` | Expected source change | lighting |
| `features_obj_loading` | Expected source change | lighting |
| `features_orientation_gizmo` | Expected source change | lighting |
| `features_overlay_card` | Expected source change | font |
| `features_panel_background` | Expected source change | lighting |
| `features_panel_view2d` | Expected source change | font |
| `features_probe_labels` | Expected source change | font |
| `features_reference_grid` | Hold: residual comparison | raster-residual |
| `features_scalebar` | Expected source change | font |
| `features_scalebar_units` | Expected source change | font |
| `features_selection_mesh_instances` | Expected source change | lighting |
| `features_selection_sphere` | Expected source change | lighting |
| `features_technique_ao` | Expected source change | effects |
| `features_technique_depth_cue` | Expected source change | effects |
| `features_technique_depth_test` | Expected source change | effects |
| `features_technique_edl` | Expected source change | effects |
| `features_technique_msaa` | Expected source change | effects |
| `features_technique_transparency` | Expected source change | effects |
| `features_text_block` | Expected source change | font |
| `features_user_scale` | Expected source change | font |
| `showcases_choropleth` | Hold: semantic correction | polygon-color |
| `showcases_cortical_activity` | Expected source change | lighting |
| `showcases_linked_probe_colorbar` | Expected source change | font |
| `showcases_panel_linked_axes` | Expected source change | font |
| `showcases_point_cloud` | Hold: residual comparison | raster-residual |
| `showcases_protein` | Expected source change | lighting |
| `showcases_scalebar_measurement` | Expected source change | font |
| `showcases_scientific_plotting` | Expected source change | font |
| `showcases_spherical_harmonics` | Expected source change | lighting |
| `showcases_streaming_daq` | Expected source change | showcase-defaults |
| `showcases_surface_grid` | Expected source change | lighting |
| `showcases_terrain_relief` | Expected source change | showcase-defaults |
| `showcases_textured_planet` | Expected source change | lighting |
| `showcases_wind_field` | Expected source change | font |
| `visuals_glyph` | Expected source change | font |
| `visuals_mesh` | Expected source change | lighting |
| `visuals_primitive` | Expected source change | lighting |
| `visuals_sphere` | Expected source change | lighting |
| `visuals_text` | Expected source change | font |
| `visuals_volume` | Hold: residual comparison | raster-residual |

## Group Evidence

| Group | Images | Evidence / next action |
| --- | --- | --- |
| font | 25 | Source Sans 3 / Source Code Pro and deterministic MSDF atlas migration; `808f7ecfa`, `f1fe7f74c`, `3ce8118dc`. |
| gui | 4 | Source fonts, GUI scale/style, docking and transparent host panels; `808f7ecfa`, `d9064facf`, `7020bdc2e`, `acd791cf9`. |
| lighting | 27 | Scene/panel-owned lights and shared material evaluation replace older defaults; `1506c96fb`. Review appearance without restoring old light ownership. |
| effects | 6 | Render-product/GTAO/MSAA/EDL/transparency changes, shared panel lights and effective-alpha corrections; `545c99379`, `1506c96fb`, `d0e904705`. Focused GPU tests pass. |
| showcase-defaults | 2 | Explicit terrain material/default tuning and deterministic showcase animation changes; `26e78f8e6`, `d89db0ea7`, `2bd302627`. |
| polygon-color | 2 | Polygon fills inherit lit mesh defaults. Set explicit unlit data-color behavior; preserve intentional alpha composition. Diagnostic builds isolate the material effect. |
| raster-residual | 3 | Small-amplitude raster differences; reference driver 595.84 versus current 595.99.02. Driver causation is unproven; use a matched-source/provider comparison or explicit disposition. |
