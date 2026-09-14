# Retained GUI Data Widgets

Status: normative v0.4 target for tree and table widgets.

## Purpose

Datoviz GUI callbacks are immediate-mode: Python enters one callback for every rendered GUI frame, and each ordinary `dvz_gui_*` widget call crosses ctypes separately. This specification defines retained tree and table widgets that copy application data only when it changes and render the complete widget with one Python-to-C call per frame.

Datoviz v0.3 exposed batched `dvz_gui_tree()` and `dvz_gui_table()` calls, but its Python adapter rebuilt `char**` buffers for every call and its tree stored expansion and selection in caller-owned row-index arrays. The v0.4 API keeps the useful batching boundary while replacing those ownership and identity semantics.

## Scope

This specification governs opaque tree and table handles, copied input data, stable row identity, row styling, selection, filtering, tree expansion, table sorting, clipped drawing, compact interaction events, generated Python adaptation, and destruction.

The widgets are domain-neutral. Allen, Beryl, Cosmos, anatomical hierarchy rules, bilateral selection, and atlas mappings belong above Datoviz.

## Architectural Decisions

### Separate public widgets and a shared private core

`DvzGuiTree` and `DvzGuiTable` are separate opaque public handles. Their implementation shares private storage and behavior for row keys, styles, selection, visibility, filtering, event revisions, and clipped display order, but Datoviz does not expose a universal cell or data-widget object.

Each handle has an application-defined stable widget ID copied at creation. The ID scopes ImGui state and remains stable across handle addresses and process runs so table layout persistence does not depend on allocator behavior when `DvzGuiConfig.ini_path` enables ImGui persistence. Every table column also has a stable unique application-defined column ID.

Handles are independent of a particular `DvzGui` at creation and are drawn inside an active callback with an explicit `DvzGui*`. They are not thread-safe and must not be mutated concurrently with drawing. The caller owns each handle and must destroy it after its last draw.

### Copy on mutation, never retain caller arrays

Row, label, column, style, selection, and visibility setters copy their inputs atomically before returning. Every failed validation or allocation leaves the previous state unchanged. The first implementation accepts copied `const char* const*` label arrays because that is simple in C and generated ctypes; a packed UTF-8 blob and offset setter may be added later if ingestion benchmarks show a material benefit.

Setting zero rows clears a widget. Tree primary labels and the primary-label array must not be `NULL` when the row count is nonzero. A `NULL` secondary-label array means that no secondary labels are installed, while a `NULL` individual secondary label means an empty label. Malformed UTF-8 is rejected. Table `set_rows()` installs new keys and resets every column to its typed missing or default value; callers then install complete typed columns before the next draw.

No input array or string is rebuilt or copied during `draw()`. Full data replacement preserves selection and expansion for surviving stable keys unless an explicit reset flag requests fresh UI state.

Tree rows use unique `uint64_t` keys and validated parent indices in depth-first preorder. A root uses `UINT32_MAX`; every non-root parent must precede its child, descendants of one row must be contiguous, and input order defines sibling order. Key zero is valid. Row indices are storage positions only and never application identity.

Table rows use the same unique key rule. Table values are installed through typed column setters for text, signed integer, double, boolean, and color data. The public API does not expose a variant union or one setter call per cell.

### Cached display order and clipping

Trees are rendered from a cached flat visible-row order with explicit indentation and disclosure controls. The implementation does not maintain a nested `TreeNodeEx()` stack because nested tree stacks do not safely compose with `ImGuiListClipper`.

The tree display order is recomputed only when rows, expansion, filtering, or application visibility changes. Collapsed descendants are absent from that order. Filtering may temporarily reveal matching paths, but filter-driven effective expansion remains separate from user expansion so clearing the filter restores the exact prior state.

The table display order is recomputed only when rows, values participating in sort or filter, sorting, filtering, or application visibility changes. Drawing visits only the clipped display range.

### Selection and compact events

Selection is stored natively by stable key. Applications set selection in one batch and query selected keys only after receiving a selection revision event. An ordinary click replaces selection, Ctrl/Command-click toggles one row, and Shift-click replaces selection with the range between the anchor and clicked row in current visible display order after filtering and sorting. Disabled rows reject user interaction but may remain programmatically selected.

One action produces one compact event even when it changes many selected rows. Events contain only fixed-size fields: type, flags or modifiers, row key, column ID, detail, and monotonically increasing revision. Events never contain strings, pointers, or variant values.

`draw()` writes into a caller-provided event array and reports both the written count and dropped count. Passing `events == NULL` with zero capacity is valid. Passing a nonzero capacity with `events == NULL`, an invalid handle, or drawing outside the active GUI callback and a successful `dvz_gui_begin()`/`dvz_gui_end()` pair returns `DVZ_ERROR`. The written and dropped outputs are required. One handle may be drawn at most once per frame and becomes associated with the first `DvzGui` that draws it; it must not move to another GUI during that GUI lifetime.

