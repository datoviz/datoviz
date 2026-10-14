# Retained GUI data widgets

Use `dvz_gui_tree()` and `dvz_gui_table()` when a native Datoviz tool needs a hierarchy or a typed tabular view. Both widgets own copied model data, use stable `uint64_t` row keys, and redraw clipped rows without rebuilding caller buffers in every GUI frame.

## Build the models once

Create the handles after `dvz_view_gui()` has attached the native Dear ImGui overlay. Install tree rows with depth-first parent indices and table rows followed by complete typed columns. The setters copy labels, keys, values, and styles, so temporary input arrays may be released after each successful call.

The compact tree presentation uses small noninteractive swatches, bold primary labels, and subdued secondary labels by default. Add keyed styles only for rows that need an accent, disabled treatment, or explicit color override.

```c
uint64_t keys[] = {10, 11};
uint32_t parents[] = {UINT32_MAX, 0};
const char* labels[] = {"Root", "Child"};
DvzGuiTree* tree = dvz_gui_tree("inspector-tree", DVZ_GUI_DATA_WIDGET_FLAGS_FILTER);
dvz_gui_tree_set_rows(tree, 2, keys, parents, labels, NULL, DVZ_GUI_DATA_SET_FLAGS_NONE);
```

Table columns are declared once with stable IDs and typed setters such as `dvz_gui_table_set_column_double()` or `dvz_gui_table_set_column_color()`. Replacing rows preserves selection for surviving keys unless `DVZ_GUI_DATA_SET_FLAGS_RESET_STATE` is requested.

## Draw and handle events

Call each widget's draw function inside the registered GUI callback, after `dvz_gui_begin()` and before `dvz_gui_end()`. Pass one event buffer to each draw, then use `written` and `dropped` to decide whether to apply the compact selection, activation, expansion, sorting, or filtering event. A dropped event requires resynchronizing widget state with the query functions.

The canonical combined example is [`features_gui_data_widgets`](../examples/features/gui-data-widgets.md), with source in [`examples/c/features/gui_data_widgets.c`](https://github.com/datoviz/datoviz/blob/main/examples/c/features/gui_data_widgets.c). It is native-only because Dear ImGui overlays are not part of the scene/WASM WebGPU route.

The generated Python facade provides ergonomic setters, draw calls, and state queries for these opaque handles. The Python adaptation follows the same one-callback, copied-model pattern as the C example; use it when the native GUI dependencies are available.

## Host-controlled trees

Use `DVZ_GUI_DATA_WIDGET_FLAGS_CONTROLLED` when application code already owns the displayed order and interaction semantics. Install copied model rows first, then call `dvz_gui_tree_set_projection()` with ordered existing keys, display depths, disclosure capability, and selection eligibility. The projection is transactional and may omit ancestors, reorder keys, or show ranked depth-zero leaves. Its depths and branches describe presentation; semantic parents remain application-owned. Replacing model rows clears the projection and annotations.

Controlled draw bypasses native visibility, filtering, ancestor expansion, and range selection. Install host selection and expansion with the existing copied setters. Row clicks, disclosure clicks, navigation keys, and hover/focus transitions become input intents; draw never changes selection or expansion. Nonselectable branch bodies still emit row clicks so the host can apply branch behavior, while nonselectable leaves suppress row clicks. Disclosure remains independent of selection eligibility. Disabled row styles disable both inputs.

Provide at least `DVZ_GUI_TREE_CONTROLLED_EVENT_CAPACITY` event slots (32). A smaller buffer returns `DVZ_ERROR` with zero written and dropped counts before draw context or input is consumed, so the caller may retry with sufficient storage in the same frame. Draw emits at most eleven navigation key intents, one row click, one disclosure click, one activation, two hover transitions, and two focus transitions: eighteen events total. Modifier flags use `DvzKeyModifier`; navigation details use `DvzKeyCode`. Hover and focus details use 1 for entry and 0 for exit, carrying the affected row key. Clear host hover when hiding the pane, because a widget that is not drawn cannot report pointer exit.

`dvz_gui_tree_set_focus()` includes a projected row in the clipper, focuses it, and scrolls to it. Controlled `dvz_gui_tree_reveal()` scrolls to a projected row without expanding any ancestor. Keys outside the projection return `DVZ_ERROR`. Directional keys emit host intents while native directional movement is canceled; Tab remains available to leave the widget.

`dvz_gui_tree_set_annotations()` copies formatted value text, missing status, complete tooltip text, and multiple UTF-8 highlight spans for primary and secondary labels. Spans are sorted, nonoverlapping byte ranges whose endpoints must lie on codepoint boundaries. Build descriptors with `dvz_gui_tree_annotation()`; zero count clears all annotations. The application provides value formatting, match locations, and lineage text. The retained widget performs no scientific calculations or hierarchy aggregation. The generated Python facade accepts NumPy projection arrays or typed sequences and lists of `DvzGuiTreeAnnotation` records, with counts inferred; nested strings and span arrays need only remain alive until the setter returns.

These controls remain part of the native Dear ImGui overlay. They do not add a retained GUI widget to the scene/WASM WebGPU route.
