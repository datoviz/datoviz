# Scene Scale Bar 3D Reference Follow-Up

> **Execution Status**
> - **Status:** `LATER FOLLOW-UP`
> - **Updated on:** `2026-05-25`
> - **Purpose:** track explicit-reference 3D scale-bar semantics after the 2D slice.


## Summary

The retained 2D scale-bar slice is implemented. A 3D scale bar still needs an explicit reference
policy because perspective projection has no single global physical scale across the panel.


## Required Semantics

A 3D scale bar must choose scale from one of these explicit references:

1. a world-space or visual-space reference point;
2. a camera/view-space depth;
3. an attached visual or bounds reference.

The implementation should project or unproject around the chosen reference using the same panel MVP,
viewport, and DPI-aware screen-scale path used during frame emission. It must not infer physical
scale from the whole perspective panel.


## Follow-Up Work

1. Extend `DvzScaleBarDesc` with an explicit 3D reference mode and reference payload.
2. Add validation for invalid or missing references on perspective panels.
3. Lower the 3D result through the existing segment/text derived visuals.
4. Update `examples/c/annotations/scalebar_2d_3d.c` so the right panel includes the real 3D scale bar.
5. Add offscreen pixel/readback validation when Vulkan-backed scene tests are available.