Event revisions are monotonically increasing per handle. Selection-changing actions emit one summary event regardless of affected row count. When `dropped > 0`, the caller must treat all interaction state as potentially stale and resynchronize selection, expansion, filter, and sort state through query functions. Programmatic setters do not emit interaction events.

Required event kinds are selection changed, activation, expansion changed, sorting changed, and filtering changed. Context menus, editing, drag and drop, and arbitrary row actions are deferred.

### Styling

Tree and table rows share a batched style record with optional foreground, background, and accent colors plus a disabled flag. Presence flags distinguish an inherited color from transparent black. Tree swatches are separate row content, and table swatches use typed color columns.

Style precedence is channel-specific and deterministic. An explicit background is the base and hover or selection overlays blend over it; an explicit foreground remains active unless disabled treatment overrides it; disabled rows use the disabled foreground treatment and reject interaction. The exact blend constants are implementation-owned and tested visually.

A tree swatch is a standard noninteractive colored square in the first implementation. Swatch activation and editable color cells may be added later without changing row ownership.

### Filtering and commands

Both widgets support a copied UTF-8 filter query, a strict application visibility mask, and an application match mask. A false strict-visibility entry always hides the row. A true match entry participates in filtering and, for trees, reveals its visible ancestors. Native filtering is ASCII case-insensitive substring matching over tree labels and secondary labels or table text columns marked searchable; non-ASCII matching is byte-exact. Domain-specific fuzzy matching remains application-owned and uses the match mask.

The tree supports expand all, collapse all, expand to depth, batched expanded-key replacement, and reveal by key. Reveal expands ancestors and schedules the row to receive focus and scroll visibility during the next draw.

Both widgets may show an integrated search field through a creation flag. Expand-all and collapse-all buttons remain ordinary composable application controls calling the public commands rather than hard-coded tree chrome.

### Python boundary

The generated `datoviz` facade retains the C-shaped `dvz_*` names. It adapts Python string sequences and contiguous NumPy arrays according to explicit entries in `spec/bindings/ctypes.yml`; it does not introduce Python `Tree` or `Table` classes.

The intended steady-state cost for one visible retained widget is one native-to-Python GUI callback transition plus one Python-to-native draw call. Selection queries and model mutations occur only after events or application data changes.

## Initial Public Shape

The exact declarations belong in `include/datoviz/gui.h`. Datoviz-owned flags use `uint32_t`; plain `int` remains reserved for calls that forward upstream ImGui flags directly. `DVZ_GUI_COLUMN_NONE` is `UINT32_MAX` and identifies an event that does not concern one table column.

Growable input records use the repository's size-versioned struct convention and canonical initializers. Column titles and format strings are copied during table creation. Column IDs must be unique and remain stable across reconstruction. Color columns are non-sortable in the initial implementation.

