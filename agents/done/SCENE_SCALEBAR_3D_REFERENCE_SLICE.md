# Scene Scale Bar 3D Reference Slice

> **Execution Status**
> - **Status:** `DONE`
> - **Updated on:** `2026-05-25`
> - **Purpose:** record the explicit-reference 3D retained scale-bar slice.


## Summary

The retained scale-bar annotation now supports a 3D world-reference mode. The default mode remains
the existing 2D panel-domain behavior. In 3D mode, the scale is resolved by projecting a unit world
direction at an explicit reference point through the active panel MVP, then feeding the resulting
local units-per-pixel value into the same nice-length and SI-formatting path used by 2D scale bars.


## Implemented Behavior

1. `DvzScaleBarDesc.reference_mode` selects panel-domain or world-point scale resolution.
2. `reference_position[3]` defines the world point where the perspective scale is measured.
3. `reference_direction[3]` defines the local world direction used for the scale sample.
4. If the direction is zero in world-point mode, `dimension` supplies the fallback X/Y/Z axis.
5. The rendered bar remains a fixed screen-space overlay and updates as camera or arcball transforms
   change.
6. `examples/c/annotations/scalebar_2d_3d.c` now shows a 2D scale bar on the left panel and a
   world-referenced 3D scale bar on the right panel.


## Validation

Focused validation added for retained 3D scale-bar realization, including a camera-distance change
that verifies the label recomputes from the world reference.
