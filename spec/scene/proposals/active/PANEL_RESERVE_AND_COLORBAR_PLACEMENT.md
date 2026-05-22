> **Execution Status**
> - **Status:** `SCENE SPEC PROPOSAL`
> - **Updated on:** `2026-05-22`
> - **Purpose:** define the near-term panel reserve and colorbar placement direction without
>   introducing a general layout solver.

# Panel Reserve And Colorbar Placement

This note records the next design direction after the first retained colorbar slice. The immediate
goal is deliberately small: reserve a fixed number of pixels on any side of a panel, place attached
colorbars in that reserved band, and support detached colorbars with explicit anchored pixel
placement.


## Goals

1. Let users reserve `X` logical pixels on any side of a panel.
2. Keep reserves stable across figure and window resize.
3. Keep colorbar geometry explicit: reservation, ramp thickness, offsets, and size should not be
   hidden behind ambiguous padding terminology.
4. Use split horizontal/vertical anchor vocabulary instead of introducing new combinatorial anchor
   enums such as `TOP_RIGHT`. The implementation audit should identify the existing names to reuse;
   if the scene public API only exposes `DvzSceneAnchor`, add a small split-anchor API rather than
   expanding the combinatorial enum further.
5. Avoid a general layout engine until concrete examples require one.


## Non-Goals

1. No stacking model for multiple adornments on the same edge in the first implementation.
2. No automatic collision avoidance.
3. No constraint solver or CSS-like layout pass.
4. No automatic tight-layout behavior.


## Core Model

A panel owns a full panel rectangle and a derived plot rectangle. The plot rectangle is the panel
rectangle minus pixel reserves:

```text
panel rect px - reserve px = plot rect px
```

Panel reserves should be expressed in logical pixels:

```c
typedef struct DvzPanelReserve
{
    float left_px;
    float right_px;
    float top_px;
    float bottom_px;
} DvzPanelReserve;
```

The public API should prefer pixel reserves:

```c
DVZ_EXPORT bool dvz_panel_set_reserve(DvzPanel* panel, const DvzPanelReserve* reserve);

DVZ_EXPORT bool dvz_panel_get_reserve(const DvzPanel* panel, DvzPanelReserve* out);

DVZ_EXPORT bool dvz_panel_plot_rect_px(const DvzPanel* panel, DvzRect* out);
```

The current normalized or panel-visual reserve remains an implementation detail or legacy bridge
during the transition. New adornment APIs should not expose normalized reserve units as their
primary sizing language.


## Attached Colorbars

Attached colorbars reserve plot space on one side of their panel. The colorbar remains
panel-attached, and its ramp, ticks, labels, and title render in panel-local pixel coordinates.

The descriptor should name geometry in terms that identify where the space is used:

```c
typedef enum DvzColorbarPlacementMode
{
    DVZ_COLORBAR_PLACEMENT_ATTACHED,
    DVZ_COLORBAR_PLACEMENT_DETACHED,
} DvzColorbarPlacementMode;

typedef struct DvzColorbarDesc
{
    DvzColorbarPlacementMode placement_mode;
    DvzColorbarOrientation orientation;
    DvzSceneAnchor anchor;
    const char* title;

    float reserve_px;
    float ramp_width_px;
    float edge_offset_px;
    float plot_gap_px;
    float tick_length_px;
    float label_gap_px;
    DvzPlacement placement;

    uint32_t flags;
} DvzColorbarDesc;
```

For a right-side colorbar:

1. `reserve_px` is the amount removed from the plot rectangle on the right.
2. `ramp_width_px` is the color ramp thickness.
3. `edge_offset_px` is the distance from the outer panel edge to the colorbar content.
4. `plot_gap_px` is the distance from the plot rectangle edge to the colorbar content.
5. `label_gap_px` is the distance from ticks to labels.

Avoid using `padding` in this API. It is unclear whether padding means space between plot and
colorbar, colorbar and panel edge, or space inside the colorbar band.


## Detached Colorbars

Detached colorbars should not reserve plot space. They use explicit anchored placement in either
figure or panel pixel space.

Use split horizontal/vertical anchors to avoid combinatorial anchor enums:

```c
typedef enum DvzPlacementSpace
{
    DVZ_PLACEMENT_SPACE_PANEL,
    DVZ_PLACEMENT_SPACE_FIGURE,
} DvzPlacementSpace;

typedef struct DvzPlacement
{
    DvzPlacementSpace space;
    DvzHorizontalAnchor horizontal_anchor;
    DvzVerticalAnchor vertical_anchor;
    float offset_x_px;
    float offset_y_px;
    float width_px;
    float height_px;
} DvzPlacement;
```

Attached mode uses panel edge anchoring and reserves plot space. Detached mode uses `DvzPlacement`
and leaves the plot rectangle unchanged.


## Example Shape

Attached:

```c
DvzColorbar* colorbar = dvz_colorbar(
    panel, scale,
    &(DvzColorbarDesc){
        .placement_mode = DVZ_COLORBAR_PLACEMENT_ATTACHED,
        .orientation = DVZ_COLORBAR_ORIENTATION_VERTICAL,
        .anchor = DVZ_SCENE_ANCHOR_PANEL_RIGHT,
        .title = "Intensity",
        .reserve_px = 140.0f,
        .ramp_width_px = 36.0f,
        .edge_offset_px = 12.0f,
        .plot_gap_px = 12.0f,
        .label_gap_px = 6.0f,
    });
```

Detached:

```c
DvzColorbar* colorbar = dvz_colorbar(
    panel, scale,
    &(DvzColorbarDesc){
        .placement_mode = DVZ_COLORBAR_PLACEMENT_DETACHED,
        .orientation = DVZ_COLORBAR_ORIENTATION_VERTICAL,
        .title = "Intensity",
        .ramp_width_px = 36.0f,
        .placement = {
            .space = DVZ_PLACEMENT_SPACE_FIGURE,
            .horizontal_anchor = DVZ_HORIZONTAL_ANCHOR_RIGHT,
            .vertical_anchor = DVZ_VERTICAL_ANCHOR_TOP,
            .offset_x_px = -32.0f,
            .offset_y_px = +48.0f,
            .width_px = 64.0f,
            .height_px = 320.0f,
        },
    });
```

The exact anchor enum names should match the existing codebase anchors after the implementation
audit.


## Implementation Plan

1. Audit existing horizontal and vertical anchor enums and reuse them for placement.
2. Add panel pixel reserve storage and accessors.
3. Derive the current plot rectangle from panel rectangle and pixel reserve on every resize/frame
   preparation.
4. Keep the existing visual-unit reserve as a compatibility bridge until callers migrate.
5. Move attached colorbars to pixel reserve and explicit geometry fields.
6. Add detached colorbar placement using figure or panel pixel space.
7. Add focused tests for fixed-pixel reserve across resize, attached colorbar reserve behavior, and
   detached colorbar placement that does not change the plot rectangle.


## Open Questions

1. Should `reserve_px = 0` mean "use default automatic reserve" or "reserve nothing"?
2. Should attached colorbars reserve space automatically by default, or should explicit reserve be
   required once this API exists?
3. Should detached placement live directly on colorbars first, or should it become a shared helper
   for annotations, legends, and screen-space overlays at the same time?