```c
typedef enum DvzGuiDataEventType
{
    DVZ_GUI_DATA_EVENT_NONE = 0,
    DVZ_GUI_DATA_EVENT_SELECTION_CHANGED,
    DVZ_GUI_DATA_EVENT_ACTIVATED,
    DVZ_GUI_DATA_EVENT_EXPANSION_CHANGED,
    DVZ_GUI_DATA_EVENT_SORT_CHANGED,
    DVZ_GUI_DATA_EVENT_FILTER_CHANGED,
} DvzGuiDataEventType;

typedef enum DvzGuiDataWidgetFlags
{
    DVZ_GUI_DATA_WIDGET_FLAGS_NONE = 0,
    DVZ_GUI_DATA_WIDGET_FLAGS_MULTI_SELECT = 1u << 0,
    DVZ_GUI_DATA_WIDGET_FLAGS_FILTER = 1u << 1,
} DvzGuiDataWidgetFlags;

typedef enum DvzGuiDataSetFlags
{
    DVZ_GUI_DATA_SET_FLAGS_NONE = 0,
    DVZ_GUI_DATA_SET_FLAGS_RESET_STATE = 1u << 0,
} DvzGuiDataSetFlags;

typedef enum DvzGuiDataStyleFlags
{
    DVZ_GUI_DATA_STYLE_FLAGS_NONE = 0,
    DVZ_GUI_DATA_STYLE_FLAGS_FOREGROUND = 1u << 0,
    DVZ_GUI_DATA_STYLE_FLAGS_BACKGROUND = 1u << 1,
    DVZ_GUI_DATA_STYLE_FLAGS_ACCENT = 1u << 2,
    DVZ_GUI_DATA_STYLE_FLAGS_DISABLED = 1u << 3,
} DvzGuiDataStyleFlags;

typedef enum DvzGuiTableColumnFlags
{
    DVZ_GUI_TABLE_COLUMN_FLAGS_NONE = 0,
    DVZ_GUI_TABLE_COLUMN_FLAGS_SORTABLE = 1u << 0,
    DVZ_GUI_TABLE_COLUMN_FLAGS_SEARCHABLE = 1u << 1,
    DVZ_GUI_TABLE_COLUMN_FLAGS_STRETCH = 1u << 2,
} DvzGuiTableColumnFlags;

typedef enum DvzGuiTableColumnType
{
    DVZ_GUI_TABLE_COLUMN_TEXT = 0,
    DVZ_GUI_TABLE_COLUMN_INT64,
    DVZ_GUI_TABLE_COLUMN_DOUBLE,
    DVZ_GUI_TABLE_COLUMN_BOOL,
    DVZ_GUI_TABLE_COLUMN_COLOR,
} DvzGuiTableColumnType;

typedef struct DvzGuiDataEvent
{
    uint32_t type;
    uint32_t flags;
    uint64_t row_key;
    uint32_t column_id;
    int32_t detail;
    uint64_t revision;
} DvzGuiDataEvent;

typedef struct DvzGuiTableColumnDesc
{
    uint32_t struct_size;
    uint32_t flags;
    uint32_t column_id;
    uint32_t type;
    float initial_width;
    const char* title;
    const char* format;
    uint32_t reserved[4];
} DvzGuiTableColumnDesc;

typedef struct DvzGuiDataStyle
{
    uint32_t struct_size;
    uint32_t flags;
    uint64_t row_key;
    DvzColor foreground;
    DvzColor background;
    DvzColor accent;
    uint32_t reserved[4];
} DvzGuiDataStyle;

DvzGuiTableColumnDesc dvz_gui_table_column_desc(void);
DvzGuiDataStyle dvz_gui_data_style(void);

DvzGuiTree* dvz_gui_tree(const char* widget_id, uint32_t flags);
DvzResult dvz_gui_tree_set_rows(
    DvzGuiTree* tree, uint32_t row_count, const uint64_t* keys,
    const uint32_t* parents, const char* const* labels,
    const char* const* secondary_labels, uint32_t flags);
DvzResult dvz_gui_tree_set_swatches(
    DvzGuiTree* tree, uint32_t row_count, const DvzColor* colors);
DvzResult dvz_gui_tree_set_styles(
    DvzGuiTree* tree, uint32_t style_count, const DvzGuiDataStyle* styles);
DvzResult dvz_gui_tree_set_selection(
    DvzGuiTree* tree, uint32_t key_count, const uint64_t* keys);
uint32_t dvz_gui_tree_get_selection(
    const DvzGuiTree* tree, uint32_t capacity, uint64_t* keys);
DvzResult dvz_gui_tree_set_expanded(
    DvzGuiTree* tree, uint32_t key_count, const uint64_t* keys);
uint32_t dvz_gui_tree_get_expanded(
    const DvzGuiTree* tree, uint32_t capacity, uint64_t* keys);
DvzResult dvz_gui_tree_expand_all(DvzGuiTree* tree);
DvzResult dvz_gui_tree_expand_to_depth(DvzGuiTree* tree, uint32_t depth);
DvzResult dvz_gui_tree_collapse_all(DvzGuiTree* tree);
DvzResult dvz_gui_tree_reveal(DvzGuiTree* tree, uint64_t key);
DvzResult dvz_gui_tree_set_visible(
    DvzGuiTree* tree, uint32_t row_count, const bool* visible);
DvzResult dvz_gui_tree_set_matches(
    DvzGuiTree* tree, uint32_t row_count, const bool* matches);
DvzResult dvz_gui_tree_set_filter(DvzGuiTree* tree, const char* filter);
uint32_t dvz_gui_tree_get_filter(
    const DvzGuiTree* tree, uint32_t capacity, char* filter);
DvzResult dvz_gui_tree_draw(
    DvzGui* gui, DvzGuiTree* tree, DvzGuiDataEvent* events,
    uint32_t capacity, uint32_t* written, uint32_t* dropped);
void dvz_gui_tree_destroy(DvzGuiTree* tree);

DvzGuiTable* dvz_gui_table(
    const char* widget_id, uint32_t column_count,
    const DvzGuiTableColumnDesc* columns, uint32_t flags);
DvzResult dvz_gui_table_set_rows(
    DvzGuiTable* table, uint32_t row_count, const uint64_t* keys, uint32_t flags);
DvzResult dvz_gui_table_set_column_text(
    DvzGuiTable* table, uint32_t column_id, uint32_t row_count,
    const char* const* values);
DvzResult dvz_gui_table_set_column_int64(
    DvzGuiTable* table, uint32_t column_id, uint32_t row_count,
    const int64_t* values);
DvzResult dvz_gui_table_set_column_double(
    DvzGuiTable* table, uint32_t column_id, uint32_t row_count,
    const double* values);
DvzResult dvz_gui_table_set_column_bool(
    DvzGuiTable* table, uint32_t column_id, uint32_t row_count,
    const bool* values);
DvzResult dvz_gui_table_set_column_color(
    DvzGuiTable* table, uint32_t column_id, uint32_t row_count,
    const DvzColor* values);
DvzResult dvz_gui_table_set_styles(
    DvzGuiTable* table, uint32_t style_count, const DvzGuiDataStyle* styles);
DvzResult dvz_gui_table_set_selection(
    DvzGuiTable* table, uint32_t key_count, const uint64_t* keys);
uint32_t dvz_gui_table_get_selection(
    const DvzGuiTable* table, uint32_t capacity, uint64_t* keys);
DvzResult dvz_gui_table_set_visible(
    DvzGuiTable* table, uint32_t row_count, const bool* visible);
DvzResult dvz_gui_table_set_matches(
    DvzGuiTable* table, uint32_t row_count, const bool* matches);
DvzResult dvz_gui_table_set_filter(DvzGuiTable* table, const char* filter);
uint32_t dvz_gui_table_get_filter(
    const DvzGuiTable* table, uint32_t capacity, char* filter);
DvzResult dvz_gui_table_get_sort(
    const DvzGuiTable* table, uint32_t* column_id, int32_t* direction);
DvzResult dvz_gui_table_draw(
    DvzGui* gui, DvzGuiTable* table, DvzGuiDataEvent* events,
    uint32_t capacity, uint32_t* written, uint32_t* dropped);
void dvz_gui_table_destroy(DvzGuiTable* table);
```

