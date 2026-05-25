# Scene Scale-Bar Update Performance Refactor

> **Execution Status**
> - **Status:** `COMPLETE`
> - **Completed on:** `2026-05-25`
> - **Commit:** `22dbf18b scene: reduce scale bar update churn`
> - **Purpose:** reduce retained scale-bar update churn during panzoom/domain changes.


## Summary

The retained scale-bar update path now computes a resolved value-only state before mutating derived
visuals. It compares that state with the previous realization and updates only the retained payloads
that changed.

Implemented behavior:

1. Segment geometry/style changes update only the scale-bar segment visual.
2. Label anchor movement with unchanged text/style updates only the derived glyph `position`
   attribute.
3. Label text, renderer, color, size, anchor, or angle changes rebuild the text/glyph layout.
4. Unchanged repeated prepares avoid visual mutation and avoid dirtying upload payloads.
5. Scale-bar labels reserve 72 glyph vertices internally, enough for 12 short ASCII glyphs, so
   common label growth does not resize glyph upload buffers.


## Implementation Notes

New internal state:

1. `DvzScaleBarRealization` caches resolved scale-bar segment endpoints, style, label text,
   label position, text style, renderer, chosen length, and screen scale.
2. `DvzTextVisualState::realized_layout_version` distinguishes layout-affecting text changes from
   anchor-position-only changes.
3. `DvzTextVisualState::reserved_glyph_vertices` lets internal text producers reserve upload
   capacity without changing generic exact-count behavior.

The text visual realization path still performs full glyph layout when strings or layout/style
attributes change. When only the per-string `position` attribute changes, it rewrites only the
derived glyph visual's `position` attribute and leaves bounds, texcoords, colors, and angles
untouched.


## Validation

Focused validation:

1. `just build`
2. `just test scalebar`
3. `just test text_sdf_visual_realization`
4. `just test scene`
5. `direnv exec . just example-c scalebar_minimal auto 120`
6. `direnv exec . just example-c scalebar_minimal bitmap auto 120`
7. `direnv exec . just example-c scalebar_2d_3d 120`
8. `DVZ_DRP2_TRACE=normal NO_COLOR=1 direnv exec . just example-c scalebar_minimal auto 120`

Observed broad scene result:

1. `328/437` selected scene tests passed.
2. `109` Vulkan/runtime tests skipped because Vulkan instance creation failed in this shell.
3. No scene test failures remained after the generic SDF text exact-count regression was fixed.

Observed DRP2 trace behavior:

1. The first app frame creates the scale-bar glyph buffers at the reserved capacity.
2. Repeated unchanged frames are reported as unchanged by the app-frame trace.
3. Label-position-only frames write the glyph `position` buffer without recreating glyph buffers.
4. Label-change frames reuse the same glyph buffer ids and upload the full reserved glyph payload.
