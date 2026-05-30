# Selection And Linked Highlight

This document defines active and planned scene-level selection state, item-state highlight
rendering, cross-visual linking, and lasso selection.


## Purpose

Selection is a first-class scene concern:

1. the user (or app code) identifies a subset of items,
2. the scene propagates that subset to all linked visuals as a visual highlight,
3. the app can read back the selected index set for downstream use.

Selection state is owned by the scene, not by individual visuals or controllers.


## Current v0.4 Slice (May 2026)

The active implementation covers the retained item-state highlight path for point-like visuals:

1. `DvzSelection` keeps the CPU-side selected item set.
2. `dvz_selection_apply_query()` writes `DVZ_ITEM_STATE_SELECTED` into the `uint32_t`
   `item_state` bitfield on matching point, pixel, and marker visuals, including visuals linked by
   scene link keys.
3. Item-state updates mark the affected visuals dirty so the scene emits a fresh frame plan.
4. The scene emits optional `item_state` attributes and `item_state_style` uniforms. DRP2 uploads
   them as separate resources, and point/pixel/marker item-state shaders apply selected and
   contextual unselected styling.
5. `examples/c/techniques/pick_hover.c` demonstrates click selection and linked point-like
   highlighting.

The broader design below still tracks planned follow-ups: explicit visual attachment APIs,
parametrizable input maps, richer highlight descriptors, image/mesh/path/volume/text selection,
box/lasso selection, and cross-scene selection synchronization.


## `DvzSelection`

The target `DvzSelection` design is a scene-level handle that owns:

1. a set of selected item indices,
2. a derived per-visual `item_state` bitfield update for selected items,
3. a visual style descriptor controlling how selected and contextual unselected items look,
4. an input mapping defining which input gestures trigger which selection actions.

```text
DvzSelection* sel = dvz_selection(scene, &(DvzSelectionDesc){
    .mode = DVZ_SELECT_TOGGLE,
    .target = DVZ_SCENE_TARGET_ITEM,
});
```

The target design lets multiple visuals attach to a selection handle to share selection state:

```text
dvz_visual_set_selection(visual_a, sel)
dvz_visual_set_selection(visual_b, sel)
```

When the selection changes, attached or synchronized visuals update their `item_state` attributes
and are marked dirty for highlight. No geometry rebuild is needed.

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

Programmatic selection bypasses the input map. The active API applies one resolved query result or
clears the selection:

```text
dvz_selection_apply_query(sel, &query)
dvz_selection_clear(sel)
```

A bulk selected-item setter remains a future API addition.


## Selection Visual Style

`DvzSelectionVisualStyle` declares how selected and contextual unselected items look relative to
their base visual style. It is composed from two `DvzItemStateVisualStyle` descriptors:

| Field | Type | Description |
|---|---|---|
| `selected` | `DvzItemStateVisualStyle` | applied to items with `DVZ_ITEM_STATE_SELECTED` |
| `unselected` | `DvzItemStateVisualStyle` | applied to items without `DVZ_ITEM_STATE_SELECTED` while the selection is non-empty |

The active point-like slice supports alpha, tint, and scale flags:

| Flag | Effect |
|---|---|
| `DVZ_ITEM_STATE_VISUAL_ALPHA` | multiply item alpha by `alpha` |
| `DVZ_ITEM_STATE_VISUAL_TINT` | mix item color toward `tint` by `tint_mix` |
| `DVZ_ITEM_STATE_VISUAL_SCALE` | multiply point-like item size by `scale` |

The default style is selected-normal and unselected-dimmed:

```text
DvzSelectionVisualStyle style = dvz_selection_visual_style();
style.unselected.flags = DVZ_ITEM_STATE_VISUAL_ALPHA;
style.unselected.alpha = 0.25f;
```

The visual style can be updated at any time:

```text
dvz_selection_set_visual_style(sel, &style)
```


## Selection Readback

The app can query the current selected index set:

```text
uint32_t count = dvz_selection_count(sel);
DvzSelectionItem items[count];
dvz_selection_copy(sel, items, count);
```

`DvzSelectionItem` carries the resolved visual id, target kind/id, and link key for each selected
target. `dvz_selection_copy()` writes into caller-owned storage, so the result can be retained
independently of the next selection change.

