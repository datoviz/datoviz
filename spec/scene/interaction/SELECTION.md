# Selection And Linked Highlight

This document defines active and planned scene-level selection state, highlight rendering,
cross-visual linking, and lasso selection.


## Purpose

Selection is a first-class scene concern:

1. the user (or app code) identifies a subset of items,
2. the scene propagates that subset to all linked visuals as a visual highlight,
3. the app can read back the selected index set for downstream use.

Selection state is owned by the scene, not by individual visuals or controllers.


## Current v0.4 Slice (May 2026)

The active implementation covers the narrow visible highlight path for point and marker visuals:

1. `DvzSelection` keeps the CPU-side selected item set.
2. `dvz_selection_apply_pick()` updates `uint8` per-item masks on matching point and marker
   visuals, including visuals linked by scene link keys.
3. Selection mask updates mark the affected visuals dirty so the scene emits a fresh frame plan.
4. The scene emits an optional `selection` attribute, DRP2 uploads it as `uint8`, and point/marker
   shaders render selected items at full alpha while dimming unselected items.
5. `examples/c/techniques/pick_hover.c` demonstrates click selection and linked point/marker
   highlighting.

The broader design below still tracks planned follow-ups: explicit visual attachment APIs,
parametrizable input maps, highlight descriptors beyond alpha dimming, image/mesh/path/volume/text
selection, box/lasso selection, and cross-scene selection synchronization.


## `DvzSelection`

The target `DvzSelection` design is a scene-level handle that owns:

1. a set of selected item indices,
2. a per-item `uint8` mask buffer (see Implementation Note below),
3. a highlight descriptor controlling how selected and unselected items look,
4. an input mapping defining which input gestures trigger which selection actions.

```text
DvzSelection* sel = dvz_selection(scene, &highlight_desc)
```

The target design lets multiple visuals attach to a selection handle to share selection state:

```text
dvz_visual_set_selection(visual_a, sel)
dvz_visual_set_selection(visual_b, sel)
```

When the selection changes, the mask buffer is updated and all attached or synchronized visuals
are marked dirty for highlight. No geometry rebuild is needed.

Multiple independent `DvzSelection` handles may coexist in the same scene.


## Selection Modes

| Mode | Description |
|---|---|
| `DVZ_SELECT_REPLACE` | new selection replaces the current one |
| `DVZ_SELECT_ADDITIVE` | new items are added to the current selection |
| `DVZ_SELECT_SUBTRACT` | matching items are removed from the current selection |
| `DVZ_SELECT_TOGGLE` | matching items toggle between selected and unselected |


## Parametrizable Input Mapping

Selection actions are not hardcoded to specific keys or buttons.
Each selection handle carries a configurable `DvzSelectionInputMap` that maps input
combinations to selection mode actions:

```text
dvz_selection_set_input(sel, DVZ_SELECT_REPLACE,  DVZ_MOUSE_LEFT,      DVZ_MOD_NONE)
dvz_selection_set_input(sel, DVZ_SELECT_ADDITIVE, DVZ_MOUSE_LEFT,      DVZ_MOD_SHIFT)
dvz_selection_set_input(sel, DVZ_SELECT_BOX,      DVZ_MOUSE_LEFT_DRAG, DVZ_MOD_NONE)
dvz_selection_set_input(sel, DVZ_SELECT_LASSO,    DVZ_MOUSE_LEFT_DRAG, DVZ_MOD_ALT)
```

A sensible default mapping is provided; the user overrides only the entries they need.
Input combinations not present in the map are ignored by the selection system and forwarded
to other controllers.

Programmatic selection bypasses the input map entirely:

```text
dvz_selection_set(sel, indices, count, DVZ_SELECT_REPLACE)
```


## Highlight Descriptor

`DvzHighlightDesc` declares how selected and unselected items look relative to their base
visual style.

| Field | Type | Default | Description |
|---|---|---|---|
| `selected_alpha_mult` | `float32` | `1.0` | alpha multiplier for selected items |
| `unselected_alpha_mult` | `float32` | `0.3` | alpha multiplier for unselected items |
| `selected_color` | `rgba_u8` or null | null | if set, overrides fill color for selected items |
| `selected_size_mult` | `float32` | `1.0` | size multiplier for selected items |
| `selected_z_layer` | `int32` | `+1` | z-layer offset for selected items — draws on top |
| `selected_edgecolor` | `rgba_u8` or null | null | if set, adds/overrides stroke edge for selected items |
| `selected_linewidth` | `float32` | `1.5` | edge width for `selected_edgecolor` when set |

All fields are optional — zero-initialize for "no highlight change on that dimension."

The default descriptor (dimmed unselected, full-opacity selected, drawn on top) is:

```text
DvzHighlightDesc default = {
    .selected_alpha_mult   = 1.0,
    .unselected_alpha_mult = 0.3,
    .selected_z_layer      = 1,
}
```

The highlight descriptor can be updated at any time:

```text
dvz_selection_set_highlight(sel, &new_desc)
```


## Selection Readback

The app can query the current selected index set:

```text
uint32_t* indices
uint32_t  count
dvz_selection_get(sel, &indices, &count)
```

