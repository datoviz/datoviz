# C Feature Examples

Scene and app capabilities belong here: panels, axes, colorbars, scale bars, annotations, overlays,
controllers, sampled fields, picking, probing, selection, and retained updates.

- `axes_2d.c`: retained numeric axes and tick labels.
- `axis_labels.c`: retained axis titles and tick-label placement with plot margins.
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
- `panel_multi.c`: multiple independent panels with panel-local panzoom controllers.
- `panel_linked.c`: two panels with one-way linked X panzoom state.
