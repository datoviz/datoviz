> **Execution Status**
> - **Status:** `SCENE SPEC PROPOSAL`
> - **Updated on:** `2026-05-22`
> - **Purpose:** define a panel-level padding model so plot content and reserved adornment bands,
>   especially colorbars, share the same outer inset.

# Panel Padding

This proposal records the layout direction exposed by the colorbar example hardening: users often
want visible breathing room around an entire panel, while still expecting the plot area and
panel-attached adornments such as colorbars to share natural alignment.

The important distinction is that panel breathing room should not be encoded by manually shrinking
visual vertices or by using colorbar-specific `edge_offset_px`. It should be a panel layout concept
that applies before reserves are resolved, so both the plot and reserved adornment bands consume the
same padded panel interior.


## Problem

The current active layout model already separates:

1. panel reserve: fixed edge space consumed by axes, colorbars, legends, or other adornments;
2. plot rect: the data viewport after all resolved reserves;
3. adornment bands: the reserved regions outside the plot rect.

This is enough to keep colorbars from overlapping the plot area, but it does not express an inner
margin inside the panel. Examples can fake that margin by shrinking visual coordinates from
`[-1, +1]` to something like `[-0.9, +0.9]`, but then colorbars, axes, probes, and future
annotations have no shared way to know the intended padded layout extent.

Colorbar-local `edge_offset_px` is not the right abstraction either. It trims only the colorbar ramp
and does not tell the image, axes, probes, or other panel-aligned objects to use the same inset.


## Recommendation

Add a panel-owned padding in fixed logical pixels:

```c
bool dvz_panel_set_padding(
    DvzPanel* panel, float left_px, float right_px, float bottom_px, float top_px);

bool dvz_panel_get_padding(const DvzPanel* panel, DvzPanelReserve* out);
bool dvz_panel_inner_rect_px(const DvzPanel* panel, DvzRect* out);
```

The resolved rectangle model becomes:

```text
panel rect
  panel padding
    inner panel rect
      reserve bands
      plot rect
```

The existing `dvz_panel_plot_rect_px()` should continue to report the rect after panel padding and
reserves. A new inner-rect query should report the panel rect after padding but before reserves.


## Semantics

Panel padding is not reserve. Padding first insets the whole panel layout area. Reserve then divides
that padded inner panel rect into the plot rect and adornment bands. This keeps the plot, axes,
legends, colorbars, and future panel-local annotations aligned to one shared inner panel boundary.

Default panel padding should be zero on every side. That preserves current edge-to-edge behavior
unless a caller explicitly opts into an inner margin.

For 2D image-like examples, a typical margin would be:

```c
dvz_panel_set_padding(panel, 32.0f, 32.0f, 32.0f, 32.0f);
```

With this configuration:

1. the panel's layout content starts inside the padded inner panel rect;
2. resolved reserve bands are carved from that inner panel rect, not from the outer panel rect;
3. an image that fills its available panel data area should fill the resolved plot rect;
4. attached colorbar ramps should align with the resolved plot rect on their long axis;
5. probes and annotations that target the panel layout extent should have a clear inner-rect query;
6. colorbar-local `edge_offset_px` remains a colorbar-only trim, not the primary panel-margin API.

This is the primary model. A separate future plot-only or data-only padding can still exist if a
visual family needs inner breathing room between data marks and the plot border, but that should be
named separately and should not drive colorbar or reserve alignment.


## Colorbar Interaction

Panel-attached colorbars should use the same resolved plot rect as data visuals for their long-axis
alignment, because that plot rect has already accounted for panel padding and reserve:

1. right vertical colorbar:
   `x0 = plot_rect.right + plot_gap_px`,
   `y0 = plot_rect.top`,
   `y1 = plot_rect.bottom`;
2. left vertical colorbar:
   `x1 = plot_rect.left - plot_gap_px`,
   `y0 = plot_rect.top`,
   `y1 = plot_rect.bottom`;
3. bottom horizontal colorbar:
   `y0 = plot_rect.bottom + plot_gap_px`,
   `x0 = plot_rect.left`,
   `x1 = plot_rect.right`;
4. top horizontal colorbar:
   `y1 = plot_rect.top - plot_gap_px`,
   `x0 = plot_rect.left`,
   `x1 = plot_rect.right`.

This keeps the colorbar adjacent to the plot area while preserving the same padded panel boundary for
the plot and reserved adornment bands.


## Implementation Notes

The first implementation can store padding directly on `DvzPanel`, alongside resolved reserves.
Validation should reject negative, non-finite, or overlarge padding that collapses the inner panel
rect or the resolved plot rect after reserve.

Scene helpers should expose both inner-panel and plot rectangles internally:

```c
_scene_panel_inner_pixel_rect(...)
_scene_panel_plot_pixel_rect(...)
```

Data visuals that are meant to fill the visible plotting area should continue to use plot-rect
normalization; the plot rect itself is now derived from the padded inner panel rect. Existing visuals
with explicit authored positions should keep their current coordinates; padding should affect
panel-level layout and generated examples, not reinterpret arbitrary user geometry silently.


## Relation To Existing Specs

This proposal refines the panel box model in
[`../../core/PANEL_LAYOUT.md`](../../core/PANEL_LAYOUT.md). Once implemented and tested, the
normative content should move there, and this proposal can be retired.

It also refines the attached colorbar alignment rules described in
[`../../semantics/LEGENDS_AND_COLORBARS.md`](../../semantics/LEGENDS_AND_COLORBARS.md) and
[`COLORBAR_COLORMAP_DESIGN.md`](COLORBAR_COLORMAP_DESIGN.md).