The header uses `set_rows`, not `rows`, for both retained objects in accordance with the public retained-state convention. Public APIs do not depend on casting unrelated opaque handle types to a common exposed base.

Count-query functions return the total available count and copy `min(capacity, total)` items. A zero-capacity query with a `NULL` destination is valid. Filter queries return the required byte count including the terminator and copy a terminated prefix when capacity is nonzero. Sort direction is negative, zero, or positive for descending, unsorted, or ascending. Table sorts are stable; integers sort by value, text uses the documented filter comparison, finite doubles sort numerically, negative infinity precedes finite values, positive infinity follows them, and NaNs follow all other values while retaining input order.

Both `set_rows()` functions accept only `DvzGuiDataSetFlags`. Without `RESET_STATE`, surviving keys preserve selection and tree expansion; with it, selection and expansion are cleared. Tree swatches and both visibility/match masks require the exact current row count. Passing zero with `NULL` clears swatches or the match mask and resets strict visibility to all rows. `set_styles()` atomically replaces the entire keyed style set, so zero styles clears every override. Unknown style, selection, expansion, or reveal keys are rejected.

## Initial v0.4 Feature Boundary

The tree includes copied primary and secondary labels, stable keys, parent validation, swatches, foreground/background/accent styles, disabled rows, selection, activation, expansion, collapse, reveal, substring filtering with ancestor reveal, application visibility, clipping, and compact events.

The table includes stable row keys, typed text/integer/double/boolean/color columns, searchable text columns, row styles, single and modifier-based row selection, activation, one-column sorting, column resizing, filtering, clipping, and compact events.

Cell editing, cell selection, arbitrary action cells, context menus, drag and drop, lazy branches, tri-state checkboxes, multiple-column sorting, frozen/reorderable/hideable columns, and secondary table columns inside the tree are deferred. The low-level cimgui escape hatch remains available for application-specific interfaces.

## Validation

Initial CPU tests cover invalid and duplicate keys, invalid parent order, copied model state,
replacement with preserved stable-key state, filtering and ancestor reveal, visibility and match masks,
and deterministic typed sorting. Interaction-level tests for event overflow and revisions, clipped large
widgets, modifier selection, activation, expansion, style precedence, and multiple widgets remain
follow-up coverage; the deterministic gallery example provides an integrated rendering smoke in the
meantime.

Python facade tests must prove that input sequences are encoded only during setters, `draw()` performs
one ctypes call regardless of row count, event buffers report written and dropped counts correctly,
and labels are not re-encoded during drawing.

One deterministic `features_gui_data_widgets` gallery example presents a colored hierarchy and a typed table together. It has a Python adaptation, an honest native-only WebGPU classification because Dear ImGui is not part of the scene/WASM route, generated API and gallery pages, and a temporary reviewed screenshot candidate. Full example completion requires later explicit approval and promotion of the exact canonical PNG through the protected `data` workflow.
