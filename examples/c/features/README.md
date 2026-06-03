# C Feature Examples

Scene and app capabilities belong here: panels, axes, colorbars, scale bars, annotations, overlays,
controllers, sampled fields, picking, probing, selection, and retained updates.

- `axes_2d.c`: retained numeric axes and tick labels.
- `axis_labels.c`: retained axis titles and tick-label placement with plot margins.
- `sampled_field_2d.c`: scene-owned 2D sampled field bound to an image visual.
- `sampled_field_3d.c`: scene-owned 3D sampled field bound to a volume visual.
- `text_block.c`: compact retained text block with stable screen placement.
- `overlay_card.c`: screen-space overlay card attached to a panel.
- `controller_arcball.c`: arcball controller attached to a small 3D mesh.
- `controller_fly.c`: fly controller attached to a sparse 3D scene.
- `controller_turntable.c`: constrained turntable controller attached to a small 3D mesh.
- `mesh_texture.c`: minimal UV textured mesh with a procedural RGBA8 texture.
- `material_mesh.c`: neutral mesh material parameters with readable normals and shading.
- `lighting.c`: simple 3D lighting direction and intensity proof.
- `datetime_axis.c`: compact data coordinates with UTC datetime tick labels.
- `scalebar.c`: retained metric scale bar using builtin length units.
- `scalebar_units.c`: retained duration scale bar using builtin duration units.
- `colorbar.c`: retained continuous colorbar attached to one scalar image.
- `colormap_scale.c`: scalar float color values mapped through a retained color scale.
- `update_partial.c`: retained point visual with one item-range data update.
- `depth_test.c`: side-by-side visual depth-test toggle.
- `alpha_blending.c`: source-over alpha blending with translucent primitives.
- `panel_background.c`: panel background styling behind a simple foreground visual.
- `panzoom_attachment.c`: one panzoom controller attached to a 2D panel.
- `pick_point.c`: point item picking with callback/readback and visible selection.
- `pick_hover.c`: latest-request-wins hover feedback with background-miss clearing.
- `probe_labels.c`: label-id probe/readout on a categorical label field.
- `selection.c`: persistent selected-item highlight on a small visual.
- `scene_basic.c`: smallest retained scene/app setup with one point visual.
- `panel_single.c`: one explicit panel rectangle with one visual and panel chrome.
- `panel_grid.c`: four grid-owned panels with clipped panel-local content.
- `panel_multi.c`: multiple independent panels with panel-local panzoom controllers.
- `panel_linked.c`: two panels with bidirectional linked X panzoom state.
- `timer_animation.c`: deterministic timer/frame-callback animation.
- `update_visual_data.c`: retained point visual with full data replacement.
- `visibility.c`: retained visual visibility toggled before rendering.

Conditional: add `legend_categorical.c` only if categorical legends stay public for v0.4, and add
`video_export.c` only if video export is included in the public v0.4 surface.
