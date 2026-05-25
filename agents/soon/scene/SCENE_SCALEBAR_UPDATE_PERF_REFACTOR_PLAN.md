# Scene Scale Bar Update Performance Refactor Plan

> **Execution Status**
> - **Status:** `PLANNED`
> - **Updated on:** `2026-05-25`
> - **Purpose:** reduce retained scale-bar update churn during panzoom/domain changes.


## Problem

The first retained 2D scale-bar slice is correct, but the update path is too expensive for live
interaction. Every visible-domain change currently prepares the annotation again, and the text side
can rebuild the label visual and glyph visual even when most state is unchanged.

This is particularly visible during panzoom:

1. the scale-bar segment endpoint geometry changes as the visible domain changes;
2. the formatted label changes only at nice-length thresholds;
3. the current path still re-enters text preparation on every relevant domain change;
4. label-length growth can dirty and resize glyph vertex buffers;
5. buffer growth forces extra DRP2 resource churn and makes the app/runtime path more fragile.

The stale upload-pointer crash fixed by `scene: keep render emission from mutating visuals` removed
one correctness hazard, but it did not make scale-bar updates cheap.


## Goals

1. Keep panzoom scale-bar updates proportional to the data that actually changed.
2. Update segment geometry every frame only when the visible-domain-derived scale length changes.
3. Rebuild text strings and glyph layout only when the formatted label or text style changes.
4. Avoid repeated glyph buffer reallocation for small dynamic labels.
5. Preserve the existing DPI/logical-to-physical-pixel contract and `_scene_screen_scale()`.
6. Keep the scale-bar implementation on top of normal scene visuals and DRP2 uploads.


## Non-Goals

1. Do not introduce a separate scale-bar DPI, logical-pixel, or physical-pixel model.
2. Do not add a custom scale-bar renderer or bypass the scene visual/FramePlan path.
3. Do not implement perspective/3D reference scale bars here; that remains a separate follow-up.
4. Do not optimize general text shaping beyond what scale-bar labels require.


## Proposed Design

### 1. Split Scale-Bar Realization State

Add retained scale-bar realization bookkeeping to `DvzAnnotation` or a scale-bar-private nested
state:

1. last visible domain for the measured dimension;
2. last `units_per_px`;
3. last chosen `length_units`;
4. last chosen `length_px`;
5. last formatted label string;
6. last segment anchor/end/tick coordinates;
7. last resolved screen scale used for screen-space style values;
8. cached text renderer/style fields relevant to glyph realization.

The scale-bar should compare the new resolved state against the cached state before deciding which
derived visuals to dirty.


### 2. Use Dirty Classes Instead Of Rebuilding Everything

Classify each update into independent dirty classes:

1. `geometry_dirty`: segment endpoints/ticks changed;
2. `label_position_dirty`: label anchor position changed but label text/style did not;
3. `label_text_dirty`: formatted label changed;
4. `style_dirty`: colors, line width, font size, renderer, or DPI screen scale changed;
5. `visibility_dirty`: annotation becomes hidden or visible again.

Expected behavior:

1. `geometry_dirty` updates only the segment visual data.
2. `label_position_dirty` updates only the text visual `position` attribute.
3. `label_text_dirty` updates strings and glyph data.
4. `style_dirty` updates only the affected visual attributes unless renderer/font changes require
   a glyph rebuild.
5. no dirty class means no visual mutation and no upload.


### 3. Reserve Small Dynamic Label Capacity

Scale-bar labels are short and bounded in practice. Add a text/glyph reservation path for retained
annotation labels, ideally as a generic internal text option rather than a scale-bar-only hack.

Candidate approach:

1. add an internal `min_glyph_vertices` or `reserved_glyph_vertices` field on the text visual state;
2. let scale-bar labels reserve enough vertices for common labels, for example 12 ASCII glyphs;
3. keep unused reserved vertices zeroed/transparent;
4. keep draw count and upload count consistent with the reserved capacity;
5. avoid buffer growth when the label moves from `2 mm` to `200 um`, `1 cm`, etc.

This should be tested carefully because generic text tests often expect exact glyph item counts.
If exact-count semantics are important for public inspection, expose both realized glyph count and
reserved upload capacity internally.


### 4. Do Not Reprepare During Render Emission

Maintain the current corrected ownership:

1. upload emission prepares/mutates derived visuals;
2. render emission treats visuals as read-only;
3. FramePlan upload nodes may borrow visual data only until DRP2 stream emission copies it;
4. render emission must not call text, colorbar, legend, axis, or scale-bar preparation helpers.

Keep the regression test that asserts scale-bar glyph upload sources survive render emission.


### 5. Make Label Changes Observable In Tests

Add focused tests that simulate panzoom/domain updates without Vulkan:

1. repeated domain changes where label remains unchanged should not dirty/re-upload glyph attrs;
2. repeated domain changes where only label position changes should update `position` only;
3. crossing a nice-length threshold should update strings/glyphs exactly once;
4. DPI screen-scale changes should dirty screen-space line width and text size through the existing
   resolver;
5. reserved-capacity label growth should not force buffer resource id churn in the emitted DRP2
   stream.


## Implementation Phases

### Phase 1: Instrument And Lock Current Churn

Add tests or debug counters around `_scalebar_prepare_visual()` and `_text_visual_prepare()` to
measure current scale-bar churn under repeated domain changes. Use this to prove the refactor
reduces:

1. `dvz_visual_set_strings()` calls;
2. glyph visual attribute replacements;
3. glyph buffer upload commands;
4. DRP2 create-buffer commands for scale-bar glyph attrs.


### Phase 2: Dirty-State Refactor

Refactor `_scalebar_prepare_visual()` so it first computes a value-only resolved state, compares it
with cached realization state, and then applies the minimum visual mutations needed.

Keep this phase CPU-only and covered by scene tests.


### Phase 3: Reserved Glyph Capacity

Add the internal reserved-capacity text/glyph path and enable it for scale-bar labels. Keep this
small and deterministic; scale-bar labels should not become a general dynamic text-layout feature.


### Phase 4: Runtime Validation

Validate with the minimal live example and the two-panel example:

1. `just test scalebar`
2. `just test scene`
3. `just example-c scalebar_minimal auto 120`
4. `just example-c scalebar_minimal bitmap auto 120`
5. `just example-c scalebar_2d_3d 120`
6. `DVZ_DRP2_TRACE=normal NO_COLOR=1 just example-c scalebar_minimal auto 120`

Use the DRP2 trace to confirm that unchanged labels do not emit repeated glyph buffer recreation.


## Acceptance Criteria

1. Panzooming the minimal scale-bar example does not rebuild glyph data on every frame.
2. Label text rebuilds happen only when the formatted label changes or text style changes.
3. Segment visual updates still track visible-domain changes correctly.
4. No scale-bar code introduces an independent DPI or pixel-scale resolver.
5. Existing scale-bar rendering and stream-order tests continue to pass.
6. The app/runtime path remains stable under automated zoom with both MSDF and bitmap text renderers.
