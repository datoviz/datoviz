# Scene Scale Bar Rendering Slice

> **Execution Status**
> - **Status:** `DONE`
> - **Updated on:** `2026-05-25`
> - **Purpose:** record the completed first 2D retained scale-bar annotation slice.


## Summary

The first scale-bar implementation slice is landed as a retained scene annotation. It adds a typed
`dvz_annotation_scalebar()` public constructor, `DvzScaleBarDesc`, retained semantic state, shared
nice-length and ASCII SI unit formatting helpers, and lowering to ordinary scene visuals: one fixed
screen-space segment visual plus one text/glyph label visual.

The implementation uses the shared scene DPI screen-scale resolver for screen-space line widths and
text sizes. It does not introduce a separate logical/physical pixel model.


## Implemented Behavior

1. Horizontal 2D scale bars attached to a panel.
2. Bottom/top/left/right/center panel anchors through the scene anchor enum.
3. Label placement above or below the line.
4. `1 / 2 / 5 * 10^n` nice-length selection with min/target/max pixel constraints.
5. ASCII SI labels including `n`, `u`, `m`, `c`, no prefix, `k`, and `M`.
6. Visible-domain updates through `dvz_panel_visible_domain()`.
7. Derived segment and text visuals hidden on invalid input or annotation destroy.
8. DPI-scaled segment line width through the normal scene visual upload path.


## Example

Added `examples/c/annotations/scalebar_2d_3d.c`, a two-panel smoke example:

1. left panel: physical-domain 2D scatter with retained 2D scale bar and panzoom;
2. right panel: 3D sphere cloud with arcball, ready for the explicit-reference 3D follow-up.

The right panel intentionally does not invent a perspective scale policy. The 3D scale-bar work is
tracked separately in [../later/SCENE_SCALEBAR_3D_REFERENCE_FOLLOWUP.md](../later/SCENE_SCALEBAR_3D_REFERENCE_FOLLOWUP.md).


## Validation

Recorded validation for this slice:

1. `git diff --check`
2. `just build`
3. `just test test_scene_scalebar_formatting`
4. `just test test_scene_scalebar_2d_realization`
5. `just test scene`

The scene suite passed with `322/431` selected tests passing, `0` failures, and `109` Vulkan-backed
tests skipped because Vulkan instance creation was unavailable in this environment.
