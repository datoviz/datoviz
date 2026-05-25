# Scale Bar 2D And 3D Panels

> **Execution Status**
> - **Status:** `WORKED EXAMPLE PLAN`
> - **Updated on:** `2026-05-25`
> - **Purpose:** pressure-test adaptive physical-unit scale bars in both 2D panzoom and explicit
>   3D reference-scale panels.


## Summary

This example demonstrates retained scale-bar annotations in a two-panel figure. The left panel is a
2D scientific view with a panzoom controller and an adaptive physical scale bar. The right panel is
a 3D scene with an explicit-reference scale bar so the label remains honest under perspective.

The eventual C example target should be:

```text
examples/c/annotations/scalebar_2d_3d.c
```


## Feature Pressure

This example should exercise:

1. retained `DVZ_ANNOTATION_SCALEBAR` objects,
2. automatic `1 / 2 / 5 * 10^n` nice-length selection,
3. physical-unit labels such as `500 um`, `5 mm`, `2 cm`, and `20 km`,
4. label placement above and below a horizontal ticked line,
5. screen-space overlay placement and panel clipping,
6. panzoom and resize invalidation,
7. explicit-reference 3D scale semantics.


## Scene Shape

Use one figure with two side-by-side panels.

Left panel:

1. a 2D image, pixel field, or scatter cloud in a physical domain,
2. panzoom enabled,
3. `dvz_panel_set_domain()` configured so the visible domain is in physical data units,
4. scale bar anchored bottom-left with label above the line,
5. unit policy that adapts from micrometers through centimeters as the view changes.

Right panel:

1. a simple 3D mesh, sphere impostor scene, terrain patch, or point cloud,
2. arcball or turntable interaction,
3. scale bar anchored bottom-right with label below the line,
4. explicit reference point or depth configured near the object center,
5. unit policy that adapts from meters to kilometers.


## Sketch

The intended visual arrangement:

```text
+------------------------------+------------------------------+
| 2D physical image/scatter     | 3D object with arcball        |
|                              |                              |
|                              |                              |
|  5 mm                        |                              |
| |-------------|              |              |-------------| |
|                              |                   20 km      |
+------------------------------+------------------------------+
```


## Minimal API Sketch

The example should stay close to the eventual public API:

```c
DvzAnnotation* sb2 = dvz_annotation_scalebar(
    panel2d, &(DvzScaleBarDesc){
                 .dimension = DVZ_DIM_X,
                 .anchor = DVZ_SCENE_ANCHOR_BOTTOM_LEFT,
                 .label_position = DVZ_SCALEBAR_LABEL_ABOVE,
                 .target_length_px = 120,
                 .min_length_px = 70,
                 .max_length_px = 180,
                 .unit = "m",
                 .data_to_unit = 1e-6,
             });

DvzAnnotation* sb3 = dvz_annotation_scalebar(
    panel3d, &(DvzScaleBarDesc){
                 .dimension = DVZ_DIM_X,
                 .anchor = DVZ_SCENE_ANCHOR_BOTTOM_RIGHT,
                 .label_position = DVZ_SCALEBAR_LABEL_BELOW,
                 .target_length_px = 140,
                 .unit = "m",
                 .data_to_unit = 1.0,
                 .flags = DVZ_SCALEBAR_FLAGS_REFERENCE_POINT,
             });
```

The 3D call should also set the reference point or depth once that API is finalized.


## Runtime Behavior

The left panel should prove that the scale bar updates as users zoom:

1. zoomed out: label might show `2 cm`,
2. medium view: label might show `5 mm`,
3. close view: label might show `500 um`.

The right panel should prove that 3D scale is tied to an explicit reference, not inferred from the
entire perspective view. Rotating the object should keep the screen-space scale-bar placement fixed
while the semantic scale follows the configured reference policy.


## Validation

Initial validation should include:

1. offscreen screenshot/readback where both panels have visible bar and text pixels,
2. a deterministic panzoom step that changes the left-panel label,
3. a resize step that keeps the bar within its configured min/max pixel bounds,
4. a 3D reference-scale smoke proving the label is generated from the explicit reference policy,
5. a destroy/hide check proving no stale segment or glyph visual remains.


## Open Questions

1. Should the public constructor be `dvz_annotation_scalebar()` or shorter `dvz_scalebar()`?
2. Should micrometers use ASCII `um` everywhere, or should exported figures allow Unicode micro
   replacement later?
3. Should the default 2D domain source be the full panel viewport or the plot rectangle after axis
   reservations?
4. Should the first 3D reference API accept a world point, camera depth, attached visual, or all
   three as separate modes?
