# Clipping

This document defines how visuals are clipped within their panel.


## Default Clipping

By default every visual in a panel is clipped to the panel's **data area** — the inner
rectangle inside the axes margins.

This is the correct default for scientific visualization: data marks do not spill over
tick labels, axis titles, or colorbar slots.

The data area rectangle is derived from the panel layout after axes margins are applied.
It updates automatically when margins change (e.g., when axes are reconfigured or the
panel is resized).


## Clip Modes

```text
dvz_visual_set_clip(visual, mode)
```

| Mode | Description |
|---|---|
| `DVZ_CLIP_DATA_AREA` | clip to the inner data rectangle, inside axes margins (default) |
| `DVZ_CLIP_PANEL` | clip to the full panel rectangle, including margins |
| `DVZ_CLIP_NONE` | no clipping — visual may render outside the panel boundary |

`DVZ_CLIP_DATA_AREA` is the default for all visual families.

`DVZ_CLIP_PANEL` is useful for visuals that should fill the panel but not spill into
adjacent panels — for example a background color rect or a watermark.

`DVZ_CLIP_NONE` is the default for annotations (labels, guides, probes, callouts) which
are intentionally allowed to overflow into margins or across panel boundaries.
See `ANNOTATIONS.md`.


## How Clipping Is Applied

Clipping is implemented as a GPU scissor rectangle set on the render pass.
The scene maps each clip mode to the appropriate pixel rectangle at frame planning time.

Clip rectangles are in physical pixels (after `dpi_scale` is applied).
The scene computes them from the logical panel and data area rects.

When multiple visuals in the same panel have different clip modes, the scene groups
draw calls by clip mode to minimise scissor state changes in the render pass.


## Clipping And The Data Area

The data area is the panel rectangle minus the four axes margin widths:

```
data_area.x      = panel.x + margin_left
data_area.y      = panel.y + margin_top
data_area.width  = panel.width  - margin_left - margin_right
data_area.height = panel.height - margin_top  - margin_bottom
```

Margins are owned by the axes layer (see `AXES.md`).
If no axes are attached, margins are zero and `DVZ_CLIP_DATA_AREA` equals
`DVZ_CLIP_PANEL`.


## Interaction With Z-Layer And Transparency

Clipping is orthogonal to z-layer ordering and alpha mode.
A transparent visual with `DVZ_CLIP_NONE` still participates in the OIT accumulation
pass; its fragments outside the panel are simply not scissored.

The scene does not enforce any relationship between clip mode and z-layer.
The application is responsible for ensuring that `DVZ_CLIP_NONE` visuals at high
z-layers do not produce unexpected visual results by overlapping adjacent panels.


## Relationship To Other Documents

| Document | Relationship |
|---|---|
| `AXES.md` | axes margins define the data area boundary |
| `ANNOTATIONS.md` | annotations default to `DVZ_CLIP_NONE` |
| `PANEL_LAYOUT.md` | panel and data area rects derived from layout |
| `TRANSPARENCY.md` | clipping is orthogonal to alpha mode and OIT |
| `HIGH_DPI.md` | clip rects are computed in physical pixels after dpi_scale |
