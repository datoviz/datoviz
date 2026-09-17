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
