# v0.4 Core Release Proofs

> **Example status:** release proof bundle
> **Target:** C examples and deterministic fixtures
> **Data:** inline or deterministic synthetic data
> **Validation:** smoke, screenshot/readback, and focused interaction checks

These scenarios prove that the retained scene stack can create small, stable, user-facing examples.
They replace the old one-file-per-core-example notes for point, axes, linked panels, marker picking,
sphere impostors, volume, and scale bars.


## `point_2d`

Smallest retained-scene smoke: one full panel, 2D point visual, panzoom or offscreen render, and a
bounded data upload. It should stay boring and deterministic.

Minimal target: runnable C example that creates a figure, one panel, a point visual, synthetic
positions/colors/sizes, and one screenshot or bounded window loop.


## `path_axes_2d`

First axes/text regression target: a path or line-strip visual in data coordinates with linear axes,
ticks, labels, grid/spines, and panzoom.

Minimal target: one path over a synthetic curve, generated ticks, rendered labels, and a screenshot
that makes axis placement and clipping obvious.


## `linked_panels_axes_panzoom`

Multi-panel proof for linked controller semantics. The important behavior is shared X or shared
XY panzoom with independent panel bounds and axis refresh.

Minimal target: two or three panels with different visual families, synchronized panzoom state, and
axes that update without rebuilding unrelated resources.


## `linked_panels_probe_colorbar`

Main 2D explanatory-object pressure test. It combines shared sampled fields, image probe requests,
crosshair/readout state, colorbar, and linked panels.

Minimal target: two image panels backed by the same field or compatible fields, one probe result
that updates a retained readout, and one continuous colorbar with stable scale identity.


## `marker_picking`

Marker visual and item-pick proof. Bounding-box GPU picking is acceptable for v0.4; exact SDF
marker hit testing can follow.

Minimal target: marker scatter with stable item ids, hover/click pick result, and a simple selection
highlight that proves the picked id propagates back into scene styling.


## `sphere_impostor`

Small 3D quality proof for analytic sphere impostors, depth, lighting, and camera interaction.

Minimal target: deterministic sphere cloud with per-item radii/colors, arcball or turntable camera,
and one offscreen capture. SSAO/material polish may be shown in a showcase but should not bloat this
fixture.


## `volume`

Combined replacement for the old volume slice and volume offscreen notes. This scenario proves that
3D sampled fields can be bound, sliced or rendered, colormapped, and captured deterministically.

Minimal target: one synthetic 3D scalar field, one slice or simple volume mode, transfer/value range
controls as constants, and offscreen readback or screenshot validation.


## `scale_bar`

Narrow proof for retained scale bars in 2D and 3D contexts.

Minimal target: one 2D panel and one 3D panel with scale bars whose labels update only when the
formatted value or relevant style changes.


## `image_probe`

Focused public image-query proof for scalar sampled fields. It keeps the source field scalar,
renders through a custom LUT colormap and continuous colorbar, and shows a compact live scalar
readout under the cursor.

Current proof: `examples/c/features/image_probe.c`.

Minimal target: one scalar image field, shared scale/colormap/colorbar, GPU-backed pixel hit/miss
query, crosshair marker, and live scalar readout aligned with the rendered plot area.


## Blockers To Track In Planning

Release stage, readiness, and global blockers belong in [../../PLANNING.md](../../PLANNING.md).
Keep this file focused on what each proof draws and how it is validated.