Returns a pointer to the scene-owned index array and its count.
Valid until the next selection change.
The app must copy the data if it needs to retain it beyond the next frame.

This is a CPU-side readback of scene-owned state — not a GPU readback.
The mask buffer on the GPU is a derived resource; the authoritative index set lives on the CPU
inside `DvzSelection`.


## Deselect

Clicking on empty space (no item picked) deselects all items.
This is a controller behavior: when a click produces an empty pick result and the selection
input map has a `DVZ_SELECT_REPLACE` entry for that input, the selection is cleared.

Programmatic deselect:

```text
dvz_selection_clear(sel)
```


## Click And Box Selection Flow

**Click selection:**

1. controller triggers a pick request on mouse click,
2. picking readback returns a scene item identity (or empty),
3. controller applies the selection mode from the input map,
4. `DvzSelection` updates its index set and mask buffer,
5. attached or synchronized visuals are marked dirty for highlight,
6. next frame: `UploadNode` uploads the updated mask; highlight renders.

**Box selection:**

1. controller tracks drag start and end in screen space,
2. on drag release, the scene computes which items fall inside the screen-space box,
3. for small datasets: CPU-side position test using the current panel transform,
4. for large datasets: GPU `ComputeNode` (see Lasso section — same mechanism),
5. `DvzSelection` updates index set and mask.


## Lasso Selection

Lasso selection is GPU-accelerated and scales to millions of items without position readback.

**User interaction:**

1. user draws a freehand lasso polygon on screen during a drag gesture,
2. the scene accumulates screen-space polygon vertices on the CPU,
3. on drag release, the scene triggers lasso evaluation.

**GPU evaluation via `ComputeNode`:**

The scene inserts a `ComputeNode` into the next `FramePlan`:

1. **Inputs**:
   - visual position buffer (already on GPU),
   - current panel transform uniforms (already on GPU — projects data positions to screen space),
   - lasso polygon vertices (small CPU upload, typically ≤ 256 points).
2. **Output**: the `DvzSelection` mask buffer (`uint8` per item, `1` = inside lasso).
3. **Algorithm**: standard GPU point-in-polygon (ray casting), one thread per item.

After the compute pass, the mask buffer is consumed by the visual shaders in the same frame's
render pass.
CPU index synchronization from the GPU mask buffer is **opt-in**. The application calls:

```c
dvz_selection_sync(sel);
```

This triggers a one-shot GPU→CPU readback; the result is available on the next rendered frame
via `dvz_selection_count(sel)` and `dvz_selection_get(sel, ...)`. Automatic readback is not
performed every frame — only when explicitly requested.

**Screen-space projection:**

The compute shader projects each item's data-space position to screen space using the same
panel transform that is already available as a GPU uniform.
No special handling is needed for zoom or pan — the transform is current.

**Lasso and the FramePlan:**

```text
FramePlan:
  UploadNode    — lasso polygon vertices
  ComputeNode   — point-in-polygon → writes DvzSelection mask buffer
  RenderNode    — visual render, reads mask buffer for highlight
```

The `ComputeNode` is only inserted when a lasso evaluation is pending.
Normal frames with no pending lasso have no compute overhead.


## Implementation Note (Non-Normative)

This section describes the preferred GPU-side implementation approach for selection highlight.
It is not normative for the scene contract but is recorded here because it drives important
architectural constraints.

**Mask buffer:**
The current point/marker path stores one visual-local `uint8` attribute per item. The broader
target keeps the same mask semantics and may realize that mask as a `BufferResource` owned by
`DvzSelection`, or as one buffer per visual depending on item count.
Value `1` = selected, `0` = unselected.

**On selection change (click/box/programmatic):**
An `UploadNode` writes the updated mask buffer.
Visual shaders read the mask at fragment or vertex time and apply the highlight descriptor
fields (alpha, color, size, z-layer, edge).

**On lasso:**
A `ComputeNode` writes the mask buffer directly on the GPU.
No CPU round-trip is needed for the highlight itself.

**Why not re-upload color arrays:**
Re-writing per-item color data on every selection change is O(N) in item count and requires a
full buffer upload. For 10M-item point clouds, this is prohibitive.
The mask buffer approach is O(N) only for the compute/upload of a small `uint8` array, while
the color arithmetic moves to the shader.


## Constraints And Limitations

**Binary highlight only:** Selection highlighting is binary (selected / unselected). Per-item
multi-level emphasis (primary, secondary, hover stacking) is not supported in v0.4 and is a
v0.4+ concern.

**One selection per visual:** A single `DvzSelection` per visual is supported. Multiple named
selection groups (primary, secondary, hover layers with stacked mask buffers) are not supported
in v0.4.


## Relationship To Other Documents

| Document | Relationship |
|---|---|
| `PICKING.md` | click selection uses the picking pipeline for item identity |
| `CONTROLLERS.md` | controllers trigger selection actions; input map lives on `DvzSelection` |
| `../pipeline/FRAME_PLAN.md` | lasso inserts `ComputeNode`; selection change inserts `UploadNode` |
| `INVALIDATION_AND_CACHING.md` | selection change marks highlight-dirty scope |
| `RESOURCE_MODEL.md` | mask buffer is a `BufferResource` owned by `DvzSelection` |
