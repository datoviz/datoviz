# C Feature Examples

Scene capabilities belong here: panels, axes, colorbars, scale bars, annotations, overlays,
controllers, sampled fields, picking, probing, selection, retained updates, and rendering
techniques. App lifecycle, capture, recording, replay, and media export examples live in
`../runtime/`.

- `axes_2d.c`: retained numeric axes and tick labels.
- `coordinate_system.c`: interactive 3D RGB-axis proof with an orbit camera and reference grid.
- `axis_labels.c`: retained axis titles and tick-label placement with plot margins.
- `sampled_field_2d.c`: scene-owned 2D sampled field bound to an image visual.
- `sampled_field_3d.c`: scene-owned 3D sampled field bound to a volume visual.
- `text_block.c`: multiline retained semantic text with explicit line spacing.
- `overlay_card.c`: screen-space overlay card attached to a panel.
- `controller_arcball.c`: arcball controller attached to a small 3D mesh.
- `controller_fly.c`: fly controller attached to a sparse 3D scene.
- `controller_turntable.c`: constrained turntable controller attached to a small 3D mesh.
- `mesh_texture.c`: minimal UV textured mesh with a procedural RGBA8 texture.
- `material_mesh.c`: neutral mesh material parameters with readable normals and shading.
- `lighting.c`: simple 3D lighting direction and intensity proof.
- `gui_controls.c`: native Datoviz GUI controls mutating retained visual state.
- `gui_viewport.c`: dockable ImGui window containing a Datoviz-rendered offscreen viewport.
- `gui_cimgui.c`: raw `datoviz/imgui.h` access for cimgui tables and tabs.
- `animation_tracks.c`: retained track-backed visual transform animation.
- `compute_buffer_animation.c`: experimental scene compute pass writing a render-consumed buffer.
- `edl.c`: focused Eye-Dome Lighting panel technique proof.
- `ssao.c`: focused screen-space ambient occlusion technique proof.
- `msaa.c`: panel multisample antialiasing comparison.
- `depth_cue.c`: retained visual depth-cue parameter proof.
- `transparency_order.c`: alpha blending, WBOIT, and depth-peeling ordering comparison.
- `volume_occlusion.c`: volume-backed scene occlusion for embedded visuals.
- `guide_lines.c`: retained horizontal and vertical guide lines with updates.
- `guide_spans.c`: retained interval guide spans with fill and outline styling.
- `bars_bands.c`: retained bars and uncertainty band plot helpers.
- `input_events.c`: native input event injection independent of picking/controllers.
- `visual_transform.c`: retained visual-local transform set/get/clear proof.
- `panel_view2d.c`: explicit panel 2D view framing and reserved plot layout.
- `json_export.c`: experimental scene JSON serialization diagnostic.
- `bezier_curve_path.c`: CPU-tessellated Bezier curves rendered through retained paths.
- `isolines.c`: CPU contour extraction rendered as retained segment overlays.
- `builtin_shapes_2d.c`: builtin 2D geometry builders rendered as retained meshes.
- `builtin_shapes_3d.c`: builtin 3D geometry builders rendered as retained meshes.
- `obj_loading.c`: Wavefront OBJ mesh loading through the public geometry loader.
- `datetime_axis.c`: compact data coordinates with UTC datetime tick labels.
- `scalebar.c`: retained metric scale bar using builtin length units.
- `scalebar_units.c`: retained duration scale bar using builtin duration units.
- `colorbar.c`: retained continuous colorbar attached to one scalar image.
- `colormap_scale.c`: scalar float color values mapped through a retained color scale.
- `update_partial.c`: retained point visual with one item-range data update.
- `depth_test.c`: side-by-side visual depth-test toggle.
- `alpha_blending.c`: source-over alpha blending with translucent primitives.
- `panel_background.c`: panel background styling behind a simple foreground visual.
- `panzoom.c`: one panzoom controller attached to a 2D panel.
- `picking.c`: marker item picking with hover scaling and persistent selection tinting.
- `probe_labels.c`: label-id probe/readout on a categorical label field.
- `basic_scene.c`: smallest runner-backed retained scene with one point visual.
- `panel_single.c`: one explicit panel rectangle with one visual and panel chrome.
- `panel_grid.c`: four grid-owned panels with clipped panel-local content.
- `panel_multi.c`: multiple independent panels with panel-local panzoom controllers.
- `panel_linked.c`: two panels linked on X panzoom extent with independent Y panzoom state.
- `timer_animation.c`: runner-backed frame animation updating retained point data.
- `marker_symbols.c`: marker symbol sets with built-in, bitmap, SDF, MSDF, and SVG-path sources.
- `update_visual_data.c`: retained point visual with full data replacement.
- `visibility.c`: retained visual visibility toggled before rendering.
- `legend_categorical.c`: tentative retained categorical legend attached to a panel.

Tentative: keep `legend_categorical.c` experimental until categorical legends are explicitly
promoted into the public v0.4 surface. Keep runtime video capture experimental in
`../runtime/video_export.c` until video export is explicitly promoted.
