> **Execution Status**
> - **Status:** `SCENE SPEC PROPOSAL`
> - **Updated on:** `2026-05-22`
> - **Purpose:** define a panel-level content-padding model so data visuals and plot-aligned
>   adornments, especially colorbars, share the same inset.

# Panel Content Padding

This proposal records the layout direction exposed by the colorbar example hardening: users often
want visible breathing room around the plotted data, while still expecting a panel-attached colorbar
to align with the visible data extent.

The important distinction is that margin around data should not be encoded by manually shrinking
visual vertices or by using colorbar-specific `edge_offset_px`. It should be a panel layout concept
that both data visuals and plot-aligned adornments can consume.


## Problem

The current active layout model already separates:

1. panel reserve: fixed edge space consumed by axes, colorbars, legends, or other adornments;
2. plot rect: the data viewport after all resolved reserves;
3. adornment bands: the reserved regions outside the plot rect.

This is enough to keep colorbars from overlapping the plot area, but it does not express an inner
margin inside the plot rect. Examples can fake that margin by shrinking visual coordinates from
`[-1, +1]` to something like `[-0.9, +0.9]`, but then colorbars, axes, probes, and future
annotations have no shared way to know the actual visible data extent.

Colorbar-local `edge_offset_px` is not the right abstraction either. It trims only the colorbar ramp
and does not tell the image, axes, probes, or other plot-aligned objects to use the same inset.


## Recommendation

Add a panel-owned plot-content padding in fixed logical pixels:

```c
bool dvz_panel_set_plot_padding(
    DvzPanel* panel, float left_px, float right_px, float bottom_px, float top_px);

bool dvz_panel_get_plot_padding(const DvzPanel* panel, DvzPanelReserve* out);
bool dvz_panel_content_rect_px(const DvzPanel* panel, DvzRect* out);
```

The resolved rectangle model becomes:

```text
panel rect
  reserve bands
    plot rect
      plot/content padding
        content rect
```

The existing `dvz_panel_plot_rect_px()` should continue to report the rect after reserves. A new
content-rect query should report the rect after reserves and content padding.


## Semantics

Plot-content padding is not reserve. Reserve moves adornments away from data and changes the plot
rect. Content padding insets the data presentation inside the plot rect while keeping the surrounding
adornment bands and panel ownership unchanged.

Default content padding should be zero on every side. That preserves current edge-to-edge behavior
unless a caller explicitly opts into an inner margin.

For 2D image-like examples, a typical margin would be:

```c
dvz_panel_set_plot_padding(panel, 32.0f, 32.0f, 32.0f, 32.0f);
```

With this configuration:

1. an image that fills its available panel data area should fill the content rect;
2. a right or left vertical colorbar should align its ramp top and bottom to the content rect;
3. a top or bottom horizontal colorbar should align its ramp left and right to the content rect;
4. probes and annotations that target the visible data extent should have a clear content-rect query;
5. colorbar-local `edge_offset_px` remains a colorbar-only trim, not the primary plot-margin API.


## Colorbar Interaction

Panel-attached colorbars should use the content rect for their long-axis alignment and the plot rect
for their cross-axis gap:

1. right vertical colorbar:
   `x0 = plot_rect.right + plot_gap_px`,
   `y0 = content_rect.top`,
   `y1 = content_rect.bottom`;
2. left vertical colorbar:
   `x1 = plot_rect.left - plot_gap_px`,
   `y0 = content_rect.top`,
   `y1 = content_rect.bottom`;
3. bottom horizontal colorbar:
   `y0 = plot_rect.bottom + plot_gap_px`,
   `x0 = content_rect.left`,
   `x1 = content_rect.right`;
4. top horizontal colorbar:
   `y1 = plot_rect.top - plot_gap_px`,
   `x0 = content_rect.left`,
   `x1 = content_rect.right`.

This keeps the colorbar adjacent to the plot area while aligning its ramp to the visible data extent.


## Implementation Notes

The first implementation can store padding directly on `DvzPanel`, alongside resolved reserves.
Validation should reject negative, non-finite, or overlarge padding that collapses the content rect.

Scene helpers should expose both plot and content rectangles internally:

```c
_scene_panel_plot_pixel_rect(...)
_scene_panel_content_pixel_rect(...)
_scene_panel_content_visual_rect(...)
```

Data visuals that are meant to fill the visible plotting area should use content-rect normalization
when panel content padding is nonzero. Existing visuals with explicit authored positions should keep
their current coordinates; padding should affect panel-level "fill" behavior and generated examples,
not reinterpret arbitrary user geometry silently.


## Relation To Existing Specs

This proposal refines the panel box model in
[`../../core/PANEL_LAYOUT.md`](../../core/PANEL_LAYOUT.md). Once implemented and tested, the
normative content should move there, and this proposal can be retired.

It also refines the attached colorbar alignment rules described in
[`../../semantics/LEGENDS_AND_COLORBARS.md`](../../semantics/LEGENDS_AND_COLORBARS.md) and
[`COLORBAR_COLORMAP_DESIGN.md`](COLORBAR_COLORMAP_DESIGN.md).