This is a CPU-side readback of scene-owned state — not a GPU readback.
The GPU `item_state` attribute is a derived resource; the authoritative index set lives on the CPU
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
4. `DvzSelection` updates its index set and item-state bitfields,
5. attached or synchronized visuals are marked dirty for highlight,
6. next frame: `UploadNode` uploads the updated `item_state`; highlight renders.

**Box selection:**

1. controller tracks drag start and end in screen space,
2. on drag release, the scene computes which items fall inside the screen-space box,
3. for small datasets: CPU-side position test using the current panel transform,
4. for large datasets: GPU `ComputeNode` (see Lasso section — same mechanism),
5. `DvzSelection` updates index set and item-state bitfields.


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
2. **Output**: a selection result buffer that can be merged into `item_state`
   (`DVZ_ITEM_STATE_SELECTED` for selected items).
3. **Algorithm**: standard GPU point-in-polygon (ray casting), one thread per item.

After the compute pass, the selection result is merged into the visual `item_state` path and
consumed by the visual shaders in the same frame's render pass.
CPU index synchronization from the GPU result buffer is **opt-in**. The application calls:

```c
dvz_selection_sync(sel);
```

This triggers a one-shot GPU→CPU readback; the result is available on the next rendered frame
via `dvz_selection_count(sel)` and `dvz_selection_copy(sel, ...)`. Automatic readback is not
performed every frame — only when explicitly requested.

**Screen-space projection:**

The compute shader projects each item's data-space position to screen space using the same
panel transform that is already available as a GPU uniform.
No special handling is needed for zoom or pan — the transform is current.

**Lasso and the FramePlan:**

```text
FramePlan:
  UploadNode    — lasso polygon vertices
  ComputeNode   — point-in-polygon → writes selection result buffer
  Upload/Copy   — merges result into item_state
  RenderNode    — visual render, reads item_state for highlight
```

The `ComputeNode` is only inserted when a lasso evaluation is pending.
Normal frames with no pending lasso have no compute overhead.


## Implementation Note (Non-Normative)

This section describes the preferred GPU-side implementation approach for selection highlight.
It is not normative for the scene contract but is recorded here because it drives important
architectural constraints.

**Item-state buffer:**
The current point/pixel/marker path stores one visual-local `uint32_t` `item_state` attribute per
item. `DvzSelection` writes `DVZ_ITEM_STATE_SELECTED`; `DvzHover` writes
`DVZ_ITEM_STATE_HOVERED`; future interaction writers should add bits to the same field.

**On selection change (click/box/programmatic):**
An `UploadNode` writes the updated `item_state` buffer. Visual shaders read the bitfield at
fragment or vertex time and apply the configured item-state visual style fields: alpha, tint, and
scale in the active point-like slice.

**On lasso:**
A `ComputeNode` should write a GPU selection result that is merged into the same item-state
bitfield. No CPU round-trip is needed for the highlight itself.

**Why not re-upload color arrays:**
Re-writing per-item color data on every selection change is O(N) in item count and requires a
full buffer upload. For 10M-item point clouds, this is prohibitive.
The item-state approach is O(N) only for the compute/upload of a compact `uint32_t` state array,
while the color and scale arithmetic moves to the shader.


## Constraints And Limitations

**Binary selection only:** Selection highlighting is binary (selected / unselected). Hover can
compose with selection through `item_state`, but multiple selection groups with independent styles
are a v0.4+ concern.

**One selection per visual:** A single `DvzSelection` per visual is supported. Multiple named
selection groups (primary, secondary, linked groups with independent styles) are not supported in
v0.4.


## Relationship To Other Documents

| Document | Relationship |
|---|---|
| `PICKING.md` | click selection uses the picking pipeline for item identity |
| `CONTROLLERS.md` | controllers trigger selection actions; input map lives on `DvzSelection` |
| `../pipeline/FRAME_PLAN.md` | lasso inserts `ComputeNode`; selection change inserts `UploadNode` |
| `INVALIDATION_AND_CACHING.md` | selection change marks highlight-dirty scope |
| `ITEM_STATE_SYSTEM.md` | selection writes `DVZ_ITEM_STATE_SELECTED` and shares item-state style plumbing |
